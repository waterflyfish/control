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
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2
import QtLocation               5.3
import QtPositioning            5.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.Controllers   1.0

Item {
    id: _root

    property var    _activeVehicle:             QGroundControl.multiVehicleManager.activeVehicle
    property bool   _isSatellite:               !_mainIsMap //? (_flightMap ? !_flightMap.isSatelliteMap : true) : true
    property bool   _lightWidgetBorders:        _isSatellite

    readonly property real _toolButtonTopMargin:    parent.height - ScreenTools.availableHeight + (ScreenTools.defaultFontPixelHeight / 2)

    QGCMapPalette { id: mapPal; lightColors: isBackgroundDark }

    ExclusiveGroup {
        id: _dropButtonsExclusiveGroup
    }

    ExclusiveGroup {
        id: _mapTypeButtonsExclusiveGroup
    }

    QGCLabel {
        id:         flyLabel
        text:       qsTr("Video")
        color:      mapPal.text
        visible:    !ScreenTools.isShortScreen && !_mainIsMap
        anchors.topMargin:          _toolButtonTopMargin
        anchors.horizontalCenter:   toolColumn.horizontalCenter
        anchors.top:                parent.top
    }

    //-- Vertical Tool Buttons
    Column {
        id:                 toolColumn
        anchors.topMargin:  ScreenTools.isShortScreen ? _toolButtonTopMargin : ScreenTools.defaultFontPixelHeight / 2
        anchors.leftMargin: ScreenTools.defaultFontPixelHeight
        anchors.left:       parent.left
        anchors.top:        ScreenTools.isShortScreen ? parent.top : flyLabel.bottom
        spacing:            ScreenTools.defaultFontPixelHeight
        visible:            !_mainIsMap

        //-- Map Center Control
        DropButton {
            id:                 centerMapDropButton
            dropDirection:      dropRight
            buttonImage:        "/qmlimages/MapCenter.svg"
            viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
            exclusiveGroup:     _dropButtonsExclusiveGroup
            z:                  QGroundControl.zOrderWidgets
            lightBorders:       _lightWidgetBorders
            visible:            false//!_mainIsMap

            dropDownComponent: Component {
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCCheckBox {
                        id:         followVehicleCheckBox
                        text:       qsTr("Follow Vehicle")
                        checked:    _flightMap ? _flightMap._followVehicle : false
                        anchors.verticalCenter: parent.verticalCenter
                        //anchors.baseline:   centerMapButton.baseline - This doesn't work correctly on mobile for some strange reason, so we center instead

                        onClicked: {
                            _dropButtonsExclusiveGroup.current = null
                            _flightMap._followVehicle = !_flightMap._followVehicle
                        }
                    }

                    QGCButton {
                        id:         centerMapButton
                        text:       qsTr("Center map on Vehicle")
                        enabled:    _activeVehicle && !followVehicleCheckBox.checked

                        property var activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

                        onClicked: {
                            _dropButtonsExclusiveGroup.current = null
                            _flightMap.center = activeVehicle.coordinate
                        }
                    }
                    QGCButton {
                        id:         find
                        text:       qsTr("Find the waypoint")
                        //enabled:    _activeVehicle && !followVehicleCheckBox.checked

                        property var activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

                        onClicked: {
                            if((_missionController.visualItems != null) && (_missionController.visualItems.count >= 2)){
                                _flightMap.center =  _missionController.visualItems.get(1).coordinate
                                _flightMap.zoomLevel=18
                            }
                            _dropButtonsExclusiveGroup.current = null
                        }
                    }
                }
            }
        }

        //-- Map Type Control
        DropButton {
            id:                 mapTypeButton
            dropDirection:      dropRight
            buttonImage:        "/qmlimages/MapType.svg"
            viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
            exclusiveGroup:     _dropButtonsExclusiveGroup
            z:                  QGroundControl.zOrderWidgets
            lightBorders:       _lightWidgetBorders
            visible:            false//!_mainIsMap

            dropDownComponent: Component {
                Column {
                    spacing: ScreenTools.defaultFontPixelWidth

                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth

                        Repeater {
                            model: QGroundControl.flightMapSettings.mapTypes

                            QGCButton {
                                checkable:      true
                                checked:        QGroundControl.flightMapSettings.mapType === text
                                text:           modelData
                                width:          clearButton.width
                                exclusiveGroup: _mapTypeButtonsExclusiveGroup

                                onClicked: {
                                    QGroundControl.flightMapSettings.mapType = text
                                    checked = true
                                    _dropButtonsExclusiveGroup.current = null
                                }
                            }
                        }
                    }

                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth
                        QGCButton {
                            id:         clearButton
                            text:       qsTr("Clear Flight Trails")
                            enabled:    QGroundControl.multiVehicleManager.activeVehicle
                            onClicked: {
                                QGroundControl.multiVehicleManager.activeVehicle.clearTrajectoryPoints()
                                _dropButtonsExclusiveGroup.current = null
                            }
                        }

                        QGCButton {
                            id:         showCameraButton
                            text:       qsTr("Hide Camera")
                            enabled:    QGroundControl.multiVehicleManager.activeVehicle
                            onClicked: {
                                _dropButtonsExclusiveGroup.current = null
                                if(text == qsTr("Show Camera")){
                                    text = qsTr("Hide Camera")
                                    _flightMap.missionController.hideCameraItems = false
                                }else{
                                    text = qsTr("Show Camera")
                                    _flightMap.missionController.hideCameraItems = true
                                }
                            }
                        }

                        QGCButton {
                            id:         showWayPointButton
                            text:       qsTr("Hide WayPoint")
                            enabled:    QGroundControl.multiVehicleManager.activeVehicle
                            onClicked: {
                                _dropButtonsExclusiveGroup.current = null
                                if(text == qsTr("Show WayPoint")){
                                    text = qsTr("Hide WayPoint")
                                    _flightMap.missionController.hideVisualItems = false
                                }else{
                                    text = qsTr("Show WayPoint")
                                    _flightMap.missionController.hideVisualItems = true
                                }
                            }
                        }
                    }

                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth
                        QGCButton {
                            id:         takePictreButton
                            text:       qsTr("Take Picture")
                            enabled:    QGroundControl.multiVehicleManager.activeVehicle
                            onClicked: {
                                QGroundControl.multiVehicleManager.activeVehicle.setDigitalCameraControl()
                                _dropButtonsExclusiveGroup.current = null
                            }
                        }
                    }
                }
            }
        }

        //-- Camera Zoom Control
        DropButton {
            id:                 centerCamZoomButton
            dropDirection:      dropRight
            buttonImage:        "/qmlimages/ZoomPluCam.svg"
            viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
            exclusiveGroup:     _dropButtonsExclusiveGroup
            z:                  QGroundControl.zOrderWidgets
            lightBorders:       _lightWidgetBorders

            dropDownComponent: Component {
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCButton {
                        id:         recordButton
                        text:       qsTr("Start Record")
                        enabled:    QGroundControl.multiVehicleManager.activeVehicle

                        property var activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

                        onClicked: {
                        }
                    }
                    QGCButton {
                        id:         camZoomOutButton
                        text:       qsTr("Zoom Out")
                        enabled:    QGroundControl.multiVehicleManager.activeVehicle

                        property var activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

                        onClicked: {
                            /// true : zoom In, false : zoom Out
                            QGroundControl.multiVehicleManager.activeVehicle.setDigitalCameraControlZoom(false)
                        }
                    }
                }
            }
        }

        //-- Zoom Map In
        RoundButton {
            id:                 mapZoomPlus
            visible:            false//!ScreenTools.isTinyScreen && !_mainIsMap
            buttonImage:        "/qmlimages/ZoomPlus.svg"
            exclusiveGroup:     _dropButtonsExclusiveGroup
            z:                  QGroundControl.zOrderWidgets
            lightBorders:       _lightWidgetBorders
            onClicked: {
                if(_flightMap)
                    _flightMap.zoomLevel += 0.5
                checked = false
            }
        }

        //-- Zoom Map Out
        RoundButton {
            id:                 mapZoomMinus
            visible:            false//!ScreenTools.isTinyScreen && !_mainIsMap
            buttonImage:        "/qmlimages/ZoomMinus.svg"
            exclusiveGroup:     _dropButtonsExclusiveGroup
            z:                  QGroundControl.zOrderWidgets
            lightBorders:       _lightWidgetBorders
            onClicked: {
                if(_flightMap)
                    _flightMap.zoomLevel -= 0.5
                checked = false
            }
        }
    }
}
