// Button with the design's motion: hover lift, press scale, color easing, icon nudges, tooltip + shortcut.
// variant: "default" | "primary" | "ghost" | "danger"
import QtQuick
import QtQuick.Controls.Basic

AbstractButton {
    id: b
    property string iconName: ""
    property string variant: "default"
    property bool small: false
    property string tip: ""
    property string kbd: ""
    property color tint: "transparent"
    readonly property bool iconOnly: text.length === 0
    readonly property bool primary: variant === "primary"
    readonly property bool ghost: variant === "ghost"
    readonly property bool danger: variant === "danger"
    readonly property bool on: checkable && checked

    hoverEnabled: true
    focusPolicy: Qt.TabFocus
    implicitHeight: small ? Theme.size.controlSmall : Theme.size.control
    implicitWidth: iconOnly ? implicitHeight : row.implicitWidth + (small ? 18 : 26)
    opacity: enabled ? 1 : 0.45
    Accessible.name: tip.length ? tip : text
    Accessible.role: Accessible.Button

    readonly property color fg: tint.a > 0 ? tint
        : primary ? Theme.onAccent
        : danger ? "#FFFFFF"
        : on ? Theme.accent
        : ghost ? (hovered ? Theme.c.text : Theme.c.text2)
        : Theme.c.text

    scale: pressed ? Theme.motion.pressScale : 1
    Behavior on scale { NumberAnimation { duration: b.pressed ? Theme.motion.press : Theme.motion.base; easing.type: Easing.OutBack } }
    transform: Translate {
        y: b.hovered && !b.pressed && b.enabled && !b.ghost ? -1 : 0
        Behavior on y { NumberAnimation { duration: Theme.motion.base; easing.type: Easing.OutCubic } }
    }

    background: Rectangle {
        radius: Theme.radius.sm
        color: b.primary ? (b.pressed ? Theme.accentPressed : b.hovered ? Theme.accentHover : Theme.accent)
             : b.danger ? (b.pressed ? Qt.darker(Theme.c.danger, 1.2) : b.hovered ? Qt.darker(Theme.c.danger, 1.08) : Theme.c.danger)
             : b.on ? Theme.accentSoft
             : b.ghost ? (b.pressed ? Theme.c.pressed : b.hovered ? Theme.c.hover : "transparent")
             : (b.pressed ? Qt.tint(Theme.c.raised, Theme.c.pressed) : b.hovered ? Qt.tint(Theme.c.raised, Theme.c.hover) : Theme.c.raised)
        border.width: b.ghost && !b.on ? 0 : 1
        border.color: b.primary ? Theme.accent : b.danger ? Theme.c.danger : b.on ? Theme.soft(Theme.accent, 0.4) : Theme.c.line
        Behavior on color { ColorAnimation { duration: Theme.motion.fast } }

        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: parent.radius + 3
            color: "transparent"
            border.width: 2
            border.color: Theme.soft(Theme.accent, 0.5)
            visible: b.visualFocus
        }
    }

    contentItem: Item {
        implicitWidth: row.implicitWidth
        implicitHeight: row.implicitHeight
        Row {
            id: row
            anchors.centerIn: parent
            spacing: b.small ? 5 : 7
            Icon {
                visible: b.iconName.length > 0
                name: b.iconName
                color: b.fg
                size: b.small ? Theme.size.iconSmall : Theme.size.icon
                anchors.verticalCenter: parent.verticalCenter
                transform: Translate {
                    x: b.hovered && (b.iconName === "chevR" || b.iconName === "external") ? 2 : 0
                    y: b.hovered && (b.iconName === "download" || b.iconName === "arrowD") ? 1.5
                       : b.hovered && (b.iconName === "upload" || b.iconName === "arrowUp2") ? -1.5 : 0
                    Behavior on x { NumberAnimation { duration: Theme.motion.base; easing.type: Easing.OutBack } }
                    Behavior on y { NumberAnimation { duration: Theme.motion.base; easing.type: Easing.OutBack } }
                }
                rotation: !b.hovered ? 0
                          : (b.iconName === "refresh" || b.iconName === "recheck" || b.iconName === "plus") ? 90
                          : b.iconName === "settings" ? 45 : 0
                Behavior on rotation { NumberAnimation { duration: Theme.motion.slow; easing.type: Easing.OutCubic } }
            }
            Text {
                visible: !b.iconOnly
                text: b.text
                color: b.fg
                font.family: Theme.fontUi
                font.pixelSize: b.small ? 13 : Theme.type.body
                font.weight: Font.DemiBold
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    Tip {
        visible: b.hovered && b.tip.length > 0
        text: b.tip
        kbd: b.kbd
    }
}
