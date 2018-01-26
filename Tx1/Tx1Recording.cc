/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "Tx1Recording.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"
#include "Vehicle.h"

//QGC_LOGGING_CATEGORY(Tx1RecordingLog, "Tx1RecordingLog")


Tx1Recording::Tx1Recording(Vehicle* vehicle)
    : TX1(vehicle)
{
    if(vehicle){
        qDebug()<<"Initialization Tx1Recording , Tx1Recording start working success";
    }
}

Tx1Recording::~Tx1Recording(){
}

void Tx1Recording::receiveMessage(mavlink_message_t message){
    // receiveMessage is signalled will all mavlink messages so we need to filter everything else out but ours.
    if (message.msgid != MAVLINK_MSG_ID_TX1_CONTROL_PIPE_RECORD) {
        _errorMessage("Tx1Recording receive mistake message id.");
        return;
    }
    mavlink_tx1_control_pipe_record_t tx1_control_pipe_record;
    mavlink_msg_tx1_control_pipe_record_decode(&message, &tx1_control_pipe_record);
}

void Tx1Recording::_sendMessage(QString target, QString fileName, uint8_t record){

    if(!_vehicle || !_vehicle->active ()){
        _errorMessage("Vehicle is null or Vehicle is not active.");
        return ;
    }

    if(target.isEmpty () || fileName.isEmpty ()){
        _errorMessage(QString("%1 is empty")
                      .arg ( target.isEmpty () == true ? "target" : "fileName"));
        return ;
    }

    if(target.length () >= 16 || fileName.length () >= 16){
        _errorMessage(QString("%1 maximum length %2")
                      .arg ( target.length () >= 16 ? "target" : "fileName").arg (16 -1));
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
    mavlink_msg_tx1_control_pipe_record_pack(_systemIdQGC,                       // QGC System ID
                                             0,                                  // QGC Component ID
                                             &message,                           // Mavlink Message to pack into
                                             target.toStdString ().c_str (),     // Tx1 record target
                                             record,                             // Tx1 record
                                             fileName.toStdString ().c_str ());  // Tx1 record fileName
    _vehicle->sendMessageOnLink(_dedicatedLink, message);
}
