/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.5
import QtQuick.Window   2.2
import QtQuick.Dialogs  1.2

import QGroundControl   1.0

import QGroundControl.Palette   1.0

/// Native QML top level window
Window {
    id:         _rootWindow
    visible:    true

    property bool _forceClose: false
    onClosing: {
        if (!_forceClose) {
            mainWindowInner.item.attemptWindowClose()
            close.accepted = false
        }
    }

    function showSetupView() {
        mainWindowInner.item.showSetupView()
    }

    function showMessage(message) {
        mainWindowInner.item.showMessage(message)
    }

    Loader {
        id:             mainWindowInner
        anchors.fill:   parent
        source:         "MainWindowInner.qml"

        Connections {
            target: mainWindowInner.item

            onReallyClose: {
                _forceClose = true
                _rootWindow.close()
            }
        }
    }

    MessageDialog{
       id: qgcDownloadStartDialog
       visible: false
       modality:           Qt.ApplicationModal
       standardButtons:StandardButton.Yes | StandardButton.No
       title: qsTr("QGC Update")
       text: qsTr("there is a New version, Do you want to download it?")
       icon: StandardIcon.Question
       detailedText: qgcUpdate.detailsText
       onYes:{
           qgcUpdate.qgcDownload = true;
           qgcDownloadStartDialog.visible = false
       }
       onNo:{
           qgcUpdate.qgcDownload = false
           qgcDownloadStartDialog.visible = false
       }
   }

   Component.onCompleted:{
       qgcUpdate.updateQGC()
   }

   QGCUpdate{
       id: qgcUpdate
       onNewVersionChanged:{
           qgcDownloadStartDialog.visible =qgcUpdate.newVersion
       }

       onProgressBarValueChanged:{
           currentValue = qgcUpdate.progressBarValue
           progressBarValueChange(currentValue)
       }
   }
}

