// Checkbox with an animated check mark. Supports tristate (partially checked).
import QtQuick
import QtQuick.Controls.Basic

CheckBox {
    id: c
    property string tip: ""
    hoverEnabled: true
    spacing: 10
    padding: 0
    font.family: Theme.fontUi
    font.pixelSize: Theme.type.body

    indicator: Rectangle {
        implicitWidth: 18
        implicitHeight: 18
        x: c.leftPadding
        y: (c.height - height) / 2
        radius: 5
        color: c.checkState !== Qt.Unchecked ? Theme.accent : Theme.c.input
        border.width: c.checkState !== Qt.Unchecked ? 0 : 1.5
        border.color: c.hovered ? Theme.c.text3 : Theme.c.line
        Behavior on color { ColorAnimation { duration: Theme.motion.fast } }
        // Pops 1.18× when checked; squashes .88 while pressed (DESIGN.md §5).
        scale: c.pressed ? Theme.motion.squashScale : 1
        Behavior on scale { NumberAnimation { duration: c.pressed ? Theme.motion.press : Theme.motion.base; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.spring } }
        Connections {
            target: c
            function onCheckStateChanged() { if (c.checkState !== Qt.Unchecked && !Theme.reducedMotion) pop.restart() }
        }
        SequentialAnimation {
            id: pop
            NumberAnimation { target: popScale; properties: "xScale,yScale"; to: 1.18; duration: Theme.motion.fast; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.standard }
            NumberAnimation { target: popScale; properties: "xScale,yScale"; to: 1; duration: Theme.motion.base; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.spring }
        }
        transform: Scale { id: popScale; origin.x: 9; origin.y: 9 }
        Icon {
            anchors.centerIn: parent
            name: c.checkState === Qt.PartiallyChecked ? "minimize" : "check"
            color: Theme.onAccent
            size: 13
            stroke: 2.4
            scale: c.checkState !== Qt.Unchecked ? 1 : 0.4
            opacity: c.checkState !== Qt.Unchecked ? 1 : 0
            Behavior on scale { NumberAnimation { duration: Theme.motion.base; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.spring } }
            Behavior on opacity { NumberAnimation { duration: Theme.motion.fast } }
        }
    }
    contentItem: Text {
        text: c.text
        visible: c.text.length > 0
        leftPadding: c.indicator.width + c.spacing
        color: c.enabled ? Theme.c.text : Theme.c.text3
        font: c.font
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    Tip { visible: c.hovered && c.tip.length > 0; text: c.tip }
}
