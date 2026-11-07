// In-page banner (info / warn / danger / ok). Slides down 6 px on show.
import QtQuick

Rectangle {
    id: b
    property string kind: "info"
    property string text: ""
    property string actionText: ""
    property string iconName: kind === "danger" ? "alertCircle" : kind === "warn" ? "alert" : kind === "ok" ? "check" : "info"
    property bool closable: true
    signal action()
    signal closed()

    readonly property color col: kind === "danger" ? Theme.c.danger : kind === "warn" ? Theme.c.warning : kind === "ok" ? Theme.c.success : Theme.accent
    implicitHeight: visible ? Math.max(42, msg.implicitHeight + 20) : 0
    radius: Theme.radius.md
    color: Theme.soft(col, Theme.dark ? 0.12 : 0.1)
    border.color: Theme.soft(col, 0.3)

    opacity: visible ? 1 : 0
    transform: Translate { id: tr; y: 0 }
    onVisibleChanged: if (visible && !Theme.reducedMotion) slide.restart()
    NumberAnimation { id: slide; target: tr; property: "y"; from: -6; to: 0; duration: Theme.motion.slow; easing.type: Easing.OutBack }

    Icon {
        id: ic
        name: b.iconName
        color: b.col
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
    }
    Text {
        id: msg
        anchors.left: ic.right
        anchors.leftMargin: 10
        anchors.right: btns.left
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        text: b.text
        wrapMode: Text.Wrap
        color: Theme.c.text
        font.family: Theme.fontUi
        font.pixelSize: 13
    }
    Row {
        id: btns
        anchors.right: parent.right
        anchors.rightMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4
        Btn {
            visible: b.actionText.length > 0
            text: b.actionText
            small: true
            tip: b.actionText
            onClicked: b.action()
        }
        Btn {
            visible: b.closable
            iconName: "x"
            variant: "ghost"
            small: true
            tip: qsTr("Dismiss")
            onClicked: b.closed()
        }
    }
}
