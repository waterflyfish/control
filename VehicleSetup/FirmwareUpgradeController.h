/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

#ifndef FirmwareUpgradeController_H
#define FirmwareUpgradeController_H

#include "PX4FirmwareUpgradeThread.h"
#include "LinkManager.h"
#include "FirmwareImage.h"
#include "FileManager.h"
#include "ConsoleUpgradeController.h"
#include "RaspBerryUpgradeController.h"
#include "UAS.h"
#include "TEA.h"

#include <QObject>
#include <QUrl>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>
#include <QQuickItem>
#include <QDomDocument>
#include <QXmlStreamReader>
#include <QJsonArray>
#include <QDebug>

#include "qextserialport.h"

#include <stdint.h>
#include <iostream>
using namespace std;

/// Supported firmware types. If you modify these you will need to update the qml file as well.

// Firmware Upgrade MVC Controller for FirmwareUpgrade.qml.
class FirmwareUpgradeController : public QObject
{
    Q_OBJECT
    
public:
    typedef enum {
        AutoPilotStackPX4,
        AutoPilotStackAPM,
        PX4Flow,
        ThreeDRRadio,
        AerialVehicle = 100
    } AutoPilotStackType_t;

    typedef enum {
        StableFirmware,
        BetaFirmware,
        DeveloperFirmware,
        CustomFirmware
    } FirmwareType_t;

    typedef enum {
        QuadFirmware,
        X8Firmware,
        HexaFirmware,
        OctoFirmware,
        YFirmware,
        Y6Firmware,
        HeliFirmware,
        PlaneFirmware,
        RoverFirmware,
        DefaultVehicleFirmware
    } FirmwareVehicleType_t;

    typedef enum {
        Board,
        RemoteControl,
        GPS,
        Battery,
        Gimbal,
        ESpeedControl,
        RaspBerry
    } SelectedFirmwareType_t;

    Q_ENUMS(AutoPilotStackType_t)
    Q_ENUMS(FirmwareType_t)
    Q_ENUMS(FirmwareVehicleType_t)
    Q_ENUMS(SelectedFirmwareType_t)

    class FirmwareIdentifier
    {
    public:
        FirmwareIdentifier(AutoPilotStackType_t stack = AutoPilotStackPX4,
                           FirmwareType_t firmware = StableFirmware,
                           FirmwareVehicleType_t vehicle = DefaultVehicleFirmware,
                           SelectedFirmwareType_t selectedFirmware = Board,
                           QString firmwareVersion = "",
                           QString url = "")
            : autopilotStackType(stack), firmwareType(firmware), firmwareVehicleType(vehicle),
              selectedFirmware(selectedFirmware),firmwareVersion(firmwareVersion),url(url) {}

        bool operator==(const FirmwareIdentifier& firmwareId) const
        {
            return (firmwareId.autopilotStackType == autopilotStackType &&
                    firmwareId.firmwareType == firmwareType &&
                    firmwareId.firmwareVehicleType == firmwareVehicleType &&
                    firmwareId.selectedFirmware == selectedFirmware &&
                    firmwareId.firmwareVersion == firmwareVersion);
        }

        // members
        AutoPilotStackType_t    autopilotStackType;
        FirmwareType_t          firmwareType;
        FirmwareVehicleType_t   firmwareVehicleType;
        SelectedFirmwareType_t  selectedFirmware;
        QString                 firmwareVersion;
        QString                 url;
    };

    FirmwareUpgradeController(void);
    ~FirmwareUpgradeController();

    Q_PROPERTY(QString          boardPort                   READ boardPort                                              NOTIFY boardFound)
    Q_PROPERTY(QString          boardDescription            READ boardDescription                                       NOTIFY boardFound)
    Q_PROPERTY(QString          boardType                   MEMBER _foundBoardTypeName                                  NOTIFY boardFound)
    Q_PROPERTY(bool             androidType                 MEMBER _androidType                                                          )
    //Q_PROPERTY(FirmwareType_t   selectedFirmwareType        READ selectedFirmwareType   WRITE setSelectedFirmwareType   NOTIFY selectedFirmwareTypeChanged)

    /// TextArea for log output
    Q_PROPERTY(QQuickItem* statusLog READ statusLog WRITE setStatusLog)
    
    /// Progress bar for you know what
    Q_PROPERTY(QQuickItem* progressBar READ progressBar WRITE setProgressBar)

    /// Starts searching for boards on the background thread
    Q_INVOKABLE void startBoardSearch(void);
    Q_INVOKABLE void startBoardSearchInfo(void);

    
    /// Cancels whatever state the upgrade worker thread is in
    Q_INVOKABLE void cancel(void);
    
    /// Called when the firmware type has been selected by the user to continue the flash process.
    Q_INVOKABLE void flash();
    Q_INVOKABLE void flash(const FirmwareIdentifier& firmwareId);
    Q_INVOKABLE void flash(AutoPilotStackType_t stackType,
                           FirmwareType_t firmwareType = StableFirmware,
                           FirmwareVehicleType_t vehicleType = DefaultVehicleFirmware,
                           QString vehicleVersion = "");

    Q_INVOKABLE FirmwareVehicleType_t vehicleTypeFromVersionIndex(int selected, int index){ Q_UNUSED(selected);Q_UNUSED(index);return DefaultVehicleFirmware;}

    /// Download file( firmware version config)
    void loadFirmwareVersions();
    /// Called when the file( firmware version config) download finished
    void firmwareVersionDownloadFinished(QString remoteFile, QString localFile);

    /// Returns firmware type as a string
    QString firmwareTypeAsString(FirmwareType_t type) const;
    /// Get file from localhost
    Q_INVOKABLE QString getLocalFile(
        #ifdef __android__
            QString downloadFirmwareFilename,
        #endif
            int board);
    /// Show the selected firmware to the user by ui
    Q_INVOKABLE void setSelectedFirmwareType(FirmwareType_t firmwareType, QList<int> board);

    /// Called when the firmware type has been selected by the user to set selected firmware.
    Q_INVOKABLE void setSelectedFirmware(QList<int> selectedFirmwareType, QList<int> currentIndex);

    /// Called when the firmware type has been selected by the user to upgrade firmware.
    Q_INVOKABLE void usbSerialDownloadFirmware();

    // Property accessors
    QQuickItem* progressBar(void) { return _progressBar; }
    void setProgressBar(QQuickItem* progressBar) { _progressBar = progressBar; }

    QQuickItem* statusLog(void) { return _statusLog; }
    void setStatusLog(QQuickItem* statusLog) { _statusLog = statusLog; }

    QString boardPort(void) { return _foundBoardInfo.portName(); }
    QString boardDescription(void) { return _foundBoardInfo.description(); }

    FirmwareType_t selectedFirmwareType(void) { return _selectedFirmwareType; }

    /// < px4FlightStack
    Q_PROPERTY(QStringList      px4FlightStackVersionsMap       READ px4AvailableVersions                           NOTIFY px4AvailableVersionsChanged)
    Q_PROPERTY(QString      px4FlightStackNew        MEMBER _px4FlightStackNew                             NOTIFY firmwareAvailableVersionsChanged)
    Q_PROPERTY(QString      px4FlightStackBoard        MEMBER _px4FlightStackBoard                         NOTIFY boardAvailableVersionsChanged)

    /// < apmFlightStack
    Q_PROPERTY(QStringList      apmFlightStackVersionsMap       READ apmAvailableVersions                           NOTIFY apmAvailableVersionsChanged)
    Q_PROPERTY(QString      apmFlightStackNew        MEMBER _apmFlightStackNew                             NOTIFY firmwareAvailableVersionsChanged)
    Q_PROPERTY(QString      apmFlightStackBoard        MEMBER _apmFlightStackBoard                         NOTIFY boardAvailableVersionsChanged)

    /// < gps
    Q_PROPERTY(QStringList      gpsAvailableVersions       READ gpsAvailableVersions                           NOTIFY gpsAvailableVersionsChanged)
    Q_PROPERTY(QString      gpsNew        MEMBER _gpsNew                                                   NOTIFY firmwareAvailableVersionsChanged)
    Q_PROPERTY(QString      gpsBoard        MEMBER _gpsBoard                                               NOTIFY boardAvailableVersionsChanged)

    /// < gimbals
    Q_PROPERTY(QStringList      gimbalsVersions        READ gimbalAvailableVersions                         NOTIFY gimbalAvailableVersionsChanged)
    Q_PROPERTY(QString      gimbalsNew        MEMBER _gimbalNew                                           NOTIFY firmwareAvailableVersionsChanged)
    Q_PROPERTY(QString      gimbalsBoard        MEMBER _gimbalBoard                                       NOTIFY boardAvailableVersionsChanged)

    /// < Battery
    Q_PROPERTY(QStringList      smartBatteryVersions        READ batteryAvailableVersions                 NOTIFY batteryAvailableVersionsChanged)
    Q_PROPERTY(QString      smartBatteryNew        MEMBER _batteryNew                                 NOTIFY firmwareAvailableVersionsChanged)
    Q_PROPERTY(QString      smartBatteryBoard        MEMBER _batteryBoard                              NOTIFY boardAvailableVersionsChanged)

    /// < smartConsole
    Q_PROPERTY(QStringList      smartConsoleVersions       READ remoteControlAvailableVersions                 NOTIFY consoleAvailableVersionsChanged)
    Q_PROPERTY(QString      smartConsoleNew        MEMBER _smartConsoleNew                                  NOTIFY firmwareAvailableVersionsChanged)
    Q_PROPERTY(QString      smartConsoleBoard        MEMBER _smartConsoleBoard                              NOTIFY boardAvailableVersionsChanged)

    /// < eSpeedControl
    Q_PROPERTY(QStringList      BrushlessFOCVersions       READ eSpeedControlAvailableVersions                 NOTIFY escAvailableVersionsChanged)
    Q_PROPERTY(QString      BrushlessFOCNew        MEMBER _eSpeedControlNew                                  NOTIFY firmwareAvailableVersionsChanged)
    Q_PROPERTY(QString      BrushlessFOCBoard        MEMBER _eSpeedControlBoard                              NOTIFY boardAvailableVersionsChanged)

    /// < RaspBerry
    Q_PROPERTY(QStringList      RaspBerryVersions       READ raspBerryAvailableVersions                NOTIFY raspAvailableVersionsChanged)
    Q_PROPERTY(QString      RaspBerryNew        MEMBER _raspBerryNew                                  NOTIFY firmwareAvailableVersionsChanged)
    Q_PROPERTY(QString      RaspBerryBoard        MEMBER _raspBerryBoard                              NOTIFY boardAvailableVersionsChanged)

signals:
    void boardFound(void);
    void noBoardFound(void);
    void boardGone(void);
    void flashComplete(void);
    void flashCancelled(void);
    void error(void);
    void selectedFirmwareTypeChanged(FirmwareType_t firmwareType);
    void firmwareAvailableVersionsChanged();
    void px4AvailableVersionsChanged();
    void apmAvailableVersionsChanged();
    void gpsAvailableVersionsChanged();
    void gimbalAvailableVersionsChanged();
    void batteryAvailableVersionsChanged();
    void consoleAvailableVersionsChanged();
    void escAvailableVersionsChanged();
    void raspAvailableVersionsChanged();
    void boardAvailableVersionsChanged(void);
    void usbSerialFound(void);
    void usbSerialUpgrade(void);
    void getVersion(QString version, bool board);
    void boardSearchInfoSignal();

private slots:
    void _downloadProgress(qint64 curr, qint64 total);
    void _downloadFinished(void);
    void _downloadError(QNetworkReply::NetworkError code);
    void _foundBoard(bool firstAttempt, const QSerialPortInfo& portInfo, int boardType);
    void _noBoardFound(void);
    void _boardGone();
    void _foundBootloader(int bootloaderVersion, int boardID, int flashSize);
    void _error(const QString& errorString);
    void _status(const QString& statusString);
    void _bootloaderSyncFailed(void);
    void _flashComplete(void);
    void _updateProgress(int curr, int total);
    void _eraseStarted(void);
    void _eraseComplete(void);
    void _eraseProgressTick(void);
    void _upgradeFirmware(const QSerialPortInfo &portInfo, int type);
    /// Show firmware version, get version from machine
    void _setFirmwareVersion(QString version, bool board);
    void _appendStatusLog(const QString& text, bool critical = false);
    /// Get the flight control serial port information
    void _boardSearchInfo();

private:
    void _getFirmwareFile(FirmwareIdentifier firmwareId);
    void _initFirmwareHash();
    void _downloadFirmware(QString downloadFirmwareFilename);
    ///< Search firmware version.
    bool _getBoardFirmwareVersion(void);
    bool _initFileManager(void);
    void _boardFirmareAvailableVersions(const QString &version);
    void _nextFirmwareUpgrade(bool sucess = false);
    FileManager* getFileManager();

    //void _appendStatusLog(const QString& text, bool critical = false);
    void _errorCancel(const QString& msg);
    QList<FirmwareIdentifier>* _firmwareHashForBoardId(int boardId);
    QList<FirmwareIdentifier>* _firmwareHashForSelectedBoardId(int selectedFirmware);
    QList<FirmwareIdentifier>* _firmwareHashForBoardType(QGCSerialPortInfo::BoardType_t boardType);

    QString _portName;
    QString _portDescription;

    // firmware hashes
    QList<FirmwareIdentifier> _rgPX4FMUV4Firmware;
    QList<FirmwareIdentifier> _rgPX4FMUV2Firmware;
    QList<FirmwareIdentifier> _rgAeroCoreFirmware;
    QList<FirmwareIdentifier> _rgPX4FMUV1Firmware;
    QList<FirmwareIdentifier> _rgMindPXFMUV2Firmware;
    QList<FirmwareIdentifier> _rgPX4FLowFirmware;
    QList<FirmwareIdentifier> _rg3DRRadioFirmware;
    QList<FirmwareIdentifier> _rgRemoteControlFirmware;
    QList<FirmwareIdentifier> _rgGPSFirmware;
    QList<FirmwareIdentifier> _rgBatteryFirmware;
    QList<FirmwareIdentifier> _rgGimbalFirmware;
    QList<FirmwareIdentifier> _rgESpeedControlFirmware;
    QList<FirmwareIdentifier> _rgRaspBerryFirmware;

    /// Information which comes back from the bootloader
    bool        _bootloaderFound;           ///< true: we have received the foundBootloader signals
    uint32_t    _bootloaderVersion;         ///< Bootloader version
    uint32_t    _bootloaderBoardID;         ///< Board ID
    uint32_t    _bootloaderBoardFlashSize;  ///< Flash size in bytes of board
    
    bool                 _startFlashWhenBootloaderFound;
    FirmwareIdentifier   _startFlashWhenBootloaderFoundFirmwareIdentity;

    QPixmap _boardIcon;             ///< Icon used to display image of board
    
    QString _firmwareFilename;      ///< Image which we are going to flash to the board
    
    QNetworkAccessManager*  _downloadManager;       ///< Used for firmware file downloading across the internet
    QNetworkReply*          _downloadNetworkReply;  ///< Used for firmware file downloading across the internet
    QString                 _downloadFirmwareFilename;
    
    /// @brief Thread controller which is used to run bootloader commands on seperate thread
    PX4FirmwareUpgradeThreadController* _threadController;

    // one for get version from APM, some for get version from internet
    QGCSerialPortInfo::BoardType_t  _remoteBoardType;      // board type on the other side of the wireless connection
    
    static const int    _eraseTickMsec = 500;       ///< Progress bar update tick time for erase
    static const int    _eraseTotalMsec = 15000;    ///< Estimated amount of time erase takes
    int                 _eraseTickCount;            ///< Number of ticks for erase progress update
    QTimer              _eraseTimer;                ///< Timer used to update progress bar for erase

    static const int    _findBoardTimeoutMsec = 30000;      ///< Amount of time for user to plug in USB
    static const int    _findBootloaderTimeoutMsec = 5000;  ///< Amount time to look for bootloader
    
    QQuickItem*     _statusLog;         ///< Status log TextArea Qml control
    QQuickItem*     _progressBar;
    
    bool _searchingForBoard;    ///< true: searching for board, false: search for bootloader
    
    QSerialPortInfo                 _foundBoardInfo;
    QGCSerialPortInfo::BoardType_t  _foundBoardType;
    QGCSerialPortInfo::BoardType_t  _BoardType;
    QString                         _foundBoardTypeName;
    bool                            _androidType;
    FirmwareType_t                  _selectedFirmwareType;
    FirmwareImage*                  _image;
    FileManager*                    _fileManager;
    QString                         _NewVersion;
    QString                         _BoardVersion;
    ConsoleUpgradeController *      _consoleUpgradeController;   ///< smartconsole controller, upgrade firmware
    RaspBerryUpgradeController *    _raspBerryUpgradeController;
    QList<FirmwareIdentifier>       _selectedFirmware;           ///< to update the firmware list
    bool                            _firmwareVersion;            ///< true: search version from device, false: do nothing
    QList<SelectedFirmwareType_t>   _firmwareVersionList;        ///< list: search version from device
    volatile int                    _deviceVersionCount;         ///< search version from device, number of devices already completed
    QTimer                          _boardInfoTimerRetry;        ///< search board information timer
    int                             _tryTimes;                   ///< maximum number of times to find board information

    /// < px4FlightStack
    QList<FirmwareIdentifier> _px4FlightStackList;
    QStringList _px4AvailableVersions;
    QString _px4FlightStackNew;
    QString _px4FlightStackBoard;

    /// < apmFlightStack
    QList<FirmwareIdentifier> _apmFlightStackList;
    QStringList _apmAvailableVersions;
    QString _apmFlightStackNew;
    QString _apmFlightStackBoard;

    /// < gps
    QList<FirmwareIdentifier> _gpsList;
    QStringList     _gpsAvailableVersions;
    QString _gpsNew;
    QString _gpsBoard;

    /// < gimbals
    QList<FirmwareIdentifier> _gimbalList;
    QStringList     _gimbalAvailableVersions;
    QString _gimbalNew;
    QString _gimbalBoard;

    /// < Battery
    QList<FirmwareIdentifier> _batteryList;
    QStringList     _batteryAvailableVersions;
    QString _batteryNew;
    QString _batteryBoard;

    /// < smartConsole
    QList<FirmwareIdentifier> _smartConsoleControlList;
    QStringList     _smartConsoleAvailableVersions;
    QString _smartConsoleNew;
    QString _smartConsoleBoard;

    /// < eSpeedControl
    QList<FirmwareIdentifier> _eSpeedControlList;
    QStringList     _eSpeedControlAvailableVersions;
    QString _eSpeedControlNew;
    QString _eSpeedControlBoard;

    /// < RaspBerry
    QList<FirmwareIdentifier> _raspBerryList;
    QStringList     _raspBerryAvailableVersions;
    QString _raspBerryNew;
    QString _raspBerryBoard;

    ///  -----------------------------------------------------------
    /// function for parse and show firmware version
private:
    /// < px4FlightStack
    void setPx4AvailableVersion(){
        if(_selectedFirmwareType == CustomFirmware){
            _px4AvailableVersions = customFirmwareAvailable(_px4FlightStackList, AutoPilotStackPX4, Board);
        }else{
            _px4AvailableVersions = apmPx4FirmwareAvailableVersions(_px4FlightStackList, true);
        }
        emit px4AvailableVersionsChanged();
    }
    QStringList px4AvailableVersions(){
        return _px4AvailableVersions;
    }
    QStringList apmPx4FirmwareAvailableVersions(QList<FirmwareIdentifier> &flightStackList, bool px4 = true){
        QList<FirmwareIdentifier> *px4List = NULL;
        switch (_BoardType) {
        case QGCSerialPortInfo::BoardTypePX4FMUV1:
            px4List = &_rgPX4FMUV1Firmware;
            break;
        case QGCSerialPortInfo::BoardTypePX4FMUV2:
            px4List = &_rgPX4FMUV2Firmware;
            break;
        case QGCSerialPortInfo::BoardTypePX4FMUV4:
            px4List = &_rgPX4FMUV4Firmware;
            break;
        case QGCSerialPortInfo::BoardTypeAeroCore:
            px4List = &_rgPX4FLowFirmware;
            break;
        case QGCSerialPortInfo::BoardTypeMINDPXFMUV2:
            px4List = &_rgMindPXFMUV2Firmware;
            break;
        case QGCSerialPortInfo::BoardTypePX4Flow:
            px4List = &_rgPX4FLowFirmware;
            break;
        case QGCSerialPortInfo::BoardTypeSikRadio:
        default:
            px4List = &_rgPX4FMUV4Firmware;
            break;
        }
        if(!px4List){
            return QStringList();
        }
        QList<FirmwareIdentifier> apmPx4;
        if(px4){
            foreach(FirmwareIdentifier firmwareId, *px4List){
                if(firmwareId.autopilotStackType ==  AutoPilotStackPX4 || firmwareId.autopilotStackType ==  PX4Flow){
                    apmPx4.append (firmwareId);
                }
            }
        }
        if(!px4){
            foreach(FirmwareIdentifier firmwareId, *px4List){
                if(firmwareId.autopilotStackType == AutoPilotStackAPM){
                    apmPx4.append (firmwareId);
                }
            }
        }
        return firmwareAvailableVersions(flightStackList, apmPx4);
    }
    QStringList firmwareAvailableVersions(QList<FirmwareIdentifier> &flightStackList, QList<FirmwareIdentifier> &firmwareIdentifier){
        QStringList strList;
        strList.clear ();
        flightStackList.clear();
        if(firmwareIdentifier.size () == 0){
            return strList;
        }

        foreach(FirmwareIdentifier firmwareId, firmwareIdentifier){
            if(firmwareId.firmwareType == _selectedFirmwareType){
                flightStackList<<firmwareId;
                strList<<firmwareId.firmwareVersion;
            }
        }
        return strList;
    }

    QStringList customFirmwareAvailable(QList<FirmwareIdentifier> &flightStackList,
                                        AutoPilotStackType_t autopilotStackType,
                                        SelectedFirmwareType_t selectedFirmware){
        QStringList strList;
        if(_selectedFirmwareType == CustomFirmware){
            FirmwareIdentifier firmwareId(autopilotStackType,
                                          CustomFirmware,DefaultVehicleFirmware,
                                          selectedFirmware,"CustomFirmware");
            qDebug()<<_downloadFirmwareFilename;
            if(_downloadFirmwareFilename.isEmpty ()){
                strList<<"CustomFirmware";
                firmwareId.firmwareVersion = "CustomFirmware";
            }
            else{
                strList<<_downloadFirmwareFilename;
                firmwareId.firmwareVersion = _downloadFirmwareFilename;
                //_downloadFirmwareFilename.clear ();
            }
            _downloadFirmwareFilename.clear ();
            flightStackList.clear();
            flightStackList<<firmwareId;
            return strList;
        }
        return strList;
    }

    /// < apmFlightStack
    void setApmAvailableVersions(){
        if(_selectedFirmwareType == CustomFirmware){
            _apmAvailableVersions = customFirmwareAvailable(_apmFlightStackList, AutoPilotStackAPM, Board);
            emit apmAvailableVersionsChanged();
            return ;
        }
        QStringList apmList, apmAvailableVersions;
        apmList = apmPx4FirmwareAvailableVersions(_apmFlightStackList, false);
        foreach (FirmwareIdentifier vehicle, _apmFlightStackList) {
            FirmwareVehicleType_t vehicleType = vehicle.firmwareVehicleType;
            QString version;
            switch (vehicleType) {
            case QuadFirmware:
                version = "Quad - ";
                break;
            case X8Firmware:
                version = "X8 - ";
                break;
            case HexaFirmware:
                version = "Hexa - ";
                break;
            case OctoFirmware:
                version = "Octo - ";
                break;
            case YFirmware:
                version = "Y - ";
                break;
            case Y6Firmware:
                version = "Y6 - ";
                break;
            case HeliFirmware:
                version = "Heli - ";
                break;
            case PlaneFirmware:
                version = "Plane - ";
                break;
            case RoverFirmware:
                version = "Rover - ";
                break;
            case DefaultVehicleFirmware:
                break;
            }
            version += vehicle.firmwareVersion;
            apmAvailableVersions << version;
        }
        _apmAvailableVersions = apmAvailableVersions;
        emit apmAvailableVersionsChanged();
    }
    QStringList apmAvailableVersions(){
        return _apmAvailableVersions;
    }

    /// < gps
    void setGpsAvailableVersions(){
        if(_selectedFirmwareType == CustomFirmware){
            _gpsAvailableVersions = customFirmwareAvailable(_gpsList, AerialVehicle, GPS);
        }else{
            _gpsAvailableVersions = firmwareAvailableVersions(_gpsList, _rgGPSFirmware);
        }
        emit gpsAvailableVersionsChanged();
    }
    QStringList gpsAvailableVersions(){
        return _gpsAvailableVersions;
    }

    /// < gimbals
    void setGimbalAvailableVersions(){
        if(_selectedFirmwareType == CustomFirmware){
            _gimbalAvailableVersions = customFirmwareAvailable(_gimbalList, AerialVehicle, Gimbal);
        }else{
            _gimbalAvailableVersions = firmwareAvailableVersions(_gimbalList, _rgGimbalFirmware);
        }
        emit gimbalAvailableVersionsChanged();
    }
    QStringList gimbalAvailableVersions(){
        return _gimbalAvailableVersions;
    }

     /// < Battery
    void setBatteryAvailableVersions(){
        if(_selectedFirmwareType == CustomFirmware){
            _batteryAvailableVersions = customFirmwareAvailable(_batteryList, AerialVehicle, Battery);
        }else{
            _batteryAvailableVersions = firmwareAvailableVersions(_batteryList, _rgBatteryFirmware);
        }
        emit batteryAvailableVersionsChanged();
    }
    QStringList batteryAvailableVersions(){
        return _batteryAvailableVersions;
    }

    /// < smartConsole
    void setRemoteControlAvailableVersions(){
        if(_selectedFirmwareType == CustomFirmware){
            _smartConsoleAvailableVersions = customFirmwareAvailable(_smartConsoleControlList, AerialVehicle, RemoteControl);
        }else{
            _smartConsoleAvailableVersions = firmwareAvailableVersions(_smartConsoleControlList, _rgRemoteControlFirmware);
        }
        emit consoleAvailableVersionsChanged();
    }
    QStringList remoteControlAvailableVersions(){
        return _smartConsoleAvailableVersions;
    }

    /// < eSpeedControl
    void setESpeedControlAvailableVersions(){
        if(_selectedFirmwareType == CustomFirmware){
            _eSpeedControlAvailableVersions = customFirmwareAvailable(_eSpeedControlList, AerialVehicle, ESpeedControl);
        }else{
            _eSpeedControlAvailableVersions = firmwareAvailableVersions(_eSpeedControlList, _rgESpeedControlFirmware);
        }
        emit escAvailableVersionsChanged();
    }
    QStringList eSpeedControlAvailableVersions(){
        return _eSpeedControlAvailableVersions;
    }

    /// < RaspBerry
    void setRaspBerryAvailableVersions(){
        if(_selectedFirmwareType == CustomFirmware){
            _raspBerryAvailableVersions = customFirmwareAvailable(_raspBerryList, AerialVehicle, RaspBerry);
        }else{
            _raspBerryAvailableVersions = firmwareAvailableVersions(_raspBerryList, _rgRaspBerryFirmware);
        }
        emit raspAvailableVersionsChanged();
    }
    QStringList raspBerryAvailableVersions(){
        return _raspBerryAvailableVersions;
    }

    ///  -----------------------------------------------------------
    /// function for parse and get firmware version, init firmware hashes
private:
    //class XmlStreamReader{
    //public :
    void XmlStreamReader(){
        Firmware autoPilotStackType[]={
            {AutoPilotStackPX4,"AutoPilotStackPX4"}
            ,{AutoPilotStackAPM,"AutoPilotStackAPM"}
            ,{PX4Flow,"PX4Flow"}
            ,{ThreeDRRadio,"ThreeDRRadio"}
            ,{AerialVehicle,"RemoteControl"}
            ,{AerialVehicle,"GPS"}
            ,{AerialVehicle,"battery"}
            ,{AerialVehicle,"gimbal"}
            ,{AerialVehicle,"eSpeedControl"}
            ,{AerialVehicle,"RaspBerry"}
        };

        _VersionNew<<&_px4FlightStackNew<<&_apmFlightStackNew
                  <<NULL<<NULL<<&_smartConsoleNew
                 <<&_gpsNew<<&_batteryNew<<&_gimbalNew
                <<&_eSpeedControlNew<<&_raspBerryNew;

        for(unsigned int i = 0; i < sizeof(autoPilotStackType)/sizeof(autoPilotStackType[0]); i++){
            const Firmware &firmware = autoPilotStackType[i];
            _autoPilotStackType.append(firmware);
            _newVersionFlag<<0;
        }

        Firmware firmwareType[]={
            {StableFirmware,"StableFirmware"}
            ,{BetaFirmware,"BetaFirmware"}
            ,{DeveloperFirmware,"DeveloperFirmware"}
            ,{CustomFirmware,"CustomFirmware"}
        };
        for(unsigned int i = 0; i < sizeof(firmwareType)/sizeof(firmwareType[0]); i++){
            const Firmware &firmware = firmwareType[i];
            _firmwareType.append(firmware);
        }

        Firmware firmwareVehicleType[]={
            {QuadFirmware,"QuadFirmware"}
            ,{X8Firmware,"X8Firmware"}
            ,{HexaFirmware,"HexaFirmware"}
            ,{OctoFirmware,"OctoFirmware"}
            ,{YFirmware,"YFirmware"}
            ,{Y6Firmware,"Y6Firmware"}
            ,{HeliFirmware,"HeliFirmware"}
            ,{PlaneFirmware,"PlaneFirmware"}
            ,{RoverFirmware,"RoverFirmware"}
            ,{DefaultVehicleFirmware,"DefaultVehicleFirmware"}
        };
        for(unsigned int i = 0; i < sizeof(firmwareVehicleType)/sizeof(firmwareVehicleType[0]); i++){
            const Firmware &firmware = firmwareVehicleType[i];
            _firmwareVehicleType.append(firmware);
        }
    }
    bool XmlReaderFile(const QString &xmlFile){
        _xmlFile.setFileName (xmlFile);
        if (!_xmlFile.open(QFile::ReadOnly|QFile::Text)){
            return false;
        }
        _xmlReader.setDevice (&_xmlFile);
        if(_xmlReader.hasError ()){
            return false;
        }
        XmlStreamReader();
        XmlReaderStart();
        return true;
    }

    bool XmlReaderStart(){
        //cout<<"start xml"<<endl<<flush;
        while(!_xmlReader.atEnd()){
            QXmlStreamReader::TokenType type = _xmlReader.readNext();
            if(type == QXmlStreamReader::DTD || type == QXmlStreamReader::StartDocument){
                continue;
            }

            if (type == QXmlStreamReader::StartElement){
//                qDebug() << "<" <<_xmlReader.name ().toString ()<<">";
                if(!_xmlReader.name ().toString ().compare ("vehicleType", Qt::CaseInsensitive)){
                    //element.
                    XmlReaderPilotStackType();
                }
            }
            if(type == QXmlStreamReader::EndElement){
                //cout<<"</"<<_xmlReader.name ().toString ().toStdString () << ">"<<endl<<flush;
            }
            if (_xmlReader.hasError()) {
                qDebug() << "error: " << _xmlReader.errorString();
                break;
            }
        }
        return true;
    }
    //px4 apm ...
    bool XmlReaderPilotStackType(){
        while(!_xmlReader.atEnd()){
            _firmwareIdentifier.autopilotStackType = AerialVehicle;
            _firmwareIdentifier.firmwareType = StableFirmware;
            _firmwareIdentifier.firmwareVehicleType = DefaultVehicleFirmware;
            _firmwareIdentifier.selectedFirmware = Board;
            QXmlStreamReader::TokenType type = _xmlReader.readNext();
            if(type == QXmlStreamReader::DTD ||type == QXmlStreamReader::StartDocument){
                continue;
            }
            if (type == QXmlStreamReader::StartElement){
                //cout<<"<"<<_xmlReader.name ().toString ().toStdString ()<<">"<<endl<<flush;
                bool findType = false;
                for(int i = 0 ; i < _autoPilotStackType.size (); i++){
                    const Firmware &firmware = _autoPilotStackType[i];
                    if(!firmware.autoPilotName.compare (_xmlReader.name (), Qt::CaseInsensitive)){
                        //
                        findType = true;
                        _firmwareName = _xmlReader.name ().toString ();
                        _firmwareIdentifier.autopilotStackType = (AutoPilotStackType_t)firmware.autoPilot;
                        _currentFimware = i;
                        if(_newVersionFlag[_currentFimware] == 0){
                            _newVersionFlag[_currentFimware] = 1;
                        }
                        XmlReaderFirmwareType();
                        break;
                    }
                }

                if(!findType){
                    _xmlReader.skipCurrentElement ();
                }
            }
            if(type == QXmlStreamReader::EndElement){
                //cout<<"</"<<_xmlReader.name ().toString ().toStdString () << ">"<<endl<<flush;
                bool findType = false;
                foreach(Firmware firmware, _autoPilotStackType){
                    if(!firmware.autoPilotName.compare (_xmlReader.name (), Qt::CaseInsensitive)){
                        findType = true;
                        break;
                    }
                }

                if(!findType){
                    return true;
                }
            }
        }
        return true;
    }

    //quad hex ...
    bool XmlReaderFirmwareType(){
        while(!_xmlReader.atEnd()){
            QXmlStreamReader::TokenType type = _xmlReader.readNext();
            if(type == QXmlStreamReader::DTD ||type == QXmlStreamReader::StartDocument){
                continue;
            }
            if (type == QXmlStreamReader::StartElement){
                //cout<<"<"<<_xmlReader.name ().toString ().toStdString ()<<">"<<endl<<flush;
                bool findType = false;
                foreach(Firmware firmware, _firmwareVehicleType){
                    if(!firmware.autoPilotName.compare (_xmlReader.name (), Qt::CaseInsensitive)){
                        //
                        findType = true;
                        _firmwareIdentifier.firmwareVehicleType = (FirmwareVehicleType_t)firmware.autoPilot;
                        XmlReaderFirmwareVehicleType();
                        break;
                    }
                }
                foreach(Firmware firmware, _firmwareType){
                    if(!firmware.autoPilotName.compare (_xmlReader.name (), Qt::CaseInsensitive)){
                        //
                        _firmwareIdentifier.firmwareType = (FirmwareType_t)firmware.autoPilot;
                        findType = true;
                        XmlReaderFirmwareVersionUrl();
                        break;
                    }
                }
                if(!_xmlReader.name ().toString ().compare ("version", Qt::CaseInsensitive)){
                    findType = true;
                    XmlReaderVersion();
                }
                if(!findType){
                    _xmlReader.skipCurrentElement ();
                }
            }
            if(type == QXmlStreamReader::EndElement){
                //cout<<"</"<<_xmlReader.name ().toString ().toStdString () << ">"<<endl<<flush;
                bool findType = false;
                foreach(Firmware firmware, _firmwareVehicleType){
                    if(!firmware.autoPilotName.compare (_xmlReader.name (), Qt::CaseInsensitive)){
                        findType = true;
                        break;
                    }
                }
                foreach(Firmware firmware, _firmwareType){
                    if(!firmware.autoPilotName.compare (_xmlReader.name (), Qt::CaseInsensitive)){
                        findType = true;
                        break;
                    }
                }
                if(!_xmlReader.name ().toString ().compare ("version", Qt::CaseInsensitive)){
                    findType = true;
                }
                if(!findType){
                    return true;
                }
            }
        }
        return true;
    }

    //beta dev stable
    bool XmlReaderFirmwareVehicleType(){
        while(!_xmlReader.atEnd()){
            QXmlStreamReader::TokenType type = _xmlReader.readNext();
            if(type == QXmlStreamReader::DTD ||type == QXmlStreamReader::StartDocument){
                continue;
            }
            if (type == QXmlStreamReader::StartElement){
                //cout<<"<"<<_xmlReader.name ().toString ().toStdString ()<<">"<<endl<<flush;
                bool findType = false;
                foreach(Firmware firmware, _firmwareType){
                    if(!firmware.autoPilotName.compare (_xmlReader.name (), Qt::CaseInsensitive)){
                        //
                        _firmwareIdentifier.firmwareType = (FirmwareType_t)firmware.autoPilot;
                        findType = true;
                        XmlReaderFirmwareVersionUrl();
                        break;
                    }
                }
                if(!findType){
                    _xmlReader.skipCurrentElement ();
                }
            }
            if(type == QXmlStreamReader::EndElement){
                //cout<<"</"<<_xmlReader.name ().toString ().toStdString () << ">"<<endl<<flush;
                bool findType = false;
                foreach(Firmware firmware, _firmwareType){
                    if(!firmware.autoPilotName.compare (_xmlReader.name (), Qt::CaseInsensitive)){
                        findType = true;
                        break;
                    }
                }
                if(!findType){
                    return true;
                }
            }
        }
        return true;
    }

    //version url
    bool XmlReaderFirmwareVersionUrl(){
        while(!_xmlReader.atEnd()){
            QXmlStreamReader::TokenType type = _xmlReader.readNext();
            if(type == QXmlStreamReader::DTD ||type == QXmlStreamReader::StartDocument){
                continue;
            }
            if (type == QXmlStreamReader::StartElement){
                //cout<<"<"<<_xmlReader.name ().toString ().toStdString ()<<">"<<endl<<flush;
                if(!_xmlReader.name ().toString ().compare ("version", Qt::CaseInsensitive)){
                    XmlReaderVersion();
                }else{
                    _xmlReader.skipCurrentElement ();
                }
            }
            if(type == QXmlStreamReader::EndElement)
            {
                //cout<<"</"<<_xmlReader.name ().toString ().toStdString () << ">"<<endl<<flush;
                if(!_xmlReader.name ().toString ().compare ("version", Qt::CaseInsensitive)){
                }else{
                    return true;
                }
            }
        }
        return true;
    }

    bool XmlReaderVersion(){
        QString version;
        while(!_xmlReader.atEnd()){
            QXmlStreamReader::TokenType type = _xmlReader.readNext();
            if(type == QXmlStreamReader::DTD ||type == QXmlStreamReader::StartDocument){
                continue;
            }
            if (type == QXmlStreamReader::StartElement){
                //cout<<"<"<<_xmlReader.name ().toString ().toStdString ()<<">"<<endl<<flush;
                QString xmlReader = _xmlReader.name ().toString ();
                if(!_xmlReader.name ().toString ().compare ("versionNum", Qt::CaseInsensitive)){
                    type = _xmlReader.readNext ();
                }else if(!_xmlReader.name ().toString ().compare ("url", Qt::CaseInsensitive)){
                    type = _xmlReader.readNext ();
                }else{
                    _xmlReader.skipCurrentElement ();
                    continue;
                }
                if(type == QXmlStreamReader::Characters && !_xmlReader.isWhitespace()){
                   // cout<<_xmlReader.text ().toString ().toStdString ()<<endl<<flush;
                    if(!xmlReader.compare ("versionNum", Qt::CaseInsensitive)){
                        version = _xmlReader.text ().toString ();
                    }else if(!xmlReader.compare ("url", Qt::CaseInsensitive)){
                        QString url =  _xmlReader.text ().toString ();
                        ///
                        XmlFirmware(url, version);
                    }

                }
            }

            if(type == QXmlStreamReader::EndElement){
                //cout<<"</"<<_xmlReader.name ().toString ().toStdString () << ">"<<endl<<flush;
                if(!_xmlReader.name ().toString ().compare ("versionNum", Qt::CaseInsensitive)){
                }else if(!_xmlReader.name ().toString ().compare ("url", Qt::CaseInsensitive)){
                }else{
                    return true;
                }
            }
        }
        return true;
    }

    void XmlFirmware(const QString &url, const QString &version){
        QList<FirmwareIdentifier> *currentFirmware = NULL;
        if(!_firmwareName.compare ("AutoPilotStackPX4",Qt::CaseInsensitive)
                ||!_firmwareName.compare ("AutoPilotStackAPM",Qt::CaseInsensitive)
                ||!_firmwareName.compare ("PX4Flow",Qt::CaseInsensitive)){
            QString fileName = QFileInfo(url).baseName();
            if(fileName.contains ("-v4", Qt::CaseInsensitive)){
                currentFirmware = &_rgPX4FMUV4Firmware;
            }else if(fileName.contains ("mindpx-v2", Qt::CaseInsensitive)){
                currentFirmware = &_rgMindPXFMUV2Firmware;
            }else if(fileName.contains ("px4flow", Qt::CaseInsensitive)){
                currentFirmware = &_rgPX4FLowFirmware;
            }else if(fileName.contains ("-v2", Qt::CaseInsensitive)){
                currentFirmware = &_rgPX4FMUV2Firmware;
            }else if(fileName.contains ("-v1", Qt::CaseInsensitive)){
                currentFirmware = &_rgPX4FMUV1Firmware;
            }
        }else if(!_firmwareName.compare ("RemoteControl",Qt::CaseInsensitive)){
            _firmwareIdentifier.selectedFirmware = RemoteControl;
            currentFirmware = &_rgRemoteControlFirmware;
        }else if(!_firmwareName.compare ("GPS",Qt::CaseInsensitive)){
            _firmwareIdentifier.selectedFirmware = GPS;
            currentFirmware = &_rgGPSFirmware;
        }else if(!_firmwareName.compare ("Battery",Qt::CaseInsensitive)){
            _firmwareIdentifier.selectedFirmware = Battery;
            currentFirmware = &_rgBatteryFirmware;
        }else if(!_firmwareName.compare ("Gimbal",Qt::CaseInsensitive)){
            _firmwareIdentifier.selectedFirmware = Gimbal;
            currentFirmware = &_rgGimbalFirmware;
        }else if(!_firmwareName.compare ("ESpeedControl",Qt::CaseInsensitive)){
            _firmwareIdentifier.selectedFirmware = ESpeedControl;
            currentFirmware = &_rgESpeedControlFirmware;
        }else if(!_firmwareName.compare ("RaspBerry",Qt::CaseInsensitive)){
            _firmwareIdentifier.selectedFirmware = RaspBerry;
            currentFirmware = &_rgRaspBerryFirmware;
        }else{
            qWarning()<<"Can prase Board version and url from remote file!";
        }
        if(currentFirmware){
//            qDebug()<<_firmwareIdentifier.autopilotStackType
//                   <<" : " <<_firmwareIdentifier.firmwareType
//                  <<" : " <<_firmwareIdentifier.firmwareVehicleType
//                 <<" : " <<_firmwareIdentifier.selectedFirmware
//                <<" : " <<version
//               <<" : "<<url
//              <<" : "<< _newVersionFlag[_currentFimware];

            _firmwareIdentifier.firmwareVersion = version;
            if(_newVersionFlag[_currentFimware] == 1 && _VersionNew[_currentFimware]){
                *_VersionNew[_currentFimware] = version;
                _newVersionFlag[_currentFimware] ++;
            }
            _firmwareIdentifier.url = url;
            if(!currentFirmware->contains (_firmwareIdentifier))
                currentFirmware->append (_firmwareIdentifier);
        }
    }

private:
    struct Firmware{
        int autoPilot;
        QString autoPilotName;
    };
    QFile _xmlFile;
    QString _firmwareName;
    FirmwareIdentifier _firmwareIdentifier;
    QXmlStreamReader _xmlReader;
    QList<Firmware> _autoPilotStackType;
    QList<Firmware>  _firmwareType;
    QList<Firmware> _firmwareVehicleType;
    int _currentFimware;
    QList<int> _newVersionFlag;
    QList<QString*> _VersionNew;
    //};
    //XmlStreamReader *_xmlStreamReader;
};
/*
        Project/Group list
        1. XH-Autopilot
        2. XH-GroundControlQt
        3. XH-GroundControlAndroid
        4. XH-SmartConsole
        5. XH-SmartBattery
        6. XH-BrushlessControllerFOC
        7. XH-BrushlessGimbalsFOC
*/
// global hashing function
uint qHash(const FirmwareUpgradeController::FirmwareIdentifier& firmwareId);

#endif
