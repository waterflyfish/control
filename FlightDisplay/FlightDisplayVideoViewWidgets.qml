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

    //-- video view widgets
    property var    _activeVehicle:             QGroundControl.multiVehicleManager.activeVehicle
    property bool   _isSatellite:               !_mainIsMap //? (_flightMap ? !_flightMap.isSatelliteMap : true) : true
    property bool   _lightWidgetBorders:        _isSatellite
    property bool   recording:                 false
    property bool   trackFlag:false
    property bool   yuntaiFlag:false
    readonly property real _toolButtonTopMargin:    parent.height - ScreenTools.availableHeight + (ScreenTools.defaultFontPixelHeight / 2)

    QGCMapPalette { id: mapPal; lightColors: isBackgroundDark }

    ExclusiveGroup {
        id: _dropButtonsExclusiveGroup
    }

    ExclusiveGroup {
        id: _mapTypeButtonsExclusiveGroup
    }
    Tx1Controller{
          id:tx1Controller
        }
    QGCLabel {
        id:         videoLabel
        text:       qsTr("Video")
        color:      mapPal.text
        visible:    !ScreenTools.isShortScreen && !_mainIsMap
        anchors.topMargin:  _toolButtonTopMargin
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.leftMargin: ScreenTools.defaultFontPixelHeight
    }

    //-- Vertical Tool Buttons
    Column {
        id:                 toolColumn
        anchors.top:        ScreenTools.isShortScreen ? parent.top : videoLabel.bottom
        anchors.topMargin:  ScreenTools.isShortScreen ? _toolButtonTopMargin : ScreenTools.defaultFontPixelHeight / 2
        anchors.left:       parent.left
        anchors.leftMargin: ScreenTools.defaultFontPixelHeight
        spacing:            ScreenTools.defaultFontPixelHeight * 1.5
        visible:            !_mainIsMap

        //-- Video Control
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
                        text:       tx1Controller.tx1RecordRunning ? qsTr("Stop Record") : qsTr("Start Record")
                        enabled:    QGroundControl.videoManager.videoRunning
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: {
                            _dropButtonsExclusiveGroup.current = null
                            if(recordButton.text == qsTr("Start Record")){
                                tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeRecord, 1)
                            }else if(recordButton.text == qsTr("Stop Record")){
                                tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeRecord, 0)
                            }
                        }
                    }
                    QGCButton {
                        id:         pictureButton
                        text:       qsTr("Take Picture")
                        enabled:    QGroundControl.videoManager.videoRunning
                        onClicked: {
                            _dropButtonsExclusiveGroup.current = null
                            tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeScreen, 1)
                        }
                    }

                    QGCButton {
                        id:         trackedButton
                        text:       qsTr("Stop Tracked")
                        enabled:    tx1Controller.tx1TrackRunning
                        onClicked: {
                            _dropButtonsExclusiveGroup.current = null
                            tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 0);
                        }
                    }
                }
            }
        }

//        //-- Local video control
//        QGCLabel{
//            text:       recording ? qsTr("Stop Recording") : qsTr("Start Recording")
//            color:      videoPal.text
//            visible:    !ScreenTools.isShortScreen && !_mainIsMap
//        }
        DropButton{
            id:            localRecord
            visible:       !_mainIsMap && QGroundControl.videoManager.videoReceiver.running
//            enabled :       false
            buttonImage:   "qrc:/camera/telecamera.svg"
            z:             QGroundControl.zOrderWidgets
            onClicked:{
                if(recording){
                    recording = false
                    QGroundControl.videoManager.saveVideo = false
                }
                else {
                    recording = true
                    QGroundControl.videoManager.saveVideo = true
                }
//                checked = false
            }
        }
//        QGCLabel{
//            text:       qsTr("Start Capturing")
//            color:      videoPal.text
//            visible:    !ScreenTools.isShortScreen && !_mainIsMap
//        }
        DropButton{
            id:             localCapture
            visible:        !_mainIsMap && QGroundControl.videoManager.videoReceiver.running
            buttonImage :   "qrc:/camera/camera.svg"
            z:              QGroundControl.zOrderWidgets
            onClicked:{
//                if(!QGroundControl.videoManager.videoRunning){
//                    checked = false
//                    return
//                }
                console.log("aaaaaaaaaaaaaaaaaaa")
                QGroundControl.videoManager.saveImage = true
                localCaptureTimer.start()
                localCapture.enabled=false

                //tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
                                      //          -200, 0
                                          //      , 0, 0)
            }
        }
        QGCLabel{
            id:track
            text:       qsTr("Tracking Control")
            color:      videoPal.text
            visible:    !ScreenTools.isShortScreen && !_mainIsMap
        }
        DropButton{
            id:             tracking
            dropDirection:      dropRight
          //  buttonImage:        "/qmlimages/ZoomPluCam.svg"
            viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
            exclusiveGroup:     _dropButtonsExclusiveGroup
            lightBorders:       _lightWidgetBorders
            visible:        !_mainIsMap
            z:              QGroundControl.zOrderWidgets
            dropDownComponent: Component {
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth
                    QGCButton {
                        id:         trackOn
                        text:       qsTr("Teacking On")
                        enabled:    true//QGroundControl.videoManager.videoRunning
                        onClicked: {
                            QGroundControl.videoManager.trackFlag=true
                        }
                    }
                    QGCButton {
                        id:         trackOff
                        text:       qsTr("Teacking Off")
                        enabled:    true//QGroundControl.videoManager.videoRunning
                        onClicked: {
                            QGroundControl.videoManager.trackFlag=false
                        }
                    }
                }

            }

//            onClicked:{

//                if(!trackFlag){
//                tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
//                                                -100, 0
//                                                , 0, 0)
//                     track.text="Tracking On"
//                    console.log("Start Tracking")
//                    checked=true
//                    trackFlag=true
//                 } else{
//                    console.log("Close Tracking")
//                    checked=false
//                    track.text="Tracking OFF"
//                    trackFlag=false
//                    tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
//                                                    -101, 0
//                                                    , 0, 0)
//                }


//            }
        }
        QGCLabel{
            //id:yuntai
            text:       qsTr("ChangeVideo")
            color:      videoPal.text
            visible:    !ScreenTools.isShortScreen && !_mainIsMap
        }
        DropButton{
            id:             yuntai
            visible:        !_mainIsMap
            z:              QGroundControl.zOrderWidgets
            onClicked:{

               // if(!yuntaiFlag){
                tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
                                                -300, 0
                                                , 0, 0)
                if(!QGroundControl.videoManager.videoChoice){
                    QGroundControl.videoManager.videoChoice=true
                }else{
                    QGroundControl.videoManager.videoChoice=false
                }

                   // yuntaiFlag=true
                 //} else{

                   // yuntaiFlag=false
                   // tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
                                            //        -301, 0
                                             //       , 0, 0)



           // }
        }
        }
        QGCLabel{
            //id:yuntai
            text:       qsTr("CarInfo")
            color:      videoPal.text
            visible:    !ScreenTools.isShortScreen && !_mainIsMap
        }
        DropButton{
            id:             yun
            visible:        !_mainIsMap
            z:              QGroundControl.zOrderWidgets
            onClicked:{
                 QGroundControl.videoManager.carInfo = true
//                if(!yuntaiFlag){
//                    tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
//                                                    -301, 0
//                                                    , 0, 0)


//                        yuntaiFlag=true
//                     } else{

//                        yuntaiFlag=false
//                        tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
//                                                        -302, 0
//                                                        , 0, 0)




//            }

        }
        }
        Timer{
            id:       localCaptureTimer
            interval: 200
            running:  false
            repeat:   false
            onTriggered: {
                QGroundControl.videoManager.saveImage = false
                localCaptureTimer.stop()
                localCapture.checked = false
                localCapture.enabled=true
            }
        }
    }
}
