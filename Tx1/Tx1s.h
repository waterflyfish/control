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

#ifndef TX1_H
#define TX1_H

#include <QObject>
#include <QGeoCoordinate>
#include <QElapsedTimer>
#include "MAVLinkProtocol.h"

class Vehicle;

//Q_DECLARE_LOGGING_CATEGORY(TX1Log)

class TX1 : public QObject
{
    Q_OBJECT

public:
    TX1(Vehicle* vehicle = NULL);
    ~TX1();

public slots:
    virtual void receiveMessage(mavlink_message_t message) = 0;

protected:
    void _errorMessage(QString errMsg);
    void  _sendMessage(const mavlink_message_t &message);

    Vehicle*        _vehicle;
    LinkInterface*  _dedicatedLink;             ///< Link to use for communication
    uint8_t         _systemIdQGC;               ///< System ID for QGC
    uint32_t        _seq;                       ///< System Camera Message
};
#endif    //TX1_H
