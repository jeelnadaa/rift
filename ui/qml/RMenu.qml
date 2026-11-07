// Menu with the design's look: raised card, scales in from .96 with a small drop.
import QtQuick
import QtQuick.Controls.Basic

Menu {
    id: m
    padding: 4
    implicitWidth: 248
    delegate: RMenuItem {}
    background: Rectangle {
        implicitWidth: 248
        radius: Theme.radius.md
        color: Theme.c.raised
        border.color: Theme.c.line
        Rectangle {
            z: -1
            anchors.fill: parent
            anchors.topMargin: 6
            anchors.margins: -2
            radius: parent.radius + 2
            color: Theme.c.shadow
            opacity: 0.35
        }
    }
    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.motion.fast }
            NumberAnimation { property: "scale"; from: 0.96; to: 1; duration: Theme.motion.base; easing.type: Easing.OutBack }
        }
    }
    exit: Transition { NumberAnimation { property: "opacity"; to: 0; duration: Theme.motion.fast } }
}
