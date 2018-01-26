/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


#include "TX1.h"
#include "MAVLinkProtocol.h"
#include "QGCApplication.h"
#include "Vehicle.h"

//QGC_LOGGING_CATEGORY(TX1Log, "TX1Log")


TX1::TX1(Vehicle* vehicle)
    : QObject(NULL)
    , _vehicle(vehicle)
    , _dedicatedLink(NULL)
    , _systemIdQGC(0)
{
    if(_vehicle){
        qDebug()<<"Initialization TX1 , TX1 start working success";
    }
}

TX1::~TX1(){
}

void TX1::_errorMessage(QString errMsg){
    qWarning()<<__FILE__<<" : "<<__LINE__<<" : errMsg : "<<errMsg;
}

void TX1::receiveMessage(mavlink_message_t message){
    // receiveMessage is signalled will all mavlink messages so we need to filter everything else out but ours.
}

void TX1::_sendMessage(const mavlink_message_t &message){
    if(!_vehicle || !_vehicle->active ()){
        _errorMessage("Vehicle is null or Vehicle is not active.");
        return ;
    }

    _dedicatedLink = _vehicle->priorityLink();
    if (!_dedicatedLink) {
        _errorMessage("Command not sent. No Vehicle links.");
        return ;
    }
    _vehicle->sendMessageOnLink(_dedicatedLink, message);
}
