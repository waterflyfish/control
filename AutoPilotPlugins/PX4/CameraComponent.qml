/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                      2.5
import QtQuick.Controls             1.2
import QtQuick.Controls.Styles      1.2
import QtQuick.Layouts              1.2
import QtGraphicalEffects           1.0

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0


SetupPage {
    id:             cameraPage
    pageComponent:  pageComponent

    Component {
        id: pageComponent

        Column {
            width:  availableWidth

            FactPanelController { id: controller; factPanel: cameraPage.viewPanel }

            property real _margins:         ScreenTools.defaultFontPixelHeight
            property real _middleRowWidth:  ScreenTools.defaultFontPixelWidth * 16
            property real _editFieldWidth:  ScreenTools.defaultFontPixelWidth * 16

            property Fact _camTriggerMode:  controller.getParameterFact(-1, "TRIG_MODE")
            property Fact _camTriggerInterface:  controller.getParameterFact(-1, "TRIG_INTERFACE", false)
            property Fact _camTriggerPol:   controller.getParameterFact(-1, "TRIG_POLARITY", false) // Don't bitch about missing as these only exist if trigger mode is enabled
            property Fact _auxPins:         controller.getParameterFact(-1, "TRIG_PINS",     false) // Ditto

            property bool _rebooting:       false
            property var  _auxChannels:     [ 0, 0]

            function clearAuxArray() {
                for(var i = 0; i < 2; i++) {
                    _auxChannels[i] = 0
                }
            }

            function setAuxPins() {
                if(_auxPins) {
                    var values = ""
                    for(var i = 0; i < 2; i++) {
                        if(_auxChannels[i]) {
                            values += ((i+1).toString())
                        }
                    }
                    _auxPins.value = parseInt(values)
                }
            }

            Component.onCompleted: {
                if(_camTriggerInterface == null || _camTriggerMode == null || _camTriggerPol == null){
                    delayedRefresh.start()
                }

                if(_auxPins) {
                    clearAuxArray()
                    var values  = _auxPins.value.toString()
                    for(var i = 0; i < values.length; i++) {
                        var b = parseInt(values[i]) - 1
                        if(b >= 0 && b < 2) {
                            _auxChannels[b] = 1
                        }
                    }
                }
            }

            Item {
                id:                     applyAndRestart
                visible:                true
                anchors.left:           parent.left
                anchors.right:          parent.right
                anchors.leftMargin:     ScreenTools.defaultFontPixelWidth * 10
                anchors.rightMargin:    ScreenTools.defaultFontPixelWidth * 10
                height:                 applyButton.height
                QGCLabel {
                    anchors.left:       parent.left
                    text:               ""//qsTr("Vehicle must be restarted for changes to take effect. ")
                }
                QGCButton {
                    id:                 applyButton
                    visible:            false
                    anchors.right:      parent.right
                    text:               qsTr("Apply")
                    onClicked:      {
                        //-- This will reboot the vehicle! We're set not to allow changes if armed.
                        //QGroundControl.multiVehicleManager.activeVehicle.rebootVehicle()
                        applyButton.visible = false
                        //_rebooting = true
                        _rebooting = false
                        if (camTrigCombo.indexModel) {
                            _camTriggerMode.value = camTrigCombo.indexSelect
                        } else {
                            _camTriggerMode.value = _camTriggerMode.enumValues[camTrigCombo.indexSelect]
                        }
                        delayedRefresh.start()
                    }
                }

                QGCButton {
                    id:                 refreshButton
                    anchors.right:      applyButton.left
                    anchors.rightMargin: 10
                    text:               qsTr("refresh")
                    onClicked:      {
                        controller.refreshParameter(-1, "TRIG_MODE");
                        controller.refreshParameter(-1, "TRIG_PINS");
                        controller.refreshParameter(-1, "TRIG_INTERFACE");
                        controller.refreshParameter(-1, "TRIG_POLARITY");
                        controller.refreshParameter(-1, "TRIG_DISTANCE")
                        controller.refreshParameter(-1, "TRIG_INTERVAL")
                        controller.refreshParameter(-1, "TRIG_ACT_TIME")
                        _camTriggerMode = controller.getParameterFact(-1, "TRIG_MODE")
                        _camTriggerInterface = controller.getParameterFact(-1, "TRIG_INTERFACE", false)
                        _camTriggerPol = controller.getParameterFact(-1, "TRIG_POLARITY", false) // Don't bitch about missing as these only exist if trigger mode is enabled
                        _auxPins = controller.getParameterFact(-1, "TRIG_PINS",     false) // Ditto
                        camInterfaceCombo.currentIndex = _camTriggerInterface.value - 1
                        timeIntervalField.fact = controller.getParameterFact(-1, "TRIG_INTERVAL", false)
                        trigDistField.fact =  controller.getParameterFact(-1, "TRIG_DISTANCE", false)
                        //if(controller.getParameterFact(-1, "TRIG_ACT_TIME", false) != null)
                        {
                            trigPeriodField.fact = controller.getParameterFact(-1, "TRIG_ACT_TIME", false)
                        }
                    }
                }

                Timer {
                    id:         delayedRefresh
                    interval:   3000
                    running:    false
                    repeat:     false
                    onTriggered:{
                        controller.refreshParameter(-1, "TRIG_MODE");
                        controller.refreshParameter(-1, "TRIG_PINS");
                        controller.refreshParameter(-1, "TRIG_INTERFACE");
                        controller.refreshParameter(-1, "TRIG_POLARITY");
                        controller.refreshParameter(-1, "TRIG_DISTANCE")
                        controller.refreshParameter(-1, "TRIG_INTERVAL")
                        controller.refreshParameter(-1, "TRIG_ACT_TIME")
                        _camTriggerMode = controller.getParameterFact(-1, "TRIG_MODE")
                        _camTriggerInterface = controller.getParameterFact(-1, "TRIG_INTERFACE", false)
                        _camTriggerPol = controller.getParameterFact(-1, "TRIG_POLARITY", false) // Don't bitch about missing as these only exist if trigger mode is enabled
                        _auxPins = controller.getParameterFact(-1, "TRIG_PINS",     false) // Ditto
                        camInterfaceCombo.currentIndex = _camTriggerInterface.value - 1
                        timeIntervalField.fact = controller.getParameterFact(-1, "TRIG_INTERVAL", false)
                        trigDistField.fact =  controller.getParameterFact(-1, "TRIG_DISTANCE", false)
                        //if(controller.getParameterFact(-1, "TRIG_ACT_TIME", false) != null)
                        {
                            trigPeriodField.fact = controller.getParameterFact(-1, "TRIG_ACT_TIME", false)
                        }
                    }
                }
            }

            Column {
                id:                                     mainCol
                spacing:                                _margins
                anchors.horizontalCenter:               parent.horizontalCenter
                /*
                   **** Camera Trigger ****
                */
                QGCLabel {
                    text:                               qsTr("Camera Trigger Settings")
                    font.family:                        ScreenTools.demiboldFontFamily
                }
                Rectangle {
                    id:                                 camTrigRect
                    color:                              qgcPal.windowShade
                    width:                              camTrigRow.width  + _margins * 2
                    height:                             camTrigRow.height + _margins * 2
                    Row {
                        id:                             camTrigRow
                        spacing:                        _margins
                        anchors.verticalCenter:         parent.verticalCenter
                        Item { width: _margins * 0.5; height: 1; }
                        QGCColoredImage {
                            color:                      qgcPal.text
                            height:                     ScreenTools.defaultFontPixelWidth * 10
                            width:                      ScreenTools.defaultFontPixelWidth * 20
                            sourceSize.width:           width
                            mipmap:                     true
                            fillMode:                   Image.PreserveAspectFit
                            source:                     "/qmlimages/CameraTrigger.svg"
                            anchors.verticalCenter:     parent.verticalCenter
                        }
                        Item { width: _margins * 0.5; height: 1; }
                        Column {
                            spacing:                    _margins * 0.5
                            anchors.verticalCenter:     parent.verticalCenter
                            Row {
                                QGCLabel {
                                    anchors.baseline:   camTrigCombo.baseline
                                    width:              _middleRowWidth
                                    text:               qsTr("Trigger mode:")
                                }
                                QGCComboBox{
                                    id:                 camTrigCombo
                                    property bool indexModel:   false  ///< true: model must be specifed, selected index is fact value, false: use enum meta data
                                    property var  changModel:   indexModel ? _camTriggerMode.value : _camTriggerMode.enumIndex
                                    property var  indexSelect : indexModel ? _camTriggerMode.value : _camTriggerMode.enumIndex
                                    width:              _editFieldWidth
                                    enabled:            !_rebooting
                                    model:              _camTriggerMode != null ? _camTriggerMode.enumStrings : null
                                    currentIndex:       indexModel ? _camTriggerMode.value : _camTriggerMode.enumIndex
                                    onActivated: {
                                        applyButton.visible = true
                                        indexSelect = index
                                    }
                                }
//                                FactComboBox {
//                                    id:                 camTrigCombo
//                                    width:              _editFieldWidth
//                                    fact:               _camTriggerMode
//                                    indexModel:         false
//                                    enabled:            !_rebooting
//                                    onActivated: {
//                                        applyButton.visible = true
//                                    }
//                                }
                            }
                            Row {
                                visible: _camTriggerInterface  && _camTriggerMode.value != 0 ? true : false
                                QGCLabel {
                                    anchors.baseline:   camInterfaceCombo.baseline
                                    width:              _middleRowWidth
                                    text:               qsTr("Trigger interface:")
                                }

                                QGCComboBox{
                                    id:                 camInterfaceCombo
                                    property bool indexModel:   false
                                    width:              _editFieldWidth
                                    enabled:            !_rebooting
                                    model:              ["GPIO", "Unknow"]
//                                    {
//                                        if(_camTriggerInterface != null){
//                                           if(_camTriggerInterface.enumIndex == 0){
//                                                return ["GPIO"]
//                                           }else{
//                                               return ["Unknow " + _camTriggerInterface.enumIndex]
//                                           }
//                                        }else{
//                                            return null
//                                        }
//                                    }
                                    currentIndex:       indexModel ? _camTriggerInterface.value - 1: _camTriggerInterface.enumIndex
                                    onActivated: {
                                        if (indexModel) {
                                            _camTriggerInterface.value = index
                                        } else {
                                            _camTriggerInterface.value = index + 1// _camTriggerInterface.enumValues[index]
                                            console.log(_camTriggerInterface.value)
                                        }
                                    }
                                }

//                                FactComboBox {
//                                    id:                 camInterfaceCombo
//                                    width:              _editFieldWidth
//                                    fact:               _camTriggerInterface
//                                    indexModel:         false
//                                    enabled:            !_rebooting
//                                    onActivated: {
//                                        //applyButton.visible = true
//                                    }
//                                }
                            }
                            Row {
                                QGCLabel {
                                    text:               qsTr("Time Interval")
                                    width:              _middleRowWidth
                                    anchors.baseline:   timeIntervalField.baseline
                                    color:              qgcPal.text
                                }
                                FactTextField {
                                    id:                 timeIntervalField
                                    fact:               controller.getParameterFact(-1, "TRIG_INTERVAL", false)
                                    showUnits:          true
                                    width:              _editFieldWidth
                                    enabled:            _auxPins && _camTriggerMode.value === 2
                                }
                            }
                            Row {
                                QGCLabel {
                                    text:               qsTr("Distance Interval")
                                    width:              _middleRowWidth
                                    anchors.baseline:   trigDistField.baseline
                                    color:              qgcPal.text
                                }
                                FactTextField {
                                    id:                 trigDistField
                                    fact:               controller.getParameterFact(-1, "TRIG_DISTANCE", false)
                                    showUnits:          true
                                    width:              _editFieldWidth
                                    enabled:            _auxPins && _camTriggerMode.value === 3
                                }
                            }
                        }
                    }
                }
                /*
                   **** Camera Hardware ****
                */
                Item { width: 1; height: _margins * 0.5; }
                QGCLabel {
                    text:                               qsTr("Hardware Settings")
                    font.family:                        ScreenTools.demiboldFontFamily
                    visible:                            _auxPins && _camTriggerMode.value != 0
                }
                Rectangle {
                    color:                              qgcPal.windowShade
                    width:                              camTrigRect.width
                    height:                             camHardwareRow.height + _margins * 2
                    visible:                            _auxPins && _camTriggerMode.value != 0
                    Row {
                        id:                             camHardwareRow
                        spacing:                        _margins
                        anchors.verticalCenter:         parent.verticalCenter
                        Item { width: _margins * 0.5; height: 1; }
                        Item {
                            height:                     ScreenTools.defaultFontPixelWidth * 10
                            width:                      ScreenTools.defaultFontPixelWidth * 20
                            Column {
                                spacing:                ScreenTools.defaultFontPixelHeight
                                anchors.centerIn:       parent
                                QGCLabel {
                                    text:               qsTr("AUX Pin Assignment")
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                Row {
                                    spacing:            ScreenTools.defaultFontPixelWidth
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    Repeater {
                                        model:          [_auxChannels[0], _auxChannels[1]]
                                        Column {
                                            spacing:            ScreenTools.defaultFontPixelWidth * 0.5
                                            QGCLabel {
                                                text:           model.index + 1
                                                color:          qgcPal.text
                                                anchors.horizontalCenter: parent.horizontalCenter
                                            }
                                            Rectangle {
                                                id:             auxPin
                                                width:          ScreenTools.defaultFontPixelWidth * 2
                                                height:         ScreenTools.defaultFontPixelWidth * 2
                                                border.color:   qgcPal.text
                                                color:  {
                                                    if(_auxPins) {
                                                        var pins = _auxPins.value.toString()
                                                        var pin  = (model.index + 1).toString()
                                                        if(pins.indexOf(pin) < 0)
                                                            return qgcPal.windowShadeDark
                                                        else
                                                            return "green"
                                                    } else {
                                                        return qgcPal.windowShade
                                                    }
                                                }
                                                MouseArea {
                                                    anchors.fill: parent
                                                    onClicked: {
                                                        _auxChannels[model.index] = 1 - _auxChannels[model.index]
                                                        auxPin.color = _auxChannels[model.index] ? "green" : qgcPal.windowShadeDark
                                                        setAuxPins()
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        Item { width: _margins * 0.5; height: 1; }
                        Column {
                            visible:                    !_camTriggerInterface || (_camTriggerInterface.value === 1)
                            spacing:                    _margins * 0.5
                            anchors.verticalCenter:     parent.verticalCenter
                            QGCLabel {
                                id:                     returnHomeLabel
                                text:                   qsTr("Trigger Pin Polarity:")
                            }
                            Row {
                                Item { height: 1; width: _margins; }
                                Column {
                                    spacing:            _margins * 0.5
                                    ExclusiveGroup { id: polarityGroup }
                                    QGCRadioButton {
                                        checked:        _camTriggerPol && _camTriggerPol.value === 0
                                        exclusiveGroup: polarityGroup
                                        text:           qsTr("Low (0V)")
                                        onClicked: {
                                            if(_camTriggerPol) {
                                                _camTriggerPol.value = 0
                                            }
                                        }
                                    }
                                    QGCRadioButton {
                                        checked:        _camTriggerPol && _camTriggerPol.value > 0
                                        exclusiveGroup: polarityGroup
                                        text:           qsTr("High (3.3V)")
                                        onClicked: {
                                            if(_camTriggerPol) {
                                                _camTriggerPol.value = 1
                                            }
                                        }
                                    }
                                }
                            }
                            Item { width: 1; height: _margins * 0.5; }
                            Row {
                                QGCLabel {
                                    text:               qsTr("Trigger Period")
                                    width:              _middleRowWidth
                                    anchors.baseline:   trigPeriodField.baseline
                                    color:              qgcPal.text
                                }
                                FactTextField {
                                    id:                 trigPeriodField
                                    fact:               controller.getParameterFact(-1, "TRIG_ACT_TIME", false)
                                    showUnits:          true
                                    width:              _editFieldWidth
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
