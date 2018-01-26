import QtQuick 2.0
import QtQuick.Layouts 1.0
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

Canvas{
    id: roundProgressBar
    width: 100
    height: 100
    antialiasing: true

    property color primaryColor : "orange"
    property color secondaryColor : "lightblue"

    property real centerWidth : width /2
    property real centerHeight : height /2

    property real radius : Math.min(roundProgressBar.width-5 , roundProgressBar.height-5 )/2

    property real minimumValue : 0
    property real maximumValue : 100
    property real currentValue : 0

    property real angle :(currentValue - minimumValue)/(maximumValue - minimumValue)*2 *Math.PI

    property real angleOffset : -Math.PI/2

    signal clicked ()

    onPrimaryColorChanged: requestPaint()
    onSecondaryColorChanged: requestPaint()
    onMinimumValueChanged: requestPaint()
    onMaximumValueChanged: requestPaint()
    onCurrentValueChanged: {
        requestPaint()
    }

    onPaint:{
        var ctx = getContext("2d")
        ctx.save()
        ctx.clearRect(0,0,roundProgressBar.width,roundProgressBar.height)

        if(currentValue){
            ctx.beginPath()
            ctx.lineWidth = 2
            ctx.fillStyle = Qt.lighter(roundProgressBar.secondaryColor,1.25)
            ctx.arc(roundProgressBar.centerWidth,
                    roundProgressBar.centerHeight,
                    roundProgressBar.radius,
                    0,
                    2*Math.PI)
            ctx.fill()
            txt_progress.text = currentValue.toString() + "%"
        }

        ctx.beginPath();  //first circle
        ctx.lineWidth = 2
        ctx.strokeStyle = primaryColor
        ctx.arc(roundProgressBar.centerWidth,
                roundProgressBar.centerHeight,
                roundProgressBar.radius,
                angleOffset + roundProgressBar.angle,
                angleOffset + 2*Math.PI)
        ctx.stroke()

        ctx.beginPath()
        ctx.lineWidth = 2
        ctx.strokeStyle = roundProgressBar.secondaryColor
        ctx.arc(roundProgressBar.centerWidth,
                roundProgressBar.centerHeight,
                roundProgressBar.radius,
                roundProgressBar.angleOffset,
                roundProgressBar.angleOffset + roundProgressBar.angle)
        ctx.stroke()
        ctx.restore()
    }
    Text{
        id: txt_progress
        anchors.centerIn: parent

        font.pixelSize: 10
//        text: roundProgressBar.text
        color: roundProgressBar.primaryColor
    }
}

