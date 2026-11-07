// On/off switch with a springy knob.
import QtQuick
import QtQuick.Controls.Basic

Switch {
    id: s
    property string tip: ""
    hoverEnabled: true
    padding: 0
    spacing: 10
    font.family: Theme.fontUi
    font.pixelSize: Theme.type.body
    Accessible.name: tip.length ? tip : text

    indicator: Rectangle {
        implicitWidth: 34
        implicitHeight: 20
        x: s.text.length ? s.width - width : 0
        y: (s.height - height) / 2
        radius: 10
        color: s.checked ? Theme.accent : (s.hovered ? Qt.tint(Theme.c.sunken, Theme.c.pressed) : Theme.c.sunken)
        border.width: s.checked ? 0 : 1
        border.color: Theme.c.line
        Behavior on color { ColorAnimation { duration: Theme.motion.base } }
        scale: s.pressed ? Theme.motion.squashScale : 1
        Behavior on scale { NumberAnimation { duration: s.pressed ? Theme.motion.press : Theme.motion.base; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.spring } }
        Rectangle {
            width: s.pressed ? 17 : 14
            height: 14
            radius: 7
            y: 3
            x: s.checked ? parent.width - width - 3 : 3
            color: s.checked ? Theme.onAccent : Theme.c.raised
            border.width: s.checked ? 0 : 1
            border.color: Theme.c.line
            Behavior on width { NumberAnimation { duration: Theme.motion.fast } }
            Behavior on x { NumberAnimation { duration: Theme.motion.base; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.spring } }
        }
    }
    contentItem: Text {
        text: s.text
        visible: s.text.length > 0
        rightPadding: s.indicator.width + s.spacing
        color: Theme.c.text
        font: s.font
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    Tip { visible: s.hovered && s.tip.length > 0; text: s.tip }
}
