import QtQuick          2.2
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0

// Editor for Survery mission items
Rectangle {
    id:         _root
    height:     visible ? (editorColumn.height + (_margin * 2)) : 0
    width:      availableWidth
    color:      qgcPal.windowShadeDark
    radius:     _radius

    // The following properties must be available up the hierarchy chain
    //property real   availableWidth    ///< Width for control
    //property var    missionItem       ///< Mission Item for editor

    property real   _margin:        ScreenTools.defaultFontPixelWidth / 2
    property int    _cameraIndex:   1

    readonly property int _gridTypeManual:          0
    readonly property int _gridTypeCustomCamera:    1
    readonly property int _gridTypeCamera:          2

    Component.onCompleted: {
        console.log("gridAltitude", missionItem.gridAltitude.value)
        console.log("gridAltitudeRelative", missionItem.gridAltitudeRelative)
        console.log("gridAngle", missionItem.gridAngle.value)
        console.log("gridSpacing", missionItem.gridSpacing.value)
        console.log("turnaroundDist", missionItem.turnaroundDist.value)
        console.log("cameraTrigger", missionItem.cameraTrigger)
        console.log("cameraTriggerDistance", missionItem.cameraTriggerDistance.value)
        console.log("groundResolution", missionItem.groundResolution.value)
        console.log("frontalOverlap", missionItem.frontalOverlap.value)
        console.log("sideOverlap", missionItem.sideOverlap.value)
        console.log("cameraSensorWidth", missionItem.cameraSensorWidth.value)
        console.log("cameraSensorHeight", missionItem.cameraSensorHeight.value)
        console.log("cameraResolutionWidth", missionItem.cameraResolutionWidth.value)
        console.log("cameraResolutionHeight", missionItem.cameraResolutionHeight.value)
        console.log("cameraFocalLength", missionItem.cameraFocalLength.value)
        console.log("fixedValueIsAltitude", missionItem.fixedValueIsAltitude)
        console.log("cameraOrientationLandscape", missionItem.cameraOrientationLandscape)
        console.log("manualGrid", missionItem.manualGrid)
        console.log("camera", missionItem.camera)

        console.log(missionController.cameraModelItems.count)
        saveDeleteModel.visible = false
        saveDeleteSpace.visible = false
        for(var i = 0; i < missionController.cameraModelItems.count; i++){
            var exitFlags = true
            for(var j = 1; j < cameraModelList.count; j++){
                if(cameraModelList.get(j).text == missionController.cameraModelItems.get(i).nameText){
                    exitFlags = false
                    break;
                }
            }
            if(exitFlags){
                exitFlags = false
                cameraModelList.append({"text":missionController.cameraModelItems.get(i).nameText,
                                           "sensorWidth":missionController.cameraModelItems.get(i).sensorWidth,
                                           "sensorHeight":missionController.cameraModelItems.get(i).sensorHeight,
                                           "imageWidth":missionController.cameraModelItems.get(i).imageWidth,
                                           "imageHeight":missionController.cameraModelItems.get(i).imageHeight,
                                           "focalLength":missionController.cameraModelItems.get(i).focalLength,
                                           "frontal" : missionController.cameraModelItems.get(i).frontal,
                                           "side" : missionController.cameraModelItems.get(i).side,
                                           "gridspacingadd1" : missionController.cameraModelItems.get(i).gridspacingadd1,
                                           "turnarounddist" : missionController.cameraModelItems.get(i).turnarounddist,
                                           "gridchangespeed" : missionController.cameraModelItems.get(i).gridchangespeed,
                                           "fixedValueIsAltitude" : missionController.cameraModelItems.get(i).fixedValueIsAltitude,
                                           "gridAltitude" : missionController.cameraModelItems.get(i).gridAltitude,
                                           "groundResolution" : missionController.cameraModelItems.get(i).groundResolution
                                       })
            }
        }
    }

    ListModel {
        id: cameraModelList

        Component.onCompleted: {
            cameraModelList.setProperty(_gridTypeCustomCamera, "sensorWidth", missionItem.cameraSensorWidth.rawValue)
            cameraModelList.setProperty(_gridTypeCustomCamera, "sensorHeight", missionItem.cameraSensorHeight.rawValue)
            cameraModelList.setProperty(_gridTypeCustomCamera, "imageWidth", missionItem.cameraResolutionWidth.rawValue)
            cameraModelList.setProperty(_gridTypeCustomCamera, "imageHeight", missionItem.cameraResolutionHeight.rawValue)
            cameraModelList.setProperty(_gridTypeCustomCamera, "focalLength", missionItem.cameraFocalLength.rawValue)
        }

        ListElement {
            text:           qsTr("Manual Grid (no camera specs)")
        }
        ListElement {
            text:           qsTr("Custom Camera Grid")
        }
        ListElement {
            text:           qsTr("Sony ILCE-QX1") //http://www.sony.co.uk/electronics/interchangeable-lens-cameras/ilce-qx1-body-kit/specifications
            sensorWidth:    23.2                  //http://www.sony.com/electronics/camera-lenses/sel16f28/specifications
            sensorHeight:   15.4
            imageWidth:     5456
            imageHeight:    3632
            focalLength:    16
        }
        ListElement {
            text:           qsTr("Canon S100 PowerShot")
            sensorWidth:    7.6
            sensorHeight:   5.7
            imageWidth:     4000
            imageHeight:    3000
            focalLength:    5.2
        }
        ListElement {
            text:           qsTr("Canon SX260 HS PowerShot")
            sensorWidth:    6.17
            sensorHeight:   4.55
            imageWidth:     4000
            imageHeight:    3000
            focalLength:    4.5
        }
        ListElement {
            text:           qsTr("Canon EOS-M 22mm")
            sensorWidth:    22.3
            sensorHeight:   14.9
            imageWidth:     5184
            imageHeight:    3456
            focalLength:    22
        }
        ListElement {
            text:           qsTr("Sony a6000 16mm") //http://www.sony.co.uk/electronics/interchangeable-lens-cameras/ilce-6000-body-kit#product_details_default
            sensorWidth:    23.5
            sensorHeight:   15.6
            imageWidth:     6000
            imageHeight:    4000
            focalLength:    16
        }
    }

    function recalcFromCameraValues() {
        var focalLength = missionItem.cameraFocalLength.rawValue
        var sensorWidth = missionItem.cameraSensorWidth.rawValue
        var sensorHeight = missionItem.cameraSensorHeight.rawValue
        var imageWidth = missionItem.cameraResolutionWidth.rawValue
        var imageHeight = missionItem.cameraResolutionHeight.rawValue

        var altitude = missionItem.gridAltitude.rawValue
        var groundResolution = missionItem.groundResolution.rawValue
        var frontalOverlap = missionItem.frontalOverlap.rawValue
        var sideOverlap = missionItem.sideOverlap.rawValue

        if (focalLength <= 0 || sensorWidth <= 0 || sensorHeight <= 0 || imageWidth <= 0 || imageHeight <= 0 || groundResolution <= 0) {
            return
        }

        var imageSizeSideGround     //size in side (non flying) direction of the image on the ground
        var imageSizeFrontGround    //size in front (flying) direction of the image on the ground
        var gridSpacing
        var cameraTriggerDistance

        if (missionItem.fixedValueIsAltitude) {
            groundResolution = (altitude * sensorWidth * 100) /  (imageWidth * focalLength)
        } else {
            altitude = (imageWidth * groundResolution * focalLength) / (sensorWidth * 100)
        }

        if (cameraOrientationLandscape.checked) {
            imageSizeSideGround = (imageWidth * groundResolution) / 100
            imageSizeFrontGround = (imageHeight * groundResolution) / 100
        } else {
            imageSizeSideGround = (imageHeight * groundResolution) / 100
            imageSizeFrontGround = (imageWidth * groundResolution) / 100
        }

        gridSpacing = imageSizeSideGround * ( (100-sideOverlap) / 100 )
        cameraTriggerDistance = imageSizeFrontGround * ( (100-frontalOverlap) / 100 )

        if (missionItem.fixedValueIsAltitude) {
            missionItem.groundResolution.rawValue = groundResolution
        } else {
            missionItem.gridAltitude.rawValue = altitude
        }
        missionItem.gridSpacing.rawValue = gridSpacing
        missionItem.cameraTriggerDistance.rawValue = cameraTriggerDistance
    }

    /*
    function recalcFromMissionValues() {
        var focalLength = missionItem.cameraFocalLength.rawValue
        var sensorWidth = missionItem.cameraSensorWidth.rawValue
        var sensorHeight = missionItem.cameraSensorHeight.rawValue
        var imageWidth = missionItem.cameraResolutionWidth.rawValue
        var imageHeight = missionItem.cameraResolutionHeight.rawValue

        var altitude = missionItem.gridAltitude.rawValue
        var gridSpacing = missionItem.gridSpacing.rawValue
        var cameraTriggerDistance = missionItem.cameraTriggerDistance.rawValue

        if (focalLength <= 0.0 || sensorWidth <= 0.0 || sensorHeight <= 0.0 || imageWidth < 0 || imageHeight < 0 || altitude < 0.0 || gridSpacing < 0.0 || cameraTriggerDistance < 0.0) {
            missionItem.groundResolution.rawValue = 0
            missionItem.sideOverlap = 0
            missionItem.frontalOverlap = 0
            return
        }

        var groundResolution
        var imageSizeSideGround     //size in side (non flying) direction of the image on the ground
        var imageSizeFrontGround    //size in front (flying) direction of the image on the ground

        groundResolution = (altitude * sensorWidth * 100) / (imageWidth * focalLength)

        if (cameraOrientationLandscape.checked) {
            imageSizeSideGround = (imageWidth * gsd) / 100
            imageSizeFrontGround = (imageHeight * gsd) / 100
        } else {
            imageSizeSideGround = (imageHeight * gsd) / 100
            imageSizeFrontGround = (imageWidth * gsd) / 100
        }

        var sideOverlap = (imageSizeSideGround == 0 ? 0 : 100 - (gridSpacing*100 / imageSizeSideGround))
        var frontOverlap = (imageSizeFrontGround == 0 ? 0 : 100 - (cameraTriggerDistance*100 / imageSizeFrontGround))

        missionItem.groundResolution.rawValue = groundResolution
        missionItem.sideOverlap.rawValue = sideOverlap
        missionItem.frontalOverlap.rawValue = frontOverlap
    }
    */

    function polygonCaptureStarted() {
        missionItem.clearPolygon()
    }

    function polygonCaptureFinished(coordinates) {
        for (var i=0; i<coordinates.length; i++) {
            missionItem.addPolygonCoordinate(coordinates[i])
        }
    }

    function polygonAdjustVertex(vertexIndex, vertexCoordinate) {
        missionItem.adjustPolygonCoordinate(vertexIndex, vertexCoordinate)
    }
    function returnList(){
        return missionItem.returnList();
    }

    function polygonAdjustStarted() { }
    function polygonAdjustFinished() { }

    property bool _noCameraValueRecalc: false   ///< Prevents uneeded recalcs

    Connections {
        target: missionItem

        onCameraValueChanged: {
            if (gridTypeCombo.currentIndex >= _gridTypeCustomCamera && !_noCameraValueRecalc) {
                recalcFromCameraValues()
            }
        }
    }

    Connections {
        target: missionItem.gridAltitude

        onValueChanged: {
            if (gridTypeCombo.currentIndex >= _gridTypeCustomCamera && missionItem.fixedValueIsAltitude && !_noCameraValueRecalc) {
                recalcFromCameraValues()
            }
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    ExclusiveGroup {
        id: cameraOrientationGroup

        onCurrentChanged: {
            if (gridTypeCombo.currentIndex >= _gridTypeCustomCamera) {
                recalcFromCameraValues()
            }
        }
    }

    ExclusiveGroup { id: fixedValueGroup }

    Column {
        id:                 editorColumn
        anchors.margins:    _margin
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right
        spacing:            _margin

        QGCLabel {
            anchors.left:   parent.left
            anchors.right:  parent.right
            wrapMode:       Text.WordWrap
            font.pointSize: ScreenTools.smallFontPointSize
            text:           gridTypeCombo.currentIndex == 0 ?
                                qsTr("Create a flight path which covers a polygonal area by specifying all grid parameters.") :
                                qsTr("Create a flight path which fully covers a polygonal area using camera specifications.")
        }

        QGCLabel { text: qsTr("Camera:") }

        Rectangle {
            anchors.left:   parent.left
            anchors.right:  parent.right
            height:         1
            color:          qgcPal.text
        }

        QGCComboBox {
            id:             gridTypeCombo
            anchors.left:   parent.left
            anchors.right:  parent.right
            model:          cameraModelList
            currentIndex:   -1

            Component.onCompleted: {
                if (missionItem.manualGrid) {
                    gridTypeCombo.currentIndex = _gridTypeManual
                } else {
                    var index = gridTypeCombo.find(missionItem.camera)
                    if (index == -1) {
                        console.log("Couldn't find camera", missionItem.camera)
                        gridTypeCombo.currentIndex = _gridTypeManual
                    } else {
                        gridTypeCombo.currentIndex = index
                    }
                }
            }

            onActivated: {
                if (index == _gridTypeManual) {
                    missionItem.manualGrid = true
                } else {
                    missionItem.manualGrid = false
                    missionItem.camera = gridTypeCombo.textAt(index)
                    _noCameraValueRecalc = true
                    if(index < 7){
                        missionItem.cameraSensorWidth.rawValue = cameraModelList.get(index).sensorWidth
                        missionItem.cameraSensorHeight.rawValue = cameraModelList.get(index).sensorHeight
                        missionItem.cameraResolutionWidth.rawValue = cameraModelList.get(index).imageWidth
                        missionItem.cameraResolutionHeight.rawValue = cameraModelList.get(index).imageHeight
                        missionItem.cameraFocalLength.rawValue = cameraModelList.get(index).focalLength
                        missionItem.cameraSensorWidth.rawValue = cameraModelList.get(index).sensorWidth
                        missionItem.cameraSensorHeight.rawValue = cameraModelList.get(index).sensorHeight
                        missionItem.cameraResolutionWidth.rawValue = cameraModelList.get(index).imageWidth
                        missionItem.cameraResolutionHeight.rawValue = cameraModelList.get(index).imageHeight
                        missionItem.cameraFocalLength.rawValue = cameraModelList.get(index).focalLength
                    }else{
                        missionItem.cameraSensorWidth.rawValue = cameraModelList.get(index).sensorWidth
                        missionItem.cameraSensorHeight.rawValue = cameraModelList.get(index).sensorHeight
                        missionItem.cameraResolutionWidth.rawValue = cameraModelList.get(index).imageWidth
                        missionItem.cameraResolutionHeight.rawValue = cameraModelList.get(index).imageHeight
                        missionItem.cameraFocalLength.rawValue = cameraModelList.get(index).focalLength
                        missionItem.cameraSensorWidth.rawValue = cameraModelList.get(index).sensorWidth
                        missionItem.cameraSensorHeight.rawValue = cameraModelList.get(index).sensorHeight
                        missionItem.cameraResolutionWidth.rawValue = cameraModelList.get(index).imageWidth
                        missionItem.cameraResolutionHeight.rawValue = cameraModelList.get(index).imageHeight
                        missionItem.cameraFocalLength.rawValue = cameraModelList.get(index).focalLength
                        missionItem.frontalOverlap.rawValue = cameraModelList.get(index).frontal
                        missionItem.sideOverlap.rawValue = cameraModelList.get(index).side
                        missionItem.gridSpacingAdd1.rawValue = cameraModelList.get(index).gridspacingadd1
                        missionItem.turnaroundDist.rawValue = cameraModelList.get(index).turnarounddist
                        missionItem.gridChangeSpeedFact.rawValue = cameraModelList.get(index).gridchangespeed
                        missionItem.fixedValueIsAltitude = cameraModelList.get(index).fixedValueIsAltitude
                        missionItem.gridAltitude.rawValue = cameraModelList.get(index).gridAltitude
                        missionItem.groundResolution.rawValue = cameraModelList.get(index).groundResolution
                    }
                    _noCameraValueRecalc = false
                    recalcFromCameraValues()
                }
                _cameraIndex = index
            }
        }

        // Camera based grid ui
        Column {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            visible:        gridTypeCombo.currentIndex != _gridTypeManual

            Row {
                spacing: _margin

                QGCRadioButton {
                    id:             cameraOrientationLandscape
                    width:          _editFieldWidth
                text:           qsTr("Landscape")
                    checked:        true
                    exclusiveGroup: cameraOrientationGroup
                }

                QGCRadioButton {
                    id:             cameraOrientationPortrait
                text:           qsTr("Portrait")
                    exclusiveGroup: cameraOrientationGroup
                }
            }

            Column {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin
                visible:        gridTypeCombo.currentIndex == _gridTypeCustomCamera

                GridLayout {
                    columns:        3
                    columnSpacing:  _margin
                    rowSpacing:     _margin

                    property real _fieldWidth: ScreenTools.defaultFontPixelWidth * 10

                    QGCLabel { }
                    QGCLabel { text: qsTr("Width") }
                    QGCLabel { text: qsTr("Height") }

                    QGCLabel { text: qsTr("Sensor:") }
                    FactTextField {
                        id:                     sensorWidthField
                        Layout.preferredWidth:  parent._fieldWidth
                        fact:                   missionItem.cameraSensorWidth
                    }
                    FactTextField {
                        id:                     sensorHeightField
                        Layout.preferredWidth:  parent._fieldWidth
                        fact:                   missionItem.cameraSensorHeight
                    }

                    QGCLabel { text: qsTr("Image:") }
                    FactTextField {
                        id:                     imageWidthField
                        Layout.preferredWidth:  parent._fieldWidth
                        fact:                   missionItem.cameraResolutionWidth
                    }
                    FactTextField {
                        id:                     imageHeightField
                        Layout.preferredWidth:  parent._fieldWidth
                        fact:                   missionItem.cameraResolutionHeight
                    }

                }

                FactTextFieldRow {
                    id:     focalLengthField
                    spacing: _margin
                    fact:   missionItem.cameraFocalLength
                }
            } // Column - custom camera

            QGCLabel { text: qsTr("Image Overlap") }

            Row {
                spacing:        _margin

                Item {
                    width:  ScreenTools.defaultFontPixelWidth * 2
                    height: 1
                }

                QGCLabel {
                    anchors.baseline:   frontalOverlapField_frontal.baseline
                    text:               qsTr("Frontal:")
                }

                FactTextField {
                    id:     frontalOverlapField_frontal
                    width:  ScreenTools.defaultFontPixelWidth * 7
                    fact:   missionItem.frontalOverlap
                }

                QGCLabel {
                    anchors.baseline:   frontalOverlapField_frontal.baseline
                    text:               qsTr("Side:")
                }

                FactTextField {
                    id: frontalOverlapField_side
                    width:  frontalOverlapField_frontal.width
                    fact:   missionItem.sideOverlap
                }
            }

            // Manual grid ui
            Row {
                spacing:        _margin

                QGCLabel {
                    anchors.baseline:   sensorNewName.baseline
                    text:               qsTr("Camera Name:")
                    visible:            _cameraIndex == 1
                }
                QGCTextField {
                    id:                 sensorNewName
                    unitsLabel:         ""
                    showUnits:          false
                    placeholderText:   qsTr("Camera Name")
                    readOnly:           false
                    visible:            _cameraIndex == 1
                    enabled:            _cameraIndex == 1
                    onEditingFinished:  {
                        if (_cameraIndex == 1) {
                            //cameraModelList.setProperty(_cameraIndex, "sensorWidth", Number(text))
                        }
                    }
                }
            }
            Row {
                spacing:        _margin

                Item {
                    width:  ScreenTools.defaultFontPixelWidth * 2
                    height: 1
                }
                QGCButton {
                    id:             saveButton
                    text:           qsTr("save camera")
                    visible:        _cameraIndex == 1
                    enabled:        _cameraIndex == 1
                    onClicked:      {
                        if(_cameraIndex == 1){
                            var saveFlags = true
                            for(var i = 0; i < cameraModelList.count; i++){
                                if(qsTr(sensorNewName.text) == qsTr(cameraModelList.get(i).text)){
                                    saveFlags = false
                                    break;
                                }
                            }
                            if(saveFlags
                                    && sensorWidthField.text  != 0
                                    && sensorHeightField.text != 0
                                    && imageWidthField.text   != 0
                                    && imageHeightField.text  != 0
                                    && focalLengthField.fact.valueString  != 0 ){
                                var cameraModel = new Array
                                cameraModel.push(sensorWidthField.text)
                                cameraModel.push(sensorHeightField.text)
                                cameraModel.push(imageWidthField.text)
                                cameraModel.push(imageHeightField.text)
                                cameraModel.push(focalLengthField.fact.valueString)
                                cameraModel.push(frontalOverlapField_frontal.text)
                                cameraModel.push(frontalOverlapField_side.text)
                                cameraModel.push(missionItem.gridSpacingAdd1.valueString)
                                cameraModel.push(missionItem.turnaroundDist.valueString)
                                cameraModel.push(missionItem.gridChangeSpeedFact.valueString)
                                cameraModel.push(missionItem.fixedValueIsAltitude)
                                cameraModel.push(gridAltitudeField.text)
                                cameraModel.push(groundResolutionField.text)
                                var result = missionController.operateCameraModel(sensorNewName.text, cameraModel, 0)
                                if(result){
                                    saveDeleteModel.text = qsTr("save success")
                                    saveDeleteModel.visible = true
                                    saveDeleteSpace.visible = true
                                    visible = false
                                    cameraModelList.append({"text":sensorNewName.text,
                                                               "sensorWidth":parseFloat(sensorWidthField.text),
                                                               "sensorHeight":parseFloat(sensorHeightField.text),
                                                               "imageWidth":parseFloat(imageWidthField.text),
                                                               "imageHeight":parseFloat(imageHeightField.text),
                                                               "focalLength":parseFloat(focalLengthField.fact.valueString),
                                                               "frontal": parseFloat(frontalOverlapField_frontal.text),
                                                               "side": parseFloat(frontalOverlapField_side.text),
                                                               "gridspacingadd1" : parseFloat(missionItem.gridSpacingAdd1.valueString),
                                                               "turnarounddist" : parseFloat(missionItem.turnaroundDist.valueString),
                                                               "gridchangespeed" : parseFloat(missionItem.gridChangeSpeedFact.valueString),
                                                               "fixedValueIsAltitude" : missionItem.fixedValueIsAltitude,
                                                               "gridAltitude" : parseFloat(gridAltitudeField.text),
                                                               "groundResolution" :parseFloat(groundResolutionField.text)
                                                           })
                                }else{
                                    saveDeleteModel.text = qsTr("save failed")
                                    saveDeleteModel.visible = true
                                    saveDeleteSpace.visible = true
                                }
                            }else{
                                saveDeleteModel.text = qsTr("Camera Exit!")
                                saveDeleteModel.visible = true
                                saveDeleteSpace.visible = true
                            }
                        }
                    }
                }

                QGCButton {
                    id:             deleteButton
                    text:           qsTr("delete camera")
                    visible:        _cameraIndex >= 7
                    enabled:        _cameraIndex >= 7
                    onClicked:      {
                        if(_cameraIndex >= 7){
                            var cameraModel = new Array
                            cameraModel.push(cameraModelList.get(_cameraIndex).text)
                            cameraModel.push(cameraModelList.get(_cameraIndex).text)
                            cameraModel.push(cameraModelList.get(_cameraIndex).text)
                            cameraModel.push(cameraModelList.get(_cameraIndex).text)
                            cameraModel.push(cameraModelList.get(_cameraIndex).text)
                            var result = missionController.operateCameraModel(cameraModelList.get(_cameraIndex).text, cameraModel, 1)
                            if(result){
                                saveDeleteModel.text = qsTr("delete success")
                                saveDeleteModel.visible = true
                                saveDeleteSpace.visible = true
                                for(var j = 1; j < cameraModelList.count; j++){
                                    if(cameraModelList.get(j).text == cameraModelList.get(_cameraIndex).text){
                                        cameraModelList.remove(j)
                                        _cameraIndex = 0
                                        gridTypeCombo.currentIndex = _cameraIndex
                                        break;
                                    }
                                }
                            }else{
                                saveDeleteModel.text = qsTr("delete failed")
                                saveDeleteModel.visible = true
                                saveDeleteSpace.visible = true
                            }

                        }
                    }
                }
                QGCLabel {
                    id:      saveDeleteSpace
                    text:    qsTr("")
                    visible: true
                }
                QGCLabel {
                    id:      saveDeleteModel
                    text:    qsTr("")
                    visible: false
                }
            }
            QGCLabel { text: qsTr("Grid:") }

            Rectangle {
                anchors.left:   parent.left
                anchors.right:  parent.right
                height:         1
                color:          qgcPal.text
            }

            FactTextFieldGrid {
                anchors.left:   parent.left
                anchors.right:  parent.right
                columnSpacing:  _margin
                rowSpacing:     _margin
                factList:       [ missionItem.gridAngle, missionItem.gridSpacing, missionItem.gridSpacingAdd1, missionItem.turnaroundDist, missionItem.gridChangeSpeedFact]
            }

            QGCLabel {
                anchors.left:   parent.left
                anchors.right:  parent.right
                wrapMode:       Text.WordWrap
                font.pointSize: ScreenTools.smallFontPointSize
                text:           qsTr("Which value would you like to keep constant as you adjust other settings:")
            }

            RowLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin

                QGCRadioButton {
                    id:                 fixedAltitudeRadio
                    anchors.baseline:   gridAltitudeField.baseline
                    text:               qsTr("Altitude:")
                    checked:            missionItem.fixedValueIsAltitude
                    exclusiveGroup:     fixedValueGroup
                    onClicked:          missionItem.fixedValueIsAltitude = true
                }

                FactTextField {
                    id:                 gridAltitudeField
                    Layout.fillWidth:   true
                    fact:               missionItem.gridAltitude
                    enabled:            fixedAltitudeRadio.checked
                }
            }

            RowLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin

                QGCRadioButton {
                    id:                 fixedGroundResolutionRadio
                    anchors.baseline:   groundResolutionField.baseline
                    text:               qsTr("Ground res:")
                    checked:            !missionItem.fixedValueIsAltitude
                    exclusiveGroup:     fixedValueGroup
                    onClicked:          missionItem.fixedValueIsAltitude = false
                }

                FactTextField {
                    id:                 groundResolutionField
                    Layout.fillWidth:   true
                    fact:               missionItem.groundResolution
                    enabled:            fixedGroundResolutionRadio.checked
                }
            }

            QGCLabel { text: qsTr("Camera:") }

            Rectangle {
                anchors.left:   parent.left
                anchors.right:  parent.right
                height:         1
                color:          qgcPal.text
            }

            RowLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin

                QGCCheckBox {
                    anchors.baseline:   showCameraTriggerDistanceField.baseline
                    text:               qsTr("Trigger Distance:")
                    checked:            missionItem.cameraTrigger
                    onClicked:          missionItem.cameraTrigger = checked
                }

                FactTextField {
                    id:                 showCameraTriggerDistanceField
                    Layout.fillWidth:   true
                    fact:               missionItem.cameraTriggerDistance
                    enabled:            missionItem.cameraTrigger
                }
            }
            RowLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin
                QGCCheckBox {
                    text:               qsTr("Hide Camera Trigger")
                    checked:            missionItem.hideCameraItems
                    onClicked:          missionItem.hideCameraItems = checked
                }
            }
            RowLayout{
                anchors.left : parent.left
                anchors.right : parent.right
                spacing : _margin
                QGCCheckBox{
                    text: qsTr("VTOL TurnAround")
                    checked: missionItem.hideturnaround
                    onClicked: missionItem.hideturnaround = checked
                }
            }
            RowLayout{
                anchors.left: parent.left
                anchors.right : parent.right
                spacing : _margin
            }
            QGCCheckBox{
                text : qsTr("Land at Home")
                checked : missionItem.landatLaunch
                onClicked : missionItem.landatLaunch = checked
            }
        }

        // Manual grid ui
        Column {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            visible:        gridTypeCombo.currentIndex == _gridTypeManual

            QGCLabel { text: qsTr("Grid:") }

            Rectangle {
                anchors.left:   parent.left
                anchors.right:  parent.right
                height:         1
                color:          qgcPal.text
            }

            FactTextFieldGrid {
                anchors.left:   parent.left
                anchors.right:  parent.right
                columnSpacing:  _margin
                rowSpacing:     _margin
                factList:       [ missionItem.gridAngle, missionItem.gridSpacing, missionItem.gridAltitude, missionItem.gridSpacingAdd1, missionItem.turnaroundDist,  missionItem.gridChangeSpeedFact]
            }

            QGCCheckBox {
                anchors.left:   parent.left
                text:           qsTr("Relative altitude")
                checked:        missionItem.gridAltitudeRelative
                onClicked:      missionItem.gridAltitudeRelative = checked
            }

            QGCLabel { text: qsTr("Camera:") }

            Rectangle {
                anchors.left:   parent.left
                anchors.right:  parent.right
                height:         1
                color:          qgcPal.text
            }

            RowLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin

                QGCCheckBox {
                    id:                 cameraTrigger
                    anchors.baseline:   cameraTriggerDistanceField.baseline
                    text:               qsTr("Trigger Distance:")
                    checked:            missionItem.cameraTrigger
                    onClicked:          missionItem.cameraTrigger = checked
                }

                FactTextField {
                    id:                 cameraTriggerDistanceField
                    Layout.fillWidth:   true
                    fact:               missionItem.cameraTriggerDistance
                    enabled:            missionItem.cameraTrigger
                }
            }
            RowLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin
                QGCCheckBox {
                    id:                 hideCameraTrigger
                    text:               qsTr("Hide Camera Trigger")
                    checked:            missionItem.hideCameraItems
                    onClicked:          missionItem.hideCameraItems = checked
                }
            }
            RowLayout{
                anchors.left : parent.left
                anchors.right : parent.right
                spacing : _margin
                QGCCheckBox{
                    text: qsTr("VTOL TurnAround")
                    checked: missionItem.hideturnaround
                    onClicked: missionItem.hideturnaround = checked
                }
            }
            RowLayout{
                anchors.left: parent.left
                anchors.right : parent.right
                spacing : _margin
            }
            QGCCheckBox{
                text : qsTr("Land at Home:")
                checked : missionItem.landatLaunch
                onClicked : missionItem.landatLaunch = checked
            }
        }

        QGCLabel { text: qsTr("Polygon:") }

        Rectangle {
            anchors.left:   parent.left
            anchors.right:  parent.right
            height:         1
            color:          qgcPal.text
        }

        Row {
            spacing: ScreenTools.defaultFontPixelWidth

            QGCButton {
                text:       editorMap.polygonDraw.drawingPolygon ? qsTr("Finish Draw") : qsTr("Draw")
                visible:    !editorMap.polygonDraw.adjustingPolygon
                enabled:    ((editorMap.polygonDraw.drawingPolygon && editorMap.polygonDraw.polygonReady) || !editorMap.polygonDraw.drawingPolygon)

                onClicked: {
                    if (editorMap.polygonDraw.drawingPolygon) {
                        editorMap.polygonDraw.finishCapturePolygon()
                    } else {
                        editorMap.polygonDraw.startCapturePolygon(_root)
                    }
                }
            }

            QGCButton {
                text:       editorMap.polygonDraw.adjustingPolygon ? qsTr("Finish Adjust") : qsTr("Adjust")
                visible:    missionItem.polygonPath.length > 0 && !editorMap.polygonDraw.drawingPolygon

                onClicked: {
                    if (editorMap.polygonDraw.adjustingPolygon) {
                        editorMap.polygonDraw.finishAdjustPolygon()
                    } else {
                        editorMap.polygonDraw.startAdjustPolygon(_root, missionItem.polygonPath)
                    }
                }
            }
        }

        QGCLabel { text: qsTr("Statistics:") }

        Rectangle {
            anchors.left:   parent.left
            anchors.right:  parent.right
            height:         1
            color:          qgcPal.text
        }

        Grid {
            columns: 2
            spacing: ScreenTools.defaultFontPixelWidth

            QGCLabel { text: qsTr("Survey area:") }
            QGCLabel { text: QGroundControl.squareMetersToAppSettingsAreaUnits(missionItem.coveredArea).toFixed(2) + " " + QGroundControl.appSettingsAreaUnitsString }

            QGCLabel { text: qsTr("# shots:") }
            QGCLabel { text: missionItem.cameraShots }
        }
    }
}


