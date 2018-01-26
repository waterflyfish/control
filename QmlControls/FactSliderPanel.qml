/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick              2.5
import QtQuick.Controls     1.4

import QGroundControl.FactSystem    1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactControls  1.0

Row{
    property ListModel sliderModel
    property var qgcViewPanel
    property real _margins:         ScreenTools.defaultFontPixelHeight
    property bool _loadComplete:    false
    property real _middleRowWidth: ScreenTools.isMobile ? ScreenTools.defaultFontPixelWidth * 11 : ScreenTools.defaultFontPixelWidth*12
    property real _middleWidth:     ScreenTools.isMobile ? ScreenTools.defaultFontPixelHeight*7 : ScreenTools.defaultFontPixelHeight*6
    property real _editFieldWidth: ScreenTools.isMobile ? ScreenTools.defaultFontPixelWidth * 12 : ScreenTools.defaultFontPixelWidth*15
    property real _titleWidth:  ScreenTools.isMobile ?  ScreenTools.defaultFontPixelWidth*32 : ScreenTools.defaultFontPixelWidth*38
    spacing:   _margins

    QGCPalette { id: palette; colorGroupEnabled: enabled }

    Rectangle  {
        width:_editFieldWidth*2 + _middleRowWidth*2
        height:_middleWidth * sliderModel.count + sliderModel.count*_margins
        color: palette.windowShadeDark
        border.width: 2
        border.color: palette.windowShade
        radius: 10

        Column{
            anchors.fill: parent

            FactPanelController {
                id:         controller
                factPanel:  qgcViewPanel
            }
            Component.onCompleted: {
                // Qml Sliders have a strange behavior in which they first set Slider::value to some internal
                // setting and then set Slider::value to the bound properties value. If you have an onValueChanged
                // handler which updates your property with the new value, this first value change will trash
                // your bound values. In order to work around this we don't set the values into the Sliders until
                // after Qml load is done. We also don't track value changes until Qml load completes.
                for (var i=0; i<sliderModel.count; i++) {
                    sliderRepeater.itemAt(i).sliderValue = controller.getParameterFact(-1, sliderModel.get(i).param).value
                }
                _loadComplete = true
                vehicletype();
            }
            Column {
                id:                                sliderOuterColumn
                anchors.left:           parent.left
                anchors.right:         parent.right
                spacing:                     _margins *0.5
                Repeater {
                    id:     sliderRepeater
                    model:  sliderModel
                    Rectangle {
                        id:                 sliderRect
                        width:          parent.width
                        height:        sliderColumn.y + sliderColumn.height + _margins * 0.5
                        color:           palette.windowShade
                        radius :        5
                        property alias sliderValue: slider.value
                        Column {
                            id:                 sliderColumn
                            anchors.margins:    _margins * 0.5
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            anchors.top:        sliderRect.top
                            Row{
                                spacing:    _margins*0.5
                                QGCLabel {
                                    text:           title
                                    font.family:    ScreenTools.demiboldFontFamily
                                    width:  _titleWidth
                                    anchors.baseline: sliderText.baseline
                                }
                                FactTextField{
                                    id: sliderText
                                    fact:       controller.getParameterFact(-1, param)
                                    showUnits: true
                                    width:  _middleRowWidth
                                }
                             }
                            Slider {
                                id:                 slider
                                anchors.left:       parent.left
                                anchors.right:      parent.right
                                minimumValue:       min
                                maximumValue:       max
                                stepSize:           isNaN(fact.increment) ? step : fact.increment
                                tickmarksEnabled:   true
                                property Fact fact: controller.getParameterFact(-1, param)
                                onValueChanged: {
                                    if (_loadComplete) {
                                        fact.value = value
                                    }
                                }
                            }
                            QGCLabel {
                                text:           description
                                anchors.left:   parent.left
                                anchors.right:  parent.right
                                wrapMode:       Text.WordWrap
                                font.pointSize: 10
                            }
                        } // Column
                    } // Rectangle
                } // Repeater
            } // Column
        } // QGCView
    }
    Rectangle{
        width: 3
        height:_middleWidth * sliderModel.count + sliderModel.count*_margins
        color:              palette.windowShade
        radius : 1
    }
    property var _vehicletype
    function vehicletype()
    {
       var  type = controller.setupSource()
        if (type  == "Plane")
            _vehicletype = plane;
        else if(type == "Copter")
            _vehicletype = copter
        else
            _vehicletype = vtol
    }

    Rectangle{
        width:_editFieldWidth*2 + _middleRowWidth*2 + _margins*3
        height:_middleWidth * sliderModel.count + sliderModel.count*_margins
        color: palette.windowShadeDark
        border.width: 2
        border.color: palette.windowShade
        radius: 10
        Loader{
            anchors.fill: parent
            sourceComponent: _vehicletype
        }
        Component{
             id :copter
             Column{
                 anchors.fill: parent
                 anchors.margins: _margins*0.5
                 spacing:  _margins *0.5
                 QGCLabel {
                     text:           qsTr("Multi Rotors")
                     font.family: ScreenTools.demiboldFontFamily
                 }
                 Rectangle{
                    anchors.left: parent.left
                    anchors.right: parent.right
                     height:     2
                 }
                 Row{
                     spacing:    _margins*0.5
                     QGCLabel{
                         text:       qsTr("LandingDescendRate:")
                         font.family:  ScreenTools.demiboldFontFamily
                         width:              _editFieldWidth
                         anchors.baseline: landSpeed.baseline
                     }
                     FactTextField{
                             id:                         landSpeed
                             fact:                     controller.getParameterFact(-1,  "MPC_LAND_SPEED")
                             showUnits:      true
                             width:                _middleRowWidth
                     }
                     QGCLabel{
                         text:                            qsTr("TakeOffClimbRate:")
                         font.family:               ScreenTools.demiboldFontFamily
                         width:                          _editFieldWidth
                         anchors.baseline:   takeOffSpeed.baseline
                     }
                     FactTextField{
                         id:                           takeOffSpeed
                         fact:                       controller.getParameterFact(-1, "MPC_TKO_SPEED")
                         showUnits:         true
                         width:                    _middleRowWidth
                     }
                 }
                 Row{
                     spacing:    _margins*0.5
                     QGCLabel{
                         text:                            qsTr("MaxDescendRate:")
                         font.family:              ScreenTools.demiboldFontFamily
                         width:                          _editFieldWidth
                         anchors.baseline:    descendSpeed.baseline
                     }
                     FactTextField{
                         id:                     descendSpeed
                         fact:                  controller.getParameterFact(-1, "MPC_Z_VEL_MAX_DN")
                         showUnits:    true
                         width:              _middleRowWidth
                     }
                     QGCLabel{
                         text:               qsTr("MaxClimbRate:")
                         font.family: ScreenTools.demiboldFontFamily
                         width:             _editFieldWidth
                         anchors.baseline:  ascentSpeed.baseline
                     }
                     FactTextField{
                         id:                 ascentSpeed
                         fact:              controller.getParameterFact(-1, "MPC_Z_VEL_MAX_UP")
                         showUnits:  true
                         width:            _middleRowWidth
                     }
                 }
                 Row{
                     spacing: _margins *0.5
                     QGCLabel{
                         text:           qsTr("MaxYaw rate:")
                         font.family: ScreenTools.demiboldFontFamily
                         width:          _editFieldWidth
                         anchors.baseline: yawRate.baseline
                     }
                     FactTextField{
                         id:         yawRate
                         fact:      controller.getParameterFact(-1 , "MC_YAWRATE_MAX")
                         showUnits: true
                         width:  _middleRowWidth
                     }
                     QGCLabel{
                         text:qsTr("Hover Throttle:")
                         font.family:  ScreenTools.demiboldFontFamily
                         width:     _editFieldWidth
                         anchors.baseline:  hoverThrottle.baseline
                     }
                     FactTextField{
                         id:        hoverThrottle
                         fact:      controller.getParameterFact(-1,  "MPC_THR_HOVER")
                         showUnits: true
                         width:     _middleRowWidth
                     }
                 }
                 Row{
                     spacing: _margins * 0.5
                     QGCLabel{
                         text:  qsTr("MinManualThr:")
                         font.family: ScreenTools.demiboldFontFamily
                         width:     _editFieldWidth
                         anchors.baseline: manualMinThr.baseline
                     }
                     FactTextField{
                         id:    manualMinThr
                         fact: controller.getParameterFact(-1, "MPC_MANTHR_MIN")
                        showUnits:  true
                        width:  _middleRowWidth
                     }
                 }
        }
    }
        Component{
            id: plane
            Column{
                anchors.fill: parent
                anchors.margins: _margins*0.5
                spacing:  _margins *0.5
                QGCLabel{
                    text:       qsTr("Plane")
                    font.family: ScreenTools.demiboldFontFamily
                }
                Rectangle{
                    height:  2
                    anchors.left: parent.left
                    anchors.right:parent.right
                }
                Row{
                    spacing: _margins *0.5
                    QGCLabel{
                        text:   qsTr("MaxThr Limit:")
                        font.family:  ScreenTools.demiboldFontFamily
                        width:          _editFieldWidth
                        anchors.baseline:  maxthrottleLimit.baseline
                    }
                    FactTextField{
                        id:     maxthrottleLimit
                        fact: controller.getParameterFact(-1,  "FW_THR_MAX")
                        showUnits: true
                        width:      _middleRowWidth
                    }
                    QGCLabel{
                        text:       qsTr("MinThr Limit:")
                        font.family: ScreenTools.demiboldFontFamily
                        width:          _editFieldWidth
                        anchors.baseline: minthrottleLimit.baseline
                    }
                    FactTextField{
                        id:     minthrottleLimit
                        fact:       controller.getParameterFact(-1, "FW_THR_MIN")
                        showUnits: true
                        width:      _middleRowWidth
                    }
                }
                Row{
                    spacing: _margins*0.5
                    QGCLabel{
                        text:  qsTr("MaxClimbRate:")
                        font.family: ScreenTools.demiboldFontFamily
                        width:  _editFieldWidth
                        anchors.baseline:   maxClimbRate.baseline
                    }
                    FactTextField{
                        id:     maxClimbRate
                        fact:   controller.getParameterFact(-1, "FW_T_CLMB_MAX")
                        showUnits:  true
                        width:      _middleRowWidth
                    }
                    QGCLabel{
                        text:   qsTr("Pitch Offset:")
                        font.family:  ScreenTools.demiboldFontFamily
                        width:      _editFieldWidth
                        anchors.baseline:  pitchOffset.baseline
                    }
                    FactTextField{
                        id:     pitchOffset
                        fact:   controller.getParameterFact(-1, "FW_PSP_OFF")
                        showUnits:  true
                        width:  _middleRowWidth
                    }
                }
                Row{
                    spacing:  _margins * 0.5
                    QGCLabel{
                        text:       qsTr("Cruise Airspeed:")
                        font.family: ScreenTools.demiboldFontFamily
                        width:          _editFieldWidth
                        anchors.baseline:  cruiseSpeed.baseline
                    }
                    FactTextField{
                        id:         cruiseSpeed
                        fact:      controller.getParameterFact(-1, "FW_AIRSPD_TRIM")
                        showUnits: true
                        width: _middleRowWidth
                    }
                    QGCLabel{
                        text:      qsTr("Cruise Throttle:")
                        font.family: ScreenTools.demiboldFontFamily
                        width:  _editFieldWidth
                        anchors.baseline: cruiseThrottle.baseline
                    }
                    FactTextField{
                        id: cruiseThrottle
                        fact: controller.getParameterFact(-1, "FW_THR_CRUISE")
                        showUnits:  true
                        width:  _middleRowWidth
                    }
                }
                Row{
                    spacing : _margins*0.5
                    QGCLabel{
                        text: qsTr("Parachute Throttle:")
                        font.family: ScreenTools.demiboldFontFamily
                        width:  _editFieldWidth
                        anchors.baseline: parachutethr.baseline
                    }
                    FactTextField{
                        id: parachutethr
                        fact:controller.getParameterFact(-1,"PARACHUTE_THR")
                        showUnits:true
                        width:_middleRowWidth
                    }
                    QGCLabel{
                        text: qsTr("Parachute Delay:")
                        font.family: ScreenTools.demiboldFontFamily
                        width:  _editFieldWidth
                        anchors.baseline: parachutedelay.baseline
                    }
                    FactTextField{
                        id: parachutedelay
                        fact:controller.getParameterFact(-1,"PARACHUTE_TIM")
                        showUnits:true
                        width:_middleRowWidth
                    }
                }
//                Rectangle{
//                    anchors.left :parent.left
//                    anchors.right: parent.right
//                    height: 2
//                }
//                QGCButton{
//                   id:planeloadButton
//                    text:qsTr("Advance settings >>")
//                    visible :true
//                    onClicked: {
//                        visible = false
//                        planeadvanceSetting.sourceComponent = rcSettings
//                        planeexistButton.visible = true
//                    }
//                }
//                QGCButton{
//                    id: planeexistButton
//                    text:qsTr("Advance settings<<")
//                    visible:false
//                    onClicked: {
//                        visible = false
//                        planeadvanceSetting.sourceComponent  = undefined
//                        planeloadButton.visible = true
//                    }
//                }
//                Loader{
//                    id: planeadvanceSetting
//                }
            }
        }
        Component{
            id : vtol
            Column{
                anchors.fill: parent
                anchors.margins: _margins*0.5
                spacing:            _margins*0.5
                QGCLabel{
                    text:   qsTr("Hovering Mode")
                    font.family: ScreenTools.demiboldFontFamily
                }
                Rectangle{
                    anchors.left: parent.left
                    anchors.right:parent.right
                    height: 2
                }
                Row{
                    spacing:    _margins*0.5
                    QGCLabel{
                        text:       qsTr("LandingDescendRate:")
                        font.family:  ScreenTools.demiboldFontFamily
                        width:              _editFieldWidth
                        anchors.baseline: landSpeed.baseline
                    }
                    FactTextField{
                            id:                         landSpeed
                            fact:                     controller.getParameterFact(-1,  "MPC_LAND_SPEED")
                            showUnits:      true
                            width:                _middleRowWidth
                    }
                    QGCLabel{
                        text:                            qsTr("TakeOffClimbRate:")
                        font.family:               ScreenTools.demiboldFontFamily
                        width:                          _editFieldWidth
                        anchors.baseline:   takeOffSpeed.baseline
                    }
                    FactTextField{
                        id:                           takeOffSpeed
                        fact:                       controller.getParameterFact(-1, "MPC_TKO_SPEED")
                        showUnits:         true
                        width:                    _middleRowWidth
                    }
                }
                Row{
                    spacing:    _margins*0.5
                    QGCLabel{
                        text:                            qsTr("MaxDescendRate:")
                        font.family:              ScreenTools.demiboldFontFamily
                        width:                          _editFieldWidth
                        anchors.baseline:    descendSpeed.baseline
                    }
                    FactTextField{
                        id:                     descendSpeed
                        fact:                  controller.getParameterFact(-1, "MPC_Z_VEL_MAX_DN")
                        showUnits:    true
                        width:              _middleRowWidth
                    }
                    QGCLabel{
                        text:               qsTr("MaxClimbRate:")
                        font.family: ScreenTools.demiboldFontFamily
                        width:             _editFieldWidth
                        anchors.baseline:  ascentSpeed.baseline
                    }
                    FactTextField{
                        id:                 ascentSpeed
                        fact:              controller.getParameterFact(-1, "MPC_Z_VEL_MAX_UP")
                        showUnits:  true
                        width:            _middleRowWidth
                    }
                }
                Row{
                    spacing: _margins *0.5
                    QGCLabel{
                        text:           qsTr("MaxYaw rate:")
                        font.family: ScreenTools.demiboldFontFamily
                        width:          _editFieldWidth
                        anchors.baseline: yawRate.baseline
                    }
                    FactTextField{
                        id:         yawRate
                        fact:      controller.getParameterFact(-1 , "MC_YAWRATE_MAX")
                        showUnits: true
                        width:  _middleRowWidth
                    }
                    QGCLabel{
                        text:qsTr("Hover Throttle:")
                        font.family:  ScreenTools.demiboldFontFamily
                        width:     _editFieldWidth
                        anchors.baseline:  hoverThrottle.baseline
                    }
                    FactTextField{
                        id:        hoverThrottle
                        fact:      controller.getParameterFact(-1,  "MPC_THR_HOVER")
                        showUnits: true
                        width:     _middleRowWidth
                    }
                }
                Row{
                    spacing: _margins * 0.5
                    QGCLabel{
                        text:  qsTr("MinManualThr:")
                        font.family: ScreenTools.demiboldFontFamily
                        width:     _editFieldWidth
                        anchors.baseline: manualMinThr.baseline
                    }
                    FactTextField{
                        id:    manualMinThr
                        fact: controller.getParameterFact(-1, "MPC_MANTHR_MIN")
                       showUnits:  true
                       width:  _middleRowWidth
                    }
                }
                QGCLabel{
                    text:   qsTr("Plane Mode")
                    font.family:    ScreenTools.demiboldFontFamily
                }
                Rectangle{
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height:2
                }
                Row{
                    spacing: _margins *0.5
                    QGCLabel{
                        text:   qsTr("MaxThr Limit:")
                        font.family:  ScreenTools.demiboldFontFamily
                        width:          _editFieldWidth
                        anchors.baseline:  maxthrottleLimit.baseline
                    }
                    FactTextField{
                        id:     maxthrottleLimit
                        fact: controller.getParameterFact(-1,  "FW_THR_MAX")
                        showUnits: true
                        width:      _middleRowWidth
                    }
                    QGCLabel{
                        text:       qsTr("MinThr Limit:")
                        font.family: ScreenTools.demiboldFontFamily
                        width:          _editFieldWidth
                        anchors.baseline: minthrottleLimit.baseline
                    }
                    FactTextField{
                        id:     minthrottleLimit
                        fact:       controller.getParameterFact(-1, "FW_THR_MIN")
                        showUnits: true
                        width:      _middleRowWidth
                    }
                }
                Row{
                    spacing: _margins*0.5
                    QGCLabel{
                        text:  qsTr("MaxClimbRate:")
                        font.family: ScreenTools.demiboldFontFamily
                        width:  _editFieldWidth
                        anchors.baseline:   maxClimbRate.baseline
                    }
                    FactTextField{
                        id:     maxClimbRate
                        fact:   controller.getParameterFact(-1, "FW_T_CLMB_MAX")
                        showUnits:  true
                        width:      _middleRowWidth
                    }
                    QGCLabel{
                        text:   qsTr("Pitch Offset:")
                        font.family:  ScreenTools.demiboldFontFamily
                        width:      _editFieldWidth
                        anchors.baseline:  pitchOffset.baseline
                    }
                    FactTextField{
                        id:     pitchOffset
                        fact:   controller.getParameterFact(-1, "FW_PSP_OFF")
                        showUnits:  true
                        width:  _middleRowWidth
                    }
                }
                Row{
                    spacing:  _margins * 0.5
                    QGCLabel{
                        text:       qsTr("Cruise Airspeed:")
                        font.family: ScreenTools.demiboldFontFamily
                        width:          _editFieldWidth
                        anchors.baseline:  cruiseSpeed.baseline
                    }
                    FactTextField{
                        id:         cruiseSpeed
                        fact:      controller.getParameterFact(-1, "FW_AIRSPD_TRIM")
                        showUnits: true
                        width: _middleRowWidth
                    }
                    QGCLabel{
                        text:       qsTr("Cruise Throttle:")
                        font.family: ScreenTools.demiboldFontFamily
                        width:  _editFieldWidth
                        anchors.baseline: cruiseThrottle.baseline
                    }
                    FactTextField{
                        id: cruiseThrottle
                        fact: controller.getParameterFact(-1, "FW_THR_CRUISE")
                        showUnits:  true
                        width:  _middleRowWidth
                    }
                }
                Row{
                    spacing : _margins*0.5
                    QGCLabel{
                        text: qsTr("Parachute Throttle:")
                        font.family: ScreenTools.demiboldFontFamily
                        width:  _editFieldWidth
                        anchors.baseline: parachutethr.baseline
                    }
                    FactTextField{
                        id: parachutethr
                        fact:controller.getParameterFact(-1,"PARACHUTE_THR")
                        showUnits:true
                        width:_middleRowWidth
                    }
                    QGCLabel{
                        text: qsTr("Parachute Delay:")
                        font.family: ScreenTools.demiboldFontFamily
                        width:  _editFieldWidth
                        anchors.baseline: parachutedelay.baseline
                    }
                    FactTextField{
                        id: parachutedelay
                        fact:controller.getParameterFact(-1,"PARACHUTE_TIM")
                        showUnits:true
                        width:_middleRowWidth
                    }
                }
//                Rectangle{
//                    anchors.left :parent.left
//                    anchors.right: parent.right
//                    height: 2
//                }
//                QGCButton{
//                   id: vtolloadButton
//                    text:qsTr("Advance settings >>")
//                    visible :true
//                    onClicked: {
//                        visible = false
//                        vtolladvanceSetting.sourceComponent = rcSettings
//                        vtolexistButton.visible = true
//                    }
//                }
//                QGCButton{
//                    id: vtolexistButton
//                    text:qsTr("Advance settings<<")
//                    visible:false
//                    onClicked: {
//                        visible = false
//                        vtolladvanceSetting.sourceComponent  = undefined
//                        vtolloadButton.visible = true
//                    }
//                }
//                Loader{
//                    id: vtolladvanceSetting
//                }
            }
        }
    }
}
