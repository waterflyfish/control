﻿/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick              2.5
import QtQuick.Controls     1.4

import QGroundControl.Controls  1.0

SetupPage {
    id:             tuningPage
    pageComponent:  pageComponent

    Component {
        id: pageComponent

        FactSliderPanel {
            width:          availableWidth
            qgcViewPanel:   tuningPage.viewPanel

            sliderModel: ListModel {
                ListElement {
                    title:          qsTr("Roll sensitivity")
                    description:    qsTr("Slide to the left to make roll control faster and more accurate. Slide to the right if roll oscillates or is too twitchy.")
                    param:          "FW_R_TC"
                    min:            0.1
                    max:            1.0
                    step:           0.05
                }

                ListElement {
                    title:          qsTr("Pitch sensitivity")
                    description:    qsTr("Slide to the left to make pitch control faster and more accurate. Slide to the right if pitch oscillates or is too twitchy.")
                    param:          "FW_P_TC"
                    min:            0.2
                    max:            0.8
                    step:           0.01
                }

//                ListElement {
//                    title:          qsTr("Cruise throttle")
//                    description:    qsTr("This is the throttle setting required to achieve the desired cruise speed. Most planes need 50-60%.")
//                    param:          "FW_THR_CRUISE"
//                    min:            20
//                    max:            80
//                    step:           1
//                }

                ListElement {
                    title:          qsTr("Mission mode sensitivity")
                    description:    qsTr("Slide to the left to make position control more accurate and more aggressive. Slide to the right to make flight in mission mode smoother and less twitchy.")
                    param:          "FW_L1_PERIOD"
                    min:            1
                    max:            50
                    step:           0.5
                }

                ListElement {
                    title:          qsTr("Statick Horizontal Sensitivity")
                    description:    qsTr("")
                    param:          "RC_HOR_FILTER"
                    min:            0
                    max:            10
                    step:           0.5
                }
                ListElement {
                    title:          qsTr("Statick Vertical Sensitivity")
                    description:    qsTr("")
                    param:          "RC_VER_FILTER"
                    min:            0
                    max:            10
                    step:           0.5
                }
            }
        }
    }
}
