import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2

import QGroundControl.ScreenTools 1.0
import QGroundControl.Palette     1.0

Rectangle {
    id: root

    property alias  label:      _label.text
    property bool   checked:    false
    property bool   small:      false

    signal clicked

    width:          _width+5
    height:         _width+5

    border.width:   small ? 1 : 2
    border.color:   "black"
    color:          checked ? "red" : qgcPal.mapButtonHighlight

    property real _width:  ScreenTools.defaultFontPixelHeight * 1.4

    QGCPalette { id: qgcPal }

    QGCLabel {
        id:                     _label
        anchors.fill:           parent
        horizontalAlignment:    Text.AlignHCenter
        verticalAlignment:      Text.AlignVCenter
        color:                  "white"
        font.pointSize:         small ? ScreenTools.smallFontPointSize : ScreenTools.defaultFontPointSize
    }

    MouseArea {
        anchors.fill: parent
        onClicked: parent.clicked()
    }

}
