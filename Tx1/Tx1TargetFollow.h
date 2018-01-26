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

#ifndef TX1_TATGETFOLLOW_H
#define TX1_TATGETFOLLOW_H

#include "MAVLinkProtocol.h"
#include "Tx1.h"

#include <QObject>
#include <QGeoCoordinate>
#include <QElapsedTimer>

class Vehicle;

//Q_DECLARE_LOGGING_CATEGORY(TX1TargetFollowLog)

class Tx1TargetFollow : public TX1
{
    Q_OBJECT

public:
    Tx1TargetFollow(Vehicle* vehicle = NULL);
    ~Tx1TargetFollow();

public slots:
    void receiveMessage(mavlink_message_t message);

private:
    void _sendMessage(uint32_t seq,int64_t time_usec = 0x0);
};
#endif    //TX1_TATGETFOLLOW_H
