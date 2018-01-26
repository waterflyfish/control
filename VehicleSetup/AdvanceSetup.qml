/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controllers           1.0
import QGroundControl.ScreenTools           1.0

QGCView {
    id:                         qgcView
    viewPanel:       panel
    color:                  qgcPal.window
    anchors.fill:       parent
     anchors.margins:    ScreenTools.defaultFontPixelWidth

     property   real    _margins :    ScreenTools.defaultFontPixelWidth
     property real _labelWidth:                  ScreenTools.defaultFontPixelWidth * 15
     property real _editFieldWidth:              ScreenTools.defaultFontPixelWidth * 20

     property var _vehicletype


    QGCPalette { id: qgcPal; colorGroupEnabled: panel.enabled }
//    ParameterEditorController { id: controller; factPanel: parent }
    FactPanelController { id:   controller; factPanel:  qgcView }
    function vehicletype()
    {
       var  type = controller.setupSource()
        if (type  == "Plane")
            _vehicletype = plane;
        else if(type == "Copter")
            _vehicletype = copter
        else
            _vehicletype = vtol
        return _vehicletype;
    }
    Component.onCompleted:{
        vehicletype();
    }
    QGCViewPanel {
        id:             panel
        anchors.fill:   parent

        QGCLabel {
            id: titleLabel
            text:           qsTr("AdvanceSetup")
            font.family:    ScreenTools.demiboldFontFamily
        }
        QGCLabel{
            id : titleLabel2
            anchors.top :  titleLabel.bottom
            text:           qsTr(" ")
            font.family:    ScreenTools.demiboldFontFamily
        }
        QGCFlickable {
            clip:               true
            anchors.left  : parent.left
            anchors.right:  parent.right
            anchors.top : titleLabel2.bottom
            anchors.bottom : parent.bottom

            contentHeight:   ccc.height
            contentWidth:   ccc.width
            Loader{
                id: ccc
                anchors.fill: parent
                sourceComponent: _vehicletype
            }
         Component{
            id: copter
            Column{
                id: settingsColumn
                width:   qgcView.width
                anchors.margins: ScreenTools.defaultFontPixelWidth
                spacing:            ScreenTools.defaultFontPixelHeight*0.5
                Item{
                    width :  (_labelWidth + _editFieldWidth) * 2 + 10 * _margins
                    height : pwmLabel.height
                    anchors.margins: ScreenTools.defaultFontPixelWidth
                    QGCLabel{
                        id : pwmLabel
                        text:   qsTr("PWM OUT")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle{
                    height: pwmOut.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:          (_labelWidth + _editFieldWidth) * 2 + 10 * _margins
                    radius:         ScreenTools.defaultFontPixelWidth*0.5
                    color:          qgcPal.windowShade
                    anchors.margins: ScreenTools.defaultFontPixelWidth
                    Column{
                        id : pwmOut
                        spacing:  ScreenTools.defaultFontPixelWidth
                        anchors.centerIn: parent
                        Row{
                            spacing:    _margins
                            QGCLabel{
                                text:       qsTr("Minimum PWM:")
                                font.family:  ScreenTools.demiboldFontFamily
                                width:              _labelWidth
                                anchors.baseline: pwmmin.baseline
                            }
                            FactTextField{
                                    id:                         pwmmin
                                    fact:                     controller.getParameterFact(-1,  "PWM_MIN")
                                    showUnits:      true
                                    width:                _editFieldWidth
                            }
                        }
                    }
                 }
                Item{
                        width :  (_labelWidth + _editFieldWidth) * 2 + 10 * _margins
                        height : copterrcLabel.height
                        anchors.margins: ScreenTools.defaultFontPixelWidth
                        QGCLabel{
                            id : copterrcLabel
                            text:   qsTr("RC Setup")
                            font.family:    ScreenTools.demiboldFontFamily
                        }
                    }
                    Rectangle{
                        height: copterrcSetup.height + (ScreenTools.defaultFontPixelHeight * 2)
                        width:          (_labelWidth + _editFieldWidth) * 2 + 10 * _margins
                        radius:         ScreenTools.defaultFontPixelWidth*0.5
                        color:          qgcPal.windowShade
                        anchors.margins: ScreenTools.defaultFontPixelWidth
                        Column{
                            id : copterrcSetup
                            spacing:  ScreenTools.defaultFontPixelWidth
                            anchors.centerIn: parent
                            Row{
                                spacing:    _margins
                                QGCLabel{
                                    text:       qsTr("RC1_REV:")
                                    font.family:  ScreenTools.demiboldFontFamily
                                    width:              _labelWidth
                                    anchors.baseline: copterrc1.baseline
                                }
                                FactTextField{
                                        id:                         copterrc1
                                        fact:                     controller.getParameterFact(-1,  "RC1_REV")
                                        showUnits:      true
                                        width:                _editFieldWidth
                                }
                                QGCLabel{
                                    text:                            qsTr("RC2_REV:")
                                    font.family:               ScreenTools.demiboldFontFamily
                                    width:                          _labelWidth
                                    anchors.baseline:  copterrc2.baseline
                                }
                                FactTextField{
                                    id:                           copterrc2
                                    fact:                       controller.getParameterFact(-1, "RC2_REV")
                                    showUnits:         true
                                    width:                    _editFieldWidth
                                }
                            }
                            Row{
                                spacing:    _margins
                                QGCLabel{
                                    text:       qsTr("RC3_REV:")
                                    font.family:  ScreenTools.demiboldFontFamily
                                    width:              _labelWidth
                                    anchors.baseline: copterrc3.baseline
                                }
                                FactTextField{
                                        id:                         copterrc3
                                        fact:                     controller.getParameterFact(-1,  "RC3_REV")
                                        showUnits:      true
                                        width:                _editFieldWidth
                                }
                                QGCLabel{
                                    text:                            qsTr("RC4_REV:")
                                    font.family:               ScreenTools.demiboldFontFamily
                                    width:                          _labelWidth
                                    anchors.baseline:   copterrc4.baseline
                                }
                                FactTextField{
                                    id:                           copterrc4
                                    fact:                       controller.getParameterFact(-1, "RC4_REV")
                                    showUnits:         true
                                    width:                    _editFieldWidth
                                }
                            }
                        }
                    }
                 }
            }
         Component{
            id: plane
            Column{
                id: planesettingsColumn
                width:   qgcView.width
                anchors.margins: ScreenTools.defaultFontPixelWidth
                spacing:            ScreenTools.defaultFontPixelHeight*0.5

                Item{
                    width: (_labelWidth + _editFieldWidth) * 2 + 10 * _margins
                    height: planeL1control.height
                    anchors.margins:ScreenTools.defaultFontPixelWidth
                    QGCLabel{
                        id: planeL1control
                        text: qsTr("L1 Controller")
                        font.family: ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle{
                    height: planeL1controlSetup.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:  (_labelWidth + _editFieldWidth) * 2 + 10 * _margins
                    radius : ScreenTools.defaultFontPixelWidth * 0.5
                    color : qgcPal.windowShade
                    anchors.margins: ScreenTools.defaultFontPixelWidth
                    Column{
                        id : planeL1controlSetup
                        spacing:ScreenTools.defaultFontPixelWidth
                        anchors.centerIn: parent

                        Row{
                            spacing : _margins
                            QGCLabel{
                                text:       qsTr("FW_P_LIM_MAX:")
                                font.family:  ScreenTools.demiboldFontFamily
                                width:              _labelWidth
                                anchors.baseline: planeP_Max.baseline
                            }
                            FactTextField{
                                    id:                          planeP_Max
                                    fact:                     controller.getParameterFact(-1,  "FW_P_LIM_MAX")
                                    showUnits:      true
                                    width:                _editFieldWidth
                            }
                            QGCLabel{
                                text:                            qsTr("FW_P_LIM_MIN:")
                                font.family:               ScreenTools.demiboldFontFamily
                                width:                          _labelWidth
                                anchors.baseline:  planeP_Min.baseline
                            }
                            FactTextField{
                                id:                           planeP_Min
                                fact:                       controller.getParameterFact(-1, "FW_P_LIM_MIN")
                                showUnits:         true
                                width:                    _editFieldWidth
                            }
                        }
                        Row{
                            spacing : _margins
                            QGCLabel{
                                text:       qsTr("FW_R_LIM:")
                                font.family:  ScreenTools.demiboldFontFamily
                                width:              _labelWidth
                                anchors.baseline: planerR_Lim.baseline
                            }
                            FactTextField{
                                    id:                         planerR_Lim
                                    fact:                     controller.getParameterFact(-1,  "FW_R_LIM")
                                    showUnits:      true
                                    width:                _editFieldWidth
                            }
                            QGCLabel{
                                text:                            qsTr("FW_AIRSPD_TRIM:")
                                font.family:               ScreenTools.demiboldFontFamily
                                width:                          _labelWidth
                                anchors.baseline:  planeAirspd_Trim.baseline
                            }
                            FactTextField{
                                id:                           planeAirspd_Trim
                                fact:                       controller.getParameterFact(-1, "FW_AIRSPD_TRIM")
                                showUnits:         true
                                width:                    _editFieldWidth
                            }
                        }
                    }
                }

                Item{
                    width :  (_labelWidth + _editFieldWidth) * 2 + 10 * _margins
                    height : planercLabel.height
                    anchors.margins: ScreenTools.defaultFontPixelWidth
                    QGCLabel{
                        id : planercLabel
                        text:   qsTr("RC Setup")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle{
                        height: planercSetup.height + (ScreenTools.defaultFontPixelHeight * 2)
                        width:          (_labelWidth + _editFieldWidth) * 2 + 10 * _margins
                        radius:         ScreenTools.defaultFontPixelWidth*0.5
                        color:          qgcPal.windowShade
                        anchors.margins: ScreenTools.defaultFontPixelWidth
                        Column{
                            id : planercSetup
                            spacing:  ScreenTools.defaultFontPixelWidth
                            anchors.centerIn: parent
                            Row{
                                spacing:    _margins
                                QGCLabel{
                                    text:       qsTr("RC1_REV:")
                                    font.family:  ScreenTools.demiboldFontFamily
                                    width:              _labelWidth
                                    anchors.baseline: planerc1.baseline
                                }
                                FactTextField{
                                        id:                         planerc1
                                        fact:                     controller.getParameterFact(-1,  "RC1_REV")
                                        showUnits:      true
                                        width:                _editFieldWidth
                                }
                                QGCLabel{
                                    text:                            qsTr("RC2_REV:")
                                    font.family:               ScreenTools.demiboldFontFamily
                                    width:                          _labelWidth
                                    anchors.baseline:  planerc2.baseline
                                }
                                FactTextField{
                                    id:                           planerc2
                                    fact:                       controller.getParameterFact(-1, "RC2_REV")
                                    showUnits:         true
                                    width:                    _editFieldWidth
                                }
                            }
                            Row{
                                spacing:    _margins
                                QGCLabel{
                                    text:       qsTr("RC3_REV:")
                                    font.family:  ScreenTools.demiboldFontFamily
                                    width:              _labelWidth
                                    anchors.baseline: planerc3.baseline
                                }
                                FactTextField{
                                        id:                         planerc3
                                        fact:                     controller.getParameterFact(-1,  "RC3_REV")
                                        showUnits:      true
                                        width:                _editFieldWidth
                                }
                                QGCLabel{
                                    text:                            qsTr("RC4_REV:")
                                    font.family:               ScreenTools.demiboldFontFamily
                                    width:                          _labelWidth
                                    anchors.baseline:   planerc4.baseline
                                }
                                FactTextField{
                                    id:                           planerc4
                                    fact:                       controller.getParameterFact(-1, "RC4_REV")
                                    showUnits:         true
                                    width:                    _editFieldWidth
                                }
                            }
                        }
                    }
                }
            }

         Component{
            id: vtol
            Column{
                id: planesettingsColumn
                width:   qgcView.width
                anchors.margins: ScreenTools.defaultFontPixelWidth
                spacing:            ScreenTools.defaultFontPixelHeight*0.5

                Item{
                    width: (_labelWidth + _editFieldWidth) * 2 + 10 * _margins
                    height: vtolL1control.height
                    anchors.margins:ScreenTools.defaultFontPixelWidth
                    QGCLabel{
                        id: vtolL1control
                        text: qsTr("L1 Controller")
                        font.family: ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle{
                    height: vtolL1controlSetup.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:  (_labelWidth + _editFieldWidth) * 2 + 10 * _margins
                    radius : ScreenTools.defaultFontPixelWidth * 0.5
                    color : qgcPal.windowShade
                    anchors.margins: ScreenTools.defaultFontPixelWidth
                    Column{
                        id : vtolL1controlSetup
                        spacing:ScreenTools.defaultFontPixelWidth
                        anchors.centerIn: parent

                        Row{
                            spacing : _margins
                            QGCLabel{
                                text:       qsTr("FW_P_LIM_MAX:")
                                font.family:  ScreenTools.demiboldFontFamily
                                width:              _labelWidth
                                anchors.baseline: vtolP_Max.baseline
                            }
                            FactTextField{
                                    id:                          vtolP_Max
                                    fact:                     controller.getParameterFact(-1,  "FW_P_LIM_MAX")
                                    showUnits:      true
                                    width:                _editFieldWidth
                            }
                            QGCLabel{
                                text:                            qsTr("FW_P_LIM_MIN:")
                                font.family:               ScreenTools.demiboldFontFamily
                                width:                          _labelWidth
                                anchors.baseline:  vtolP_Min.baseline
                            }
                            FactTextField{
                                id:                           vtolP_Min
                                fact:                       controller.getParameterFact(-1, "FW_P_LIM_MIN")
                                showUnits:         true
                                width:                    _editFieldWidth
                            }
                        }
                        Row{
                            spacing : _margins
                            QGCLabel{
                                text:       qsTr("FW_R_LIM:")
                                font.family:  ScreenTools.demiboldFontFamily
                                width:              _labelWidth
                                anchors.baseline: vtolR_Lim.baseline
                            }
                            FactTextField{
                                    id:                         vtolR_Lim
                                    fact:                     controller.getParameterFact(-1,  "FW_R_LIM")
                                    showUnits:      true
                                    width:                _editFieldWidth
                            }
                            QGCLabel{
                                text:                            qsTr("FW_AIRSPD_TRIM:")
                                font.family:               ScreenTools.demiboldFontFamily
                                width:                          _labelWidth
                                anchors.baseline:  vtolAirspd_Trim.baseline
                            }
                            FactTextField{
                                id:                           vtolAirspd_Trim
                                fact:                       controller.getParameterFact(-1, "FW_AIRSPD_TRIM")
                                showUnits:         true
                                width:                    _editFieldWidth
                            }
                        }
                    }
                }

                Item{
                        width :  (_labelWidth + _editFieldWidth) * 2 + 10 * _margins
                        height : vtolrcLabel.height
                        anchors.margins: ScreenTools.defaultFontPixelWidth
                        QGCLabel{
                            id : vtolrcLabel
                            text:   qsTr("RC Setup")
                            font.family:    ScreenTools.demiboldFontFamily
                        }
                    }
                    Rectangle{
                        height: vtolrcSetup.height + (ScreenTools.defaultFontPixelHeight * 2)
                        width:          (_labelWidth + _editFieldWidth) * 2 + 10 * _margins
                        radius:         ScreenTools.defaultFontPixelWidth*0.5
                        color:          qgcPal.windowShade
                        anchors.margins: ScreenTools.defaultFontPixelWidth
                        Column{
                            id : vtolrcSetup
                            spacing:  ScreenTools.defaultFontPixelWidth
                            anchors.centerIn: parent
                            Row{
                                spacing:    _margins
                                QGCLabel{
                                    text:       qsTr("RC1_REV:")
                                    font.family:  ScreenTools.demiboldFontFamily
                                    width:              _labelWidth
                                    anchors.baseline: vtolrc1.baseline
                                }
                                FactTextField{
                                        id:                        vtolrc1
                                        fact:                     controller.getParameterFact(-1,  "RC1_REV")
                                        showUnits:      true
                                        width:                _editFieldWidth
                                }
                                QGCLabel{
                                    text:                            qsTr("RC2_REV:")
                                    font.family:               ScreenTools.demiboldFontFamily
                                    width:                          _labelWidth
                                    anchors.baseline:  vtolrc2.baseline
                                }
                                FactTextField{
                                    id:                           vtolrc2
                                    fact:                       controller.getParameterFact(-1, "RC2_REV")
                                    showUnits:         true
                                    width:                    _editFieldWidth
                                }
                            }
                            Row{
                                spacing:    _margins
                                QGCLabel{
                                    text:       qsTr("RC3_REV:")
                                    font.family:  ScreenTools.demiboldFontFamily
                                    width:              _labelWidth
                                    anchors.baseline: vtolrc3.baseline
                                }
                                FactTextField{
                                        id:                         vtolrc3
                                        fact:                     controller.getParameterFact(-1,  "RC3_REV")
                                        showUnits:      true
                                        width:                _editFieldWidth
                                }
                                QGCLabel{
                                    text:                            qsTr("RC4_REV:")
                                    font.family:               ScreenTools.demiboldFontFamily
                                    width:                          _labelWidth
                                    anchors.baseline:   vtolrc4.baseline
                                }
                                FactTextField{
                                    id:                           vtolrc4
                                    fact:                       controller.getParameterFact(-1, "RC4_REV")
                                    showUnits:         true
                                    width:                    _editFieldWidth
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
