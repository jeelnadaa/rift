// State badge: colored dot + label. Active states pulse (1.6 s), unless reduced motion.
import QtQuick

Rectangle {
    id: b
    property string state_: ""
    property string text: Theme.stateLabel(state_)
    readonly property color col: Theme.stateColor(state_)
    implicitHeight: 22
    implicitWidth: row.implicitWidth + 16
    radius: 11
    color: Theme.soft(col, Theme.dark ? 0.14 : 0.11)

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 6
        Item {
            width: 6
            height: 6
            anchors.verticalCenter: parent.verticalCenter
            Rectangle {
                anchors.centerIn: parent
                width: 6; height: 6; radius: 3
                color: b.col
            }
            Rectangle {
                id: halo
                anchors.centerIn: parent
                width: 6; height: 6; radius: 3
                color: b.col
                visible: Theme.isLive(b.state_) && !Theme.reducedMotion
                SequentialAnimation on scale {
                    running: halo.visible
                    loops: Animation.Infinite
                    NumberAnimation { from: 1; to: 2.6; duration: 1600; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.standard }
                }
                SequentialAnimation on opacity {
                    running: halo.visible
                    loops: Animation.Infinite
                    NumberAnimation { from: 0.5; to: 0; duration: 1600; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.standard }
                }
            }
        }
        Text {
            text: b.text
            color: b.col
            font.family: Theme.fontUi
            font.pixelSize: 12
            font.weight: Font.DemiBold
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
