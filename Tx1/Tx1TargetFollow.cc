/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "Tx1TargetFollow.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"
#include "Vehicle.h"

//QGC_LOGGING_CATEGORY(Tx1TargetFollowLog, "Tx1TargetFollowLog")


Tx1TargetFollow::Tx1TargetFollow(Vehicle* vehicle)
    : TX1(vehicle)
{
    if(vehicle){
        qDebug()<<"Initialization Tx1TargetFollow , Tx1TargetFollow start working success";
    }
}

Tx1TargetFollow::~Tx1TargetFollow(){
}

void Tx1TargetFollow::receiveMessage(mavlink_message_t message){
    // receiveMessage is signalled will all mavlink messages so we need to filter everything else out but ours.
    if (message.msgid != MAVLINK_MSG_ID_DIGICAM_CONTROL) {
        _errorMessage("Vehicle is null or Vehicle is not active.");
        return;
    }

    mavlink_camera_trigger_t data;
    mavlink_msg_camera_trigger_decode(&message, &data);
    _seq = data.seq;
    qDebug()<<"seq : "<<data.seq<<" ,time_usec : "<<data.time_usec;
}

void Tx1TargetFollow::_sendMessage(uint32_t seq,int64_t time_usec){
    if(!_vehicle || !_vehicle->active ()){
        _errorMessage("Vehicle is null or Vehicle is not active.");
        return ;
    }

    _dedicatedLink = _vehicle->priorityLink();
    if (!_dedicatedLink) {
        _errorMessage("Command not sent. No Vehicle links.");
        return ;
    }

    if (_systemIdQGC == 0) {
        _systemIdQGC = qgcApp()->toolbox()->mavlinkProtocol()->getSystemId();
    }
    mavlink_message_t message;
    mavlink_msg_camera_trigger_pack(_systemIdQGC,       // QGC System ID
                                    0,                  // QGC Component ID
                                    &message,           // Mavlink Message to pack into
                                    time_usec,          // Timestamp for the image frame in microseconds, Init Zero for ever
                                    seq);               // Image frame sequence, Camera Control Command
    _vehicle->sendMessageOnLink(_dedicatedLink, message);
}
