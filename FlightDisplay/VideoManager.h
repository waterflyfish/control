/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#ifndef VideoManager_H
#define VideoManager_H

#include <QObject>
#include <QTimer>

#include "QGCLoggingCategory.h"
#include "VideoSurface.h"
#include "QThread"
#include "VideoReceiver.h"
#include "QGCToolbox.h"
#include "CarInfo.h"
#include "Ftp.h"
#include "QVector"
#include "QJSValue"

Q_DECLARE_LOGGING_CATEGORY(VideoManagerLog)

class VideoManager : public QGCTool
{
    Q_OBJECT

public:
    VideoManager    (QGCApplication* app);
    ~VideoManager   ();

    Q_PROPERTY(bool             hasVideo        READ    hasVideo                                NOTIFY hasVideoChanged)
    Q_PROPERTY(bool             isGStreamer     READ    isGStreamer                             NOTIFY isGStreamerChanged)
    Q_PROPERTY(QString          videoSourceID   READ    videoSourceID                           NOTIFY videoSourceIDChanged)
    Q_PROPERTY(QString          videoSource     READ    videoSource     WRITE setVideoSource    NOTIFY videoSourceChanged)
    Q_PROPERTY(QStringList      videoSourceList READ    videoSourceList                         NOTIFY videoSourceListChanged)
    Q_PROPERTY(bool             videoRunning    READ    videoRunning                            NOTIFY videoRunningChanged)
    Q_PROPERTY(quint16          udpPort         READ    udpPort         WRITE setUdpPort        NOTIFY udpPortChanged)
    Q_PROPERTY(QString          rtspURL         READ    rtspURL         WRITE setRtspURL        NOTIFY rtspURLChanged)

    Q_PROPERTY(bool             uvcEnabled      READ    uvcEnabled                              CONSTANT)
    Q_PROPERTY(VideoSurface*    videoSurface    MEMBER  _videoSurface                           CONSTANT)
    Q_PROPERTY(VideoReceiver*   videoReceiver   MEMBER  _videoReceiver                          CONSTANT)
    //===========================================================================
    Q_PROPERTY(bool             saveVideo     READ    saveVideo     WRITE setSaveVideo    NOTIFY saveVideoChanged)
    Q_PROPERTY(bool             saveImage     READ    saveImage     WRITE setSaveImage    NOTIFY saveImageChanged)
    Q_PROPERTY(bool             carInfo     READ    carInfo     WRITE setCarInfo    NOTIFY carInfoChanged)
    Q_PROPERTY(bool             updateInfo     READ    updateInfo     WRITE setUpdateInfo    NOTIFY updateInfoChanged)
    Q_PROPERTY(bool             videoChoice     READ    videoChoice     WRITE setVideoChoice    NOTIFY videoChoiceChanged)
    Q_PROPERTY(QString          videoLatency         READ    videoLatency         WRITE setVideoLatency        NOTIFY videoLatencyChanged)
    Q_PROPERTY(QString          inf     READ    inf     WRITE setInf    NOTIFY infChanged)
    Q_PROPERTY(QString          updateInf     READ    updateInf     WRITE setUpdateInf    NOTIFY updateInfChanged)
    Q_PROPERTY(bool             trackFlag     READ    trackFlag     WRITE setTrackFlag    NOTIFY trackFlagChanged)
    //==================================================================
    Q_INVOKABLE float getProcess                 ();
    Q_INVOKABLE void setLocation                 (QVariantList list);
    bool        hasVideo            ();
    bool        isGStreamer         ();
    bool        videoRunning        () { return _videoRunning; }
    QString     videoSourceID       () { return _videoSourceID; }
    QString     inf                 () { return _inf;}
    QString     updateInf           () {return _updateInf;}
    QString     videoSource         () { return _videoSource; }
    QStringList videoSourceList     ();
    quint16     udpPort             () { return _udpPort; }
    QString     rtspURL             () { return _rtspURL; }
    QString     videoLatency        () { return _videoLatency;}
#if defined(QGC_DISABLE_UVC)
    bool        uvcEnabled          () { return false; }
#else
    bool        uvcEnabled          ();
#endif

    void        setVideoSource      (QString vSource);
    void        setInf              (QString inf);
    void        setUpdateInf        (QString updateInf);
    void        setUdpPort          (quint16 port);
    void        setRtspURL          (QString url);
    void        setVideoLatency     (QString videoLatency);
    // Override from QGCTool
    void        setToolbox          (QGCToolbox *toolbox);

    //=======================================
     bool        saveVideo();
     bool        saveImage();
     bool        carInfo();
     bool        updateInfo();
     bool        videoChoice();
     bool        trackFlag();
    //=====================================
    //=======================================
     void        setSaveVideo(bool saveVideo);
     void        setSaveImage(bool saveImage);
     void        setCarInfo(bool carInfo);
     void        setUpdateInfo(bool updateInfo);
     void        setVideoChoice(bool videoChoice);
     void        setTrackFlag(bool trackFlag);
    //====================================

    void        getRecordVideoName();

signals:
    void hasVideoChanged        ();
    void videoRunningChanged    ();
    void videoSourceChanged     ();
    void infChanged             ();
    void updateInfChanged       ();
    void videoSourceListChanged ();
    void isGStreamerChanged     ();
    void videoSourceIDChanged   ();
    void udpPortChanged         ();
    void rtspURLChanged         ();
    void videoLatencyChanged    ();
    //============================
     void saveVideoChanged     ();
     void saveImageChanged     ();
     void carInfoChanged       ();
     void updateInfoChanged    ();
     void videoChoiceChanged    ();
     void trackFlagChanged      ();
     void change(bool change);
    //================================

private:
    void _updateTimer           ();
    void _updateVideo           ();
#if defined __android__ || defined __mobile__
    void cleanJavaException();
#endif

private:
    VideoSurface*       _videoSurface;
    VideoReceiver*      _videoReceiver;
    bool                _videoRunning;
#if defined(QGC_GST_STREAMING)
    QTimer              _frameTimer;
#endif
    QString             _videoSource;
    QString             _inf;
    QString             _updateInf;
    QString             _videoSourceID;
    QStringList         _videoSourceList;
    quint16             _udpPort;
    QString             _rtspURL;
    QString             _videoLatency;
    bool                _init;
    bool                _recordVideo;    ///<< true : save receive video to a file; false;
    bool                _recordImage;
   //=============================
    bool                _saveVideo;
    bool                _saveImage;
    bool                _carInfo;
    bool                _updateInfo;
    bool                _videoChoice;
    QVariantList           _location;
    bool                _trackFlag;
    //===========================
    QString             _recordVideoDir;
    QString             _recordPictureDir;
    int                        _counter;
    CarInfo* car;
    Ftp *ftp;
    QThread* thread;
    QFile* file;
};

#endif
