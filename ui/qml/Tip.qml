// Tooltip used everywhere (design: every interactive thing explains itself). Optional shortcut key cap.
import QtQuick
import QtQuick.Controls.Basic

ToolTip {
    id: tip
    property string kbd: ""
    delay: Theme.motion.tooltipDelay
    timeout: -1
    padding: 0
    margins: 8
    y: parent ? parent.height + 6 : 0
    x: parent ? Math.round((parent.width - width) / 2) : 0

    contentItem: Row {
        spacing: 8
        leftPadding: 9
        rightPadding: tip.kbd.length ? 6 : 9
        topPadding: 6
        bottomPadding: 6
        Text {
            text: tip.text
            color: Theme.c.window
            font.family: Theme.fontUi
            font.pixelSize: 13
            font.weight: Font.Medium
            anchors.verticalCenter: parent.verticalCenter
        }
        Rectangle {
            visible: tip.kbd.length > 0
            anchors.verticalCenter: parent.verticalCenter
            radius: 4
            color: Qt.rgba(Theme.c.window.r, Theme.c.window.g, Theme.c.window.b, 0.16)
            implicitWidth: kbdText.implicitWidth + 10
            implicitHeight: 18
            Text {
                id: kbdText
                anchors.centerIn: parent
                text: tip.kbd
                color: Theme.c.window
                font.family: Theme.fontMono; font.features: Theme.numFeatures
                font.pixelSize: 11
                font.weight: Font.DemiBold
            }
        }
    }
    background: Rectangle {
        radius: 6
        color: Theme.c.text
    }
    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.motion.fast }
            NumberAnimation { property: "scale"; from: 0.94; to: 1; duration: Theme.motion.base; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.spring }
        }
    }
    exit: Transition { NumberAnimation { property: "opacity"; to: 0; duration: Theme.motion.fast } }
}
