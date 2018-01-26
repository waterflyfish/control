/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controllers           1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0

QGCView {
    id:         qgcView
    viewPanel:  panel

    // Those user visible strings are hard to translate because we can't send the
    // HTML strings to translation as this can create a security risk. we need to find
    // a better way to hightlight them, or use less hightlights.

    // User visible strings
    readonly property string title:             qsTr("FIRMWARE")
    readonly property string highlightPrefix:   "<font color=\"" + qgcPal.warningText + "\">"
    readonly property string highlightSuffix:   "</font>"
    readonly property string welcomeText:       qsTr("QGroundControl can upgrade the firmware on Pixhawk devices, SiK Radios and PX4 Flow Smart Cameras.")
    readonly property string plugInText:        "<big>" + highlightPrefix + qsTr("Plug in your device") + highlightSuffix + qsTr(" via USB to ") + highlightPrefix + qsTr("start") + highlightSuffix + qsTr(" firmware upgrade.")+"</big>"
    /* changed by gc 20160428 *.
    readonly property string flashFailText:     "If upgrade failed, make sure to connect " + highlightPrefix + "directly" + highlightSuffix + " to a powered USB port on your computer, not through a USB hub. " +
                                                "Also make sure you are only powered via USB " + highlightPrefix + "not battery" + highlightSuffix + "."
    */
    readonly property string flashFailText:     qsTr("If upgrade failed, make sure to connect ") + highlightPrefix + qsTr("directly") + highlightSuffix + qsTr(" to a powered USB port on your computer, or connect with ") + highlightPrefix + qsTr("USB Serial port") +highlightSuffix + qsTr(", not through a USB hub. ") +
                                                qsTr("Also make sure you are only powered via USB ") + highlightPrefix + qsTr("not battery") + highlightSuffix + "."
    /* change end 20160428 */
    readonly property string qgcUnplugText1:    qsTr("All QGroundControl connections to vehicles must be ") + highlightPrefix + qsTr(" disconnected ") + highlightSuffix + qsTr("prior to firmware upgrade.")
    readonly property string qgcUnplugText2:    highlightPrefix + "<big>"+qsTr("Please unplug your Pixhawk and/or Radio from USB.")+"</big>" + highlightSuffix

    property string firmwareWarningMessage
    property bool   controllerCompleted:      false
    property bool   initialBoardSearch:       true
    property string firmwareName

    property var selectFirmwareArray: new Array
    property var firmwareSelectedTypeArray : new Array
    property var selectFirmwareComArray: new Array
    property int currentSelected : -1
    property var currentSelectedFirmware
    function cancelFlash() {
        statusTextArea.append(highlightPrefix + qsTr("Upgrade cancelled") + highlightSuffix)
        statusTextArea.append("------------------------------------------")
        controller.cancel()
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }
    QGCPalette { id: __qgcPal; colorGroupEnabled: true }

    FirmwareUpgradeController {
        id:             controller
        progressBar:    progressBar
        statusLog:      statusTextArea

        property var activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

        Component.onCompleted: {
            controllerCompleted = true
            if (qgcView.completedSignalled) {
                // We can only start the board search when the Qml and Controller are completely done loading
                //controller.startBoardSearch()
                controller.startBoardSearchInfo()
                if(androidType){
                    showDialog(pixhawkFirmwareSelectDialog, title, qgcView.showDialogDefaultWidth, StandardButton.Ok | StandardButton.Cancel)
                }else{
                    showDialog(pixhawkFirmwareSelectDialog, title, qgcView.showDialogDefaultWidth * 2, StandardButton.Ok | StandardButton.Cancel)
                }
            }
        }

        onActiveVehicleChanged: {
            if (!activeVehicle) {
                statusTextArea.append(plugInText)
            }
        }

        onNoBoardFound: {
            initialBoardSearch = false
            if (!QGroundControl.multiVehicleManager.activeVehicleAvailable) {
                statusTextArea.append(plugInText)
            }
        }

        onBoardGone: {
            initialBoardSearch = false
            if (!QGroundControl.multiVehicleManager.activeVehicleAvailable) {
                statusTextArea.append(plugInText)
            }
        }

        onBoardFound: {
            if (initialBoardSearch) {
                // Board was found right away, so something is already plugged in before we've started upgrade
                statusTextArea.append(qgcUnplugText1)
                statusTextArea.append(qgcUnplugText2)
                if(QGroundControl.multiVehicleManager.activeVehicle)
                    QGroundControl.multiVehicleManager.activeVehicle.autoDisconnect = true
            } else {
                // We end up here when we detect a board plugged in after we've started upgrade
                statusTextArea.append(highlightPrefix + qsTr("Found device") + highlightSuffix + ": " + controller.boardType)
                //if (controller.boardType == "Pixhawk" || controller.boardType == "AeroCore" || controller.boardType == "PX4 Flow" || controller.boardType == "PX4 FMU V1" || controller.boardType == "MindPX") {
                if(controller.boardDescription != "USB Serial Port" && controller.boardDescription != "Silicon Labs CP210x USB to UART Bridge"){
                    //showDialog(pixhawkFirmwareSelectDialogComponent, title, qgcView.showDialogDefaultWidth, StandardButton.Ok | StandardButton.Cancel)
                    controller.flash()
                }
            }
        }

        onUsbSerialFound: {
            statusTextArea.append(highlightPrefix + qsTr("Found device") + highlightSuffix + ": " + controller.boardType)
        }

        onUsbSerialUpgrade: {
            controller.usbSerialDownloadFirmware()
            //showDialog(pixhawkFirmwareSelectDialog, title, qgcView.showDialogDefaultWidth, StandardButton.Ok | StandardButton.Cancel)
        }

        onError: {
            hideDialog()
            statusTextArea.append(flashFailText)
        }
    }

    onCompleted: {
        if (controllerCompleted) {
            // We can only start the board search when the Qml and Controller are completely done loading
            //controller.startBoardSearch()
        }
    }

    FileDialog {
        id:             fileDialog
        folder:         shortcuts.home
        nameFilters:    [qsTr("Firmware Files (*.bin *.px4 *.ihx *.txt)")]
        selectExisting: true
        title:          qsTr("Select Firmware File")
        onAccepted: {
            //debugMessageModel.writeMessages(fileUrl);
            visible = false;
            currentSelectedFirmware.checked = true;
            if(fileDialog.fileUrl.toString().match("(CustomFirmware)|(.*\.(txt|bin|ihx|px4)$)")){
                var fileName = fileDialog.fileUrl.toString().replace("file://", "")
                controller.getLocalFile(fileName, currentSelected)
            }
        }
        onRejected:     visible = false
    }

    Component {
        id: pixhawkFirmwareSelectDialog


        QGCViewDialog {
            id:             qgcViewDialog
            anchors.fill:   parent
            property bool px4Flow:                      controller.boardType == "PX4 Flow"

            Component.onCompleted: {
                selectFirmwareArray = [qgcStack, px4FlightStack, apmFlightStack,
                                       gps, gimbals, smartBattery,
                                       smartConsole, brushlessFOC, raspberry]
                selectFirmwareComArray = [qgcStackNewCombo, px4FlightStackNewCombo, apmFlightStackNewCombo,
                                          gpsNewCombo, gimbalsNewCombo, smartBatteryNewCombo,
                                          smartConsoleNewCombo, brushlessFOCNewCombo]
                firmwareSelectedTypeArray = [FirmwareUpgradeController.Board -2 , FirmwareUpgradeController.Board - 1, FirmwareUpgradeController.Board,
                                             FirmwareUpgradeController.GPS, FirmwareUpgradeController.Gimbal, FirmwareUpgradeController.Battery,
                                             FirmwareUpgradeController.RemoteControl, FirmwareUpgradeController.ESpeedControl]
            }


            function accept() {
                hideDialog()
                var firmwareSelectedType = new Array
                var currentIndex = new Array
                for(var i = 0; i < selectFirmwareArray.length; i++){
                    if(selectFirmwareArray[i].checked == false){
                        continue;
                    }
                    if(selectFirmwareComArray[i].count){
                        firmwareSelectedType.push(firmwareSelectedTypeArray[i])
                        currentIndex.push(selectFirmwareComArray[i].currentIndex)
                    }
                }
                controller.setSelectedFirmware(firmwareSelectedType, currentIndex);
                controller.startBoardSearch()
            }

            function reject() {
                hideDialog()
                cancelFlash()
            }

            ExclusiveGroup {
                id: firmwareGroup
            }

            ListModel {
                id: firmwareTypeList

                ListElement {
                    text:           qsTr("Standard Version (stable)")
                    firmwareType:   FirmwareUpgradeController.StableFirmware
                }
                ListElement {
                    text:           qsTr("Beta Testing (beta)")
                    firmwareType:   FirmwareUpgradeController.BetaFirmware
                }
                ListElement {
                    text:           qsTr("Developer Build (master)")
                    firmwareType:   FirmwareUpgradeController.DeveloperFirmware
                }
                ListElement {
                    text:           qsTr("Custom firmware file...")
                    firmwareType:   FirmwareUpgradeController.CustomFirmware
                }
            }

            ListModel {
                id: px4FlowTypeList

                ListElement {
                    text:           qsTr("Standard Version (stable)")
                    firmwareType:   FirmwareUpgradeController.StableFirmware
                }
                ListElement {
                    text:           qsTr("Custom firmware file...")
                    firmwareType:   FirmwareUpgradeController.CustomFirmware
                }
            }

            function firmwareVersionChanged(model) {
                firmwareVersionWarningLabel.visible = false
                // All of this bizarre, setting model to null and index to 1 and then to 0 is to work around
                // strangeness in the combo box implementation. This sequence of steps correctly changes the combo model
                // without generating any warnings and correctly updates the combo text with the new selection.
                firmwareVersionCombo.model = null
                firmwareVersionCombo.model = model
                firmwareVersionCombo.currentIndex = 1
                firmwareVersionCombo.currentIndex = 0

                controller.selectedFirmwareType = FirmwareUpgradeController.StableFirmware
            }

            function firmwareVersionComboxChanged(combox , model) {
                firmwareVersionWarningLabel.visible = false
                // All of this bizarre, setting model to null and index to 1 and then to 0 is to work around
                // strangeness in the combo box implementation. This sequence of steps correctly changes the combo model
                // without generating any warnings and correctly updates the combo text with the new selection.
                combox.model = null
                combox.model = model
                combox.currentIndex = 1
                combox.currentIndex = 0
            }

            QGCFlickable {
                width:      parent.width
                clip:               true
                anchors.fill:       parent
                contentHeight:      firmwareVersionInfo.height
                contentWidth:       firmwareVersionInfo.width

                Column {
                    id:                 firmwareVersionInfo
                    anchors.margins:    ScreenTools.defaultFontPixelWidth
                    width:              parent.width
                    spacing:            ScreenTools.defaultFontPixelHeight / 2

                    Row{
                        width:      parent.width
                        spacing:    ScreenTools.defaultFontPixelHeight
                        visible:    true
                        QGCLabel {
                            text:       px4Flow ? qsTr("Detected PX4 Flow board. You can select from the following firmware:") : qsTr("Detected Pixhawk board. You can select from the following flight stacks:")
                            width:      parent.width
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                    }

                    Row{
                        width:      parent.width
                        spacing:            ScreenTools.defaultFontPixelHeight
                        Rectangle {
                            height:     1
                            width:      parent.width
                            color:      __qgcPal.text
                            opacity:    0.15
                        }
                    }

                    Row {
                        width:      parent.width
                        spacing:        ScreenTools.defaultFontPixelWidth
                        QGCLabel {
                            text:       qsTr("Firwmare")
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                            //anchors.verticalCenter: parent.verticalCenter
                        }
                        QGCLabel {
                            text:       qsTr("Board Version")
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                            //anchors.verticalCenter: parent.verticalCenter
                        }
                        QGCLabel {
                            text:       qsTr("Latest Version")
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                            //anchors.verticalCenter: parent.verticalCenter
                        }
                        QGCLabel {
                            text:       qsTr(" ")
                            //width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                            //anchors.verticalCenter: parent.verticalCenter
                        }
                        QGCLabel {
                            text:       qsTr("Selected Version")
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                            //anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    Row{
                        width:      parent.width
                        visible:    true
                        Rectangle {
                            height:     1
                            width:      parent.width
                            color:      __qgcPal.text
                            opacity:    0.15
                        }
                    }


                    Row{
                        width:      parent.width
                        visible:    false
                        QGCLabel {
                            text:       qsTr("QGroundControl")
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    false
                        }
                        QGCLabel {
                            id :        qgcBoard
                            text:       controller.px4FlightStackBoard
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    false
                        }
                        QGCLabel {
                            id :        qgcNew
                            text:       controller.px4FlightStackNew
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCCheckBox {
                            id:             qgcStack
                            visible:        false
                            onClicked: {
                            }
                        }
                        QGCComboBox {
                            id:         qgcStackNewCombo
                            width:      parent.width * 3 /13
                            visible:    false
                            // model:      controller.qgroundControl
                        }

                    }
                    Row{
                        width:      parent.width
                        visible:    false
                        Rectangle {
                            height:     1
                            width:      parent.width
                            color:      __qgcPal.text
                            opacity:    0.15
                        }
                    }

                    // PX4 Flight Stack
                    Row{
                        width:      parent.width
                        QGCLabel {
                            text:       qsTr("PX4 Flight Stack")
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCLabel {
                            id :        px4FlightStackBoard
                            text:       controller.px4FlightStackBoard  //activeVehicle.firmwareMajorVersion == -1 ? qsTr("Unknown") : "V" + activeVehicle.firmwareMajorVersion + "." + activeVehicle.firmwareMinorVersion + "." + activeVehicle.firmwarePatchVersion
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCLabel {
                            id :        px4FlightStackNew
                            text:       controller.px4FlightStackNew
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCCheckBox {
                            id:             px4FlightStack
                            visible:        true
                            onClicked: {
                                if(px4FlightStack.checked == true){
                                    apmFlightStack.checked = false
                                }
                            }
                        }
                        QGCComboBox {
                            id:         px4FlightStackNewCombo
                            width:      parent.width * 3 /13
                            visible:    true
                            model:      {
                                var models = new Array
                                models = controller.px4FlightStackVersionsMap
                                if(models.length){
                                    px4FlightStackNewCombo.currentIndex = 0
                                }else{
                                    px4FlightStackNewCombo.currentIndex = -1
                                }
                                return models
                            }
                            MouseArea{
                                anchors.fill: parent
                                visible: {
                                    var str = controller.px4FlightStackVersionsMap[0]
                                    if(str)
                                        return (str.match("(CustomFirmware)|(.*\.(txt|bin|ihx|px4)$)") != null)
                                    return false
                                }
                                onClicked: {
                                    if(controller.androidType == false){
                                        if(controller.getLocalFile(FirmwareUpgradeController.Board - 1)){
                                            px4FlightStack.checked = true
                                            apmFlightStack.checked = false
                                        }
                                    }
                                    else{
                                        currentSelectedFirmware =  px4FlightStack
                                        apmFlightStack.checked = false
                                        currentSelected = FirmwareUpgradeController.Board - 1
                                        fileDialog.open()
                                    }
                                    mouse.accepted = false
                                }
                            }
                        }
                    }
                    Row{
                        width:      parent.width
                        Rectangle {
                            height:     1
                            width:      parent.width
                            color:      __qgcPal.text
                            opacity:    0.15
                        }
                    }

                    // ArduPilot Flight Stack
                    Row{
                        width:      parent.width
                        QGCLabel {
                            text:       qsTr("APM Flight Stack")
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCLabel {
                            id :        apmFlightStackBoard
                            text:       controller.apmFlightStackBoard
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCLabel {
                            id :        apmFlightStackNew
                            text:       controller.apmFlightStackNew
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCCheckBox {
                            id:             apmFlightStack
                            visible:        true
                            onClicked: {
                                if(apmFlightStack.checked == true){
                                    px4FlightStack.checked = false
                                }
                            }
                        }
                        QGCComboBox {
                            id:         apmFlightStackNewCombo
                            width:      parent.width * 3 /13
                            visible:    true
                            model:      {
                                var models = new Array
                                models =  controller.apmFlightStackVersionsMap
                                if(models.length){
                                    apmFlightStackNewCombo.currentIndex = 0
                                }else{
                                    apmFlightStackNewCombo.currentIndex = -1
                                }
                                return models
                            }
                            MouseArea{
                                anchors.fill: parent
                                visible: {
                                    var str = controller.apmFlightStackVersionsMap[0]
                                    if(str)
                                        return (str.match("(CustomFirmware)|(.*\.(txt|bin|ihx|px4)$)") != null)
                                    return false
                                }
                                onClicked: {
                                    if(controller.androidType == false){
                                        if(controller.getLocalFile(FirmwareUpgradeController.Board)){
                                            apmFlightStack.checked = true
                                            px4FlightStack.checked = false
                                        }
                                    }
                                    else{
                                        currentSelectedFirmware =  apmFlightStack
                                        px4FlightStack.checked = false
                                        currentSelected = FirmwareUpgradeController.Board
                                        fileDialog.open()
                                    }
                                    mouse.accepted = false
                                }
                            }
                        }
                    }
                    Row{
                        width:      parent.width
                        Rectangle {
                            height:     1
                            width:      parent.width
                            color:      __qgcPal.text
                            opacity:    0.15
                            visible:    true
                        }
                    }

                    // GPS
                    Row{
                        width:      parent.width
                        QGCLabel {
                            text:       qsTr("GPS")
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCLabel {
                            id :        gpsBoard
                            text:       controller.gpsBoard
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCLabel {
                            id :        gpsNew
                            text:       controller.gpsNew
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCCheckBox {
                            id:             gps
                            visible:        true
                            onClicked:{
                            }
                        }
                        QGCComboBox {
                            id:         gpsNewCombo
                            width:      parent.width * 3 /13
                            visible:    true
                            model:       {
                                var models = new Array
                                models = controller.gpsAvailableVersions
                                if(models.length){
                                    gpsNewCombo.currentIndex = 0
                                }else{
                                    gpsNewCombo.currentIndex = -1
                                }
                                return models
                            }
                            MouseArea{
                                anchors.fill: parent
                                visible: {
                                    var str = controller.gpsAvailableVersions[0]
                                    if(str)
                                        return (str.match("(CustomFirmware)|(.*\.(txt|bin|ihx|px4)$)") != null)
                                    return false
                                }
                                onClicked: {
                                    if(controller.androidType == false){
                                        if(controller.getLocalFile(FirmwareUpgradeController.GPS)){
                                            gps.checked = true
                                        }
                                    }
                                    else{
                                        currentSelectedFirmware =  gps
                                        currentSelected = FirmwareUpgradeController.GPS
                                        fileDialog.open()
                                    }
                                    mouse.accepted = false
                                }
                            }
                        }
                    }
                    Row{
                        width:      parent.width
                        Rectangle {
                            height:     1
                            width:      parent.width
                            color:      __qgcPal.text
                            opacity:    0.15
                        }
                    }

                    // Gimbals
                    Row{
                        width:      parent.width
                        QGCLabel {
                            text:       qsTr("Gimbals")
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCLabel {
                            id :        gimbalsBoard
                            text:       controller.gimbalsBoard
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCLabel {
                            id :        gimbalsNew
                            text:       controller.gimbalsNew
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCCheckBox {
                            id:             gimbals
                            visible:        true
                            onClicked: {
                            }
                        }
                        QGCComboBox {
                            id:         gimbalsNewCombo
                            width:      parent.width * 3 /13
                            visible:    true
                            model:      {
                                var models = new Array
                                models = controller.gimbalsVersions
                                if(models.length){
                                    gimbalsNewCombo.currentIndex = 0
                                }else{
                                    gimbalsNewCombo.currentIndex = -1
                                }
                                return models
                            }
                            MouseArea{
                                anchors.fill: parent
                                visible: {
                                    var str = controller.gimbalsVersions[0]
                                    if(str)
                                        return (str.match("(CustomFirmware)|(.*\.(txt|bin|ihx|px4)$)") != null)
                                    return false
                                }
                                onClicked: {
                                    if(controller.androidType == false){
                                        if(controller.getLocalFile(FirmwareUpgradeController.Gimbal)){
                                            gimbals.checked = true
                                        }
                                    }
                                    else{
                                        currentSelectedFirmware =  gimbals
                                        currentSelected = FirmwareUpgradeController.Gimbal
                                        fileDialog.open()
                                    }
                                    mouse.accepted = false
                                }
                            }
                        }
                    }
                    Row{
                        width:      parent.width
                        Rectangle {
                            height:     1
                            width:      parent.width
                            color:      __qgcPal.text
                            opacity:    0.15
                        }
                    }

                    // SmartBattery
                    Row{
                        width:      parent.width
                        QGCLabel {
                            text:       qsTr("SmartBattery")
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCLabel {
                            id :        smartBatteryBoard
                            text:       controller.smartBatteryBoard
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCLabel {
                            id :        smartBatteryNew
                            text:       controller.smartBatteryNew
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCCheckBox {
                            id:             smartBattery
                            visible:        true
                            onClicked:{
                            }
                        }
                        QGCComboBox {
                            id:         smartBatteryNewCombo
                            width:      parent.width * 3 /13
                            visible:    true
                            model:       {
                                var models = new Array
                                models = controller.smartBatteryVersions
                                if(models.length){
                                    smartBatteryNewCombo.currentIndex = 0
                                }else{
                                    smartBatteryNewCombo.currentIndex = -1
                                }
                                return models
                            }
                            MouseArea{
                                anchors.fill: parent
                                visible: {
                                    var str = controller.smartBatteryVersions[0]
                                    if(str)
                                        return (str.match("(CustomFirmware)|(.*\.(txt|bin|ihx|px4)$)") != null)
                                    return false
                                }
                                onClicked: {
                                    if(controller.androidType == false){
                                        if(controller.getLocalFile(FirmwareUpgradeController.Battery)){
                                            smartBattery.checked = true
                                        }
                                    }
                                    else{
                                        currentSelectedFirmware =  smartBattery
                                        currentSelected = FirmwareUpgradeController.Battery
                                        fileDialog.open()
                                    }
                                    mouse.accepted = false
                                }
                            }
                        }
                    }
                    Row{
                        width:      parent.width
                        Rectangle {
                            height:     1
                            width:      parent.width
                            color:      __qgcPal.text
                            opacity:    0.15
                        }
                    }

                    // SmartConsole
                    Row{
                        width:      parent.width
                        QGCLabel {
                            text:       qsTr("SmartConsole")
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCLabel {
                            id :        smartConsoleBoard
                            text:       controller.smartConsoleBoard
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCLabel {
                            id :        smartConsoleNew
                            text:       controller.smartConsoleNew
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCCheckBox {
                            id:             smartConsole
                            visible:        true

                            onClicked:{

                            }
                        }
                        QGCComboBox {
                            id:         smartConsoleNewCombo
                            width:      parent.width * 3 /13
                            visible:    true

                            model:      {
                                var models = new Array
                                models = controller.smartConsoleVersions
                                if(models.length){
                                    smartConsoleNewCombo.currentIndex = 0
                                }else{
                                    smartConsoleNewCombo.currentIndex = -1
                                }
                                return models
                            }
                            MouseArea{
                                anchors.fill: parent
                                visible: {
                                    var str = controller.smartConsoleVersions[0]
                                    if(str)
                                        return (str.match("(CustomFirmware)|(.*\.(txt|bin|ihx|px4)$)") != null)
                                    return false
                                }
                                onClicked: {
                                    if(controller.androidType == false){
                                        if(controller.getLocalFile(FirmwareUpgradeController.RemoteControl)){
                                            smartConsole.checked = true
                                        }
                                    }
                                    else{
                                        currentSelectedFirmware =  smartConsole
                                        currentSelected = FirmwareUpgradeController.RemoteControl
                                        fileDialog.open()
                                    }
                                    mouse.accepted = false
                                }
                            }
                        }
                    }
                    Row{
                        width:      parent.width
                        Rectangle {
                            height:     1
                            width:      parent.width
                            color:      __qgcPal.text
                            opacity:    0.15
                        }
                    }

                    // BrushlessFOC
                    Row{
                        width:      parent.width
                        QGCLabel {
                            text:       qsTr("BrushlessFOC")
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCLabel {
                            id :        brushlessFOCBoard
                            text:       controller.BrushlessFOCBoard
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCLabel {
                            id :        brushlessFOCNew
                            text:       controller.BrushlessFOCNew
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCCheckBox {
                            id:             brushlessFOC
                            visible:        true

                            onClicked:{
                            }
                        }
                        QGCComboBox {
                            id:         brushlessFOCNewCombo
                            width:      parent.width * 3 /13
                            visible:    true
                            model:      {
                                var models = new Array
                                models = controller.BrushlessFOCVersions
                                if(models.length){
                                    brushlessFOCNewCombo.currentIndex = 0
                                }else{
                                    brushlessFOCNewCombo.currentIndex = -1
                                }
                                return models
                            }
                            MouseArea{
                                anchors.fill: parent
                                visible: {
                                    var str = controller.BrushlessFOCVersions[0]
                                    if(str)
                                        return (str.match("(CustomFirmware)|(.*\.(txt|bin|ihx|px4)$)") != null)
                                    return false
                                }
                                onClicked: {
                                    if(controller.androidType == false){
                                        if(controller.getLocalFile(FirmwareUpgradeController.ESpeedControl)){
                                            brushlessFOC.checked = true
                                        }
                                    }
                                    else{
                                        currentSelectedFirmware =  brushlessFOC
                                        currentSelected = FirmwareUpgradeController.ESpeedControl
                                        fileDialog.open()
                                    }
                                    mouse.accepted = false
                                }
                            }
                        }
                    }
                    Row{
                        width:      parent.width
                        Rectangle {
                            height:     1
                            width:      parent.width
                            color:      __qgcPal.text
                            opacity:    0.15
                        }
                    }

                    // Raspberry
                    Row{
                        width:      parent.width
                        QGCLabel {
                            text:       qsTr("Raspberry")
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCLabel {
                            id :        raspberryBoard
                            text:       controller.RaspBerryBoard
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCLabel {
                            id :        raspberryNew
                            text:       controller.RaspBerryNew
                            width:      parent.width * 3 /13
                            wrapMode:   Text.WordWrap
                            visible:    true
                        }
                        QGCCheckBox {
                            id:             raspberry
                            visible:        true

                            onClicked:{

                            }
                        }
                        QGCComboBox {
                            id:         raspberryNewCombo
                            width:      parent.width * 3 /13
                            visible:    true
                            model:      {
                                var models = new Array
                                models = controller.RaspBerryVersions
                                if(models.length){
                                    raspberryNewCombo.currentIndex = 0
                                }else{
                                    raspberryNewCombo.currentIndex = -1
                                }
                                return models
                            }
                            MouseArea{
                                anchors.fill: parent
                                visible: {
                                    var str = controller.RaspBerryVersions[0]
                                    if(str)
                                        return (str.match("(CustomFirmware)|(.*\.(txt|bin|ihx|px4)$)") != null)
                                    return false
                                }
                                onClicked: {
                                    if(controller.androidType == false){
                                        if(controller.getLocalFile(FirmwareUpgradeController.RaspBerry)){
                                            raspberry.checked = true
                                        }
                                    }
                                    else{
                                        currentSelectedFirmware =  raspberry
                                        currentSelected = FirmwareUpgradeController.RaspBerry
                                        fileDialog.open()
                                    }
                                    mouse.accepted = false
                                }
                            }
                        }
                    }

                    Row{
                        width:      parent.width
                        Rectangle {
                            height:     1
                            width:      parent.width
                            color:      __qgcPal.text
                            opacity:    0.15
                        }
                    }
                    Row {
                        width:      parent.width
                        spacing:    ScreenTools.defaultFontPixelWidth / 2
                        visible:    true

                        Rectangle {
                            height: 1
                            width:      ScreenTools.defaultFontPixelWidth * 5
                            color:      qgcPal.text
                            anchors.verticalCenter: _advanced.verticalCenter
                        }
                        QGCLabel{
                            id:         _advanced
                            text:       qsTr("Advanced settings")
                        }

                        Rectangle {
                            height:     1
                            width:      ScreenTools.defaultFontPixelWidth * 5
                            color:      qgcPal.text
                            anchors.verticalCenter: _advanced.verticalCenter
                        }
                    }

                    Row{
                        width:      parent.width
                        spacing:    defaultTextWidth
                        visible:    true
                        QGCLabel {
                            width:      parent.width
                            wrapMode:   Text.WordWrap
                            visible:    true
                            text:       px4Flow ? qsTr("Select which version of the firmware you would like to install:") : qsTr("Select which version of the above flight stack you would like to install:")
                        }
                    }

                    Row{
                        id : qgcconboxfirw
                        width:      parent.width
                        spacing:    defaultTextWidth
                        visible:    true
                        QGCComboBox {
                            id:             firmwareVersionCombo
                            width:          200
                            visible:        true
                            model:          firmwareTypeList
                            currentIndex:   controller.selectedFirmwareType
                            onActivated: {
                                //controller.selectedFirmwareType = index
                                var firmwareSelectedType = new Array
                                for(var i = 0; i < selectFirmwareArray.length; i++){
                                    if(selectFirmwareArray[i].checked == false){
                                        firmwareSelectedType.push(firmwareSelectedTypeArray[i])
                                    }
                                }

                                controller.setSelectedFirmwareType(index, firmwareSelectedType)
                                if (model.get(index).firmwareType == FirmwareUpgradeController.BetaFirmware) {
                                    firmwareVersionWarningLabel.visible = true
                                    firmwareVersionWarningLabel.text = qsTr("WARNING: BETA FIRMWARE. ") +
                                            qsTr("This firmware version is ONLY intended for beta testers. ") +
                                            qsTr("Although it has received FLIGHT TESTING, it represents actively changed code. ") +
                                            qsTr("Do NOT use for normal operation.")
                                } else if (model.get(index).firmwareType == FirmwareUpgradeController.DeveloperFirmware) {
                                    firmwareVersionWarningLabel.visible = true
                                    firmwareVersionWarningLabel.text = qsTr("WARNING: CONTINUOUS BUILD FIRMWARE. ") +
                                            qsTr("This firmware has NOT BEEN FLIGHT TESTED. ") +
                                            qsTr("It is only intended for DEVELOPERS. ") +
                                            qsTr("Run bench tests without props first. ") +
                                            qsTr("Do NOT fly this without additonal safety precautions. ") +
                                            qsTr("Follow the mailing list actively when using it.")
                                } else {
                                    firmwareVersionWarningLabel.visible = false
                                }
                            }
                        }
                    }

                    Row{
                        width:      parent.width
                        spacing: defaultTextWidth
                        visible:    true
                        QGCLabel {
                            id:         firmwareVersionWarningLabel
                            width:      parent.width
                            wrapMode:   Text.WordWrap
                            visible:    false
                        }
                    }
                } // Column
            } // QGCFlickable
        } // QGCViewDialog
    } // Component - pixhawkFirmwareSelectDialog

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        QGCLabel {
            id:             titleLabel
            text:           title
            font.pointSize: ScreenTools.mediumFontPointSize
        }

        ProgressBar {
            id:                 progressBar
            anchors.topMargin:  ScreenTools.defaultFontPixelHeight
            anchors.top:        titleLabel.bottom
            width:              parent.width
        }

        TextArea {
            id:                 statusTextArea
            anchors.topMargin:  ScreenTools.defaultFontPixelHeight
            anchors.top:        progressBar.bottom
            anchors.bottom:     parent.bottom
            width:              parent.width
            readOnly:           true
            frameVisible:       false
            font.pointSize:     ScreenTools.defaultFontPointSize
            textFormat:         TextEdit.RichText
            text:               welcomeText

            style: TextAreaStyle {
                textColor:          qgcPal.text
                backgroundColor:    qgcPal.windowShade
            }
        }
    } // QGCViewPabel
} // QGCView
