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

#ifndef TX1_TAKEPICTURE_H
#define TX1_TAKEPICTURE_H

#include "MAVLinkProtocol.h"
#include "Tx1.h"

#include <QObject>
#include <QGeoCoordinate>
#include <QElapsedTimer>

class Vehicle;

//Q_DECLARE_LOGGING_CATEGORY(Tx1TakePictureLog)

class Tx1TakePicture : public TX1
{
    Q_OBJECT

public:
    Tx1TakePicture(Vehicle* vehicle = NULL);
    ~Tx1TakePicture();

public slots:
    void receiveMessage(mavlink_message_t message);

private:
    void _sendMessage(QString target, QString fileName, uint8_t screenee);
};
#endif    //TX1_TAKEPICTURE_H
