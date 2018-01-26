/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                      2.4
import QtQuick.Controls             1.3

import QGroundControl               1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controllers   1.0


Item {
    id: root

    //property var   videoWidgets
    property bool  _selectZoom:         false
    property int m :0
    property int b :0
    property int c: 0
    property int d:0
    property int e:0
    property int f:0
    property var boundboxArray:[]
    property var apointArrary: []
    property var locationArray:[]
    property int k:1
    property int kk:1
    property int num:0
    property var scal:0
    property int trackTime:0
    property bool tracking:true
    Rectangle {
        id:             noVideo
        anchors.fill:   parent
        color:          Qt.rgba(0,0,0,0.75)
        visible:        !QGroundControl.videoManager.videoRunning
        QGCLabel {
            text:               qsTr("NO VIDEO")
            font.family:        ScreenTools.demiboldFontFamily
            color:              "white"
            font.pointSize:     _mainIsMap ? ScreenTools.smallFontPointSize : ScreenTools.largeFontPointSize
            anchors.centerIn:   parent
        }
    }

    Tx1Controller{
        id : tx1Controller
        onTx1StatusTrackedPossibleTargetChanged:{
            if(rb_x==-101){
                trackedZoom.visible = false
                trackedZoom1.visible = true

            }
            if(rb_x==-100){

                selectedZoom.visible=false
            }

            if(rb_x==-106){
                selectedZoom.visible=true

            }
        }

        onTx1StatusTrackedTargetChanged:{
           // trackedTimer.stop()
            if(c_x==0){
                //console.log("CCCCCCCCCCCCCAAAAAAAAAAAA")
                if(trackedZoom.visible == false){
                    trackedZoom.visible = true
                    if(!tracking){
                      //  num=0
                        trackedZoom.visible = false
                    }
                }
                // num++
               // console.log("num==="+num)
                //trackedZoom.rotation = rotation
                trackedZoom.x = root.width*tl_x/1280
                trackedZoom1.x = trackedZoom.x
                var x2 =  root.width*rb_x/1280
                var y2 =  root.height*rb_y/720
                trackedZoom.y = root.height*tl_y/720
                trackedZoom1.y =trackedZoom.y
                trackedZoom.width = Math.abs(x2 - trackedZoom.x)
                trackedZoom1.width =trackedZoom.width
                trackedZoom.height = Math.abs(y2 - trackedZoom.y)
                trackedZoom1.height =trackedZoom.height
                trackedZoom1.visible = false

            }
           else{
                c=root.width*tl_x/1280
                d=root.height*tl_y/720
                e=root.width*rb_x/1280-root.width*tl_x/1280
                f=root.height*rb_y/720-root.height*tl_y/720
                var tt = Qt.createQmlObject(
                                            "import QtQuick                     2.5; " +
                                            "import QtLocation                  5.3; " +
                                            "import QGroundControl.ScreenTools  1.0; " +
                                            "Rectangle {" +
                                            "   id:     vertexDrag; " +
                                            "   width:  5; " +
                                            "   height: 5; " +
                                            "   color:  'blue'; " +
                                            "   opacity: 0.3;"+
                                            "} ",
                                            root)
                tt.x=c
                tt.y=d
                tt.width=e
                tt.height=f
               tt.visible=true
                if(kk!=c_y||c_y==1){
                    for(var i=0; i<boundboxArray.length;i++){
                         boundboxArray[i].visible = false

                    }
                    boundboxArray.splice(1,1)
                    //boundboxArray.length = 0
                }
                 boundboxArray.push(tt)
                kk=c_y



            }

           // trackedTimer.start()
            //console.log("Target : " + rotation + " " + tl_x + " " + tl_y + " " + rb_x + " " + rb_y
            //            + " " + Math.abs(rb_x - tl_x) + " " + Math.abs(rb_y - tl_y))
        }
        onTx1StatusFeaturePointChanged:{

//            m = root.width*x/1280
//            b = root.height*y/720
//           var tt = Qt.createQmlObject(
//                                       "import QtQuick                     2.5; " +
//                                       "import QtLocation                  5.3; " +
//                                       "import QGroundControl.ScreenTools  1.0; " +
//                                       "Rectangle {" +
//                                       "   id:     vertexDrag; " +
//                                       "   width:  5; " +
//                                       "   height: 5; " +
//                                       "   color:  'red'; " +
//                                       "} ",
//                                       root)
//           tt.x = m
//           tt.y = b
//           apointArrary.push(tt)
//            if(k!=z){
//                for(var i=0; i<apointArrary.length;i++){
//                    apointArrary[i].visible = false
//                }
//                apointArrary.length = 0
//            }
//            k=z



        }
    }
        Timer{
            id: trackFlag
            interval: 100
            running: false
            repeat:true

            onTriggered: {
                //console.log(apointArrary.length+"KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK")
                           trackTime++
            }
        }
    QGCVideoBackground {
        id:             videoBackgroundss
        anchors.fill:   parent
        display:        QGroundControl.videoManager.videoSurface
        receiver:       QGroundControl.videoManager.videoReceiver
    }
    PinchArea{
        anchors.fill: parent
//        pinch.maximumX: root.width*1.0
//        pinch.minimumX: 0
//        pinch.maximumY: root.height*1.0
//        pinch.minimumY: 0
        pinch.maximumScale: 10
        pinch.minimumScale: 0.1
        //pinch.dragAxis:  Pinch.XAndYAxis
         // pinch.dragAxis:Pinch.NoDrag

       // pinch.target:selectedZoom
        onPinchUpdated: {
            //console.log("angle=="+pinch.angle)
            //console.log("scale=="+pinch.scale)
          //  console.log("selectedZoom.x=="+selectedZoom.x)
          //  console.log("selectedZoom.y=="+selectedZoom.y)
           // console.log("selectedZoom.width=="+selectedZoom.width)
          //  console.log("selectedZoom.height=="+selectedZoom.height)
            var zz
            if(scal>0)zz=pinch.scale-scal
            if(scal>0){
            if(((-20<=pinch.angle&&pinch.angle<=20) || (-180<=pinch.angle&&pinch.angle<=-160) || (160<=pinch.angle&&pinch.angle<=180))&&zz>0){
              //  console.log("WWWWWWWWW")
                selectedZoom.width=selectedZoom.width+6
                selectedZoom.height=selectedZoom.height
            }
            else if(((-20<=pinch.angle&&pinch.angle<=20) || (-180<=pinch.angle&&pinch.angle<=-160) || (160<=pinch.angle&&pinch.angle<=180))&&zz<0){
                selectedZoom.width=selectedZoom.width-6
                selectedZoom.height=selectedZoom.height
            }
            else if(((70<=pinch.angle&&pinch.angle<=110) || (-110<=pinch.angle&&pinch.angle<=-70))&&zz>0){
              //  console.log("HAHAHA")
                selectedZoom.width=selectedZoom.width
                selectedZoom.height=selectedZoom.height+6
            }
            else if(((70<=pinch.angle&&pinch.angle<=110) || (-110<=pinch.angle&&pinch.angle<=-70))&&zz<0){
                selectedZoom.width=selectedZoom.width
                selectedZoom.height=selectedZoom.height-6
            }
            else if(((20<pinch.angle&&pinch.angle<70) || (110<pinch.angle&&pinch.angle<160) || (-160<pinch.angle&&pinch.angle<-110) || (-70<pinch.angle&&pinch.angle<-20))&&zz>0){
                selectedZoom.width=selectedZoom.width+6
                selectedZoom.height=selectedZoom.height+6
            }
            else if(((20<pinch.angle&&pinch.angle<70) || (110<pinch.angle&&pinch.angle<160) || (-160<pinch.angle&&pinch.angle<-110) || (-70<pinch.angle&&pinch.angle<-20))&&zz<0){
                selectedZoom.width=selectedZoom.width-6
                selectedZoom.height=selectedZoom.height-6
            }
            }
                 scal = pinch.scale
        }

onPinchFinished: {
    //console.log("pinch finished")
    //console.log("selectedZoom.width=="+selectedZoom.width)
   // console.log("selectedZoom.x=="+selectedZoom.x)
}
    MouseArea {
        id:           menuMouse
        visible:      true//!QGroundControl.videoManager.videoRunning
        anchors.fill: parent
        hoverEnabled: false//true
        property string buttonID

            acceptedButtons: Qt.AllButtons
    //            selectedZoom.x = menuMouse.mouseX;
            //        onPressed: {
            //            selectedZoom.visible = false;
    //            selectedZoom.y = menuMouse.mouseY;
    //            //mouse.accepted = false//true
    //            _selectZoom = true
    //        }

    //        onReleased: {
    //            if(selectedZoom.visible){
    //                selectedZoom.visible = false
    //                var rb_x = selectedZoom.x + selectedZoom.width
    //                var rb_y = selectedZoom.y + selectedZoom.height
    //                // start tracked
    //                tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
    //                                                selectedZoom.x, selectedZoom.y
    //                                                , selectedZoom.x + selectedZoom.width, selectedZoom.y + selectedZoom.height)
    //                console.log("onReleased :" + selectedZoom.x +" : "+selectedZoom.y + " : "+ rb_x + " : " + rb_x);
    //            }
    //            _selectZoom = false
    //        }

            //onPressAndHold: console.log('Press and hold')
            onPressed: {
                //selectedZoom.scale = 1
                if(ScreenTools.isMobile){
                    if(selectedZoom.width==0){
                        selectedZoom.width = 100
                        selectedZoom.height = 100
                    }

                    selectedZoom.x = menuMouse.mouseX - selectedZoom.width/2;
                    selectedZoom.y = menuMouse.mouseY - selectedZoom.height/2;

                    selectedZoom.visible = true
                    trackFlag.start()
                    _selectZoom = true
                }
               else{
                if(mouse.button == Qt.LeftButton){

                   tracking=true
                  // console.log("TTTTTTTTTTTTTTTtt===="+trackTime)
                    if(selectedZoom.width==0){
                        selectedZoom.width = 100
                        selectedZoom.height = 100
                    }

                    selectedZoom.x = menuMouse.mouseX - selectedZoom.width/2;
                    selectedZoom.y = menuMouse.mouseY - selectedZoom.height/2;

                    selectedZoom.visible = true

                    _selectZoom = true
                }else{
                    //trackFlag.start()
                }

}

            }

            onWheel:{
               // console.log("AAAAAAAAAAAAAAAAAAAAAAAAaa")
//                if(onwheelTimer.running == true)
//                    onwheelTimer.restart()
//                else
//                    onwheelTimer.start()
                if(wheel.modifiers & Qt.ShiftModifier){
                    if(wheel.angleDelta.y > 0){
                        selectedZoom.width += 4
                        selectedZoom.x     -= 2

                    }else{
                        selectedZoom.width -= 4
                        selectedZoom.x     += 2
                    }
                }else{
                    if(wheel.angleDelta.y > 0){
                        selectedZoom.width += 4
                        selectedZoom.x     -= 2

                        selectedZoom.height += 4
                        selectedZoom.y      -= 2
                    }else{
                        selectedZoom.width -= 4
                        selectedZoom.x     += 2

                        selectedZoom.height -= 4
                        selectedZoom.y      += 2
                    }
                }

            }
            onReleased: {
                if(ScreenTools.isMobile){
                    _selectZoom = false
                    tracking=true

                    trackFlag.stop()
                    if(trackTime>=30){
                        tracking=false
                    }

                    if(tracking){

                        tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
                                                    1280*selectedZoom.x/root.width, 720*selectedZoom.y/root.height
                                                    , 1280*selectedZoom.x/root.width + 1280*selectedZoom.width/root.width, 720*selectedZoom.y/root.height+ 720*selectedZoom.height/root.height)
                    trackTime=0


                    }else{
                        tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
                                                        -101, 0
                                                        , 0, 0)
                        trackTime=0
                        trackedZoom.visible=false
                       trackedZoom.height = 0
                       trackedZoom.width  = 0
                    }
                }
else{
                if(mouse.button == Qt.LeftButton){


                        _selectZoom = false
                        tracking=true
                       // trackTime=0
                        if(QGroundControl.videoManager.trackFlag){
                            tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
                                                        1280*selectedZoom.x/root.width, 720*selectedZoom.y/root.height
                                                        , 1280*selectedZoom.x/root.width + 1280*selectedZoom.width/root.width, 720*selectedZoom.y/root.height+ 720*selectedZoom.height/root.height)
                        }else{
                            console.log("FFFFFFFFFFFFFFFFFFFFF")
                            locationArray.length=0
                            locationArray.push(parseInt(1280*selectedZoom.x/root.width))
                            locationArray.push(parseInt(720*selectedZoom.y/root.height))
                            locationArray.push(parseInt(1280*selectedZoom.x/root.width+ 1280*selectedZoom.width/root.width))
                            locationArray.push(parseInt(720*selectedZoom.y/root.height+ 720*selectedZoom.height/root.height))
                            for(var i=0;i<locationArray.length;i++){
                                console.log("locationArray[i]=="+locationArray[i])
                            }

                            QGroundControl.videoManager.setLocation(locationArray)

                        }

                }

                if(mouse.button == Qt.RightButton){
                   // trackFlag.stop()
                  //  if(trackTime>=30){
                        tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
                                                        -101, 0
                                                        , 0, 0)
                        tracking=false
                       // trackTime=0
                    trackedZoom1.visible = false
                     trackedZoom.visible=false
                    trackedZoom.height = 0
                    trackedZoom.width  = 0
                        //console.log("stop TRAcking")
                   // }

                }
}
            }

            onPositionChanged:{
               // console.log("position changed")
                if(_selectZoom){
//                    var tmpx = menuMouse.mouseX - selectedZoom.x;
//                    var tmpy = menuMouse.mouseY - selectedZoom.y;
//                    selectedZoom.width = 0;
//                    if(tmpx >= 5 && tmpy >= 5){
//                        selectedZoom.visible = true;
//                        selectedZoom.width = tmpx;
//                        selectedZoom.height = tmpy;
//                    }else{
//                        selectedZoom.visible = false;
//                        selectedZoom.height = 0;
//                        selectedZoom.height = 0
//                    }

                    selectedZoom.visible = true;
                    var x = menuMouse.mouseX - selectedZoom.width/2;
                    var y = menuMouse.mouseY - selectedZoom.height/2;
                    selectedZoom.x = x
                    selectedZoom.y = y
                }
            }
        }
    }
//    Timer{
//        id: pointCounter
//        interval: 200
//        running: true
//        repeat:true

//        onTriggered: {
//            //console.log(apointArrary.length+"KKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK")
//                        for(var i=0; i<apointArrary.length;i++){
//                            apointArrary[i].visible = false
//                        }
//                        apointArrary.length = 0
//        }
//    }
    Timer{
        id: onwheelTimer
        interval: 500
        repeat:false
        running: false
        triggeredOnStart: false

        onTriggered: {
//            tx1Controller.controlTx1Manager(Tx1Controller.Tx1PipeTrack, 1,
//                                            1280*selectedZoom.x/root.width, 720*selectedZoom.y/root.height
//                                            , 1280*selectedZoom.x/root.width + 50, 720*selectedZoom.y/root.height+ 50)
        }
    }

    Loader{
           id:      selectedZoom
           visible: false
           enabled: visible
//           foucs : true
           sourceComponent:Component{
               Rectangle{
                   id:           selectRect
                   opacity:      0.8
                   color:        Qt.rgba(0,0,0,0.0)
                   anchors.fill: parent
                   Rectangle{
                       anchors.top:  selectRect.top
                       anchors.left: selectRect.left
                       opacity:      0.8
                       height:       6
                       width:        parent.width / 5 * 2
                       color:        "yellow"
                   }
                   Rectangle{
                       anchors.top:  selectRect.top
                       anchors.left: selectRect.left
                       opacity:      0.8
                       height:       parent.height / 5 * 2
                       width:        6
                       color:        "yellow"
                   }

                   Rectangle{
                       anchors.top:   selectRect.top
                       anchors.right: selectRect.right
                       opacity:       0.8
                       height:        6
                       width:         parent.width / 5 * 2
                       color:         "yellow"
                   }
                   Rectangle{
                       anchors.top:   selectRect.top
                       anchors.right: selectRect.right
                       opacity:       0.8
                       height:        parent.height / 5 * 2
                       width:         6
                       color:         "yellow"
                   }

                   Rectangle{
                       anchors.bottom: selectRect.bottom
                       anchors.left:   selectRect.left
                       opacity:        0.8
                       height:         6
                       width:          parent.width / 5 * 2
                       color:          "yellow"
                   }
                   Rectangle{
                       anchors.bottom: selectRect.bottom
                       anchors.left:   selectRect.left
                       opacity:        0.8
                       height:         parent.height / 5 * 2
                       width:          6
                       color:          "yellow"
                   }

                   Rectangle{
                       anchors.bottom: selectRect.bottom
                       anchors.right:  selectRect.right
                       opacity:        0.8
                       height:         6
                       width:          parent.width / 5 * 2
                       color:          "yellow"
                   }
                   Rectangle{
                       anchors.bottom: selectRect.bottom
                       anchors.right:  selectRect.right
                       opacity:        0.8
                       height:         parent.height / 5 * 2
                       width:          6
                       color:          "yellow"
                   }
               }
           }
       }

    Timer {
        id:       trackedTimer
        interval: 1500
        running:  false
        repeat:   false
        triggeredOnStart: false
        onTriggered: {
           // console.log("Timer trackedTimer")
            trackedZoom.visible = false
            trackedZoom.height = 0
            trackedZoom.width  = 0
            //trackedZoom.rotation = 0
        }
    }

    Loader{
           id:       trackedZoom
           visible: false
           enabled: visible
           sourceComponent:Component{
               Rectangle{
                   id:           trackedRect
                   opacity:      0.8
                   color:        Qt.rgba(0,0,0,0.0)
                   anchors.fill: parent
                   //rotation:     rotation
                   Rectangle{
                       anchors.top:  trackedRect.top
                       anchors.left: trackedRect.left
                       opacity:      0.8
                       height:       6//2
                       width:        parent.width / 5 * 2
                       color:        "red"
                   }
                   Rectangle{
                       anchors.top:  trackedRect.top
                       anchors.left: trackedRect.left
                       opacity:      0.8
                       height:       parent.height / 5 * 2
                       width:        6//2
                       color:        "red"
                   }

                   Rectangle{
                       anchors.top:   trackedRect.top
                       anchors.right: trackedRect.right
                       opacity:       0.8
                       height:        6//2
                       width:         parent.width / 5 * 2
                       color:         "red"
                   }
                   Rectangle{
                       anchors.top:   trackedRect.top
                       anchors.right: trackedRect.right
                       opacity:       0.8
                       height:        parent.height / 5 * 2
                       width:         6//2
                       color:         "red"
                   }

                   Rectangle{
                       anchors.bottom: trackedRect.bottom
                       anchors.left:   trackedRect.left
                       opacity:        0.8
                       height:         6//2
                       width:          parent.width / 5 * 2
                       color:          "red"
                   }
                   Rectangle{
                       anchors.bottom: trackedRect.bottom
                       anchors.left:   trackedRect.left
                       opacity:        0.8
                       height:         parent.height / 5 * 2
                       width:          6//2
                       color:          "red"
                   }

                   Rectangle{
                       anchors.bottom: trackedRect.bottom
                       anchors.right:  trackedRect.right
                       opacity:        0.8
                       height:         6//2
                       width:          parent.width / 5 * 2
                       color:          "red"
                   }
                   Rectangle{
                       anchors.bottom: trackedRect.bottom
                       anchors.right:  trackedRect.right
                       opacity:        0.8
                       height:         parent.height / 5 * 2
                       width:          6//2
                       color:          "red"
                   }
               }
           }
       }
    Loader{
           id:       trackedZoom1
           visible: false
           enabled: visible
           sourceComponent:Component{
               Rectangle{
                   id:           trackedRect1
                   opacity:      0.8
                   color:        Qt.rgba(0,0,0,0.0)
                   anchors.fill: parent
                   //rotation:     rotation
                   Rectangle{
                       anchors.top:  trackedRect1.top
                       anchors.left: trackedRect1.left
                       opacity:      0.8
                       height:       2
                       width:        parent.width / 5 * 2
                       color:        "red"
                   }
                   Rectangle{
                       anchors.top:  trackedRect1.top
                       anchors.left: trackedRect1.left
                       opacity:      0.8
                       height:       parent.height / 5 * 2
                       width:        2
                       color:        "red"
                   }

                   Rectangle{
                       anchors.top:   trackedRect1.top
                       anchors.right: trackedRect1.right
                       opacity:       0.8
                       height:        2
                       width:         parent.width / 5 * 2
                       color:         "red"
                   }
                   Rectangle{
                       anchors.top:   trackedRect1.top
                       anchors.right: trackedRect1.right
                       opacity:       0.8
                       height:        parent.height / 5 * 2
                       width:         2
                       color:         "red"
                   }

                   Rectangle{
                       anchors.bottom: trackedRect1.bottom
                       anchors.left:   trackedRect1.left
                       opacity:        0.8
                       height:         2
                       width:          parent.width / 5 * 2
                       color:          "red"
                   }
                   Rectangle{
                       anchors.bottom: trackedRect1.bottom
                       anchors.left:   trackedRect1.left
                       opacity:        0.8
                       height:         parent.height / 5 * 2
                       width:          2
                       color:          "red"
                   }

                   Rectangle{
                       anchors.bottom: trackedRect1.bottom
                       anchors.right:  trackedRect1.right
                       opacity:        0.8
                       height:         2
                       width:          parent.width / 5 * 2
                       color:          "red"
                   }
                   Rectangle{
                       anchors.bottom: trackedRect1.bottom
                       anchors.right:  trackedRect1.right
                       opacity:        0.8
                       height:         parent.height / 5 * 2
                       width:          2
                       color:          "red"
                   }
               }
           }
       }
    QGCMapPalette { id: videoPal; lightColors: isBackgroundDark }

    //-- Video Vertical Tool Buttons
    FlightDisplayVideoViewWidgets{
        id: videoVideWidgets
    }
}
