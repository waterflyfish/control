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
import QGroundControl.FactSystem    1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactControls  1.0

Item {
    id: _root

    property alias  guidedModeBar:  _guidedModeBar
    property bool   gotoEnabled:    _activeVehicle && _activeVehicle.guidedMode && _activeVehicle.flying

    property var    _activeVehicle:             QGroundControl.multiVehicleManager.activeVehicle
    property var    _groundSpeed :              _activeVehicle ? _activeVehicle.groundSpeed.value : 0.0
    property var    _altitudeRelative :         _activeVehicle ? _activeVehicle.altitudeRelative.value : 0.0
    property var    _altitude :                 _activeVehicle ? _activeVehicle.altitudeAMSL.value : 0.0
    property var    _climRate :                 _activeVehicle ? _activeVehicle.climbRate.value : 0.0
    property var    _pitch :                    _activeVehicle ? _activeVehicle.pitch.value : 0.0
    property var    _roll :                      _activeVehicle ? _activeVehicle.roll.value : 0.0
    property bool   _isSatellite:               _mainIsMap ? (_flightMap ? _flightMap.isSatelliteMap : true) : true
    property bool   _lightWidgetBorders:        _isSatellite
    property bool   _useAlternateInstruments:   QGroundControl.virtualTabletJoystick && (_activeVehicle && _activeVehicle.isPosCtlMode) || ScreenTools.isTinyScreen
    property bool   tabDis:                     true
     property bool   imgDis:                     false
    property var    missionController :         _flightMap.missionController
    property var    _currentMissionItem
    property int    _currentMissionIndex

    property real   _labelWidth : ScreenTools.isMobile ? ScreenTools.defaultFontPixelWidth * 12 : ScreenTools.defaultFontPixelWidth * 17
    property real   _editFieldWidth: ScreenTools.isMobile ? ScreenTools.defaultFontPixelWidth * 10 : ScreenTools.defaultFontPixelWidth * 15
    property Fact   _emptyFact:         Fact { }
    property bool   _fullParameterVehicleAvailable: QGroundControl.multiVehicleManager.parameterReadyVehicleAvailable && !QGroundControl.multiVehicleManager.activeVehicle.parameterManager.missingParameters
    property Fact   _triggerModeFact :  _fullParameterVehicleAvailable ? _activeVehicle.getParameterFact(-1, "TRIG_MODE") : _emptyFact
    readonly property real _margins:                ScreenTools.defaultFontPixelHeight / 3
    readonly property real _toolButtonTopMargin:    parent.height - ScreenTools.availableHeight + (ScreenTools.defaultFontPixelHeight / 2)
    property var    _vehicletype
    property int totalPoints:0
    property int trackingPoints:0
    property int trackingActivePoints:0
    property int proceNum:0
    property int protimenum:0
    property int bitrate:0
    property int iframe:0
    property int pframe:0
    QGCMapPalette { id: mapPal; lightColors: isBackgroundDark }
    QGCPalette { id: qgcPal }
    ParameterEditorController {
        id:         controller
       // factPanel:  panel

    }
    FileDialog {
        id: fileDialog
        title: "Choose a folder with some images"
       // folder:shortcuts.home
        onAccepted: {
            process.start()
            proceNum=0;
            QGroundControl.videoManager.updateInf=fileUrl

           // console.log("AAAAAA="+fileUrl + "/")
        }
    }
    function process(){
        console.log("proceNum=="+proceNum)
        proceNum=QGroundControl.videoManager.getProcess()*100
        if(proceNum<100)protimenum=0
        if(proceNum>100)proceNum=0
        if(proceNum==100) {
                    protimenum++
                    if(protimenum==5){
                        process.stop()
                        proceNum=0
                    }


        }
    }

    Timer{
        id:process
        interval:1000
        running:false
        repeat:true
        onTriggered: _root.process()
    }

    Tx1Controller{
        id : tx1Controller
        onTx1StatusTrackedPossibleTargetChanged:{
             if(rb_x==-100){
                 totalPoints=tl_x
                trackingPoints=tl_y
                console.log("x=="+totalPoints)
             }


        }
        onTx1StatusTrackedTargetChanged:{

                trackingActivePoints=c_y
                //console.log("x=="+totalPoints)



        }
    }
    function setCurrentItem(sequenceNumber) {
        _currentMissionItem = undefined
        for (var i=0; i<missionController.visualItems.count; i++) {
            var visualItem = missionController.visualItems.get(i)
            if (visualItem.sequenceNumber == sequenceNumber) {
                _currentMissionItem = visualItem
                _currentMissionItem.isCurrentItem = true
                _currentMissionIndex = i
           } else {
                visualItem.isCurrentItem = false
            }
        }
    }
    function vehicletype(){
        if(!_activeVehicle || _activeVehicle.vtol){
            _vehicletype = vtol
        }else{
            if(_activeVehicle.fixedWing)
                _vehicletype = plane
            if(_activeVehicle.multiRotor)
                _vehicletype = copter
        }
        return _vehicletype
    }
    Component.onCompleted: {
        vehicletype()
    }
    on_ActiveVehicleChanged: vehicletype()
    function getGadgetWidth() {
        if(ScreenTools.isMobile) {
            return ScreenTools.isTinyScreen ? mainWindow.width * 0.2 : mainWindow.width * 0.15
        }
        var w = mainWindow.width * 0.15
        return Math.min(w, 200)
    }
    ExclusiveGroup {
        id: _dropButtonsExclusiveGroup
    }

    ExclusiveGroup {
        id: _mapTypeButtonsExclusiveGroup
    }

    //-- Map warnings
    Column {
        anchors.horizontalCenter:   parent.horizontalCenter
        anchors.verticalCenter:     parent.verticalCenter
        spacing:                    ScreenTools.defaultFontPixelHeight

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            visible:                    _activeVehicle && !_activeVehicle.coordinateValid && _mainIsMap
            z:                          QGroundControl.zOrderTopMost
            color:                      mapPal.text
            font.pointSize:             ScreenTools.largeFontPointSize
            text:                       qsTr("No GPS Lock for Vehicle")
        }

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            visible:                    _activeVehicle && _activeVehicle.prearmError
            z:                          QGroundControl.zOrderTopMost
            color:                      mapPal.text
            font.pointSize:             ScreenTools.largeFontPointSize
            text:                       _activeVehicle ? _activeVehicle.prearmError : ""
        }
    }

    //-- Dismiss Drop Down (if any)
    MouseArea {
        anchors.fill:   parent
        enabled:        _dropButtonsExclusiveGroup.current != null
        onClicked: {
            if(_dropButtonsExclusiveGroup.current)
                _dropButtonsExclusiveGroup.current.checked = false
            _dropButtonsExclusiveGroup.current = null
        }
    }
    TabView{
      id : tabview
      anchors.top : parent.top
      anchors.right: altitudeSlider.visible ? altitudeSlider.left : parent.right
      anchors.bottom: parent.bottom
      width : ScreenTools.isTinyScreen ? getGadgetWidth() * 1.5 : getGadgetWidth()*1.8
      visible: !_useAlternateInstruments

      Tab{
          title: qsTr("General")
          QGCViewPanel{
             id : flightviewpanel
             anchors.fill: parent
             color :      qgcPal.window
             QGCFlickable{
                clip : true
                anchors.fill : parent
                contentHeight : flyDataColumn.height
                contentWidth : flyDataColumn.width
                Column{
                   id : flyDataColumn
                   width : flightviewpanel.width
                   spacing : ScreenTools.defaultFontPixelHeight * 0.5
                   anchors.margins : ScreenTools.defaultFontPixelWidth
                   //-------------------------------------------------
                   Rectangle{
                       width : parent.width*0.8;height: 3
                       color : qgcPal.window
                       anchors.horizontalCenter: parent.horizontalCenter
                   }
                   Item {
                       width:              parent.width * 0.9
                       height:             unitLabel.height
                       anchors.margins:    ScreenTools.defaultFontPixelWidth *0.5
                       anchors.horizontalCenter: parent.horizontalCenter
                       QGCLabel {
                           id:             unitLabel
                           text:           qsTr("Fly Data")
                           font.family:    ScreenTools.demiboldFontFamily
                       }
                   }
                   Rectangle{
                       id : flyData
                       width : parent.width * 0.9
                       height : unitsCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                       color : qgcPal.windowShade
                       anchors.margins: ScreenTools.defaultFontPixelWidth *0.5
                       anchors.horizontalCenter: parent.horizontalCenter
                       radius : ScreenTools.defaultFontPixelWidth
                       border.color: "lightsteelblue"
                       Column{
                           id : unitsCol
                           spacing : ScreenTools.defaultFontPixelWidth
                           anchors.centerIn: parent
                           Row{
                               spacing : ScreenTools.defaultFontPixelWidth * 2
                               QGCLabel{
                                   text : qsTr("Ground Speed : ")
                                   width : _labelWidth
   //                                horizontalAlignment : Text.AlignHCenter
                                   font.family:  ScreenTools.normalFontFamily

                               }
                               QGCLabel{
                                   width : _editFieldWidth
                                   horizontalAlignment : Text.AlignHCenter
                                   font.family:   ScreenTools.normalFontFamily
                                   text : _groundSpeed.toFixed(1) + "m/s"
                               }
                           }

                           Row{
                               spacing : ScreenTools.defaultFontPixelWidth * 2
                               QGCLabel{
                                   text : qsTr("Altitude-rel : ")
                                   width : _labelWidth
   //                                horizontalAlignment : Text.AlignHCenter
                                   font.family:  ScreenTools.normalFontFamily

                               }
                               QGCLabel{
                                   width : _editFieldWidth
                                   horizontalAlignment : Text.AlignHCenter
                                   font.family:   ScreenTools.normalFontFamily
                                   text : _altitudeRelative.toFixed(1) + "m"
                               }
                           }
                           Row{
                               spacing : ScreenTools.defaultFontPixelWidth * 2
                               QGCLabel{
                                   text : qsTr("Altitude : ")
                                   width : _labelWidth
   //                                horizontalAlignment : Text.AlignHCenter
                                   font.family:  ScreenTools.normalFontFamily

                               }
                               QGCLabel{
                                   width : _editFieldWidth
                                   horizontalAlignment : Text.AlignHCenter
                                   font.family:   ScreenTools.normalFontFamily
                                   text : _altitude.toFixed(1) + "m"
                               }
                           }
                           Row{
                               spacing : ScreenTools.defaultFontPixelWidth * 2
                               QGCLabel{
                                   text : qsTr("ClimRate : ")
                                   width : _labelWidth
   //                                horizontalAlignment : Text.AlignHCenter
                                   font.family:  ScreenTools.normalFontFamily

                               }
                               QGCLabel{
                                   width : _editFieldWidth
                                   horizontalAlignment : Text.AlignHCenter
                                   font.family:   ScreenTools.normalFontFamily
                                   text : _climRate.toFixed(1) + "m/s"
                               }
                           }
                           Row{
                                spacing : ScreenTools.defaultFontPixelWidth * 2
                                QGCLabel{
                                    text : qsTr("Pitch :")
                                    width : _labelWidth
                                    font.family: ScreenTools.normalFontFamily
                                }
                                QGCLabel{
                                    width : _editFieldWidth
                                    horizontalAlignment : Text.AlignHCenter
                                    font.family : ScreenTools.normalFontFamily
                                    text : _pitch.toFixed(1) + "deg"
                                }
                           }
                           Row{
                                spacing : ScreenTools.defaultFontPixelWidth * 2
                                QGCLabel{
                                    text : qsTr("Roll :")
                                    width : _labelWidth
                                    font.family: ScreenTools.normalFontFamily
                                }
                                QGCLabel{
                                    width : _editFieldWidth
                                    horizontalAlignment : Text.AlignHCenter
                                    font.family : ScreenTools.normalFontFamily
                                    text : _roll.toFixed(1) + "deg"
                                }
                           }
                       }
                   }
                   Item {
                       width:              parent.width * 0.9
                       height:             attitudeLabel.height
                       anchors.margins:    ScreenTools.defaultFontPixelWidth *0.5
                       anchors.horizontalCenter: parent.horizontalCenter
                       QGCLabel {
                           id:             attitudeLabel
                           text:           qsTr("Attitude ")
                           font.family:    ScreenTools.demiboldFontFamily
                       }
                   }
                   Rectangle{
                       id : attitude
                       width : parent.width * 0.9
                       height : instrumentGadget.height + (ScreenTools.defaultFontPixelHeight * 2)
                       color : qgcPal.windowShade
                       anchors.margins: ScreenTools.defaultFontPixelWidth *0.5
                       anchors.horizontalCenter: parent.horizontalCenter
                       radius : ScreenTools.defaultFontPixelWidth
                       border.color: "lightsteelblue"
                       QGCInstrumentWidgetAlternate {
                           id:    instrumentGadget
                           width :  parent.width * 0.9
                           anchors.margins: ScreenTools.defaultFontPixelWidth/2
                           anchors.horizontalCenter: parent.horizontalCenter
                           anchors.verticalCenter: parent.verticalCenter
                           visible:                true
                           active:                 _activeVehicle != null
                           heading:                _heading
                           rollAngle:              _roll
                           pitchAngle:             _pitch
                           groundSpeedFact:        _groundSpeedFact
                           airSpeedFact:           _airSpeedFact
                           isSatellite:            _isSatellite
                           z:                      QGroundControl.zOrderWidgets
                       }
                   }
                   Item {
                       width:              parent.width * 0.9
                       height:             currentMissionLabel.height
                       anchors.margins:    ScreenTools.defaultFontPixelWidth * 0.5
                       anchors.horizontalCenter: parent.horizontalCenter
                       QGCLabel {
                           id:             currentMissionLabel
                           text:           qsTr("CurrentMission ")
                           font.family:    ScreenTools.demiboldFontFamily
                       }
                   }
                   Rectangle{
                       id : currentMission
                       width : parent.width * 0.9
                       height :  ScreenTools.defaultFontPixelHeight * 12
                       color : qgcPal.windowShade
                       anchors.margins: ScreenTools.defaultFontPixelWidth * 0.5
                       anchors.horizontalCenter: parent.horizontalCenter
                       radius : ScreenTools.defaultFontPixelWidth
                       border.color: "lightsteelblue"
                       ListView {
                           id:             missionItemEditorListView
                           anchors.left:   parent.left
                           anchors.right:  parent.right
                           anchors.top:    parent.top
                           height:         parent.height
                           anchors.margins: ScreenTools.defaultFontPixelWidth
                           anchors.verticalCenter: parent.verticalCenter
                           spacing:        ScreenTools.defaultFontPixelWidth
                           orientation:    ListView.Vertical
                           model:          missionController.visualItems
                           cacheBuffer:    height * 2 > 0 ? height * 2 : height * -2
                           clip:           true
                           currentIndex:   _currentMissionIndex
                           highlightMoveDuration: 250
                           delegate: MissionItemEditorAdd {
                               missionItem:    object
                               width:          parent.width
                               readOnly:       false
                               onClicked:  setCurrentItem(object.sequenceNumber)
                               onSendMissionItem:{
                                   missionController.sendMissionItem(index);
                               }
                           }
                       } // ListView
                   }
                }
             }
          }
      }
      Tab{
        title: qsTr("Setup")
        Loader{
            id : loader
            anchors.fill: parent
            sourceComponent: _vehicletype
        }
      }
      Tab{
        title : qsTr("Camera")
        Loader{
            id : cameraloader
            anchors.fill: parent
            sourceComponent: cameraComponent
        }
      }
      Tab{
        title : qsTr("Tracking")
        Loader{
            id : tracking
            anchors.fill: parent
            QGCViewPanel{
               id : flightviewpanel1
               anchors.fill: parent
               color :      qgcPal.window
               QGCFlickable{
                  clip : true
                  anchors.fill : parent
                  contentHeight : flyDataColumn1.height
                  contentWidth : flyDataColumn1.width
                  Column{
                     id : flyDataColumn1
                     width : flightviewpanel1.width
                     spacing : ScreenTools.defaultFontPixelHeight * 0.5
                     anchors.margins : ScreenTools.defaultFontPixelWidth
                     //-------------------------------------------------
                     Rectangle{
                         width : parent.width*0.8;height: 3
                         color : qgcPal.window
                         anchors.horizontalCenter: parent.horizontalCenter
                     }
                     Item {
                         width:              parent.width * 0.9
                         height:             unitLabel1.height
                         anchors.margins:    ScreenTools.defaultFontPixelWidth *0.5
                         anchors.horizontalCenter: parent.horizontalCenter
                         QGCLabel {
                             id:             unitLabel1
                             text:           qsTr("tracking Data")
                             font.family:    ScreenTools.demiboldFontFamily
                         }
                     }
                     Rectangle{
                         id : flyData1
                         width : parent.width * 0.9
                         height : unitsCol1.height + (ScreenTools.defaultFontPixelHeight * 2)
                         color : qgcPal.windowShade
                         anchors.margins: ScreenTools.defaultFontPixelWidth *0.5
                         anchors.horizontalCenter: parent.horizontalCenter
                         radius : ScreenTools.defaultFontPixelWidth
                         border.color: "lightsteelblue"
                         Column{
                             id : unitsCol1
                             spacing : ScreenTools.defaultFontPixelWidth
                             anchors.centerIn: parent
                             Row{
                                 spacing : ScreenTools.defaultFontPixelWidth * 2
                                 QGCLabel{
                                     text : qsTr("total feature points : ")
                                     width : _labelWidth
     //                                horizontalAlignment : Text.AlignHCenter
                                     font.family:  ScreenTools.normalFontFamily

                                 }
                                 QGCLabel{
                                     width : _editFieldWidth
                                     horizontalAlignment : Text.AlignHCenter
                                     font.family:   ScreenTools.normalFontFamily
                                     text : totalPoints+""
                                 }
                             }

                             Row{
                                 spacing : ScreenTools.defaultFontPixelWidth * 2
                                 QGCLabel{
                                     text : qsTr("object feature points : ")
                                     width : _labelWidth
     //                                horizontalAlignment : Text.AlignHCenter
                                     font.family:  ScreenTools.normalFontFamily

                                 }
                                 QGCLabel{
                                     width : _editFieldWidth
                                     horizontalAlignment : Text.AlignHCenter
                                     font.family:   ScreenTools.normalFontFamily
                                     text : trackingPoints + ""
                                 }
                             }
                             Row{
                                 spacing : ScreenTools.defaultFontPixelWidth * 2
                                 QGCLabel{
                                     text : qsTr("object tracking points : ")
                                     width : _labelWidth
     //                                horizontalAlignment : Text.AlignHCenter
                                     font.family:  ScreenTools.normalFontFamily

                                 }
                                 QGCLabel{
                                     width : _editFieldWidth
                                     horizontalAlignment : Text.AlignHCenter
                                     font.family:   ScreenTools.normalFontFamily
                                     text : trackingActivePoints + ""
                                 }
                             }
                             Row{
                                 spacing : ScreenTools.defaultFontPixelWidth
                                 QGCLabel{
                                    // anchors.baseline: timeInterval.baseline
                                     width : _labelWidth
                                     text : qsTr("Image Quality :")
                                 }
                                TextField{
                                     id : gauss
                                     width : _editFieldWidth
                                     onAccepted: {

                                         var m = parseInt(gauss.getText(0,1))

                                         if(m<=3)m=3
                                         else if(3<m&&m<=5)m=5
                                         else if(5<m&&m<=7)m=7
                                         else m=9
                                         tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
                                                                         -400, m
                                                                         , 0, 0)
                                     }
                                 }
                             }
                             Row{
                                 spacing : ScreenTools.defaultFontPixelWidth
                                 QGCLabel{
                                    // anchors.baseline: timeInterval.baseline
                                     width : _labelWidth
                                     text : qsTr("Position Filter :")
                                 }
                                TextField{
                                     id : filter
                                     width : _editFieldWidth
                                     onAccepted: {

                                         var m = parseFloat(filter.getText(0,6))
                                        //console.log("m===="+m)
                                         if(m>=1)m=1
                                         if(m<=0)m=0
                                          m=m*10000
                                         tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
                                                                         -401, m
                                                                         , 0, 0)
                                     }
                                 }
                             }
                             Row{
                                 spacing : ScreenTools.defaultFontPixelWidth
                                 QGCLabel{
                                    // anchors.baseline: timeInterval.baseline
                                     width : _labelWidth
                                     text : qsTr("Size Filter :")
                                 }
                                TextField{
                                     id : filter1
                                     width : _editFieldWidth
                                     onAccepted: {

                                         var m = parseFloat(filter1.getText(0,6))
                                        //console.log("m===="+m)
                                         if(m>=1)m=1
                                         if(m<=0)m=0
                                          m=m*10000
                                         tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
                                                                         -402, m
                                                                         , 0, 0)
                                     }
                                 }
                             }
                             Row{
                                 spacing : ScreenTools.defaultFontPixelWidth
                                 QGCLabel{
                                    // anchors.baseline: timeInterval.baseline
                                     width : _labelWidth
                                     text : qsTr("PITCH_I :")
                                 }
                                 FactTextField{
                                         id:            pidi
                                         fact:          _fullParameterVehicleAvailable  ? _activeVehicle.getParameterFact(-1,  "MNT_PITCH_I") : _emptyFact
                                         showUnits:     true
                                         width:         _editFieldWidth

                                 }
//
                             }
                             Row{
                                 spacing : ScreenTools.defaultFontPixelWidth
                                 QGCLabel{
                                    // anchors.baseline: timeInterval.baseline
                                     width : _labelWidth
                                     text : qsTr("PITCH_P :")
                                 }
                                 FactTextField{
                                         id:            pidp
                                         fact:          _fullParameterVehicleAvailable  ? _activeVehicle.getParameterFact(-1,  "MNT_PITCH_P") : _emptyFact
                                         showUnits:     true
                                         width:         _editFieldWidth

                                 }
//
                             }
                             Row{
                                 spacing : ScreenTools.defaultFontPixelWidth
                                 QGCLabel{
                                    // anchors.baseline: timeInterval.baseline
                                     width : _labelWidth
                                     text : qsTr("YAW_I :")
                                 }
                                 FactTextField{
                                         id:            yawi
                                         fact:          _fullParameterVehicleAvailable  ? _activeVehicle.getParameterFact(-1,  "MNT_YAW_I") : _emptyFact
                                         showUnits:     true
                                         width:         _editFieldWidth

                                 }
//
                             }
                             Row{
                                 spacing : ScreenTools.defaultFontPixelWidth
                                 QGCLabel{
                                    // anchors.baseline: timeInterval.baseline
                                     width : _labelWidth
                                     text : qsTr("YAW_P :")
                                 }
                                 FactTextField{
                                         id:            yawp
                                         fact:          _fullParameterVehicleAvailable  ? _activeVehicle.getParameterFact(-1,  "MNT_YAW_P") : _emptyFact
                                         showUnits:     true
                                         width:         _editFieldWidth

                                 }
//
                             }

                         }
                     }
                     Rectangle{
                         width : parent.width*0.8;height: 3
                         color : qgcPal.window
                         anchors.horizontalCenter: parent.horizontalCenter
                     }
                     Item {
                         width:              parent.width * 0.9
                         height:             unitLabel111.height
                         anchors.margins:    ScreenTools.defaultFontPixelWidth *0.5
                         anchors.horizontalCenter: parent.horizontalCenter
                         QGCLabel {
                             id:             unitLabel111
                             text:           qsTr("Update Info")
                             font.family:    ScreenTools.demiboldFontFamily
                         }
                     }
                     Rectangle{
                         id : flyData111
                         width : parent.width * 0.9
                         height : unitsCol111.height + (ScreenTools.defaultFontPixelHeight * 2)
                         color : qgcPal.windowShade
                         anchors.margins: ScreenTools.defaultFontPixelWidth *0.5
                         anchors.horizontalCenter: parent.horizontalCenter
                         radius : ScreenTools.defaultFontPixelWidth
                         border.color: "lightsteelblue"
                         Column{
                             id : unitsCol111
                             spacing : ScreenTools.defaultFontPixelWidth
                            anchors.centerIn: parent
                             Row{
                                 spacing : ScreenTools.defaultFontPixelWidth * 2
//                                 Slider {
//                                     id: proce


//                                     maximumValue: 100
//                                     value:proceNum
//                                     property bool sync: false

//                                     onValueChanged: {

//                                     }


//                                 }
                                 Rectangle{
                                     width:_labelWidth*1.5
                                     height:25
                                     radius: 5
                                     border.width: 2
                                     border.color: "white"
                                     color:"black"
                                     Rectangle{

                                         anchors.top: parent.top
                                         anchors.bottom: parent.bottom
                                         anchors.left: parent.left
                                        // anchors.right: parent.right


                                         width:1.5*(_labelWidth-4)*proceNum/100
                                         anchors.margins: 3
                                         height:20
                                         radius:5
                                         color:"green"

                                     }

                                 }

                                 Button{
                                     text:"update"
                                     width:_labelWidth/2
                                     onClicked: {
                                         fileDialog.open()
                                     }
                                 }


                             }

}

                         }
                     Rectangle{
                         width : parent.width*0.8;height: 3
                         color : qgcPal.window
                         anchors.horizontalCenter: parent.horizontalCenter
                     }
                     Item {
                         width:              parent.width * 0.9
                         height:             unitLabel11.height
                         anchors.margins:    ScreenTools.defaultFontPixelWidth *0.5
                         anchors.horizontalCenter: parent.horizontalCenter
                         QGCLabel {
                             id:             unitLabel11
                             text:           qsTr("Car Info")
                             font.family:    ScreenTools.demiboldFontFamily
                         }
                     }
                     Rectangle{
                         id : flyData11
                         width : parent.width * 0.9
                         height : unitsCol11.height + (ScreenTools.defaultFontPixelHeight * 2)
                         color : qgcPal.windowShade
                         anchors.margins: ScreenTools.defaultFontPixelWidth *0.5
                         anchors.horizontalCenter: parent.horizontalCenter
                         radius : ScreenTools.defaultFontPixelWidth
                         border.color: "lightsteelblue"
                         Column{
                             id : unitsCol11
                             spacing : ScreenTools.defaultFontPixelWidth
                            // anchors.centerIn: parent
                             Row{
                                 spacing : ScreenTools.defaultFontPixelWidth * 2
                                 QGCLabel{
                                     id:carinformation
                                     text : QGroundControl.videoManager.inf
                                     width : _labelWidth
     //                                horizontalAlignment : Text.AlignHCenter
                                     font.family:  ScreenTools.normalFontFamily

                                 }

                             }



                         }
                     }



                  }
               }
            }
        }
      }
      style: TabViewStyle{
        frameOverlap: 1
        tab:Rectangle{
            color : qgcPal.window
            border.color: styleData.selected ? qgcPal.buttonHighlight : "lightsteelblue"
            implicitWidth: ScreenTools.isTinyScreen ? getGadgetWidth() * 1.5/3 : getGadgetWidth()*2/4
            implicitHeight: ScreenTools.defaultFontPixelHeight * 2.5
            radius:  ScreenTools.defaultFontPixelHeight/1.5
            Text{
                id: text
                anchors.centerIn: parent
                text : styleData.title
                font.family: ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.defaultFontPointSize*1.1
                color  : qgcPal.text
            }
        }
        frame : Rectangle{
            color : qgcPal.window
            radius:  ScreenTools.defaultFontPixelHeight/1.5
            border.color :  "lightsteelblue"
            }
        }
    }

    Rectangle{
        id:change
        anchors.top : parent.top
        anchors.right:parent.right
        width:10
        height:parent.height
        color:"black"
        opacity:0
        //anchors.bottom: parent.bottom
        MouseArea{
            anchors.fill:parent
            hoverEnabled : true
            onEntered: {
                //console.log("JJJINRU")
                if(tabDis){
                    tabDis=false
                    tabview.visible=false
                }
                else {
                    tabDis=true
                    tabview.visible=true
                }


            }
        }

    }
    Rectangle{
        id:imagechange
        anchors.top : change.bottom
        anchors.right:parent.right
        width:10
        height:parent.height/2
        color:"black"
        opacity:0
        anchors.bottom: parent.bottom
        MouseArea{
            anchors.fill:parent
            hoverEnabled : true
            onEntered: {
                //console.log("JJJINRU")
                if(imgDis){
                    imgDis=false
                    //if(photoFrame.x<0)photoFrame.x=0
                   // if(photoFrame.x>imageDis.width-photoFrame.width)photoFrame.x=imageDis.width-photoFrame.width
                    //if(photoFrame.y<0)photoFrame.y=0
                   // if(photoFrame.y>imageDis.height-photoFrame.height)photoFrame.y=imageDis.height-photoFrame.height
                    imageDis.visible=false
                }
                else {
                    imgDis=true
                    imageDis.visible=true
                }


            }
        }

    }
    Component{
        id : vtol
        QGCViewPanel{
           id : setuppanel
           anchors.fill: parent
           color :      qgcPal.window
           QGCFlickable{
              clip : true
              anchors.fill: parent
              contentWidth : setupColumn.width
              contentHeight : setupColumn.height
              Column{
                  id : setupColumn
                  width : setuppanel.width
                  spacing : ScreenTools.defaultFontPixelHeight * 0.5
                  anchors.margins : ScreenTools.defaultFontPixelWidth
                  Rectangle{
                      width : parent.width*0.8;height: 3
                      color : qgcPal.window
                      anchors.horizontalCenter:   parent.horizontalCenter
                  }
                  Item {
                      width:              parent.width * 0.9
                      height:             hoverModeLabel.height
                      anchors.margins:    ScreenTools.defaultFontPixelWidth *0.5
                      anchors.horizontalCenter: parent.horizontalCenter
                      QGCLabel {
                          id:             hoverModeLabel
                          text:           qsTr("Hover Mode")
                          font.family:    ScreenTools.demiboldFontFamily
                      }
                  }
                  Rectangle{
                      id : hoverMode
                      width : parent.width * 0.9
                      enabled: _fullParameterVehicleAvailable && (_activeVehicle.multiRotor || _activeVehicle.vtol)
                      height : hoverModeCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                      color : qgcPal.windowShade
                      anchors.margins: ScreenTools.defaultFontPixelWidth *0.5
                      anchors.horizontalCenter: parent.horizontalCenter
                      radius : ScreenTools.defaultFontPixelWidth
                      border.color: "lightsteelblue"

                      Column{
                        id: hoverModeCol
                        spacing: ScreenTools.defaultFontPixelHeight * 0.5
                        anchors.margins: ScreenTools.defaultFontPixelWidth *0.5
                        anchors.centerIn: parent
                        Row{
                            spacing:    ScreenTools.defaultFontPixelHeight * 0.5
                            QGCLabel{
                                text:           qsTr("LandDescendRate:")
                                font.family:   ScreenTools.normalFontFamily
                                width:          _labelWidth
                                anchors.baseline: landSpeed.baseline
                            }
                            FactTextField{
                                    id:            landSpeed
                                    fact:          _fullParameterVehicleAvailable  ? _activeVehicle.getParameterFact(-1,  "MPC_LAND_SPEED") : _emptyFact
                                    showUnits:     true
                                    width:         _editFieldWidth

                            }
                        }
                        Row{
                            spacing:    ScreenTools.defaultFontPixelHeight * 0.5
                            QGCLabel{
                                text:              qsTr("TakeOffClimbRate:")
                                font.family:      ScreenTools.normalFontFamily
                                width:             _labelWidth
                                anchors.baseline:  takeOffSpeed.baseline
                            }
                            FactTextField{
                                id:                takeOffSpeed
                                fact:              _fullParameterVehicleAvailable  ? _activeVehicle.getParameterFact(-1, "MPC_TKO_SPEED") : _emptyFact
                                showUnits:         true
                                width:                    _editFieldWidth
                            }
                        }
                        Row{
                            spacing:    ScreenTools.defaultFontPixelHeight * 0.5
                            QGCLabel{
                                text:                            qsTr("MaxDescendRate:")
                                font.family:      ScreenTools.normalFontFamily
                                width:              _labelWidth
                                anchors.baseline:    descendSpeed.baseline
                            }
                            FactTextField{
                                id:                     descendSpeed
                                fact:                 _fullParameterVehicleAvailable  ? _activeVehicle.getParameterFact(-1, "MPC_Z_VEL_MAX_DN") : _emptyFact
                                showUnits:    true
                                width:              _editFieldWidth
                            }
                        }
                        Row{
                            spacing : ScreenTools.defaultFontPixelHeight * 0.5
                            QGCLabel{
                                text:               qsTr("MaxClimbRate:")
                                font.family:      ScreenTools.normalFontFamily
                                width:             _labelWidth
                                anchors.baseline:  ascentSpeed.baseline
                            }
                            FactTextField{
                                id:                 ascentSpeed
                                fact:              _fullParameterVehicleAvailable  ? _activeVehicle.getParameterFact(-1, "MPC_Z_VEL_MAX_UP") : _emptyFact
                                showUnits:  true
                                width:            _editFieldWidth
                            }
                        }
                        Row{
                            spacing: ScreenTools.defaultFontPixelHeight * 0.5
                            QGCLabel{
                                text:           qsTr("MaxYaw rate:")
                                font.family:      ScreenTools.normalFontFamily
                                width:          _labelWidth
                                anchors.baseline: yawRate.baseline
                            }
                            FactTextField{
                                id:         yawRate
                                fact:      _fullParameterVehicleAvailable  ? _activeVehicle.getParameterFact(-1 , "MC_YAWRATE_MAX") : _emptyFact
                                showUnits: true
                                width:  _editFieldWidth
                            }
                        }
                        Row{
                            spacing: ScreenTools.defaultFontPixelHeight * 0.5
                            QGCLabel{
                                text:qsTr("Hover Throttle:")
                                font.family:      ScreenTools.normalFontFamily
                                width:     _labelWidth
                                anchors.baseline:  hoverThrottle.baseline
                            }
                            FactTextField{
                                id:        hoverThrottle
                                fact:      _fullParameterVehicleAvailable  ? _activeVehicle.getParameterFact(-1,  "MPC_THR_HOVER") : _emptyFact
                                showUnits: true
                                width:     _editFieldWidth
                            }
                        }
                        Row{
                            spacing: ScreenTools.defaultFontPixelHeight * 0.5
                            QGCLabel{
                                text:  qsTr("MinManualThr:")
                                font.family:      ScreenTools.normalFontFamily
                                width:     _labelWidth
                                anchors.baseline: manualMinThr.baseline
                            }
                            FactTextField{
                                id:    manualMinThr
                                fact:  _fullParameterVehicleAvailable ? _activeVehicle.getParameterFact(-1, "MPC_MANTHR_MIN") : _emptyFact
                                showUnits:  true
                                width:  _editFieldWidth
                            }
                        }
                    }
                }
                Item {
                    width:              parent.width * 0.9
                    height:             vtolPlanemodeLabel.height
                    anchors.margins:    ScreenTools.defaultFontPixelWidth *0.5
                    anchors.horizontalCenter: parent.horizontalCenter
                    QGCLabel {
                        id:             vtolPlanemodeLabel
                        text:           qsTr("Plane Mode")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle{
                    id : vtolPlane
                    enabled: _fullParameterVehicleAvailable
                    width : parent.width * 0.9
                    height : vtolPlaneCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    color : qgcPal.windowShade
                    anchors.margins: ScreenTools.defaultFontPixelWidth *0.5
                    anchors.horizontalCenter: parent.horizontalCenter
                    radius : ScreenTools.defaultFontPixelWidth
                    border.color: "lightsteelblue"
                    Column{
                        id : vtolPlaneCol

                        spacing : ScreenTools.defaultFontPixelHeight * 0.5
                        anchors.margins: ScreenTools.defaultFontPixelWidth *0.5
                        anchors.centerIn: parent
                        Row{
                            spacing: ScreenTools.defaultFontPixelHeight * 0.5
                            QGCLabel{
                                text:   qsTr("MaxThr Limit:")
                                font.family:   ScreenTools.normalFontFamily
                                width:          _labelWidth
                                anchors.baseline:  maxthrottleLimit.baseline
                            }
                            FactTextField{
                                id:     maxthrottleLimit
                                fact:   _fullParameterVehicleAvailable ? _activeVehicle.getParameterFact(-1,  "FW_THR_MAX") : _emptyFact
                                showUnits: true
                                width:      _editFieldWidth
                            }
                        }
                        Row{
                            spacing: ScreenTools.defaultFontPixelHeight * 0.5
                            QGCLabel{
                                text:       qsTr("MinThr Limit:")
                                font.family:   ScreenTools.normalFontFamily
                                width:          _labelWidth
                                anchors.baseline: minthrottleLimit.baseline
                            }
                            FactTextField{
                                id:     minthrottleLimit
                                fact:      _fullParameterVehicleAvailable ? _activeVehicle.getParameterFact(-1, "FW_THR_MIN") : _emptyFact
                                showUnits: true
                                width:      _editFieldWidth
                            }
                        }
                        Row{
                            spacing: ScreenTools.defaultFontPixelHeight * 0.5
                            QGCLabel{
                                text:  qsTr("MaxClimbRate:")
                                font.family:   ScreenTools.normalFontFamily
                                width:  _labelWidth
                                anchors.baseline:   maxClimbRate.baseline
                            }
                            FactTextField{
                                id:     maxClimbRate
                                fact:   _fullParameterVehicleAvailable ? _activeVehicle.getParameterFact(-1, "FW_T_CLMB_MAX") : _emptyFact
                                showUnits:  true
                                width:      _editFieldWidth
                            }
                        }
                        Row{
                            spacing:ScreenTools.defaultFontPixelHeight * 0.5
                            QGCLabel{
                                text:   qsTr("Pitch Offset:")
                                font.family:   ScreenTools.normalFontFamily
                                width:      _labelWidth
                                anchors.baseline:  pitchOffset.baseline
                            }
                            FactTextField{
                                id:     pitchOffset
                                fact:   _fullParameterVehicleAvailable  ? _activeVehicle.getParameterFact(-1, "FW_PSP_OFF") : _emptyFact
                                showUnits:  true
                                width:  _editFieldWidth
                            }
                        }
                        Row{
                            spacing: ScreenTools.defaultFontPixelHeight * 0.5
                            QGCLabel{
                                text:       qsTr("CruiseAirspeed:")
                                font.family:   ScreenTools.normalFontFamily
                                width:          _labelWidth
                                anchors.baseline:  cruiseSpeed.baseline
                            }
                            FactTextField{
                                id:         cruiseSpeed
                                fact:      _fullParameterVehicleAvailable ? _activeVehicle.getParameterFact(-1, "FW_AIRSPD_TRIM") : _emptyFact
                                showUnits: true
                                width: _editFieldWidth
                            }
                        }
                        Row{
                            spacing: ScreenTools.defaultFontPixelHeight * 0.5
                            QGCLabel{
                                text:       qsTr("Cruise Throttle:")
                                font.family:   ScreenTools.normalFontFamily
                                width:  _labelWidth
                                anchors.baseline: cruiseThrottle.baseline
                            }
                            FactTextField{
                                id: cruiseThrottle
                                fact: _fullParameterVehicleAvailable ? _activeVehicle.getParameterFact(-1, "FW_THR_CRUISE") : _emptyFact
                                showUnits:  true
                                width:  _editFieldWidth
                            }
                        }
                    }
                }
              }
            }//QGCFlickable
        }//QGCViewPanel
    } //Component
    Component{
        id : copter
        QGCViewPanel{
           id : copterPanel
           anchors.fill: parent
           color :      qgcPal.window
           QGCFlickable{

               clip : true
               anchors.fill: parent
               contentWidth : copterColumn.width
               contentHeight : copterColumn.height
               Column{
                   id : copterColumn
                   width : copterPanel.width
                   spacing : ScreenTools.defaultFontPixelHeight * 0.5
                   anchors.margins : ScreenTools.defaultFontPixelWidth
                   Rectangle{
                       width : parent.width*0.8;height: 3
                       color : qgcPal.window
                       anchors.horizontalCenter:   parent.horizontalCenter
                   }
                   Item {
                       width:              parent.width * 0.9
                       height:             multiRotorsLabel.height
                       anchors.margins:    ScreenTools.defaultFontPixelWidth *0.5
                       anchors.horizontalCenter: parent.horizontalCenter
                       QGCLabel {
                           id:             multiRotorsLabel
                           text:           qsTr("Multi Rotors")
                           font.family:    ScreenTools.demiboldFontFamily
                       }
                   }
                   Rectangle{
                       id : multiRotors
                       width : parent.width * 0.9
                       enabled: _fullParameterVehicleAvailable
                       height : multiRotrosCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                       color : qgcPal.windowShade
                       anchors.margins: ScreenTools.defaultFontPixelWidth *0.5
                       anchors.horizontalCenter: parent.horizontalCenter
                       radius : ScreenTools.defaultFontPixelWidth
                       border.color: "lightsteelblue"
                       Column{
                           id: multiRotrosCol
                           spacing: ScreenTools.defaultFontPixelHeight * 0.5
                           anchors.margins: ScreenTools.defaultFontPixelWidth *0.5
                           anchors.centerIn: parent
                           Row{
                               spacing:    ScreenTools.defaultFontPixelHeight * 0.5
                               QGCLabel{
                                   text:           qsTr("LandDescendRate:")
                                   font.family:   ScreenTools.normalFontFamily
                                   width:          _labelWidth
                                   anchors.baseline: landSpeed.baseline
                               }
                               FactTextField{
                                       id:            landSpeed
                                       fact:          _fullParameterVehicleAvailable  ? _activeVehicle.getParameterFact(-1,  "MPC_LAND_SPEED") : _emptyFact
                                       showUnits:     true
                                       width:         _editFieldWidth

                               }
                           }
                           Row{
                               spacing:    ScreenTools.defaultFontPixelHeight * 0.5
                               QGCLabel{
                                   text:              qsTr("TakeOffClimbRate:")
                                   font.family:      ScreenTools.normalFontFamily
                                   width:             _labelWidth
                                   anchors.baseline:  takeOffSpeed.baseline
                               }
                               FactTextField{
                                   id:                takeOffSpeed
                                   fact:              _fullParameterVehicleAvailable  ? _activeVehicle.getParameterFact(-1, "MPC_TKO_SPEED") : _emptyFact
                                   showUnits:         true
                                   width:                    _editFieldWidth
                               }
                           }
                           Row{
                               spacing:    ScreenTools.defaultFontPixelHeight * 0.5
                               QGCLabel{
                                   text:                            qsTr("MaxDescendRate:")
                                   font.family:      ScreenTools.normalFontFamily
                                   width:              _labelWidth
                                   anchors.baseline:    descendSpeed.baseline
                               }
                               FactTextField{
                                   id:                     descendSpeed
                                   fact:                 _fullParameterVehicleAvailable  ? _activeVehicle.getParameterFact(-1, "MPC_Z_VEL_MAX_DN") : _emptyFact
                                   showUnits:    true
                                   width:              _editFieldWidth
                               }
                           }
                           Row{
                               spacing : ScreenTools.defaultFontPixelHeight * 0.5
                               QGCLabel{
                                   text:               qsTr("MaxClimbRate:")
                                   font.family:      ScreenTools.normalFontFamily
                                   width:             _labelWidth
                                   anchors.baseline:  ascentSpeed.baseline
                               }
                               FactTextField{
                                   id:                 ascentSpeed
                                   fact:              _fullParameterVehicleAvailable  ? _activeVehicle.getParameterFact(-1, "MPC_Z_VEL_MAX_UP") : _emptyFact
                                   showUnits:  true
                                   width:            _editFieldWidth
                               }
                           }
                           Row{
                               spacing: ScreenTools.defaultFontPixelHeight * 0.5
                               QGCLabel{
                                   text:           qsTr("MaxYaw rate:")
                                   font.family:      ScreenTools.normalFontFamily
                                   width:          _labelWidth
                                   anchors.baseline: yawRate.baseline
                               }
                               FactTextField{
                                   id:         yawRate
                                   fact:      _fullParameterVehicleAvailable  ? _activeVehicle.getParameterFact(-1 , "MC_YAWRATE_MAX") : _emptyFact
                                   showUnits: true
                                   width:  _editFieldWidth
                               }
                           }
                           Row{
                               spacing: ScreenTools.defaultFontPixelHeight * 0.5
                               QGCLabel{
                                   text:qsTr("Hover Throttle:")
                                   font.family:      ScreenTools.normalFontFamily
                                   width:     _labelWidth
                                   anchors.baseline:  hoverThrottle.baseline
                               }
                               FactTextField{
                                   id:        hoverThrottle
                                   fact:      _fullParameterVehicleAvailable  ? _activeVehicle.getParameterFact(-1,  "MPC_THR_HOVER") : _emptyFact
                                   showUnits: true
                                   width:     _editFieldWidth
                               }
                           }
                           Row{
                               spacing: ScreenTools.defaultFontPixelHeight * 0.5
                               QGCLabel{
                                   text:  qsTr("MinManualThr:")
                                   font.family:      ScreenTools.normalFontFamily
                                   width:     _labelWidth
                                   anchors.baseline: manualMinThr.baseline
                               }
                               FactTextField{
                                   id:    manualMinThr
                                   fact:  _fullParameterVehicleAvailable ? _activeVehicle.getParameterFact(-1, "MPC_MANTHR_MIN") : _emptyFact
                                   showUnits:  true
                                   width:  _editFieldWidth
                               }
                           }
                       }
                   }
               }
           }
        }
    }
    Component{
        id : plane
        QGCViewPanel{
           id : planePanel
           anchors.fill: parent
           color :      qgcPal.window
           QGCFlickable{
               clip : true
               anchors.fill: parent
               contentWidth : planeColumn.width
               contentHeight : planeColumn.height
               Column{
                   id : planeColumn
                   width : planePanel.width
                   spacing : ScreenTools.defaultFontPixelHeight * 0.5
                   anchors.margins : ScreenTools.defaultFontPixelWidth
                   Rectangle{
                       width : parent.width*0.8;height: 3
                       color : qgcPal.window
                       anchors.horizontalCenter:   parent.horizontalCenter
                   }
                   Item {
                       width:              parent.width * 0.9
                       height:             planeLabel.height
                       anchors.margins:    ScreenTools.defaultFontPixelWidth *0.5
                       anchors.horizontalCenter: parent.horizontalCenter
                       QGCLabel {
                           id:             planeLabel
                           text:           qsTr("Plane")
                           font.family:    ScreenTools.demiboldFontFamily
                       }
                   }
                   Rectangle{
                       id : planeData
                       width : parent.width * 0.9
                       enabled: _fullParameterVehicleAvailable
                       height : planeCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                       color : qgcPal.windowShade
                       anchors.margins: ScreenTools.defaultFontPixelWidth *0.5
                       anchors.horizontalCenter: parent.horizontalCenter
                       radius : ScreenTools.defaultFontPixelWidth
                       border.color: "lightsteelblue"
                       Column{
                           id: planeCol
                           spacing: ScreenTools.defaultFontPixelHeight * 0.5
                           anchors.margins: ScreenTools.defaultFontPixelWidth *0.5
                           anchors.centerIn: parent
                           Row{
                               spacing: ScreenTools.defaultFontPixelHeight * 0.5
                               QGCLabel{
                                   text:   qsTr("MaxThr Limit:")
                                   font.family:   ScreenTools.normalFontFamily
                                   width:          _labelWidth
                                   anchors.baseline:  maxthrottleLimit.baseline
                               }
                               FactTextField{
                                   id:     maxthrottleLimit
                                   fact:   _fullParameterVehicleAvailable ? _activeVehicle.getParameterFact(-1,  "FW_THR_MAX") : _emptyFact
                                   showUnits: true
                                   width:      _editFieldWidth
                               }
                           }
                           Row{
                               spacing: ScreenTools.defaultFontPixelHeight * 0.5
                               QGCLabel{
                                   text:       qsTr("MinThr Limit:")
                                   font.family:   ScreenTools.normalFontFamily
                                   width:          _labelWidth
                                   anchors.baseline: minthrottleLimit.baseline
                               }
                               FactTextField{
                                   id:     minthrottleLimit
                                   fact:      _fullParameterVehicleAvailable ? _activeVehicle.getParameterFact(-1, "FW_THR_MIN") : _emptyFact
                                   showUnits: true
                                   width:      _editFieldWidth
                               }
                           }
                           Row{
                               spacing: ScreenTools.defaultFontPixelHeight * 0.5
                               QGCLabel{
                                   text:  qsTr("MaxClimbRate:")
                                   font.family:   ScreenTools.normalFontFamily
                                   width:  _labelWidth
                                   anchors.baseline:   maxClimbRate.baseline
                               }
                               FactTextField{
                                   id:     maxClimbRate
                                   fact:   _fullParameterVehicleAvailable ? _activeVehicle.getParameterFact(-1, "FW_T_CLMB_MAX") : _emptyFact
                                   showUnits:  true
                                   width:      _editFieldWidth
                               }
                           }
                           Row{
                               spacing:ScreenTools.defaultFontPixelHeight * 0.5
                               QGCLabel{
                                   text:   qsTr("Pitch Offset:")
                                   font.family:   ScreenTools.normalFontFamily
                                   width:      _labelWidth
                                   anchors.baseline:  pitchOffset.baseline
                               }
                               FactTextField{
                                   id:     pitchOffset
                                   fact:   _fullParameterVehicleAvailable  ? _activeVehicle.getParameterFact(-1, "FW_PSP_OFF") : _emptyFact
                                   showUnits:  true
                                   width:  _editFieldWidth
                               }
                           }
                           Row{
                               spacing: ScreenTools.defaultFontPixelHeight * 0.5
                               QGCLabel{
                                   text:       qsTr("Cruise Airspeed:")
                                   font.family:   ScreenTools.normalFontFamily
                                   width:          _labelWidth
                                   anchors.baseline:  cruiseSpeed.baseline
                               }
                               FactTextField{
                                   id:         cruiseSpeed
                                   fact:      _fullParameterVehicleAvailable ? _activeVehicle.getParameterFact(-1, "FW_AIRSPD_TRIM") : _emptyFact
                                   showUnits: true
                                   width: _editFieldWidth
                               }
                           }
                           Row{
                               spacing: ScreenTools.defaultFontPixelHeight * 0.5
                               QGCLabel{
                                   text:       qsTr("Cruise Throttle:")
                                   font.family:   ScreenTools.normalFontFamily
                                   width:  _labelWidth
                                   anchors.baseline: cruiseThrottle.baseline
                               }
                               FactTextField{
                                   id: cruiseThrottle
                                   fact: _fullParameterVehicleAvailable ? _activeVehicle.getParameterFact(-1, "FW_THR_CRUISE") : _emptyFact
                                   showUnits:  true
                                   width:  _editFieldWidth
                               }
                           }
                           Row{
                              spacing : ScreenTools.defaultFontPixelHeight * 0.5
                              QGCLabel{
                                  text: qsTr("Parachute Thr:")
                                  font.family:   ScreenTools.normalFontFamily
                                  width:  _labelWidth
                                  anchors.baseline: parachutethr.baseline
                              }
                              FactTextField{
                                  id: parachutethr
                                  fact: _fullParameterVehicleAvailable ? _activeVehicle.getParameterFact(-1,"PARACHUTE_THR") : _emptyFact
                                  showUnits:true
                                  width:_editFieldWidth
                              }
                           }
                           Row{
                              spacing : ScreenTools.defaultFontPixelHeight * 0.5
                              QGCLabel{
                                  text: qsTr("Parachute Del:")
                                  font.family:   ScreenTools.normalFontFamily
                                  width:  _labelWidth
                                  anchors.baseline: parachutedelay.baseline
                              }
                              FactTextField{
                                  id: parachutedelay
                                  fact: _fullParameterVehicleAvailable ? _activeVehicle.getParameterFact(-1,"PARACHUTE_TIM") : _emptyFact
                                  showUnits:true
                                  width:_editFieldWidth
                              }
                           }
                        }
                    }
                }
            }
        }
    }
    Component{
        id : cameraComponent
        QGCViewPanel{
            id : cameraPanel
            anchors.fill: parent
            color : qgcPal.window
            QGCFlickable{
                clip : true
                anchors.fill : parent
                contentWidth : cameraPanel.width
                contentHeight : cameraPanel.height
                Column{
                    id : cameraCol
                    width : cameraPanel.width
                    spacing : ScreenTools.defaultFontPixelHeight * 0.5
                    anchors.margins: ScreenTools.defaultFontPixelWidth
                    Rectangle{
                        width : parent.width * 0.8; height : 3
                        color : qgcPal.window
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Item{
                        width : parent.width * 0.9
                        height : cameraLabel.height
                        anchors.margins : ScreenTools.defaultFontPixelWidth * 0.5
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCLabel{
                            id : cameraLabel
                            text: qsTr("Camera Trigger Settings")
                            font.family : ScreenTools.demiboldFontFamily
                        }
                    }
                    Rectangle{
                        id : cameraSettings
                        width : parent.width * 0.9
                        enabled: _fullParameterVehicleAvailable
                        height : cameraSettingsCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                        color : qgcPal.windowShade
                        anchors.margins: ScreenTools.defaultFontPixelWidth * 0.5
                        anchors.horizontalCenter: parent.horizontalCenter
                        radius : ScreenTools.defaultFontPixelWidth
                        border.color : "lightsteelblue"
                        Column{
                            id : cameraSettingsCol
                            spacing : ScreenTools.defaultFontPixelWidth
                            anchors.centerIn: parent
                            Row{
                                spacing : ScreenTools.defaultFontPixelWidth
                                QGCLabel{
                                    anchors.baseline: camtriggerMode.baseline
                                    width : _labelWidth
                                    text: qsTr("Trigger mode :")
                                }
                                QGCComboBox{
                                    id : camtriggerMode
                                    property bool indexModel : false
                                    property var  changModel : indexModel ? _triggerModeFact.value : _triggerModeFact.enumIndex
                                    property var  indexSelect : indexModel ? _triggerModeFact.value : _triggerModeFact.enumIndex
                                    width : _editFieldWidth
                                    model : indexModel != null && _fullParameterVehicleAvailable ? _triggerModeFact.enumStrings : null
                                    currentIndex : _fullParameterVehicleAvailable ? _triggerModeFact.enumIndex : 0
                                    onActivated :{
                                        indexSelect = index
                                        if(camtriggerMode.indexModel){
                                            _triggerModeFact.value = camtriggerMode.indexSelect
                                        }else{
                                            _triggerModeFact.value = _triggerModeFact.enumValues[camtriggerMode.indexSelect]
                                        }
                                    }
                                }
                            }

                            Row{
                                spacing : ScreenTools.defaultFontPixelWidth
                                QGCLabel{
                                    anchors.baseline: timeInterval.baseline
                                    width : _labelWidth
                                    text : qsTr("Time Internal :")
                                }
                                FactTextField{
                                    id : timeInterval
                                    width : _editFieldWidth
                                    fact : _fullParameterVehicleAvailable ? _activeVehicle.getParameterFact(-1,"TRIG_INTERVAL") : _emptyFact
                                    showUnits : true
                                    enabled : _triggerModeFact.value === 2
                                }
                            }
                            Row{
                                spacing : ScreenTools.defaultFontPixelWidth
                                QGCLabel{
                                    anchors.baseline: distanceInterval.baseline
                                    width : _labelWidth
                                    text : qsTr("Distance Interval :")
                                }
                                FactTextField{
                                    id : distanceInterval
                                    width : _editFieldWidth
                                    fact : _fullParameterVehicleAvailable ? _activeVehicle.getParameterFact(-1,"TRIG_DISTANCE") : _emptyFact
                                    enabled : _triggerModeFact.value === 3
                                }
                            }
                        }
                    }
                    Rectangle{
                        width : parent.width * 0.8; height : 3
                        color : qgcPal.window
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Item{
                        width : parent.width * 0.9
                        height : cameraLabel1.height
                        anchors.margins : ScreenTools.defaultFontPixelWidth * 0.5
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCLabel{
                            id : cameraLabel1
                            text: qsTr("Tx1 Camera Settings")
                            font.family : ScreenTools.demiboldFontFamily
                        }
                    }
                    Rectangle{
                        id : cameraSettings1
                        width : parent.width * 0.9
                        enabled: _fullParameterVehicleAvailable
                        height : cameraSettingsCol1.height + (ScreenTools.defaultFontPixelHeight * 2)
                        color : qgcPal.windowShade
                        anchors.margins: ScreenTools.defaultFontPixelWidth * 0.5
                        anchors.horizontalCenter: parent.horizontalCenter
                        radius : ScreenTools.defaultFontPixelWidth
                        border.color : "lightsteelblue"
                        Column{
                            id : cameraSettingsCol1
                            spacing : ScreenTools.defaultFontPixelWidth
                            anchors.centerIn: parent


                            Row{
                                spacing : ScreenTools.defaultFontPixelWidth
                                QGCLabel{
                                   // anchors.baseline: timeInterval.baseline
                                    width : _labelWidth
                                    text : qsTr("bitrate :")
                                }
                               TextField{
                                    id : param1
                                    width : _editFieldWidth
                                    text:"40000000"
                                    onEditingFinished: {

                                      bitrate = parseInt(param1.getText(0,12))
                                        if(bitrate<=1000000)bitrate=1000000
                                        console.log("bitrate="+bitrate)
//                                        tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
//                                                                        -402, m
//                                                                        , 0, 0)
                                    }
                                }
                            }


                            Row{
                                spacing : ScreenTools.defaultFontPixelWidth
                                QGCLabel{
                                    anchors.baseline: param2.baseline
                                    width : _labelWidth
                                    text : qsTr("Iframe :")
                                }
                                TextField{
                                    id : param2
                                    width : _editFieldWidth
                                    text:"0"
                                    onEditingFinished: {
                                    iframe = parseInt(param2.getText(0,12))
                                    if(iframe<5)iframe=5
//                                        tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
//                                                                        -402, m
//                                                                        , 0, 0)
                                    }
                                }
                            }
                            Row{
                                spacing : ScreenTools.defaultFontPixelWidth
                                QGCLabel{
                                    anchors.baseline: param3.baseline
                                    width : _labelWidth
                                    text : qsTr("Pframe :")
                                }
                                TextField{
                                    id : param3
                                    width : _editFieldWidth
                                    text:"0"
                                    onEditingFinished: {
                                      pframe=parseInt(param3.getText(0,12))
                                      if(pframe<5)pframe=5
//                                        tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
//                                                                        -402, m
//                                                                        , 0, 0)
                                    }
                                }
                            }

                            Row{
                                spacing : ScreenTools.defaultFontPixelWidth

                                Button{
                                    id : submit
                                    width : _editFieldWidth
                                    text:"submit"
                                    anchors.centerIn: parent.Center
                                    onClicked: {


                                        tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
                                                                        -501, bitrate
                                                                        , iframe, pframe)
                                    }
                                }
                            }

                        }
                    }
                    Rectangle{
                        width : parent.width * 0.8; height : 3
                        color : qgcPal.window
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Item{
                        width : parent.width * 0.9
                        height : cameraLabel1.height
                        anchors.margins : ScreenTools.defaultFontPixelWidth * 0.5
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCLabel{
                            id : cameraLabel11
                            text: qsTr("Tx1 Algorithm Settings")
                            font.family : ScreenTools.demiboldFontFamily
                        }
                    }
                    Rectangle{
                        id : cameraSettings11
                        width : parent.width * 0.9
                        enabled: _fullParameterVehicleAvailable
                        height : cameraSettingsCol11.height + (ScreenTools.defaultFontPixelHeight * 2)
                        color : qgcPal.windowShade
                        anchors.margins: ScreenTools.defaultFontPixelWidth * 0.5
                        anchors.horizontalCenter: parent.horizontalCenter
                        radius : ScreenTools.defaultFontPixelWidth
                        border.color : "lightsteelblue"
                        Column{
                            id : cameraSettingsCol11
                            spacing : ScreenTools.defaultFontPixelWidth
                            anchors.centerIn: parent
                            Row{
                                spacing : ScreenTools.defaultFontPixelWidth

                                Button{
                                    id : brisk
                                    width : _editFieldWidth
                                    text:"BRISK"
                                    anchors.centerIn: parent.Center
                                    onClicked: {


                                        tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
                                                                        -403, 0
                                                                        , 0, 0)
                                    }
                                }
                                Button{
                                    id : orb
                                    width : _editFieldWidth
                                    text:"ORB"
                                    anchors.centerIn: parent.Center
                                    onClicked: {


                                        tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
                                                                        -404,0
                                                                        , 0, 0)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    QGCInstrumentWidgetAlternate {
        id:                     instrumentGadgetAlternate
        anchors.margins:        ScreenTools.defaultFontPixelHeight / 2
        anchors.top:            parent.top
        anchors.right:          altitudeSlider.visible ? altitudeSlider.left : parent.right
        visible:                _useAlternateInstruments
        width:                  ScreenTools.isTinyScreen ? getGadgetWidth() * 1.5 : getGadgetWidth()
        active:                 _activeVehicle != null
        heading:                _heading
        rollAngle:              _roll
        pitchAngle:             _pitch
        groundSpeedFact:        _groundSpeedFact
        airSpeedFact:           _airSpeedFact
        isSatellite:            _isSatellite
        z:                      QGroundControl.zOrderWidgets
    }
    ValuesWidget {
        anchors.topMargin:          ScreenTools.defaultFontPixelHeight
        anchors.top:                instrumentGadgetAlternate.bottom
        anchors.horizontalCenter:   instrumentGadgetAlternate.horizontalCenter
        width:                      getGadgetWidth()
        qgcView:                    parent.parent.qgcView
        textColor:                  _isSatellite ? "white" : "black"
        visible:                    _useAlternateInstruments
        maxHeight:                  virtualJoystickMultiTouch.visible ? virtualJoystickMultiTouch.y - y : parent.height - anchors.margins - y
    }
    QGCLabel {
        id:         flyLabel
        text:       qsTr("Fly")
        color:      mapPal.text
        visible:    !ScreenTools.isShortScreen && _mainIsMap
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
        visible:            _mainIsMap

        //-- Map Center Control
        DropButton {
            id:                 centerMapDropButton
            dropDirection:      dropRight
            buttonImage:        "/qmlimages/MapCenter.svg"
            viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
            exclusiveGroup:     _dropButtonsExclusiveGroup
            z:                  QGroundControl.zOrderWidgets
            lightBorders:       _lightWidgetBorders

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
                        enabled:    _activeVehicle

                        property var activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

                        onClicked: {
                            if((_flightMap.missionController.visualItems != null) && (_flightMap.missionController.visualItems.count >= 2)){
                                _flightMap.center =  _flightMap.missionController.visualItems.get(1).coordinate
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
                            visible:    false
                            enabled:    QGroundControl.multiVehicleManager.activeVehicle && false
                            onClicked: {
                                QGroundControl.multiVehicleManager.activeVehicle.setDigitalCameraControl()
                                _dropButtonsExclusiveGroup.current = null
                            }
                        }
                    }
                }
            }
        }

        //-- Zoom Map In
        RoundButton {
            id:                 mapZoomPlus
            visible:            !ScreenTools.isTinyScreen && _mainIsMap
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
            visible:            !ScreenTools.isTinyScreen && _mainIsMap
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

    //-- Guided mode buttons
    Rectangle {
        id:                         _guidedModeBar
        anchors.margins:            _barMargin
        anchors.bottom:             parent.bottom
        anchors.horizontalCenter:   parent.horizontalCenter
        width:                      guidedModeColumn.width  + (_margins * 2)
        height:                     guidedModeColumn.height + (_margins * 2)
        radius:                     ScreenTools.defaultFontPixelHeight * 0.25
        color:                      _lightWidgetBorders ? Qt.rgba(qgcPal.mapWidgetBorderLight.r, qgcPal.mapWidgetBorderLight.g, qgcPal.mapWidgetBorderLight.b, 0.8) : Qt.rgba(qgcPal.mapWidgetBorderDark.r, qgcPal.mapWidgetBorderDark.g, qgcPal.mapWidgetBorderDark.b, 0.75)
        visible:                    _activeVehicle
        z:                          QGroundControl.zOrderWidgets
        state:                      "Shown"

        property real _fontPointSize: ScreenTools.isMobile ? ScreenTools.mediumFontPointSize : ScreenTools.defaultFontPointSize

        states: [
            State {
                name: "Shown"
                PropertyChanges { target: showAnimation; running: true  }
                PropertyChanges { target: guidedModeHideTimer; running: true }
            },
            State {
                name: "Hidden"
                PropertyChanges { target: hideAnimation; running: true  }
            }
        ]

        PropertyAnimation {
            id:             hideAnimation
            target:         _guidedModeBar
            property:       "_barMargin"
            duration:       1000
            easing.type:    Easing.InOutQuad
            from:           _guidedModeBar._showMargin
            to:             _guidedModeBar._hideMargin
        }

        PropertyAnimation {
            id:             showAnimation
            target:         _guidedModeBar
            property:       "_barMargin"
            duration:       250
            easing.type:    Easing.InOutQuad
            from:           _guidedModeBar._hideMargin
            to:             _guidedModeBar._showMargin
        }

        Timer {
            id:             guidedModeHideTimer
            interval:       7000
            running:        true
            onTriggered: {
                if (ScreenTools.isShortScreen) {
                    _guidedModeBar.state = "Hidden"
                }
            }
        }

        readonly property int confirmHome:          1
        readonly property int confirmLand:          2
        readonly property int confirmTakeoff:       3
        readonly property int confirmArm:           4
        readonly property int confirmDisarm:        5
        readonly property int confirmEmergencyStop: 6
//        readonly property int confirmChangeAlt:     7
        readonly property int confirmGoTo:          8
//        readonly property int confirmRetask:        9
//        readonly property int confirmOrbit:         10
        readonly property int confirmParachute:     11
        readonly property int confirmMission:       12
        readonly property int confirmPosition:      13
        readonly property int confirmVtolSwitch:    14
        property int    confirmActionCode
        property real   _showMargin:    _margins
        property real   _hideMargin:    _margins - _guidedModeBar.height
        property real   _barMargin:     _showMargin

        function actionConfirmed() {
            switch (confirmActionCode) {
            case confirmHome:
                _activeVehicle.guidedModeRTL()
                break;
            case confirmLand:
                _activeVehicle.guidedModeLand()
                break;
            case confirmTakeoff:
                var altitude1 = altitudeSlider.getValue()
                if (!isNaN(altitude1)) {
                    _activeVehicle.guidedModeTakeoff(altitude1)
                }
                break;
            case confirmArm:
                _activeVehicle.armed = true
                break;
            case confirmDisarm:
                _activeVehicle.armed = false
                break;
            case confirmEmergencyStop:
                _activeVehicle.emergencyStop()
                break;
//            case confirmChangeAlt:
//                var altitude2 = altitudeSlider.getValue()
//                if (!isNaN(altitude2)) {
//                    _activeVehicle.guidedModeChangeAltitude(altitude2)
//                }
//                break;
            case confirmGoTo:
                _activeVehicle.guidedModeGotoLocation(_flightMap._gotoHereCoordinate)
                break;
//            case confirmRetask:
//                _activeVehicle.setCurrentMissionSequence(_flightMap._retaskSequence)
//                break;
//            case confirmOrbit:
//                //-- All parameters controlled by RC
//                _activeVehicle.guidedModeOrbit()
//                //-- Center on current flight map position and orbit with a 50m radius (velocity/direction controlled by the RC)
//                //_activeVehicle.guidedModeOrbit(QGroundControl.flightMapPosition, 50.0)
//                break;
            case confirmParachute:
                _activeVehicle.guidedModeParachute();
                break;
            case confirmMission:
                _activeVehicle.guidedModeMission();
                break;
            case confirmPosition:
                _activeVehicle.guidedModePosition();
                break;
            case confirmVtolSwitch:
                _activeVehicle.guidedModeVTOLMcandFw();
                break;
            default:
                console.warn(qsTr("Internal error: unknown confirmActionCode"), confirmActionCode)
            }
        }

        function rejectGuidedModeConfirm() {
            guidedModeConfirm.visible = false
            _guidedModeBar.visible = true
            altitudeSlider.visible = false
            _flightMap._gotoHereCoordinate = QtPositioning.coordinate()
            guidedModeHideTimer.restart()
        }

        function confirmAction(actionCode) {
            guidedModeHideTimer.stop()
            confirmActionCode = actionCode
            switch (confirmActionCode) {
            case confirmArm:
                guidedModeConfirm.confirmText = qsTr("arm")
                break;
            case confirmDisarm:
                guidedModeConfirm.confirmText = qsTr("disarm")
                break;
            case confirmEmergencyStop:
                guidedModeConfirm.confirmText = qsTr("STOP ALL MOTORS!")
                break;
            case confirmTakeoff:
                altitudeSlider.visible = true
                altitudeSlider.setInitialValueMeters(3)
                guidedModeConfirm.confirmText = qsTr("takeoff")
                break;
            case confirmLand:
                guidedModeConfirm.confirmText = qsTr("land")
                break;
            case confirmHome:
                guidedModeConfirm.confirmText = qsTr("return to land")
                break;
//            case confirmChangeAlt:
//                altitudeSlider.visible = true
//                altitudeSlider.setInitialValueAppSettingsDistanceUnits(_activeVehicle.altitudeAMSL.value)
//                guidedModeConfirm.confirmText = qsTr("change altitude")
//                break;
            case confirmGoTo:
                guidedModeConfirm.confirmText = qsTr("move vehicle")
                break;
//            case confirmRetask:
//                guidedModeConfirm.confirmText = qsTr("active waypoint change")
//                break;
//            case confirmOrbit:
//                guidedModeConfirm.confirmText = qsTr("enter orbit mode")
//                break;
            case confirmParachute:
                guidedModeConfirm.confirmText = qsTr("parachute")
                break;
            case confirmMission:
                guidedModeConfirm.confirmText = qsTr("mission")
                break;
            case confirmPosition:
                guidedModeConfirm.confirmText = qsTr("position")
                break;
            case confirmVtolSwitch:
                if(_activeVehicle.vehicletype == 2)
                    guidedModeConfirm.confirmText = qsTr ("Plane mode")
                else
                    guidedModeConfirm.confirmText = qsTr("Hover mode")
                 break;
            }

            _guidedModeBar.visible = false
            guidedModeConfirm.visible = true
        }

        Column {
            id:                 guidedModeColumn
            anchors.margins:    _margins
            anchors.top:        parent.top
            anchors.left:       parent.left
            spacing:            _margins

            QGCLabel {
                anchors.horizontalCenter: parent.horizontalCenter
                color:      _lightWidgetBorders ? qgcPal.mapWidgetBorderDark : qgcPal.mapWidgetBorderLight
                text:       "Click in map to move vehicle"
                visible:    gotoEnabled
            }

            Row {
                spacing: _margins
                QGCButton {
                    pointSize:  _guidedModeBar._fontPointSize
                    text:       (_activeVehicle && _activeVehicle.armed) ? (_activeVehicle.flying ? qsTr("Emergency Stop") : qsTr("Disarm")) :  qsTr("Arm")
                    visible:    _activeVehicle
                    onClicked:  _guidedModeBar.confirmAction(_activeVehicle.armed ? (_activeVehicle.flying ? _guidedModeBar.confirmEmergencyStop : _guidedModeBar.confirmDisarm) : _guidedModeBar.confirmArm)
                }

                QGCButton {
                    pointSize:  _guidedModeBar._fontPointSize
                    text:       qsTr("RTL")
                    visible:    (_activeVehicle && _activeVehicle.armed) && _activeVehicle.guidedModeSupported && _activeVehicle.flying
                    onClicked:  _guidedModeBar.confirmAction(_guidedModeBar.confirmHome)
                }

                QGCButton {
                    pointSize:  _guidedModeBar._fontPointSize
                    text:       (_activeVehicle && _activeVehicle.flying) ?  qsTr("Land"):  qsTr("Takeoff")
                    visible:    _activeVehicle && _activeVehicle.guidedModeSupported && _activeVehicle.armed
                    onClicked:  _guidedModeBar.confirmAction(_activeVehicle.flying ? _guidedModeBar.confirmLand : _guidedModeBar.confirmTakeoff)
                }
                QGCButton{
                    pointSize : _guidedModeBar._fontPointSize
                    text : qsTr("Position")
                    visible:    (_activeVehicle && _activeVehicle.flying) && _activeVehicle.guidedModeSupported && _activeVehicle.armed
                    onClicked : _guidedModeBar.confirmAction(_guidedModeBar.confirmPosition)
                }
                QGCButton{
                    pointSize : _guidedModeBar._fontPointSize
                    text : qsTr("Mission")
                    visible: _activeVehicle  && _activeVehicle.guidedModeSupported && _activeVehicle.armed
                    onClicked : _guidedModeBar.confirmAction(_guidedModeBar.confirmMission)
                }
                QGCButton{
                    pointSize: _guidedModeBar._fontPointSize
                    text:   qsTr("Parachute")
                    visible: (_activeVehicle && _activeVehicle.flying) && _activeVehicle.paratureModeSupported && _activeVehicle.armed
                    onClicked: _guidedModeBar.confirmAction(_guidedModeBar.confirmParachute)
                }
                QGCButton{
                    pointSize : _guidedModeBar._fontPointSize
                    text :  (_activeVehicle && _activeVehicle.vtol && _activeVehicle.vehicletype == 2) ?  qsTr("VTOL FW"):  qsTr("VTOL MC")
                    visible : (_activeVehicle && _activeVehicle.vtol && _activeVehicle.flying)&& _activeVehicle.armed
                    onClicked: _guidedModeBar.confirmAction(_guidedModeBar.confirmVtolSwitch)
                }
            } // Row
        } // Column
    } // Rectangle - Guided mode buttons

    MouseArea {
        anchors.fill:   parent
        enabled:        guidedModeConfirm.visible
        onClicked:      _guidedModeBar.rejectGuidedModeConfirm()
    }

    // Action confirmation control
    SliderSwitch {
        id:                         guidedModeConfirm
        anchors.bottomMargin:       _margins
        anchors.bottom:             parent.bottom
        anchors.horizontalCenter:   parent.horizontalCenter
        visible:                    false
        z:                          QGroundControl.zOrderWidgets
        fontPointSize:              _guidedModeBar._fontPointSize

        onAccept: {
            guidedModeConfirm.visible = false
            _guidedModeBar.visible = true
            _guidedModeBar.actionConfirmed()
            altitudeSlider.visible = false
            guidedModeHideTimer.restart()
        }

        onReject: _guidedModeBar.rejectGuidedModeConfirm()
    }

    //-- Altitude slider
    Rectangle {
        id:                 altitudeSlider
        anchors.margins:    _margins
        anchors.right:      parent.right
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        color:              qgcPal.window
        width:              ScreenTools.defaultFontPixelWidth * 10
        opacity:            0.8
        visible:            false

        function setInitialValueMeters(meters) {
            altSlider.value = QGroundControl.metersToAppSettingsDistanceUnits(meters)
        }

        function setInitialValueAppSettingsDistanceUnits(height) {
            altSlider.value = height
        }

        /// Returns NaN for bad value
        function getValue() {
            var value =  parseFloat(altField.text)
            if (!isNaN(value)) {
                return QGroundControl.appSettingsDistanceUnitsToMeters(value);
            } else {
                return value;
            }
        }

        Column {
            id:                 headerColumn
            anchors.margins:    _margins
            anchors.top:        parent.top
            anchors.left:       parent.left
            anchors.right:      parent.right

            QGCLabel {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Alt (rel)")
            }

            QGCLabel {
                anchors.horizontalCenter: parent.horizontalCenter
                text: QGroundControl.appSettingsDistanceUnitsString
            }

            QGCTextField {
                id:             altField
                anchors.left:   parent.left
                anchors.right:  parent.right
                text:           altSlider.value.toFixed(1)
            }
        }

        Slider {
            id:                 altSlider
            anchors.margins:    _margins
            anchors.top:        headerColumn.bottom
            anchors.bottom:     parent.bottom
            anchors.left:       parent.left
            anchors.right:      parent.right
            orientation:        Qt.Vertical
            minimumValue:       QGroundControl.metersToAppSettingsDistanceUnits(2)
            maximumValue:       QGroundControl.metersToAppSettingsDistanceUnits((_activeVehicle && _activeVehicle.flying) ? 100 : 10)
        }
    }
}
