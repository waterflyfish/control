/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.5
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2

import QGroundControl           1.0
import QGroundControl.Controls  1.0

import QGroundControl.Palette   1.0


/// Native QML top level window
Item {
    property var currentValue: 0
    property bool progressBarVisible : false

    function showSetupView() {
        mainWindowInner.item.showSetupView()
    }

    function attemptWindowClose() {
        mainWindowInner.item.attemptWindowClose()
    }

    function showMessage(message) {
        mainWindowInner.item.showMessage(message)
    }
    function progressBarValueChange(value){
        mainWindowInner.item.currentProgressBarValue = value
    }
    function progressBarEnable(enabled){
        mainWindowInner.item.progressBarEnabled = enabled
    }

    Loader {
        id:             mainWindowInner
        anchors.fill:   parent
        source:         "MainWindowInner.qml"

        Connections {
            target: mainWindowInner.item

            onReallyClose: controller.reallyClose()
        }
    }
	 MessageDialog{
            id: qgcDownloadStartDialog
            visible: false
            //icon: information
            modality:           Qt.ApplicationModal
            standardButtons:StandardButton.Yes | StandardButton.No
            title: qsTr("QGC Update")
            text: qsTr("there is a New version, Do you want to download it?")
            icon: StandardIcon.Question
            detailedText: qgcUpdate.detailsText
            onYes:{
                qgcUpdate.qgcDownload = true;
                progressBarVisible = true;
                progressBarEnable(progressBarVisible)
                qgcDownloadStartDialog.visible = false
            }
            onNo:{
                qgcUpdate.qgcDownload = false
                qgcDownloadStartDialog.visible = false
            }
    }

//	MessageDialog{
//		id: qgcDownloadStopDialog
//		visible:false
//		modality:   Qt.ApplicationModal
//        standardButtons: StandardButton.Yes | StandardButton.No
//		title:  qsTr("QGC Download")
//		text: qsTr("QGC downloaded successfully,now you can install it!")
//        onYes:{
//            qgcUpdate.startInstall()
//        }
//        onNo: {}
//	}

	Component.onCompleted:{
		qgcUpdate.updateQGC()
	}

	QGCUpdate{
		id: qgcUpdate
		onNewVersionChanged:{
			qgcDownloadStartDialog.visible =qgcUpdate.newVersion
		}

//		onQgcDownloadFinishedflagChanged:{
//			qgcDownloadStopDialog.visible = qgcUpdate.qgcDownloadFinishedflag
//		}
        onProgressBarValueChanged:{
            currentValue = qgcUpdate.progressBarValue
            progressBarValueChange(currentValue)
        }
	}
}
