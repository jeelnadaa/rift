// Drop-down select. options: [[value, label], ...]
import QtQuick
import QtQuick.Controls.Basic

AbstractButton {
    id: s
    property var options: []
    property var value
    property string tip: ""
    property bool small: false
    signal activated(var value)

    hoverEnabled: true
    implicitHeight: small ? Theme.size.controlSmall : Theme.size.control
    implicitWidth: Math.max(140, lbl.implicitWidth + 44)
    Accessible.role: Accessible.ComboBox
    Accessible.name: tip.length ? tip : lbl.text

    readonly property string label: {
        for (var i = 0; i < options.length; ++i)
            if (options[i][0] == value) return options[i][1]
        return options.length ? options[0][1] : ""
    }

    background: Rectangle {
        radius: Theme.radius.sm
        color: s.pressed ? Qt.tint(Theme.c.input, Theme.c.pressed) : s.hovered ? Qt.tint(Theme.c.input, Theme.c.hover) : Theme.c.input
        border.width: pop.visible || s.visualFocus ? 1.5 : 1
        border.color: pop.visible || s.visualFocus ? Theme.accent : Theme.c.line
        Behavior on color { ColorAnimation { duration: Theme.motion.fast } }
    }
    contentItem: Item {
        Text {
            id: lbl
            anchors.left: parent.left
            anchors.leftMargin: 11
            anchors.right: chev.left
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            text: s.label
            color: Theme.c.text
            elide: Text.ElideRight
            font.family: Theme.fontUi
            font.pixelSize: s.small ? 13 : Theme.type.body
        }
        Icon {
            id: chev
            name: "chevD"
            size: 14
            color: Theme.c.text3
            anchors.right: parent.right
            anchors.rightMargin: 9
            anchors.verticalCenter: parent.verticalCenter
            rotation: pop.visible ? 180 : 0
            Behavior on rotation { NumberAnimation { duration: Theme.motion.base; easing.type: Easing.OutCubic } }
        }
    }
    onClicked: pop.visible ? pop.close() : pop.open()

    Popup {
        id: pop
        y: s.height + 4
        width: Math.max(s.width, 160)
        padding: 4
        height: Math.min(list.contentHeight + 8, 320)
        background: Rectangle {
            radius: Theme.radius.md
            color: Theme.c.raised
            border.color: Theme.c.line
        }
        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.motion.fast }
            NumberAnimation { property: "scale"; from: 0.96; to: 1; duration: Theme.motion.base; easing.type: Easing.OutBack }
        }
        exit: Transition { NumberAnimation { property: "opacity"; to: 0; duration: Theme.motion.fast } }
        contentItem: ListView {
            id: list
            clip: true
            model: s.options
            boundsBehavior: Flickable.StopAtBounds
            delegate: ItemDelegate {
                id: d
                required property var modelData
                width: ListView.view.width
                height: Theme.size.control
                hoverEnabled: true
                readonly property bool current: modelData[0] == s.value
                background: Rectangle {
                    radius: Theme.radius.sm - 2
                    color: d.hovered ? Theme.c.hover : "transparent"
                }
                contentItem: Row {
                    spacing: 8
                    leftPadding: 4
                    Icon {
                        name: "check"
                        size: 14
                        color: Theme.accent
                        opacity: d.current ? 1 : 0
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: d.modelData[1]
                        color: Theme.c.text
                        font.family: Theme.fontUi
                        font.pixelSize: 13.5
                        font.weight: d.current ? Font.DemiBold : Font.Normal
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                onClicked: {
                    s.value = modelData[0]
                    s.activated(modelData[0])
                    pop.close()
                }
            }
        }
    }
    Tip { visible: s.hovered && s.tip.length > 0 && !pop.visible; text: s.tip }
}
