/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.4
import QtQuick.Controls 1.3
import QtQuick.Dialogs  1.2
import QtLocation       5.3
import QtPositioning    5.3
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Mavlink       1.0
import QGroundControl.Controllers   1.0

/// Mission Editor

QGCView {
    id:         qgcView
    viewPanel:  panel

    // zOrder comes from the Loader in MainWindow.qml
    z: QGroundControl.zOrderTopMost

    readonly property int       _decimalPlaces:         8
    readonly property real      _horizontalMargin:      ScreenTools.defaultFontPixelWidth  / 2
    readonly property real      _margin:                ScreenTools.defaultFontPixelHeight * 0.5
    readonly property var       _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    readonly property var       _vehicType :             _activeVehicle? _activeVehicle.vehicletype : 100
    readonly property real      _rightPanelWidth:       Math.min(parent.width / 3, ScreenTools.defaultFontPixelWidth * 30)
    readonly property real      _rightPanelOpacity:     0.8
    readonly property int       _toolButtonCount:       6
    readonly property real      _toolButtonTopMargin:   parent.height - ScreenTools.availableHeight + (ScreenTools.defaultFontPixelHeight / 2)
    readonly property var       _defaultVehicleCoordinate:   QtPositioning.coordinate(37.803784, -122.462276)

    property var    _visualItems:           missionController.visualItems
    property var    _currentMissionItem
    property int    _currentMissionIndex:   0
    property bool   _firstVehiclePosition:  true
    property var    activeVehiclePosition:  _activeVehicle ? _activeVehicle.coordinate : QtPositioning.coordinate()
    property bool   _lightWidgetBorders:    editorMap.isSatelliteMap
    //===================================================================
    property var mm:    0
    property var nn:    0
    property var mmm:   0
    property var nnn:   0
    property var flagg: 0
//    property  var co1:QtPositioning.coordinate(37.803784, -122.462276)
//    property var co2:QtPositioning.coordinate(37.803784, -122.462276)
    property var largNum: 1
    property var smalNum: 1
    property var angleNum:0

    property var xx:editorMap.fromCoordinate(missionController.visualItems.get(0).coordinate).x
    property var yy:editorMap.fromCoordinate(missionController.visualItems.get(0).coordinate).y
    property var clickxx:   0
    property var clickyy:   0
    property var originalx: 0
    property var originaly: 0
    property var multiple:  0
    property var mousecoord:QtPositioning.coordinate(37.803784, -122.462276)
    property var ff:        1
    property bool fff:            false
    property bool currentitemflag:false
    property var rotateFlag:0
    property var rotate:0
    property var coordinatee: QtPositioning.coordinate(37.803784, -122.462276)

    property var shuzu:                 new Array
    //====================================================================

    /// The controller which should be called for load/save, send to/from vehicle calls
    property var _syncDropDownController: missionController

    readonly property int _layerMission:        1
    readonly property int _layerGeoFence:       2
    readonly property int _layerRallyPoints:    3
    property int _editingLayer: _layerMission
    property bool measureflag : true

    onActiveVehiclePositionChanged: updateMapToVehiclePosition()

    Connections {
        target: QGroundControl.multiVehicleManager

        onActiveVehicleChanged: {
            // When the active vehicle changes we need to allow the first vehicle position to move the map again
            editorMap.updateposition()
            _firstVehiclePosition = true
            updateMapToVehiclePosition()
        }
    }

    function updateMapToVehiclePosition() {
        if (_activeVehicle && _activeVehicle.coordinateValid && _activeVehicle.coordinate.isValid && _firstVehiclePosition) {
            _firstVehiclePosition = false
            editorMap.center = _activeVehicle.coordinate
        }
    }

    function normalizeLat(lat) {
        // Normalize latitude to range: 0 to 180, S to N
        return lat + 90.0
    }

    function normalizeLon(lon) {
        // Normalize longitude to range: 0 to 360, W to E
        return lon  + 180.0
    }

    /// Fits the visible region of the map to inclues all of the specified coordinates. If no coordinates
    /// are specified the map will fit to the home position
    function fitMapViewportToAllCoordinates(coordList) {
        if(coordList == null){
            return ;
        }

        if (coordList.length == 0) {
            if(_visualItems != null)
            editorMap.center = _visualItems.get(0).coordinate
            return
        }

        // Determine the size of the inner portion of the map available for display
        var toolbarHeight = qgcView.height - ScreenTools.availableHeight
        var rightPanelWidth = _rightPanelWidth
        var leftToolWidth = centerMapButton.x + centerMapButton.width
        var availableWidth = qgcView.width - rightPanelWidth - leftToolWidth
        var availableHeight = qgcView.height - toolbarHeight

        // Create the normalized lat/lon corners for the coordinate bounding rect from the list of coordinates
        var north = normalizeLat(coordList[0].latitude)
        var south = north
        var east = normalizeLon(coordList[0].longitude)
        var west = east
        for (var i = 1; i<coordList.length; i++) {
            var lat = normalizeLat(coordList[i].latitude)
            var lon = normalizeLon(coordList[i].longitude)

            north = Math.max(north, lat)
            south = Math.min(south, lat)
            east = Math.max(east, lon)
            west = Math.min(west, lon)
        }

        // Expand the coordinate bounding rect to make room for the tools around the edge of the map
        var latDegreesPerPixel = (north - south) / availableWidth
        var lonDegreesPerPixel = (east - west) / availableHeight
        north = Math.min(north + (toolbarHeight * latDegreesPerPixel), 180)
        west = Math.max(west - (leftToolWidth * lonDegreesPerPixel), 0)
        east = Math.min(east + (rightPanelWidth * lonDegreesPerPixel), 360)

        // Fix the map region to the new bounding rect
        var topLeftCoord = QtPositioning.coordinate(north - 90.0, west - 180.0)
        var bottomRightCoord  = QtPositioning.coordinate(south - 90.0, east - 180.0)
        editorMap.visibleRegion = QtPositioning.rectangle(topLeftCoord, bottomRightCoord)
    }

    function addMissionItemCoordsForFit(coordList) {
        if(qgcView._visualItems == null){
            return
        }

        for (var i = 1; i<qgcView._visualItems.count; i++) {
           var  items = qgcView._visualItems.get(i)
            if (items.specifiesCoordinate && !items.isStandaloneCoordinate) {
                coordList.push(items.coordinate)
            }
        }
    }

    function fitMapViewportToMissionItems() {
        var coordList = [ ]
        addMissionItemCoordsForFit(coordList)
        fitMapViewportToAllCoordinates(coordList)
    }

    function addFenceItemCoordsForFit(coordList) {
        if (geoFenceController.circleSupported) {
            var azimuthList = [ 0, 180, 90, 270 ]
            for (var i=0; i<azimuthList.length; i++) {
                var edgeCoordinate = _visualItems.get(0).coordinate.atDistanceAndAzimuth(geoFenceController.circleRadius, azimuthList[i])
                coordList.push(edgeCoordinate)
            }
        }
        if (geoFenceController.polygonSupported && geoFenceController.polygon.count() > 2) {
            for (var i=0; i<geoFenceController.polygon.count(); i++) {
                coordList.push(geoFenceController.polygon.path[i])
            }
        }
    }

    function fitMapViewportToFenceItems() {
        var coordList = [ ]
        addFenceItemCoordsForFit(coordList)
        fitMapViewportToAllCoordinates(coordList)
    }

    function addRallyItemCoordsForFit(coordList) {
        for (var i=0; i<rallyPointController.points.count; i++) {
            coordList.push(rallyPointController.points.get(i).coordinate)
        }
    }

    function fitMapViewportToRallyItems() {
        var coordList = [ ]
        addRallyItemCoordsForFit(coordList)
        fitMapViewportToAllCoordinates(coordList)
    }

    function fitMapViewportToAllItems() {
        var coordList = [ ]
        addMissionItemCoordsForFit(coordList)
        addFenceItemCoordsForFit(coordList)
        addRallyItemCoordsForFit(coordList)
        fitMapViewportToAllCoordinates(coordList)
    }

    property bool _firstMissionLoadComplete:    false
    property bool _firstFenceLoadComplete:      false
    property bool _firstRallyLoadComplete:      false
    property bool _firstLoadComplete:           false

    function checkFirstLoadComplete() {
        if (!_firstLoadComplete && _firstMissionLoadComplete && _firstRallyLoadComplete && _firstFenceLoadComplete) {
            _firstLoadComplete = true
            fitMapViewportToAllItems()
        }
    }

    MissionController {
        id: missionController

        Component.onCompleted: {
            start(true /* editMode */)
            setCurrentItem(0)
        }

        function loadFromSelectedFile() {
            if (ScreenTools.isMobile) {
                qgcView.showDialog(mobileFilePicker, qsTr("Select Mission File"), qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
            } else {
                missionController.loadFromFilePicker()
                fitMapViewportToMissionItems()
                _currentMissionItem = _visualItems.get(0)
            }
        }

        function saveToSelectedFile() {
            if (ScreenTools.isMobile) {
                qgcView.showDialog(mobileFileSaver, qsTr("Save Mission File"), qgcView.showDialogDefaultWidth, StandardButton.Save | StandardButton.Cancel)
            } else {
                missionController.saveToFilePicker()
            }
        }

        function fitViewportToItems() {
            fitMapViewportToMissionItems()
        }

        onVisualItemsChanged: {
            if(ima.visible){
                im.visible = false
                ima.visible = false
                ima1.visible = false
                ima2.visible = false
                ima3.visible = false
                ima4.visible = false
            }
            itemDragger.clearItem()
            if(editorMap.doingAdjustPolygon){
                editorMap.colseAdjustPolygon()
            }
        }

        onNewItemsFromVehicle: {
            fitMapViewportToMissionItems()
            setCurrentItem(0)
            _firstMissionLoadComplete = true
            checkFirstLoadComplete()
        }
    }

    GeoFenceController {
        id: geoFenceController

        Component.onCompleted: start(true /* editMode */)

        function saveToSelectedFile() {
            if (ScreenTools.isMobile) {
                qgcView.showDialog(mobileFileSaver, qsTr("Save Fence File"), qgcView.showDialogDefaultWidth, StandardButton.Save | StandardButton.Cancel)
            } else {
                geoFenceController.saveToFilePicker()
            }
        }

        function loadFromSelectedFile() {
            if (ScreenTools.isMobile) {
                qgcView.showDialog(mobileFilePicker, qsTr("Select Fence File"), qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
            } else {
                geoFenceController.loadFromFilePicker()
                fitMapViewportToFenceItems()
            }
        }

        function validateBreachReturn() {
            if (geoFenceController.polygon.path.length > 0) {
                if (!geoFenceController.polygon.containsCoordinate(geoFenceController.breachReturnPoint)) {
                    geoFenceController.breachReturnPoint = geoFenceController.polygon.center()
                }
                if (!geoFenceController.polygon.containsCoordinate(geoFenceController.breachReturnPoint)) {
                    geoFenceController.breachReturnPoint = geoFenceController.polygon.path[0]
                }
            }
        }

        function fitViewportToItems() {
            fitMapViewportToFenceItems()
        }

        onLoadComplete: {
            _firstFenceLoadComplete = true
            switch (_syncDropDownController) {
            case geoFenceController:
                fitMapViewportToFenceItems()
                break
            case missionController:
                checkFirstLoadComplete()
                break
            }
        }
    }

    RallyPointController {
        id: rallyPointController

        onCurrentRallyPointChanged: {
            if (_editingLayer == _layerRallyPoints && !currentRallyPoint) {
                itemDragger.visible = false
                itemDragger.coordinateItem = undefined
                itemDragger.mapCoordinateIndicator = undefined
            }
        }

        Component.onCompleted: start(true /* editMode */)

        function saveToSelectedFile() {
            if (ScreenTools.isMobile) {
                qgcView.showDialog(mobileFileSaver, qsTr("Save Rally Point File"), qgcView.showDialogDefaultWidth, StandardButton.Save | StandardButton.Cancel)
            } else {
                rallyPointController.saveToFilePicker()
            }
        }

        function loadFromSelectedFile() {
            if (ScreenTools.isMobile) {
                qgcView.showDialog(mobileFilePicker, qsTr("Select Rally Point File"), qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
            } else {
                rallyPointController.loadFromFilePicker()
                fitMapViewportToRallyItems()
            }
        }

        function fitViewportToItems() {
            fitMapViewportToRallyItems()
        }

        onLoadComplete: {
            _firstRallyLoadComplete = true
            switch (_syncDropDownController) {
            case rallyPointController:
                fitMapViewportToRallyItems()
                break
            case missionController:
                checkFirstLoadComplete()
                break
            }
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    ExclusiveGroup {
        id: _mapTypeButtonsExclusiveGroup
    }

    ExclusiveGroup {
        id: _dropButtonsExclusiveGroup
    }

    function setCurrentItem(sequenceNumber) {
        editorMap.polygonDraw.cancelPolygonEdit()
        _currentMissionItem = undefined
        for (var i = 0; i < _visualItems.count; i++) {
            var visualItem = _visualItems.get(i)
            if (visualItem.sequenceNumber == sequenceNumber) {
                _currentMissionItem = visualItem
                _currentMissionItem.isCurrentItem = true
                _currentMissionIndex = i
            } else {
                visualItem.isCurrentItem = false
            }
        }
    }

    property int _moveDialogMissionItemIndex

    Component {
        id: mobileFilePicker

        QGCMobileFileDialog {
            openDialog:         true
            fileExtension:      _syncDropDownController.fileExtension
            onFilenameReturned: {
                _syncDropDownController.loadFromFile(filename)
                _syncDropDownController.fitViewportToItems()
            }
        }
    }

    Component {
        id: mobileFileSaver

        QGCMobileFileDialog {
            openDialog:         false
            fileExtension:      _syncDropDownController.fileExtension
            onFilenameReturned: _syncDropDownController.saveToFile(filename)
        }
    }

    Component {
        id: moveDialog

        QGCViewDialog {
            function accept() {
                var toIndex = toCombo.currentIndex

                if (toIndex == 0) {
                    toIndex = 1
                }
                missionController.moveMissionItem(_moveDialogMissionItemIndex, toIndex)
                hideDialog()
            }

            Column {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        ScreenTools.defaultFontPixelHeight

                QGCLabel {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    wrapMode:       Text.WordWrap
                    text:           qsTr("Move the selected mission item to the be after following mission item:")
                }

                QGCComboBox {
                    id:             toCombo
                    model:          _visualItems.count
                    currentIndex:   _moveDialogMissionItemIndex
                }
            }
        }
    }

    QGCViewPanel {
        id:             panel
        height:         ScreenTools.availableHeight
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        anchors.right:  parent.right

        Item {
            anchors.fill: parent

            FlightMap {
                id:             editorMap
                height:         qgcView.height
                anchors.bottom: parent.bottom
                anchors.left:   parent.left
                anchors.right:  parent.right
                mapName:        "MissionEditor"

                readonly property real animationDuration: 500

                function updateposition(){
                    shuzu.length = 0;
                    for(var j=0;j<polygonLineNextPoint.path.length;j++){
                        if(j<=polygonLineNextPoint.path.length-2 && polygonLineNextPoint.path.length>=2){
                            var origin = polygonLineNextPoint.path[j]
                            var coord = polygonLineNextPoint.path[j+1]
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
                            var linelen = Math.sqrt(Math.pow(x,2)+Math.pow(y,2)).toFixed(2)
                            shuzu.push(linelen)
                        }
                    }
                     rr.model = shuzu.length
                     for(var m=0;m< rr.model && m <  polygonLineNextPoint.path.length ;m++){
                     rr.itemAt(m).x =( editorMap.fromCoordinate(polygonLineNextPoint.path[m]).x + editorMap.fromCoordinate(polygonLineNextPoint.path[m+1]).x)/2
                     rr.itemAt(m).y = (editorMap.fromCoordinate(polygonLineNextPoint.path[m]).y + editorMap.fromCoordinate(polygonLineNextPoint.path[m+1]).y)/2
                     rr.itemAt(m).color = "#FFFFFF"
                     rr.itemAt(m).text=shuzu[m]
                     }
                }

                // Initial map position duplicates Fly view position
                Component.onCompleted: editorMap.center = QGroundControl.flightMapPosition
                onCenterChanged: {
                    if(clickxx!==0){
                        clickxx = editorMap.fromCoordinate(mousecoord).x
                        clickyy = editorMap.fromCoordinate(mousecoord).y
                        ima4.x = clickxx-image5.width
                        ima4.y = clickyy-image5.width*1.732
                        im.x = clickxx+image6.width
                        im.y = clickyy-image6.width*1.732
                        ima.x = clickxx-image1.width*2
                        ima1.x = clickxx+image2.width*2
                        //ima1.y = clickyy+image2.width*1.732
                        ima2.x = clickxx-image3.width
                        ima2.y = clickyy+image3.width*1.732
                        ima3.x = clickxx+image4.width
                        ima3.y = clickyy+image4.width*1.732                  
                        }

                        if(polygonLineNextPoint.path.length >=2){
                            updateposition()
                    }
                    QGroundControl.editorMapPosition = center
                }
                onZoomLevelChanged: {
                    if(clickxx!==0){
                        clickxx = editorMap.fromCoordinate(mousecoord).x
                        clickyy = editorMap.fromCoordinate(mousecoord).y
                        ima4.x = clickxx-image5.width
                        ima4.y = clickyy-image5.width*1.732
                        im.x = clickxx+image6.width
                        im.y = clickyy-image6.width*1.732
                        ima.x = clickxx-image1.width*2
                        ima1.x = clickxx+image2.width*2
                        //ima1.y = clickyy+image2.width*1.732
                        ima2.x = clickxx-image3.width
                        ima2.y = clickyy+image3.width*1.732
                        ima3.x = clickxx+image4.width
                        ima3.y = clickyy+image4.width*1.732
                    }
                    if(polygonLineNextPoint.path.length >=2){
                        updateposition()
                    }
                    QGroundControl.editorMapZoom = zoomLevel
                }

                Behavior on zoomLevel {
                    NumberAnimation {
                        duration:       editorMap.animationDuration
                        easing.type:    Easing.InOutQuad
                    }
                }

                QGCMapPalette { id: mapPal; lightColors: editorMap.isSatelliteMap }
                //===========================================================================
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
                            mm = mmm
                            nn = nnn
                            ff=2
                        }
                        var coordinatee1 = editorMap.toCoordinate(Qt.point(mmm,nnn))
                        var coordinateee2 = editorMap.toCoordinate(Qt.point(mm,nn))
                        var x1 = coordinatee1.latitude-coordinateee2.latitude
                        var y1 = coordinatee1.longitude-coordinateee2.longitude
                        for(var i = 0;i<missionController.visualItems.count;i++)
                        {
                            var m = missionController.visualItems.get(i);
                            coordinatee.altitude = m.coordinate.altitude
                            coordinatee.latitude = m.coordinate.latitude+x1
                            coordinatee.longitude = m.coordinate.longitude+y1
                            m.coordinate = coordinatee
                            //camera
                            if(m.command == 206 && m.param1 > 0.5){
                                m.param3 = m.param3 + x1
                                m.param4 = m.param4 + y1
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
                        editorMap.gesture.enabled=false
                        timer2.start()
                        missionController.visualItems.get(1).flage=true
                    }
                    function dodo()
                    {
                        missionController.loadMoveLine()
                    }
                    function recover()
                    {
                        ff=1
                        editorMap.gesture.enabled=true
                        test.visible=false
                        image6.checked=false
                        timer2.stop()
                        flagg=2
                        im.visible = false
                        ima.visible = false
                        ima1.visible = false
                        ima2.visible = false
                        ima3.visible = false
                        ima4.visible = false
                    missionController.visualItems.get(1).flage=false
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
                        onPressed:{
                            image6.checked=true
                            test.visible = true
                        }
                    }
                }
                //=================================================================
                Timer{
                    id:timerLine
                    interval:100
                    running:false
                    repeat:true
                    onTriggered: test.dodo()
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
                        interval:100
                        running:false
                        repeat:true
                        onTriggered: ima.dodo()
                    }
                    function dodo(){
                        var coordinatee1 = editorMap.toCoordinate(Qt.point(clickxx,clickyy))
                        var sizeVisiualItems = missionController.visualItems.count
                        for(var i = 0; i < sizeVisiualItems; i++)
                        {
                             var m = missionController.visualItems.get(i)
                            if(m.sequenceNumber!==0){
                                m.coordinate.latitude = (m.coordinate.latitude - coordinatee1.latitude)*multiple+m.coordinate.latitude
                                m.coordinate.longitude = (m.coordinate.longitude - coordinatee1.longitude)*multiple+m.coordinate .longitude
                                coordinatee.altitude = m.coordinate.altitude
                                coordinatee.latitude = m.coordinate.latitude
                                coordinatee.longitude =  m.coordinate.longitude
                                m.coordinate = coordinatee
                            }

                            if(m.command == 206 && m.param1 >= 0.5)
                            {
                                var cameCoord1 = QtPositioning.coordinate(m.param3, m.param4)
                                var camDist1 = parseInt(m.param1)
                                cameCoord1.latitude = (cameCoord1.latitude - coordinatee1.latitude)*multiple+cameCoord1.latitude
                                cameCoord1.longitude = (cameCoord1.longitude  - coordinatee1.longitude)*multiple+cameCoord1.longitude
                                m.param3 = cameCoord1.latitude
                                m.param4 = cameCoord1.longitude
                                for(var j = i -1; j > 0; j-- ){
                                    var item = missionController.visualItems.get(j)
                                    if(item.specifiesCoordinate){
                                        var lat_previousitem = item.coordinate.latitude
                                        var lon_previousitem = item.coordinate.longitude
                                        var lat_nowitem_param3 = cameCoord1.latitude
                                        var lon_nowitem_param4 = cameCoord1.longitude
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

                                 //get_distance_to_next_waypoint
                                if(camDist1 > 0)
                                {
                                    var lat_now = cameCoord1.latitude
                                    var lon_now = cameCoord1.longitude
                                    var lat_next = m.coordinate.latitude
                                    var lon_next = m.coordinate.longitude
                                    var way_point_distance = Math.round((m.param1 - camDist1) * 1000) * camDist1 // previously distance
                                    var lat_now_rad = lat_now / 180.0 * 3.14159265358979323846
                                    var lon_now_rad = lon_now / 180.0 * 3.14159265358979323846
                                    var lat_next_rad = lat_next / 180.0 * 3.14159265358979323846
                                    var lon_next_rad = lon_next / 180.0 * 3.14159265358979323846

                                    var d_lat = lat_next_rad - lat_now_rad
                                    var d_lon = lon_next_rad - lon_now_rad
                                    var a = Math.sin(d_lat / 2.0) * Math.sin(d_lat / 2.0) + Math.sin(d_lon / 2.0) * Math.sin(d_lon / 2.0) * Math.cos(lat_now_rad) * Math.cos(lat_next_rad)
                                    var c = 2.0 * Math.atan2(Math.sqrt(a), Math.sqrt(1.0 - a))

                                    way_point_distance = 6371000 * c
                                    var totalNumber1 = parseInt((way_point_distance  - m.param2)/ camDist1)
                                    m.param1 = camDist1 + totalNumber1 * 1.0 / 1000.0
                                }
                            }
                        }
                        largNum++
                    }
                    function recover(){
                        timerLine.stop()
                        timer3.stop()
                        flagg=3
                        missionController.visualItems.get(1).flage=false
                    }
                    MouseArea{
                        anchors.fill: parent
                        onPressed:{
                            image1.checked=true
                            ima.get()
                        }
                        onReleased:{
                            image1.checked=false
                            ima.recover()
                            largNum=1
                            //missionController.loadRecover()
                        }
                    }
                }
                //==========================================

                //==========================================
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
                    //PropertyAnimation on y{id:ani5;to: clickyy+image2.width*1.732 ; duration:1000}
                    NumberAnimation {id:ani6;target:ima1;properties:"opacity";from:0.1;to: 0.9 ; duration:500}

                    RoundButton {
                        id:             image2
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
                        interval:100
                        running:false
                        repeat:true
                        onTriggered: ima1.dododo()
                    }
                    function dododo(){
                        var coordinatee1 = editorMap.toCoordinate(Qt.point(xx,yy))
                        var sizeVisiualItems = missionController.visualItems.count
                        for(var i = 0, iPoint = 0; i < sizeVisiualItems; i++)
                        {
                            var m = missionController.visualItems.get(i);
                            if(m.sequenceNumber!==0)
                            {
                                m.coordinate.latitude = -(m.coordinate.latitude - coordinatee1.latitude)*multiple+m.coordinate.latitude
                                m.coordinate.longitude = -(m.coordinate.longitude - coordinatee1.longitude)*multiple+m.coordinate.longitude
                                coordinatee.altitude = m.coordinate.altitude
                                coordinatee.latitude = m.coordinate.latitude
                                coordinatee.longitude =  m.coordinate.longitude
                                m.coordinate = coordinatee
                            }

                            if(m.command == 206 && m.param1 >= 0.5)
                            {
                                var cameCoord2 = QtPositioning.coordinate(m.param3, m.param4)
                                var camDist2 = parseInt(m.param1)
                                cameCoord2.latitude = -(cameCoord2.latitude - coordinatee1.latitude)*multiple+cameCoord2.latitude
                                cameCoord2.longitude =  -(cameCoord2.longitude - coordinatee1.longitude)*multiple+cameCoord2.longitude
                                m.param3 = cameCoord2.latitude
                                m.param4 = cameCoord2.longitude
                                for(var j = i -1; j > 0; j-- ){
                                    var item = missionController.visualItems.get(j)
                                    if(item.specifiesCoordinate){
                                        var lat_previousitem = item.coordinate.latitude
                                        var lon_previousitem = item.coordinate.longitude
                                        var lat_nowitem_param3 = m.param3
                                        var lon_nowitem_param4 = m.param4

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
                                //get_distance_to_next_waypoint
                                if(camDist2 > 0)
                                {
                                    var lat_now = m.coordinate.latitude;
                                    var lon_now = m.coordinate.longitude;
                                    var lat_next =cameCoord2.latitude;
                                    var lon_next = cameCoord2.longitude;
                                    var way_point_distance = Math.round((m.param1 - camDist2) * 1000.0) * camDist2; // previously distance
                                    var lat_now_rad = lat_now / 180.0 * 3.14159265358979323846;
                                    var lon_now_rad = lon_now / 180.0 * 3.14159265358979323846;
                                    var lat_next_rad = lat_next / 180.0 * 3.14159265358979323846;
                                    var lon_next_rad = lon_next / 180.0 * 3.14159265358979323846;

                                    var d_lat = lat_next_rad - lat_now_rad;
                                    var d_lon = lon_next_rad - lon_now_rad;

                                    var a = Math.sin(d_lat / 2.0) * Math.sin(d_lat / 2.0) + Math.sin(d_lon / 2.0) * Math.sin(d_lon / 2.0) * Math.cos(lat_now_rad) * Math.cos(lat_next_rad);
                                    var c = 2.0 * Math.atan2(Math.sqrt(a), Math.sqrt(1.0 - a));
                                    way_point_distance = 6371000 * c;
                                    var totalNumber1 = parseInt((way_point_distance  - m.param2)/ camDist2);
                                    m.param1 = camDist2 + (totalNumber1 * 1.0 / 1000.0);
                                }
                            }
                        }
                        smalNum++
                    }
                    function recoverr(){
                        timerLine.stop()
                        timer4.stop()
                        flagg=4
                        missionController.visualItems.get(1).flage=false
                    }
                    MouseArea{
                        anchors.fill: parent
                        onPressed: {
                            image2.checked=true
                            ima1.gett()
                        }
                        onReleased: {
                            image2.checked=false
                            ima1.recoverr()
                            smalNum=1
                            //missionController.loadRecover()
                        }
                    }
                }
                //============================================

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
                            //if(flagg==5){
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

                            //camera
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
                        missionController.visualItems.get(1).flage=false
                    }
                    MouseArea{
                        anchors.fill: parent
                        onPressed: {

                            ima2.gettt()
                            image3.checked = true
                        }
                        onReleased: {
                            image3.checked=false
                            ima2.recoverrr()
                            angleNum = 0
                        }
                    }
                }

                //========================================
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
                        //timer5.start()
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
                                //coordinateeee.latitude = m.coordinate.altitude
                                m.coordinate.latitude = lat_rad*180.0/3.141592654
                                m.coordinate.longitude = lon_rad*180.0/3.141592654
                            }
                            //camera
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
                        flagg=6
                        missionController.visualItems.get(1).flage= false
                    }
                    RoundButton {
                        id:             image4
                        visible:       true
                        buttonImage:    "/qmlimages/rotateButto.svg"
                    }
                    MouseArea{
                        anchors.fill: parent
                        onPressed: {
                            ima3.gettt()
                            image4.checked = true
                        }
                        onReleased: {
                            image4.checked=false
                            ima3.recoverrr()
                            angleNum = 0
                            //missionController.loadRecover()
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
                            //sendlatitude.visible = true
                            image5.checked = false
                        }
                    }
                }
                Repeater {
                   id:rr
                     QGCLabel {
                         color:"#FFFFFF"
                         z:              QGroundControl.zOrderMapItems
                    }
                }
                QGCLabel {
                    id:bb
                    color:"#FFFFFF"
                    z:              QGroundControl.zOrderMapItems
               }
                //============================================================================
                MouseArea {
                    //-- It's a whole lot faster to just fill parent and deal with top offset below
                    //   than computing the coordinate offset.
                    id: missionEditorLayer
                    anchors.fill: parent
                    acceptedButtons:    Qt.LeftButton | Qt.RightButton
                    property bool drawedPolyLine : false
                    property alias drawingPolyLine: missionEditorLayer.hoverEnabled
                    onClicked: {
                        //==========================================
                        if(!addMissionItemsButton.checked && !measuringButton.checked){
                            clickxx=mouse.x
                            clickyy=mouse.y
                            xx=mouse.x
                            yy=mouse.y
                            mousecoord = editorMap.toCoordinate(Qt.point(clickxx, clickyy))
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
                            var maporigin = editorMap.toCoordinate(Qt.point(clickxx,clickyy))
                            for(var i = 0; i<missionController.waypointLines.count;i++){

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
                                var cos_angle = (x * x1 + y * y1)/Math.sqrt((Math.pow(x,2) + Math.pow(y,2)) * (Math.pow(x1,2) + Math.pow(y1,2))) //向量夹角
                                var angle = Math.acos(cos_angle);
                                var distance = Math.sqrt(Math.pow(x1,2) + Math.pow(y1,2)) * Math.sin(angle) //距离公式
                                var dis = Math.sqrt(Math.pow(x1,2) + Math.pow(y1,2))
                                var dis2 = Math.sqrt(Math.pow(x,2) + Math.pow(y,2))
                                if(distance < d){
                                    if(0<cos_angle  && dis < dis2){ //夹角在-90 到90度之间
                                        fff = true;
                                        break
                                    }
                                }
                            }
                            if(fff){
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
                                    //ima.y = clickyy
                                    ima1.x = clickxx
                                    //ima1.y = clickyy
                                    ima2.x = clickxx
                                    ima2.y = clickyy
                                    ima3.x = clickxx
                                    ima3.y = clickyy
                                    ima4.x = clickxx
                                    ima4.y = clickyy
                                    ani1.start()
                                    //ani2.start()
                                    ani3.start()
                                    ani4.start()
                                    // ani5.start()
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
                        //=============================================
                        //-- Don't pay attention to items beneath the toolbar.
                        var topLimit = parent.height - ScreenTools.availableHeight
                        if(mouse.y < topLimit) {
                            return
                        }
                        var coordinate = editorMap.toCoordinate(Qt.point(mouse.x, mouse.y))
                        coordinate.latitude = coordinate.latitude.toFixed(_decimalPlaces)
                        coordinate.longitude = coordinate.longitude.toFixed(_decimalPlaces)
                        coordinate.altitude = coordinate.altitude.toFixed(_decimalPlaces)

                        switch (_editingLayer) {
                        case _layerMission:
                            if (addMissionItemsButton.checked) {
                                var sequenceNumber = missionController.insertSimpleMissionItem(coordinate, missionController.visualItems.count)
                                setCurrentItem(sequenceNumber)
                            }
                            if(measuringButton.checked){
                                missionEditorLayer.drawedPolyLine = true
                                if(measureflag == true){
                                    if(mouse.button == Qt.LeftButton ){
                                        missionEditorLayer. drawingPolyLine = true
                                        bb.visible = true
                                        var polygonPath = polygonLineNextPoint.path
                                        if (polygonPath.length == 0) {
                                            // Add first coordinate
                                            polygonPath.push(coordinate)
                                        }else{
                                        	polygonPath[polygonLineNextPoint.path.length-1] = coordinate
                                        	editorMap.updateposition()

                                       }
                                        polygonPath.push(coordinate)
                                        polygonLineNextPoint.path = polygonPath
                                    }else{
                                       measureflag = false;
                                        missionEditorLayer. drawingPolyLine = false
                                        polygonPath = polygonLineNextPoint.removeCoordinate(polygonLineNextPoint.path[polygonLineNextPoint.path.length -1] )
                                        polygonLineNextPoint.path = polygonPath
                                        bb.visible = false
                                    }
                                }
                            }
                            break
                        case _layerGeoFence:
                            if (geoFenceController.breachReturnSupported) {
                                geoFenceController.breachReturnPoint = coordinate
                                geoFenceController.validateBreachReturn()
                            }
                            break
                        case _layerRallyPoints:
                            if (rallyPointController.rallyPointsSupported) {
                                rallyPointController.addPoint(coordinate)
                            }
                            break
                        }
                    }
                    onPositionChanged: {
                        if (ScreenTools.isMobile) {
                            // We don't track mouse drag on mobile
                            return
                        }
                      if(measureflag == true){
                          if (polygonLineNextPoint.path.length) {
                                 var dragCoordinate = editorMap.toCoordinate(Qt.point(mouse.x, mouse.y))
                                polygonLineNext.path = [polygonLineNextPoint.path[polygonLineNextPoint.path.length-2], dragCoordinate]
                                var origin = polygonLineNextPoint.path[polygonLineNextPoint.path.length-2]
                                 var coord = dragCoordinate
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
                                var linelen = Math.sqrt(Math.pow(x,2)+Math.pow(y,2)).toFixed(2)
                                bb.x = mouse.x+10
                                bb.y = mouse.y
                                bb.text=linelen
                                var polygonPath = polygonLineNextPoint.path
                                polygonPath[polygonLineNextPoint.path.length-1] = dragCoordinate
                                polygonLineNextPoint.path = polygonPath
                                }
                         }
                     }
                }

                // We use this item to support dragging since dragging a MapQuickItem just doesn't seem to work
                Rectangle {
                    id:             itemDragger
                    x:              mapCoordinateIndicator ? (mapCoordinateIndicator.x + mapCoordinateIndicator.anchorPoint.x - (itemDragger.width / 2)) : 100
                    y:              mapCoordinateIndicator ? (mapCoordinateIndicator.y + mapCoordinateIndicator.anchorPoint.y - (itemDragger.height / 2)) : 100
                    width:          ScreenTools.defaultFontPixelHeight * 2
                    height:         ScreenTools.defaultFontPixelHeight * 2
                    color:          "transparent"
                    visible:        false
                    z:              QGroundControl.zOrderMapItems + 1    // Above item icons

                    property var    coordinateItem
                    property var    mapCoordinateIndicator
                    property bool   preventCoordinateBindingLoop: false

                    onXChanged: liveDrag()
                    onYChanged: liveDrag()

                    function liveDrag() {
                        if (!itemDragger.preventCoordinateBindingLoop && Drag.active) {
                            var point = Qt.point(itemDragger.x + (itemDragger.width  / 2), itemDragger.y + (itemDragger.height / 2))
                            var coordinate = editorMap.toCoordinate(point)
                            coordinate.altitude = itemDragger.coordinateItem.coordinate.altitude
                            itemDragger.preventCoordinateBindingLoop = true
                            itemDragger.coordinateItem.coordinate = coordinate
                            itemDragger.preventCoordinateBindingLoop = false
                        }
                    }

                    function clearItem() {
                        itemDragger.visible = false
                        itemDragger.coordinateItem = undefined
                        itemDragger.mapCoordinateIndicator = undefined
                    }

                    Drag.active:    itemDrag.drag.active
                    Drag.hotSpot.x: width  / 2
                    Drag.hotSpot.y: height / 2

                    MouseArea {
                        id:             itemDrag
                        anchors.fill:   parent
                        drag.target:    parent
                        drag.minimumX:  0
                        drag.minimumY:  0
                        drag.maximumX:  itemDragger.parent.width - parent.width
                        drag.maximumY:  itemDragger.parent.height - parent.height
                    }
                }

                MapItemView {
                    model: missionController.cameraFeedItemsLocal
                    delegate:   missionCameraComponent
                }
                Component {
                    id: missionCameraComponent
                    MapQuickItem{
                        coordinate:     object.coordinate
                        z:              QGroundControl.zOrderMapItems
                        anchorPoint.x:  8 / 2
                        anchorPoint.y:  8 / 2
                        sourceItem:
                            Rectangle {
                            width:          8
                            height:         8
                            radius:         8 / 2
                            color:          "red"
                        }
                    }
                }

                CameraFeedItemView{
                    model: missionController.cameraFeedItems
                }
                // Add the complex mission item polygon to the map
                MapItemView {
                    model: missionController.complexVisualItems

                    delegate: MapPolygon {
                        color:      'green'
                        path:       object.polygonPath
                        opacity:    0.5
                    }
                }

                MapItemView {
                    model: missionController.polygonn

                    delegate: MapPolygon {
                        color:      'red'
                        path:       object.gisFlag
                        opacity:    0.5
                    }
                }
                // Add the complex mission item grid to the map
                MapItemView {
                    model: missionController.complexVisualItems

                    delegate: MapPolyline {
                        line.color: "white"
                        line.width: 2
                        path:       object.gridPoints
                    }
                }

                MapItemView {
                    model : missionController.lineString

                    delegate : MapPolyline{
                        line.color: "yellow"
                        line.width: 1.5
                        path :      object.linePoints
                    }
                }

                // Add the complex mission item exit coordinates
                MapItemView {
                    model: missionController.complexVisualItems
                    delegate:   exitCoordinateComponent
                }

                Component {
                    id: exitCoordinateComponent

                    MissionItemIndicatorAdd {
                        coordinate:     object.exitCoordinate
                        z:              QGroundControl.zOrderMapItems
                        missionItem:    object
                        sequenceNumber: object.lastSequenceNumber
                        visible:        object.specifiesCoordinate
                    }
                }

                // Add the simple mission items to the map
                MapItemView {
                    model:      missionController.visualItems
                    delegate:   missionItemComponent
                }

                Component {
                    id: missionItemComponent

                    MissionItemIndicator {
                        id:             itemIndicator
                        coordinate:     object.coordinate
                        visible:        object.specifiesCoordinate
                        z:              QGroundControl.zOrderMapItems
                        missionItem:    object
                        sequenceNumber: object.sequenceNumber

                        //-- If you don't want to allow selecting items beneath the
                        //   toolbar, the code below has to check and see if mouse.y
                        //   is greater than (map.height - ScreenTools.availableHeight)
                        onClicked: setCurrentItem(object.sequenceNumber)

                        function updateItemIndicator() {
                            if (object.isCurrentItem && itemIndicator.visible && object.specifiesCoordinate && object.isSimpleItem) {
                                // Setup our drag item
                                itemDragger.visible = true
                                itemDragger.coordinateItem = Qt.binding(function() { return object })
                                itemDragger.mapCoordinateIndicator = Qt.binding(function() { return itemIndicator })
                            }
                        }

                        Connections {
                            target: object

                            onIsCurrentItemChanged:         updateItemIndicator()
                            onSpecifiesCoordinateChanged:   updateItemIndicator()
                        }

                        // These are the non-coordinate child mission items attached to this item
                        Row {
                            anchors.top:    parent.top
                            anchors.left:   parent.right

                            Repeater {
                                model: object.childItems

                                delegate: MissionItemIndexLabel {
                                    label:      object.abbreviation
                                    checked:    object.isCurrentItem
                                    z:          2

                                    onClicked: setCurrentItem(object.sequenceNumber)
                                }
                            }
                        }
                    }
                }

                // Add lines between waypoints
                MissionLineView {
                    model:      _editingLayer == _layerMission ? missionController.waypointLines : undefined
                }

                // Add the vehicles to the map
                MapItemView {
                    model: QGroundControl.multiVehicleManager.vehicles 
                    delegate:
                        VehicleMapItem {
                        vehicle:        object
                        vehicletype: object.vehicletype
                        coordinate:     object.coordinate
                        isSatellite:    editorMap.isSatelliteMap
                        size:           ScreenTools.defaultFontPixelHeight * 4
                        z:              QGroundControl.zOrderMapItems - 1
                    }
                }

                // Plan Element selector (Mission/Fence/Rally)
                Row {
                    id:                 planElementSelectorRow
                    anchors.topMargin:  parent.height - ScreenTools.availableHeight + _margin
                    anchors.top:        parent.top
                    anchors.leftMargin: parent.width - _rightPanelWidth
                    anchors.left:       parent.left
                    spacing:            _horizontalMargin

                    readonly property real _buttonRadius: ScreenTools.defaultFontPixelHeight * 0.75

                    ExclusiveGroup {
                        id: planElementSelectorGroup
                        onCurrentChanged: {
                            switch (current) {
                            case planElementMission:
                                _editingLayer = _layerMission
                                _syncDropDownController = missionController
                                break
                            case planElementGeoFence:
                                _editingLayer = _layerGeoFence
                                _syncDropDownController = geoFenceController
                                break
                            case planElementRallyPoints:
                                _editingLayer = _layerRallyPoints
                                _syncDropDownController = rallyPointController
                                break
                            }
                            _syncDropDownController.fitViewportToItems()
                        }
                    }

                    QGCRadioButton {
                        id:             planElementMission
                        exclusiveGroup: planElementSelectorGroup
                        text:           qsTr("Mission")
                        checked:        true
                    }

                    Item { height: 1; width: 1 }

                    QGCRadioButton {
                        id:             planElementGeoFence
                        exclusiveGroup: planElementSelectorGroup
                        text:           qsTr("Fence")
                    }

                    Item { height: 1; width: 1 }

                    QGCRadioButton {
                        id:             planElementRallyPoints
                        exclusiveGroup: planElementSelectorGroup
                        text:           qsTr("Rally")
                    }
                } // Row - Plan Element Selector

                // Mission Item Editor
                Item {
                    id:                 missionItemEditor
                    anchors.topMargin:  _margin
                    anchors.top:        planElementSelectorRow.bottom
                    anchors.bottom:     parent.bottom
                    anchors.right:      parent.right
                    width:              _rightPanelWidth
                    opacity:            _rightPanelOpacity
                    z:                  QGroundControl.zOrderTopMost
                    visible:            _editingLayer == _layerMission

                    MouseArea {
                        // This MouseArea prevents the Map below it from getting Mouse events. Without this
                        // things like mousewheel will scroll the Flickable and then scroll the map as well.
                        anchors.fill:       missionItemEditorListView
                        onWheel:            wheel.accepted = true
                    }

                    ListView {
                        id:             missionItemEditorListView
                        anchors.left:   parent.left
                        anchors.right:  parent.right
                        anchors.top:    parent.top
                        height:         parent.height
                        spacing:        _margin / 2
                        orientation:    ListView.Vertical
                        model:          missionController.visualItems
                        cacheBuffer:    height * 2 > 0 ? height * 2 : height * -2
                        clip:           true
                        currentIndex:   _currentMissionIndex
                        highlightMoveDuration: 250

                        delegate: MissionItemEditor {
                            missionItem:    object
                            width:          parent.width
                            readOnly:       false
                            vehicletype:   _vehicType

                            onClicked:  setCurrentItem(object.sequenceNumber)

                            onRemove: {
                                itemDragger.clearItem()
                                missionController.removeMissionItem(index)
                                editorMap.polygonDraw.cancelPolygonEdit()
                            }

                            onInsert: {
                                var sequenceNumber = missionController.insertSimpleMissionItem(editorMap.center, index)
                                setCurrentItem(sequenceNumber)
                            }

                            onMoveHomeToMapCenter: _visualItems.get(0).coordinate = editorMap.center
                        }
                    } // ListView
                } // Item - Mission Item editor

                // GeoFence Editor
                Loader {
                    anchors.topMargin:  _margin
                    anchors.top:        planElementSelectorRow.bottom
                    anchors.right:      parent.right
                    opacity:            _rightPanelOpacity
                    z:                  QGroundControl.zOrderTopMost
                    source:             _editingLayer == _layerGeoFence ? "qrc:/qml/GeoFenceEditor.qml" : ""

                    property real availableWidth:   _rightPanelWidth
                    property real availableHeight:  ScreenTools.availableHeight
                }

                // GeoFence polygon
                MapPolygon {
                    border.color:   "#80FF0000"
                    border.width:   3
                    path:           geoFenceController.polygon.path
                    z:              QGroundControl.zOrderMapItems
                    visible:        geoFenceController.polygonSupported
                }

                // GeoFence circle
                MapCircle {
                    border.color:   "#80FF0000"
                    border.width:   3
                    center:         missionController.plannedHomePosition
                    radius:         geoFenceController.circleSupported ? geoFenceController.circleRadius : 0
                    z:              QGroundControl.zOrderMapItems
                    visible:        geoFenceController.circleSupported
                }

                MapPolyline{
                    id:         polygonLineNextPoint
                    line.color: "green"
                    line.width: 3
                    visible:    missionEditorLayer.drawedPolyLine
                }
                MapPolyline{
                    id:         polygonLineNext
                    line.color: "green"
                    line.width: 3
                    visible:    missionEditorLayer.drawingPolyLine
                }
                // GeoFence breach return point
                MapQuickItem {
                    anchorPoint:    Qt.point(sourceItem.width / 2, sourceItem.height / 2)
                    coordinate:     geoFenceController.breachReturnPoint
                    visible:        geoFenceController.breachReturnSupported
                    sourceItem:     MissionItemIndexLabel { label: "F" }
                    z:              QGroundControl.zOrderMapItems
                }

                // Rally Point Editor

                RallyPointEditorHeader {
                    id:                 rallyPointHeader
                    anchors.topMargin:  _margin
                    anchors.top:        planElementSelectorRow.bottom
                    anchors.right:      parent.right
                    width:              _rightPanelWidth
                    opacity:            _rightPanelOpacity
                    z:                  QGroundControl.zOrderTopMost
                    visible:            _editingLayer == _layerRallyPoints
                    controller:         rallyPointController
                }

                RallyPointItemEditor {
                    id:                 rallyPointEditor
                    anchors.topMargin:  _margin
                    anchors.top:        rallyPointHeader.bottom
                    anchors.right:      parent.right
                    width:              _rightPanelWidth
                    opacity:            _rightPanelOpacity
                    z:                  QGroundControl.zOrderTopMost
                    visible:            _editingLayer == _layerRallyPoints && rallyPointController.points.count
                    rallyPoint:         rallyPointController.currentRallyPoint
                    controller:         rallyPointController
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
                            checked:    _editingLayer == _layerRallyPoints ? object == rallyPointController.currentRallyPoint : false

                            onClicked: rallyPointController.currentRallyPoint = object

                            onCheckedChanged: {
                                if (checked) {
                                    // Setup our drag item
                                    itemDragger.visible = true
                                    itemDragger.coordinateItem = Qt.binding(function() { return object })
                                    itemDragger.mapCoordinateIndicator = Qt.binding(function() { return itemIndicator })
                                }
                            }
                        }
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

                QGCLabel {
                    id:         planLabel
                    text:       qsTr("Plan")
                    color:      mapPal.text
                    visible:    !ScreenTools.isShortScreen
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
                    anchors.top:        ScreenTools.isShortScreen ? parent.top : planLabel.bottom
                    spacing:            ScreenTools.defaultFontPixelHeight
                    z:                  QGroundControl.zOrderWidgets

                    RoundButton {
                        id:             addMissionItemsButton
                        buttonImage:    "/qmlimages/MapAddMission.svg"
                        lightBorders:   _lightWidgetBorders
                        visible:        _editingLayer == _layerMission
                        onClicked:{
                            if(measuringButton.checked){
                                measuringButton.checked = false
                                missionEditorLayer.drawedPolyLine = false
                                bb.visible =false
                                rr.model = 0
                                var polygonPath = polygonLineNextPoint.path
                                polygonPath.pop()
                                polygonLineNextPoint.path = [ ]
                            }
                            if(editorMap && editorMap.polygonDraw){
                                editorMap.polygonDraw.finishAdjustPolygon()
                            }
                        }
                    }
                    RoundButton{
                        id:     measuringButton
                        buttonImage:    "qrc:/qmlimages/ZoomMinu1.svg"
                        lightBorders:   _lightWidgetBorders
                        visible: _editingLayer == _layerMission && !ScreenTools.isMobile
                        onClicked:{
                            if(measuringButton.checked ==  false){
                                measureflag = true
                                missionEditorLayer.drawingPolyLine = false
                                polygonLineNext.path = [ ]
                                missionEditorLayer.drawedPolyLine = false
                                bb.visible = false
                                shuzu.pop();
                                rr.model = 0;
                               var polygonPath = polygonLineNextPoint.path
                               polygonPath.pop()
                               polygonLineNextPoint.path = [ ]
                            }
                            if(addMissionItemsButton.checked){
                                addMissionItemsButton.checked = false
                            }

                            if(editorMap && editorMap.polygonDraw){
                                editorMap.polygonDraw.finishAdjustPolygon()
                            }
                        }
                    }
                    RoundButton {
                        id:             addShapeButton
                        buttonImage:    "/qmlimages/MapDrawShape.svg"
                        lightBorders:   _lightWidgetBorders
                        visible:        _editingLayer == _layerMission

                        onClicked: {
                            if(editorMap && editorMap.polygonDraw){
                                editorMap.polygonDraw.finishAdjustPolygon()
                            }
                            var coordinate = editorMap.center
                            coordinate.latitude = coordinate.latitude.toFixed(_decimalPlaces)
                            coordinate.longitude = coordinate.longitude.toFixed(_decimalPlaces)
                            coordinate.altitude = coordinate.altitude.toFixed(_decimalPlaces)
                            var sequenceNumber = missionController.insertComplexMissionItem(coordinate, missionController.visualItems.count)
                            setCurrentItem(sequenceNumber)
                            checked = false
                            addMissionItemsButton.checked = false
                        }
                    }

                    DropButton {
                        id:                 syncButton
                        dropDirection:      dropRight
                        buttonImage:        _syncDropDownController.dirty ? "/qmlimages/MapSyncChanged.svg" : "/qmlimages/MapSync.svg"
                        viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
                        exclusiveGroup:     _dropButtonsExclusiveGroup
                        dropDownComponent:  syncDropDownComponent
                        enabled:            !_syncDropDownController.syncInProgress
                        rotateImage:        _syncDropDownController.syncInProgress
                        lightBorders:       _lightWidgetBorders
                    }

                    DropButton {
                        id:                 centerMapButton
                        dropDirection:      dropRight
                        buttonImage:        "/qmlimages/MapCenter.svg"
                        viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
                        exclusiveGroup:     _dropButtonsExclusiveGroup
                        lightBorders:       _lightWidgetBorders

                        dropDownComponent: Component {
                            Column {
                                spacing: ScreenTools.defaultFontPixelWidth * 0.5
                                QGCLabel { text: qsTr("Center map:") }
                                Row {
                                    spacing: ScreenTools.defaultFontPixelWidth
                                    QGCButton {
                                        text: qsTr("Home")
                                        width:  ScreenTools.defaultFontPixelWidth * 10
                                        onClicked: {
                                            if(editorMap && editorMap.polygonDraw){
                                                editorMap.polygonDraw.finishAdjustPolygon()
                                            }
                                            centerMapButton.hideDropDown()
                                            editorMap.center = missionController.visualItems.get(0).coordinate
                                        }
                                    }
                                    QGCButton {
                                        text: qsTr("Mission")
                                        width:  ScreenTools.defaultFontPixelWidth * 10
                                        onClicked: {
                                            if(editorMap && editorMap.polygonDraw){
                                                editorMap.polygonDraw.finishAdjustPolygon()
                                            }
                                            centerMapButton.hideDropDown()
                                            fitMapViewportToMissionItems()
                                        }
                                    }
                                    QGCButton {
                                        text: qsTr("All items")
                                        width:  ScreenTools.defaultFontPixelWidth * 10
                                        onClicked: {
                                            centerMapButton.hideDropDown()
                                            fitMapViewportToAllItems()
                                        }
                                    }
                                    QGCButton {
                                        text:       qsTr("Vehicle")
                                        width:      ScreenTools.defaultFontPixelWidth * 10
                                        enabled:    activeVehicle && activeVehicle.latitude != 0 && activeVehicle.longitude != 0
                                        property var activeVehicle: _activeVehicle
                                        onClicked: {
                                            if(editorMap && editorMap.polygonDraw){
                                                editorMap.polygonDraw.finishAdjustPolygon()
                                            }
                                            centerMapButton.hideDropDown()
                                            editorMap.center = activeVehicle.coordinate
                                        }
                                    }
                                }
                                Row{
                                    spacing: ScreenTools.defaultFontPixelWidth
                                    QGCButton{
                                        text: qsTr("TakeOff to Location")
                                        width: ScreenTools.defaultFontPixelWidth * 21
                                        onClicked:{
                                            if(editorMap &&editorMap.polygonDraw){
                                                editorMap.polygonDraw.finishAdjustPolygon()
                                            }
                                            centerMapButton.hideDropDown();
                                            for(var i = 0; i < missionController.visualItems.count; i++){
                                                var item = missionController.visualItems.get(i);
                                                if(i == missionController.visualItems.count - 1){

                                                }
                                                if(item.command == 22 || item.command == 84){
                                                    if(_activeVehicle && _activeVehicle.latitude != 0 && _activeVehicle.longitude != 0){
                                                        var temp = item.coordinate.altitude
                                                        item.coordinate = _activeVehicle.coordinate;
                                                        item.coordiante.altitude = temp
                                                    }else{
                                                         item.coordinate = editorMap.center;
                                                    }
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    QGCButton{
                                        text: qsTr("Mission to Location")
                                        width: ScreenTools.defaultFontPixelWidth * 21
                                        onClicked:{
                                            if(editorMap && editorMap.polygonDraw){
                                                editorMap.polygonDraw.finishAdjustPolygon();
                                            }
                                            centerMapButton.hideDropDown();
                                            var firstspecifiesCoordinate ,latitude_offset, longitude_offset
                                            for(var i = 1 ; i < missionController.visualItems.count; i++){
                                                var item = missionController.visualItems.get(i);
                                                if(item.specifiesCoordinate){
                                                    firstspecifiesCoordinate = item.coordinate;
                                                    if(_activeVehicle && _activeVehicle.latitude != 0&& _activeVehicle.longitude != 0){
                                                        latitude_offset = firstspecifiesCoordinate.latitude - _activeVehicle.latitude
                                                        longitude_offset = firstspecifiesCoordinate.latitude - _activeVehicle.longitude
                                                    }else{
                                                        latitude_offset = firstspecifiesCoordinate.latitude - editorMap.center.latitude;
                                                        longitude_offset = firstspecifiesCoordinate.longitude - editorMap.center.longitude;
                                                    }
                                                    break;
                                                }
                                            }
                                            for(var i = 1 ; i < missionController.visualItems.count; i++){
                                                item = missionController.visualItems.get(i);
                                                item.coordinate.latitude = item.coordinate.latitude - latitude_offset;
                                                item.coordinate.longitude = item.coordinate.longitude - longitude_offset;

                                                if(item.command == 206 && item.param1 > 0.5 && item.param3){
                                                    item.param3 = item.param3 - latitude_offset;
                                                    item.param4 = item.param4 - longitude_offset;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    DropButton {
                        id:                 mapTypeButton
                        dropDirection:      dropRight
                        buttonImage:        "/qmlimages/MapType.svg"
                        viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
                        exclusiveGroup:     _dropButtonsExclusiveGroup
                        lightBorders:       _lightWidgetBorders

                        dropDownComponent: Component {
                            Column {
                                spacing: _margin
                                QGCLabel { text: qsTr("Map type:") }
                                Row {
                                    spacing: ScreenTools.defaultFontPixelWidth
                                    Repeater {
                                        model: QGroundControl.flightMapSettings.mapTypes

                                        QGCButton {
                                            checkable:      true
                                            checked:        QGroundControl.flightMapSettings.mapType === text
                                            text:           modelData
                                            exclusiveGroup: _mapTypeButtonsExclusiveGroup
                                            onClicked: {
                                                if(editorMap && editorMap.polygonDraw){
                                                    editorMap.polygonDraw.finishAdjustPolygon()
                                                }
                                                QGroundControl.flightMapSettings.mapType = text
                                                checked = true
                                                mapTypeButton.hideDropDown()
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }

                    //-- Zoom Map In
                    RoundButton {
                        id:             mapZoomPlus
                        visible:        !ScreenTools.isTinyScreen && !ScreenTools.isShortScreen
                        buttonImage:    "/qmlimages/ZoomPlus.svg"
                        lightBorders:   _lightWidgetBorders

                        onClicked: {
                            if(editorMap && editorMap.polygonDraw){
                                editorMap.polygonDraw.finishAdjustPolygon()
                            }
                            if(editorMap)
                                editorMap.zoomLevel += 0.5
                            checked = false
                        }
                    }

                    //-- Zoom Map Out
                    RoundButton {
                        id:             mapZoomMinus
                        visible:        !ScreenTools.isTinyScreen && !ScreenTools.isShortScreen
                        buttonImage:    "/qmlimages/ZoomMinus.svg"
                        lightBorders:   _lightWidgetBorders
                        onClicked: {
                            if(editorMap && editorMap.polygonDraw){
                                editorMap.polygonDraw.finishAdjustPolygon()
                            }
                            if(editorMap)
                                editorMap.zoomLevel -= 0.5
                            checked = false
                        }
                    }
                }

                MapScale {
                    id:mapss
                    anchors.margins:    ScreenTools.defaultFontPixelHeight * (0.66)
                    anchors.bottom:     waypointValuesDisplay.visible ? waypointValuesDisplay.top : parent.bottom
                    anchors.left:       parent.left
                    anchors.leftMargin: ScreenTools.isMobile ? planElementSelector.width : 0
                    z:                  QGroundControl.zOrderWidgets
                    mapControl:         editorMap
                    visible:            !ScreenTools.isTinyScreen
                }

                MissionItemStatus {
                    id:                     waypointValuesDisplay
                    anchors.margins:        ScreenTools.defaultFontPixelWidth
                    anchors.left:           parent.left
                    anchors.bottom:         parent.bottom
                    z:                      QGroundControl.zOrderTopMost
                    currentMissionItem:     _currentMissionItem
                    missionItems:           missionController.visualItems
                    expandedWidth:          missionItemEditor.x - (ScreenTools.defaultFontPixelWidth * 2)
                    missionDistance:        missionController.missionDistance
                    missionMaxTelemetry:    missionController.missionMaxTelemetry
                    cruiseDistance:         missionController.cruiseDistance
                    hoverDistance:          missionController.hoverDistance
                    visible:                _editingLayer == _layerMission && !ScreenTools.isShortScreen
                }
            } // FlightMap
        } // Item - split view container
    } // QGCViewPanel

    Component {
        id: syncLoadFromVehicleOverwrite
        QGCViewMessage {
            id:         syncLoadFromVehicleCheck
            message:   qsTr("You have unsaved/unsent changes. Loading from the Vehicle will lose these changes. Are you sure you want to load from the Vehicle?")
            function accept() {
                hideDialog()
                _syncDropDownController.loadFromVehicle()
            }
        }
    }

    Component {
        id: syncLoadFromFileOverwrite
        QGCViewMessage {
            id:         syncLoadFromVehicleCheck
            message:   qsTr("You have unsaved/unsent changes. Loading a from a file will lose these changes. Are you sure you want to load from a file?")
            function accept() {
                hideDialog()
                _syncDropDownController.loadFromSelectedFile()
            }
        }
    }

    Component {
        id: removeAllPromptDialog
        QGCViewMessage {
            message: qsTr("Are you sure you want to remove all items?")
            function accept() {
                itemDragger.clearItem()
                _syncDropDownController.removeAll()
                hideDialog()
            }
        }
    }

    Component {
        id: syncDropDownComponent

        Column {
            id:         columnHolder
            spacing:    _margin

            property string _overwriteText: (_editingLayer == _layerMission) ? qsTr("Mission overwrite") : ((_editingLayer == _layerGeoFence) ? qsTr("GeoFence overwrite") : qsTr("Rally Points overwrite"))

            QGCLabel {
                width:      sendSaveGrid.width
                wrapMode:   Text.WordWrap
                text:       _syncDropDownController.dirty ?
                                qsTr("You have unsaved changes. You should send to your vehicle, or save to a file:") :
                                qsTr("Sync:")
            }

            GridLayout {
                id:                 sendSaveGrid
                columns:            2
                anchors.margins:    _margin
                rowSpacing:         _margin
                columnSpacing:      ScreenTools.defaultFontPixelWidth

                QGCButton {
                    text:               qsTr("Send To Vehicle")
                    Layout.fillWidth:   true
                    enabled:            _activeVehicle && !_syncDropDownController.syncInProgress
                    onClicked: {
                        if(editorMap && editorMap.polygonDraw){
                            editorMap.polygonDraw.finishAdjustPolygon()
                        }
                        syncButton.hideDropDown()
                        _syncDropDownController.sendToVehicle(editorMap.center)
                    }
                }

                QGCButton {
                    text:               qsTr("Load From Vehicle")
                    Layout.fillWidth:   true
                    enabled:            _activeVehicle && !_syncDropDownController.syncInProgress
                    onClicked: {
                        if(editorMap && editorMap.polygonDraw){
                            editorMap.polygonDraw.finishAdjustPolygon()
                        }
                        syncButton.hideDropDown()
                        if (_syncDropDownController.dirty) {
                            qgcView.showDialog(syncLoadFromVehicleOverwrite, columnHolder._overwriteText, qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
                        } else {
                            _syncDropDownController.loadFromVehicle()
                        }
                    }
                }

                QGCButton {
                    text:               qsTr("Save To File...")
                    Layout.fillWidth:   true
                    enabled:            !_syncDropDownController.syncInProgress
                    onClicked: {
                        if(editorMap && editorMap.polygonDraw){
                            editorMap.polygonDraw.finishAdjustPolygon()
                        }
                        syncButton.hideDropDown()
                        _syncDropDownController.saveToSelectedFile()
                    }
                }

                QGCButton {
                    text:               qsTr("Load From File...")
                    Layout.fillWidth:   true
                    enabled:            !_syncDropDownController.syncInProgress
                    onClicked: {
                        if(editorMap && editorMap.polygonDraw){
                            editorMap.polygonDraw.finishAdjustPolygon()
                        }
                        syncButton.hideDropDown()
                        if (_syncDropDownController.dirty) {
                            qgcView.showDialog(syncLoadFromFileOverwrite, columnHolder._overwriteText, qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.Cancel)
                        } else {
                            _syncDropDownController.loadFromSelectedFile()
                        }
                    }
                }

                QGCButton {
                    text:               qsTr("Remove All")
                    Layout.fillWidth:   true
                    onClicked:  {
                        if(editorMap && editorMap.polygonDraw){
                            editorMap.polygonDraw.finishAdjustPolygon()
                        }
                        syncButton.hideDropDown()
                        //_syncDropDownController.removeAll()
                        qgcView.showDialog(removeAllPromptDialog, qsTr("Remove all"), qgcView.showDialogDefaultWidth, StandardButton.Yes | StandardButton.No)
                    }
                }
            }
        }
    }
} // QGCVIew
