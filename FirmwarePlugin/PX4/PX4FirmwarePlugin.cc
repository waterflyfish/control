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

#include "PX4FirmwarePlugin.h"
#include "PX4ParameterMetaData.h"
#include "QGCApplication.h"
#include "AutoPilotPlugins/PX4/PX4AutoPilotPlugin.h"    // FIXME: Hack

#include <QDebug>

#include "px4_custom_mode.h"
//======================================
#include <QSettings>

struct Modes2Name {
    uint8_t     main_mode;
    uint8_t     sub_mode;
    const char* name;       ///< Name for flight mode
    bool        canBeSet;   ///< true: Vehicle can be set to this flight mode
    bool        fixedWing;  /// fixed wing compatible
    bool        multiRotor;  /// multi rotor compatible
};

const char* PX4FirmwarePlugin::manualFlightMode =       "Manual";
const char* PX4FirmwarePlugin::altCtlFlightMode =       "Altitude";
const char* PX4FirmwarePlugin::posCtlFlightMode =       "Position";
const char* PX4FirmwarePlugin::missionFlightMode =      "Mission";
const char* PX4FirmwarePlugin::holdFlightMode =         "Hold";
const char* PX4FirmwarePlugin::takeoffFlightMode =      "Takeoff";
const char* PX4FirmwarePlugin::landingFlightMode =      "Land";
const char* PX4FirmwarePlugin::rtlFlightMode =          "Return";
const char* PX4FirmwarePlugin::acroFlightMode =         "Acro";
const char* PX4FirmwarePlugin::offboardFlightMode =     "Offboard";
const char* PX4FirmwarePlugin::stabilizedFlightMode =   "Stabilized";
const char* PX4FirmwarePlugin::rattitudeFlightMode =    "Rattitude";
const char* PX4FirmwarePlugin::followMeFlightMode =     "Follow Me";

const char* PX4FirmwarePlugin::rtgsFlightMode =         "Return to Groundstation";

const char* PX4FirmwarePlugin::readyFlightMode =        "Ready"; // unused

/// Tranlates from PX4 custom modes to flight mode names

static const struct Modes2Name rgModes2Name[] = {
    //main_mode                         sub_mode                                name                                      canBeSet  FW      MC
    { PX4_CUSTOM_MAIN_MODE_MANUAL,      0,                                      PX4FirmwarePlugin::manualFlightMode,        true,   true,   true },
    { PX4_CUSTOM_MAIN_MODE_STABILIZED,  0,                                      PX4FirmwarePlugin::stabilizedFlightMode,    true,   true,   true },
    { PX4_CUSTOM_MAIN_MODE_ACRO,        0,                                      PX4FirmwarePlugin::acroFlightMode,          true,   false,  true },
    { PX4_CUSTOM_MAIN_MODE_RATTITUDE,   0,                                      PX4FirmwarePlugin::rattitudeFlightMode,     true,   false,  true },
    { PX4_CUSTOM_MAIN_MODE_ALTCTL,      0,                                      PX4FirmwarePlugin::altCtlFlightMode,        true,   true,   true },
    { PX4_CUSTOM_MAIN_MODE_POSCTL,      0,                                      PX4FirmwarePlugin::posCtlFlightMode,        true,   true,   true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_LOITER,        PX4FirmwarePlugin::holdFlightMode,          true,   true,   true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_MISSION,       PX4FirmwarePlugin::missionFlightMode,       true,   true,   true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_RTL,           PX4FirmwarePlugin::rtlFlightMode,           true,   true,   true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_FOLLOW_TARGET, PX4FirmwarePlugin::followMeFlightMode,      true,   true,   true },
    { PX4_CUSTOM_MAIN_MODE_OFFBOARD,    0,                                      PX4FirmwarePlugin::offboardFlightMode,      true,   true,   true },
    // modes that can't be directly set by the user
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_LAND,          PX4FirmwarePlugin::landingFlightMode,       false,  true,   true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_READY,         PX4FirmwarePlugin::readyFlightMode,         false,  true,   true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_RTGS,          PX4FirmwarePlugin::rtgsFlightMode,          false,  true,   true },
    { PX4_CUSTOM_MAIN_MODE_AUTO,        PX4_CUSTOM_SUB_MODE_AUTO_TAKEOFF,       PX4FirmwarePlugin::takeoffFlightMode,       false,  true,   true },
};

PX4FirmwarePlugin::PX4FirmwarePlugin(void)
    : _versionNotified(false)
    , _isPosCtlMode(false)
{

}

QList<VehicleComponent*> PX4FirmwarePlugin::componentsForVehicle(AutoPilotPlugin* vehicle)
{
    Q_UNUSED(vehicle);

    return QList<VehicleComponent*>();
}

QStringList PX4FirmwarePlugin::flightModes(Vehicle* vehicle)
{
    QStringList flightModes;
    QSettings setting;
    QString namee;
    bool m = setting.value("language",false).toBool();
    for (size_t i=0; i<sizeof(rgModes2Name)/sizeof(rgModes2Name[0]); i++) {
        const struct Modes2Name* pModes2Name = &rgModes2Name[i];

        if (pModes2Name->canBeSet) {
            bool fw = (vehicle->fixedWing() && pModes2Name->fixedWing);
            bool mc = (vehicle->multiRotor() && pModes2Name->multiRotor);

            // show all modes for generic, vtol, etc
            bool other = !vehicle->fixedWing() && !vehicle->multiRotor();

            if (fw || mc || other) {
                if(m == false){
                    if((pModes2Name->name)==PX4FirmwarePlugin::manualFlightMode){
                        namee = QStringLiteral("手动");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::altCtlFlightMode){
                        namee = QStringLiteral("定高");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::posCtlFlightMode){
                        namee = QStringLiteral("定点");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::missionFlightMode){
                        namee = QStringLiteral("任务");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::holdFlightMode){
                        namee = QStringLiteral("悬停");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::takeoffFlightMode){
                        namee = QStringLiteral("起飞");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::landingFlightMode){
                        namee = QStringLiteral("降落");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::rtlFlightMode){
                        namee = QStringLiteral("返回");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::acroFlightMode){
                        namee = QStringLiteral("特技");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::offboardFlightMode){
                        namee = QStringLiteral("外控");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::stabilizedFlightMode){
                        namee = QStringLiteral("增稳");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::rattitudeFlightMode){
                        namee = QStringLiteral("姿态");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::followMeFlightMode){
                        namee = QStringLiteral("跟随");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::rtgsFlightMode){
                        namee = QStringLiteral("返回到地面站");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::readyFlightMode){
                        namee = QStringLiteral("准备");
                    }
                    flightModes += namee;
                }
                if(m){
                    flightModes += pModes2Name->name;
                }
            }
        }
    }

    return flightModes;
}

QString PX4FirmwarePlugin::flightMode(uint8_t base_mode, uint32_t custom_mode) const
{
    QString flightMode = "Unknown";
    QSettings setting;
    QString namee;
    bool m = setting.value("language",false).toBool();
    if(!m){
        flightMode = QStringLiteral("未知");
    }

    if (base_mode & MAV_MODE_FLAG_CUSTOM_MODE_ENABLED) {
        union px4_custom_mode px4_mode;
        px4_mode.data = custom_mode;

        bool found = false;
        for (size_t i=0; i<sizeof(rgModes2Name)/sizeof(rgModes2Name[0]); i++) {
            const struct Modes2Name* pModes2Name = &rgModes2Name[i];

            if (pModes2Name->main_mode == px4_mode.main_mode && pModes2Name->sub_mode == px4_mode.sub_mode) {
                if(m == false){
                    if((pModes2Name->name)==PX4FirmwarePlugin::manualFlightMode){
                        namee =QStringLiteral("手动");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::altCtlFlightMode){
                        namee = QStringLiteral("定高");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::posCtlFlightMode){
                        namee = QStringLiteral("定点");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::missionFlightMode){
                        namee = QStringLiteral("任务");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::holdFlightMode){
                        namee = QStringLiteral("悬停");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::takeoffFlightMode){
                        namee = QStringLiteral("起飞");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::landingFlightMode){
                        namee = QStringLiteral("降落");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::rtlFlightMode){
                        namee = QStringLiteral("返回");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::acroFlightMode){
                        namee = QStringLiteral("特技");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::offboardFlightMode){
                        namee = "外控";
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::stabilizedFlightMode){
                        namee = QStringLiteral("增稳");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::rattitudeFlightMode){
                        namee = QStringLiteral("姿态");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::followMeFlightMode){
                        namee = QStringLiteral("跟随");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::rtgsFlightMode){
                        namee = QStringLiteral("返回到地面站");
                    }
                    if((pModes2Name->name)== PX4FirmwarePlugin::readyFlightMode){
                        namee = QStringLiteral("准备");
                    }
                    flightMode =namee;
                }
                if(m){
                    flightMode = pModes2Name->name;
                }
                found = true;
                break;
            }
        }

        if (!found) {
            qWarning() << "Unknown flight mode" << custom_mode;
        }
    } else {
        qWarning() << "PX4 Flight Stack flight mode without custom mode enabled?";
    }
    return flightMode;
}

bool PX4FirmwarePlugin::setFlightMode(const QString& flightMode, uint8_t* base_mode, uint32_t* custom_mode)
{
    *base_mode = 0;
    *custom_mode = 0;
    QSettings setting;
    QString namee;
    QString flightModee;
    bool m = setting.value("language",false).toBool();
    bool found = false;
    for (size_t i=0; i<sizeof(rgModes2Name)/sizeof(rgModes2Name[0]); i++) {
        const struct Modes2Name* pModes2Name = &rgModes2Name[i];
        if(m == false){
            if(flightMode==PX4FirmwarePlugin::manualFlightMode){
                flightModee=QStringLiteral("手动");
            }

            if(flightMode==PX4FirmwarePlugin::altCtlFlightMode){
                flightModee=QStringLiteral("定高");
            }

            if(flightMode==PX4FirmwarePlugin::posCtlFlightMode){
                flightModee=QStringLiteral("定点");
            }

            if(flightMode==PX4FirmwarePlugin::missionFlightMode){
                flightModee=QStringLiteral("任务");
            }

            if(flightMode==PX4FirmwarePlugin::holdFlightMode){
                flightModee=QStringLiteral("悬停");
            }

            if(flightMode==PX4FirmwarePlugin::takeoffFlightMode){
                flightModee=QStringLiteral("起飞");
            }

            if(flightMode==PX4FirmwarePlugin::landingFlightMode){
                flightModee=QStringLiteral("降落");
            }

            if(flightMode==PX4FirmwarePlugin::rtlFlightMode){
                flightModee=QStringLiteral("返回");
            }

            if(flightMode==PX4FirmwarePlugin::acroFlightMode){
                flightModee=QStringLiteral("特技");
            }

            if(flightMode==PX4FirmwarePlugin::offboardFlightMode){
                flightModee=QStringLiteral("外控");
            }

            if(flightMode==PX4FirmwarePlugin::stabilizedFlightMode){
                flightModee=QStringLiteral("增稳");
            }

            if(flightMode==PX4FirmwarePlugin::rattitudeFlightMode){
                flightModee=QStringLiteral("姿态");
            }

            if(flightMode==PX4FirmwarePlugin::followMeFlightMode){
                flightModee=QStringLiteral("跟随");
            }

            if(flightMode==PX4FirmwarePlugin::rtgsFlightMode){
                flightModee=QStringLiteral("返回到地面站");
            }
            if(flightMode==PX4FirmwarePlugin::readyFlightMode){
                flightModee=QStringLiteral("准备");
            }

            if((pModes2Name->name)==PX4FirmwarePlugin::manualFlightMode){
                namee = QStringLiteral("手动");
            }
            if((pModes2Name->name)== PX4FirmwarePlugin::altCtlFlightMode){
                namee = QStringLiteral("定高");
            }
            if((pModes2Name->name)== PX4FirmwarePlugin::posCtlFlightMode){
                namee = QStringLiteral("定点");
            }
            if((pModes2Name->name)== PX4FirmwarePlugin::missionFlightMode){
                namee = QStringLiteral("任务");
            }
            if((pModes2Name->name)== PX4FirmwarePlugin::holdFlightMode){
                namee = QStringLiteral("悬停");
            }
            if((pModes2Name->name)== PX4FirmwarePlugin::takeoffFlightMode){
                namee = QStringLiteral("起飞");
            }
            if((pModes2Name->name)== PX4FirmwarePlugin::landingFlightMode){
                namee = QStringLiteral("降落");
            }
            if((pModes2Name->name)== PX4FirmwarePlugin::rtlFlightMode){
                namee = QStringLiteral("返回");
            }
            if((pModes2Name->name)== PX4FirmwarePlugin::acroFlightMode){
                namee = QStringLiteral("特技");
            }
            if((pModes2Name->name)== PX4FirmwarePlugin::offboardFlightMode){
                namee = QStringLiteral("外控");
            }
            if((pModes2Name->name)== PX4FirmwarePlugin::stabilizedFlightMode){
                namee = QStringLiteral("增稳");
            }
            if((pModes2Name->name)== PX4FirmwarePlugin::rattitudeFlightMode){
                namee = QStringLiteral("姿态");
            }
            if((pModes2Name->name)== PX4FirmwarePlugin::followMeFlightMode){
                namee = QStringLiteral("跟随");
            }
            if((pModes2Name->name)== PX4FirmwarePlugin::rtgsFlightMode){
                namee = QStringLiteral("返回到地面站");
            }
            if((pModes2Name->name)== PX4FirmwarePlugin::readyFlightMode){
                namee = QStringLiteral("准备");
            }
            if (flightModee.compare(namee, Qt::CaseInsensitive) == 0||flightMode.compare(namee, Qt::CaseInsensitive) == 0) {
                union px4_custom_mode px4_mode;

                px4_mode.data = 0;
                px4_mode.main_mode = pModes2Name->main_mode;
                px4_mode.sub_mode = pModes2Name->sub_mode;

                *base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
                *custom_mode = px4_mode.data;

                found = true;
                break;
            }
        }
        if(m){
            if (flightMode.compare(pModes2Name->name, Qt::CaseInsensitive) == 0) {
                union px4_custom_mode px4_mode;

            px4_mode.data = 0;
            px4_mode.main_mode = pModes2Name->main_mode;
            px4_mode.sub_mode = pModes2Name->sub_mode;

            *base_mode = MAV_MODE_FLAG_CUSTOM_MODE_ENABLED;
            *custom_mode = px4_mode.data;

            found = true;
            break;
            }
        }
    }

    if (!found) {
        qWarning() << "Unknown flight Mode" << flightMode;
    }

    return found;
}

bool PX4FirmwarePlugin::isPosCtlMode(Vehicle *vehicle)
{
    QString flightMode = vehicle->flightMode ();
    if(flightMode == QStringLiteral("定点") || flightMode == "Position"){
        _isPosCtlMode = true;
    }else{
        _isPosCtlMode = false;
    }
    return _isPosCtlMode;
}

int PX4FirmwarePlugin::manualControlReservedButtonCount(void)
{
    return 0;   // 0 buttons reserved for rc switch simulation
}

bool PX4FirmwarePlugin::supportsManualControl(void)
{
    return true;
}

bool PX4FirmwarePlugin::isCapable(const Vehicle *vehicle, FirmwareCapabilities capabilities)
{
    if(vehicle->multiRotor())
        return (capabilities & (MavCmdPreflightStorageCapability | GuidedModeCapability | SetFlightModeCapability | PauseVehicleCapability | OrbitModeCapability)) == capabilities;
    else if(vehicle->vtol ())
        return (capabilities & (MavCmdPreflightStorageCapability | GuidedModeCapability | SetFlightModeCapability | PauseVehicleCapability | OrbitModeCapability | McandFwModeCapability)) == capabilities;

    return (capabilities & (MavCmdPreflightStorageCapability | GuidedModeCapability | SetFlightModeCapability | PauseVehicleCapability | ParatureModeCapability)) == capabilities;
}

void PX4FirmwarePlugin::initializeVehicle(Vehicle* vehicle)
{
    Q_UNUSED(vehicle);

    // PX4 Flight Stack doesn't need to do any extra work
}

bool PX4FirmwarePlugin::sendHomePositionToVehicle(void)
{
    // PX4 stack does not want home position sent in the first position.
    // Subsequent sequence numbers must be adjusted.
    return false;
}

void PX4FirmwarePlugin::addMetaDataToFact(QObject* parameterMetaData, Fact* fact, MAV_TYPE vehicleType)
{
    PX4ParameterMetaData* px4MetaData = qobject_cast<PX4ParameterMetaData*>(parameterMetaData);

    if (px4MetaData) {
        px4MetaData->addMetaDataToFact(fact, vehicleType);
    } else {
        qWarning() << "Internal error: pointer passed to PX4FirmwarePlugin::addMetaDataToFact not PX4ParameterMetaData";
    }
}

QList<MAV_CMD> PX4FirmwarePlugin::supportedMissionCommands(void)
{
    QList<MAV_CMD> list;

    list << MAV_CMD_NAV_WAYPOINT
         << MAV_CMD_NAV_LOITER_UNLIM << MAV_CMD_NAV_LOITER_TIME << MAV_CMD_NAV_LOITER_TO_ALT
         << MAV_CMD_NAV_LAND << MAV_CMD_NAV_TAKEOFF
         << MAV_CMD_DO_JUMP
         << MAV_CMD_DO_VTOL_TRANSITION << MAV_CMD_NAV_VTOL_TAKEOFF << MAV_CMD_NAV_VTOL_LAND
         << MAV_CMD_DO_DIGICAM_CONTROL
         << MAV_CMD_DO_SET_CAM_TRIGG_DIST
         << MAV_CMD_DO_SET_SERVO
         << MAV_CMD_DO_CHANGE_SPEED
         <<MAV_CMD_DO_PARACHUTE
         << MAV_CMD_DO_LAND_START
         << MAV_CMD_DO_MOUNT_CONFIGURE
         << MAV_CMD_DO_MOUNT_CONTROL;

    return list;
}

QString PX4FirmwarePlugin::missionCommandOverrides(MAV_TYPE vehicleType) const
{
    QSettings setting;
    bool m = setting.value("language",0).toBool();
    switch (vehicleType) {
    case MAV_TYPE_GENERIC:
        if(m==false)return QStringLiteral(":/json/PX4/MavCmdInfoCommon.json");
        if(m==true)return QStringLiteral(":/json/PX4/MavCmdInfoCommonAdd.json");
        break;
    case MAV_TYPE_FIXED_WING:
        if(m==false)return QStringLiteral(":/json/PX4/MavCmdInfoFixedWing.json");
        if(m==true)return QStringLiteral(":/json/PX4/MavCmdInfoFixedWingAdd.json");
        break;
    case MAV_TYPE_QUADROTOR:
        if(m==false)return QStringLiteral(":/json/PX4/MavCmdInfoMultiRotor.json");
        if(m==true)return QStringLiteral(":/json/PX4/MavCmdInfoMultiRotorAdd.json");
        break;
    case MAV_TYPE_VTOL_QUADROTOR:
        if(m==false)return QStringLiteral(":/json/APM/MavCmdInfoVTOL.json");
        if(m==true)return QStringLiteral(":/json/APM/MavCmdInfoVTOLAdd.json");
        break;
    case MAV_TYPE_SUBMARINE:
        if(m==false)return QStringLiteral(":/json/PX4/MavCmdInfoSub.json");
        if(m==true)return QStringLiteral(":/json/PX4/MavCmdInfoSubAdd.json");
        break;
    case MAV_TYPE_GROUND_ROVER:
        if(m==false)return QStringLiteral(":/json/PX4/MavCmdInfoRover.json");
        if(m==true)return QStringLiteral(":/json/PX4/MavCmdInfoRoverAdd.json");
        break;
    default:
        qWarning() << "PX4FirmwarePlugin::missionCommandOverrides called with bad MAV_TYPE:" << vehicleType;
        return QString();
    }
    return QString();
}

QObject* PX4FirmwarePlugin::loadParameterMetaData(const QString& metaDataFile)
{
    PX4ParameterMetaData* metaData = new PX4ParameterMetaData;
    if (!metaDataFile.isEmpty()) {
        metaData->loadParameterFactMetaDataFile(metaDataFile);
    }
    return metaData;
}

void PX4FirmwarePlugin::pauseVehicle(Vehicle* vehicle)
{
    // then tell it to loiter at the current position
    mavlink_message_t msg;
    mavlink_command_long_t cmd;

    cmd.command = (uint16_t)MAV_CMD_DO_REPOSITION;
    cmd.confirmation = 0;
    cmd.param1 = -1.0f;
    cmd.param2 = MAV_DO_REPOSITION_FLAGS_CHANGE_MODE;
    cmd.param3 = 0.0f;
    cmd.param4 = NAN;
    cmd.param5 = NAN;
    cmd.param6 = NAN;
    cmd.param7 = NAN;
    cmd.target_system = vehicle->id();
    cmd.target_component = vehicle->defaultComponentId();

    MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    mavlink_msg_command_long_encode_chan(mavlink->getSystemId(),
                                         mavlink->getComponentId(),
                                         vehicle->priorityLink()->mavlinkChannel(),
                                         &msg,
                                         &cmd);

    vehicle->sendMessageOnLink(vehicle->priorityLink(), msg);
}

void PX4FirmwarePlugin::guidedModeRTL(Vehicle* vehicle)
{
    vehicle->setFlightMode(rtlFlightMode);
}

void PX4FirmwarePlugin::guidedModeLand(Vehicle* vehicle)
{
    vehicle->setFlightMode(landingFlightMode);
}

void PX4FirmwarePlugin::guidedModeParachute(Vehicle *vehicle)
{
    // tell vehicle to parachute at current position
    mavlink_message_t msg;
    mavlink_command_long_t cmd;

    cmd.command = (uint16_t)MAV_CMD_DO_PARACHUTE;
    cmd.confirmation = 0;
    cmd.param1 = -1.0f;
    cmd.param2 = 0;
    cmd.param3 = 1;
    cmd.param4 = NAN;
    cmd.param5 = NAN;
    cmd.param6 = NAN;
    cmd.param7 = NAN;
    cmd.target_system = vehicle->id();
    cmd.target_component = vehicle->defaultComponentId();

    MAVLinkProtocol *mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    mavlink_msg_command_long_encode_chan(mavlink->getSystemId(),
                                         mavlink->getComponentId(),
                                         vehicle->priorityLink()->mavlinkChannel(),
                                         &msg,
                                         &cmd);
    vehicle->sendMessageOnLink(vehicle->priorityLink(),msg);
    qDebug()<<"send message parachute,thro";
}

void PX4FirmwarePlugin::guidedModeVTOLMcandFw(Vehicle *vehicle)
{
    mavlink_message_t msg;
    mavlink_command_long_t cmd;
    float state;
    if(!vehicle->vtol ())
        return ;
    if(vehicle->vehicletype () == 2)
        state = 4;
    else
        state = 3;

    cmd.command = (uint16_t)MAV_CMD_DO_VTOL_TRANSITION;
    cmd.confirmation = 0;
    cmd.param1 = state;
    cmd.param2 = 0;
    cmd.param3 = 0;
    cmd.param4 = NAN;
    cmd.param5 = NAN;
    cmd.param6 = NAN;
    cmd.param7 = NAN;
    cmd.target_system = vehicle->id();
    cmd.target_component = vehicle->defaultComponentId();

    MAVLinkProtocol *mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    mavlink_msg_command_long_encode_chan(mavlink->getSystemId(),
                                         mavlink->getComponentId(),
                                         vehicle->priorityLink()->mavlinkChannel(),
                                         &msg,
                                         &cmd);
    vehicle->sendMessageOnLink(vehicle->priorityLink(),msg);
    qDebug()<<"swtich to state "<<state;
}

void PX4FirmwarePlugin::guidedModeMission(Vehicle *vehicle)
{
    vehicle->setFlightMode (missionFlightMode);
}

void PX4FirmwarePlugin::guidedModePosition(Vehicle *vehicle)
{
    vehicle->setFlightMode (posCtlFlightMode);
}

//void PX4FirmwarePlugin::guidedModeOrbit(Vehicle* vehicle, const QGeoCoordinate& centerCoord, double radius, double velocity, double altitude)
//{
//    //-- If not in "guided" mode, make it so.
//    if(!isGuidedMode(vehicle))
//        setGuidedMode(vehicle, true);
//    MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
//    mavlink_message_t msg;
//    mavlink_command_long_t cmd;
//    cmd.command = (uint16_t)MAV_CMD_SET_GUIDED_SUBMODE_CIRCLE;
//    cmd.confirmation = 0;
//    cmd.param1 = radius;
//    cmd.param2 = velocity;
//    cmd.param3 = altitude;
//    cmd.param4 = NAN;
//    cmd.param5 = centerCoord.isValid() ? centerCoord.latitude()  : NAN;
//    cmd.param6 = centerCoord.isValid() ? centerCoord.longitude() : NAN;
//    cmd.param7 = centerCoord.isValid() ? centerCoord.altitude()  : NAN;
//    cmd.target_system = vehicle->id();
//    cmd.target_component = vehicle->defaultComponentId();
//    mavlink_msg_command_long_encode_chan(mavlink->getSystemId(),
//                                         mavlink->getComponentId(),
//                                         vehicle->priorityLink()->mavlinkChannel(),
//                                         &msg,
//                                         &cmd);
//    vehicle->sendMessageOnLink(vehicle->priorityLink(), msg);
//}

void PX4FirmwarePlugin::guidedModeTakeoff(Vehicle* vehicle, double altitudeRel)
{
    Q_UNUSED(altitudeRel);
    if (qIsNaN(vehicle->altitudeAMSL()->rawValue().toDouble())) {
        qgcApp()->showMessage(QStringLiteral("Unable to takeoff, vehicle position not known."));
        return;
    }

    MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();

    // Set destination altitude
    mavlink_message_t msg;
    mavlink_command_long_t cmd;

    cmd.command = (uint16_t)MAV_CMD_NAV_TAKEOFF;
    cmd.confirmation = 0;
    cmd.param1 = -1.0f;
    cmd.param2 = 0.0f;
    cmd.param3 = 0.0f;
    cmd.param4 = NAN;
    cmd.param5 = NAN;
    cmd.param6 = NAN;
    cmd.param7 = vehicle->altitudeAMSL()->rawValue().toDouble() + altitudeRel;
    cmd.target_system = vehicle->id();
    cmd.target_component = vehicle->defaultComponentId();

    mavlink_msg_command_long_encode_chan(mavlink->getSystemId(),
                                         mavlink->getComponentId(),
                                         vehicle->priorityLink()->mavlinkChannel(),
                                         &msg,
                                         &cmd);
    vehicle->sendMessageOnLink(vehicle->priorityLink(), msg);
}

void PX4FirmwarePlugin::guidedModeGotoLocation(Vehicle* vehicle, const QGeoCoordinate& gotoCoord)
{
    if (qIsNaN(vehicle->altitudeAMSL()->rawValue().toDouble())) {
        qgcApp()->showMessage(QStringLiteral("Unable to go to location, vehicle position not known."));
        return;
    }

    mavlink_message_t msg;
    mavlink_command_long_t cmd;

    cmd.command = (uint16_t)MAV_CMD_DO_REPOSITION;
    cmd.confirmation = 0;
    cmd.param1 = -1.0f;
    cmd.param2 = MAV_DO_REPOSITION_FLAGS_CHANGE_MODE;
    cmd.param3 = 0.0f;
    cmd.param4 = NAN;
    cmd.param5 = gotoCoord.latitude();
    cmd.param6 = gotoCoord.longitude();
    cmd.param7 = vehicle->altitudeAMSL()->rawValue().toDouble();
    cmd.target_system = vehicle->id();
    cmd.target_component = vehicle->defaultComponentId();

    MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
    mavlink_msg_command_long_encode_chan(mavlink->getSystemId(),
                                         mavlink->getComponentId(),
                                         vehicle->priorityLink()->mavlinkChannel(),
                                         &msg,
                                         &cmd);

    vehicle->sendMessageOnLink(vehicle->priorityLink(), msg);
}

//void PX4FirmwarePlugin::guidedModeChangeAltitude(Vehicle* vehicle, double altitudeRel)
//{
//    if (!vehicle->homePositionAvailable()) {
//        qgcApp()->showMessage(QStringLiteral("Unable to change altitude, home position unknown."));
//        return;
//    }
//    if (qIsNaN(vehicle->homePosition().altitude())) {
//        qgcApp()->showMessage(QStringLiteral("Unable to change altitude, home position altitude unknown."));
//        return;
//    }

//    mavlink_message_t msg;
//    mavlink_command_long_t cmd;

//    cmd.command = (uint16_t)MAV_CMD_DO_REPOSITION;
//    cmd.confirmation = 0;
//    cmd.param1 = -1.0f;
//    cmd.param2 = MAV_DO_REPOSITION_FLAGS_CHANGE_MODE;
//    cmd.param3 = 0.0f;
//    cmd.param4 = NAN;
//    cmd.param5 = NAN;
//    cmd.param6 = NAN;
//    cmd.param7 = vehicle->homePosition().altitude() + altitudeRel;
//    cmd.target_system = vehicle->id();
//    cmd.target_component = vehicle->defaultComponentId();

//    MAVLinkProtocol* mavlink = qgcApp()->toolbox()->mavlinkProtocol();
//    mavlink_msg_command_long_encode_chan(mavlink->getSystemId(),
//                                         mavlink->getComponentId(),
//                                         vehicle->priorityLink()->mavlinkChannel(),
//                                         &msg,
//                                         &cmd);

//    vehicle->sendMessageOnLink(vehicle->priorityLink(), msg);
//}

void PX4FirmwarePlugin::setGuidedMode(Vehicle* vehicle, bool guidedMode)
{
    if (guidedMode) {
        vehicle->setFlightMode(holdFlightMode);
    } else {
        pauseVehicle(vehicle);
    }
}

bool PX4FirmwarePlugin::isGuidedMode(const Vehicle* vehicle) const
{
    // Not supported by generic vehicle
    return (vehicle->flightMode() == holdFlightMode || vehicle->flightMode() == takeoffFlightMode
            || vehicle->flightMode() == landingFlightMode );
}

bool PX4FirmwarePlugin::isFollowMeMode(const Vehicle* vehicle) const
{
    return (vehicle->flightMode() == followMeFlightMode);
}

bool PX4FirmwarePlugin::adjustIncomingMavlinkMessage(Vehicle* vehicle, mavlink_message_t* message)
{
    //-- Don't process messages to/from UDP Bridge. It doesn't suffer from these issues
    if (message->compid == MAV_COMP_ID_UDP_BRIDGE) {
        return true;
    }

    switch (message->msgid) {
    case MAVLINK_MSG_ID_AUTOPILOT_VERSION:
        _handleAutopilotVersion(vehicle, message);
        break;
    }

    return true;
}

void PX4FirmwarePlugin::_handleAutopilotVersion(Vehicle* vehicle, mavlink_message_t* message)
{
    Q_UNUSED(vehicle);

    if (!_versionNotified) {
        bool notifyUser = false;
        int supportedMajorVersion = 1;
        int supportedMinorVersion = 4;
        int supportedPatchVersion = 1;

        mavlink_autopilot_version_t version;
        mavlink_msg_autopilot_version_decode(message, &version);

        if (version.flight_sw_version != 0) {
            int majorVersion, minorVersion, patchVersion;

            majorVersion = (version.flight_sw_version >> (8*3)) & 0xFF;
            minorVersion = (version.flight_sw_version >> (8*2)) & 0xFF;
            patchVersion = (version.flight_sw_version >> (8*1)) & 0xFF;

            if (majorVersion < supportedMajorVersion) {
                notifyUser = true;
            } else if (majorVersion == supportedMajorVersion) {
                if (minorVersion < supportedMinorVersion) {
                    notifyUser = true;
                } else if (minorVersion == supportedMinorVersion) {
                    notifyUser = patchVersion < supportedPatchVersion;
                }
            }
        } else {
            notifyUser = true;
        }

        if (notifyUser) {
            _versionNotified = true;
            //qgcApp()->showMessage(QString(tr("QGroundControl supports PX4 Pro firmware Version %1.%2.%3 and above. You are using a version prior to that which will lead to unpredictable results. Please upgrade your firmware.")).arg(supportedMajorVersion).arg(supportedMinorVersion).arg(supportedPatchVersion));
        }
    }
}
QString PX4FirmwarePlugin::missionCommandOverridesAdd(MAV_TYPE vehicleType) const
{
    switch (vehicleType) {
    case MAV_TYPE_GENERIC:
        return QStringLiteral(":/json/PX4/MavCmdInfoCommonAdd.json");
        break;
    case MAV_TYPE_FIXED_WING:
        return QStringLiteral(":/json/PX4/MavCmdInfoFixedWingAdd.json");
        break;
    case MAV_TYPE_QUADROTOR:
        return QStringLiteral(":/json/PX4/MavCmdInfoMultiRotorAdd.json");
        break;
    case MAV_TYPE_VTOL_QUADROTOR:
        return QStringLiteral(":/json/APM/MavCmdInfoVTOLAdd.json");
        break;
    case MAV_TYPE_SUBMARINE:
        return QStringLiteral(":/json/PX4/MavCmdInfoSubAdd.json");
        break;
    case MAV_TYPE_GROUND_ROVER:
        return QStringLiteral(":/json/PX4/MavCmdInfoRoverAdd.json");
        break;
    default:
        qWarning() << "PX4FirmwarePlugin::missionCommandOverrides called with bad MAV_TYPE:" << vehicleType;
        return QString();
    }
}
