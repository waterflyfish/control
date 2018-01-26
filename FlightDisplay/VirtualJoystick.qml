/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick 2.5

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0
Rectangle {
    //property bool useLightColors - Must be passed in from loaded
    color : "transparent"

    Timer {
        interval:   40  // 25Hz, same as real joystick rate
        running:    QGroundControl.virtualTabletJoystick && _activeVehicle && _activeVehicle.isPosCtlMode && !_activeVehicle.keyCtlAtitude
        repeat:     true
        onTriggered: {
            if (_activeVehicle) {
                _activeVehicle.virtualTabletJoystickValue(rightStick.xAxis, leftStick.yAxis,  leftStick.xAxis, rightStick.yAxis)
//                console.log("rightStick.yAxis:"+ rightStick.yAxis + "leftStick.yAxis:" + leftStick.yAxis)
            }
        }
    }

    JoystickThumbPad {
        id:                     rightStick
        anchors.rightMargin:    -xPositionDelta//xPositionDelta
        anchors.bottomMargin:   -yPositionDelta
        anchors.right:          parent.right//parent.left
        anchors.bottom:         parent.bottom
        width:                  parent.height * 1.2
        height:                 parent.height * 1.2
        yAxisThrottle:          true
        lightColors:            useLightColors
    }

    JoystickThumbPad {
        id:                     leftStick
        anchors.leftMargin:     xPositionDelta
        anchors.bottomMargin:   -yPositionDelta
        anchors.left:           parent.left
        anchors.bottom:         parent.bottom
        width:                  parent.height * 1.2
        height:                 parent.height * 1.2
        lightColors:            useLightColors
    }
}
