// Progress bar. Width eases; active downloads show a slow sheen; indeterminate when value < 0.
import QtQuick

Rectangle {
    id: p
    property real value: 0
    property color fill: Theme.accent
    property bool active: false
    property bool thick: false
    implicitHeight: thick ? 8 : 5
    implicitWidth: 120
    radius: height / 2
    color: Theme.c.pieceMissing
    clip: true

    Rectangle {
        id: bar
        visible: p.value >= 0
        height: parent.height
        radius: parent.radius
        width: Math.max(p.value > 0 ? height : 0, parent.width * Math.min(1, Math.max(0, p.value)))
        color: p.fill
        Behavior on width { NumberAnimation { duration: Theme.motion.slow; easing.type: Easing.OutCubic } }

        Rectangle {
            visible: p.active && !Theme.reducedMotion && bar.width > 20
            width: 40
            height: parent.height
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0; color: "transparent" }
                GradientStop { position: 0.5; color: Qt.rgba(1, 1, 1, Theme.dark ? 0.28 : 0.4) }
                GradientStop { position: 1; color: "transparent" }
            }
            NumberAnimation on x {
                running: p.active && !Theme.reducedMotion
                loops: Animation.Infinite
                from: -40
                to: Math.max(bar.width, 41)
                duration: 1800
            }
        }
    }
    Rectangle {
        visible: p.value < 0
        height: parent.height
        width: parent.width * 0.3
        radius: parent.radius
        color: p.fill
        NumberAnimation on x {
            running: p.value < 0
            loops: Animation.Infinite
            from: -p.width * 0.3
            to: p.width
            duration: 1200
            easing.type: Easing.InOutQuad
        }
    }
}
