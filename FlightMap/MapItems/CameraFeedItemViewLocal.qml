/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.4
import QtQuick.Controls         1.3
import QtQuick.Dialogs          1.2
import QtLocation               5.3
import QtPositioning            5.3
import QtQuick.Controls.Styles  1.2

import QGroundControl               1.0


/// The MissionItemView control is used to add Mission Item Indicators to a FlightMap.
MapItemView {
    id: rootid

    delegate: MapQuickItem{
        id:             itemIndicator
        coordinate:     object.coordinate
        visible:        true
        z:              QGroundControl.zOrderMapItems
        anchorPoint.x:  sourceItem.width  / 2
        anchorPoint.y:  sourceItem.height / 2

        //anchorPoint.x: image.width/4
        //anchorPoint.y: image.height

        sourceItem: Rectangle {
            property real   imageSize:10
            width:          imageSize    //ScreenTools.defaultFontPixelHeight * 1.75
            height:         imageSize    //ScreenTools.defaultFontPixelHeight * 1.75
            radius:         imageSize / 2//ScreenTools.defaultFontPixelHeight * 1.75 / 2
            color:          "red"        //Qt.rgba(0,0,0,0.75)
        }
//        sourceItem:
//            Rectangle {
//            property real   imageSize:20
//            width:          imageSize    //ScreenTools.defaultFontPixelHeight * 1.75
//            height:         imageSize    //ScreenTools.defaultFontPixelHeight * 1.75
//            radius:         imageSize / 2//ScreenTools.defaultFontPixelHeight * 1.75 / 2
//            color:          "red"        //Qt.rgba(0,0,0,0.75)
//            Rectangle{
//                anchors.centerIn: parent
//                width:            parent.imageSize / 2
//                height:           parent.imageSize / 2
//                radius:           parent.imageSize / 8
//                color:            "red"
//                border.width:     parent.imageSize / 8
//                border.color:     "#fdfafa"
//            }
//        }
    }
}
