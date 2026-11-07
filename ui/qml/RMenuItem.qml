// Menu row: icon, label, optional check, shortcut cap, submenu arrow; danger items are red.
import QtQuick
import QtQuick.Controls.Basic

MenuItem {
    id: it
    property string iconName: ""
    property string kbd: ""
    property bool danger: false
    implicitHeight: visible ? Theme.size.control : 0
    implicitWidth: 240
    hoverEnabled: true
    leftPadding: 10
    rightPadding: 10
    arrow: null
    indicator: null

    background: Rectangle {
        radius: Theme.radius.sm - 2
        color: !it.enabled ? "transparent" : it.highlighted ? (it.danger ? Theme.soft(Theme.c.danger, 0.12) : Theme.c.hover) : "transparent"
    }
    contentItem: Item {
        implicitWidth: row.implicitWidth + 40
        implicitHeight: Theme.size.control
        opacity: it.enabled ? 1 : 0.42
        Row {
            id: row
            spacing: 10
            anchors.verticalCenter: parent.verticalCenter
            Item {
                width: 16
                height: 16
                anchors.verticalCenter: parent.verticalCenter
                Icon {
                    anchors.centerIn: parent
                    name: it.checkable ? (it.checked ? "check" : "") : it.iconName
                    size: 16
                    color: it.danger ? Theme.c.danger : it.checkable ? Theme.accent : Theme.c.text2
                }
            }
            Text {
                text: it.text
                color: it.danger ? Theme.c.danger : Theme.c.text
                font.family: Theme.fontUi
                font.pixelSize: 13.5
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        Text {
            visible: it.kbd.length > 0 && !it.subMenu
            text: it.kbd
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            color: Theme.c.text3
            font.family: Theme.fontMono
            font.pixelSize: 11.5
        }
        Icon {
            visible: it.subMenu !== null
            name: "chevR"
            size: 14
            color: Theme.c.text3
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
