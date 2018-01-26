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
import QtLocation                   5.3
import QtPositioning                5.2
import QtQuick.Dialogs              1.2
import QGroundControl               1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0

FlightMap {
    id:             flightMap
    anchors.fill:   parent
    mapName:        _mapName

    property alias  missionController: missionController
    property var    flightWidgets
    property var   screenwidth:parent.width
    property var   screenheight:parent.height
    property bool   _followVehicle:                 true
    property var    _activeVehicle:                 QGroundControl.multiVehicleManager.activeVehicle
    property bool   _activeVehicleCoordinateValid:  _activeVehicle ? _activeVehicle.coordinateValid : false
    property var    activeVehicleCoordinate:        _activeVehicle ? _activeVehicle.coordinate : QtPositioning.coordinate()
    property var    _gotoHereCoordinate:            QtPositioning.coordinate()
    property int    _retaskSequence:                0
    property var    _visualItems:          missionController.visualItems
    property var    _currentMissionItem
    property var mm:0
    property var nn:0
    property var mmm:0
    property var num1:0
    property var nnn:0
    property var md:0
    property var nd:0
    property var flagg:0
    property var num:0
    property  var co1:QtPositioning.coordinate(37.803784, -122.462276)
    property var co2:QtPositioning.coordinate(37.803784, -122.462276)
    property var co1x:0
    property var co2y:0
    property var largNum:1
    property var smalNum:1
    property var angleNum:0
    property var angle : 0
    property var waypointlatiOffsetArray : new Array
    property var waypointlonOffsetArray: new Array
    property var waypointlatiArray : new Array
    property var waypointlongArray : new Array

    property real _middleRowWidth:  ScreenTools.defaultFontPixelWidth * 20
    property real _editFieldWidth:  ScreenTools.defaultFontPixelWidth * 14

    property var xx:{
        if(missionController.visualItems)
            return flightMap.fromCoordinate(missionController.visualItems.get(0).coordinate).x
        return -1
    }

    property var yy:{
        if(missionController.visualItems)
            flightMap.fromCoordinate(missionController.visualItems.get(0).coordinate).y
        return -1
    }
    property var clickxx:0
    property var clickyy:0
    property var originalx:0
    property var originaly:0
    property var multiple:0
    property var mousecoord:QtPositioning.coordinate(37.803784, -122.462276)
    property var ff:1
    property bool fff:false
    property bool ffff:false
    property bool currentitemflag:false
    property var rotateFlag:0
    property var rotate:0
    property var coordinatee:QtPositioning.coordinate(37.803784, -122.462276)
    Component.onCompleted: {
        QGroundControl.flightMapPosition = center
        QGroundControl.flightMapZoom = zoomLevel
    }
    onCenterChanged: {
        if(clickxx!==0){
            clickxx = flightMap.fromCoordinate(mousecoord).x
            clickyy = flightMap.fromCoordinate(mousecoord).y
            ima4.x = clickxx-image5.width
            ima4.y = clickyy-image5.width*1.732
            im.x = clickxx+image6.width
            im.y = clickyy-image6.width*1.732
            ima.x = clickxx-image1.width*2
            ima1.x = clickxx+image2.width*2
            ima2.x = clickxx-image3.width
            ima2.y = clickyy+image3.width*1.732
            ima3.x = clickxx+image4.width
            ima3.y = clickyy+image4.width*1.732
        }
        QGroundControl.flightMapPosition = center
    }
    onZoomLevelChanged: {
        if(clickxx!==0){
            clickxx = flightMap.fromCoordinate(mousecoord).x
            clickyy = flightMap.fromCoordinate(mousecoord).y
            ima4.x = clickxx-image5.width
            ima4.y = clickyy-image5.width*1.732
            im.x = clickxx+image6.width
            im.y = clickyy-image6.width*1.732
            ima.x = clickxx-image1.width*2
            ima1.x = clickxx+image2.width*2
            ima2.x = clickxx-image3.width
            ima2.y = clickyy+image3.width*1.732
            ima3.x = clickxx+image4.width
            ima3.y = clickyy+image4.width*1.732
        }
        QGroundControl.flightMapZoom = zoomLevel
    }

    onActiveVehicleCoordinateChanged: {
        if (_followVehicle && _activeVehicleCoordinateValid && activeVehicleCoordinate.isValid) {
            _initialMapPositionSet = true
            flightMap.center  = activeVehicleCoordinate
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    function normalizeLat(lat) {
        // Normalize latitude to range: 0 to 180, S to N
        return lat + 90.0
    }
    function normalizeLon(lon) {
        // Normalize longitude to range: 0 to 360, W to E
        return lon  + 180.0
    }
    /// Fix the map viewport to the current mission items.
    function fitViewportToMissionItems() {
        if (_visualItems.count == 1) {
            flightMap.center = _visualItems.get(0).coordinate
        } else {
            var missionItem = _visualItems.get(1)
            for(var i = 1; i < _visualItems.count; i++){
                missionItem = _visualItems.get(i)
                if(missionItem.specifiesCoordinate && !missionItem.isStandaloneCoordinate){
                    break;
                }
            }
            var north = normalizeLat(missionItem.coordinate.latitude)
            var south = north
            var east = normalizeLon(missionItem.coordinate.longitude)
            var west = east
            for (var i=1; i<_visualItems.count; i++) {
                missionItem = _visualItems.get(i)
                if (missionItem.specifiesCoordinate && !missionItem.isStandaloneCoordinate) {
                    var lat = normalizeLat(missionItem.coordinate.latitude)
                    var lon = normalizeLon(missionItem.coordinate.longitude)
                    north = Math.max(north, lat)
                    south = Math.min(south, lat)
                    east = Math.max(east, lon)
                    west = Math.min(west, lon)
                }
            }
            flightMap.visibleRegion = QtPositioning.rectangle(QtPositioning.coordinate(north - 90.0, west - 180.0), QtPositioning.coordinate(south - 90.0, east - 180.0))
        }
    }

    function changMissionItemsTemp(latitude, longtitude, altitude){
        tempMissionItemsTimer.stop()
        if(tempMissionItems.visible == false){
            tempMissionItems.visible = true
        }
        tempMissionItems.coordinate = QtPositioning.coordinate(latitude, longtitude, altitude)
        tempMissionItemsTimer.start()
    }

    Timer{
        id: tempMissionItemsTimer
        interval:   100000
        running:    false
        repeat:     false
        onTriggered:{
            tempMissionItems.visible = false
        }
    }

    MapQuickItem{
        id:             tempMissionItems
        visible:        false
        z:              QGroundControl.zOrderMapItems
        anchorPoint.x:  sourceItem.width  / 2
        anchorPoint.y:  sourceItem.height / 2

        sourceItem:
            Rectangle {
            property real   imageSize:{
                var tempMissionItemsSize = ScreenTools.defaultFontPixelHeight * 3
                return 20 > tempMissionItemsSize ? tempMissionItemsSize : 20
            }
            width:          imageSize
            height:         imageSize
            radius:         imageSize / 2
            color:          "green"//Qt.rgba(0,0,0,0.75)
            border.color: "black"
            border.width: 1
        }
    }
    MissionController {
        id: missionController
        Component.onCompleted: start(false /* editMode */)
        onVisualItemsChanged: {
            if(ima.visible){
                im.visible = false
                ima.visible = false
                ima1.visible = false
                ima2.visible = false
                ima3.visible = false
                ima4.visible = false
            }
        }
        onNewItemsFromVehicle: fitViewportToMissionItems()
        onActiveVehicleMissionItemsTempChanged: changMissionItemsTemp(latitude, longtitude, altitude)
    }

    GeoFenceController {
        id: geoFenceController
        Component.onCompleted: start(false /* editMode */)
    }

    RallyPointController {
        id: rallyPointController
        Component.onCompleted: start(false /* editMode */)
    }

    // Add follow me trajectory points to the map
    MapItemView {
        model: _mainIsMap ? _activeVehicle ? _activeVehicle.followMePoints : 0 : 0
        delegate:
            MapPolyline {         
            line.width: 2
            line.color: "white"
            //z:          QGroundControl.zOrderMapItems - 1
            path: [
                object.coordinate1,
                object.coordinate2,
            ]
        }
    }

    // Add trajectory points to the map
    MapItemView {
        model: _mainIsMap ? _activeVehicle ? _activeVehicle.trajectoryPoints : 0 : 0
        delegate:
            MapPolyline {
            line.width: 1
            line.color: "red"
            z:          QGroundControl.zOrderMapItems - 1
            path: [
                object.coordinate1,
                object.coordinate2,
            ]
        }
    }

    // Add the vehicles to the map
    MapItemView {
        model: QGroundControl.multiVehicleManager.vehicles
        delegate:
            VehicleMapItem {
            vehicle:        object
            vehicletype:    object.vehicletype
            coordinate:     object.coordinate
            isSatellite:    flightMap.isSatelliteMap
            size:           ScreenTools.defaultFontPixelHeight * 4
            z:              QGroundControl.zOrderMapItems - 1
        }
    }

    function setCurrentItem(sequenceNumber) {
        _currentMissionItem = undefined
        for (var i=0; i<_visualItems.count; i++) {
            var visualItem = _visualItems.get(i)
            if (visualItem.sequenceNumber == sequenceNumber) {
                _currentMissionItem = visualItem
                _currentMissionItem.isCurrentItem = true
                currentitemflag = true
           } else {
                visualItem.isCurrentItem = false
            }
        }
    }
    function getCurrentItem(){
        return _currentMissionItem
    }
    Rectangle {
        id:             itemDragger
        x:              missionItemIndicator ? (missionItemIndicator.x + missionItemIndicator.anchorPoint.x - (itemDragger.width / 2)) : 100
        y:              missionItemIndicator ? (missionItemIndicator.y + missionItemIndicator.anchorPoint.y - (itemDragger.height / 2)) : 100
        width:          ScreenTools.defaultFontPixelHeight * 2
        height:         ScreenTools.defaultFontPixelHeight * 2
        color:          "transparent"
        visible:        false
        z:              QGroundControl.zOrderMapItems + 1    // Above item icons

        property var    missionItem
        property var    missionItemIndicator
        property bool   preventCoordinateBindingLoop: false

        onXChanged: liveDrag()
        onYChanged: liveDrag()


        function liveDrag() {
            if (!itemDragger.preventCoordinateBindingLoop && Drag.active) {
                var point = Qt.point(itemDragger.x + (itemDragger.width  / 2), itemDragger.y + (itemDragger.height / 2))
                var coordinate =  flightMap.toCoordinate(point)

                coordinate.altitude = itemDragger.missionItem.coordinate.altitude
                itemDragger.preventCoordinateBindingLoop = true
                itemDragger.missionItem.coordinate = coordinate
                itemDragger.preventCoordinateBindingLoop = false
            }
        }

        function clearItem() {
            itemDragger.visible = false
            itemDragger.missionItem = undefined
            itemDragger.missionItemIndicator = undefined
        }

        Drag.active:    itemDrag.drag.active
        Drag.hotSpot.x: width  / 2
        Drag.hotSpot.y: height / 2
        Timer{
            id:timer1
            interval:150
            running:false
            repeat:true
            onTriggered: itemDragger.dodo()
        }
        function get()
        {
            ffff = false
            md =  itemDragger.x
            nd =  itemDragger.y
            im.visible = false
            ima.visible = false
            ima1.visible = false
            ima2.visible = false
            ima3.visible = false
            ima4.visible = false
            itemDragger.visible=true
            timer1.start()
            var m = missionController.visualItems.get(1);
            m.flage=true

        }
        function dodo()
        {
            missionController.loadMoveLine()
        }
        function recover()
        {
            itemDragger.visible=false
            timer1.stop()
            missionController.loadRecover()
            flagg=1
            messageDialog.visible = true
        }
        MouseArea {
            id:             itemDrag
            anchors.fill:   parent
            drag.target:    parent
            drag.minimumX:  0
            drag.minimumY:  0
            drag.maximumX:  itemDragger.parent.width - parent.width
            drag.maximumY:  itemDragger.parent.height - parent.height
            onPressed:itemDragger.get()
            onReleased:itemDragger.recover()

        }
    }

    MapItemView {
        model:     missionController.visualItems
        delegate:   missionItemComponent
    }
    CameraFeedItemView{
        model: _mainIsMap ? missionController.cameraFeedItems : 0
    }
    //==============================================================================

    Rectangle{
        id:          sendlatitude
        anchors.horizontalCenter:   parent.horizontalCenter
        anchors.verticalCenter:     parent.verticalCenter
        opacity:     0.7
        width:      check1.width+sendlabel.width+textField.width+ScreenTools.defaultFontPixelWidth*4
        height:     textField.height+textField1.height+sendbutto.height+ScreenTools.defaultFontPixelWidth*4
        color:      "green"
        visible:    false
        radius:     2
        z:          QGroundControl.zOrderWidgets+1
        border.width: 1
        border.color: "yellow"
        Column{
            anchors.fill: parent
            anchors.margins: ScreenTools.defaultFontPixelWidth
            spacing: ScreenTools.defaultFontPixelWidth
            Row{
                spacing: ScreenTools.defaultFontPixelWidth
                QGCCheckBox{
                    id:check1
                    checked:false
                    anchors.baseline:   textField.baseline
                }
                QGCLabel{
                    id:sendlabel
                    width:      _middleRowWidth
                    text:qsTr("Altitude increment:")
                    color:"white"
                    anchors.baseline:   textField.baseline
                }
                QGCTextField
                {
                    id:textField
                    validator:          DoubleValidator {}
                    text: "0.0"
                    width:              _editFieldWidth
                }
            }
            Row{
                spacing: ScreenTools.defaultFontPixelWidth
                QGCCheckBox{
                    id:check2
                    checked:false
                    anchors.baseline:   textField1.baseline
                }
                QGCLabel{
                    id:sendlabel1
                    width:      _middleRowWidth
                    text:qsTr("Speed increment:")
                    color:"white"
                    anchors.baseline:   textField1.baseline
                }
                QGCTextField
                {
                    id:textField1
                    validator:          DoubleValidator {}
                    text: "0.0"
                    width:              _editFieldWidth
                }
            }
            Row{
                spacing: ScreenTools.defaultFontPixelWidth+textField.width+8
                QGCButton{
                    id:           sendbutto
                    text:         qsTr("Send", "sendMsg")
                    onClicked:{
                        image5.checked=false
                        if(check1.checked){
                            missionController.sendAltitudee(textField.text)
                        }
                        if(check2.checked){
                            _activeVehicle.doCommandLon(0,178,0.0,textField1.text,-1.0,0.0,0.0,0.0,0.0)
                        }
                        sendlatitude.visible=false
                        timersend.start()
                    }
                }
                QGCButton{
                    id:            close
                    text:          qsTr("Close")
                    onClicked:{
                        image5.checked=false
                        sendlatitude.visible=false
                    }
                }
            }
        }

    }

    Timer{
        id:timersend
        interval:500
        running:false
        repeat:true
        onTriggered: flightMap.sendd()
    }
    function sendd(){
        num++
        missionController.sendFlagg()
        if(missionController.visualItemsflag.count>0){
            num=0
            num1=0
            //================================================
            var coord = flightMap.toCoordinate(Qt.point(clickxx,clickyy))
            var coordinatee1 = flightMap.toCoordinate(Qt.point(mmm,nnn))
            var coordinateee2 = flightMap.toCoordinate(Qt.point(co1x,co2y))

            var ref_lon_rad = coord.longitude * 3.141592654/180;
            var ref_lat_rad = coord.latitude * 3.141592654/180;
            for(var i=0;i<missionController.visualItems.count;i++){
                var m = missionController.visualItems.get(i)
                if(flagg==2){
                    if((coordinatee1.latitude-coordinateee2.latitude)-missionController.lati<=0.000005){
                        m.coordinate.latitude =  m.coordinate.latitude//-(coordinatee1.latitude-coordinateee2.latitude)
                        m.coordinate.longitude =  m.coordinate.longitude//-(coordinatee1.longitude-coordinateee2.longitude)
                    }
                }
                if(flagg==3){
                    if((Math.pow(1+multiple,largNum-1)-missionController.multi<=0.0001)){
//                        m.coordinate.latitude =  (m.coordinate.latitude - coord.latitude)/Math.pow(1+multiple,largNum-1)+coord.latitude
//                        m.coordinate.longitude =  (m.coordinate.longitude - coord.longitude)/Math.pow(1+multiple,largNum-1)+coord.longitude
                        if(m.sequenceNumber != 0){
                            console.log("================")
                            m.coordinate.latitude = waypointlatiOffsetArray[i-1] * Math.pow(1 + multiple,largNum - 1) + coord.latitude
                            m.coordinate.longitude = waypointlonOffsetArray[i-1] * Math.pow(1 + multiple,largNum - 1) + coord.longitude
                            console.log(waypointlatiOffsetArray[i-1]  + "  " + waypointlonOffsetArray[i-1])
                        }
                    }
                }
                if(flagg==4){
                    if((Math.pow(1-multiple,smalNum-1)-missionController.multi<=0.0001)){
//                        m.coordinate.latitude =  (m.coordinate.latitude-coord.latitude)/Math.pow(1-multiple,smalNum-1)+coord.latitude
//                        m.coordinate.longitude =  (m.coordinate.longitude-coord.longitude)/Math.pow(1-multiple,smalNum-1)+coord.longitude
                        if(m.sequenceNumber != 0){
                            console.log("---------------------")
                            m.coordinate.latitude = waypointlatiOffsetArray[i-1] * Math.pow(1 - multiple,smalNum - 1) + coord.latitude
                            m.coordinate.longitude = waypointlonOffsetArray[i-1] * Math.pow(1 - multiple,smalNum - 1) + coord.longitude
                        }

                    }
                }
                if(flagg==5){
                    var angle
                    if(angleNum <= 50){
                        rotate = 0.1
                        angle = angleNum * rotate
                    }else if(angleNum <= 100){
                        rotate = 0.2
                        angle = rotate *(angleNum - 50) + 5
                    }else if(angleNum <= 150){
                        rotate =  0.3
                        angle = rotate * (angleNum -100) + 15
                    }else if(angleNum <= 200){
                        rotate = 0.4
                        angle = rotate * (angleNum - 150) + 30
                    }else if(angleNum > 200){
                        rotate = 0.5
                        angle = rotate * (angleNum - 200) + 50
                    }
                    if(missionController.multi/10000-angle<=0.00001){
                        if(m.sequenceNumber!==0){
                            m.coordinate.latitude =  m.coordinate.latitude
                            m.coordinate.longitude =  m.coordinate.longitude
                        }
                    }
                }
                if(flagg==6){
                    var angle
                    if(angleNum <= 50){
                        rotate = 0.1
                        angle = angleNum * rotate
                    }else if(angleNum <= 100){
                        rotate = 0.2
                        angle = rotate *(angleNum - 50) + 5
                    }else if(angleNum <= 150){
                        rotate =  0.3
                        angle = rotate * (angleNum -100) + 15
                    }else if(angleNum <= 200){
                        rotate = 0.4
                        angle = rotate * (angleNum - 150) + 30
                    }else if(angleNum > 200){
                        rotate = 0.5
                        angle = rotate * (angleNum - 200) + 50
                    }
                    if(missionController.multi/10000 + angle<=0.00001){
                        if(m.sequenceNumber!==0){
                            m.coordinate.latitude =  m.coordinate.latitude
                            m.coordinate.longitude =  m.coordinate.longitude
                        }
                    }
                }
            }
            ffff=false
            missionController.loadRecover()
            missionController.loadMoveLine()
            waypointlatiOffsetArray.length = 0;
            waypointlonOffsetArray.length = 0;
            largNum = 1
            smalNum = 1
            angleNum = 0
            im.visible = false
            ima.visible = false
            ima1.visible = false
            ima2.visible = false
            ima3.visible = false
            ima4.visible = false
            timersend.stop()
            timerresend.stop()
            //==================================================
            missionController.visualItemsflag.clear()
        }

        if(num==5){
            timerresend.start()
        }
    }
    Timer{
        id:timerresend
        interval:2500
        running:false
        repeat:true
        onTriggered: flightMap.senddd()
    }
    function senddd(){
        num1++
        if(flagg==1){
            missionController.sendMovePoint(itemDragger.missionItemIndicator.sequenceNumber,missionController.visualItems.get(itemDragger.missionItemIndicator.sequenceNumber).coordinate)
            timersend.start()
        }
        if(flagg==2){
            var coordinatee1 = flightMap.toCoordinate(Qt.point(mmm,nnn))
            var coordinateee2 = flightMap.toCoordinate(Qt.point(co1x,co2y))
            var x1 = coordinatee1.latitude-coordinateee2.latitude
            var y1 = coordinatee1.longitude-coordinateee2.longitude
            missionController.sendMoveOffset(x1,y1)
            timersend.start()
        }
        if(flagg==3){
            missionController.sendLarge(Math.pow(1+multiple,largNum-1),flightMap.toCoordinate(Qt.point(xx,yy)))
            timersend.start()
        }
        if(flagg==4){
            missionController.sendSmall(Math.pow(1-multiple,smalNum-1),flightMap.toCoordinate(Qt.point(xx,yy)))
            timersend.start()
        }
        if(flagg==5){
            if(angleNum <= 50){
                rotate = 0.1
                missionController.sendShun(rotate*angleNum,flightMap.toCoordinate(Qt.point(xx,yy)))
            }else if(angleNum <= 100){
                rotate = 0.2
                missionController.sendShun((rotate*(angleNum - 50)+ 5),flightMap.toCoordinate(Qt.point(xx,yy)))
            }else if(angleNum <= 150){
                rotate = 0.3
                missionController.sendShun((rotate*(angleNum-100) + 15),flightMap.toCoordinate(Qt.point(xx,yy)))
            }else if(angleNum <= 200){
                rotate = 0.4
                missionController.sendShun((rotate*(angleNum - 150)+ 30),flightMap.toCoordinate(Qt.point(xx,yy)))
            }else if(angleNum > 200){
                rotate =  0.5
                missionController.sendShun((rotate*(angleNum-200)+ 50),flightMap.toCoordinate(Qt.point(xx,yy)))
            }
//            missionController.sendShun(rotate*angleNum,flightMap.toCoordinate(Qt.point(xx,yy)))
            timersend.start()
        }
        if(flagg==6){
            if(angleNum <= 50){
                rotate = 0.1
                missionController.sendNi(-rotate*angleNum,flightMap.toCoordinate(Qt.point(xx,yy)))
            }else if(angleNum <= 100){
                rotate = 0.2
                missionController.sendNi(-(rotate*(angleNum - 50)+5),flightMap.toCoordinate(Qt.point(xx,yy)))
            }else if(angleNum <= 150){
                rotate = 0.3
                missionController.sendNi(-(rotate*(angleNum-100) + 15),flightMap.toCoordinate(Qt.point(xx,yy)))
            }else if(angleNum <= 200){
                rotate = 0.4
                missionController.sendNi(-(rotate*(angleNum - 150)+ 30),flightMap.toCoordinate(Qt.point(xx,yy)))
            }else if(angleNum > 200){
                rotate =  0.5
                missionController.sendNi(-(rotate*(angleNum-200)+50),flightMap.toCoordinate(Qt.point(xx,yy)))
            }
//            missionController.sendNi(-rotate*angleNum,flightMap.toCoordinate(Qt.point(xx,yy)))
            timersend.start()
        }
        if(num1==5){
            num1=0
            num=0
            timersend.stop()
            timerresend.stop()
        }

    }
    Menu{
        id:singlepointm
        title: "function"
        MenuItem {
            text: qsTr("Move the waypoint")
            onTriggered: {
                itemDragger.visible=true
            }

        }
        MenuItem{
            text: qsTr("Jump to this waypoint")
            onTriggered: {
                itemDragger.visible=false
                _activeVehicle.setCurrentMissionSequence(itemDragger.missionItemIndicator.sequenceNumber)

            }
        }

    }
    MessageDialog {
        id:         messageDialog
        title:      qsTr("Send Command")
        text:       qsTr("This will change the existing route,\n do you want to send this command? ")
        standardButtons: StandardButton.Yes | StandardButton.No
        modality:   Qt.ApplicationModal
        visible:    false
        icon:       StandardIcon.Warning
        onNo:{
            var coord = flightMap.toCoordinate(Qt.point(clickxx,clickyy))
            var coordinatee1 = flightMap.toCoordinate(Qt.point(mmm,nnn))
            var coordinateee2 = flightMap.toCoordinate(Qt.point(co1x,co2y))
            var coordinatee33 = flightMap.toCoordinate(Qt.point(md,nd))
            var coordinatee44= flightMap.toCoordinate(Qt.point(itemDragger.x,itemDragger.y))
            var ref_lon_rad = coord.longitude * 3.141592654/180;
            var ref_lat_rad = coord.latitude * 3.141592654/180;
            for(var i=0;i<missionController.visualItems.count;i++){
                var m = missionController.visualItems.get(i)
                if(flagg==1 && i==itemDragger.missionItemIndicator.sequenceNumber ){
                    m.coordinate.latitude = m.coordinate.latitude + coordinatee33.latitude -coordinatee44.latitude
                    m.coordinate.longitude =  m.coordinate.longitude+coordinatee33.longitude - coordinatee44.longitude
                    _currentMissionItem.isCurrentItem = false

                }
                if(flagg==2){
                    m.coordinate.latitude =   m.coordinate.latitude-(coordinatee1.latitude-coordinateee2.latitude)
                    m.coordinate.longitude =  m.coordinate.longitude-(coordinatee1.longitude-coordinateee2.longitude)
                    if(m.command == 206 && m.param1 > 0.5){
                        m.param3 = m.param3 - (coordinatee1.latitude-coordinateee2.latitude)
                        m.param4 = m.param4 - (coordinatee1.latitude-coordinateee2.latitude)
                    }
                }

                if(flagg==3){
                    m.coordinate.latitude =  (m.coordinate.latitude-coord.latitude)/Math.pow(1+multiple,largNum-1)+coord.latitude
                    m.coordinate.longitude =  (m.coordinate.longitude-coord.longitude)/Math.pow(1+multiple,largNum-1)+coord.longitude
                    if(m.command == 206 && m.param1 > 0.5){
                        m.param3 = (m.param3 - coord.latitude)/Math.pow(1+ multiple, largNum -1) + coord.latitude
                        m.param4 = (m.param4 - coord.longitude)/Math.pow(1+ multiple, largNum - 1) + coord.longitude

                        for(var j = i -1; j > 0; j-- ){
                            var item = missionController.visualItems.get(j)
                            if(item.specifiesCoordinate){
                                var lat_previousitem = item.coordinate.latitude
                                var lon_previousitem = item.coordinate.longitude
                                var lat_nowitem_param3 =  m.param3
                                var lon_nowitem_param4 =  m.param4
                                var lat_previousitem_rad = lat_previousitem * 3.14159265358979323846 / 180
                                var lon_previousitem_rad = lon_previousitem * 3.14159265358979323846 / 180
                                var lat_nowitem_param3_rad = lat_nowitem_param3 * 3.14159265358979323846 / 180
                                var lon_nowitem_param4_rad = lon_nowitem_param4 * 3.14159265358979323846 / 180

                                var d_lat_rad = lat_nowitem_param3_rad - lat_previousitem_rad
                                var d_lon_rad = lon_nowitem_param4_rad - lon_previousitem_rad
                                var d_a = Math.sin(d_lat_rad / 2.0) * Math.sin(d_lat_rad / 2.0) + Math.sin(d_lon_rad / 2.0) * Math.sin(d_lon_rad / 2.0) * Math.cos(lat_previousitem_rad) * Math.cos(lat_nowitem_param3_rad)
                                var d_c = 2.0 * Math.atan2(Math.sqrt(d_a), Math.sqrt(1.0 - d_a))
                                var firsttrigger_distance = 6371000 * d_c
                                m.param2 = firsttrigger_distance
                                break
                            }
                        }
                        var camDist1 = parseInt(m.param1)
                        if(camDist1 > 0){
                            var lat_now = m.param3
                            var lon_now = m.param4
                            var lat_next = m.coordinate.latitude
                            var lon_next = m.coordinate.longitude

                            var lat_now_rad = lat_now / 180.0 * 3.14159265358979323846
                            var lon_now_rad = lon_now / 180.0 * 3.14159265358979323846
                            var lat_next_rad = lat_next / 180.0 * 3.14159265358979323846
                            var lon_next_rad = lon_next / 180.0 * 3.14159265358979323846

                            var d_lat = lat_next_rad - lat_now_rad
                            var d_lon = lon_next_rad - lon_now_rad
                            var a = Math.sin(d_lat / 2.0) * Math.sin(d_lat / 2.0) + Math.sin(d_lon / 2.0) * Math.sin(d_lon / 2.0) * Math.cos(lat_now_rad) * Math.cos(lat_next_rad)
                            var c = 2.0 * Math.atan2(Math.sqrt(a), Math.sqrt(1.0 - a))

                            var way_point_distance = 6371000 * c
                            var totalNumber = parseInt((way_point_distance  - m.param2)/ camDist1)
                            m.param1 = camDist1 + totalNumber * 1.0 / 1000.0
                        }
                    }
                }

                if(flagg==4){
                    m.coordinate.latitude =  (m.coordinate.latitude-coord.latitude)/Math.pow(1-multiple,smalNum-1)+coord.latitude
                    m.coordinate.longitude =  (m.coordinate.longitude-coord.longitude)/Math.pow(1-multiple,smalNum-1)+coord.longitude
                }
                if(flagg==5||flagg==6){
                    if(m.sequenceNumber!==0){
                        var lat_rad = m.coordinate.latitude * 3.141592654/180;
                        var lon_rad = m.coordinate.longitude * 3.141592654/180;
                        var sin_lat = Math.sin(lat_rad);
                        var cos_lat = Math.cos(lat_rad);
                        var cos_d_lon = Math.cos(lon_rad - ref_lon_rad);

                        var ref_sin_lat = Math.sin(ref_lat_rad);
                        var ref_cos_lat = Math.cos(ref_lat_rad);

                        var c = Math.acos(ref_sin_lat * sin_lat + ref_cos_lat * cos_lat * cos_d_lon);
                        var k = (Math.abs(c) < 1.1920929e-07) ? 1.0 : (c / Math.sin(c));

                        var x = k * (ref_cos_lat * sin_lat - ref_sin_lat * cos_lat * cos_d_lon) * 6371000;
                        var y = k * cos_lat * Math.sin(lon_rad - ref_lon_rad) * 6371000;

                        var hhhh
                        var llll
                        if(flagg==5){
                            if(angleNum <= 50){
                                rotate = 0.1
                                hhhh = x*Math.cos(-angleNum*rotate*3.141592654/180)-y*Math.sin(-angleNum*rotate*3.141592654/180)
                                llll = x*Math.sin(-angleNum*rotate*3.141592654/180)+y*Math.cos(-angleNum*rotate*3.141592654/180)
                            }else if(angleNum <= 100){
                                rotate = 0.2
                                hhhh = x*Math.cos(-(angleNum-50)*rotate*3.141592654/180 - 0.087266463)-y*Math.sin(-(angleNum-50)*rotate*3.141592654/180 - 0.087266463)
                                llll = x*Math.sin(-(angleNum-50)*rotate*3.141592654/180 - 0.087266463)+y*Math.cos(-(angleNum - 50)*rotate*3.141592654/180 - 0.087266463)
                            }else if(angleNum <= 150){
                                rotate = 0.3
                                hhhh = x*Math.cos(-(angleNum -100)*rotate*3.141592654/180 - 0.261799388)-y*Math.sin(-(angleNum - 100)*rotate*3.141592654/180 - 0.261799388)
                                llll = x*Math.sin(-(angleNum-100)*rotate*3.141592654/180 -  0.261799388)+y*Math.cos(-(angleNum-100)*rotate*3.141592654/180 - 0.261799388)
                            }else if(angleNum <= 200){
                                rotate = 0.4
                                hhhh = x*Math.cos(-(angleNum-150)*rotate*3.141592654/180 - 0.523598776 )-y*Math.sin(-(angleNum-150)*rotate*3.141592654/180 - 0.523598776)
                                llll = x*Math.sin(-(angleNum-150)*rotate*3.141592654/180 - 0.523598776)+y*Math.cos(-(angleNum-150)*rotate*3.141592654/180 - 0.523598776)
                            }else if(angleNum > 200){
                                hhhh = x*Math.cos(-(angleNum - 200)*rotate*3.141592654/180 - 0.872664626)-y*Math.sin(-(angleNum - 200)*rotate*3.141592654/180 -0.872664626)
                                llll = x*Math.sin(-(angleNum -200 )*rotate*3.141592654/180 - 0.872664626)+y*Math.cos(-(angleNum - 200)*rotate*3.141592654/180 -0.872664626)
                            }
       //                     hhhh = x*Math.cos(-angleNum*rotate*3.141592654/180)-y*Math.sin(-angleNum*rotate*3.141592654/180)
       //                     llll = x*Math.sin(-angleNum*rotate*3.141592654/180)+y*Math.cos(-angleNum*rotate*3.141592654/180)
                        }
                        if(flagg==6){
                            if(angleNum <= 50){
                                rotate = 0.1
                                hhhh = x*Math.cos(angleNum*rotate*3.141592654/180)-y*Math.sin(angleNum*rotate*3.141592654/180)
                                llll = x*Math.sin(angleNum*rotate*3.141592654/180)+y*Math.cos(angleNum*rotate*3.141592654/180)
                            }else if(angleNum <= 100){
                                rotate = 0.2
                                hhhh = x*Math.cos((angleNum-50)*rotate*3.141592654/180 + 0.087266463 )-y*Math.sin((angleNum-50)*rotate*3.141592654/180 + 0.087266463)
                                llll = x*Math.sin((angleNum-50)*rotate*3.141592654/180 + 0.087266463)+y*Math.cos((angleNum - 50)*rotate*3.141592654/180 + 0.087266463)
                            }else if(angleNum <= 150){
                                rotate =  0.3
                                hhhh = x*Math.cos((angleNum-100)*rotate*3.141592654/180 + 0.261799388)-y*Math.sin((angleNum - 100)*rotate*3.141592654/180 + 0.261799388)
                                llll = x*Math.sin((angleNum - 100)*rotate*3.141592654/180 + 0.261799388 )+y*Math.cos((angleNum -100)*rotate*3.141592654/180 + 0.261799388)
                            }else if(angleNum <= 200){
                                rotate = 0.4
                                hhhh = x*Math.cos((angleNum - 150)*rotate*3.141592654/180 + 0.523598776 )-y*Math.sin((angleNum - 150)*rotate*3.141592654/180 + 0.523598776)
                                llll = x*Math.sin((angleNum - 150)*rotate*3.141592654/180 +  0.523598776 )+y*Math.cos((angleNum-150)*rotate*3.141592654/180 +  0.523598776)
                            }else if(angleNum > 200){
                                rotate = 0.5
                                hhhh = x*Math.cos((angleNum - 200)*rotate*3.141592654/180 + 0.872664626)-y*Math.sin((angleNum - 200)*rotate*3.141592654/180 + 0.872664626)
                                llll = x*Math.sin((angleNum -200)*rotate*3.141592654/180 +  0.872664626)+y*Math.cos((angleNum - 200)*rotate*3.141592654/180 +  0.872664626)
                            }
                        }

                        var x_rad = hhhh / 6371000
                        var y_rad = llll / 6371000
                        c = Math.sqrt(x_rad * x_rad + y_rad * y_rad);
                        var sin_c = Math.sin(c);
                        var cos_c = Math.cos(c);

                        if (Math.abs(c) > 1.1920929e-07) {
                            lat_rad = Math.asin(cos_c * ref_sin_lat + (x_rad * sin_c * ref_cos_lat) / c);
                            lon_rad = (ref_lon_rad + Math.atan2(y_rad * sin_c, c * ref_cos_lat * cos_c - x_rad * ref_sin_lat * sin_c));
                        } else {
                            lat_rad = ref_lat_rad;
                            lon_rad = ref_lon_rad;
                        }
                        m.coordinate.latitude = lat_rad*180.0/3.141592654
                        m.coordinate.longitude = lon_rad*180.0/3.141592654
                    }
                    if(m.command == 206 && m.param1 > 0){
                        var cam_lat_rad = m.param3 * 3.141592654/180;
                        var cam_lon_rad = m.param4 * 3.141592654/180;
                        var cam_sin_lat = Math.sin(cam_lat_rad);
                        var cam_cos_lat = Math.cos(cam_lat_rad);
                        var cam_cos_d_lon = Math.cos(cam_lon_rad - ref_lon_rad);

                        var cam_ref_sin_lat = Math.sin(ref_lat_rad);
                        var cam_ref_cos_lat = Math.cos(ref_lat_rad);

                        var cam_c = Math.acos(cam_ref_sin_lat * cam_sin_lat + cam_ref_cos_lat * cam_cos_lat * cam_cos_d_lon);
                        var cam_k = (Math.abs(cam_c) < 1.1920929e-07) ? 1.0 : (cam_c / Math.sin(cam_c));

                        var cam_x = cam_k * (cam_ref_cos_lat * cam_sin_lat - cam_ref_sin_lat * cam_cos_lat * cam_cos_d_lon) * 6371000;
                        var cam_y = cam_k * cam_cos_lat * Math.sin(cam_lon_rad - ref_lon_rad) * 6371000;
                        var cam_hhhh ;
                        var cam_llll;
                        if(flagg == 5){
                            if(angleNum <= 50){
                                rotate = 0.1
                                cam_hhhh =cam_x*Math.cos(-angleNum*rotate*3.141592654/180)-cam_y*Math.sin(-angleNum*rotate*3.141592654/180)
                               cam_llll = cam_x*Math.sin(-angleNum*rotate*3.141592654/180)+cam_y*Math.cos(-angleNum*rotate*3.141592654/180)
                            }else if(angleNum <= 100){
                                rotate = 0.2
                                cam_hhhh =cam_x*Math.cos(-(angleNum-50)*rotate*3.141592654/180 - 0.087266463)-cam_y*Math.sin(-(angleNum-50)*rotate*3.141592654/180 - 0.087266463)
                                cam_llll =cam_x*Math.sin(-(angleNum-50)*rotate*3.141592654/180 - 0.087266463)+cam_y*Math.cos(-(angleNum - 50)*rotate*3.141592654/180 - 0.087266463)
                            }else if(angleNum <= 150){
                                rotate =  0.3
                                cam_hhhh = cam_x*Math.cos(-(angleNum-100)*rotate*3.141592654/180 - 0.261799388)-cam_y*Math.sin(-(angleNum - 100)*rotate*3.141592654/180 - 0.261799388)
                                cam_llll = cam_x*Math.sin(-(angleNum - 100)*rotate*3.141592654/180 - 0.261799388 )+cam_y*Math.cos(-(angleNum -100)*rotate*3.141592654/180 - 0.261799388)
                            }else if(angleNum <= 200){
                                rotate = 0.4
                                cam_hhhh = cam_x*Math.cos(-(angleNum - 150)*rotate*3.141592654/180 - 0.523598776 )-cam_y*Math.sin(-(angleNum - 150)*rotate*3.141592654/180 - 0.523598776)
                                cam_llll = cam_x*Math.sin(-(angleNum - 150)*rotate*3.141592654/180 -  0.523598776 )+cam_y*Math.cos(-(angleNum-150)*rotate*3.141592654/180 -  0.523598776)
                            }else if(angleNum > 200){
                                rotate = 0.5
                                cam_hhhh = cam_x*Math.cos(-(angleNum - 200)*rotate*3.141592654/180 - 0.872664626)-cam_y*Math.sin(-(angleNum - 200)*rotate*3.141592654/180 - 0.872664626)
                                cam_llll = cam_x*Math.sin(-(angleNum -200)*rotate*3.141592654/180 -  0.872664626)+cam_y*Math.cos(-(angleNum - 200)*rotate*3.141592654/180 -  0.872664626)
                            }
                        }
                        if(flagg == 6){
                            if(angleNum <= 50){
                                rotate = 0.1
                                cam_hhhh =cam_x*Math.cos(angleNum*rotate*3.141592654/180)-cam_y*Math.sin(angleNum*rotate*3.141592654/180)
                               cam_llll = cam_x*Math.sin(angleNum*rotate*3.141592654/180)+cam_y*Math.cos(angleNum*rotate*3.141592654/180)
                            }else if(angleNum <= 100){
                                rotate = 0.2
                                cam_hhhh =cam_x*Math.cos((angleNum-50)*rotate*3.141592654/180 + 0.087266463 )-cam_y*Math.sin((angleNum-50)*rotate*3.141592654/180 + 0.087266463)
                                cam_llll =cam_x*Math.sin((angleNum-50)*rotate*3.141592654/180 + 0.087266463)+cam_y*Math.cos((angleNum - 50)*rotate*3.141592654/180 + 0.087266463)
                            }else if(angleNum <= 150){
                                rotate =  0.3
                                cam_hhhh = cam_x*Math.cos((angleNum-100)*rotate*3.141592654/180 + 0.261799388)-cam_y*Math.sin((angleNum - 100)*rotate*3.141592654/180 + 0.261799388)
                                cam_llll = cam_x*Math.sin((angleNum - 100)*rotate*3.141592654/180 + 0.261799388 )+cam_y*Math.cos((angleNum -100)*rotate*3.141592654/180 + 0.261799388)
                            }else if(angleNum <= 200){
                                rotate = 0.4
                                cam_hhhh = cam_x*Math.cos((angleNum - 150)*rotate*3.141592654/180 + 0.523598776 )-cam_y*Math.sin((angleNum - 150)*rotate*3.141592654/180 + 0.523598776)
                                cam_llll = cam_x*Math.sin((angleNum - 150)*rotate*3.141592654/180 +  0.523598776 )+cam_y*Math.cos((angleNum-150)*rotate*3.141592654/180 +  0.523598776)
                            }else if(angleNum > 200){
                                rotate = 0.5
                                cam_hhhh = cam_x*Math.cos((angleNum - 200)*rotate*3.141592654/180 + 0.872664626)-cam_y*Math.sin((angleNum - 200)*rotate*3.141592654/180 + 0.872664626)
                                cam_llll = cam_x*Math.sin((angleNum -200)*rotate*3.141592654/180 +  0.872664626)+cam_y*Math.cos((angleNum - 200)*rotate*3.141592654/180 +  0.872664626)
                            }
                        }
                        var cam_x_rad = cam_hhhh / 6371000
                        var cam_y_rad = cam_llll / 6371000
                        cam_c = Math.sqrt(cam_x_rad * cam_x_rad + cam_y_rad * cam_y_rad);
                        var cam_sin_c = Math.sin(cam_c);
                        var cam_cos_c = Math.cos(cam_c);
                        if (Math.abs(cam_c) > 1.1920929e-07) {
                            cam_lat_rad = Math.asin(cam_cos_c * cam_ref_sin_lat + (cam_x_rad * cam_sin_c * cam_ref_cos_lat) / cam_c);
                            cam_lon_rad = (ref_lon_rad + Math.atan2(cam_y_rad * cam_sin_c, cam_c * cam_ref_cos_lat * cam_cos_c - cam_x_rad * cam_ref_sin_lat * cam_sin_c));
                        } else {
                            cam_lat_rad = ref_lat_rad;
                            cam_lon_rad = ref_lon_rad;
                        }
                        m.param3 = cam_lat_rad*180.0/3.141592654
                        m.param4 = cam_lon_rad*180.0/3.141592654
                    }
                }
            }

            missionController.loadRecover()
            missionController.loadMoveLineAdd()
            largNum = 1
            smalNum = 1
            angleNum = 0
            waypointlatiOffsetArray.length = 0;
            waypointlonOffsetArray.length = 0;
        }

        onRejected:
        {
            var coord = flightMap.toCoordinate(Qt.point(clickxx,clickyy))
            var coordinatee1 = flightMap.toCoordinate(Qt.point(mmm,nnn))
            var coordinateee2 = flightMap.toCoordinate(Qt.point(co1x,co2y))
            var coordinatee33 = flightMap.toCoordinate(Qt.point(md,nd))
            var coordinatee44= flightMap.toCoordinate(Qt.point(itemDragger.x,itemDragger.y))
            var ref_lon_rad = coord.longitude * 3.141592654/180;
            var ref_lat_rad = coord.latitude * 3.141592654/180;
            for(var i=0;i<missionController.visualItems.count;i++){
                var m = missionController.visualItems.get(i)
                if(flagg==1&& i==itemDragger.missionItemIndicator.sequenceNumber ){
                    m.coordinate.latitude = m.coordinate.latitude + coordinatee33.latitude -coordinatee44.latitude
                    m.coordinate.longitude =  m.coordinate.longitude+coordinatee33.longitude - coordinatee44.longitude
                    _currentMissionItem.isCurrentItem = false
                }
                if(flagg==2){
                    m.coordinate.latitude =  m.coordinate.latitude-(coordinatee1.latitude-coordinateee2.latitude)
                    m.coordinate.longitude =  m.coordinate.longitude-(coordinatee1.longitude-coordinateee2.longitude)
                }
                if(flagg==3){
                    m.coordinate.latitude =  (m.coordinate.latitude-coord.latitude)/Math.pow(1+multiple,largNum-1)+coord.latitude
                    m.coordinate.longitude =  (m.coordinate.longitude-coord.longitude)/Math.pow(1+multiple,largNum-1)+coord.longitude

                }
                if(flagg==4){
                    m.coordinate.latitude =  (m.coordinate.latitude-coord.latitude)/Math.pow(1-multiple,smalNum-1)+coord.latitude
                    m.coordinate.longitude =  (m.coordinate.longitude-coord.longitude)/Math.pow(1-multiple,smalNum-1)+coord.longitude

                }
                if(flagg==5||flagg==6){
                    if(m.sequenceNumber!==0){

                        var lat_rad = m.coordinate.latitude * 3.141592654/180;
                        var lon_rad = m.coordinate.longitude * 3.141592654/180;
                        var sin_lat = Math.sin(lat_rad);
                        var cos_lat = Math.cos(lat_rad);
                        var cos_d_lon = Math.cos(lon_rad - ref_lon_rad);

                        var ref_sin_lat = Math.sin(ref_lat_rad);
                        var ref_cos_lat = Math.cos(ref_lat_rad);

                        var c = Math.acos(ref_sin_lat * sin_lat + ref_cos_lat * cos_lat * cos_d_lon);
                        var k = (Math.abs(c) < 1.1920929e-07) ? 1.0 : (c / Math.sin(c));

                        var x = k * (ref_cos_lat * sin_lat - ref_sin_lat * cos_lat * cos_d_lon) * 6371000;
                        var y = k * cos_lat * Math.sin(lon_rad - ref_lon_rad) * 6371000;
                        var hhhh
                        var llll
                        if(flagg==5){
                            if(angleNum <= 50){
                                rotate = 0.1
                                hhhh = x*Math.cos(-angleNum*rotate*3.141592654/180)-y*Math.sin(-angleNum*rotate*3.141592654/180)
                                llll = x*Math.sin(-angleNum*rotate*3.141592654/180)+y*Math.cos(-angleNum*rotate*3.141592654/180)
                            }else if(angleNum <= 100){
                                rotate = 0.2
                                hhhh = x*Math.cos(-(angleNum-50)*rotate*3.141592654/180 - 0.087266463)-y*Math.sin(-(angleNum-50)*rotate*3.141592654/180 - 0.087266463)
                                llll = x*Math.sin(-(angleNum-50)*rotate*3.141592654/180 - 0.087266463)+y*Math.cos(-(angleNum - 50)*rotate*3.141592654/180 - 0.087266463)
                            }else if(angleNum <= 150){
                                rotate = 0.3
                                hhhh = x*Math.cos(-(angleNum -100)*rotate*3.141592654/180 - 0.261799388)-y*Math.sin(-(angleNum - 100)*rotate*3.141592654/180 - 0.261799388)
                                llll = x*Math.sin(-(angleNum-100)*rotate*3.141592654/180 -  0.261799388)+y*Math.cos(-(angleNum-100)*rotate*3.141592654/180 - 0.261799388)
                            }else if(angleNum <= 200){
                                rotate = 0.4
                                hhhh = x*Math.cos(-(angleNum-150)*rotate*3.141592654/180 - 0.523598776 )-y*Math.sin(-(angleNum-150)*rotate*3.141592654/180 - 0.523598776)
                                llll = x*Math.sin(-(angleNum-150)*rotate*3.141592654/180 - 0.523598776)+y*Math.cos(-(angleNum-150)*rotate*3.141592654/180 - 0.523598776)
                            }else if(angleNum > 200){
                                hhhh = x*Math.cos(-(angleNum - 200)*rotate*3.141592654/180 - 0.872664626)-y*Math.sin(-(angleNum - 200)*rotate*3.141592654/180 -0.872664626)
                                llll = x*Math.sin(-(angleNum -200 )*rotate*3.141592654/180 - 0.872664626)+y*Math.cos(-(angleNum - 200)*rotate*3.141592654/180 -0.872664626)
                            }
//                            hhhh = x*Math.cos(-angleNum*rotate*3.141592654/180)-y*Math.sin(-angleNum*rotate*3.141592654/180)
//                            llll = x*Math.sin(-angleNum*rotate*3.141592654/180)+y*Math.cos(-angleNum*rotate*3.141592654/180)
                        }
                        if(flagg==6){
                            if(angleNum <= 50){
                                rotate = 0.1
                                hhhh = x*Math.cos(angleNum*rotate*3.141592654/180)-y*Math.sin(angleNum*rotate*3.141592654/180)
                                llll = x*Math.sin(angleNum*rotate*3.141592654/180)+y*Math.cos(angleNum*rotate*3.141592654/180)
                            }else if(angleNum <= 100){
                                rotate = 0.2
                                hhhh = x*Math.cos((angleNum-50)*rotate*3.141592654/180 + 0.087266463 )-y*Math.sin((angleNum-50)*rotate*3.141592654/180 + 0.087266463)
                                llll = x*Math.sin((angleNum-50)*rotate*3.141592654/180 + 0.087266463)+y*Math.cos((angleNum - 50)*rotate*3.141592654/180 + 0.087266463)
                            }else if(angleNum <= 150){
                                rotate =  0.3
                                hhhh = x*Math.cos((angleNum-100)*rotate*3.141592654/180 + 0.261799388)-y*Math.sin((angleNum - 100)*rotate*3.141592654/180 + 0.261799388)
                                llll = x*Math.sin((angleNum - 100)*rotate*3.141592654/180 + 0.261799388 )+y*Math.cos((angleNum -100)*rotate*3.141592654/180 + 0.261799388)
                            }else if(angleNum <= 200){
                                rotate = 0.4
                                hhhh = x*Math.cos((angleNum - 150)*rotate*3.141592654/180 + 0.523598776 )-y*Math.sin((angleNum - 150)*rotate*3.141592654/180 + 0.523598776)
                                llll = x*Math.sin((angleNum - 150)*rotate*3.141592654/180 +  0.523598776 )+y*Math.cos((angleNum-150)*rotate*3.141592654/180 +  0.523598776)
                            }else if(angleNum > 200){
                                rotate = 0.5
                                hhhh = x*Math.cos((angleNum - 200)*rotate*3.141592654/180 + 0.872664626)-y*Math.sin((angleNum - 200)*rotate*3.141592654/180 + 0.872664626)
                                llll = x*Math.sin((angleNum -200)*rotate*3.141592654/180 +  0.872664626)+y*Math.cos((angleNum - 200)*rotate*3.141592654/180 +  0.872664626)
                            }
//                            hhhh = x*Math.cos(angleNum*rotate*3.141592654/180)-y*Math.sin(angleNum*rotate*3.141592654/180)
//                            llll = x*Math.sin(angleNum*rotate*3.141592654/180)+y*Math.cos(angleNum*rotate*3.141592654/180)
                        }

                        var x_rad = hhhh / 6371000
                        var y_rad = llll / 6371000
                        c = Math.sqrt(x_rad * x_rad + y_rad * y_rad);
                        var sin_c = Math.sin(c);
                        var cos_c = Math.cos(c);
                        if (Math.abs(c) > 1.1920929e-07) {
                            lat_rad = Math.asin(cos_c * ref_sin_lat + (x_rad * sin_c * ref_cos_lat) / c);
                            lon_rad = (ref_lon_rad + Math.atan2(y_rad * sin_c, c * ref_cos_lat * cos_c - x_rad * ref_sin_lat * sin_c));
                        } else {
                            lat_rad = ref_lat_rad;
                            lon_rad = ref_lon_rad;
                        }
                        m.coordinate.latitude = lat_rad*180.0/3.141592654
                        m.coordinate.longitude = lon_rad*180.0/3.141592654
                    }
                    if(m.command == 206 && m.param1 > 0){
                        var cam_lat_rad = m.param3 * 3.141592654/180;
                        var cam_lon_rad = m.param4 * 3.141592654/180;
                        var cam_sin_lat = Math.sin(cam_lat_rad);
                        var cam_cos_lat = Math.cos(cam_lat_rad);
                        var cam_cos_d_lon = Math.cos(cam_lon_rad - ref_lon_rad);

                        var cam_ref_sin_lat = Math.sin(ref_lat_rad);
                        var cam_ref_cos_lat = Math.cos(ref_lat_rad);

                        var cam_c = Math.acos(cam_ref_sin_lat * cam_sin_lat + cam_ref_cos_lat * cam_cos_lat * cam_cos_d_lon);
                        var cam_k = (Math.abs(cam_c) < 1.1920929e-07) ? 1.0 : (cam_c / Math.sin(cam_c));

                        var cam_x = cam_k * (cam_ref_cos_lat * cam_sin_lat - cam_ref_sin_lat * cam_cos_lat * cam_cos_d_lon) * 6371000;
                        var cam_y = cam_k * cam_cos_lat * Math.sin(cam_lon_rad - ref_lon_rad) * 6371000;
                        var cam_hhhh ;
                        var cam_llll;
                        if(flagg == 5){
                            if(angleNum <= 50){
                                rotate = 0.1
                                cam_hhhh =cam_x*Math.cos(-angleNum*rotate*3.141592654/180)-cam_y*Math.sin(-angleNum*rotate*3.141592654/180)
                               cam_llll = cam_x*Math.sin(-angleNum*rotate*3.141592654/180)+cam_y*Math.cos(-angleNum*rotate*3.141592654/180)
                            }else if(angleNum <= 100){
                                rotate = 0.2
                                cam_hhhh =cam_x*Math.cos(-(angleNum-50)*rotate*3.141592654/180 - 0.087266463)-cam_y*Math.sin(-(angleNum-50)*rotate*3.141592654/180 - 0.087266463)
                                cam_llll =cam_x*Math.sin(-(angleNum-50)*rotate*3.141592654/180 - 0.087266463)+cam_y*Math.cos(-(angleNum - 50)*rotate*3.141592654/180 - 0.087266463)
                            }else if(angleNum <= 150){
                                rotate =  0.3
                                cam_hhhh = cam_x*Math.cos(-(angleNum-100)*rotate*3.141592654/180 - 0.261799388)-cam_y*Math.sin(-(angleNum - 100)*rotate*3.141592654/180 - 0.261799388)
                                cam_llll = cam_x*Math.sin(-(angleNum - 100)*rotate*3.141592654/180 - 0.261799388 )+cam_y*Math.cos(-(angleNum -100)*rotate*3.141592654/180 - 0.261799388)
                            }else if(angleNum <= 200){
                                rotate = 0.4
                                cam_hhhh = cam_x*Math.cos(-(angleNum - 150)*rotate*3.141592654/180 - 0.523598776 )-cam_y*Math.sin(-(angleNum - 150)*rotate*3.141592654/180 - 0.523598776)
                                cam_llll = cam_x*Math.sin(-(angleNum - 150)*rotate*3.141592654/180 -  0.523598776 )+cam_y*Math.cos(-(angleNum-150)*rotate*3.141592654/180 -  0.523598776)
                            }else if(angleNum > 200){
                                rotate = 0.5
                                cam_hhhh = cam_x*Math.cos(-(angleNum - 200)*rotate*3.141592654/180 - 0.872664626)-cam_y*Math.sin(-(angleNum - 200)*rotate*3.141592654/180 - 0.872664626)
                                cam_llll = cam_x*Math.sin(-(angleNum -200)*rotate*3.141592654/180 -  0.872664626)+cam_y*Math.cos(-(angleNum - 200)*rotate*3.141592654/180 -  0.872664626)
                            }
                        }
                        if(flagg == 6){
                            if(angleNum <= 50){
                                rotate = 0.1
                                cam_hhhh =cam_x*Math.cos(angleNum*rotate*3.141592654/180)-cam_y*Math.sin(angleNum*rotate*3.141592654/180)
                               cam_llll = cam_x*Math.sin(angleNum*rotate*3.141592654/180)+cam_y*Math.cos(angleNum*rotate*3.141592654/180)
                            }else if(angleNum <= 100){
                                rotate = 0.2
                                cam_hhhh =cam_x*Math.cos((angleNum-50)*rotate*3.141592654/180 + 0.087266463 )-cam_y*Math.sin((angleNum-50)*rotate*3.141592654/180 + 0.087266463)
                                cam_llll =cam_x*Math.sin((angleNum-50)*rotate*3.141592654/180 + 0.087266463)+cam_y*Math.cos((angleNum - 50)*rotate*3.141592654/180 + 0.087266463)
                            }else if(angleNum <= 150){
                                rotate =  0.3
                                cam_hhhh = cam_x*Math.cos((angleNum-100)*rotate*3.141592654/180 + 0.261799388)-cam_y*Math.sin((angleNum - 100)*rotate*3.141592654/180 + 0.261799388)
                                cam_llll = cam_x*Math.sin((angleNum - 100)*rotate*3.141592654/180 + 0.261799388 )+cam_y*Math.cos((angleNum -100)*rotate*3.141592654/180 + 0.261799388)
                            }else if(angleNum <= 200){
                                rotate = 0.4
                                cam_hhhh = cam_x*Math.cos((angleNum - 150)*rotate*3.141592654/180 + 0.523598776 )-cam_y*Math.sin((angleNum - 150)*rotate*3.141592654/180 + 0.523598776)
                                cam_llll = cam_x*Math.sin((angleNum - 150)*rotate*3.141592654/180 +  0.523598776 )+cam_y*Math.cos((angleNum-150)*rotate*3.141592654/180 +  0.523598776)
                            }else if(angleNum > 200){
                                rotate = 0.5
                                cam_hhhh = cam_x*Math.cos((angleNum - 200)*rotate*3.141592654/180 + 0.872664626)-cam_y*Math.sin((angleNum - 200)*rotate*3.141592654/180 + 0.872664626)
                                cam_llll = cam_x*Math.sin((angleNum -200)*rotate*3.141592654/180 +  0.872664626)+cam_y*Math.cos((angleNum - 200)*rotate*3.141592654/180 +  0.872664626)
                            }
                        }
                        var cam_x_rad = cam_hhhh / 6371000
                        var cam_y_rad = cam_llll / 6371000
                        cam_c = Math.sqrt(cam_x_rad * cam_x_rad + cam_y_rad * cam_y_rad);
                        var cam_sin_c = Math.sin(cam_c);
                        var cam_cos_c = Math.cos(cam_c);
                        if (Math.abs(cam_c) > 1.1920929e-07) {
                            cam_lat_rad = Math.asin(cam_cos_c * cam_ref_sin_lat + (cam_x_rad * cam_sin_c * cam_ref_cos_lat) / cam_c);
                            cam_lon_rad = (ref_lon_rad + Math.atan2(cam_y_rad * cam_sin_c, cam_c * cam_ref_cos_lat * cam_cos_c - cam_x_rad * cam_ref_sin_lat * cam_sin_c));
                        } else {
                            cam_lat_rad = ref_lat_rad;
                            cam_lon_rad = ref_lon_rad;
                        }
                        m.param3 = cam_lat_rad*180.0/3.141592654
                        m.param4 = cam_lon_rad*180.0/3.141592654
                    }
                }
            }
            missionController.loadRecover()
            missionController.loadMoveLineAdd()
            largNum = 1
            smalNum = 1
            angleNum = 0
            waypointlatiOffsetArray.length = 0;
            waypointlonOffsetArray.length = 0;
        }

        onYes: {
            if(flagg==1){
                missionController.sendMovePoint(itemDragger.missionItemIndicator.sequenceNumber,missionController.visualItems.get(itemDragger.missionItemIndicator.sequenceNumber).coordinate)
                timersend.start()
                _currentMissionItem.isCurrentItem = false
            }
            if(flagg==2){
                var coordinatee1 = flightMap.toCoordinate(Qt.point(mmm,nnn))
                var coordinateee2 = flightMap.toCoordinate(Qt.point(co1x,co2y))
                var x1 = coordinatee1.latitude-coordinateee2.latitude
                var y1 = coordinatee1.longitude-coordinateee2.longitude
                missionController.sendMoveOffset(x1,y1)
                timersend.start()
            }
            if(flagg==3){
                missionController.sendLarge(Math.pow(1+multiple,largNum-1),flightMap.toCoordinate(Qt.point(xx,yy)))
                timersend.start()
            }

            if(flagg==4){
                missionController.sendSmall(Math.pow(1-multiple,smalNum-1),flightMap.toCoordinate(Qt.point(xx,yy)))
                timersend.start()

            }
            if(flagg==5){
                if(angleNum <= 50){
                    rotate = 0.1
                    missionController.sendShun(rotate*angleNum,flightMap.toCoordinate(Qt.point(xx,yy)))
                }else if(angleNum <= 100){
                    rotate = 0.2
                    missionController.sendShun((rotate*(angleNum - 50)+ 5),flightMap.toCoordinate(Qt.point(xx,yy)))
                }else if(angleNum <= 150){
                    rotate = 0.3
                    missionController.sendShun((rotate*(angleNum-100) + 15),flightMap.toCoordinate(Qt.point(xx,yy)))
                }else if(angleNum <= 200){
                    rotate = 0.4
                    missionController.sendShun((rotate*(angleNum - 150)+ 30),flightMap.toCoordinate(Qt.point(xx,yy)))
                }else if(angleNum > 200){
                    rotate =  0.5
                    missionController.sendShun((rotate*(angleNum-200) + 50),flightMap.toCoordinate(Qt.point(xx,yy)))
                }
//               missionController.sendShun(rotate*angleNum,flightMap.toCoordinate(Qt.point(xx,yy)))
                timersend.start()
            }
            if(flagg==6){
                if(angleNum <= 50){
                    rotate = 0.1
                    missionController.sendNi(-rotate*angleNum,flightMap.toCoordinate(Qt.point(xx,yy)))
                }else if(angleNum <= 100){
                    rotate = 0.2
                    missionController.sendNi(-(rotate*(angleNum - 50)+ 5),flightMap.toCoordinate(Qt.point(xx,yy)))
                }else if(angleNum <= 150){
                    rotate = 0.3
                    missionController.sendNi(-(rotate*(angleNum-100) + 15),flightMap.toCoordinate(Qt.point(xx,yy)))
                }else if(angleNum <= 200){
                    rotate = 0.4
                    missionController.sendNi(-(rotate*(angleNum - 150)+30),flightMap.toCoordinate(Qt.point(xx,yy)))
                }else if(angleNum > 200){
                    rotate =  0.5
                    missionController.sendNi(-(rotate*(angleNum-200)+ 50),flightMap.toCoordinate(Qt.point(xx,yy)))
                }
//                missionController.sendNi(-rotate*angleNum,flightMap.toCoordinate(Qt.point(xx,yy)))
                timersend.start()
            }
            messageDialog.visible = false
        }
    }


    Component {
        id: missionItemComponent

        MissionItemIndicator {
            id:             itemIndicator
            coordinate:     object.coordinate
            visible:       object.specifiesCoordinate && (index != 0 || object.showHomePosition)
            z:              QGroundControl.zOrderMapItems
            missionItem:    object
            sequenceNumber: object.sequenceNumber

            //-- If you don't want to allow selecting items beneath the
            //toolbar, the code below has to check and see if mouse.y
            //is greater than (map.height - ScreenTools.availableHeight)
            onClicked:  itemIndicator.dis()
            function dis(){
                setCurrentItem(object.sequenceNumber)
                singlepointm.popup()
                itemDragger.missionItem = Qt.binding(function() { return object })
                itemDragger.missionItemIndicator = Qt.binding(function() { return itemIndicator })
            }

            // These are the non-coordinate child mission items attached to this item
            Row {
                anchors.top:    parent.top
                anchors.left:   parent.right

                Repeater {
                    model: object.childItems

                    delegate: MissionItemIndexLabel {
                        label:          object.sequenceNumber
                        checked:  object.isCurrentItem
                        z:              2
                        onClicked: setCurrentItem(object.sequenceNumber)
                    }
                }
            }
        }
    }
    // Add lines between waypoints
    MissionLineView {
        model: _mainIsMap ? missionController.waypointLines : 0
    }
    MissionLineViewAdd {
        model: ffff&&_mainIsMap ? missionController.waypointLinesAdd : 0
    }
    // GeoFence polygon
    MapPolygon {
        border.color:   "#80FF0000"
        border.width:   3
        path:           geoFenceController.polygon.path
        visible:        geoFenceController.fenceEnabled && geoFenceController.polygonSupported
    }

    // GeoFence circle
    MapCircle {
        border.color:   "#80FF0000"
        border.width:   3
        center:         missionController.plannedHomePosition
        radius:         (geoFenceController.fenceEnabled && geoFenceController.circleSupported) ? geoFenceController.circleRadius : 0
        z:              QGroundControl.zOrderMapItems
        visible:         geoFenceController.fenceEnabled && geoFenceController.circleSupported
    }

    // GeoFence breach return point
    MapQuickItem {
        anchorPoint:    Qt.point(sourceItem.width / 2, sourceItem.height / 2)
        coordinate:     geoFenceController.breachReturnPoint
        visible:        geoFenceController.fenceEnabled && geoFenceController.breachReturnSupported
        sourceItem:     MissionItemIndexLabel { label: "F" }
        z:              QGroundControl.zOrderMapItems
    }

    // Rally points on map
    MapItemView {
        model: rallyPointController.points

        delegate: MapQuickItem {
            id:             itemIndicator
            anchorPoint:    Qt.point(sourceItem.width / 2, sourceItem.height / 2)
            coordinate:     object.coordinate
            z:              QGroundControl.zOrderMapItems

            sourceItem: MissionItemIndexLabel {
                id:         itemIndexLabel
                label:      qsTr("R", "rally point map item label")
            }
        }
    }

    // GoTo here waypoint
    MapQuickItem {
        coordinate:     _gotoHereCoordinate
        visible:        _activeVehicle && _activeVehicle.guidedMode && _gotoHereCoordinate.isValid
        z:              QGroundControl.zOrderMapItems
        anchorPoint.x:  sourceItem.width  / 2
        anchorPoint.y:  sourceItem.height / 2

        sourceItem: MissionItemIndexLabel {
            checked: true
            label:   qsTr("G", "Goto here waypoint") // second string is translator's hint.
        }
    }

    MapQuickItem {
        id: centerPoint
        coordinate:     flightMap.toCoordinate(Qt.point(xx,yy))
        visible:       false
        z:              QGroundControl.zOrderMapItems
        anchorPoint.x:  sourceItem.width  / 2
        anchorPoint.y:  sourceItem.height / 2

        sourceItem: MissionItemIndexLabel {
            checked:  true
            label:          qsTr("C", "center") // second string is translator's hint.
        }
    }
    MapScale {
        id:mapss
        anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight  + mainWindow.width *0.2 * 9/16
        anchors.leftMargin:    ScreenTools.defaultFontPixelHeight
        anchors.bottom:         parent.bottom
        anchors.left:          parent.left
        z:                      QGroundControl.zOrderWidgets
        mapControl:             flightMap
        visible:                !ScreenTools.isTinyScreen
    }
    Rectangle{
        id:test
        width:parent.width
        height:parent.height
        color:          "transparent"
        opacity: 0.7
        visible:       false
        z:              QGroundControl.zOrderMapItems+1
        function gogo(){
            if(ff===1){
                mm=mmm
                nn=nnn
                co1x = mmm
                co2y = nnn
                ff=2
            }
            var coordinatee1 = flightMap.toCoordinate(Qt.point(mmm,nnn))
            var coordinateee2 = flightMap.toCoordinate(Qt.point(mm,nn))
            var x1 = coordinatee1.latitude-coordinateee2.latitude
            var y1 = coordinatee1.longitude-coordinateee2.longitude
            for(var i = 0;i<missionController.visualItems.count;i++)
            {
                var m = missionController.visualItems.get(i);
                coordinatee.altitude = m.coordinate.altitude
                coordinatee.latitude = m.coordinate.latitude+x1
                coordinatee.longitude = m.coordinate.longitude+y1
                m.coordinate=coordinatee
                if(m.command == 206 && m.param1 > 0.5){
                    m.param3  = m.param3  + x1
                    m.param4 = m.param4  + y1
                }
            }
            mm=mmm
            nn=nnn
        }

        Timer{
            id:timer2
            interval:150
            running:false
            repeat:true
            onTriggered: test.dodo()
        }
        function get()
        {
            flightMap.gesture.enabled=false
            timer2.start()
            missionController.visualItems.get(1).flage = true;
        }
        function dodo()
        {
            missionController.loadMoveLineAdd()
        }
        function recover()
        {
            ff=1
            flightMap.gesture.enabled=true
            test.visible=false
            image6.checked=false
            timer2.stop()
          //  messageDialog.text =  qsTr("Send")
            messageDialog.visible=true
            flagg=2
        }
        MouseArea {
            anchors.fill:   parent
            onPositionChanged:{
                mmm=mouse.x
                nnn=mouse.y
                test.gogo()
            }
            //onPressed / onReleased
            onPressed:test.get()
            onReleased:test.recover()
        }

    }
    //================================================================

    Rectangle{
        id:im
        x:clickxx
        y:clickyy
        color:          "transparent"
        opacity: 0.1
        z:              QGroundControl.zOrderMapItems+1
        visible : false
        width:image6.width
        height:image6.height
        PropertyAnimation on x{id:ani16;to: clickxx+image6.width ; duration:1000}
        PropertyAnimation on y{id:ani17;to: clickyy-image6.width*1.732 ; duration:1000}
        NumberAnimation {id:ani18;target:im;properties:"opacity";from:0.1;to: 0.9 ; duration:500}

        RoundButton {
            id:             image6
            visible:       true
            buttonImage:    "/qmlimages/move.svg"
        }
        MouseArea{
            anchors.fill: parent
            onClicked:{
                if(image5.checked == true){
                    image5.checked = false
                    sendlatitude.visible = false
                }
                if(image6.checked == false){
                    image6.checked=true
                    test.visible = true
               }else{
                    image6.checked = false
                    test.visible = false
                }
            }
        }

    }
    //=================================================================
    Timer{
        id:timerLine
        interval:300
        running:false
        repeat:true
        onTriggered: test.dodo()
    }

    function formatDouble(src){
        return Math.round(src*Math.pow(10,7))/Math.pow(10, 7)
    }
    Rectangle{
        id:ima
        x:clickxx
        y:clickyy
        opacity: 0.1
        color:          "transparent"
        z:              QGroundControl.zOrderMapItems+1
        visible : false
        width:image1.width
        height:image1.height
        PropertyAnimation on x{id:ani1;to: clickxx-image1.width*2 ; duration:1000}
        NumberAnimation {id:ani3;target:ima;properties:"opacity";from:0.1;to: 0.9 ; duration:500}

        RoundButton {
            id:             image1
            visible:       true
            buttonImage:    "/qmlimages/ZoomPlu.svg"

        }
        function get(){
            timer3.start()
            timerLine.start()
            multiple=0.009
            missionController.visualItems.get(1).flage=true
        }
        Timer{
            id:timer3
            interval:150
            running:false
            repeat:true
            onTriggered: ima.dodo()
        }

        function dodo(){
            var coordinatee1 = flightMap.toCoordinate(Qt.point(clickxx,clickyy))
//            if((waypointlatiOffsetArray.length != 0) && (waypointlatiOffsetArray.length != 0))
//            {
//                waypointlatiOffsetArray.length = 0;
//                waypointlonOffsetArray.length = 0
//            }
            if(largNum == 1){
                for(var i = 0; i < missionController.visualItems.count; i++){
                     var item = missionController.visualItems.get(i)
                    if(item.sequenceNumber !==0){
                        var latitudedis = item.coordinate.latitude - coordinatee1.latitude
                        var longitudedis = item.coordinate.longitude - coordinatee1.longitude
                        latitudedis = formatDouble(latitudedis)
                        longitudedis = formatDouble(longitudedis)
                        console.log("sequencenumber: "  + item.sequenceNumber + " i: " + i + " " + latitudedis + " " + longitudedis)
                        waypointlatiOffsetArray.push(latitudedis)
                        waypointlonOffsetArray.push(longitudedis)
                    }
                }
            }
            for(var i = 0;i<missionController.visualItems.count;i++){
                var m = missionController.visualItems.get(i);
                if(m.sequenceNumber!==0){
                    m.coordinate.latitude = (m.coordinate.latitude - coordinatee1.latitude)*multiple + m.coordinate.latitude
                    m.coordinate.longitude = (m.coordinate.longitude - coordinatee1.longitude)*multiple + m.coordinate.longitude

                    coordinatee.altitude = m.coordinate.altitude
                    coordinatee.latitude = m.coordinate.latitude
                    coordinatee.longitude =  m.coordinate.longitude
                    m.coordinate = coordinatee
                }
//                if(m.command == 206 && m.param1 >= 0.5){
//                    var cameCoord1 = QtPositioning.coordinate(m.param3, m.param4)
//                    var camDist1 = parseInt(m.param1)
//                    cameCoord1.latitude = (cameCoord1.latitude - coordinatee1.latitude)*multiple+cameCoord1.latitude
//                    cameCoord1.longitude = (cameCoord1.longitude  - coordinatee1.longitude)*multiple+cameCoord1.longitude
//                    m.param3 = cameCoord1.latitude
//                    m.param4 = cameCoord1.longitude
//                    for(var j = i -1; j > 0; j-- ){
//                        var item = missionController.visualItems.get(j)
//                        if(item.specifiesCoordinate){
//                            var lat_previousitem = item.coordinate.latitude
//                            var lon_previousitem = item.coordinate.longitude
//                            var lat_nowitem_param3 = cameCoord1.latitude
//                            var lon_nowitem_param4 = cameCoord1.longitude
//                            var lat_previousitem_rad = lat_previousitem * 3.14159265358979323846 / 180
//                            var lon_previousitem_rad = lon_previousitem * 3.14159265358979323846 / 180
//                            var lat_nowitem_param3_rad = lat_nowitem_param3 * 3.14159265358979323846 / 180
//                            var lon_nowitem_param4_rad = lon_nowitem_param4 * 3.14159265358979323846 / 180

//                            var d_lat_rad = lat_nowitem_param3_rad - lat_previousitem_rad
//                            var d_lon_rad = lon_nowitem_param4_rad - lon_previousitem_rad
//                            var d_a = Math.sin(d_lat_rad / 2.0) * Math.sin(d_lat_rad / 2.0) + Math.sin(d_lon_rad / 2.0) * Math.sin(d_lon_rad / 2.0) * Math.cos(lat_previousitem_rad) * Math.cos(lat_nowitem_param3_rad)
//                            var d_c = 2.0 * Math.atan2(Math.sqrt(d_a), Math.sqrt(1.0 - d_a))
//                            var firsttrigger_distance = 6371000 * d_c
//                            m.param2 = firsttrigger_distance
//                            break
//                        }
//                    }
//                    if(camDist1 > 0)
//                    {
//                        var lat_now = cameCoord1.latitude
//                        var lon_now = cameCoord1.longitude
//                        var lat_next = m.coordinate.latitude
//                        var lon_next = m.coordinate.longitude
//                        var way_point_distance = Math.round((m.param1 - camDist1) * 1000) * camDist1 // previously distance
//                        var lat_now_rad = lat_now / 180.0 * 3.14159265358979323846
//                        var lon_now_rad = lon_now / 180.0 * 3.14159265358979323846
//                        var lat_next_rad = lat_next / 180.0 * 3.14159265358979323846
//                        var lon_next_rad = lon_next / 180.0 * 3.14159265358979323846

//                        var d_lat = lat_next_rad - lat_now_rad
//                        var d_lon = lon_next_rad - lon_now_rad
//                        var a = Math.sin(d_lat / 2.0) * Math.sin(d_lat / 2.0) + Math.sin(d_lon / 2.0) * Math.sin(d_lon / 2.0) * Math.cos(lat_now_rad) * Math.cos(lat_next_rad)
//                        var c = 2.0 * Math.atan2(Math.sqrt(a), Math.sqrt(1.0 - a))

//                        way_point_distance = 6371000 * c
//                        var totalNumber1 = parseInt((way_point_distance  - m.param2)/ camDist1)
//                        m.param1 = camDist1 + totalNumber1 * 1.0 / 1000.0
//                    }
//                }
            }
            largNum++
        }
        function recover(){
            timerLine.stop()
            timer3.stop()
//            messageDialog.text =  qsTr("Send")
           messageDialog.visible=true
            flagg=3
        }
        MouseArea{
            anchors.fill: parent
            onPressed:{
                if(image5.checked == true){
                    image5.checked = false
                    sendlatitude.visible = false
                }
                if(image6.checked == true){
                    image6.checked = false
                    test.visible = false
                }

                image1.checked=true
                ima.get()
            }
            onReleased:{
                image1.checked=false
                ima.recover()
            }
        }
    }
    //==========================================

    //============================================ added by wang li cheng
    Rectangle{
        id:ima1
        x:clickxx
        y:clickyy
        color:          "transparent"
        opacity: 0.1
        z:              QGroundControl.zOrderMapItems+1
        visible : false
        width:image2.width
        height:image2.height
        PropertyAnimation on x{id:ani4;to: clickxx+image2.width*2 ; duration:1000}
        NumberAnimation {id:ani6;target:ima1;properties:"opacity";from:0.1;to: 0.9 ; duration:500}

        RoundButton {
            id:            image2
            visible:       true
            buttonImage:    "/qmlimages/ZoomMinu.svg"
        }
        function gett(){
            timer4.start()
            timerLine.start()
            multiple=0.009
            missionController.visualItems.get(1).flage=true
        }
        Timer{
            id:timer4
            interval:300
            running:false
            repeat:true
            onTriggered: ima1.dododo()
        }
        function dododo(){
            var coordinatee1 = flightMap.toCoordinate(Qt.point(xx,yy))
//            if((waypointlatiOffsetArray.length != 0) && (waypointlatiOffsetArray.length != 0))
//            {
//                waypointlatiOffsetArray.length = 0;
//                waypointlonOffsetArray.length = 0
//            }
            if(smalNum == 1){
                for(var i = 0; i < missionController.visualItems.count; i++){
                     var item = missionController.visualItems.get(i)
                    if(item.sequenceNumber !==0){
                        var latitudedis = item.coordinate.latitude - coordinatee1.latitude
                        var longitudedis = item.coordinate.longitude - coordinatee1.longitude
                        latitudedis = formatDouble(latitudedis)
                        longitudedis = formatDouble(longitudedis)
                        console.log("sequencenumber: "  + item.sequenceNumber + " i: " + i + " " + latitudedis + " " + longitudedis)
                        waypointlatiOffsetArray.push(latitudedis)
                        waypointlonOffsetArray.push(longitudedis)
                    }
                }
            }
            for(var i = 0;i<missionController.visualItems.count;i++){
                var m = missionController.visualItems.get(i);
                if(m.sequenceNumber!==0){
                    m.coordinate.latitude = -(m.coordinate.latitude - coordinatee1.latitude)*multiple + m.coordinate.latitude
                    m.coordinate.longitude = -(m.coordinate.longitude - coordinatee1.longitude)*multiple + m.coordinate.longitude

                    coordinatee.altitude = m.coordinate.altitude
                    coordinatee.latitude = m.coordinate.latitude
                    coordinatee.longitude =  m.coordinate.longitude
                    m.coordinate = coordinatee
                }
            }
            smalNum++
        }
        function recoverr(){
            timerLine.stop()
            timer4.stop()
            messageDialog.visible=true
            flagg=4
        }
        MouseArea{
            anchors.fill: parent
            onPressed: {
                if(image5.checked == true){
                    image5.checked = false
                    sendlatitude.visible = false
                }
                if(image6.checked == true){
                    image6.checked = false
                    test.visible = false
                }
                image2.checked=true
                ima1.gett()
            }
            onReleased: {
                image2.checked=false
                ima1.recoverr()
            }
        }
    }

    //============================================ added by wang li cheng
    Rectangle{
        id:ima2
        x:clickxx
        y:clickyy
        color:          "transparent"
        opacity: 0.1
        visible : false
        z:              QGroundControl.zOrderMapItems+1
        width:image3.width
        height:image3.height
        PropertyAnimation on x{id:ani7;to: clickxx-image3.width; duration:1000}
        PropertyAnimation on y{id:ani8;to: clickyy+image3.width*1.732 ; duration:1000}
        NumberAnimation {id:ani9;target:ima2;properties:"opacity";from:0.1;to: 0.9 ; duration:500}

        RoundButton {
            id:             image3
            visible:       true
            buttonImage:    "/qmlimages/rotateButt.svg"
        }
        function gettt(){
            timerLine.start()
            timer5.start()
            missionController.visualItems.get(1).flage=true
        }
        Timer{
            id:timer5
            interval:10
            running:false
            repeat:true
            onTriggered:{
                 angleNum++
                ima2.dodododo()
            }
        }
        function dodododo(){
            if(angleNum <= 50){
                rotate = 0.1
            }else if(angleNum <=100){
                rotate = 0.2
            }else if(angleNum <= 150){
                rotate = 0.3
            }else if (angleNum<= 200){
                rotate = 0.4
            }else if(angleNum > 200) {
                rotate = 0.5
            }
            var origin = mousecoord
            var ref_lon_rad = origin.longitude * 3.141592654/180;
            var ref_lat_rad = origin.latitude * 3.141592654/180;

            for(var i = 0;i<missionController.visualItems.count;i++)
            {
                var m = missionController.visualItems.get(i);
                if(m.sequenceNumber!==0){
                    var lat_rad = m.coordinate.latitude * 3.141592654/180;
                    var lon_rad = m.coordinate.longitude * 3.141592654/180;
                    var sin_lat = Math.sin(lat_rad);
                    var cos_lat = Math.cos(lat_rad);
                    var cos_d_lon = Math.cos(lon_rad - ref_lon_rad);

                    var ref_sin_lat = Math.sin(ref_lat_rad);
                    var ref_cos_lat = Math.cos(ref_lat_rad);

                    var c = Math.acos(ref_sin_lat * sin_lat + ref_cos_lat * cos_lat * cos_d_lon);
                    var k = (Math.abs(c) < 1.1920929e-07) ? 1.0 : (c / Math.sin(c));

                    var x = k * (ref_cos_lat * sin_lat - ref_sin_lat * cos_lat * cos_d_lon) * 6371000;
                    var y = k * cos_lat * Math.sin(lon_rad - ref_lon_rad) * 6371000;
                    var hhhh
                    var llll
                    hhhh = x*Math.cos(rotate*3.141592654/180)-y*Math.sin(rotate*3.141592654/180)
                    llll = x*Math.sin(rotate*3.141592654/180)+y*Math.cos(rotate*3.141592654/180)

                    var x_rad = hhhh / 6371000
                    var y_rad = llll / 6371000
                    c = Math.sqrt(x_rad * x_rad + y_rad * y_rad);
                    var sin_c = Math.sin(c);
                    var cos_c = Math.cos(c);

                    if (Math.abs(c) > 1.1920929e-07) {
                        lat_rad = Math.asin(cos_c * ref_sin_lat + (x_rad * sin_c * ref_cos_lat) / c);
                        lon_rad = (ref_lon_rad + Math.atan2(y_rad * sin_c, c * ref_cos_lat * cos_c - x_rad * ref_sin_lat * sin_c));
                    } else {
                        lat_rad = ref_lat_rad;
                        lon_rad = ref_lon_rad;
                    }
                    m.coordinate.latitude = lat_rad*180.0/3.141592654
                    m.coordinate.longitude = lon_rad*180.0/3.141592654
                }                
                if(m.command == 206 && m.param1 > 0){
                    var cam_lat_rad = m.param3 * 3.141592654/180
                    var cam_lon_rad = m.param4 * 3.141592654/180
                    var cam_sin_lat = Math.sin(cam_lat_rad)
                    var cam_cos_lat = Math.cos(cam_lat_rad)
                    var cam_cos_d_lon = Math.cos(cam_lon_rad - ref_lon_rad);
                    var cam_ref_sin_lat = Math.sin(ref_lat_rad);
                    var cam_ref_cos_lat = Math.cos(ref_lat_rad);

                    var cam_c = Math.acos(cam_ref_sin_lat * cam_sin_lat + cam_ref_cos_lat * cam_cos_lat * cam_cos_d_lon);
                    var cam_k = (Math.abs(cam_c) < 1.1920929e-07) ? 1.0 : (cam_c / Math.sin(cam_c));

                    var cam_x = cam_k * (cam_ref_cos_lat * cam_sin_lat - cam_ref_sin_lat * cam_cos_lat * cam_cos_d_lon) * 6371000;
                    var cam_y = cam_k * cam_cos_lat * Math.sin(cam_lon_rad - ref_lon_rad) * 6371000;
                    var cam_hhhh = cam_x*Math.cos(rotate*3.141592654/180)-cam_y*Math.sin(rotate*3.141592654/180)
                    var cam_llll = cam_x*Math.sin(rotate*3.141592654/180)+cam_y*Math.cos(rotate*3.141592654/180)
                    var cam_x_rad = cam_hhhh / 6371000
                    var cam_y_rad = cam_llll / 6371000
                    cam_c = Math.sqrt(cam_x_rad * cam_x_rad + cam_y_rad * cam_y_rad);
                    var cam_sin_c = Math.sin(cam_c);
                    var cam_cos_c = Math.cos(cam_c);
                    if (Math.abs(cam_c) > 1.1920929e-07) {
                        cam_lat_rad = Math.asin(cam_cos_c * cam_ref_sin_lat + (cam_x_rad * cam_sin_c * cam_ref_cos_lat) / cam_c);
                        cam_lon_rad = (ref_lon_rad + Math.atan2(cam_y_rad * cam_sin_c, cam_c * cam_ref_cos_lat * cam_cos_c - cam_x_rad * cam_ref_sin_lat * cam_sin_c));
                    } else {
                        cam_lat_rad = ref_lat_rad;
                        cam_lon_rad = ref_lon_rad;
                    }
                    m.param3 = cam_lat_rad*180.0/3.141592654
                    m.param4 = cam_lon_rad*180.0/3.141592654
                }
            }
        }

        function recoverrr(){
            timerLine.stop()
            timer5.stop()
            messageDialog.visible=true
            flagg=5
        }
        MouseArea{
            anchors.fill: parent
            onPressed: {
                if(image5.checked == true){
                    image5.checked = false
                    sendlatitude.visible = false
                }
                if(image6.checked == true){
                    image6.checked = false
                    test.visible = false
                }
                ima2.gettt()
                image3.checked = true
            }
            onReleased: {
                image3.checked=false
                ima2.recoverrr()
            }
        }
    }
    //======================================== added by wang li cheng
    Rectangle{
        id:ima3
        x:clickxx
        y:clickyy
        color:          "transparent"
        z:              QGroundControl.zOrderMapItems+1
        opacity: 0.1
        visible : false
        width:image4.width
        height:image4.height
        PropertyAnimation on x{id:ani10;to: clickxx+image4.width ; duration:1000}
        PropertyAnimation on y{id:ani11;to: clickyy+image4.width*1.732 ; duration:1000}
        NumberAnimation {id:ani12;target:ima3;properties:"opacity";from:0.1;to: 0.9 ; duration:500}
        function gettt(){
            timer6.start()
            timerLine.start()
            missionController.visualItems.get(1).flage=true
        }

        function dodododo(){
            if(angleNum <= 50){
                rotate = 0.1
            }else if(angleNum <=100){
                rotate = 0.2
            }else if(angleNum <= 150){
                rotate = 0.3
            }else if (angleNum<= 200){
                rotate = 0.4
            }else if(angleNum > 200) {
                rotate = 0.5
            }
            //var origin = mousecoord
            var origin = mousecoord //flightMap.toCoordinate(Qt.point(xx,yy))
            var ref_lon_rad = origin.longitude * 3.141592654/180;
            var ref_lat_rad = origin.latitude * 3.141592654/180;
            for(var i = 0;i<missionController.visualItems.count;i++)
            {
                var m = missionController.visualItems.get(i);
                if(m.sequenceNumber!==0){
                    var lat_rad = m.coordinate.latitude * 3.141592654/180;
                    var lon_rad = m.coordinate.longitude * 3.141592654/180;
                    var sin_lat = Math.sin(lat_rad);
                    var cos_lat = Math.cos(lat_rad);
                    var cos_d_lon = Math.cos(lon_rad - ref_lon_rad);

                    var ref_sin_lat = Math.sin(ref_lat_rad);
                    var ref_cos_lat = Math.cos(ref_lat_rad);

                    var c = Math.acos(ref_sin_lat * sin_lat + ref_cos_lat * cos_lat * cos_d_lon);
                    var k = (Math.abs(c) < 1.1920929e-07) ? 1.0 : (c / Math.sin(c));

                    var x = k * (ref_cos_lat * sin_lat - ref_sin_lat * cos_lat * cos_d_lon) * 6371000;
                    var y = k * cos_lat * Math.sin(lon_rad - ref_lon_rad) * 6371000;
                    var hhhh
                    var llll

                    hhhh = x*Math.cos(-rotate*3.141592654/180)-y*Math.sin(-rotate*3.141592654/180)
                    llll = x*Math.sin(-rotate*3.141592654/180)+y*Math.cos(-rotate*3.141592654/180)

                    var x_rad = hhhh / 6371000
                    var y_rad = llll / 6371000
                    c = Math.sqrt(x_rad * x_rad + y_rad * y_rad);
                    var sin_c = Math.sin(c);
                    var cos_c = Math.cos(c);
                    if (Math.abs(c) > 1.1920929e-07) {
                        lat_rad = Math.asin(cos_c * ref_sin_lat + (x_rad * sin_c * ref_cos_lat) / c);
                        lon_rad = (ref_lon_rad + Math.atan2(y_rad * sin_c, c * ref_cos_lat * cos_c - x_rad * ref_sin_lat * sin_c));
                    } else {
                        lat_rad = ref_lat_rad;
                        lon_rad = ref_lon_rad;
                    }
                    m.coordinate.latitude = lat_rad*180.0/3.141592654
                    m.coordinate.longitude = lon_rad*180.0/3.141592654
                }
                if(m.command == 206 && m.param1 > 0){
                    var cam_lat_rad = m.param3 * 3.141592654/180;
                    var cam_lon_rad = m.param4 * 3.141592654/180;
                    var cam_sin_lat = Math.sin(cam_lat_rad);
                    var cam_cos_lat = Math.cos(cam_lat_rad);
                    var cam_cos_d_lon = Math.cos(cam_lon_rad - ref_lon_rad);

                    var cam_ref_sin_lat = Math.sin(ref_lat_rad);
                    var cam_ref_cos_lat = Math.cos(ref_lat_rad);

                    var cam_c = Math.acos(cam_ref_sin_lat * cam_sin_lat + cam_ref_cos_lat * cam_cos_lat * cam_cos_d_lon);
                    var cam_k = (Math.abs(cam_c) < 1.1920929e-07) ? 1.0 : (cam_c / Math.sin(cam_c));

                    var cam_x = cam_k * (cam_ref_cos_lat * cam_sin_lat - cam_ref_sin_lat * cam_cos_lat * cam_cos_d_lon) * 6371000;
                    var cam_y = cam_k * cam_cos_lat * Math.sin(cam_lon_rad - ref_lon_rad) * 6371000;
                    var cam_hhhh = cam_x*Math.cos(-rotate*3.141592654/180)-cam_y*Math.sin(-rotate*3.141592654/180)
                    var cam_llll = cam_x*Math.sin(-rotate*3.141592654/180)+cam_y*Math.cos(-rotate*3.141592654/180)
                    var cam_x_rad = cam_hhhh / 6371000
                    var cam_y_rad = cam_llll / 6371000
                    cam_c = Math.sqrt(cam_x_rad * cam_x_rad + cam_y_rad * cam_y_rad);
                    var cam_sin_c = Math.sin(cam_c);
                    var cam_cos_c = Math.cos(cam_c);
                    if (Math.abs(cam_c) > 1.1920929e-07) {
                        cam_lat_rad = Math.asin(cam_cos_c * cam_ref_sin_lat + (cam_x_rad * cam_sin_c * cam_ref_cos_lat) / cam_c);
                        cam_lon_rad = (ref_lon_rad + Math.atan2(cam_y_rad * cam_sin_c, cam_c * cam_ref_cos_lat * cam_cos_c - cam_x_rad * cam_ref_sin_lat * cam_sin_c));
                    } else {
                        cam_lat_rad = ref_lat_rad;
                        cam_lon_rad = ref_lon_rad;
                    }
                    m.param3 = cam_lat_rad*180.0/3.141592654
                    m.param4 = cam_lon_rad*180.0/3.141592654
                }
            }
        }
        Timer{
            id:timer6
            interval:10
            running:false
            repeat:true
            onTriggered:{
                angleNum++
                ima3.dodododo()
            }
        }

        function recoverrr(){
            timerLine.stop()
            timer6.stop()
            messageDialog.visible=true
            flagg=6
        }
        RoundButton {
            id:             image4
            visible:       true
            buttonImage:    "/qmlimages/rotateButto.svg"
        }
        MouseArea{
            anchors.fill: parent
            onPressed: {
                if(image5.checked == true){
                    image5.checked = false
                    sendlatitude.visible = false
                }
                if(image6.checked == true){
                    image6.checked = false
                    test.visible = false
                }
                ima3.gettt()
                image4.checked = true
            }
            onReleased: {
                image4.checked=false
                ima3.recoverrr()
            }
        }
    }
    Rectangle{

        id:ima4
        x:clickxx
        y:clickyy
        color:          "transparent"
        opacity: 0.1
        visible : false
        z:              QGroundControl.zOrderMapItems+1
        width:image5.width
        height:image5.height
        PropertyAnimation on x{id:ani13;to: clickxx-image5.width ; duration:1000}
        PropertyAnimation on y{id:ani14;to: clickyy-image5.width*1.732 ; duration:1000}
        NumberAnimation {id:ani15;target:ima4;properties:"opacity";from:0.1;to: 0.9 ; duration:500}

        RoundButton {
            id:             image5
            visible:       true
            buttonImage:    "/qmlimages/sendd.svg"

            onClicked: {
                if(image6.checked == true)
                    image6.checked = false
                if(sendlatitude.visible == false)
                    sendlatitude.visible = true
                else
                    sendlatitude.visible = false
            }
        }

    }
    // Handle guided mode clicks added by wang li cheng
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: {
            if (mouse.button == Qt.RightButton) {
                if (_activeVehicle && _activeVehicle.followMeMode) {
                    _gotoHereCoordinate = flightMap.toCoordinate(Qt.point(mouse.x, mouse.y))
                    _activeVehicle.guidedModeFollowMe(_gotoHereCoordinate)
                }
            }
        }

        onClicked: {
            if (_activeVehicle) {
                if(mouse.button == Qt.LeftButton) {
                    if (_activeVehicle.guidedMode && flightWidgets.guidedModeBar.state == "Shown") {
                       _gotoHereCoordinate = flightMap.toCoordinate(Qt.point(mouse.x, mouse.y))
                       flightWidgets.guidedModeBar.confirmAction(flightWidgets.guidedModeBar.confirmGoTo)
                    } else {
                        flightWidgets.guidedModeBar.state = "Shown"
                        flagg = 0
                        //====================================================
                        xx=mouse.x
                        yy=mouse.y

                    clickxx=mouse.x
                    clickyy=mouse.y
                    ffff = false
                    mousecoord = flightMap.toCoordinate(Qt.point(clickxx, clickyy))
                    im.visible = false
                    ima.visible = false
                    ima1.visible = false
                    ima2.visible = false
                    ima3.visible = false
                    ima4.visible = false
                    var d
                    if(mapss.maptext>=10&&mapss.maptext<100){
                        d=2
                    }
                    if(mapss.maptext>=100&&mapss.maptext<250){
                        d=10
                    }
                    if(mapss.maptext>=250&&mapss.maptext<=500){
                        d=30
                    }
                    if(mapss.maptext>500&&mapss.maptext<=1000){
                        d=40
                    }
                    if(mapss.maptext<10){
                        d=100
                    }
                    var maporigin = flightMap.toCoordinate(Qt.point(xx,yy))

                    for(var i = 0; i <  missionController.waypointLines.count;i++){

                        var origin = missionController.waypointLines.get(i).coordinate1
                        var coord = missionController.waypointLines.get(i).coordinate2
                        var ref_lon_rad = origin.longitude * 3.141592654/180;
                        var ref_lat_rad = origin.latitude * 3.141592654/180;
                        var lat_rad = coord.latitude * 3.141592654/180;
                        var lon_rad = coord.longitude * 3.141592654/180;
                        var sin_lat = Math.sin(lat_rad);
                        var cos_lat = Math.cos(lat_rad);
                        var cos_d_lon = Math.cos(lon_rad - ref_lon_rad);

                        var ref_sin_lat = Math.sin(ref_lat_rad);
                        var ref_cos_lat = Math.cos(ref_lat_rad);

                        var c = Math.acos(ref_sin_lat * sin_lat + ref_cos_lat * cos_lat * cos_d_lon);
                        var k = (Math.abs(c) < 1.1920929e-07) ? 1.0 : (c / Math.sin(c));

                        var x = k * (ref_cos_lat * sin_lat - ref_sin_lat * cos_lat * cos_d_lon) * 6371000;
                        var y = k * cos_lat * Math.sin(lon_rad - ref_lon_rad) * 6371000;
                        //==============================================================
                        lat_rad = maporigin.latitude * 3.141592654/180;
                        lon_rad = maporigin.longitude * 3.141592654/180;
                        sin_lat = Math.sin(lat_rad);
                        cos_lat = Math.cos(lat_rad);
                        cos_d_lon = Math.cos(lon_rad - ref_lon_rad);

                        ref_sin_lat = Math.sin(ref_lat_rad);
                        ref_cos_lat = Math.cos(ref_lat_rad);

                        c = Math.acos(ref_sin_lat * sin_lat + ref_cos_lat * cos_lat * cos_d_lon);
                        k = (Math.abs(c) < 1.1920929e-07) ? 1.0 : (c / Math.sin(c));

                        var x1 = k * (ref_cos_lat * sin_lat - ref_sin_lat * cos_lat * cos_d_lon) * 6371000;
                        var y1 = k * cos_lat * Math.sin(lon_rad - ref_lon_rad) * 6371000;

//                        console.log("X:"+ x + "Y:"+ y + "\n"
//                                    + "X1:"+ x1 + "Y1:" + y1)
                        var cos_angle = (x * x1 + y * y1)/Math.sqrt((Math.pow(x,2) + Math.pow(y,2)) * (Math.pow(x1,2) + Math.pow(y1,2))) //向量夹角
                        var angle = Math.acos(cos_angle);
                        var distance = Math.sqrt(Math.pow(x1,2) + Math.pow(y1,2)) * Math.sin(angle) //距离公式
                        var dis = Math.sqrt(Math.pow(x1,2) + Math.pow(y1,2))
                        var dis2 = Math.sqrt(Math.pow(x,2) + Math.pow(y,2))
//                        console.log("cos_angle: " + cos_angle + "angle: " + angle + "sin_angle : " + Math.sin(angle) + "distance ：" + distance)
                        if(distance < d){
                            if(0<cos_angle  && dis < dis2){ //夹角在-90 到90度之间
                                fff = true;
                                break
                            }
                        }
//                        if(Math.abs((x1*y/x-y1)/Math.sqrt(1+Math.pow(y/x,2)))<=d&&x!=0&&y!=0){
//                            if(y>=0){
//                                if(x>=0){
//                                    if(x1>=0&&x1<=x&&y1<=y+2){
//                                        fff=true
//                                    }
//                                }
//                                if(x<0){
//                                    if(x1<=0&&x1>=x&&y1<=y)fff=true
//                                }
//                            }
//                            if(y<0){
//                                if(x>=0){
//                                    if(x1>=0&&x1<=x&&y1>=y)fff=true
//                                }
//                                if(x<0){
//                                    if(x1<=0&&x1>=x&&y1>=y)fff=true
//                                }
//                            }

//                        }
//                        console.log("y/x  :" + y/x + "-----------------")
//                        if(y/x>=10000||y/x<=-10000){
//                            if(y>=0){
//                                if(x1<=d&&x1>=-d&&y1<=y &&y1 > 0){
//                                    fff=true
//                                }
//                            }
//                            if(y<0){
//                                if(x1<=d&&x1>=-d&&y1>=y && y1 < 0 ){
//                                    fff=true
//                                }
//                            }
//                        }
//                        if(y/x<=0.05&&y/x>=-0.05){
//                            if(x>=0){
//                                if(y1<=d&&y1>=-d&&x1<=x){
//                                    fff=true
//                                }
//                            }
//                            if(x<0){
//                                if(y1<=d&&y1>=-d&&x1>=x){
//                                    fff=true
//                                }
//                            }
//                        }
//                        if(fff)break
                    }


                    //===================================================
                    if(fff){
                        ffff = true
                        missionController.loadMoveLineAdd()
                        if(currentitemflag){
                        }
                        if(ima.visible){
                            im.visible = false
                            ima.visible = false
                            ima1.visible = false
                            ima2.visible = false
                            ima3.visible = false
                            ima4.visible = false
                        }
                        if(!ima.visible){
                            im.visible = true
                            ima.visible = true
                            ima1.visible = true
                            ima2.visible = true
                            ima3.visible = true
                            ima4.visible = true
                            im.x = clickxx
                            im.y = clickyy
                            ima.x = clickxx
                            ima1.x = clickxx
                            ima2.x = clickxx
                            ima2.y = clickyy
                            ima3.x = clickxx
                            ima3.y = clickyy
                            ima4.x = clickxx
                            ima4.y = clickyy
                            ani1.start()
                            ani3.start()
                            ani4.start()
                            ani6.start()
                            ani7.start()
                            ani8.start()
                            ani9.start()
                            ani10.start()
                            ani11.start()
                            ani12.start()
                            ani13.start()
                            ani14.start()
                            ani15.start()
                            ani16.start()
                            ani17.start()
                            ani18.start()
                        }
                        fff=false
                        }
                    }
                }
            }
        }
    }
}
