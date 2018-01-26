/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief PX4 Firmware Upgrade UI
///     @author Don Gagne <don@thegagnes.com>

#include "FirmwareUpgradeController.h"
#include "Bootloader.h"
#ifndef __android__
#include "QGCFileDialog.h"
#endif
#include "QGCApplication.h"
#include "QGCFileDownload.h"
#include "ConsoleUpgradeController.h"

#include <QStandardPaths>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkProxy>
#include <QXmlStreamReader>
#include <QMessageBox>

const static QString config_xml_url("https://download.brainybeeuav.com/Firmware/.version.xml");

struct FirmwareToUrlElement_t {
    FirmwareUpgradeController::AutoPilotStackType_t    stackType;
    FirmwareUpgradeController::FirmwareType_t          firmwareType;
    FirmwareUpgradeController::FirmwareVehicleType_t   vehicleType;
    QString                                            url;
};

uint qHash(const FirmwareUpgradeController::FirmwareIdentifier& firmwareId)
{
    return ( firmwareId.autopilotStackType |
             (firmwareId.firmwareType << 8) |
             (firmwareId.firmwareVehicleType << 16) );
}

/// @Brief Constructs a new FirmwareUpgradeController Widget. This widget is used within the PX4VehicleConfig set of screens.
FirmwareUpgradeController::FirmwareUpgradeController(void)
    : _bootloaderBoardID(Bootloader::boardIDPX4FMUV4)
    , _downloadManager(NULL)
    , _downloadNetworkReply(NULL)
    , _statusLog(NULL)
    #ifdef __android__
    , _androidType(true)
    #else
    , _androidType(false)
    #endif
    , _selectedFirmwareType(StableFirmware)
    , _image(NULL)
    , _fileManager(NULL)
    , _NewVersion("")
    , _BoardVersion("")
    , _firmwareVersion(false)
    , _tryTimes(5)
    , _px4FlightStackNew("")
    , _px4FlightStackBoard("")
    , _apmFlightStackNew("")
    , _apmFlightStackBoard("")
    , _gpsNew("")
    , _gpsBoard("")
    , _gimbalNew("")
    , _gimbalBoard("")
    , _batteryNew("")
    , _batteryBoard ("")
    , _smartConsoleNew("")
    , _smartConsoleBoard("")
    , _eSpeedControlNew("")
    , _eSpeedControlBoard("")
    , _raspBerryNew("")
    , _raspBerryBoard("")
{
    _initFileManager();
    _threadController = new PX4FirmwareUpgradeThreadController(this);
    Q_CHECK_PTR(_threadController);

    connect(_threadController, &PX4FirmwareUpgradeThreadController::foundBoard,             this, &FirmwareUpgradeController::_foundBoard);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::noBoardFound,           this, &FirmwareUpgradeController::_noBoardFound);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::boardGone,              this, &FirmwareUpgradeController::_boardGone);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::foundBootloader,        this, &FirmwareUpgradeController::_foundBootloader);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::bootloaderSyncFailed,   this, &FirmwareUpgradeController::_bootloaderSyncFailed);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::error,                  this, &FirmwareUpgradeController::_error);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::updateProgress,         this, &FirmwareUpgradeController::_updateProgress);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::status,                 this, &FirmwareUpgradeController::_status);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::eraseStarted,           this, &FirmwareUpgradeController::_eraseStarted);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::eraseComplete,          this, &FirmwareUpgradeController::_eraseComplete);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::flashComplete,          this, &FirmwareUpgradeController::_flashComplete);
    connect(_threadController, &PX4FirmwareUpgradeThreadController::updateProgress,         this, &FirmwareUpgradeController::_updateProgress);
    
    connect(&_eraseTimer, &QTimer::timeout, this, &FirmwareUpgradeController::_eraseProgressTick);

    connect(_threadController, &PX4FirmwareUpgradeThreadController::startUpgradeFirmware,          this, &FirmwareUpgradeController::_upgradeFirmware);
    connect(&_boardInfoTimerRetry, &QTimer::timeout, this, &FirmwareUpgradeController::_boardSearchInfo);
    connect(this,&FirmwareUpgradeController::boardSearchInfoSignal, this , &FirmwareUpgradeController::_boardSearchInfo);
    _boardInfoTimerRetry.setSingleShot(true);
    _boardInfoTimerRetry.setInterval(100);

    _initFirmwareHash();
    //_xmlStreamReader = new XmlStreamReader();
    _consoleUpgradeController = new ConsoleUpgradeController();
    _raspBerryUpgradeController = new RaspBerryUpgradeController();
    MAVLinkProtocol* _mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    if(_mavlink){
        connect(_mavlink, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), _consoleUpgradeController, SLOT(_receiveBytes(LinkInterface*, mavlink_message_t)));
        connect(_mavlink, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), _raspBerryUpgradeController, SLOT(_receiveBytes(LinkInterface*, mavlink_message_t)));
    }
    connect(_consoleUpgradeController,          &ConsoleUpgradeController::getVersion,                                    this, &FirmwareUpgradeController::_setFirmwareVersion);
    connect(_consoleUpgradeController,          &ConsoleUpgradeController::uploadProgram,                                 this, &FirmwareUpgradeController::_downloadProgress);
    connect(_consoleUpgradeController,          &ConsoleUpgradeController::appendStatusLog,                               this, &FirmwareUpgradeController::_appendStatusLog);
    connect(_consoleUpgradeController,          &ConsoleUpgradeController::nextFirmwareUpgrade,                           this, &FirmwareUpgradeController::_nextFirmwareUpgrade);

    connect(_raspBerryUpgradeController,          &RaspBerryUpgradeController::getVersion,                                    this, &FirmwareUpgradeController::_setFirmwareVersion);
    connect(_raspBerryUpgradeController,          &RaspBerryUpgradeController::uploadProgram,                                 this, &FirmwareUpgradeController::_downloadProgress);
    connect(_raspBerryUpgradeController,          &RaspBerryUpgradeController::appendStatusLog,                               this, &FirmwareUpgradeController::_appendStatusLog);
    connect(_raspBerryUpgradeController,          &RaspBerryUpgradeController::nextFirmwareUpgrade,                           this, &FirmwareUpgradeController::_nextFirmwareUpgrade);
}

FirmwareUpgradeController::~FirmwareUpgradeController(){
    if(qgcApp()->toolbox()->linkManager()){
        qgcApp()->toolbox()->linkManager()->startAutoconnect ();
        qgcApp()->toolbox()->linkManager()->setConnectionsAllowed();
    }
    if(getFileManager()){
        _fileManager->stopAckTimeout();
    }

    MAVLinkProtocol* _mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    if(_mavlink){
        disconnect(_mavlink, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), _consoleUpgradeController, SLOT(_receiveBytes(LinkInterface*, mavlink_message_t)));
        disconnect(_mavlink, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), _raspBerryUpgradeController, SLOT(_receiveBytes(LinkInterface*, mavlink_message_t)));
    }
    if(_consoleUpgradeController){
        _consoleUpgradeController->stopAckTimeout();
        delete _consoleUpgradeController;
        _consoleUpgradeController = NULL;
    }
    if(_raspBerryUpgradeController){
        _raspBerryUpgradeController->stopAckTimeout();
        delete _raspBerryUpgradeController;
        _raspBerryUpgradeController = NULL;
    }
}

/// @brief intializes the firmware hashes with proper urls.
/// This happens only once for a class instance first time when it is needed.
void FirmwareUpgradeController::_initFirmwareHash()
{
    // indirect check whether this function has been called before or not
    // may have to be modified if _rgPX4FMUV2Firmware disappears
    _rgPX4FMUV4Firmware.clear ();
    _rgPX4FMUV2Firmware.clear ();
    _rgAeroCoreFirmware.clear ();
    _rgPX4FMUV1Firmware.clear ();
    _rgMindPXFMUV2Firmware.clear ();
    _rgPX4FLowFirmware.clear ();
    _rg3DRRadioFirmware.clear ();
    _rgRemoteControlFirmware.clear ();
    _rgGPSFirmware.clear ();
    _rgBatteryFirmware.clear ();
    _rgGimbalFirmware.clear ();
    _rgESpeedControlFirmware.clear ();
    _rgRaspBerryFirmware.clear ();
    XmlStreamReader();

    /// init _prgFirmware
    _firmwareVersionList<< SelectedFirmwareType_t::RemoteControl << SelectedFirmwareType_t::RaspBerry
                        << SelectedFirmwareType_t::Board << SelectedFirmwareType_t::Battery
                        << SelectedFirmwareType_t::Gimbal << SelectedFirmwareType_t::ESpeedControl
                        << SelectedFirmwareType_t::GPS;

    _deviceVersionCount = 0 ; //_firmwareVersionList.size();
}

/// @brief Starts searching for boards on the background thread
void FirmwareUpgradeController::startBoardSearch(void)
{
    LinkManager* linkMgr = qgcApp()->toolbox()->linkManager();

    linkMgr->setConnectionsSuspended(tr("Connect not allowed during Firmware Upgrade."));

    // FIXME: Why did we get here with active vehicle?
    if (!qgcApp()->toolbox()->multiVehicleManager()->activeVehicle()) {
        // We have to disconnect any inactive links
        //linkMgr->disconnectAll();
    }

    _bootloaderFound = false;
    _startFlashWhenBootloaderFound = false;
    if(!_selectedFirmware.isEmpty()){
        if(_selectedFirmware.back().selectedFirmware == RaspBerry){
            qDebug()<<"Raspberry start upgrade";
            usbSerialDownloadFirmware();
        }else{
#ifdef __android__
        usbSerialDownloadFirmware();
#else
        _threadController->startFindBoardLoop(_selectedFirmware.back().selectedFirmware);
#endif
        }
    }else{
        _appendStatusLog(tr("You must select firmware!"), true);
    }
}

void FirmwareUpgradeController::startBoardSearchInfo(void){
    _getBoardFirmwareVersion();
    emit boardSearchInfoSignal();
}

/// @brief get the flight control serial port information
void FirmwareUpgradeController::_boardSearchInfo(){
    //apmFirmware() ;
    //px4Firmware() ;
    QSerialPortInfo                 portInfo;
    QGCSerialPortInfo::BoardType_t  boardType = QGCSerialPortInfo::BoardTypePX4FMUV4;
    QString portName;
    bool canFlash = false, findBoard = false;
    MultiVehicleManager *linkManager = qgcApp()->toolbox()->multiVehicleManager();
    if (linkManager && linkManager->activeVehicle()){
        SerialLink* pSerialLink = qobject_cast<SerialLink*>(linkManager->activeVehicle()->priorityLink());
        if (pSerialLink) {
            LinkConfiguration* pLinkConfig = pSerialLink->getLinkConfiguration();
            if (pLinkConfig) {
                SerialConfiguration* pSerialConfig = qobject_cast<SerialConfiguration*>(pLinkConfig);
                if (pSerialConfig) {
                    findBoard = true;
                    portName = pSerialConfig->portName ();
                    qDebug() <<__FILE__  << " : " << __LINE__ << " : Serial Port --------------";
                }
            }
        }
    }
    if(findBoard){
        foreach (QGCSerialPortInfo info, QGCSerialPortInfo::availablePorts()) {
            if(portName == info.systemLocation()){
                boardType = info.boardType();
                qDebug() <<__FILE__  << " : " << __LINE__ << " : Serial Port : "<<boardType;
                break;
            }
        }
    }
    else
    foreach (QGCSerialPortInfo info, QGCSerialPortInfo::availablePorts()) {
        qDebug()<<"description : "<<info.description ();
        if( boardType == QGCSerialPortInfo::BoardTypePX4FMUV1
                || boardType == QGCSerialPortInfo::BoardTypePX4FMUV2
                || boardType == QGCSerialPortInfo::BoardTypePX4FMUV4
                || boardType == QGCSerialPortInfo::BoardTypeAeroCore
                || boardType == QGCSerialPortInfo::BoardTypeMINDPXFMUV2
                || boardType == QGCSerialPortInfo::BoardTypePX4Flow)
        {
            qDebug() <<__FILE__  << " : " << __LINE__ << " : Serial Port : "<<boardType;
            boardType = info.boardType();
            portInfo = info;
            canFlash = true;
            break;
        }
    }
    if (canFlash || findBoard){
        _deviceVersionCount = 0;
        _foundBoardInfo = portInfo;
        switch (boardType) {
        case QGCSerialPortInfo::BoardTypePX4FMUV1:
            _foundBoardTypeName = "PX4 FMU V1";
            _startFlashWhenBootloaderFound = false;
            break;
        case QGCSerialPortInfo::BoardTypePX4FMUV2:
            _foundBoardTypeName = "Pixhawk";
            _startFlashWhenBootloaderFound = false;
            break;
        case QGCSerialPortInfo::BoardTypePX4FMUV4:
            _foundBoardTypeName = "Pixhawk";
            _startFlashWhenBootloaderFound = false;
            break;
        case QGCSerialPortInfo::BoardTypeAeroCore:
            _foundBoardTypeName = "AeroCore";
            _startFlashWhenBootloaderFound = false;
            break;
        case QGCSerialPortInfo::BoardTypeMINDPXFMUV2:
            _foundBoardTypeName = "MindPX";
            _startFlashWhenBootloaderFound = false;
            break;
        case QGCSerialPortInfo::BoardTypePX4Flow:
            _foundBoardTypeName = "PX4 Flow";
            _startFlashWhenBootloaderFound = false;
            break;
        case QGCSerialPortInfo::BoardTypeSikRadio:
            _foundBoardTypeName = "SiK Radio";
            _startFlashWhenBootloaderFound = false;
            break;
        default:
            boardType = QGCSerialPortInfo::BoardTypePX4FMUV4;
            break;
        }
        _tryTimes = -1;
    }
    if((_tryTimes--) >= 0){
        _boardInfoTimerRetry.start();
    }
    else{
        _BoardType = boardType;
        loadFirmwareVersions();
    }
}

/// @brief Download file( firmware version config)
void FirmwareUpgradeController::loadFirmwareVersions(){
    QGCFileDownload* downloader = new QGCFileDownload(this);
    connect(downloader, &QGCFileDownload::downloadFinished, this, &FirmwareUpgradeController::firmwareVersionDownloadFinished);
    downloader->download(config_xml_url);
}

/// @brief Called when the file( firmware version config) download finished
void FirmwareUpgradeController::firmwareVersionDownloadFinished(QString remoteFile, QString localFile){
    Q_UNUSED(remoteFile);
    QFile versionFile(localFile);
    versionFile.open(QIODevice::ReadOnly | QIODevice::Text);
    if(!versionFile.isOpen ()){
        return ;
    }
    versionFile.close ();
    //    _xmlStreamReader->XmlReaderFile(localFile);
    XmlReaderFile(localFile);
    emit firmwareAvailableVersionsChanged();
    setPx4AvailableVersion();
    setApmAvailableVersions();
    setGpsAvailableVersions();
    setGimbalAvailableVersions();
    setBatteryAvailableVersions();
    setRemoteControlAvailableVersions();
    setESpeedControlAvailableVersions();
    setRaspBerryAvailableVersions();
}

/// @brief Called when the firmware type has been selected by the user to continue the flash process.
void FirmwareUpgradeController::flash(){
    if(!_selectedFirmware.isEmpty()){
        flash(_selectedFirmware.back());
    }
}

void FirmwareUpgradeController::flash(const FirmwareIdentifier& firmwareId){
    flash(firmwareId.autopilotStackType, firmwareId.firmwareType, firmwareId.firmwareVehicleType, firmwareId.firmwareVersion);
}

void FirmwareUpgradeController::flash(AutoPilotStackType_t stackType,
                                      FirmwareType_t firmwareType,
                                      FirmwareVehicleType_t vehicleType, QString vehicleVersion){
    FirmwareIdentifier firmwareId = FirmwareIdentifier(stackType, firmwareType, vehicleType, SelectedFirmwareType_t::Board, vehicleVersion);

    if (_bootloaderFound) {
        if(!_selectedFirmware.isEmpty ()){
            _getFirmwareFile(_selectedFirmware.back());
        }else{
            qWarning()<<__FILE__<<" : "<<__LINE__<<" no select firmware for upgrade frimware.";
        }
    } else {
        // We haven't found the bootloader yet. Need to wait until then to flash
        _startFlashWhenBootloaderFound = true;
        _startFlashWhenBootloaderFoundFirmwareIdentity = firmwareId;
    }
}

void FirmwareUpgradeController::cancel(void){
    _eraseTimer.stop();
    _threadController->cancel();
}

/// @brief Called when the firmware type has been selected by the user to set selected firmware.
void FirmwareUpgradeController::setSelectedFirmware(QList<int> selectedFirmwareType, QList<int> currentIndex){
    qDebug() << __FILE__ << " : " << __LINE__ << " : " <<"selectedFirmwareType : "<<selectedFirmwareType <<" : "<<currentIndex;
    if(selectedFirmwareType.size () <= 0 || currentIndex.size () <= 0){
        return ;
    }
    try{
        for(int i = 0; i < selectedFirmwareType.size (); i++){
            FirmwareIdentifier *firmwareId = NULL;
            switch(selectedFirmwareType[i]){
            case Board -2 : //qgroundcontrol
                break;
            case Board - 1: // px4
                if(!_px4FlightStackList.isEmpty () && _px4FlightStackList.size() > currentIndex[i])
                    firmwareId = &_px4FlightStackList[currentIndex[i]];
                break;
            case Board:     //apm
                if(!_apmFlightStackList.isEmpty () && _apmFlightStackList.size() > currentIndex[i])
                    firmwareId = &_apmFlightStackList[currentIndex[i]];
                break;
            case GPS:
                if(!_gpsList.isEmpty () && _gpsList.size() > currentIndex[i])
                    firmwareId = &_gpsList[currentIndex[i]];
                break;
            case RemoteControl:
                if(!_smartConsoleControlList.isEmpty () && _smartConsoleControlList.size() > currentIndex[i])
                    firmwareId = &_smartConsoleControlList[currentIndex[i]];
                break;
            case Battery:
                if(!_batteryList.isEmpty () && _batteryList.size() > currentIndex[i])
                    firmwareId = &_batteryList[currentIndex[i]];
                break;
            case Gimbal:
                if(!_gimbalList.isEmpty () && _gimbalList.size() > currentIndex[i])
                    firmwareId = &_gimbalList[currentIndex[i]];
                break;
            case ESpeedControl:
                if(!_eSpeedControlList.isEmpty () && _eSpeedControlList.size() > currentIndex[i])
                    firmwareId = &_eSpeedControlList[currentIndex[i]];
                break;
            case RaspBerry:
                if(!_raspBerryList.isEmpty () && _raspBerryList.size() > currentIndex[i])
                    firmwareId = &_raspBerryList[currentIndex[i]];
                break;
            default:
                break;
            }
            if(!firmwareId){
                _appendStatusLog(tr("First, You must select upgrade firmware"), true);
                return ;
            }
            FirmwareIdentifier firmware = FirmwareIdentifier(firmwareId->autopilotStackType, firmwareId->firmwareType,
                                                             firmwareId->firmwareVehicleType,firmwareId->selectedFirmware,
                                                             firmwareId->firmwareVersion, firmwareId->url);
            _selectedFirmware << firmware;
        }
    }catch(...){
    }
}

void FirmwareUpgradeController::_foundBoard(bool firstAttempt, const QSerialPortInfo& info, int boardType)
{
    _foundBoardInfo = info;
    _foundBoardType = (QGCSerialPortInfo::BoardType_t)boardType;
    if(boardType == QGCSerialPortInfo::BoardTypeSerial){
        boardType = _remoteBoardType;
    }
    if(boardType != Bootloader::boardIDPX4FMUV4
            || boardType != Bootloader::boardIDPX4FMUV1
            || boardType != Bootloader::boardIDPX4Flow
            || boardType != Bootloader::boardIDAeroCore
            || boardType != Bootloader::boardID3DRRadio
            || boardType != Bootloader::boardIDMINDPXFMUV2)
        boardType = Bootloader::boardIDPX4FMUV4;
    switch (boardType) {
    case QGCSerialPortInfo::BoardTypePX4FMUV1:
        _foundBoardTypeName = "PX4 FMU V1";
        _startFlashWhenBootloaderFound = false;
        break;
    case QGCSerialPortInfo::BoardTypePX4FMUV2:
    case QGCSerialPortInfo::BoardTypePX4FMUV4:
        _foundBoardTypeName = "Pixhawk";
        _startFlashWhenBootloaderFound = false;
        break;
    case QGCSerialPortInfo::BoardTypeAeroCore:
        _foundBoardTypeName = "AeroCore";
        _startFlashWhenBootloaderFound = false;
        break;
    case QGCSerialPortInfo::BoardTypeMINDPXFMUV2:
        _foundBoardTypeName = "MindPX";
        _startFlashWhenBootloaderFound = false;
        break;
    case QGCSerialPortInfo::BoardTypePX4Flow:
        _foundBoardTypeName = "PX4 Flow";
        _startFlashWhenBootloaderFound = false;
        break;
    case QGCSerialPortInfo::BoardTypeSikRadio:
        _foundBoardTypeName = "SiK Radio";
        if (!firstAttempt) {
            // Radio always flashes latest firmware, so we can start right away without
            // any further user input.
            _startFlashWhenBootloaderFound = true;
            _startFlashWhenBootloaderFoundFirmwareIdentity = FirmwareIdentifier(ThreeDRRadio,
                                                                                StableFirmware,
                                                                                DefaultVehicleFirmware);
        }
        break;
    }
    
    qCDebug(FirmwareUpgradeLog) << _foundBoardType;
    if(_foundBoardType == (QGCSerialPortInfo::BoardType_t)QGCSerialPortInfo::BoardTypeSerial
            ||_foundBoardType == (QGCSerialPortInfo::BoardType_t)QGCSerialPortInfo::BoardTypeSikRadio) {
        emit usbSerialFound();
    }else{
        emit boardFound();
    }
}

void FirmwareUpgradeController::_noBoardFound(void)
{
    emit noBoardFound();
}

void FirmwareUpgradeController::_boardGone(void)
{
    emit boardGone();
}

/// @brief Called when the bootloader is connected to by the findBootloader process. Moves the state machine
///         to the next step.
void FirmwareUpgradeController::_foundBootloader(int bootloaderVersion, int boardID, int flashSize)
{
    _bootloaderFound = true;
    _bootloaderVersion = bootloaderVersion;
    _bootloaderBoardID = boardID;
    _bootloaderBoardFlashSize = flashSize;
    
    _appendStatusLog(tr("Connected to bootloader:"));
    _appendStatusLog(QString(tr("  Version: %1")).arg(_bootloaderVersion));
    _appendStatusLog(QString(tr("  Board ID: %1")).arg(_bootloaderBoardID));
    _appendStatusLog(QString(tr("  Flash size: %1")).arg(_bootloaderBoardFlashSize));
    
    if (_startFlashWhenBootloaderFound) {
        flash(_startFlashWhenBootloaderFoundFirmwareIdentity);
    }
}

/// @brief Called when the findBootloader process is unable to sync to the bootloader. Moves the state
///         machine to the appropriate error state.
void FirmwareUpgradeController::_bootloaderSyncFailed(void)
{
    _errorCancel(tr("Unable to sync with bootloader."));
}

QList<FirmwareUpgradeController::FirmwareIdentifier> *FirmwareUpgradeController::_firmwareHashForBoardId(int boardId)
{
    switch (boardId) {
    case Bootloader::boardIDPX4FMUV1:
        return &_rgPX4FMUV1Firmware;
    case Bootloader::boardIDPX4Flow:
        return &_rgPX4FLowFirmware;
    case Bootloader::boardIDPX4FMUV2:
        return &_rgPX4FMUV2Firmware;
    case Bootloader::boardIDPX4FMUV4:
        return &_rgPX4FMUV4Firmware;
    case Bootloader::boardIDAeroCore:
        return &_rgAeroCoreFirmware;
    case Bootloader::boardIDMINDPXFMUV2:
        return &_rgMindPXFMUV2Firmware;
    case Bootloader::boardID3DRRadio:
        return &_rg3DRRadioFirmware;
    default:
        if(_foundBoardType != (QGCSerialPortInfo::BoardType_t)QGCSerialPortInfo::BoardTypeSerial){
            return NULL;
        }
    }
    return NULL;
}
QList<FirmwareUpgradeController::FirmwareIdentifier>* FirmwareUpgradeController::_firmwareHashForSelectedBoardId(int selectedFirmware){
    switch (selectedFirmware) {
    case SelectedFirmwareType_t::RemoteControl:
        return &_rgRemoteControlFirmware;
    case SelectedFirmwareType_t::GPS:
        return &_rgGPSFirmware;
    case SelectedFirmwareType_t::Battery:
        return &_rgBatteryFirmware;
    case SelectedFirmwareType_t::Gimbal:
        return &_rgGimbalFirmware;
    case SelectedFirmwareType_t::ESpeedControl:
        return &_rgESpeedControlFirmware;
    case SelectedFirmwareType_t::RaspBerry:
        return &_rgRaspBerryFirmware;
    case SelectedFirmwareType_t::Board:
    default:
        break;
    }
    return NULL;
}
QList<FirmwareUpgradeController::FirmwareIdentifier>* FirmwareUpgradeController::_firmwareHashForBoardType(QGCSerialPortInfo::BoardType_t boardType)
{
    int boardId = Bootloader::boardIDPX4FMUV2;

    switch (boardType) {
    case QGCSerialPortInfo::BoardTypePX4FMUV1:
        boardId = Bootloader::boardIDPX4FMUV1;
        break;
    case QGCSerialPortInfo::BoardTypePX4FMUV2:
        boardId = Bootloader::boardIDPX4FMUV2;
        break;
    case QGCSerialPortInfo::BoardTypePX4FMUV4:
        boardId = Bootloader::boardIDPX4FMUV4;
        break;
    case QGCSerialPortInfo::BoardTypeAeroCore:
        boardId = Bootloader::boardIDAeroCore;
        break;
    case QGCSerialPortInfo::BoardTypeMINDPXFMUV2:
        boardId = Bootloader::boardIDMINDPXFMUV2;
        break;
    case QGCSerialPortInfo::BoardTypePX4Flow:
        boardId = Bootloader::boardIDPX4Flow;
        break;
    case QGCSerialPortInfo::BoardTypeSikRadio:
        boardId = Bootloader::boardID3DRRadio;
        break;
    case QGCSerialPortInfo::BoardTypeSerial:
        break;
    default:
        qWarning() << "Internal error: invalid board type for flashing" << boardType;
        boardId = Bootloader::boardIDPX4FMUV2;
        break;
    }

    if(!_selectedFirmware.isEmpty() && _selectedFirmware.back().autopilotStackType == AerialVehicle){
        return _firmwareHashForSelectedBoardId(_selectedFirmware.back().selectedFirmware);
    }else{
        return _firmwareHashForBoardId(boardId);
    }
}

/// @brief Prompts the user to select a firmware file if needed and moves the state machine to the next state.
void FirmwareUpgradeController::_getFirmwareFile(FirmwareIdentifier firmwareId)
{
     QList<FirmwareIdentifier>* prgFirmware = NULL;
    if(!_selectedFirmware.isEmpty() && _selectedFirmware.back().autopilotStackType == AerialVehicle){
        prgFirmware = _firmwareHashForSelectedBoardId(_selectedFirmware.back().selectedFirmware);
    }else{
        prgFirmware = _firmwareHashForBoardId(_bootloaderBoardID);
    }
    
    if (!prgFirmware && firmwareId.firmwareType != CustomFirmware) {
        _errorCancel(tr("Attempting to flash an unknown board type, you must select 'Custom firmware file'"));
        return;
    }
    QString downloadFirmwareFilename;
    if (firmwareId.firmwareType == CustomFirmware) {
//        downloadFirmwareFilename = QGCFileDialog::getOpenFileName(NULL,                                                                // Parent to main window
//                                                                  "Select Firmware File",                                              // Dialog Caption
//                                                                  QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), // Initial directory
//                                                                  "Firmware Files (*.px4 *.bin *.ihx *.txt)");                         // File filter
//
        if(firmwareId.firmwareVersion != "CustomFirmware")
            downloadFirmwareFilename = firmwareId.firmwareVersion;
    } else {

        if (prgFirmware->contains(firmwareId)) {
            downloadFirmwareFilename = firmwareId.url;
        } else {
            _errorCancel(tr("Unable to find specified firmware download location"));
            return;
        }
    }
    
    if (downloadFirmwareFilename.isEmpty()) {
        _errorCancel(tr("No firmware file selected"));
    } else {
        _downloadFirmware(downloadFirmwareFilename);
    }
}

/// @brief Begins the process of downloading the selected firmware file.
void FirmwareUpgradeController::_downloadFirmware(QString downloadFirmwareFilename){
    Q_ASSERT(!downloadFirmwareFilename.isEmpty());
    
    _appendStatusLog(tr("Downloading firmware..."));
    _appendStatusLog(QString(tr(" From: %1")).arg(downloadFirmwareFilename));
    
    // Split out filename from path
    QString firmwareFilename = QFileInfo(downloadFirmwareFilename).fileName();
    Q_ASSERT(!firmwareFilename.isEmpty());
    
    // Determine location to download file to
    QString downloadFile = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    if (downloadFile.isEmpty()) {
        downloadFile = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (downloadFile.isEmpty()) {
            _errorCancel(tr("Unabled to find writable download location. Tried downloads and temp directory."));
            return;
        }
    }
    Q_ASSERT(!downloadFile.isEmpty());
    downloadFile += "/"  + firmwareFilename;

    QUrl firmwareUrl;
    if (downloadFirmwareFilename.startsWith("http:") || downloadFirmwareFilename.startsWith("https:")) {
        firmwareUrl.setUrl(downloadFirmwareFilename);
    } else {
        firmwareUrl = QUrl::fromLocalFile(downloadFirmwareFilename);
    }
    if(!firmwareUrl.isValid()){
        _errorCancel(tr("Unabled to find download location. Tried downloads."));
        return ;
    }
    Q_ASSERT(firmwareUrl.isValid());
    
    QNetworkRequest networkRequest(firmwareUrl);
    
    // Store download file location in user attribute so we can retrieve when the download finishes
    networkRequest.setAttribute(QNetworkRequest::User, downloadFile);
    
    _downloadManager = new QNetworkAccessManager(this);
    Q_CHECK_PTR(_downloadManager);
    QNetworkProxy tProxy;
    tProxy.setType(QNetworkProxy::DefaultProxy);
    _downloadManager->setProxy(tProxy);
    QSslConfiguration conf = QSslConfiguration::defaultConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    conf.setProtocol(QSsl::TlsV1_0);
    networkRequest.setSslConfiguration(conf);

    _downloadNetworkReply = _downloadManager->get(networkRequest);
    Q_ASSERT(_downloadNetworkReply);
    connect(_downloadNetworkReply, &QNetworkReply::downloadProgress, this, &FirmwareUpgradeController::_downloadProgress);
    connect(_downloadNetworkReply, &QNetworkReply::finished, this, &FirmwareUpgradeController::_downloadFinished);

    connect(_downloadNetworkReply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
            this, &FirmwareUpgradeController::_downloadError);
}

/// @brief Updates the progress indicator while downloading
void FirmwareUpgradeController::_downloadProgress(qint64 curr, qint64 total){
    // Take care of cases where 0 / 0 is emitted as error return value
    if (total > 0) {
        _progressBar->setProperty("value", (float)curr / (float)total);
    }
}

/// @brief Called when the firmware download completes.
void FirmwareUpgradeController::_downloadFinished(void)
{
    _appendStatusLog(tr("Download complete"));
    
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(QObject::sender());
    Q_ASSERT(reply);
    
    Q_ASSERT(_downloadNetworkReply == reply);
    
    _downloadManager->deleteLater();
    _downloadManager = NULL;
    
    // When an error occurs or the user cancels the download, we still end up here. So bail out in
    // those cases.
    if (reply->error() != QNetworkReply::NoError) {
        return;
    }
    
    // Download file location is in user attribute
    QString downloadFilename = reply->request().attribute(QNetworkRequest::User).toString();
    Q_ASSERT(!downloadFilename.isEmpty());
    
    // Store downloaded file in download location
    QFile file(downloadFilename);
    if (!file.open(QIODevice::WriteOnly)) {
        _errorCancel(QString(tr("Could not save downloaded file to %1. Error: %2")).arg(downloadFilename).arg(file.errorString()));
        return;
    }
    
    file.write(reply->readAll());
    file.close();
    FirmwareImage* image = new FirmwareImage(this);
    
    connect(image, &FirmwareImage::statusMessage, this, &FirmwareUpgradeController::_status);
    connect(image, &FirmwareImage::errorMessage, this, &FirmwareUpgradeController::_error);
    
    if (!image->load(downloadFilename, _bootloaderBoardID)) {
        _errorCancel(tr("Image load failed"));
        return;
    }

#ifndef __android__
    if(_foundBoardType == (QGCSerialPortInfo::BoardType_t)QGCSerialPortInfo::BoardTypeSerial
            ||_foundBoardType == (QGCSerialPortInfo::BoardType_t)QGCSerialPortInfo::BoardTypeSikRadio
            ||_foundBoardType == (QGCSerialPortInfo::BoardType_t)QGCSerialPortInfo::BoardTypeUnknown
            ||(_selectedFirmware.back().selectedFirmware == RaspBerry))
#endif
    {

        qDebug() << __FILE__ << " : " << __LINE__ << " : " << "_foundBoardType_: BoardTypeSerial";
        QString uploadFromHere = downloadFilename;
        if (downloadFilename.endsWith(".px4")) {
            uploadFromHere = image->binFilename();
        }
        //encrypt_file(uploadFromHere.toStdString().c_str());
        //This dir name must uniqueness
        QString path="/fs/microsd";
        QFile file(uploadFromHere);
        SelectedFirmwareType_t firmwareType = _selectedFirmware.back().selectedFirmware;
        if(firmwareType == GPS){
            path = "/fs/microsd/fw/com.zubax.gnss/1.0";
            uploadFromHere = QFileInfo(uploadFromHere).path() + "/2.0.e87da7d7.bin";
            QFile::remove (uploadFromHere);
            file.rename (QFileInfo(uploadFromHere).path() + "/2.0.e87da7d7.bin");
        }else if(firmwareType == RemoteControl){
            uploadFromHere = QFileInfo(uploadFromHere).path() + "/remotecontrol.bin";
            QFile::remove (uploadFromHere);
            file.rename (QFileInfo(uploadFromHere).path() + "/remotecontrol.bin");
            if(getFileManager()){
                _fileManager->stopAckTimeout();
            }
            if(_consoleUpgradeController){
                _appendStatusLog(tr("Uploading firmware..."));
                _updateProgress(0,1);
                _consoleUpgradeController->stopAckTimeout();
                _consoleUpgradeController->uploadPath (NULL, path, uploadFromHere, _selectedFirmware.back ().selectedFirmware);
            }
            return ;
        }else if(firmwareType == Battery){
            uploadFromHere = QFileInfo(uploadFromHere).path() + "/battery.txt";
            QFile::remove (uploadFromHere);
            file.rename (QFileInfo(uploadFromHere).path() + "/battery.txt");
        }else if(firmwareType == Gimbal){
            uploadFromHere = QFileInfo(uploadFromHere).path() + "/gimbal.bin";
            QFile::remove (uploadFromHere);
            file.rename (QFileInfo(uploadFromHere).path() + "/gimbal.bin");
        }else if(firmwareType == ESpeedControl){
            uploadFromHere = QFileInfo(uploadFromHere).path() + "/escontrol.bin";
            QFile::remove (uploadFromHere);
            file.rename (QFileInfo(uploadFromHere).path() + "/escontrol.bin");
        }else if(firmwareType == RaspBerry){
            uploadFromHere = QFileInfo(uploadFromHere).path() + "/main.bin";
            QFile::remove (uploadFromHere);
            file.rename (QFileInfo(uploadFromHere).path() + "/main.bin");
            if(getFileManager()){
                _fileManager->stopAckTimeout();
            }
            if(_raspBerryUpgradeController){
                _appendStatusLog(tr("Uploading firmware..."));
                _updateProgress(0,1);
                _raspBerryUpgradeController->stopAckTimeout();
                _raspBerryUpgradeController->uploadPath(NULL, path, uploadFromHere, _selectedFirmware.back ().selectedFirmware);
            }
            return ;
        }else if(firmwareType == Board){
            uploadFromHere = QFileInfo(uploadFromHere).path() + "/firmware.bin";
            QFile::remove (uploadFromHere);
            file.rename (QFileInfo(uploadFromHere).path() + "/firmware.bin");
        }else{
            _appendStatusLog(tr("Error upload firmware..."));
        }
        qDebug()<<"QFile FileError : "<<file.errorString ();


        if(!_fileManager){
            if(!_initFileManager()){
                 _appendStatusLog(tr("Please plugin your vechile"));
                return ;
            }
        }

        qDebug()<<"Current time : " << QTime::currentTime().toString();
        qDebug() << __FILE__ << " : " << __LINE__ << " : uploadFromHere : " << uploadFromHere << " : path : " << path;
        _consoleUpgradeController->stopAckTimeout();
        if(getFileManager()){
            _appendStatusLog(tr("Uploading firmware..."));
            _updateProgress(0,1);
            _fileManager->stopAckTimeout();
            _fileManager->uploadPath(path, uploadFromHere, _selectedFirmware.back ().selectedFirmware);
        }
        return;
    }
#ifndef __android__
    // We can't proceed unless we have the bootloader
    if (!_bootloaderFound) {
        _errorCancel(tr("Bootloader not found"));
        return;
    }
    
    //encrypt_file(image->binFilename ().toStdString ().c_str ());
    if (_bootloaderBoardFlashSize != 0 && image->imageSize() > _bootloaderBoardFlashSize) {
        _errorCancel(QString(tr("Image size of %1 is too large for board flash size %2")).arg(image->imageSize()).arg(_bootloaderBoardFlashSize));
        return;
    }

    _threadController->flash(image);
#endif
}

/// @brief Called when an error occurs during download
void FirmwareUpgradeController::_downloadError(QNetworkReply::NetworkError code)
{
    QString errorMsg;
    
    if (code == QNetworkReply::OperationCanceledError) {
        errorMsg = tr("Download cancelled");

    } else if (code == QNetworkReply::ContentNotFoundError) {
        errorMsg = QString(tr("Error: File Not Found. Please check %1 firmware version is available."))
                      .arg(firmwareTypeAsString(_selectedFirmwareType));

    } else {
        errorMsg = QString(tr("Error during download. Error: %1")).arg(code);
    }
    _errorCancel(errorMsg);
}

/// @brief Signals completion of one of the specified bootloader commands. Moves the state machine to the
///         appropriate next step.
void FirmwareUpgradeController::_flashComplete(void)
{
    delete _image;
    _image = NULL;
    
    _appendStatusLog(tr("Upgrade complete"), true);
    _appendStatusLog("------------------------------------------", false);
    emit flashComplete();
    qgcApp()->toolbox()->linkManager()->setConnectionsAllowed();
}

void FirmwareUpgradeController::_error(const QString& errorString)
{
    delete _image;
    _image = NULL;
    
    _errorCancel(QString("Error: %1").arg(errorString));
}

void FirmwareUpgradeController::_status(const QString& statusString)
{
    _appendStatusLog(statusString);
}

/// @brief Updates the progress bar from long running bootloader commands
void FirmwareUpgradeController::_updateProgress(int curr, int total)
{
    // Take care of cases where 0 / 0 is emitted as error return value
    if (total > 0) {
        _progressBar->setProperty("value", (float)curr / (float)total);
    }
}

/// @brief Moves the progress bar ahead on tick while erasing the board
void FirmwareUpgradeController::_eraseProgressTick(void)
{
    _eraseTickCount++;
    _progressBar->setProperty("value", (float)(_eraseTickCount*_eraseTickMsec) / (float)_eraseTotalMsec);
}

/// Appends the specified text to the status log area in the ui
void FirmwareUpgradeController::_appendStatusLog(const QString& text, bool critical)
{
    Q_ASSERT(_statusLog);
    
    QVariant returnedValue;
    QVariant varText;
    
    if (critical) {
        varText = QString("<font color=\"yellow\">%1</font>").arg(text);
    } else {
        varText = text;
    }
    
    QMetaObject::invokeMethod(_statusLog,
                              "append",
                              Q_RETURN_ARG(QVariant, returnedValue),
                              Q_ARG(QVariant, varText));
}

void FirmwareUpgradeController::_errorCancel(const QString& msg){
    if(_selectedFirmware.size() > 0){
        _appendStatusLog(msg, false);
        _appendStatusLog(tr("Upgrade failed"), true);
        if(_selectedFirmware.size() != 1){
            _appendStatusLog(tr("Will be the next firmware update"), false);
        }
        _nextFirmwareUpgrade();
    }else{
    _appendStatusLog(msg, false);
    _appendStatusLog(tr("Upgrade cancelled"), true);
    _appendStatusLog("------------------------------------------", false);
    emit error();
    cancel();
    qgcApp()->toolbox()->linkManager()->setConnectionsAllowed();
    }
}

void FirmwareUpgradeController::_eraseStarted(void)
{
    // We set up our own progress bar for erase since the erase command does not provide one
    _eraseTickCount = 0;
    _eraseTimer.start(_eraseTickMsec);
}

void FirmwareUpgradeController::_eraseComplete(void)
{
    _eraseTimer.stop();
}

/// @brief returns firmware type as a string
QString FirmwareUpgradeController::firmwareTypeAsString(FirmwareType_t type) const
{
    switch (type) {
    case StableFirmware:
        return "stable";
    case DeveloperFirmware:
        return "developer";
    case BetaFirmware:
        return "beta";
    default:
        return "custom";
    }
}

/// @brief Get file from localhost
QString FirmwareUpgradeController::getLocalFile(
        #ifdef __android__
        QString downloadFirmwareFilename,
        #endif
        int board){
#ifndef __android__
    QString fileFilter("Firmware Files (");
    switch(board){
    case Board -2:
    case Board -1:
    case Board   :
        fileFilter+="*.px4 *.bin *.ihx)";
        break;
    case GPS:
        fileFilter+="*.bin)";
        break;
    case RemoteControl:
        fileFilter+="*.bin)";
        break;
    case Battery:
        fileFilter+="*.txt)";
        break;
    case Gimbal:
        fileFilter+="*.bin)";
        break;
    case ESpeedControl:
        fileFilter+="*.bin)";
        break;
    case RaspBerry:
        fileFilter+="*.bin)";
        break;
    }
    QString downloadFirmwareFilename = QGCFileDialog::getOpenFileName(NULL,                                                                // Parent to main window
                                                                      "Select Firmware File",                                              // Dialog Caption
                                                                      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation), // Initial directory
                                                                      fileFilter);                         // File filter

#endif
    _downloadFirmwareFilename = downloadFirmwareFilename;
    QList<int> list;
    list<<board;
    setSelectedFirmwareType(CustomFirmware, list);
    return downloadFirmwareFilename;
}

/// @brief Show the selected firmware to the user by ui
void FirmwareUpgradeController::setSelectedFirmwareType(FirmwareType_t firmwareType, QList<int> board){
    _selectedFirmwareType = firmwareType;
    qDebug()<<board.size ()<<board;
    for(int i = 0; i < board.size (); i++){
        switch(board[i]){
        case Board -2 : //qgroundcontrol
            break;
        case Board - 1: // px4
            setPx4AvailableVersion();
            break;
        case Board:     //apm
            setApmAvailableVersions();
            break;
        case GPS:
            setGpsAvailableVersions();
            break;
        case Gimbal:
            setGimbalAvailableVersions();
            break;
        case Battery:
            setBatteryAvailableVersions();
            break;
        case RemoteControl:
            setRemoteControlAvailableVersions();
            break;
        case ESpeedControl:
            setESpeedControlAvailableVersions();
            break;
        case RaspBerry:
            setRaspBerryAvailableVersions();
            break;
        default:
            break;
        }
    }
}

void FirmwareUpgradeController::_upgradeFirmware(const QSerialPortInfo &portInfo, int type){
    if( _selectedFirmware.size() <= 0){
        _appendStatusLog("First, You must select upgrade firmware", true);
        return ;
    }
    _deviceVersionCount = 0;
    _foundBoardInfo = portInfo;
    _foundBoardType = (QGCSerialPortInfo::BoardType_t)type;
    usbSerialDownloadFirmware();
}

/// @brief Called when the firmware type has been selected by the user to upgrade firmware.
void FirmwareUpgradeController::usbSerialDownloadFirmware() {
    if( _selectedFirmware.size() <= 0){
        _appendStatusLog("You must select upgrade firmware", true);
        return ;
    }

    QString newVersion = "unknow version";
    QString boardVersion = "unknow version";
    switch(_selectedFirmware.back().selectedFirmware){
    case Board:
        if(_selectedFirmware.back ().autopilotStackType == AutoPilotStackAPM){
            boardVersion = _apmFlightStackBoard;
        }else{
            boardVersion = _px4FlightStackBoard;
        }
        //boardVersion = _BoardVersion;
        break;
    case RemoteControl:
        boardVersion = _smartConsoleBoard;
        break;
    case GPS:
        boardVersion = _gpsBoard;
        break;
    case Battery:
        boardVersion = _batteryBoard;
        break;
    case Gimbal:
        boardVersion = _gimbalBoard;
        break;
    case ESpeedControl:
        boardVersion = _eSpeedControlBoard;
        break;
    case RaspBerry:
        boardVersion = _raspBerryBoard;
        break;
    }

    _firmwareVersion = false ;
    _deviceVersionCount = 0;
    FirmwareIdentifier firmwareId =_selectedFirmware.back();
    newVersion = firmwareId.firmwareVersion;
    QString varText = QString("BOARD Version: <font color=\"yellow\">%1</font>"
                              "   - - - - -   NEW Version: "
                              "<font color=\"yellow\">%2</font>").arg(boardVersion).arg(newVersion);
    _appendStatusLog(varText);
    if(firmwareId.selectedFirmware == Board && _foundBoardType != (QGCSerialPortInfo::BoardType_t)QGCSerialPortInfo::BoardTypeSerial
            && _foundBoardType != (QGCSerialPortInfo::BoardType_t)QGCSerialPortInfo::BoardTypeSikRadio){
        qDebug() << __FILE__ << " : " << __LINE__ << " : " << " : _selectedComponent : " << firmwareId.selectedFirmware;
#ifndef __android__
        startBoardSearch();
#else
        _getFirmwareFile(firmwareId);
#endif
    }else{
        _getFirmwareFile(firmwareId);
    }
}

/// @brief Called when the next firmware type to upgrade.
void FirmwareUpgradeController::_nextFirmwareUpgrade(bool sucess){

    if(_selectedFirmware.back ().selectedFirmware == RemoteControl && sucess){
        QMessageBox noNewVersionMsg(QMessageBox::Information,"Update for SmartConsole","You must restart your SmartConsole.",
                                     QMessageBox::Ok,NULL);
        if (noNewVersionMsg.exec()==QMessageBox::Ok){}
    }
    //first pop
    if(!_selectedFirmware.isEmpty()){
        _selectedFirmware.pop_back();
    }

    _appendStatusLog("\n\n");
    if( _selectedFirmware.size() <= 0){
        QString varText = QString("<font color=\"yellow\">All firmware update completed</font>");
        _appendStatusLog(varText);
        return ;
    }
    usbSerialDownloadFirmware();
}

/// @brief Search firmware version.
bool FirmwareUpgradeController::_getBoardFirmwareVersion(void){
    if(_firmwareVersionList[_deviceVersionCount] == RemoteControl || _firmwareVersionList[_deviceVersionCount] == RaspBerry){
        // smart console firmware version
        if(getFileManager()){
            _fileManager->stopAckTimeout();
        }
        if(_firmwareVersionList[_deviceVersionCount] == RemoteControl){
            if(_consoleUpgradeController){
                _consoleUpgradeController->stopAckTimeout();
                qDebug()<<__FILE__<<" : "<<__LINE__<< _firmwareVersionList[_deviceVersionCount];
                _consoleUpgradeController->_sendSearchVersionCommand(NULL, 0);
                _firmwareVersion = true ;
                return true;
            }
        }else {
            if(_raspBerryUpgradeController){
                _raspBerryUpgradeController->stopAckTimeout();
                qDebug()<<__FILE__<<" : "<<__LINE__<< _firmwareVersionList[_deviceVersionCount];
                _raspBerryUpgradeController->_sendSearchVersionCommand(NULL, 0);
                _firmwareVersion = true ;
                return true;
            }
        }
        return false;
    }
    if(!_fileManager){
        if(!_initFileManager()){
            return  false;
        }
    }
     _consoleUpgradeController->stopAckTimeout();
     _raspBerryUpgradeController->stopAckTimeout();

    _firmwareVersion = true ;
    if(getFileManager()){
        _fileManager->stopAckTimeout();
        _fileManager->_sendSearchVersionCommand( _firmwareVersionList[_deviceVersionCount]);
        return true;
    }
    return false;
}

/// @brief Show firmware version, get version from device
void FirmwareUpgradeController::_setFirmwareVersion(QString version , bool board) {
    Q_UNUSED(board);
    qDebug() << __FILE__ << " : " << __LINE__ << " : " << "version : " << version << " : " << _firmwareVersion
             << " : " << _deviceVersionCount << " : " << _firmwareVersionList[_deviceVersionCount];
    if(_firmwareVersion){
        _boardFirmareAvailableVersions(version);
        if(_deviceVersionCount + 1 < _firmwareVersionList.size() ){
            _deviceVersionCount++;
            _getBoardFirmwareVersion();
            return ;
        }
        _firmwareVersion = false ;
    }
}

/// @brief Get firmware version by parsing string, then show version on ui
void FirmwareUpgradeController::_boardFirmareAvailableVersions(const QString &version){
    if(version.isEmpty ()){
        return ;
    }
    bool ok = false;
    QGCSerialPortInfo::BoardType_t remoteBoardType = (QGCSerialPortInfo::BoardType_t)version.left(2).toInt(&ok);
//    qDebug()<<"remoteBoardType"<<remoteBoardType<<"ok"<<ok<<"*******************";
    if(!ok){
        remoteBoardType = (QGCSerialPortInfo::BoardType_t)AutoPilotStackPX4;
    }
    QString boardVersion = version.mid(6);
    QString px4boardVersion = version.left(6);
    if(_firmwareVersionList[_deviceVersionCount] == SelectedFirmwareType_t::Board){
        //_BoardVersion = version.mid(6);
        _remoteBoardType = (QGCSerialPortInfo::BoardType_t)version.left(2).toInt();
        _bootloaderBoardID = version.mid(2, 4).toInt();
//        qDebug()<<"_remoteBoardType"<<_remoteBoardType<<"_bootloaderBoardID"<<_bootloaderBoardID;
        if(_bootloaderBoardID != Bootloader::boardIDPX4FMUV4
                || _bootloaderBoardID != Bootloader::boardIDPX4FMUV1
                || _bootloaderBoardID != Bootloader::boardIDPX4Flow
                || _bootloaderBoardID != Bootloader::boardIDAeroCore
                || _bootloaderBoardID != Bootloader::boardID3DRRadio
                || _bootloaderBoardID != Bootloader::boardIDMINDPXFMUV2)
            _bootloaderBoardID = Bootloader::boardIDPX4FMUV4;
        switch(remoteBoardType){
        case AutoPilotStackType_t::PX4Flow :
        case AutoPilotStackType_t::AutoPilotStackPX4 :
            _px4FlightStackBoard = px4boardVersion;
            break;
        case AutoPilotStackType_t::AutoPilotStackAPM :
            _apmFlightStackBoard = boardVersion;
            break;
        case AutoPilotStackType_t::ThreeDRRadio :
            qWarning()<< "GroundControl can not supported this vehicle, can not find board type from fmu";
            break;
        case AutoPilotStackType_t::AerialVehicle :
            qWarning()<< "GroundControl can not supported this vehicle, can not find board type from fmu";
            break;
        default:
            qWarning()<< "GroundControl can not supported this vehicle, can not find board type from fmu";
            break;
        }
    }else{
        switch(_firmwareVersionList[_deviceVersionCount]){
        case SelectedFirmwareType_t::GPS :
            _gpsBoard = boardVersion;
            break;
        case SelectedFirmwareType_t::Gimbal :
            _gimbalBoard = boardVersion;
            break;
        case SelectedFirmwareType_t::Battery :
            _batteryBoard = boardVersion;
            break;
        case SelectedFirmwareType_t::RemoteControl :
            //_remoteConsoleBoard = boardVersion;
            _smartConsoleBoard = version;
            break;
        case SelectedFirmwareType_t::ESpeedControl :
            _eSpeedControlBoard = boardVersion;
            break;
        case SelectedFirmwareType_t::RaspBerry :
            _raspBerryBoard = version;
            break;
        default:
            qWarning()<< "GroundControl can not supported this vehicle, can not find selected type from fmu";
            break;
        }
    }
    emit boardAvailableVersionsChanged();
}

bool FirmwareUpgradeController::_initFileManager(void){
    if(!_fileManager){
        MultiVehicleManager * multiVehicle = qgcApp()->toolbox()->multiVehicleManager();
        if( !multiVehicle || ! (multiVehicle->activeVehicle())){
            qWarning()<< ("Please plugin your vechile");
            return false;
        }
        UAS *uas_tmp = multiVehicle->activeVehicle()->uas();
        if(!uas_tmp){
            qWarning()<<("Please plugin your vechile");
            return false;
        }
        _fileManager = uas_tmp->getFileManager();
        connect(_fileManager,          &FileManager::getVersion,                                    this, &FirmwareUpgradeController::_setFirmwareVersion);
        connect(_fileManager,          &FileManager::uploadProgram,                                 this, &FirmwareUpgradeController::_downloadProgress);
        connect(_fileManager,          &FileManager::appendStatusLog,                               this, &FirmwareUpgradeController::_appendStatusLog);
        connect(_fileManager,          &FileManager::nextFirmwareUpgrade,                           this, &FirmwareUpgradeController::_nextFirmwareUpgrade);
    }
    return true;
}

FileManager* FirmwareUpgradeController::getFileManager(){
    MultiVehicleManager * multiVehicle = qgcApp()->toolbox()->multiVehicleManager();
    if( !multiVehicle || ! (multiVehicle->activeVehicle())){
        return NULL;
    }
    UAS *uas_tmp = multiVehicle->activeVehicle()->uas();
    if(!uas_tmp){
        return NULL;
    }
    _fileManager = uas_tmp->getFileManager();
    return _fileManager;
}

