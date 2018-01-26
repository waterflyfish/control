/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

///  include tx1 mavlink message

#include "Tx1Manager.h"
#include "Vehicle.h"
#include "QGCApplication.h"

// define for including file order
/* Wrong include order: MAVLINK_TX1.H MUST NOT BE DIRECTLY USED.
 * Include mavlink.h from the mavlink 2.0 directory instead or set ALL AND EVERY defines from MAVLINK.H manually accordingly,
 * including the #define MAVLINK_H call.
 * */
#ifndef MAVLINK_TYPES_H_
#define MAVLINK_TYPES_H_
#endif

#ifndef _CHECKSUM_H_
#define _CHECKSUM_H_
#endif

#ifndef  _MAVLINK_CONVERSIONS_H_
#define  _MAVLINK_CONVERSIONS_H_
#endif

#ifndef  HAVE_MAVLINK_SHA256
#define  HAVE_MAVLINK_SHA256
#endif

#ifndef  _MAVLINK_HELPERS_H_
#define  _MAVLINK_HELPERS_H_
#endif

#ifndef  _MAVLINK_PROTOCOL_H_
#define  _MAVLINK_PROTOCOL_H_
#endif

#ifndef  _MAVLINK_GET_INFO_H_
#define  _MAVLINK_GET_INFO_H_
#endif

#include <tx1/tx1/tx1.h>
#include <tx1/tx1_debug/tx1_debug.h>

// Enums tx1 com chan type
//typedef enum TX1_COM_PIPE_CHAN_TYPE
//{
//    TX1_COM_PIPE_CHAN_TYPE_UNKNOWN = CHAN_TYPE_UNKNOWN, /* Used if the communication channel type is unknown for some reason | */
//    TX1_COM_PIPE_CHAN_TYPE_FIFO = CHAN_TYPE_FIFO, /*  | */
//    TX1_COM_PIPE_CHAN_TYPE_UDP = CHAN_TYPE_UDP, /*  | */
//    TX1_COM_PIPE_CHAN_TYPE_SERIAL = CHAN_TYPE_SERIAL, /*  | */
//    TX1_COM_PIPE_CHAN_TYPE_UNSET = CHAN_TYPE_UNSET, /*  | */
//    TX1_COM_PIPE_CHAN_TYPE_ENUM_END = TX1_COM_CHAN_TYPE_ENUM_END, /*  | */
//} TX1_COM_PIPE_CHAN_TYPE;


Tx1Manager::Tx1Manager(Vehicle* vehicle)
    : _vehicle(vehicle)
    , _mavlink(NULL)
//    , _type(TX1_COM_PIPE_CHAN_TYPE_UDP)
    , _runing(false)
    , _recording(false)
    , _tracking(false)
    #ifdef QGC_TX1_TEST_UDP
    , _id(1)       // Test target id
    #endif
{
    _mavlink = qgcApp()->toolbox()->mavlinkProtocol();
#ifndef QGC_TX1_TEST_UDP
    if(_vehicle){
        QObject::connect(_vehicle, &Vehicle::mavlinkMessageReceived, this, &Tx1Manager::_handleTx1Message);
    }
#else
    if(_mavlink){
        QObject::connect(_mavlink, SIGNAL(messageReceived(LinkInterface*, mavlink_message_t)), this, SLOT(_handleTx1Message(LinkInterface*, mavlink_message_t)));
    }
    QTimer::singleShot(6000, this, SLOT(_testTargetTracked()));
#endif
}

Tx1Manager::~Tx1Manager(){
}

#ifdef QGC_TX1_TEST_UDP
void Tx1Manager::_testTargetTracked(){
    qDebug()<<__FILE__<<" "<<__LINE__;
    //emit tx1StatusTrackedTargetChanged(200, 200, 303, 303);
}

void Tx1Manager::_sendMessageOnLink(const mavlink_message_t &message){
    LinkManager *linkManager = qgcApp()->toolbox()->linkManager ();
    if(0 && linkManager->links ()->contains (_link)){
        if(_link && _link->isConnected()){
            uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
            int len = mavlink_msg_to_send_buffer(buffer, &message);
            _link->writeBytesSafe((const char*)buffer, len);
        }
        return ;
    }

    for (int i = 0; i < linkManager->links()->count(); i++) {
        LinkInterface* links = linkManager->links()->value<LinkInterface*>(i);
        if(links && links->isConnected()){
            uint8_t buffer[MAVLINK_MAX_PACKET_LEN];
            int len = mavlink_msg_to_send_buffer(buffer, &message);
            links->writeBytesSafe((const char*)buffer, len);
        }
    }
}

void Tx1Manager::_handleTx1Message(LinkInterface *link,const mavlink_message_t message){
    _link = link;
    this->_handleTx1Message(message);
}
#endif

/// Called when a new mavlink message for out vehicle is received
void Tx1Manager::_handleTx1Message(const mavlink_message_t& message)
{
    switch (message.msgid) {

    case MAVLINK_MSG_ID_TX1_STATUS_SYSTEM:  // 65500
        _handleTx1StatusSystem(message);
        break;

    case MAVLINK_MSG_ID_TX1_STATUS_TRACKING_TARGET_LOCATION:    // 65001
        _handleTx1StatusTrackedTarget(message);
        break;

    case MAVLINK_MSG_ID_TX1_STATUS_POSSIBLE_TARGET_LOCATION:    // 65002
        _handleTx1StatusPossibleTrackedTarget(message);
        break;

    case MAVLINK_MSG_ID_TX1_DEBUG_STATUS_FP_TRACKED:            //65101
        _handleTx1StatusFpTracked(message);
        break;
    default:
        break;
    }
}

void Tx1Manager::_handleTx1StatusTrackedTarget(const mavlink_message_t& message){
    // MAVLINK_MSG_ID_TX1_STATUS_TRACKED_TARGET 65508
    mavlink_tx1_status_tracking_target_location_t tx1_status_tracked_target;
    mavlink_msg_tx1_status_tracking_target_location_decode(&message, &tx1_status_tracked_target);
//    qDebug()<<"recv tracked target : "/*<<tx1_status_tracked_target.rotation*/
//           <<" ,tl_x : "<<tx1_status_tracked_target.tl_x <<" ,tl_y : "<<tx1_status_tracked_target.tl_y
//          <<" ,rb_x : "<<tx1_status_tracked_target.br_x <<" ,rb_y : "<<tx1_status_tracked_target.br_y
//            <<" ,status : "<<tx1_status_tracked_target.c_x <<" ,num : "<<tx1_status_tracked_target.c_y;
    emit tx1StatusTrackedTargetChanged( tx1_status_tracked_target.tl_x
                                        , tx1_status_tracked_target.tl_y
                                        , tx1_status_tracked_target.br_x
                                        , tx1_status_tracked_target.br_y
                                        , tx1_status_tracked_target.c_x
                                        , tx1_status_tracked_target.c_y);
}

void Tx1Manager::_handleTx1StatusSystem(const mavlink_message_t& message){
    mavlink_tx1_status_system_t tx1_status_system;
    mavlink_msg_tx1_status_system_decode(&message,&tx1_status_system);
    qDebug()<<"tx1 current status:"<<tx1_status_system.tracking;
}

void Tx1Manager::_handleTx1StatusFpTracked(const mavlink_message_t &message)
{
    mavlink_tx1_debug_status_fp_tracked_t tx1_status_fp_tracked;
    mavlink_msg_tx1_debug_status_fp_tracked_decode(&message,&tx1_status_fp_tracked);

//    qDebug()<<"error:"<<tx1_status_fp_tracked.error
//           <<"orientation:"<<tx1_status_fp_tracked.orientation
//           <<"scale:"<<tx1_status_fp_tracked.scale
//           <<"strength"<<tx1_status_fp_tracked.strength
//           <<"tracking_status:"<<tx1_status_fp_tracked.tracking_status
//           <<"x:"<<tx1_status_fp_tracked.x<<"y"<<tx1_status_fp_tracked.y;
   // qDebug()<<"SSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSSs==========="<<tx1_status_fp_tracked.fpset_serial;
    emit tx1StatusFeaturePointChanged(tx1_status_fp_tracked.x,tx1_status_fp_tracked.y,tx1_status_fp_tracked.fpset_serial);
}

/// Message to control the streaming flow of a pipeline
///     @param target_id : the id of the target pipe channel
void Tx1Manager::tx1ControlPipeFlow(uint8_t target_id){
//#ifndef QGC_TX1_TEST_UDP
//    if(_vehicle && _mavlink){
//        mavlink_message_t msg;
//        mavlink_tx1_control_pipe_flow_t tx1_control_pipe_flow;
//        tx1_control_pipe_flow.target_system = _vehicle->id ();                    /*< System ID*/
//        tx1_control_pipe_flow.target_component = _vehicle->defaultComponentId (); // Component ID
//        tx1_control_pipe_flow.target_id = target_id;                              // The id of the target pipeline

//        _dedicatedLink = _vehicle->priorityLink();
//        if(_dedicatedLink){
//            // MAVLINK_MSG_ID_TX1_CONTROL_PIPE_FLOW 65500
//            mavlink_msg_tx1_control_pipe_flow_encode_chan(_mavlink->getSystemId(),     // system_id ID of this system
//                                                          _mavlink->getComponentId(),  // component_id ID of this component (e.g. 200 for IMU)
//                                                          _dedicatedLink->mavlinkChannel(),
//                                                          &msg,
//                                                          &tx1_control_pipe_flow);
//            _vehicle->sendMessageOnLink (_dedicatedLink, msg);
//        }
//    }
//#else
//    if(_mavlink){
//        mavlink_message_t msg;
//        mavlink_tx1_control_pipe_flow_t tx1_control_pipe_flow;
//        tx1_control_pipe_flow.target_system = _id;                           /*< System ID*/
//        tx1_control_pipe_flow.target_component = _mavlink->getComponentId(); // Component ID
//        tx1_control_pipe_flow.target_id = target_id;                         // The id of the target pipeline

//        // MAVLINK_MSG_ID_TX1_CONTROL_PIPE_FLOW 65500
//        // channel : 0
//        mavlink_msg_tx1_control_pipe_flow_encode(_mavlink->getSystemId(),     // system_id ID of this system
//                                                 _mavlink->getComponentId(),  // component_id ID of this component (e.g. 200 for IMU)
//                                                 &msg,
//                                                 &tx1_control_pipe_flow);
//        this->_sendMessageOnLink(msg);
//    }
//#endif
}

void Tx1Manager::tx1ControlPipeFp(uint8_t target_id, uint8_t send){
    Q_UNUSED(target_id)
#ifndef QGC_TX1_TEST_UDP
    if(_vehicle && _mavlink){
        mavlink_message_t msg;
        mavlink_tx1_debug_request_fp_tracked_t tx1_control_pipe_fp;
        tx1_control_pipe_fp.target_system = _vehicle->id ();
        tx1_control_pipe_fp.target_component = _vehicle->defaultComponentId ();       // The id of the target pipeline
        tx1_control_pipe_fp.send = send;                                              // Start/Stop send on the pipeline
//        memset(tx1_control_pipe_record.fileName, 0, sizeof(tx1_control_pipe_record.fileName));  ?

        _dedicatedLink = _vehicle->priorityLink();
        if(_dedicatedLink){
            // MAVLINK_MSG_ID_TX1_CONTROL_PIPE_RECORD 65501
            mavlink_msg_tx1_debug_request_fp_tracked_encode_chan(_mavlink->getSystemId(),
                                                                 _mavlink->getComponentId(),
                                                                 _dedicatedLink->mavlinkChannel(),
                                                                 &msg,
                                                                 &tx1_control_pipe_record);
            _vehicle->sendMessageOnLink (_dedicatedLink, msg);
        }
    }
#else
    if(_mavlink){
        mavlink_message_t msg;
        mavlink_tx1_debug_request_fp_tracked_t tx1_control_pipe_fp;
        tx1_control_pipe_fp.target_system = _id;
        tx1_control_pipe_fp.target_component = _mavlink->getComponentId();
        tx1_control_pipe_fp.send = send;
 //       memset(tx1_control_pipe_record.fileName, 0, sizeof(tx1_control_pipe_record.fileName));
        mavlink_msg_tx1_debug_request_fp_tracked_encode(_mavlink->getSystemId(),
                                                        _mavlink->getComponentId(),
                                                        &msg,
                                                        &tx1_control_pipe_fp);
        this->_sendMessageOnLink(msg);
    }
#endif
}

/// Message to control the screenshot function of a pipeline
///     @param target_id : the id of the target pipe channel
///     @param screenee  : take screene or stop screene series (if that is implemented)
///     @param fileName  : optional filename for the video recording
void Tx1Manager::tx1ControlPipeScreenee(uint8_t target_id, uint8_t screenee, char *fileName){
//#ifndef QGC_TX1_TEST_UDP
//    if(_vehicle && _mavlink){
//        mavlink_message_t msg;
//        mavlink_tx1_control_pipe_screenee_t tx1_control_pipe_screenee;
//        tx1_control_pipe_screenee.target_system = _vehicle->id ();                    /*< System ID*/
//        tx1_control_pipe_screenee.target_component = _vehicle->defaultComponentId (); // Component ID
//        tx1_control_pipe_screenee.target_id = target_id;                              // The id of the target pipeline
//        tx1_control_pipe_screenee.screenee = screenee;                                //  Take screene or stop screene series (if that is implemented)
//        memset(tx1_control_pipe_screenee.fileName, 0, sizeof(tx1_control_pipe_screenee.fileName));

//        _dedicatedLink = _vehicle->priorityLink();
//        if(_dedicatedLink){
//            // MAVLINK_MSG_ID_TX1_CONTROL_PIPE_SCREENEE 65502
//            mavlink_msg_tx1_control_pipe_screenee_encode_chan(_mavlink->getSystemId(),     // system_id ID of this system
//                                                              _mavlink->getComponentId(),  // component_id ID of this component (e.g. 200 for IMU)
//                                                              _dedicatedLink->mavlinkChannel(),
//                                                              &msg,
//                                                              &tx1_control_pipe_screenee);
//            _vehicle->sendMessageOnLink (_dedicatedLink, msg);
//        }
//    }
//#else
//    if(_mavlink){
//        mavlink_message_t msg;
//        mavlink_tx1_control_pipe_screenee_t tx1_control_pipe_screenee;
//        tx1_control_pipe_screenee.target_system = _id;                             /*< System ID*/
//        tx1_control_pipe_screenee.target_component = _mavlink->getComponentId();  // Component ID
//        tx1_control_pipe_screenee.target_id = target_id;                          // The id of the target pipeline
//        tx1_control_pipe_screenee.screenee = screenee;                            //  Take screene or stop screene series (if that is implemented)
//        memset(tx1_control_pipe_screenee.fileName, 0, sizeof(tx1_control_pipe_screenee.fileName));

//        // MAVLINK_MSG_ID_TX1_CONTROL_PIPE_SCREENEE 65502
//        mavlink_msg_tx1_control_pipe_screenee_encode(_mavlink->getSystemId(),     // system_id ID of this system
//                                                     _mavlink->getComponentId(),  // component_id ID of this component (e.g. 200 for IMU)
//                                                     &msg,
//                                                     &tx1_control_pipe_screenee);
//        this->_sendMessageOnLink(msg);
//    }
//#endif
}

/// Message to control the tracking system of a pieline
///     @param target_id : tThe id of the target pipeline
///     @param track     : true  : start tracking,
///                        false : stop tracking
///     @param topLeftX  : top left corner x coordinate of target to track
///     @param topLeftY  : top left corner y coordinate of target to track
///     @param bottomRightX  : bottom right corner x coordinate of target to track
///     @param bottomRightY  : bottom right corner y coordinate of target to track
void Tx1Manager::tx1ControlPipeTrack(uint8_t target_id, uint8_t track,int topLeftX, int topLeftY, int bottomRightX, int bottomRightY){
    Q_UNUSED(target_id);
    Q_UNUSED(track);
#ifndef QGC_TX1_TEST_UDP
    if(_vehicle && _mavlink){
        mavlink_message_t msg;
        mavlink_tx1_control_tracking_start_t tx1_control_pipe_track;
        tx1_control_pipe_track.target_system = _vehicle->id ();                    /*< System ID*/
        tx1_control_pipe_track.target_component = _vehicle->defaultComponentId (); // Component ID
        tx1_control_pipe_track.tl_x = topLeftX;      // Top left corner x coordinate of target to track
        tx1_control_pipe_track.tl_y = topLeftY;      // Top left corner y coordinate of target to track
        tx1_control_pipe_track.br_x = bottomRightX;  // Bottom right corner x coordinate of target to track
        tx1_control_pipe_track.br_y = bottomRightY;  // Bottom right corner y coordinate of target to track

        _dedicatedLink = _vehicle->priorityLink();
        if(_dedicatedLink){
            // MAVLINK_MSG_ID_TX1_CONTROL_PIPE_TRACK 65503
            mavlink_msg_tx1_control_tracking_start_pack_chan(_mavlink->getSystemId(),     // system_id ID of this system
                                                           _mavlink->getComponentId(),  // component_id ID of this component (e.g. 200 for IMU)
                                                           _dedicatedLink->mavlinkChannel(),
                                                           &msg,
                                                           tx1_control_pipe_track.target_system,
                                                           tx1_control_pipe_track.target_component,
                                                           tx1_control_pipe_track.tl_x
                                                           tx1_control_pipe_track.tl_y
                                                           tx1_control_pipe_track.br_x
                                                           tx1_control_pipe_track.br_y);
            _vehicle->sendMessageOnLink (_dedicatedLink, msg);
        }
    }
#else
    if(_mavlink){
        mavlink_message_t msg;
        mavlink_tx1_control_tracking_start_t tx1_control_pipe_track;
        tx1_control_pipe_track.target_system = _id;                           /*< System ID*/
        tx1_control_pipe_track.target_component = _mavlink->getComponentId(); // Component ID
        tx1_control_pipe_track.tl_x = topLeftX;      // Top left corner x coordinate of target to track
        tx1_control_pipe_track.tl_y = topLeftY;      // Top left corner y coordinate of target to track
        tx1_control_pipe_track.br_x = bottomRightX;  // Bottom right corner x coordinate of target to track
        tx1_control_pipe_track.br_y = bottomRightY;  // Bottom right corner y coordinate of target to track

        // MAVLINK_MSG_ID_TX1_CONTROL_PIPE_TRACK 65503
        mavlink_msg_tx1_control_tracking_start_encode(_mavlink->getSystemId(),     // system_id ID of this system
                                                  _mavlink->getComponentId(),  // component_id ID of this component (e.g. 200 for IMU)
                                                  &msg,
                                                  &tx1_control_pipe_track);
        this->_sendMessageOnLink(msg);
    }
#endif
}

/// To control the activation of a com channel
///     @param target_id : the id of the target com channel
void Tx1Manager::tx1ControlComFlow(uint8_t target_id){
//#ifndef QGC_TX1_TEST_UDP
//    if(_vehicle && _mavlink){
//        mavlink_message_t msg;
//        mavlink_tx1_control_com_flow_t tx1_control_com_flow;
//        tx1_control_com_flow.target_system = _vehicle->id ();                    /*< System ID*/
//        tx1_control_com_flow.target_component = _vehicle->defaultComponentId (); // Component ID
//        tx1_control_com_flow.target_id = target_id;                              // The id of the target pipeline

//        _dedicatedLink = _vehicle->priorityLink();
//        if(_dedicatedLink){
//            // MAVLINK_MSG_ID_TX1_CONTROL_COM_FLOW 65504
//            mavlink_msg_tx1_control_com_flow_encode_chan(_mavlink->getSystemId(),     // system_id ID of this system
//                                                         _mavlink->getComponentId(),  // component_id ID of this component (e.g. 200 for IMU)
//                                                         _dedicatedLink->mavlinkChannel(),
//                                                         &msg,
//                                                         &tx1_control_com_flow);
//            _vehicle->sendMessageOnLink (_dedicatedLink, msg);
//        }
//    }
//#else
//    if(_mavlink){
//        mavlink_message_t msg;
//        mavlink_tx1_control_com_flow_t tx1_control_com_flow;
//        tx1_control_com_flow.target_system = _id;                           /*< System ID*/
//        tx1_control_com_flow.target_component = _mavlink->getComponentId(); // Component ID
//        tx1_control_com_flow.target_id = target_id;                         // The id of the target pipeline
//        // MAVLINK_MSG_ID_TX1_CONTROL_COM_FLOW 65504
//        mavlink_msg_tx1_control_com_flow_encode(_mavlink->getSystemId(),     // system_id ID of this system
//                                                _mavlink->getComponentId(),  // component_id ID of this component (e.g. 200 for IMU)
//                                                &msg,
//                                                &tx1_control_com_flow);
//        this->_sendMessageOnLink(msg);
//    }
//#endif
}

/// Message to setup or change a com channel
///     @param target_id : the id of the target com channele
void Tx1Manager::tx1ControlSetupCom(uint8_t target_id){
//#ifndef QGC_TX1_TEST_UDP
//    if(_vehicle && _mavlink){
//        mavlink_message_t msg;
//        mavlink_tx1_control_setup_com_t tx1_control_setup_com;
//        tx1_control_setup_com.target_system = _vehicle->id ();                    /*< System ID*/
//        tx1_control_setup_com.target_component = _vehicle->defaultComponentId (); // Component ID
//        tx1_control_setup_com.target_id = target_id;                              // The id of the target pipeline

//        _dedicatedLink = _vehicle->priorityLink();
//        if(_dedicatedLink){
//            // MAVLINK_MSG_ID_TX1_CONTROL_SETUP_COM 65505
//            mavlink_msg_tx1_control_setup_com_encode_chan(_mavlink->getSystemId(),     // system_id ID of this system
//                                                          _mavlink->getComponentId(),  // component_id ID of this component (e.g. 200 for IMU)
//                                                          _dedicatedLink->mavlinkChannel(),
//                                                          &msg,
//                                                          &tx1_control_setup_com);
//            _vehicle->sendMessageOnLink (_dedicatedLink, msg);
//        }
//    }
//#else
//    if(_mavlink){
//        mavlink_message_t msg;
//        mavlink_tx1_control_setup_com_t tx1_control_setup_com;
//        tx1_control_setup_com.target_system = _id;                           /*< System ID*/
//        tx1_control_setup_com.target_component = _mavlink->getComponentId(); // Component ID
//        tx1_control_setup_com.target_id = target_id;                         // The id of the target pipeline

//        // MAVLINK_MSG_ID_TX1_CONTROL_SETUP_COM 65505
//        mavlink_msg_tx1_control_setup_com_encode(_mavlink->getSystemId(),     // system_id ID of this system
//                                                 _mavlink->getComponentId(),  // component_id ID of this component (e.g. 200 for IMU)
//                                                 &msg,
//                                                 &tx1_control_setup_com);
//        this->_sendMessageOnLink(msg);
//    }
//#endif
}

/// Message to setup or change a pipeline
///     @param target_id : the id of the target pipeline
void Tx1Manager::tx1ControlSetupPipe(uint8_t target_id){
//#ifndef QGC_TX1_TEST_UDP
//    if(_vehicle && _mavlink){
//        mavlink_message_t msg;
//        mavlink_tx1_control_setup_pipe_t tx1_control_setup_pipe;
//        tx1_control_setup_pipe.target_system = _vehicle->id ();                    /*< System ID*/
//        tx1_control_setup_pipe.target_component = _vehicle->defaultComponentId (); // Component ID
//        tx1_control_setup_pipe.target_id = target_id;                              // The id of the target pipeline

//        _dedicatedLink = _vehicle->priorityLink();
//        if(_dedicatedLink){
//            // MAVLINK_MSG_ID_TX1_CONTROL_SETUP_PIPE 65506
//            mavlink_msg_tx1_control_setup_pipe_encode_chan(_mavlink->getSystemId(),     // system_id ID of this system
//                                                           _mavlink->getComponentId(),  // component_id ID of this component (e.g. 200 for IMU)
//                                                           _dedicatedLink->mavlinkChannel(),
//                                                           &msg,
//                                                           &tx1_control_setup_pipe);
//            _vehicle->sendMessageOnLink (_dedicatedLink, msg);
//        }
//    }
//#else
//    if(_mavlink){
//        mavlink_message_t msg;
//        mavlink_tx1_control_setup_pipe_t tx1_control_setup_pipe;
//        tx1_control_setup_pipe.target_system = _id;                           /*< System ID*/
//        tx1_control_setup_pipe.target_component = _mavlink->getComponentId(); // Component ID
//        tx1_control_setup_pipe.target_id = target_id;                         // The id of the target pipeline

//        // MAVLINK_MSG_ID_TX1_CONTROL_SETUP_PIPE 65506
//        mavlink_msg_tx1_control_setup_pipe_encode(_mavlink->getSystemId(),     // system_id ID of this system
//                                                  _mavlink->getComponentId(),  // component_id ID of this component (e.g. 200 for IMU)
//                                                  &msg,
//                                                  &tx1_control_setup_pipe);
//        this->_sendMessageOnLink(msg);
//    }
//#endif
}

/// Request the status of a target (com channel or pipeline)
///     @param target_type : type of the target for that the status message is requested
///     @param target_id   : the id of the target pipeline or com channel
void Tx1Manager::tx1RequestStatus(uint8_t target_type, uint8_t target_id){
//#ifndef QGC_TX1_TEST_UDP
//    if(_vehicle && _mavlink){
//        //TX1_COM_CHAN_TYPE type;
//        mavlink_message_t msg;
//        mavlink_tx1_request_status_t tx1_request_status;
//        tx1_request_status.target_system = _vehicle->id ();                    /*< System ID*/
//        tx1_request_status.target_component = _vehicle->defaultComponentId (); // Component ID
//        tx1_request_status.target_type = target_type;  // Type of the target for that the status message is requested
//        tx1_request_status.target_id = target_id;      // The id of the target pipeline

//        _dedicatedLink = _vehicle->priorityLink();
//        if(_dedicatedLink){
//            // MAVLINK_MSG_ID_TX1_REQUEST_STATUS 65507
//            mavlink_msg_tx1_request_status_encode_chan(_mavlink->getSystemId(),     // system_id ID of this system
//                                                       _mavlink->getComponentId(),  // component_id ID of this component (e.g. 200 for IMU)
//                                                       _dedicatedLink->mavlinkChannel(),
//                                                       &msg,
//                                                       &tx1_request_status);
//            _vehicle->sendMessageOnLink (_dedicatedLink, msg);
//        }
//    }
//#else
//    if(_mavlink){
//        //TX1_COM_CHAN_TYPE type;
//        mavlink_message_t msg;
//        mavlink_tx1_request_status_t tx1_request_status;
//        tx1_request_status.target_system = _id;                    /*< System ID*/
//        tx1_request_status.target_component = _mavlink->getComponentId (); // Component ID
//        tx1_request_status.target_type = target_type;  // Type of the target for that the status message is requested
//        tx1_request_status.target_id = target_id;      // The id of the target pipeline

//        // MAVLINK_MSG_ID_TX1_REQUEST_STATUS 65507
//        mavlink_msg_tx1_request_status_encode(_mavlink->getSystemId(),     // system_id ID of this system
//                                              _mavlink->getComponentId(),  // component_id ID of this component (e.g. 200 for IMU)
//                                              &msg,
//                                              &tx1_request_status);
//        this->_sendMessageOnLink(msg);
//    }
//#endif
}

/// A message acknowledging a received message
///     @param status : enum TX1_STATUS_ACK : ACK status of the command this message is acknowledgeing
void Tx1Manager::tx1StatusAck(uint8_t status){
    Q_UNUSED(status);
}
/// Message to stop tracking
///     @param system_id ID of this system
///     @param component_id ID of this component (e.g. 200 for IMU)
///     @param msg The MAVLink message to compress the data into
///     @param tx1_control_tracking_stop C-struct to read the message contents from
void Tx1Manager::tx1StopTracking(uint8_t target_id)
{
    Q_UNUSED(target_id);
#ifndef QGC_TX1_TEST_UDP
    if(_vehicle && _mavlink){
        mavlink_message_t msg;
        mavlink_tx1_control_tracking_stop_t tx1_control_track;
        tx1_control_track.target_system = _vehicle->id();                           /*< System ID*/
        tx1_control_track.target_component = _vehicle->defaultComponentId();

        _dedicatedLink = _vehicle->priorityLink();

        if(_dedicatedLink){
            mavlink_msg_tx1_control_tracking_stop_encode_chan(_mavlink->getSystemId(),
                                                              _mavlink->getComponentId(),
                                                              _dedicatedLink,
                                                              &msg,
                                                              &tx1_control_track
                        );
            _vehicle->sendMessageOnLink(_vehicle,msg);
    }
#else
    if(_mavlink){
        mavlink_message_t msg;
        mavlink_tx1_control_tracking_stop_t tx1_control_track;
        tx1_control_track.target_system = _id;                           /*< System ID*/
        tx1_control_track.target_component = _mavlink->getComponentId();
        mavlink_msg_tx1_control_tracking_stop_encode(_mavlink->getSystemId(),
                                                     _mavlink->getComponentId(),
                                                     &msg,
                                                     &tx1_control_track
                    );
        this->_sendMessageOnLink(msg);
    }
#endif
}

//// !!! dummy messages - not used right now. just fror blocking the ids !!!
void Tx1Manager::tx1Dummy1(){

}
void Tx1Manager::tx1Dummy2(){

}
void Tx1Manager::tx1Dummy3(){

}
void Tx1Manager::tx1Dummy4(){

}
void Tx1Manager::tx1Dummy5(){

}
void Tx1Manager::_handleTx1StatusPossibleTrackedTarget(const mavlink_message_t& message){
    mavlink_tx1_status_possible_target_location_t tx1_status_tracked_target;
    mavlink_msg_tx1_status_possible_target_location_decode(&message, &tx1_status_tracked_target);
    emit tx1StatusTrackedPossibleTargetChanged( tx1_status_tracked_target.tl_x
                                        , tx1_status_tracked_target.tl_y
                                        , tx1_status_tracked_target.br_x
                                        , tx1_status_tracked_target.br_y
                                        );
}

//// !!! dummy messages - not used right now. just fror blocking the ids !!!
