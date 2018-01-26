/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Map Background
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick          2.4
import QtQuick.Controls 1.3
import QtLocation       5.3
import QtPositioning    5.3

import QGroundControl                       1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.Controls              1.0
import QGroundControl.FlightMap             1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Vehicle               1.0
import QGroundControl.Mavlink               1.0

import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controllers   1.0
import QGroundControl.FactControls  1.0

Map {
    id: _map

    property string mapName:            'defaultMap'
    property bool   isSatelliteMap:     activeMapType.name.indexOf("Satellite") > -1 || activeMapType.name.indexOf("Hybrid") > -1
    property var shuzu:                 new Array
    readonly property real  maxZoomLevel: 20
    property variant        scaleLengths: [5, 10, 25, 50, 100, 150, 250, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000, 2000000]
    property bool           doingAdjustPolygon: false
    property var pathh : []
    property var inde:0

    property real _margin: ScreenTools.defaultFontPixelHeight
    readonly property real      _rightPanelWidth:       Math.min(parent.width / 3, ScreenTools.defaultFontPixelWidth * 30)
    readonly property real      _rightPanelOpacity:     0.8
//========================================================================
    onZoomLevelChanged:{

    }
    function setCurrentItem(sequenceNumber) {
        _currentMissionItem = undefined
        for (var i=0; i<pathh.count; i++) {
            var visualItem = pathh.get(i)
            if (visualItem.sequenceNumber == sequenceNumber) {
                _currentMissionItem = visualItem
                _currentMissionItem.isCurrentItem = true
                currentitemflag = true
            } else {
                visualItem.isCurrentItem = false
            }
        }
    }
    function colseAdjustPolygon() {
        rr.model=0
        itemDragger.visible = false
        gg.model = 0
        timer1.stop()
        doingAdjustPolygon = false
        polygonDrawer._cancelAdjustPolygon()
        polygonDrawer._callbackObject.polygonAdjustFinished()
    }
    Rectangle {
        id:             itemDragger
        x:              missionItemIndicator ? (missionItemIndicator.x + missionItemIndicator.anchorPoint.x - (itemDragger.width / 2)) : 100
        y:              missionItemIndicator ? (missionItemIndicator.y + missionItemIndicator.anchorPoint.y - (itemDragger.height / 2)) : 100
        width:          ScreenTools.defaultFontPixelHeight*2
        height:         ScreenTools.defaultFontPixelHeight*2
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
                var coordinate =  _map.toCoordinate(point)
                coordinate.altitude = itemDragger.missionItem.coordinate.altitude
                itemDragger.preventCoordinateBindingLoop = true
                itemDragger.missionItem.coordinate = coordinate
                polygonDrawer._callbackObject.polygonAdjustVertex(inde, itemDragger.missionItem.coordinate)
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
        id:gg
        delegate:   missionCameraComponent
    }
    Component {
        id: missionCameraComponent
        DragAre {
            id:             itemIndicator
            coordinate:     object.coordinate
            visible:       true
            z:              QGroundControl.zOrderMapItems
            missionItem:    object
            sequenceNumber: object.sequenceNumber

            //-- If you don't want to allow selecting items beneath the
            //toolbar, the code below has to check and see if mouse.y
            //is greater than (map.height - ScreenTools.availableHeight)
            onClicked:  itemIndicator.dis()
            function dis(){
                inde = object.sequenceNumber
                setCurrentItem(object.sequenceNumber)
                itemDragger.visible=true
                itemDragger.missionItem = Qt.binding(function() { return object })
                itemDragger.missionItemIndicator = Qt.binding(function() { return itemIndicator })
            }

            function updateItemIndicator() {
                if (object.isCurrentItem && itemIndicator.visible && object.specifiesCoordinate && object.isSimpleItem) {
                }
            }

            Connections {
                target: object
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
    //=============================================================
    function formatDistance(meters)
    {
        var dist = Math.round(meters)
        if (dist > 1000 ){
            if (dist > 100000){
                dist = Math.round(dist / 1000)
            }
            else{
                dist = Math.round(dist / 100)
                dist = dist / 10
            }
            dist = dist + " km"
        }
        else{
            dist = dist + " m"
        }
        return dist
    }

    function calculateScale() {
        var coord1, coord2, dist, text, f
        f = 0
        coord1 = _map.toCoordinate(Qt.point(0, scale.y))
        coord2 = _map.toCoordinate(Qt.point(0 + scaleImage.sourceSize.width, scale.y))
        dist = Math.round(coord1.distanceTo(coord2))
        if (dist === 0) {
            // not visible
        } else {
            for (var i = 0; i < scaleLengths.length - 1; i++) {
                if (dist < (scaleLengths[i] + scaleLengths[i+1]) / 2 ) {
                    f = scaleLengths[i] / dist
                    dist = scaleLengths[i]
                    break;
                }
            }
            if (f === 0) {
                f = dist / scaleLengths[i]
                dist = scaleLengths[i]
            }
        }
        text = formatDistance(dist)
        scaleImage.width = (scaleImage.sourceSize.width * f) - 2 * scaleImageLeft.sourceSize.width
        scaleText.text = text
    }

    zoomLevel:                  18
    center:                     QGroundControl.lastKnownHomePosition
    gesture.flickDeceleration:  3000

    plugin: Plugin { name: "QGroundControl" }

    ExclusiveGroup { id: mapTypeGroup }

    property bool _initialMapPositionSet: false

    Connections {
        target: mainWindow
        onGcsPositionChanged: {
            if (!_initialMapPositionSet) {
                _initialMapPositionSet = true
                center = mainWindow.gcsPosition
            }
        }
    }

    function updateActiveMapType() {
        var fullMapName = QGroundControl.flightMapSettings.mapProvider + " " + QGroundControl.flightMapSettings.mapType
        console.log(fullMapName + "LLLLLLLLLLLLLLLL")
        for (var i = 0; i < _map.supportedMapTypes.length; i++) {
            if (fullMapName === _map.supportedMapTypes[i].name) {
                _map.activeMapType = _map.supportedMapTypes[i]
                return
            }
        }
    }

    Component.onCompleted: updateActiveMapType()

    Connections {
        target:             QGroundControl.flightMapSettings
        onMapTypeChanged:   updateActiveMapType()
    }

    Connections {
        target:             QGroundControl.flightMapSettings
        onMapProviderChanged:   {console.log("***************");updateActiveMapType()}
    }

    /// Ground Station location
    MapQuickItem {
        anchorPoint.x:  sourceItem.width  / 2
        anchorPoint.y:  sourceItem.height / 2
        visible:        mainWindow.gcsPosition.isValid
        coordinate:     mainWindow.gcsPosition
        sourceItem:     MissionItemIndexLabel {
        label: "Q"
        }
    }
//===================================================================
    function dis(){
        if(polygonDrawer._vertexDragList.length>0){
            polygonDrawer.display()
        }
    }

    Timer{
        id:timer1
        interval:200
        running:false
        repeat:true
        onTriggered: dis()
    }

    // Not sure why this is needed, but trying to reference polygonDrawer directly from other code doesn't work
    property alias polygonDraw: polygonDrawer

    QGCLabel {
        id:                     polygonHelp
        anchors.topMargin:      parent.height - ScreenTools.availableHeight
        anchors.top:            parent.top
        anchors.left:           parent.left
        anchors.right:          parent.right
        horizontalAlignment:    Text.AlignHCenter
        text:                   qsTr("Click to add point %1").arg(ScreenTools.isMobile || !polygonDrawer.polygonReady ? "" : qsTr("- Right Click to end polygon"))
        visible:                polygonDrawer.drawingPolygon

        Connections {
            target: polygonDrawer

            onDrawingPolygonChanged: {
                if (polygonDrawer.drawingPolygon) {
                    polygonHelp.text = qsTr("Click to add point")
                }
                polygonHelp.visible = polygonDrawer.drawingPolygon
            }

            onPolygonReadyChanged: {
                if (polygonDrawer.polygonReady && !ScreenTools.isMobile) {
                    polygonHelp.text = qsTr("Click to add point - Right Click to end polygon")
                }
            }

            onAdjustingPolygonChanged: {
                if (polygonDrawer.adjustingPolygon) {
                    polygonHelp.text = qsTr("Adjust polygon by dragging corners")
                }
                polygonHelp.visible = polygonDrawer.adjustingPolygon
            }
        }
    }

    MouseArea {
        id:                 polygonDrawer
        anchors.fill:       parent
        acceptedButtons:    Qt.LeftButton | Qt.RightButton
        visible:            drawingPolygon
        z:                  1000 // Hack to fix MouseArea layering for now

        property alias  drawingPolygon:     polygonDrawer.hoverEnabled
        property bool   adjustingPolygon:   false
        property bool   polygonReady:       polygonDrawerPolygon.path.length > 3 //polygonDrawerPolygonSet.path.length > 2 ///< true: enough points have been captured to create a closed polygon
        property bool   justClicked: false

        property var _callbackObject

        property var _vertexDragList: []

        /// Begin capturing a new polygon
        ///     polygonCaptureStarted will be signalled
        function startCapturePolygon(callback) {
            polygonDrawer._callbackObject = callback
            polygonDrawer.drawingPolygon = true
            polygonDrawer._clearPolygon()
            polygonDrawer._callbackObject.polygonCaptureStarted()
        }

        /// Finish capturing the polygon
        ///     polygonCaptureFinished will be signalled
        /// @return true: polygon completed, false: not enough points to complete polygon
        function finishCapturePolygon() {
            if (!polygonDrawer.polygonReady) {
                return false
            }

            var polygonPath =polygonDrawerPolygon.path// polygonDrawerPolygonSet.path
            polygonPath.pop()
            _cancelCapturePolygon()
            polygonDrawer._callbackObject.polygonCaptureFinished(polygonPath)
            return true
        }

        function display(){
            shuzu.length=0
            for(var j=0;j<_vertexDragList.length;j++){
                if(j<=_vertexDragList.length-2&&_vertexDragList.length>=2){
                    var origin = _vertexDragList[j].coordinate
                    var coord = _vertexDragList[j+1].coordinate
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
            for(var m=0;m<rr.model;m++){
                rr.itemAt(m).x = _map.fromCoordinate(_vertexDragList[m+1].coordinate).x+15
                rr.itemAt(m).y = _map.fromCoordinate(_vertexDragList[m+1].coordinate).y
                rr.itemAt(m).color = "#FFFFFF"
                rr.itemAt(m).text=shuzu[m]
            }
        }
        function startAdjustPolygon(callback, vertexCoordinates) {
            doingAdjustPolygon = true
            polygonDraw._callbackObject = callback
            polygonDrawer.adjustingPolygon = true
            pathh  = polygonDrawer._callbackObject.returnList()
            for(var j = 0;j<pathh.count;j++){
                var mapitem = pathh.get(j)
                polygonDrawer._vertexDragList.push(mapitem)
            }

            gg.model = pathh
            timer1.start()
        }

        function finishAdjustPolygon() {
            rr.model=0
            timer1.stop()
            if(doingAdjustPolygon){
                doingAdjustPolygon = false
                _cancelAdjustPolygon()
                polygonDrawer._callbackObject.polygonAdjustFinished()
            }
        }
        /// Cancels an in progress draw or adjust
        function cancelPolygonEdit() {
            _cancelAdjustPolygon()
            _cancelCapturePolygon()
        }

        function _cancelAdjustPolygon() {
            polygonDrawer.adjustingPolygon = false
            timer1.stop()
            polygonDrawer._vertexDragList = []
            pathh = []
            gg.model=0

        }

        function _cancelCapturePolygon() {
            polygonDrawer._clearPolygon()
            polygonDrawer.drawingPolygon = false
            timer1.stop()
            polygonDrawer._vertexDragList = []
            pathh = []
            gg.model=0
        }

        function _clearPolygon() {
            // Simpler methods to clear the path simply don't work due to bugs. This craziness does.
            var bogusCoord = _map.toCoordinate(Qt.point(height/2, width/2))
            polygonDrawerPolygon.path = [ bogusCoord, bogusCoord ]
            polygonDrawerNextPoint.path = [ bogusCoord, bogusCoord ]
            polygonDrawerPolygon.path = [ ]
            polygonDrawerNextPoint.path = [ ]
        }

        onClicked: {
            if (mouse.button == Qt.LeftButton) {
                polygonDrawer.justClicked = true
                bb.visible = true
                if (polygonDrawerPolygon.path.length > 2) {
                    // Make sure the new line doesn't intersect the existing polygon
                    var lastSegment = polygonDrawerPolygon.path.length - 2
                    var newLineA = _map.fromCoordinate(polygonDrawerPolygon.path[lastSegment], false /* clipToViewPort */)
                    var newLineB = _map.fromCoordinate(polygonDrawerPolygon.path[lastSegment+1], false /* clipToViewPort */)
                    for (var i=0; i<lastSegment; i++) {
                        var oldLineA = _map.fromCoordinate(polygonDrawerPolygon.path[i], false /* clipToViewPort */)
                        var oldLineB = _map.fromCoordinate(polygonDrawerPolygon.path[i+1], false /* clipToViewPort */)
                        if (QGroundControl.linesIntersect(newLineA, newLineB, oldLineA, oldLineB)) {
                            return;
                        }
                    }
                }

                var clickCoordinate = _map.toCoordinate(Qt.point(mouse.x, mouse.y))
                var polygonPath = polygonDrawerPolygon.path
                if (polygonPath.length == 0) {
                    // Add first coordinate
                    polygonPath.push(clickCoordinate)
                } else {
                    // Update finalized coordinate
                    shuzu.length=0
                    polygonPath[polygonDrawerPolygon.path.length - 1] = clickCoordinate
                    for(var j=0;j<polygonDrawerPolygon.path.length;j++){
                        if(j<=polygonDrawerPolygon.path.length-2&&polygonDrawerPolygon.path.length>=2){
                            var origin = polygonDrawerPolygon.path[j]
                            var coord = polygonDrawerPolygon.path[j+1]
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
                        for(var m=0;m<rr.model && m < polygonDrawerPolygon.path.length ;m++){
                            rr.itemAt(m).x = _map.fromCoordinate(polygonDrawerPolygon.path[m+1]).x
                            rr.itemAt(m).y = _map.fromCoordinate(polygonDrawerPolygon.path[m+1]).y
                            rr.itemAt(m).color = "#FFFFFF"
                            rr.itemAt(m).text=shuzu[m]
                        }
                    }
                //polygonDrawerPolygonSet.path = polygonPath
                // Add next drag coordinate
                polygonPath.push(clickCoordinate)
                polygonDrawerPolygon.path = polygonPath
                if (ScreenTools.isMobile) {
                    rr.model = 0
                    bb.visible = false
                }
            } else if (polygonDrawer.polygonReady) {
                rr.model = 0
                bb.visible = false
                finishCapturePolygon()
            }
        }

        onPositionChanged: {
            if (ScreenTools.isMobile) {
                // We don't track mouse drag on mobile
                return
            }
            if (polygonDrawerPolygon.path.length) {
                var dragCoordinate = _map.toCoordinate(Qt.point(mouse.x, mouse.y))
                // Update drag line
                polygonDrawerNextPoint.path = [ polygonDrawerPolygon.path[polygonDrawerPolygon.path.length - 2], dragCoordinate ]
                var origin = polygonDrawerPolygon.path[polygonDrawerPolygon.path.length - 2]
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
                // Update drag coordinate
                var polygonPath = polygonDrawerPolygon.path
                polygonPath[polygonDrawerPolygon.path.length - 1] = dragCoordinate
                polygonDrawerPolygon.path = polygonPath
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

    /// Polygon being drawn
    MapPolygon {
        id:         polygonDrawerPolygon
        color:      "blue"
        opacity:    0.5
        border.color: "yellow"
        border.width: 2
        visible:    polygonDrawer.drawingPolygon    //polygonDrawer.polygonReady//polygonDrawerPolygon.path.length > 2
    }

    /// Next line for polygon
    MapPolyline {
        id:         polygonDrawerNextPoint
        line.color: "green"
        line.width: 5
        visible:    polygonDrawer.drawingPolygon
    }

    //---- End Polygon code
} // Map
