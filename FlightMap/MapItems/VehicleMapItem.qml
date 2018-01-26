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

import QtQuick          2.4
import QtLocation       5.3
import QtPositioning    5.3

import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0

/// Marker for displaying a vehicle location on the map
MapQuickItem {
    property var    vehicle                ///< Vehicle object
    property bool   isSatellite:    false  ///< true: satellite map is showing
    property real   size:           ScreenTools.defaultFontPixelHeight * 5
    property int vehicletype
    property var imagesource
    function imageSource(){
          console.log("vehicletype is "+   vehicletype);
        if(vehicle && isSatellite ){
            switch(vehicletype){
            case 1:
                return ("qrc:/qmlimages/Airframe/Plane")
            case 2:
                return ("qrc:/qmlimages/Airframe/VTOLDuoRotorTailSitter1")
            case 3:
                return ("qrc:/qmlimages/Airframe/VTOLDuoRotorTailSitter")
            case 4:
                return ("qrc:/qmlimages/Airframe/QuadRotorX")
            case 5:
                return ("qrc:/qmlimages/Airframe/HexaRotorX")
            case 6:
                return ("qrc:/qmlimages/Airframe/OctoRotorX")
            default :
                return ("qrc:/qmlimages/airplaneOpaque.svg")
            }
        }else{
            return ("qrc:/qmlimages/airplaneOutline.svg")
        }
    }

    anchorPoint.x:  vehicleIcon.width  / 2
    anchorPoint.y:  vehicleIcon.height / 2
    visible:        vehicle && vehicle.coordinateValid

    sourceItem: Image {
        id:                 vehicleIcon
        source:             imageSource()
        mipmap:             true
        width:              size
        sourceSize.width:   size
        fillMode:           Image.PreserveAspectFit
        transform: Rotation {
            origin.x:       vehicleIcon.width  / 2
            origin.y:       vehicleIcon.height / 2
            angle:          vehicle ? vehicle.heading.value : 0
        }
    }
}
