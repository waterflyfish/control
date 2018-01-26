/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @author Don Gagne <don@thegagnes.com>

import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2

import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

QGCView {
    viewPanel:  panel

    property real   _margins:    ScreenTools.defaultFontPixelHeight
    property string _emptyText:  "Attention! If you want to change the language,you should have to restart the QGC!"

    QGCPalette                      { id: qgcPal; colorGroupEnabled: enabled }
    CustomCommandWidgetController   { id: controller; factPanel: panel }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent
        Rectangle {
            anchors.fill:   parent
            color:          qgcPal.window
            QGCLabel {
                id:                 textOutput
                anchors.margins:    _margins
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.top:        parent.top
                anchors.bottom:     loader.buttonRow.top
                wrapMode:           Text.WordWrap
                textFormat:         Text.RichText
                text:               _emptyText
                visible:            !loader.visible
            }
            Loader {
                id:                 loader
                anchors.margins:    _margins
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.top:        parent.top
                anchors.bottom:     buttonRow.top
                source:             controller.customQmlFile
                visible:            false
                onStatusChanged: {
                    loader.visible = true
                    if (loader.status == Loader.Error) {
                        if (sourceComponent.status == Component.Error) {
                            textOutput.text = sourceComponent.errorString()
                            loader.visible = false
                        }
                    }
                }
            }

        }
	}
}
