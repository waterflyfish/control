﻿/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "Vehicle.h"
#include "MAVLinkProtocol.h"
#include "FirmwarePluginManager.h"
#include "LinkManager.h"
#include "FirmwarePlugin.h"
#include "AutoPilotPluginManager.h"
#include "UAS.h"
#include "JoystickManager.h"
#include "MissionManager.h"
#include "GeoFenceManager.h"
#include "RallyPointManager.h"
#include "CoordinateVector.h"
#include "ParameterManager.h"
#include "QGCApplication.h"
#include "QGCImageProvider.h"
#include "GAudioOutput.h"
#include "FollowMe.h"
#include "MissionCommandTree.h"
#include "QGroundControlQmlGlobal.h"

//=========================================
#include <QSettings>
QGC_LOGGING_CATEGORY(VehicleLog, "VehicleLog")

#define UPDATE_TIMER 50
#define DEFAULT_LAT  38.965767f
#define DEFAULT_LON -120.083923f

extern const char* guided_mode_not_supported_by_vehicle;

const char* Vehicle::_settingsGroup =               "Vehicle%1";        // %1 replaced with mavlink system id
const char* Vehicle::_joystickModeSettingsKey =     "JoystickMode";
const char* Vehicle::_joystickEnabledSettingsKey =  "JoystickEnabled";

const char* Vehicle::_rollFactName =                "roll";
const char* Vehicle::_pitchFactName =               "pitch";
const char* Vehicle::_headingFactName =             "heading";
const char* Vehicle::_airSpeedFactName =            "airSpeed";
const char* Vehicle::_groundSpeedFactName =         "groundSpeed";
const char* Vehicle::_climbRateFactName =           "climbRate";
const char* Vehicle::_altitudeRelativeFactName =    "altitudeRelative";
const char* Vehicle::_altitudeAMSLFactName =        "altitudeAMSL";

const char* Vehicle::_gpsFactGroupName =        "gps";
const char* Vehicle::_batteryFactGroupName =    "battery";
const char* Vehicle::_windFactGroupName =       "wind";
const char* Vehicle::_vibrationFactGroupName =  "vibration";

const int Vehicle::_lowBatteryAnnounceRepeatMSecs = 30 * 1000;

Vehicle::Vehicle(LinkInterface*             link,
                 int                        vehicleId,
                 MAV_AUTOPILOT              firmwareType,
                 MAV_TYPE                   vehicleType,
                 FirmwarePluginManager*     firmwarePluginManager,
                 AutoPilotPluginManager*    autopilotPluginManager,
                 JoystickManager*           joystickManager)
    : FactGroup(_vehicleUIUpdateRateMSecs, loadseet())
    , _id(vehicleId)
    , _active(false)
    , _offlineEditingVehicle(false)
    , _firmwareType(firmwareType)
    , _vehicleType(vehicleType)
    , _firmwarePlugin(NULL)
    , _autopilotPlugin(NULL)
    , _mavlink(NULL)
    , _soloFirmware(false)
    , _joystickMode(JoystickModeRC)
    , _joystickEnabled(false)
    , _uas(NULL)
    , _coordinate(37.803784, -122.462276)
    , _coordinateValid(false)
    , _homePositionAvailable(false)
    , _mav(NULL)
    , _currentMessageCount(0)
    , _messageCount(0)
    , _currentErrorCount(0)
    , _currentWarningCount(0)
    , _currentNormalCount(0)
    , _currentMessageType(MessageNone)
    , _navigationAltitudeError(0.0f)
    , _navigationSpeedError(0.0f)
    , _navigationCrosstrackError(0.0f)
    , _navigationTargetBearing(0.0f)
    , _refreshTimer(new QTimer(this))
    , _updateCount(0)
    , _rcRSSI(255)
    , _rcRSSIstore(255)
    , _autoDisconnect(false)
    , _flying(false)    
    , _vtolfw(false)
    , _onboardControlSensorsPresent(0)
    , _onboardControlSensorsEnabled(0)
    , _onboardControlSensorsHealth(0)
    , _onboardControlSensorsUnhealthy(0)
    , _connectionLost(false)
    , _connectionLostEnabled(true)
    , _missionManager(NULL)
    , _tx1Manager(NULL)
    , _missionManagerInitialRequestSent(false)
    , _geoFenceManager(NULL)
    , _geoFenceManagerInitialRequestSent(false)
    , _rallyPointManager(NULL)
    , _rallyPointManagerInitialRequestSent(false)
    , _parameterManager(NULL)
    , _armed(false)
    , _base_mode(0)
    , _custom_mode(0)
    , _isKeyPressed(false)
    , _nextSendMessageMultipleIndex(0)
    , _firmwarePluginManager(firmwarePluginManager)
    , _autopilotPluginManager(autopilotPluginManager)
    , _joystickManager(joystickManager)
    , _flowImageIndex(0)
    , _allLinksInactiveSent(false)
    , _messagesReceived(0)
    , _messagesSent(0)
    , _messagesLost(0)
    , _messageSeq(0)
    , _compID(0)
    , _heardFrom(false)
    , _firmwareMajorVersion(versionNotSetValue)
    , _firmwareMinorVersion(versionNotSetValue)
    , _firmwarePatchVersion(versionNotSetValue)
    , _firmwareVersionType(FIRMWARE_VERSION_TYPE_OFFICIAL)
    , _rollFact             (0, _rollFactName,              FactMetaData::valueTypeDouble)
    , _pitchFact            (0, _pitchFactName,             FactMetaData::valueTypeDouble)
    , _headingFact          (0, _headingFactName,           FactMetaData::valueTypeDouble)
    , _groundSpeedFact      (0, _groundSpeedFactName,       FactMetaData::valueTypeDouble)
    , _airSpeedFact         (0, _airSpeedFactName,          FactMetaData::valueTypeDouble)
    , _climbRateFact        (0, _climbRateFactName,         FactMetaData::valueTypeDouble)
    , _altitudeRelativeFact (0, _altitudeRelativeFactName,  FactMetaData::valueTypeDouble)
    , _altitudeAMSLFact     (0, _altitudeAMSLFactName,      FactMetaData::valueTypeDouble)
    , _gpsFactGroup(this)
    , _batteryFactGroup(this)
    , _windFactGroup(this)
    , _vibrationFactGroup(this)
{
    _addLink(link);

    _mavlink = qgcApp()->toolbox()->mavlinkProtocol();

    connect(_mavlink, &MAVLinkProtocol::messageReceived,     this, &Vehicle::_mavlinkMessageReceived);

    connect(this, &Vehicle::_sendMessageOnLinkOnThread, this, &Vehicle::_sendMessageOnLink, Qt::QueuedConnection);
    connect(this, &Vehicle::flightModeChanged,          this, &Vehicle::_handleFlightModeChanged);
    connect(this, &Vehicle::armedChanged,               this, &Vehicle::_announceArmedChanged);
    _uas = new UAS(_mavlink, this, _firmwarePluginManager);

    setLatitude(_uas->getLatitude());
    setLongitude(_uas->getLongitude());

    connect(_uas, &UAS::latitudeChanged,                this, &Vehicle::setLatitude);
    connect(_uas, &UAS::longitudeChanged,               this, &Vehicle::setLongitude);
    connect(_uas, &UAS::imageReady,                     this, &Vehicle::_imageReady);
    connect(this, &Vehicle::remoteControlRSSIChanged,   this, &Vehicle::_remoteControlRSSIChanged);

    _firmwarePlugin     = _firmwarePluginManager->firmwarePluginForAutopilot(_firmwareType, _vehicleType);
    _autopilotPlugin    = _autopilotPluginManager->newAutopilotPluginForVehicle(this);

    // connect this vehicle to the follow me handle manager
    connect(this, &Vehicle::flightModeChanged,qgcApp()->toolbox()->followMe(), &FollowMe::followMeHandleManager);
    _mapFollowMeHaveFirstCoordinate = false;
    _mapFollowMeList.clear();
    // Refresh timer
    connect(_refreshTimer, &QTimer::timeout, this, &Vehicle::_checkUpdate);
    _refreshTimer->setInterval(UPDATE_TIMER);
    _refreshTimer->start(UPDATE_TIMER);

    // PreArm Error self-destruct timer
    connect(&_prearmErrorTimer, &QTimer::timeout, this, &Vehicle::_prearmErrorTimeout);
    _prearmErrorTimer.setInterval(_prearmErrorTimeoutMSecs);
    _prearmErrorTimer.setSingleShot(true);

    // Connection Lost time
    _connectionLostTimer.setInterval(Vehicle::_connectionLostTimeoutMSecs);
    _connectionLostTimer.setSingleShot(false);
    _connectionLostTimer.start();
    connect(&_connectionLostTimer, &QTimer::timeout, this, &Vehicle::_connectionLostTimeout);

    _mav = uas();

    // Listen for system messages
    connect(qgcApp()->toolbox()->uasMessageHandler(), &UASMessageHandler::textMessageCountChanged,  this, &Vehicle::_handleTextMessage);
    connect(qgcApp()->toolbox()->uasMessageHandler(), &UASMessageHandler::textMessageReceived,      this, &Vehicle::_handletextMessageReceived);
    // Now connect the new UAS
    connect(_mav, SIGNAL(attitudeChanged                    (UASInterface*,double,double,double,quint64)),              this, SLOT(_updateAttitude(UASInterface*, double, double, double, quint64)));
    connect(_mav, SIGNAL(attitudeChanged                    (UASInterface*,int,double,double,double,quint64)),          this, SLOT(_updateAttitude(UASInterface*,int,double, double, double, quint64)));
    connect(_mav, SIGNAL(statusChanged                      (UASInterface*,QString,QString)),                           this, SLOT(_updateState(UASInterface*, QString,QString)));

    connect(_mav, &UASInterface::speedChanged, this, &Vehicle::_updateSpeed);
    connect(_mav, &UASInterface::altitudeChanged, this, &Vehicle::_updateAltitude);
    connect(_mav, &UASInterface::navigationControllerErrorsChanged,this, &Vehicle::_updateNavigationControllerErrors);
    connect(_mav, &UASInterface::NavigationControllerDataChanged,   this, &Vehicle::_updateNavigationControllerData);

    _loadSettings();

    _missionManager = new MissionManager(this);
    connect(_missionManager, &MissionManager::error,                    this, &Vehicle::_missionManagerError);
    connect(_missionManager, &MissionManager::newMissionItemsAvailable, this, &Vehicle::_newMissionItemsAvailable);

    _parameterManager = new ParameterManager(this);
    connect(_parameterManager, &ParameterManager::parametersReadyChanged, this, &Vehicle::_parametersReady);

    _tx1Manager = new Tx1Manager(this);
    // GeoFenceManager needs to access ParameterManager so make sure to create after
    _geoFenceManager = _firmwarePlugin->newGeoFenceManager(this);
    connect(_geoFenceManager, &GeoFenceManager::error, this, &Vehicle::_geoFenceManagerError);
    connect(_geoFenceManager, &GeoFenceManager::loadComplete, this, &Vehicle::_newGeoFenceAvailable);

    _rallyPointManager = _firmwarePlugin->newRallyPointManager(this);
    connect(_rallyPointManager, &RallyPointManager::error, this, &Vehicle::_rallyPointManagerError);

    // Ask the vehicle for firmware version info. This must be MAV_COMP_ID_ALL since we don't know default component id yet.

    mavlink_message_t       versionMsg;
    mavlink_command_long_t  versionCmd;

    versionCmd.command = MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES;
    versionCmd.confirmation = 0;
    versionCmd.param1 = 1; // Request firmware version
    versionCmd.param2 = versionCmd.param3 = versionCmd.param4 = versionCmd.param5 = versionCmd.param6 = versionCmd.param7 = 0;
    versionCmd.target_system = id();
    versionCmd.target_component = MAV_COMP_ID_ALL;
    mavlink_msg_command_long_encode_chan(_mavlink->getSystemId(),
                                         _mavlink->getComponentId(),
                                         priorityLink()->mavlinkChannel(),
                                         &versionMsg,
                                         &versionCmd);
    sendMessageMultiple(versionMsg);

    _firmwarePlugin->initializeVehicle(this);

    _sendMultipleTimer.start(_sendMessageMultipleIntraMessageDelay);
    connect(&_sendMultipleTimer, &QTimer::timeout, this, &Vehicle::_sendMessageMultipleNext);

    _mapTrajectoryTimer.setInterval(_mapTrajectoryMsecsBetweenPoints);
    connect(&_mapTrajectoryTimer, &QTimer::timeout, this, &Vehicle::_addNewMapTrajectoryPoint);

    // Invalidate the timer to signal first announce
    _lowBatteryAnnounceTimer.invalidate();

    // Build FactGroup object model

    _addFact(&_rollFact,                _rollFactName);
    _addFact(&_pitchFact,               _pitchFactName);
    _addFact(&_headingFact,             _headingFactName);
    _addFact(&_groundSpeedFact,         _groundSpeedFactName);
    _addFact(&_airSpeedFact,            _airSpeedFactName);
    _addFact(&_climbRateFact,           _climbRateFactName);
    _addFact(&_altitudeRelativeFact,    _altitudeRelativeFactName);
    _addFact(&_altitudeAMSLFact,        _altitudeAMSLFactName);

    _addFactGroup(&_gpsFactGroup,       _gpsFactGroupName);
    _addFactGroup(&_batteryFactGroup,   _batteryFactGroupName);
    _addFactGroup(&_windFactGroup,      _windFactGroupName);
    _addFactGroup(&_vibrationFactGroup, _vibrationFactGroupName);

    _gpsFactGroup.setVehicle(this);
    _batteryFactGroup.setVehicle(this);
    _windFactGroup.setVehicle(this);
    _vibrationFactGroup.setVehicle(this);
}

// Disconnected Vehicle for offline editing
Vehicle::Vehicle(MAV_AUTOPILOT              firmwareType,
                 MAV_TYPE                   vehicleType,
                 FirmwarePluginManager*     firmwarePluginManager,
                 QObject*                   parent)
    : FactGroup(_vehicleUIUpdateRateMSecs, loadseet(), parent)
    , _id(0)
    , _active(false)
    , _offlineEditingVehicle(true)
    , _firmwareType(firmwareType)
    , _vehicleType(vehicleType)
    , _firmwarePlugin(NULL)
    , _autopilotPlugin(NULL)
    , _joystickMode(JoystickModeRC)
    , _joystickEnabled(false)
    , _uas(NULL)
    , _coordinate(37.803784, -122.462276)
    , _coordinateValid(false)
    , _homePositionAvailable(false)
    , _mav(NULL)
    , _currentMessageCount(0)
    , _messageCount(0)
    , _currentErrorCount(0)
    , _currentWarningCount(0)
    , _currentNormalCount(0)
    , _currentMessageType(MessageNone)
    , _navigationAltitudeError(0.0f)
    , _navigationSpeedError(0.0f)
    , _navigationCrosstrackError(0.0f)
    , _navigationTargetBearing(0.0f)
    , _refreshTimer(new QTimer(this))
    , _updateCount(0)
    , _rcRSSI(255)
    , _rcRSSIstore(255)
    , _autoDisconnect(false)
    , _flying(false)  
    , _vtolfw(false)
    , _onboardControlSensorsPresent(0)
    , _onboardControlSensorsEnabled(0)
    , _onboardControlSensorsHealth(0)
    , _onboardControlSensorsUnhealthy(0)
    , _connectionLost(false)
    , _connectionLostEnabled(true)
    , _missionManager(NULL)
    , _tx1Manager(NULL)
    , _missionManagerInitialRequestSent(false)
    , _geoFenceManager(NULL)
    , _geoFenceManagerInitialRequestSent(false)
    , _rallyPointManager(NULL)
    , _rallyPointManagerInitialRequestSent(false)
    , _parameterManager(NULL)
    , _armed(false)
    , _base_mode(0)
    , _custom_mode(0)
    , _nextSendMessageMultipleIndex(0)
    , _firmwarePluginManager(firmwarePluginManager)
    , _autopilotPluginManager(NULL)
    , _joystickManager(NULL)
    , _flowImageIndex(0)
    , _allLinksInactiveSent(false)
    , _messagesReceived(0)
    , _messagesSent(0)
    , _messagesLost(0)
    , _messageSeq(0)
    , _compID(0)
    , _heardFrom(false)
    , _firmwareMajorVersion(versionNotSetValue)
    , _firmwareMinorVersion(versionNotSetValue)
    , _firmwarePatchVersion(versionNotSetValue)
    , _rollFact             (0, _rollFactName,              FactMetaData::valueTypeDouble)
    , _pitchFact            (0, _pitchFactName,             FactMetaData::valueTypeDouble)
    , _headingFact          (0, _headingFactName,           FactMetaData::valueTypeDouble)
    , _groundSpeedFact      (0, _groundSpeedFactName,       FactMetaData::valueTypeDouble)
    , _airSpeedFact         (0, _airSpeedFactName,          FactMetaData::valueTypeDouble)
    , _climbRateFact        (0, _climbRateFactName,         FactMetaData::valueTypeDouble)
    , _altitudeRelativeFact (0, _altitudeRelativeFactName,  FactMetaData::valueTypeDouble)
    , _altitudeAMSLFact     (0, _altitudeAMSLFactName,      FactMetaData::valueTypeDouble)
    , _gpsFactGroup(this)
    , _batteryFactGroup(this)
    , _windFactGroup(this)
    , _vibrationFactGroup(this)
{
    _firmwarePlugin = _firmwarePluginManager->firmwarePluginForAutopilot(_firmwareType, _vehicleType);
    _firmwarePlugin->initializeVehicle(this);

    _missionManager = new MissionManager(this);
    connect(_missionManager, &MissionManager::error, this, &Vehicle::_missionManagerError);

    _parameterManager = new ParameterManager(this);
#ifdef QGC_TX1_TEST_UDP
    _tx1Manager = new Tx1Manager(this);
#endif
    // GeoFenceManager needs to access ParameterManager so make sure to create after
    _geoFenceManager = _firmwarePlugin->newGeoFenceManager(this);
    connect(_geoFenceManager, &GeoFenceManager::error, this, &Vehicle::_geoFenceManagerError);

    _rallyPointManager = _firmwarePlugin->newRallyPointManager(this);
    connect(_rallyPointManager, &RallyPointManager::error, this, &Vehicle::_rallyPointManagerError);

    // Build FactGroup object model

    _addFact(&_rollFact,                _rollFactName);
    _addFact(&_pitchFact,               _pitchFactName);
    _addFact(&_headingFact,             _headingFactName);
    _addFact(&_groundSpeedFact,         _groundSpeedFactName);
    _addFact(&_airSpeedFact,            _airSpeedFactName);
    _addFact(&_climbRateFact,           _climbRateFactName);
    _addFact(&_altitudeRelativeFact,    _altitudeRelativeFactName);
    _addFact(&_altitudeAMSLFact,        _altitudeAMSLFactName);

    _addFactGroup(&_gpsFactGroup,       _gpsFactGroupName);
    _addFactGroup(&_batteryFactGroup,   _batteryFactGroupName);
    _addFactGroup(&_windFactGroup,      _windFactGroupName);
    _addFactGroup(&_vibrationFactGroup, _vibrationFactGroupName);

    _gpsFactGroup.setVehicle(NULL);
    _batteryFactGroup.setVehicle(NULL);
    _windFactGroup.setVehicle(NULL);
    _vibrationFactGroup.setVehicle(NULL);
}

Vehicle::~Vehicle()
{
    qCDebug(VehicleLog) << "~Vehicle" << this;

    delete _missionManager;
    _missionManager = NULL;

    delete _autopilotPlugin;
    _autopilotPlugin = NULL;

    delete _mav;
    _mav = NULL;

}

void
Vehicle::resetCounters()
{
    _messagesReceived   = 0;
    _messagesSent       = 0;
    _messagesLost       = 0;
    _messageSeq         = 0;
    _heardFrom          = false;
}

void Vehicle::_mavlinkMessageReceived(LinkInterface* link, mavlink_message_t message)
{

    if (message.sysid != _id && message.sysid != 0) {
        return;
    }

    if (!_containsLink(link)) {
        _addLink(link);
    }

    //-- Check link status
    _messagesReceived++;
    emit messagesReceivedChanged();
    if(!_heardFrom) {
        if(message.msgid == MAVLINK_MSG_ID_HEARTBEAT) {
            _heardFrom = true;
            _compID = message.compid;
            _messageSeq = message.seq + 1;
        }
    } else {
        if(_compID == message.compid) {
            uint16_t seq_received = (uint16_t)message.seq;
            uint16_t packet_lost_count = 0;
            //-- Account for overflow during packet loss
            if(seq_received < _messageSeq) {
                packet_lost_count = (seq_received + 255) - _messageSeq;
            } else {
                packet_lost_count = seq_received - _messageSeq;
            }
            _messageSeq = message.seq + 1;
            _messagesLost += packet_lost_count;
            if(packet_lost_count)
                emit messagesLostChanged();
        }
    }

    // Give the plugin a change to adjust the message contents
    if (!_firmwarePlugin->adjustIncomingMavlinkMessage(this, &message)) {
        return;
    }

    switch (message.msgid) {
    case MAVLINK_MSG_ID_HOME_POSITION:
        _handleHomePosition(message);
        break;
    case MAVLINK_MSG_ID_HEARTBEAT:
        _handleHeartbeat(message);
        break;
    case MAVLINK_MSG_ID_RC_CHANNELS:
        _handleRCChannels(message);
        break;
    case MAVLINK_MSG_ID_RC_CHANNELS_RAW:
        _handleRCChannelsRaw(message);
        break;
    case MAVLINK_MSG_ID_BATTERY_STATUS:
        _handleBatteryStatus(message);
        break;
    case MAVLINK_MSG_ID_SYS_STATUS:
        _handleSysStatus(message);
        break;
    case MAVLINK_MSG_ID_RAW_IMU:
        emit mavlinkRawImu(message);
        break;
    case MAVLINK_MSG_ID_SCALED_IMU:
        emit mavlinkScaledImu1(message);
        break;
    case MAVLINK_MSG_ID_SCALED_IMU2:
        emit mavlinkScaledImu2(message);
        break;
    case MAVLINK_MSG_ID_SCALED_IMU3:
        emit mavlinkScaledImu3(message);
        break;
    case MAVLINK_MSG_ID_VIBRATION:
        _handleVibration(message);
        break;
    case MAVLINK_MSG_ID_EXTENDED_SYS_STATE:
        _handleExtendedSysState(message);
        break;
    case MAVLINK_MSG_ID_COMMAND_ACK:
        _handleCommandAck(message);
        break;
    case MAVLINK_MSG_ID_AUTOPILOT_VERSION:
        _handleAutopilotVersion(link, message);
        break;
    case MAVLINK_MSG_ID_WIND_COV:
        _handleWindCov(message);
        break;
    case MAVLINK_MSG_ID_HIL_ACTUATOR_CONTROLS:
        _handleHilActuatorControls(message);
        break;
    case MAVLINK_MSG_ID_LOGGING_DATA:
        _handleMavlinkLoggingData(message);
        break;
    case MAVLINK_MSG_ID_LOGGING_DATA_ACKED:
        _handleMavlinkLoggingDataAcked(message);
        break;

    // Following are ArduPilot dialect messages

    case MAVLINK_MSG_ID_WIND:
        _handleWind(message);
        break;
    //Camera FeedBack
    case MAVLINK_MSG_ID_CAMERA_FEEDBACK:
        emit mavlinkCameraFeedBack(message);
        break;
    }

    emit mavlinkMessageReceived(message);

    _uas->receiveMessage(message);
}

void Vehicle::_handleAutopilotVersion(LinkInterface *link, mavlink_message_t& message)
{
    mavlink_autopilot_version_t autopilotVersion;
    mavlink_msg_autopilot_version_decode(&message, &autopilotVersion);

    bool isMavlink2 = (autopilotVersion.capabilities & MAV_PROTOCOL_CAPABILITY_MAVLINK2) != 0;
    if(isMavlink2) {
        mavlink_status_t* mavlinkStatus = mavlink_get_channel_status(link->mavlinkChannel());
        mavlinkStatus->flags &= ~MAVLINK_STATUS_FLAG_OUT_MAVLINK1;
    }

    if (autopilotVersion.flight_sw_version != 0) {
        int majorVersion, minorVersion, patchVersion;
        FIRMWARE_VERSION_TYPE versionType;

        majorVersion = (autopilotVersion.flight_sw_version >> (8*3)) & 0xFF;
        minorVersion = (autopilotVersion.flight_sw_version >> (8*2)) & 0xFF;
        patchVersion = (autopilotVersion.flight_sw_version >> (8*1)) & 0xFF;
        versionType = (FIRMWARE_VERSION_TYPE)((autopilotVersion.flight_sw_version >> (8*0)) & 0xFF);
        _flightCustomVersion.clear ();
        for(int i = sizeof(autopilotVersion.flight_custom_version) - 1; i >= 0 ; i--)
        {
            _flightCustomVersion.append ( QString("%1").arg(uint(autopilotVersion.flight_custom_version[i]),2, 16, QChar('0') ));
        }
        setFirmwareVersion(majorVersion, minorVersion, patchVersion, versionType);
    }
}

void Vehicle::_handleHilActuatorControls(mavlink_message_t &message)
{
    mavlink_hil_actuator_controls_t hil;
    mavlink_msg_hil_actuator_controls_decode(&message, &hil);
    emit hilActuatorControlsChanged(hil.time_usec, hil.flags,
                                    hil.controls[0],
                                    hil.controls[1],
                                    hil.controls[2],
                                    hil.controls[3],
                                    hil.controls[4],
                                    hil.controls[5],
                                    hil.controls[6],
                                    hil.controls[7],
                                    hil.controls[8],
                                    hil.controls[9],
                                    hil.controls[10],
                                    hil.controls[11],
                                    hil.controls[12],
                                    hil.controls[13],
                                    hil.controls[14],
                                    hil.controls[15],
                                    hil.mode);
}

void Vehicle::_handleCommandAck(mavlink_message_t& message)
{
    mavlink_command_ack_t ack;
    mavlink_msg_command_ack_decode(&message, &ack);

    emit commandLongAck(message.compid, ack.command, ack.result);

    // Disregard failures for these (handled above)
    switch (ack.command) {
        case MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES:
        case MAV_CMD_LOGGING_START:
        case MAV_CMD_LOGGING_STOP:
            return;
        default:
            break;
    }

    QString commandName = qgcApp()->toolbox()->missionCommandTree()->friendlyName((MAV_CMD)ack.command);

    switch (ack.result) {
    case MAV_RESULT_TEMPORARILY_REJECTED:
        qgcApp()->showMessage(tr("%1 command temporarily rejected").arg(commandName));
        break;
    case MAV_RESULT_DENIED:
        qgcApp()->showMessage(tr("%1 command denied").arg(commandName));
        break;
    case MAV_RESULT_UNSUPPORTED:
        qgcApp()->showMessage(tr("%1 command not supported").arg(commandName));
        break;
    case MAV_RESULT_FAILED:
        qgcApp()->showMessage(tr("%1 command failed").arg(commandName));
        break;
    default:
        // Do nothing
        break;
    }
}

void Vehicle::_handleExtendedSysState(mavlink_message_t& message)
{
    mavlink_extended_sys_state_t extendedState;
    mavlink_msg_extended_sys_state_decode(&message, &extendedState);

    switch (extendedState.landed_state) {
    case MAV_LANDED_STATE_UNDEFINED:
        break;
    case MAV_LANDED_STATE_ON_GROUND:
        setFlying(false);
        break;
    case MAV_LANDED_STATE_IN_AIR:
        setFlying(true);
        //return;
        break;
    }

    if(!vtol()){
        return ;
    }

    switch(extendedState.vtol_state){
    case MAV_VTOL_STATE_UNDEFINED: /* MAV is not configured as VTOL | */
        break;
    case MAV_VTOL_STATE_TRANSITION_TO_FW: /* VTOL is in transition from multicopter to fixed-wing | */
        break;
    case MAV_VTOL_STATE_TRANSITION_TO_MC: /* VTOL is in transition from fixed-wing to multicopter | */
        break;
    case MAV_VTOL_STATE_MC: /* VTOL is in multicopter state | */
        if(_vtolfw){
        _vtolfw = false;
        }
        emit vehicletypeChanged(2);
        break;
    case  MAV_VTOL_STATE_FW:  /* VTOL is in fixed-wing state | */
        if(!_vtolfw){
        _vtolfw = true;
        }
        emit vehicletypeChanged(3);
        break;
    case MAV_VTOL_STATE_ENUM_END:
        break;
    }
}

void Vehicle::_handleVibration(mavlink_message_t& message)
{
    mavlink_vibration_t vibration;
    mavlink_msg_vibration_decode(&message, &vibration);

    _vibrationFactGroup.xAxis()->setRawValue(vibration.vibration_x);
    _vibrationFactGroup.yAxis()->setRawValue(vibration.vibration_y);
    _vibrationFactGroup.zAxis()->setRawValue(vibration.vibration_z);
    _vibrationFactGroup.clipCount1()->setRawValue(vibration.clipping_0);
    _vibrationFactGroup.clipCount2()->setRawValue(vibration.clipping_1);
    _vibrationFactGroup.clipCount3()->setRawValue(vibration.clipping_2);
}

void Vehicle::_handleWindCov(mavlink_message_t& message)
{
    mavlink_wind_cov_t wind;
    mavlink_msg_wind_cov_decode(&message, &wind);

    float direction = qRadiansToDegrees(qAtan2(wind.wind_y, wind.wind_x));
    float speed = qSqrt(qPow(wind.wind_x, 2) + qPow(wind.wind_y, 2));

    _windFactGroup.direction()->setRawValue(direction);
    _windFactGroup.speed()->setRawValue(speed);
    _windFactGroup.verticalSpeed()->setRawValue(0);
}

void Vehicle::_handleWind(mavlink_message_t& message)
{
    mavlink_wind_t wind;
    mavlink_msg_wind_decode(&message, &wind);

    _windFactGroup.direction()->setRawValue(wind.direction);
    _windFactGroup.speed()->setRawValue(wind.speed);
    _windFactGroup.verticalSpeed()->setRawValue(wind.speed_z);
}

void Vehicle::_handleSysStatus(mavlink_message_t& message)
{
    mavlink_sys_status_t sysStatus;
    mavlink_msg_sys_status_decode(&message, &sysStatus);
    QSettings setting;
    bool m = setting.value("language",false).toBool();

    if (sysStatus.current_battery == -1) {
        _batteryFactGroup.current()->setRawValue(VehicleBatteryFactGroup::_currentUnavailable);
    } else {
        _batteryFactGroup.current()->setRawValue((double)sysStatus.current_battery * 10);
    }
    if (sysStatus.voltage_battery == UINT16_MAX) {
        _batteryFactGroup.voltage()->setRawValue(VehicleBatteryFactGroup::_voltageUnavailable);
    } else {
        _batteryFactGroup.voltage()->setRawValue((double)sysStatus.voltage_battery / 1000.0);
    }
    _batteryFactGroup.percentRemaining()->setRawValue(sysStatus.battery_remaining);

    if (sysStatus.battery_remaining > 0 && sysStatus.battery_remaining < QGroundControlQmlGlobal::batteryPercentRemainingAnnounce()->rawValue().toInt()) {
        if (!_lowBatteryAnnounceTimer.isValid() || _lowBatteryAnnounceTimer.elapsed() > _lowBatteryAnnounceRepeatMSecs) {
            _lowBatteryAnnounceTimer.restart();
            if(m)_say(QString("%1 low battery: %2 percent remaining").arg(_vehicleIdSpeech()).arg(sysStatus.battery_remaining));
            if(!m)_say(QString("%1 低电量: %2 剩余").arg(_vehicleIdSpeech()).arg(sysStatus.battery_remaining));
        }
    }

    _onboardControlSensorsPresent = sysStatus.onboard_control_sensors_present;
    _onboardControlSensorsEnabled = sysStatus.onboard_control_sensors_enabled;
    _onboardControlSensorsHealth = sysStatus.onboard_control_sensors_health;

    uint32_t newSensorsUnhealthy = _onboardControlSensorsEnabled & ~_onboardControlSensorsHealth;
    if (newSensorsUnhealthy != _onboardControlSensorsUnhealthy) {
        _onboardControlSensorsUnhealthy = newSensorsUnhealthy;
        emit unhealthySensorsChanged();
    }
}

void Vehicle::_handleBatteryStatus(mavlink_message_t& message)
{
    mavlink_battery_status_t bat_status;
    mavlink_msg_battery_status_decode(&message, &bat_status);

    if (bat_status.temperature == INT16_MAX) {
        _batteryFactGroup.temperature()->setRawValue(VehicleBatteryFactGroup::_temperatureUnavailable);
    } else {
        _batteryFactGroup.temperature()->setRawValue((double)bat_status.temperature / 100.0);
    }
    if (bat_status.current_consumed == -1) {
        _batteryFactGroup.mahConsumed()->setRawValue(VehicleBatteryFactGroup::_mahConsumedUnavailable);
    } else {
        _batteryFactGroup.mahConsumed()->setRawValue(bat_status.current_consumed);
    }

    int cellCount = 0;
    for (int i=0; i<10; i++) {
        if (bat_status.voltages[i] != UINT16_MAX) {
            cellCount++;
        }
    }
    if (cellCount == 0) {
        cellCount = -1;
    }

    _batteryFactGroup.cellCount()->setRawValue(cellCount);

    if(parameterExists(-1,"HY_ENGINE_EN")){
        Fact* fact = getParameterFact(-1, "HY_ENGINE_EN");
        int enable = fact->rawValue().value<int>();
        if(!enable){
            _batteryFactGroup.voltages()->setRawValue(VehicleBatteryFactGroup::_voltagesUnavailable);
        }else{
             _batteryFactGroup.voltages()->setRawValue(bat_status.voltages[9] * 10);
        }
    }else{
        _batteryFactGroup.voltages()->setRawValue(VehicleBatteryFactGroup::_voltagesUnavailable);
    }
}

void Vehicle::_handleHomePosition(mavlink_message_t& message)
{
    bool emitHomePositionChanged =          false;
    bool emitHomePositionAvailableChanged = false;

    mavlink_home_position_t homePos;

    mavlink_msg_home_position_decode(&message, &homePos);

    QGeoCoordinate newHomePosition (homePos.latitude / 10000000.0,
                                    homePos.longitude / 10000000.0,
                                    homePos.altitude / 1000.0);
    if (!_homePositionAvailable || newHomePosition != _homePosition) {
        emitHomePositionChanged = true;
        _homePosition = newHomePosition;
    }

    if (!_homePositionAvailable) {
        emitHomePositionAvailableChanged = true;
        _homePositionAvailable = true;
    }

    if (emitHomePositionChanged) {
        qCDebug(VehicleLog) << "New home position" << newHomePosition;
        qgcApp()->setLastKnownHomePosition(_homePosition);
        emit homePositionChanged(_homePosition);
    }
    if (emitHomePositionAvailableChanged) {
        emit homePositionAvailableChanged(true);
    }
}

void Vehicle::_handleHeartbeat(mavlink_message_t& message)
{
    _connectionActive();

    mavlink_heartbeat_t heartbeat;

    mavlink_msg_heartbeat_decode(&message, &heartbeat);

    bool newArmed = heartbeat.base_mode & MAV_MODE_FLAG_DECODE_POSITION_SAFETY;

    if (_armed != newArmed) {
        _armed = newArmed;
        emit armedChanged(_armed);

        // We are transitioning to the armed state, begin tracking trajectory points for the map
        if (_armed) {
            _mapTrajectoryStart();
        } else {
            _mapTrajectoryStop();
        }
    }

    if (heartbeat.base_mode != _base_mode || heartbeat.custom_mode != _custom_mode) {
        _base_mode = heartbeat.base_mode;
        _custom_mode = heartbeat.custom_mode;
        emit flightModeChanged(flightMode());
        emit isPosCtlModeChanged (isPosCtlMode());
    }
}

void Vehicle::_handleRCChannels(mavlink_message_t& message)
{
    mavlink_rc_channels_t channels;

    mavlink_msg_rc_channels_decode(&message, &channels);

    uint16_t* _rgChannelvalues[cMaxRcChannels] = {
        &channels.chan1_raw,
        &channels.chan2_raw,
        &channels.chan3_raw,
        &channels.chan4_raw,
        &channels.chan5_raw,
        &channels.chan6_raw,
        &channels.chan7_raw,
        &channels.chan8_raw,
        &channels.chan9_raw,
        &channels.chan10_raw,
        &channels.chan11_raw,
        &channels.chan12_raw,
        &channels.chan13_raw,
        &channels.chan14_raw,
        &channels.chan15_raw,
        &channels.chan16_raw,
        &channels.chan17_raw,
        &channels.chan18_raw,
    };
    int pwmValues[cMaxRcChannels];

    for (int i=0; i<cMaxRcChannels; i++) {
        uint16_t channelValue = *_rgChannelvalues[i];

        if (i < channels.chancount) {
            pwmValues[i] = channelValue == UINT16_MAX ? -1 : channelValue;
        } else {
            pwmValues[i] = -1;
        }
    }

    emit remoteControlRSSIChanged(channels.rssi);
    emit rcChannelsChanged(channels.chancount, pwmValues);
}

void Vehicle::_handleRCChannelsRaw(mavlink_message_t& message)
{
    // We handle both RC_CHANNLES and RC_CHANNELS_RAW since different firmware will only
    // send one or the other.

    mavlink_rc_channels_raw_t channels;

    mavlink_msg_rc_channels_raw_decode(&message, &channels);

    uint16_t* _rgChannelvalues[cMaxRcChannels] = {
        &channels.chan1_raw,
        &channels.chan2_raw,
        &channels.chan3_raw,
        &channels.chan4_raw,
        &channels.chan5_raw,
        &channels.chan6_raw,
        &channels.chan7_raw,
        &channels.chan8_raw,
    };

    int pwmValues[cMaxRcChannels];
    int channelCount = 0;

    for (int i=0; i<cMaxRcChannels; i++) {
        pwmValues[i] = -1;
    }

    for (int i=0; i<8; i++) {
        uint16_t channelValue = *_rgChannelvalues[i];

        if (channelValue == UINT16_MAX) {
            pwmValues[i] = -1;
        } else {
            channelCount = i + 1;
            pwmValues[i] = channelValue;
        }
    }
    for (int i=9; i<18; i++) {
        pwmValues[i] = -1;
    }

    emit remoteControlRSSIChanged(channels.rssi);
    emit rcChannelsChanged(channelCount, pwmValues);
}

bool Vehicle::_containsLink(LinkInterface* link)
{
    return _links.contains(link);
}

void Vehicle::_addLink(LinkInterface* link)
{
    if (!_containsLink(link)) {
        _links += link;
        qCDebug(VehicleLog) << "_addLink:" << QString("%1").arg((ulong)link, 0, 16);
        connect(qgcApp()->toolbox()->linkManager(), &LinkManager::linkInactive, this, &Vehicle::_linkInactiveOrDeleted);
        connect(qgcApp()->toolbox()->linkManager(), &LinkManager::linkDeleted, this, &Vehicle::_linkInactiveOrDeleted);
    }
}

void Vehicle::_linkInactiveOrDeleted(LinkInterface* link)
{
    qCDebug(VehicleLog) << "_linkInactiveOrDeleted linkCount" << _links.count();

    _links.removeOne(link);

    if (_links.count() == 0 && !_allLinksInactiveSent) {
        qCDebug(VehicleLog) << "All links inactive";
        // Make sure to not send this more than one time
        _allLinksInactiveSent = true;
        emit allLinksInactive(this);
    }
}

bool Vehicle::sendMessageOnLink(LinkInterface* link, mavlink_message_t message)
{
    if (!link || !_links.contains(link) || !link->isConnected()) {
        return false;
    }
    emit _sendMessageOnLinkOnThread(link, message);

    return true;
}

void Vehicle::_sendMessageOnLink(LinkInterface* link, mavlink_message_t message)
{
    // Make sure this is still a good link
    if (!link || !_links.contains(link) || !link->isConnected()) {
        return;
    }

#if 0
    // Leaving in for ease in Mav 2.0 testing
    mavlink_status_t* mavlinkStatus = mavlink_get_channel_status(link->mavlinkChannel());
    qDebug() << "_sendMessageOnLink" << mavlinkStatus << link->mavlinkChannel() << mavlinkStatus->flags << message.magic;
#endif

    // Give the plugin a chance to adjust
    _firmwarePlugin->adjustOutgoingMavlinkMessage(this, link, &message);

    // Write message into buffer, prepending start sign
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
    int len = mavlink_msg_to_send_buffer(buffer, &message);

    link->writeBytesSafe((const char*)buffer, len);
    _messagesSent++;
    emit messagesSentChanged();
}

/// @return Direct usb connection link to board if one, NULL if none
LinkInterface* Vehicle::priorityLink(void)
{
#ifndef __ios__
    foreach (LinkInterface* link, _links) {
        if (link->isConnected()) {
            SerialLink* pSerialLink = qobject_cast<SerialLink*>(link);
            if (pSerialLink) {
                LinkConfiguration* pLinkConfig = pSerialLink->getLinkConfiguration();
                if (pLinkConfig) {
                    SerialConfiguration* pSerialConfig = qobject_cast<SerialConfiguration*>(pLinkConfig);
                    //if (pSerialConfig && pSerialConfig->usbDirect()) {
                    if (pSerialConfig) {
                        return link;
                    }
                }
            }
        }
    }
#endif
    return _links.count() ? _links[0] : NULL;
}

void Vehicle::setLatitude(double latitude)
{
    _coordinate.setLatitude(latitude);
    emit coordinateChanged(_coordinate);
}

void Vehicle::setLongitude(double longitude){
    _coordinate.setLongitude(longitude);
    emit coordinateChanged(_coordinate);
}

/**
 * get rotation matrix from euler angles
 */
void dcm_from_euler(double data[][3], double roll, double pitch, double yaw) {
    double cp = cosf(pitch);
    double sp = sinf(pitch);
    double sr = sinf(roll);
    double cr = cosf(roll);
    double sy = sinf(yaw);
    double cy = cosf(yaw);

    data[0][0] = cp * cy;
    data[0][1] = (sr * sp * cy) - (cr * sy);
    data[0][2] = (cr * sp * cy) + (sr * sy);
    data[1][0] = cp * sy;
    data[1][1] = (sr * sp * sy) + (cr * cy);
    data[1][2] = (cr * sp * sy) - (sr * cy);
    data[2][0] = -sp;
    data[2][1] = sr * cp;
    data[2][2] = cr * cp;
}


/**
 * get euler angles from rotation matrix
 */
void dcm_to_euler(double *roll, double *pitch, double *yaw, double data[][3]) {
    *pitch = asinf(-data[2][0]);

    if (fabsf(*pitch - 1.57079632679489661923f) < 1.0e-3f) {
        *roll = 0.0f;
        *yaw = atan2f(data[1][2] - data[0][1], data[0][2] + data[1][1]) + *roll;

    } else if (fabsf(*pitch + 1.57079632679489661923f) < 1.0e-3f) {
        *roll = 0.0f;
        *yaw = atan2f(data[1][2] - data[0][1], data[0][2] + data[1][1]) - *roll;

    } else {
        *roll = atan2f(data[2][1], data[2][2]);
        *yaw = atan2f(data[1][0], data[0][0]);
    }
}
void Vehicle::_updateAttitude(UASInterface*, double roll, double pitch, double yaw, quint64)
{
/*    if(_vtolfw){
        double Me[3][3] = {0}, Him[3][3] = {0};
        dcm_from_euler(Me, roll, pitch, yaw);
        dcm_from_euler(Him, 0.0f, 1.57079632679489661923f, 0.0f);

        double  self[3][3] = {0}, other[3][3] = {0}, res[3][3] = {0};
        memcpy(self, Me, sizeof(Me));
        memcpy(other, (Him), sizeof(Him));
        memset(res, 0, sizeof(res));

        for (size_t i = 0; i < 3; i++) {
            for (size_t k = 0; k < 3; k++) {
                for (size_t j = 0; j < 3; j++) {
                    res[i][k] += self[i][j] * other[j][k];
                }
            }
        }
        dcm_to_euler(&roll, &pitch, &yaw, res);
    } */
    if (qIsInf(roll)) {
        _rollFact.setRawValue(0);
    } else {
        _rollFact.setRawValue(roll * (180.0 / M_PI));
    }
    if (qIsInf(pitch)) {
        _pitchFact.setRawValue(0);
    } else {
        _pitchFact.setRawValue(pitch * (180.0 / M_PI));
    }
    if (qIsInf(yaw)) {
        _headingFact.setRawValue(0);
    } else {
        yaw = yaw * (180.0 / M_PI);
        if (yaw < 0) yaw += 360;
        _headingFact.setRawValue(yaw);
    }
}

void Vehicle::_updateAttitude(UASInterface* uas, int, double roll, double pitch, double yaw, quint64 timestamp)
{
    _updateAttitude(uas, roll, pitch, yaw, timestamp);
}

void Vehicle::_updateSpeed(UASInterface*, double groundSpeed, double airSpeed, quint64)
{
    _groundSpeedFact.setRawValue(groundSpeed);
    _airSpeedFact.setRawValue(airSpeed);
}

void Vehicle::_updateAltitude(UASInterface*, double altitudeAMSL, double altitudeRelative, double climbRate, quint64)
{
    _altitudeAMSLFact.setRawValue(altitudeAMSL);
    _altitudeRelativeFact.setRawValue(altitudeRelative);
    _climbRateFact.setRawValue(climbRate);
}

void Vehicle::_updateNavigationControllerErrors(UASInterface*, double altitudeError, double speedError, double xtrackError) {
    _navigationAltitudeError   = altitudeError;
    _navigationSpeedError      = speedError;
    _navigationCrosstrackError = xtrackError;
}

void Vehicle::_updateNavigationControllerData(UASInterface *uas, float, float, float, float targetBearing, float) {
    if (_mav == uas) {
        _navigationTargetBearing = targetBearing;
    }
}

int Vehicle::motorCount(void)
{
    switch (_vehicleType) {
    case MAV_TYPE_HELICOPTER:
        return 1;
    case MAV_TYPE_VTOL_DUOROTOR:
        return 2;
    case MAV_TYPE_TRICOPTER:
        return 3;
    case MAV_TYPE_QUADROTOR:
    case MAV_TYPE_VTOL_QUADROTOR:
        return 4;
    case MAV_TYPE_HEXAROTOR:
        return 6;
    case MAV_TYPE_OCTOROTOR:
        return 8;
    default:
        return -1;
    }
}

bool Vehicle::coaxialMotors(void)
{
    return _firmwarePlugin->multiRotorCoaxialMotors(this);
}

bool Vehicle::xConfigMotors(void)
{
    return _firmwarePlugin->multiRotorXConfig(this);
}

/*
 * Internal
 */

void Vehicle::_checkUpdate()
{
    // Update current location
    if(_mav) {
        if(latitude() != _mav->getLatitude()) {
            setLatitude(_mav->getLatitude());
        }
        if(longitude() != _mav->getLongitude()) {
            setLongitude(_mav->getLongitude());
        }
    }
}

QString Vehicle::getMavIconColor()
{
    // TODO: Not using because not only the colors are ghastly, it doesn't respect dark/light palette
    if(_mav)
        return _mav->getColor().name();
    else
        return QString("black");
}

QString Vehicle::formatedMessages()
{
    QString messages;
    foreach(UASMessage* message, qgcApp()->toolbox()->uasMessageHandler()->messages()) {
        messages += message->getFormatedText();
    }
    return messages;
}

void Vehicle::clearMessages()
{
    qgcApp()->toolbox()->uasMessageHandler()->clearMessages();
}

void Vehicle::_handletextMessageReceived(UASMessage* message)
{
    if(message)
    {
        _formatedMessage = message->getFormatedText();
        emit formatedMessageChanged();
    }
}

void Vehicle::_updateState(UASInterface*, QString name, QString)
{
    if (_currentState != name) {
        _currentState = name;
        emit currentStateChanged();
    }
}

void Vehicle::_handleTextMessage(int newCount)
{
    // Reset?
    if(!newCount) {
        _currentMessageCount = 0;
        _currentNormalCount  = 0;
        _currentWarningCount = 0;
        _currentErrorCount   = 0;
        _messageCount        = 0;
        _currentMessageType  = MessageNone;
        emit newMessageCountChanged();
        emit messageTypeChanged();
        emit messageCountChanged();
        return;
    }

    UASMessageHandler* pMh = qgcApp()->toolbox()->uasMessageHandler();
    Q_ASSERT(pMh);
    MessageType_t type = newCount ? _currentMessageType : MessageNone;
    int errorCount     = _currentErrorCount;
    int warnCount      = _currentWarningCount;
    int normalCount    = _currentNormalCount;
    //-- Add current message counts
    errorCount  += pMh->getErrorCount();
    warnCount   += pMh->getWarningCount();
    normalCount += pMh->getNormalCount();
    //-- See if we have a higher level
    if(errorCount != _currentErrorCount) {
        _currentErrorCount = errorCount;
        type = MessageError;
    }
    if(warnCount != _currentWarningCount) {
        _currentWarningCount = warnCount;
        if(_currentMessageType != MessageError) {
            type = MessageWarning;
        }
    }
    if(normalCount != _currentNormalCount) {
        _currentNormalCount = normalCount;
        if(_currentMessageType != MessageError && _currentMessageType != MessageWarning) {
            type = MessageNormal;
        }
    }
    int count = _currentErrorCount + _currentWarningCount + _currentNormalCount;
    if(count != _currentMessageCount) {
        _currentMessageCount = count;
        // Display current total new messages count
        emit newMessageCountChanged();
    }
    if(type != _currentMessageType) {
        _currentMessageType = type;
        // Update message level
        emit messageTypeChanged();
    }
    // Update message count (all messages)
    if(newCount != _messageCount) {
        _messageCount = newCount;
        emit messageCountChanged();
    }
    QString errMsg = pMh->getLatestError();
    if(errMsg != _latestError) {
        _latestError = errMsg;
        emit latestErrorChanged();
    }
}

void Vehicle::resetMessages()
{
    // Reset Counts
    int count = _currentMessageCount;
    MessageType_t type = _currentMessageType;
    _currentErrorCount   = 0;
    _currentWarningCount = 0;
    _currentNormalCount  = 0;
    _currentMessageCount = 0;
    _currentMessageType = MessageNone;
    if(count != _currentMessageCount) {
        emit newMessageCountChanged();
    }
    if(type != _currentMessageType) {
        emit messageTypeChanged();
    }
}

int Vehicle::manualControlReservedButtonCount(void)
{
    return _firmwarePlugin->manualControlReservedButtonCount();
}

void Vehicle::_loadSettings(void)
{
    QSettings settings;

    settings.beginGroup(QString(_settingsGroup).arg(_id));

    bool convertOk;

    _joystickMode = (JoystickMode_t)settings.value(_joystickModeSettingsKey, JoystickModeRC).toInt(&convertOk);
    if (!convertOk) {
        _joystickMode = JoystickModeRC;
    }

    // Joystick enabled is a global setting so first make sure there are any joysticks connected
    if (qgcApp()->toolbox()->joystickManager()->joysticks().count()) {
        _joystickEnabled = settings.value(_joystickEnabledSettingsKey, false).toBool();
    }
}

void Vehicle::_saveSettings(void)
{
    QSettings settings;

    settings.beginGroup(QString(_settingsGroup).arg(_id));

    settings.setValue(_joystickModeSettingsKey, _joystickMode);

    // The joystick enabled setting should only be changed if a joystick is present
    // since the checkbox can only be clicked if one is present
    if (qgcApp()->toolbox()->joystickManager()->joysticks().count()) {
        settings.setValue(_joystickEnabledSettingsKey, _joystickEnabled);
    }
}

int Vehicle::joystickMode(void)
{
    return _joystickMode;
}

void Vehicle::setJoystickMode(int mode)
{
    if (mode < 0 || mode >= JoystickModeMax) {
        qCWarning(VehicleLog) << "Invalid joystick mode" << mode;
        return;
    }

    _joystickMode = (JoystickMode_t)mode;
    _saveSettings();
    emit joystickModeChanged(mode);
}

QStringList Vehicle::joystickModes(void)
{
    QStringList list;

    list << "Normal" << "Attitude" << "Position" << "Force" << "Velocity";

    return list;
}

bool Vehicle::joystickEnabled(void)
{
    return _joystickEnabled;
}

void Vehicle::setJoystickEnabled(bool enabled)
{
    _joystickEnabled = enabled;
    _startJoystick(_joystickEnabled);
    _saveSettings();
    emit joystickEnabledChanged(_joystickEnabled);
}

void Vehicle::_startJoystick(bool start)
{
    Joystick* joystick = _joystickManager->activeJoystick();
    if (joystick) {
        if (start) {
            if (_joystickEnabled) {
                joystick->startPolling(this);
            }
        } else {
            joystick->stopPolling();
        }
    }
}

bool Vehicle::active(void)
{
    return _active;
}

void Vehicle::setActive(bool active)
{
    _active = active;

    _startJoystick(_active);
}

bool Vehicle::homePositionAvailable(void)
{
    return _homePositionAvailable;
}

QGeoCoordinate Vehicle::homePosition(void)
{
    return _homePosition;
}

void Vehicle::setArmed(bool armed)
{
    // We specifically use COMMAND_LONG:MAV_CMD_COMPONENT_ARM_DISARM since it is supported by more flight stacks.

    mavlink_message_t msg;
    mavlink_command_long_t cmd;

    cmd.command = (uint16_t)MAV_CMD_COMPONENT_ARM_DISARM;
    cmd.confirmation = 0;
    cmd.param1 = armed ? 1.0f : 0.0f;
    cmd.param2 = 0.0f;
    cmd.param3 = 0.0f;
    cmd.param4 = 0.0f;
    cmd.param5 = 0.0f;
    cmd.param6 = 0.0f;
    cmd.param7 = 0.0f;
    cmd.target_system = id();
    cmd.target_component = defaultComponentId();

    mavlink_msg_command_long_encode_chan(_mavlink->getSystemId(),
                                         _mavlink->getComponentId(),
                                         priorityLink()->mavlinkChannel(),
                                         &msg,
                                         &cmd);

    sendMessageOnLink(priorityLink(), msg);
}

bool Vehicle::flightModeSetAvailable(void)
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::SetFlightModeCapability);
}

QStringList Vehicle::flightModes(void)
{
    return _firmwarePlugin->flightModes(this);
}

QString Vehicle::flightMode(void) const
{
    return _firmwarePlugin->flightMode(_base_mode, _custom_mode);
}

void Vehicle::setFlightMode(const QString& flightMode)
{
    uint8_t     base_mode;
    uint32_t    custom_mode;

    if (_firmwarePlugin->setFlightMode(flightMode, &base_mode, &custom_mode)) {
        // setFlightMode will only set MAV_MODE_FLAG_CUSTOM_MODE_ENABLED in base_mode, we need to move back in the existing
        // states.
        uint8_t newBaseMode = _base_mode & ~MAV_MODE_FLAG_DECODE_POSITION_CUSTOM_MODE;
        newBaseMode |= base_mode;

        mavlink_message_t msg;
        mavlink_msg_set_mode_pack_chan(_mavlink->getSystemId(),
                                       _mavlink->getComponentId(),
                                       priorityLink()->mavlinkChannel(),
                                       &msg,
                                       id(),
                                       newBaseMode,
                                       custom_mode);
        sendMessageOnLink(priorityLink(), msg);
    } else {
        qWarning() << "FirmwarePlugin::setFlightMode failed, flightMode:" << flightMode;
    }
}

bool Vehicle::isPosCtlMode()
{
    return _firmwarePlugin->isPosCtlMode(this);
}

bool Vehicle::keyCtlAtitude()
{
    return _isKeyPressed;
}

void Vehicle::setKeyCtlAtitude(bool isKeyPressed)
{
    _isKeyPressed = isKeyPressed;
    emit keyCtlAtitudeChanged(isKeyPressed);
}

bool Vehicle::hilMode(void)
{
    return _base_mode & MAV_MODE_FLAG_HIL_ENABLED;
}

void Vehicle::setHilMode(bool hilMode)
{
    mavlink_message_t msg;

    uint8_t newBaseMode = _base_mode & ~MAV_MODE_FLAG_DECODE_POSITION_HIL;
    if (hilMode) {
        newBaseMode |= MAV_MODE_FLAG_HIL_ENABLED;
    }

    mavlink_msg_set_mode_pack_chan(_mavlink->getSystemId(),
                                   _mavlink->getComponentId(),
                                   priorityLink()->mavlinkChannel(),
                                   &msg,
                                   id(),
                                   newBaseMode,
                                   _custom_mode);
    sendMessageOnLink(priorityLink(), msg);
}

void Vehicle::requestDataStream(MAV_DATA_STREAM stream, uint16_t rate, bool sendMultiple)
{
    mavlink_message_t               msg;
    mavlink_request_data_stream_t   dataStream;

    dataStream.req_stream_id = stream;
    dataStream.req_message_rate = rate;
    dataStream.start_stop = 1;  // start
    dataStream.target_system = id();
    dataStream.target_component = defaultComponentId();

    mavlink_msg_request_data_stream_encode_chan(_mavlink->getSystemId(),
                                                _mavlink->getComponentId(),
                                                priorityLink()->mavlinkChannel(),
                                                &msg,
                                                &dataStream);

    if (sendMultiple) {
        // We use sendMessageMultiple since we really want these to make it to the vehicle
        sendMessageMultiple(msg);
    } else {
        sendMessageOnLink(priorityLink(), msg);
    }
}

void Vehicle::_sendMessageMultipleNext(void)
{
    if (_nextSendMessageMultipleIndex < _sendMessageMultipleList.count()) {
        qCDebug(VehicleLog) << "_sendMessageMultipleNext:" << _sendMessageMultipleList[_nextSendMessageMultipleIndex].message.msgid;

        sendMessageOnLink(priorityLink(), _sendMessageMultipleList[_nextSendMessageMultipleIndex].message);

        if (--_sendMessageMultipleList[_nextSendMessageMultipleIndex].retryCount <= 0) {
            _sendMessageMultipleList.removeAt(_nextSendMessageMultipleIndex);
        } else {
            _nextSendMessageMultipleIndex++;
        }
    }

    if (_nextSendMessageMultipleIndex >= _sendMessageMultipleList.count()) {
        _nextSendMessageMultipleIndex = 0;
    }
}

void Vehicle::sendMessageMultiple(mavlink_message_t message)
{
    SendMessageMultipleInfo_t   info;

    info.message =      message;
    info.retryCount =   _sendMessageMultipleRetries;

    _sendMessageMultipleList.append(info);
}

void Vehicle::_missionManagerError(int errorCode, const QString& errorMsg)
{
    Q_UNUSED(errorCode);
    qgcApp()->showMessage(QString(tr("Error during Mission communication with Vehicle: %1")).arg(errorMsg));
}

void Vehicle::_geoFenceManagerError(int errorCode, const QString& errorMsg)
{
    Q_UNUSED(errorCode);
    qgcApp()->showMessage(QString(tr("Error during GeoFence communication with Vehicle: %1")).arg(errorMsg));
}

void Vehicle::_rallyPointManagerError(int errorCode, const QString& errorMsg)
{
    Q_UNUSED(errorCode);
    qgcApp()->showMessage(QString(tr("Error during Rally Point communication with Vehicle: %1")).arg(errorMsg));
}

void Vehicle::_addNewMapTrajectoryPoint(void)
{
    if (_mapTrajectoryHaveFirstCoordinate) {
        // Keep three minutes of trajectory
        //if (_mapTrajectoryList.count() * _mapTrajectoryMsecsBetweenPoints > 3 * 1000 * 60 * 6) {
            //_mapTrajectoryList.removeAt(0)->deleteLater();
        //}
        _mapTrajectoryList.append(new CoordinateVector(_mapTrajectoryLastCoordinate, _coordinate, this));
    }
    _mapTrajectoryHaveFirstCoordinate = true;
    _mapTrajectoryLastCoordinate = _coordinate;
}

void Vehicle::_mapTrajectoryStart(void)
{
    _mapTrajectoryHaveFirstCoordinate = false;
    _mapTrajectoryList.clear();
    _mapTrajectoryTimer.start();
}

void Vehicle::_mapTrajectoryStop()
{
    _mapTrajectoryTimer.stop();
}

void Vehicle::_parametersReady(bool parametersReady)
{
    if (parametersReady && !_missionManagerInitialRequestSent) {
        _missionManagerInitialRequestSent = true;
        _missionManager->requestMissionItems();
    }

    if (parametersReady) {
        setJoystickEnabled(_joystickEnabled);
    }
}

void Vehicle::disconnectInactiveVehicle(void)
{
    // Vehicle is no longer communicating with us, disconnect all links


    LinkManager* linkMgr = qgcApp()->toolbox()->linkManager();
    for (int i=0; i<_links.count(); i++) {
        // FIXME: This linkInUse check is a hack fix for multiple vehicles on the same link.
        // The real fix requires significant restructuring which will come later.
        if (!qgcApp()->toolbox()->multiVehicleManager()->linkInUse(_links[i], this)) {
            linkMgr->disconnectLink(_links[i]);
        }
    }
}

void Vehicle::_imageReady(UASInterface*)
{
    if(_uas)
    {
        QImage img = _uas->getImage();
        qgcApp()->toolbox()->imageProvider()->setImage(&img, _id);
        _flowImageIndex++;
        emit flowImageIndexChanged();
    }
}

void Vehicle::_remoteControlRSSIChanged(uint8_t rssi)
{
    if (_rcRSSIstore < 0 || _rcRSSIstore > 100) {
        _rcRSSIstore = rssi;
    }

    // Low pass to git rid of jitter
    _rcRSSIstore = (_rcRSSIstore * 0.9f) + ((float)rssi * 0.1);
    uint8_t filteredRSSI = (uint8_t)ceil(_rcRSSIstore);
    if(_rcRSSIstore < 0.1) {
        filteredRSSI = 0;
    }
    if(_rcRSSI != filteredRSSI) {
        _rcRSSI = filteredRSSI;
        emit rcRSSIChanged(_rcRSSI);
    }
}

void Vehicle::virtualTabletJoystickValue(double roll, double pitch, double yaw, double thrust)
{

    // The following if statement prevents the virtualTabletJoystick from sending values if the standard joystick is enabled
    if ( !_joystickEnabled ) {
//        double vtolMode;
//        if(this->vtol ()){
//            if(_vtolfw){
//                vtolMode = 2000;
//            }else{
//                vtolMode = 1000;
//            }
//        }else{
//            vtolMode = 65535;
//        }
//        _uas->setVirtualControlSetpoint(roll, pitch, yaw, thrust, vtolMode);
        _uas->setExternalControlSetpoint (roll, pitch, yaw, thrust,0,Vehicle::JoystickModeRC);
    }
}

void Vehicle::setConnectionLostEnabled(bool connectionLostEnabled)
{
    if (_connectionLostEnabled != connectionLostEnabled) {
        _connectionLostEnabled = connectionLostEnabled;
        emit connectionLostEnabledChanged(_connectionLostEnabled);
    }
}

void Vehicle::_connectionLostTimeout(void)
{
    QSettings setting;
    bool m = setting.value("language",false).toBool();
    if (_connectionLostEnabled && !_connectionLost) {
        _connectionLost = true;
        _heardFrom = false;
        emit connectionLostChanged(true);
        //========================================================
        if(m)_say(QString("%1 communication lost").arg(_vehicleIdSpeech()));
        if(!m)_say(QString("%1 通信丢失").arg(_vehicleIdSpeech()));
        if (_autoDisconnect) {
            disconnectInactiveVehicle();
        }
    }
}

void Vehicle::_connectionActive(void)
{
    QSettings setting;
    bool m = setting.value("language",false).toBool();
    _connectionLostTimer.start();
    if (_connectionLost) {
        _connectionLost = false;
        emit connectionLostChanged(false);
        //============================================
        if(m)_say(QString("%1 communication regained").arg(_vehicleIdSpeech()));
        if(!m)_say(QString("%1 通信恢复").arg(_vehicleIdSpeech()));
    }
}

void Vehicle::_say(const QString& text)
{
    qgcApp()->toolbox()->audioOutput()->say(text.toLower());
}

bool Vehicle::fixedWing(void) const
{
    return QGCMAVLink::isFixedWing(vehicleType());
}

bool Vehicle::rover(void) const
{
    return QGCMAVLink::isRover(vehicleType());
}

bool Vehicle::sub(void) const
{
    return QGCMAVLink::isSub(vehicleType());
}

bool Vehicle::multiRotor(void) const
{
    return QGCMAVLink::isMultiRotor(vehicleType());
}

bool Vehicle::vtol(void) const
{
    switch (vehicleType()) {
    case MAV_TYPE_VTOL_DUOROTOR:
    case MAV_TYPE_VTOL_QUADROTOR:
    case MAV_TYPE_VTOL_TILTROTOR:
    case MAV_TYPE_VTOL_RESERVED2:
    case MAV_TYPE_VTOL_RESERVED3:
    case MAV_TYPE_VTOL_RESERVED4:
    case MAV_TYPE_VTOL_RESERVED5:
        return true;
    default:
        return false;
    }
}

bool Vehicle::supportsManualControl(void) const
{
    return _firmwarePlugin->supportsManualControl();
}

bool Vehicle::supportsThrottleModeCenterZero(void) const
{
    return _firmwarePlugin->supportsThrottleModeCenterZero();
}

bool Vehicle::supportsRadio(void) const
{
    return _firmwarePlugin->supportsRadio();
}

bool Vehicle::supportsJSButton(void) const
{
    return _firmwarePlugin->supportsJSButton();
}

void Vehicle::_setCoordinateValid(bool coordinateValid)
{
    if (coordinateValid != _coordinateValid) {
        _coordinateValid = coordinateValid;
        emit coordinateValidChanged(_coordinateValid);
    }
}

QString Vehicle::vehicleTypeName() const {
    static QMap<int, QString> typeNames = {
        { MAV_TYPE_GENERIC,         tr("Generic micro air vehicle" )},
        { MAV_TYPE_FIXED_WING,      tr("Fixed wing aircraft")},
        { MAV_TYPE_QUADROTOR,       tr("Quadrotor")},
        { MAV_TYPE_COAXIAL,         tr("Coaxial helicopter")},
        { MAV_TYPE_HELICOPTER,      tr("Normal helicopter with tail rotor.")},
        { MAV_TYPE_ANTENNA_TRACKER, tr("Ground installation")},
        { MAV_TYPE_GCS,             tr("Operator control unit / ground control station")},
        { MAV_TYPE_AIRSHIP,         tr("Airship, controlled")},
        { MAV_TYPE_FREE_BALLOON,    tr("Free balloon, uncontrolled")},
        { MAV_TYPE_ROCKET,          tr("Rocket")},
        { MAV_TYPE_GROUND_ROVER,    tr("Ground rover")},
        { MAV_TYPE_SURFACE_BOAT,    tr("Surface vessel, boat, ship")},
        { MAV_TYPE_SUBMARINE,       tr("Submarine")},
        { MAV_TYPE_HEXAROTOR,       tr("Hexarotor")},
        { MAV_TYPE_OCTOROTOR,       tr("Octorotor")},
        { MAV_TYPE_TRICOPTER,       tr("Octorotor")},
        { MAV_TYPE_FLAPPING_WING,   tr("Flapping wing")},
        { MAV_TYPE_KITE,            tr("Flapping wing")},
        { MAV_TYPE_ONBOARD_CONTROLLER, tr("Onboard companion controller")},
        { MAV_TYPE_VTOL_DUOROTOR,   tr("Two-rotor VTOL using control surfaces in vertical operation in addition. Tailsitter")},
        { MAV_TYPE_VTOL_QUADROTOR,  tr("Quad-rotor VTOL using a V-shaped quad config in vertical operation. Tailsitter")},
        { MAV_TYPE_VTOL_TILTROTOR,  tr("Tiltrotor VTOL")},
        { MAV_TYPE_VTOL_RESERVED2,  tr("VTOL reserved 2")},
        { MAV_TYPE_VTOL_RESERVED3,  tr("VTOL reserved 3")},
        { MAV_TYPE_VTOL_RESERVED4,  tr("VTOL reserved 4")},
        { MAV_TYPE_VTOL_RESERVED5,  tr("VTOL reserved 5")},
        { MAV_TYPE_GIMBAL,          tr("Onboard gimbal")},
        { MAV_TYPE_ADSB,            tr("Onboard ADSB peripheral")},
    };
    return typeNames[_vehicleType];
}

/// Returns the string to speak to identify the vehicle
QString Vehicle::_vehicleIdSpeech(void)
{
    QSettings setting;
    bool m = setting.value("language",false).toBool();
    if (qgcApp()->toolbox()->multiVehicleManager()->vehicles()->count() > 1) {
        if(m)return QString("vehicle %1").arg(id());
        if(!m)return QString("载具 %1").arg(id());
        return QString("vehicle %1").arg(id());
    } else {
        return QString();
    }
}

void Vehicle::_handleFlightModeChanged(const QString& flightMode)
{
    //==================================================
    QSettings setting;
    bool m = setting.value("language",false).toBool();
    if(m)_say(QString("%1 %2 flight mode").arg(_vehicleIdSpeech()).arg(flightMode));
    if(!m)_say(QString("%1 切换成为 %2 模式").arg(_vehicleIdSpeech()).arg(flightMode));
    emit guidedModeChanged(_firmwarePlugin->isGuidedMode(this));
}

void Vehicle::_announceArmedChanged(bool armed)
{
    QSettings setting;
    bool m = setting.value("language",false).toBool();
    if(m)_say(QString("%1 %2").arg(_vehicleIdSpeech()).arg(armed ? QStringLiteral("armed") : QStringLiteral("disarmed")));
    if(!m)_say(QString("%1 %2").arg(_vehicleIdSpeech()).arg(armed ? QString("解锁") : QString("上锁")));
}

void Vehicle::clearTrajectoryPoints(void)
{
    _mapTrajectoryList.clearAndDeleteContents();
}

void Vehicle::setFlying(bool flying)
{
    if (armed() && _flying != flying) {
        _flying = flying;
        emit flyingChanged(flying);
    }
}

bool Vehicle::guidedModeSupported(void) const
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::GuidedModeCapability);
}

bool Vehicle::pauseVehicleSupported(void) const
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::PauseVehicleCapability);
}

bool Vehicle::orbitModeSupported() const
{
    return _firmwarePlugin->isCapable(this, FirmwarePlugin::OrbitModeCapability);
}

bool Vehicle::paratureModeSupported() const
{
    return _firmwarePlugin->isCapable (this,FirmwarePlugin::ParatureModeCapability);
}

bool Vehicle::vtolMcandFwModeSupported() const
{
    return _firmwarePlugin->isCapable (this,FirmwarePlugin::McandFwModeCapability);
}

void Vehicle::guidedModeRTL(void)
{
    if (!guidedModeSupported()) {
        qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeRTL(this);
}

void Vehicle::guidedModeLand(void)
{
    if (!guidedModeSupported()) {
        qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeLand(this);
}

void Vehicle::guidedModeParachute()
{
    if(!paratureModeSupported()){
        qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeParachute(this);
}

void Vehicle::guidedModeVTOLMcandFw()
{
    if(!vtolMcandFwModeSupported()){
        qgcApp ()->showMessage (guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeVTOLMcandFw(this);
}

void Vehicle::guidedModeMission()
{
    if(!guidedModeSupported()){
        qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeMission(this);
}

void Vehicle::guidedModeTakeoff(double altitudeRel)
{
    if (!guidedModeSupported()) {
        qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    setGuidedMode(true);
    _firmwarePlugin->guidedModeTakeoff(this, altitudeRel);
}

void Vehicle::guidedModeGotoLocation(const QGeoCoordinate& gotoCoord)
{
    if (!guidedModeSupported()) {
        qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModeGotoLocation(this, gotoCoord);
}

void Vehicle::guidedModeFollowMe(const QGeoCoordinate& gotoCoord)
{
   if(!guidedModeSupported()) {
       qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
   }

   QDateTime timestamp = QDateTime::currentDateTime();
   QGeoPositionInfo update(gotoCoord, timestamp);
   qgcApp()->toolbox()->followMe()->setGPSLocation(update);

   if (_mapFollowMeHaveFirstCoordinate) {
       _mapFollowMeList.append(new CoordinateVector(_mapTraFollowMeLastCoordinate, gotoCoord, this));
   }
   _mapFollowMeHaveFirstCoordinate = true;
   _mapTraFollowMeLastCoordinate = gotoCoord;
}

void Vehicle::guidedModePosition()
{
    if(!guidedModeSupported()){
        qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
        return;
    }
    _firmwarePlugin->guidedModePosition(this);
}

//void Vehicle::guidedModeChangeAltitude(double altitudeRel)
//{
//    if (!guidedModeSupported()) {
//        qgcApp()->showMessage(guided_mode_not_supported_by_vehicle);
//        return;
//    }
//    _firmwarePlugin->guidedModeChangeAltitude(this, altitudeRel);
//}

//void Vehicle::guidedModeOrbit(const QGeoCoordinate& centerCoord, double radius, double velocity, double altitude)
//{
//    if (!orbitModeSupported()) {
//        qgcApp()->showMessage(QString(tr("Orbit mode not supported by Vehicle.")));
//        return;
//    }
//    _firmwarePlugin->guidedModeOrbit(this, centerCoord, radius, velocity, altitude);
//}

void Vehicle::pauseVehicle(void)
{
    if (!pauseVehicleSupported()) {
        qgcApp()->showMessage(QString(tr("Pause not supported by vehicle.")));
        return;
    }
    _firmwarePlugin->pauseVehicle(this);
}

bool Vehicle::guidedMode(void) const
{
    return _firmwarePlugin->isGuidedMode(this);
}

bool Vehicle::followMeMode(void) const
{
    return _firmwarePlugin->isFollowMeMode(this);
}

void Vehicle::setGuidedMode(bool guidedMode)
{
    return _firmwarePlugin->setGuidedMode(this, guidedMode);
}

void Vehicle::emergencyStop(void)
{
    mavlink_message_t msg;
    mavlink_command_long_t cmd;

    cmd.command = (uint16_t)MAV_CMD_COMPONENT_ARM_DISARM;
    cmd.confirmation = 0;
    cmd.param1 = 0.0f;
    cmd.param2 = 21196.0f;  // Magic number for emergency stop
    cmd.param3 = 0.0f;
    cmd.param4 = 0.0f;
    cmd.param5 = 0.0f;
    cmd.param6 = 0.0f;
    cmd.param7 = 0.0f;
    cmd.target_system = id();
    cmd.target_component = defaultComponentId();
    mavlink_msg_command_long_encode_chan(_mavlink->getSystemId(),
                                         _mavlink->getComponentId(),
                                         priorityLink()->mavlinkChannel(),
                                         &msg,
                                         &cmd);

    sendMessageOnLink(priorityLink(), msg);
}

void Vehicle::setCurrentMissionSequence(int seq)
{
    if (!_firmwarePlugin->sendHomePositionToVehicle()) {
        seq--;
    }
    mavlink_message_t msg;
    mavlink_msg_mission_set_current_pack_chan(_mavlink->getSystemId(),
                                              _mavlink->getComponentId(),
                                              priorityLink()->mavlinkChannel(),
                                              &msg,
                                              id(),
                                              _compID,
                                              seq);
    sendMessageOnLink(priorityLink(), msg);
}
void Vehicle::setDigitalCameraControl(bool shot){
        mavlink_digicam_control_t digicam_control;
        mavlink_message_t msg;
        digicam_control.target_system = id();
        digicam_control.target_component = defaultComponentId();
        digicam_control.shot = (shot == true) ? (int8_t) 1 : (int8_t) 0;
        mavlink_msg_digicam_control_encode(_mavlink->getSystemId(), _mavlink->getComponentId(), &msg, &digicam_control);
        sendMessageOnPriorityLink(msg);

        doCommandLong(defaultComponentId(), MAV_CMD_DO_DIGICAM_CONTROL, 0, 0, 0, 0, 1, 0, 0);
}

bool Vehicle::parameterExists(int componentId, const QString& name) const
{
    if(!_parameterManager){
        return false;
    }

    return _parameterManager->parameterExists(componentId, name);
}

/// Returns the specified parameter Fact from the default component
/// WARNING: Returns a default Fact if parameter does not exists. If that possibility exists, check for existence first with
/// parameterExists.
Fact* Vehicle::getParameterFact(int componentId, const QString& name)
{
    if(!_parameterManager){
        return NULL;
    }

    return _parameterManager->getParameter(componentId, name);
}

double Vehicle::getMisDistOneWp()
{
    if(!parameterExists(-1,"MIS_DIST_1WP")){
        return -1;
    }

    Fact* fact = getParameterFact(-1, "MIS_DIST_1WP");
    if(!fact){
        return -1;
    }
    QVariant variant = fact->cookedValue ();

    bool ok = false;
    double distance = variant.toDouble (&ok);
    qDebug()<<__FILE__<<" : "<<__LINE__<<" d : "<<distance<<" : ok :"<<ok;
    if(!ok){
        return -1;
    }

    return distance;
}

void Vehicle::doCommandLong(int component, MAV_CMD command, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
    mavlink_message_t       msg;
    mavlink_command_long_t  cmd;

    cmd.command = command;
    cmd.confirmation = 0;
    cmd.param1 = param1;
    cmd.param2 = param2;
    cmd.param3 = param3;
    cmd.param4 = param4;
    cmd.param5 = param5;
    cmd.param6 = param6;
    cmd.param7 = param7;
    cmd.target_system = id();
    cmd.target_component = component;
    mavlink_msg_command_long_encode_chan(_mavlink->getSystemId(),
                                         _mavlink->getComponentId(),
                                         priorityLink()->mavlinkChannel(),
                                         &msg,
                                         &cmd);

    sendMessageOnLink(priorityLink(), msg);
}

void Vehicle::setPrearmError(const QString& prearmError)
{
    _prearmError = prearmError;
    emit prearmErrorChanged(_prearmError);
    if (!_prearmError.isEmpty()) {
        _prearmErrorTimer.start();
    }
}

void Vehicle::_prearmErrorTimeout(void)
{
    setPrearmError(QString());
}

void Vehicle::setFirmwareVersion(int majorVersion, int minorVersion, int patchVersion, FIRMWARE_VERSION_TYPE versionType)
{
    _firmwareMajorVersion = majorVersion;
    _firmwareMinorVersion = minorVersion;
    _firmwarePatchVersion = patchVersion;
    _firmwareVersionType = versionType;
    emit firmwareMajorVersionChanged(_firmwareMajorVersion);
    emit firmwareMinorVersionChanged(_firmwareMinorVersion);
    emit firmwarePatchVersionChanged(_firmwarePatchVersion);
    emit firmwareVersionTypeChanged(_firmwareVersionType);
}

QString Vehicle::firmwareVersionTypeString(void) const
{
    switch (_firmwareVersionType) {
    case FIRMWARE_VERSION_TYPE_DEV:
        return QStringLiteral("dev ") + _flightCustomVersion.left(6);
    case FIRMWARE_VERSION_TYPE_ALPHA:
        return QStringLiteral("alpha ") + _flightCustomVersion.left(6);
    case FIRMWARE_VERSION_TYPE_BETA:
        return QStringLiteral("beta ") + _flightCustomVersion.left(6);
    case FIRMWARE_VERSION_TYPE_RC:
        return QStringLiteral("rc ") + _flightCustomVersion.left(6);
    case FIRMWARE_VERSION_TYPE_OFFICIAL:
    default:
        return QStringLiteral("");
    }
}

void Vehicle::rebootVehicle()
{
    doCommandLong(defaultComponentId(), MAV_CMD_PREFLIGHT_REBOOT_SHUTDOWN, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
}

int Vehicle::defaultComponentId(void)
{
    return _parameterManager->defaultComponentId();
}

void Vehicle::setSoloFirmware(bool soloFirmware)
{
    if (soloFirmware != _soloFirmware) {
        _soloFirmware = soloFirmware;
        emit soloFirmwareChanged(soloFirmware);
    }
}

#if 0
    // Temporarily removed, waiting for new command implementation
void Vehicle::motorTest(int motor, int percent, int timeoutSecs)
{
    doCommandLong(defaultComponentId(), MAV_CMD_DO_MOTOR_TEST, motor, MOTOR_TEST_THROTTLE_PERCENT, percent, timeoutSecs);
}
#endif

void Vehicle::_newMissionItemsAvailable(void)
{
    // After the initial mission request complets we ask for the geofence
    if (!_geoFenceManagerInitialRequestSent) {
        _geoFenceManagerInitialRequestSent = true;
        _geoFenceManager->loadFromVehicle();
    }
}

void Vehicle::_newGeoFenceAvailable(void)
{
    // After the initial mission request complets we ask for the geofence
    if (!_rallyPointManagerInitialRequestSent) {
        _rallyPointManagerInitialRequestSent = true;
        _rallyPointManager->loadFromVehicle();
    }
}

QString Vehicle::brandImage(void) const
{
    return _firmwarePlugin->brandImage(this);
}

QStringList Vehicle::unhealthySensors(void) const
{
    QStringList sensorList;

    struct sensorInfo_s {
        uint32_t    bit;
        const char* sensorName;
    };

    static const sensorInfo_s rgSensorInfo[] = {
        { MAV_SYS_STATUS_SENSOR_3D_GYRO,                "Gyro" },
        { MAV_SYS_STATUS_SENSOR_3D_ACCEL,               "Accelerometer" },
        { MAV_SYS_STATUS_SENSOR_3D_MAG,                 "Magnetometer" },
        { MAV_SYS_STATUS_SENSOR_ABSOLUTE_PRESSURE,      "Absolute pressure" },
        { MAV_SYS_STATUS_SENSOR_DIFFERENTIAL_PRESSURE,  "Differential pressure" },
        { MAV_SYS_STATUS_SENSOR_GPS,                    "GPS" },
        { MAV_SYS_STATUS_SENSOR_OPTICAL_FLOW,           "Optical flow" },
        { MAV_SYS_STATUS_SENSOR_VISION_POSITION,        "Computer vision position" },
        { MAV_SYS_STATUS_SENSOR_LASER_POSITION,         "Laser based position" },
        { MAV_SYS_STATUS_SENSOR_EXTERNAL_GROUND_TRUTH,  "External ground truth" },
        { MAV_SYS_STATUS_SENSOR_ANGULAR_RATE_CONTROL,   "Angular rate control" },
        { MAV_SYS_STATUS_SENSOR_ATTITUDE_STABILIZATION, "Attitude stabilization" },
        { MAV_SYS_STATUS_SENSOR_YAW_POSITION,           "Yaw position" },
        { MAV_SYS_STATUS_SENSOR_Z_ALTITUDE_CONTROL,     "Z/altitude control" },
        { MAV_SYS_STATUS_SENSOR_XY_POSITION_CONTROL,    "X/Y position control" },
        { MAV_SYS_STATUS_SENSOR_MOTOR_OUTPUTS,          "Motor outputs / control" },
        { MAV_SYS_STATUS_SENSOR_RC_RECEIVER,            "RC receiver" },
        { MAV_SYS_STATUS_SENSOR_3D_GYRO2,               "Gyro 2" },
        { MAV_SYS_STATUS_SENSOR_3D_ACCEL2,              "Accelerometer 2" },
        { MAV_SYS_STATUS_SENSOR_3D_MAG2,                "Magnetometer 2" },
        { MAV_SYS_STATUS_GEOFENCE,                      "GeoFence" },
        { MAV_SYS_STATUS_AHRS,                          "AHRS" },
        { MAV_SYS_STATUS_TERRAIN,                       "Terrain" },
        { MAV_SYS_STATUS_REVERSE_MOTOR,                 "Motors reversed" },
        { MAV_SYS_STATUS_LOGGING,                       "Logging" },
    };

    for (size_t i=0; i<sizeof(rgSensorInfo)/sizeof(sensorInfo_s); i++) {
        const sensorInfo_s* pSensorInfo = &rgSensorInfo[i];
        if ((_onboardControlSensorsEnabled & pSensorInfo->bit) && !(_onboardControlSensorsHealth & pSensorInfo->bit)) {
            sensorList << pSensorInfo->sensorName;
        }
    }

    return sensorList;
}


const char* VehicleGPSFactGroup::_hdopFactName =                "hdop";
const char* VehicleGPSFactGroup::_vdopFactName =                "vdop";
const char* VehicleGPSFactGroup::_courseOverGroundFactName =    "courseOverGround";
const char* VehicleGPSFactGroup::_countFactName =               "count";
const char* VehicleGPSFactGroup::_lockFactName =                "lock";

VehicleGPSFactGroup::VehicleGPSFactGroup(QObject* parent)
    : FactGroup(1000, loadseet(), parent)
    , _vehicle(NULL)
    , _hdopFact             (0, _hdopFactName,              FactMetaData::valueTypeDouble)
    , _vdopFact             (0, _vdopFactName,              FactMetaData::valueTypeDouble)
    , _courseOverGroundFact (0, _courseOverGroundFactName,  FactMetaData::valueTypeDouble)
    , _countFact            (0, _countFactName,             FactMetaData::valueTypeInt32)
    , _lockFact             (0, _lockFactName,              FactMetaData::valueTypeInt32)
{
    _addFact(&_hdopFact,                _hdopFactName);
    _addFact(&_vdopFact,                _vdopFactName);
    _addFact(&_courseOverGroundFact,    _courseOverGroundFactName);
    _addFact(&_lockFact,                _lockFactName);
    _addFact(&_countFact,               _countFactName);

    _hdopFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _vdopFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _courseOverGroundFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
}

//-----------------------------------------------------------------------------
void
Vehicle::startMavlinkLog()
{
    doCommandLong(defaultComponentId(), MAV_CMD_LOGGING_START);
}

//-----------------------------------------------------------------------------
void
Vehicle::stopMavlinkLog()
{
    doCommandLong(defaultComponentId(), MAV_CMD_LOGGING_STOP);
}

//-----------------------------------------------------------------------------
void
Vehicle::_ackMavlinkLogData(uint16_t sequence)
{
    mavlink_message_t msg;
    mavlink_logging_ack_t ack;
    ack.sequence = sequence;
    ack.target_component = defaultComponentId();
    ack.target_system = id();
    mavlink_msg_logging_ack_encode_chan(
        _mavlink->getSystemId(),
        _mavlink->getComponentId(),
        priorityLink()->mavlinkChannel(),
        &msg,
        &ack);
    sendMessageOnLink(priorityLink(), msg);
}

//-----------------------------------------------------------------------------
void
Vehicle::_handleMavlinkLoggingData(mavlink_message_t& message)
{
    mavlink_logging_data_t log;
    mavlink_msg_logging_data_decode(&message, &log);
    emit mavlinkLogData(this, log.target_system, log.target_component, log.sequence,
        log.first_message_offset, QByteArray((const char*)log.data, log.length), false);
}

//-----------------------------------------------------------------------------
void
Vehicle::_handleMavlinkLoggingDataAcked(mavlink_message_t& message)
{
    mavlink_logging_data_t log;
    mavlink_msg_logging_data_decode(&message, &log);
    _ackMavlinkLogData(log.sequence);
    emit mavlinkLogData(this, log.target_system, log.target_component, log.sequence,
        log.first_message_offset, QByteArray((const char*)log.data, log.length), true);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void VehicleGPSFactGroup::setVehicle(Vehicle* vehicle)
{
    _vehicle = vehicle;

    if (!vehicle) {
        // Disconnected Vehicle
        return;
    }

    connect(_vehicle->uas(), &UASInterface::localizationChanged, this, &VehicleGPSFactGroup::_setSatLoc);

    UAS* pUas = dynamic_cast<UAS*>(_vehicle->uas());
    connect(pUas, &UAS::satelliteCountChanged,  this, &VehicleGPSFactGroup::_setSatelliteCount);
    connect(pUas, &UAS::satRawHDOPChanged,      this, &VehicleGPSFactGroup::_setSatRawHDOP);
    connect(pUas, &UAS::satRawVDOPChanged,      this, &VehicleGPSFactGroup::_setSatRawVDOP);
    connect(pUas, &UAS::satRawCOGChanged,       this, &VehicleGPSFactGroup::_setSatRawCOG);
}

void VehicleGPSFactGroup::_setSatelliteCount(double val, QString)
{
    // I'm assuming that a negative value or over 99 means there is no GPS
    if(val < 0.0)  val = -1.0;
    if(val > 99.0) val = -1.0;

    _countFact.setRawValue(val);
}

void VehicleGPSFactGroup::_setSatRawHDOP(double val)
{
    _hdopFact.setRawValue(val);
}

void VehicleGPSFactGroup::_setSatRawVDOP(double val)
{
    _vdopFact.setRawValue(val);
}

void VehicleGPSFactGroup::_setSatRawCOG(double val)
{
    _courseOverGroundFact.setRawValue(val);
}

void VehicleGPSFactGroup::_setSatLoc(UASInterface*, int fix)
{
    _lockFact.setRawValue(fix);

    // fix 0: lost, 1: at least one satellite, but no GPS fix, 2: 2D lock, 3: 3D lock
    if (fix > 2) {
        _vehicle->_setCoordinateValid(true);
    }
}

const char* VehicleBatteryFactGroup::_voltageFactName =                     "voltage";
const char* VehicleBatteryFactGroup::_percentRemainingFactName =            "percentRemaining";
const char* VehicleBatteryFactGroup::_mahConsumedFactName =                 "mahConsumed";
const char* VehicleBatteryFactGroup::_currentFactName =                     "current";
const char* VehicleBatteryFactGroup::_temperatureFactName =                 "temperature";
const char* VehicleBatteryFactGroup::_cellCountFactName =                   "cellCount";
const char* VehicleBatteryFactGroup::_voltagesFactName =                    "currents";

const char* VehicleBatteryFactGroup::_settingsGroup =                       "Vehicle.battery";

const double VehicleBatteryFactGroup::_voltageUnavailable =           -1.0;
const int    VehicleBatteryFactGroup::_percentRemainingUnavailable =  -1;
const int    VehicleBatteryFactGroup::_mahConsumedUnavailable =       -1;
const int    VehicleBatteryFactGroup::_currentUnavailable =           -1;
const double VehicleBatteryFactGroup::_temperatureUnavailable =       -1.0;
const int    VehicleBatteryFactGroup::_cellCountUnavailable =         -1.0;
const int    VehicleBatteryFactGroup::_voltagesUnavailable =          -1;

VehicleBatteryFactGroup::VehicleBatteryFactGroup(QObject* parent)
    : FactGroup(1000, loadseet(), parent)
    , _vehicle(NULL)
    , _voltageFact                  (0, _voltageFactName,                   FactMetaData::valueTypeDouble)
    , _percentRemainingFact         (0, _percentRemainingFactName,          FactMetaData::valueTypeInt32)
    , _mahConsumedFact              (0, _mahConsumedFactName,               FactMetaData::valueTypeInt32)
    , _currentFact                  (0, _currentFactName,                   FactMetaData::valueTypeInt32)
    , _temperatureFact              (0, _temperatureFactName,               FactMetaData::valueTypeDouble)
    , _cellCountFact                (0, _cellCountFactName,                 FactMetaData::valueTypeInt32)
    , _voltagesFact                 (0, _voltagesFactName,                  FactMetaData::valueTypeInt32)
{
    _addFact(&_voltageFact,                 _voltageFactName);
    _addFact(&_percentRemainingFact,        _percentRemainingFactName);
    _addFact(&_mahConsumedFact,             _mahConsumedFactName);
    _addFact(&_currentFact,                 _currentFactName);
    _addFact(&_temperatureFact,             _temperatureFactName);
    _addFact(&_cellCountFact,               _cellCountFactName);
    _addFact(&_voltagesFact,                _voltagesFactName);

    // Start out as not available
    _voltageFact.setRawValue            (_voltageUnavailable);
    _percentRemainingFact.setRawValue   (_percentRemainingUnavailable);
    _mahConsumedFact.setRawValue        (_mahConsumedUnavailable);
    _currentFact.setRawValue            (_currentUnavailable);
    _temperatureFact.setRawValue        (_temperatureUnavailable);
    _cellCountFact.setRawValue          (_cellCountUnavailable);
    _voltagesFact.setRawValue           (_voltagesUnavailable);
}

void VehicleBatteryFactGroup::setVehicle(Vehicle* vehicle)
{
    _vehicle = vehicle;
}

const char* VehicleWindFactGroup::_directionFactName =      "direction";
const char* VehicleWindFactGroup::_speedFactName =          "speed";
const char* VehicleWindFactGroup::_verticalSpeedFactName =  "verticalSpeed";

VehicleWindFactGroup::VehicleWindFactGroup(QObject* parent)
    : FactGroup(1000, loadseet(), parent)
    , _vehicle(NULL)
    , _directionFact    (0, _directionFactName,     FactMetaData::valueTypeDouble)
    , _speedFact        (0, _speedFactName,         FactMetaData::valueTypeDouble)
    , _verticalSpeedFact(0, _verticalSpeedFactName, FactMetaData::valueTypeDouble)
{
    _addFact(&_directionFact,       _directionFactName);
    _addFact(&_speedFact,           _speedFactName);
    _addFact(&_verticalSpeedFact,   _verticalSpeedFactName);

    // Start out as not available "--.--"
    _directionFact.setRawValue      (std::numeric_limits<float>::quiet_NaN());
    _speedFact.setRawValue          (std::numeric_limits<float>::quiet_NaN());
    _verticalSpeedFact.setRawValue  (std::numeric_limits<float>::quiet_NaN());
}

void VehicleWindFactGroup::setVehicle(Vehicle* vehicle)
{
    _vehicle = vehicle;
}

const char* VehicleVibrationFactGroup::_xAxisFactName =      "xAxis";
const char* VehicleVibrationFactGroup::_yAxisFactName =      "yAxis";
const char* VehicleVibrationFactGroup::_zAxisFactName =      "zAxis";
const char* VehicleVibrationFactGroup::_clipCount1FactName = "clipCount1";
const char* VehicleVibrationFactGroup::_clipCount2FactName = "clipCount2";
const char* VehicleVibrationFactGroup::_clipCount3FactName = "clipCount3";

VehicleVibrationFactGroup::VehicleVibrationFactGroup(QObject* parent)
    : FactGroup(1000, loadseet(), parent)
    , _vehicle(NULL)
    , _xAxisFact        (0, _xAxisFactName,         FactMetaData::valueTypeDouble)
    , _yAxisFact        (0, _yAxisFactName,         FactMetaData::valueTypeDouble)
    , _zAxisFact        (0, _zAxisFactName,         FactMetaData::valueTypeDouble)
    , _clipCount1Fact   (0, _clipCount1FactName,    FactMetaData::valueTypeUint32)
    , _clipCount2Fact   (0, _clipCount2FactName,    FactMetaData::valueTypeUint32)
    , _clipCount3Fact   (0, _clipCount3FactName,    FactMetaData::valueTypeUint32)
{
    _addFact(&_xAxisFact,       _xAxisFactName);
    _addFact(&_yAxisFact,       _yAxisFactName);
    _addFact(&_zAxisFact,       _zAxisFactName);
    _addFact(&_clipCount1Fact,  _clipCount1FactName);
    _addFact(&_clipCount2Fact,  _clipCount2FactName);
    _addFact(&_clipCount3Fact,  _clipCount3FactName);

    // Start out as not available "--.--"
    _xAxisFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _yAxisFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
    _zAxisFact.setRawValue(std::numeric_limits<float>::quiet_NaN());
}

void VehicleVibrationFactGroup::setVehicle(Vehicle* vehicle)
{
    _vehicle = vehicle;
}
void Vehicle::doCommandLon(int component, int command, float param1, float param2, float param3, float param4, float param5, float param6, float param7)
{
    mavlink_message_t       msg;
    mavlink_command_long_t  cmd;

    cmd.command = command;
    cmd.confirmation = 0;
    cmd.param1 = param1;
    cmd.param2 = param2;
    cmd.param3 = param3;
    cmd.param4 = param4;
    cmd.param5 = param5;
    cmd.param6 = param6;
    cmd.param7 = param7;
    cmd.target_system = id();
    cmd.target_component = component;
    mavlink_msg_command_long_encode(_mavlink->getSystemId(), _mavlink->getComponentId(), &msg, &cmd);

    sendMessageOnPriorityLink(msg);
}

int Vehicle::vehicletype(void)
{
    switch(vehicleType ()){
    case MAV_TYPE_FIXED_WING:           //Flying Wing
        _type = 1;
       break;
    case MAV_TYPE_VTOL_DUOROTOR:    //tailsitter
        if(_vtolfw == false)
              _type = 2;
        else
            _type = 3;
        break;
    case MAV_TYPE_QUADROTOR:        //quadrotor
        _type = 4;
        break;
    case MAV_TYPE_HEXAROTOR:   //hexarotor
        _type =  5;
        break;
    case MAV_TYPE_OCTOROTOR:
        _type = 6;
       break;       //octorotor
    default:
        _type = 100;
    }
    return _type;
}
//=======================================

QString Vehicle::loadseet(){
             QSettings setting;
              //qDebug()<<setting.value("language",false).toBool()<<"queren";
             return setting.value("language",false).toBool()?":/json/Vehicle/VehicleFact.json":":/json/Vehicle/VehicleFactAdd.json";
}
QString VehicleBatteryFactGroup::loadseet(){
    QSettings setting;
     //qDebug()<<setting.value("language",false).toBool()<<"queren";
    return setting.value("language",false).toBool()?":/json/Vehicle/BatteryFact.json":":/json/Vehicle/BatteryFactAdd.json";
}
QString VehicleGPSFactGroup::loadseet(){
    QSettings setting;
     //qDebug()<<setting.value("language",false).toBool()<<"queren";
    return setting.value("language",false).toBool()?":/json/Vehicle/GPSFact.json":":/json/Vehicle/GPSFactAdd.json";
}
QString VehicleVibrationFactGroup::loadseet(){
    QSettings setting;
     //qDebug()<<setting.value("language",false).toBool()<<"queren";
    return setting.value("language",false).toBool()?":/json/Vehicle/VibrationFact.json":":/json/Vehicle/VibrationFactAdd.json";
}
QString VehicleWindFactGroup::loadseet(){
    QSettings setting;
     //qDebug()<<setting.value("language",false).toBool()<<"queren";
    return setting.value("language",false).toBool()?":/json/Vehicle/WindFact.json":":/json/Vehicle/WindFactAdd.json";
}
