/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef LogReplayLink_H
#define LogReplayLink_H

#include "LinkInterface.h"
#include "LinkConfiguration.h"
#include "MAVLinkProtocol.h"

#include <QTimer>
#include <QFile>

class LogReplayLinkConfiguration : public LinkConfiguration
{
    Q_OBJECT

public:

    Q_PROPERTY(QString  fileName    READ logFilename    WRITE setLogFilename    NOTIFY fileNameChanged)

    LogReplayLinkConfiguration(const QString& name);
    LogReplayLinkConfiguration(LogReplayLinkConfiguration* copy);

    QString logFilename(void) { return _logFilename; }
    void setLogFilename(const QString logFilename) { _logFilename = logFilename; emit fileNameChanged(); }

    QString logFilenameShort(void);

    // Virtuals from LinkConfiguration
    LinkType    type                    () { return LinkConfiguration::TypeLogReplay; }
    void        copyFrom                (LinkConfiguration* source);
    void        loadSettings            (QSettings& settings, const QString& root);
    void        saveSettings            (QSettings& settings, const QString& root);
    void        updateSettings          ();
    bool        isAutoConnectAllowed    () { return false; }
    QString     settingsURL             () { return "LogReplaySettings.qml"; }
signals:
    void fileNameChanged();

private:
    static const char*  _logFilenameKey;
    QString             _logFilename;
};

class LogReplayLink : public LinkInterface
{
    Q_OBJECT

    friend class LinkManager;

public:
    virtual LinkConfiguration* getLinkConfiguration() { return _config; }

    /// @return true: log is currently playing, false: log playback is paused
    bool isPlaying(void) { return _readTickTimer.isActive(); }

    /// Start replay at current position
    void play(void) { emit _playOnThread(); }

    /// Pause replay
    void pause(void) { emit _pauseOnThread(); }

    /// Move the playhead to the specified percent complete
    void movePlayhead(int percentComplete);

    /// Sets the acceleration factor: -100: 0.01X, 0: 1.0X, 100: 100.0X
    void setAccelerationFactor(int factor) { emit _setAccelerationFactorOnThread(factor); }

    // Virtuals from LinkInterface
    virtual QString getName(void) const { return _config->name(); }
    virtual void requestReset(void){ }
    virtual bool isConnected(void) const { return _connected; }
    virtual qint64 getConnectionSpeed(void) const { return 100000000; }
    virtual qint64 bytesAvailable(void) { return 0; }
    virtual bool isLogReplay(void) { return true; }

    // These are left unimplemented in order to cause linker errors which indicate incorrect usage of
    // connect/disconnect on link directly. All connect/disconnect calls should be made through LinkManager.
    bool connect(void);
    bool disconnect(void);

private slots:
    virtual void _writeBytes(const QByteArray bytes);

signals:
    void logFileStats(bool logTimestamped, int logDurationSecs, int binaryBaudRate);
    void playbackStarted(void);
    void playbackPaused(void);
    void playbackAtEnd(void);
    void playbackError(void);
    /* Changed by chu.fumin 2016121313 start : replay log file */
    //void playbackPercentCompleteChanged(int percentComplete); 
    void playbackPercentCompleteChanged(double percentComplete);
    /* 2016121313 end : replay log file */

    // Internal signals
    void _playOnThread(void);
    void _pauseOnThread(void);
    void _setAccelerationFactorOnThread(int factor);

private slots:
    void _readNextLogEntry(void);
    void _play(void);
    void _pause(void);
    void _setAccelerationFactor(int factor);

private:
    // Links are only created/destroyed by LinkManager so constructor/destructor is not public
    LogReplayLink(LogReplayLinkConfiguration* config);
    ~LogReplayLink();

    void _replayError(const QString& errorMsg);
    quint64 _parseTimestamp(const QByteArray& bytes);
    quint64 _seekToNextMavlinkMessage(mavlink_message_t* nextMsg);
    bool _loadLogFile(void);
    void _finishPlayback(void);
    void _playbackError(void);
    void _resetPlaybackToBeginning(void);

    /* Changed by chu.fumin 2016121313 start : replay log file : replay log file */
    /// @brief Read byte from log file, convert to mavlink message and return a BigEndian quint64 timestamp
    ///    @param file : log file
    ///    @param msg  : mavlink message
    /// @return A Unix timestamp in microseconds UTC for found message or 0 if parsing failed
    quint64 _readMessageFromFile(QFile &file, mavlink_message_t *msg);

    /// @brief Read byte from log file, convert to mavlink message and send to the vehicle
    ///    @param pos : the position of message in log file
    void _respondWithMavlinkMessage(quint64 pos);

    /// @brief Sends the list of parameter to the vehicle
    ///    @param param : the list of the vehicle param
    void _paramRequestListWorker(const QMap<uint32_t, quint64> &param);

    /// @brief Sends the list of mission to the vehicle
    ///    @param mission : the list of the vehicle mission
    void _missionRequestListWorker(const QMap<uint32_t, quint64> &mission);

    ///  @brief Called to handle vehicle messages. All messages should be passed to this method.
    ///         It will handle the appropriate set.
    ///    @param msg : the received message
    void _handleMessage (const mavlink_message_t *msg);

    /// @brief Called to handle the messages read from log file. All messages should be passed to this method.
    ///         It will handle the appropriate set.
    ///    @param msg : the message parsed
    ///    @param pos : the position of log file
    void _handleLogMessage(mavlink_message_t *msg, quint64 pos);
    /* 2016121313 end : replay log file */

    // Virtuals from LinkInterface
    virtual bool _connect(void);
    virtual void _disconnect(void);

    // Virtuals from QThread
    virtual void run(void);

    LogReplayLinkConfiguration* _config;

    bool    _connected;     
    /* Changed by chu.fumin 2016121313 start : replay log file */
    bool    _start; 
    /* 2016121313 end : replay log file */
    QTimer _readTickTimer;      ///< Timer which signals a read of next log record

    static const char* _errorTitle; ///< Title for communicatorError signals

    quint64 _logCurrentTimeUSecs;   ///< The timestamp of the next message in the log file.
    quint64 _logStartTimeUSecs;     ///< The first timestamp in the current log file.
    quint64 _logEndTimeUSecs;       ///< The last timestamp in the current log file.
    quint64 _logDurationUSecs;

    static const int    _defaultBinaryBaudRate = 57600;
    int                 _binaryBaudRate;        ///< Playback rate for binary log format

    float   _replayAccelerationFactor;  ///< Factor to apply to playback rate
    quint64 _playbackStartTimeMSecs;    ///< The time when the logfile was first played back. This is used to pace out replaying the messages to fix long-term drift/skew. 0 indicates that the player hasn't initiated playback of this log file.

    MAVLinkProtocol*    _mavlink;
    QFile               _logFile;
    quint64             _logFileSize;
    bool                _logTimestamped;    ///< true: Timestamped log format, false: no timestamps

    /* Changed by chu.fumin 2016121313 start : replay log file */
    QFile               _logRespondFile;
    int                 _paraSize;          ///< the number of param
    QMap<uint32_t, QMap<uint32_t, quint64> > _logParameter;
    QMap<uint32_t, QMap<uint32_t, quint64> > _logMission;
    QMap<uint32_t, QMap<uint32_t, quint64> > _logMissionCount;
    /* 2016121313 end : replay log file */

    static const int cbTimestamp = sizeof(quint64);
    /* Changed by chu.fumin 2016121313 start : replay log file */
    static const int cbMavlink = sizeof(mavlink_message_t);
    /* 2016121313 end : replay log file */
};

#endif
