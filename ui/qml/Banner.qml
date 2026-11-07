// In-page banner (info / warn / danger / ok). Slides down 6 px on show.
import QtQuick

Rectangle {
    id: b
    property string kind: "info"
    property string text: ""
    property string actionText: ""
    property string iconName: kind === "danger" ? "alertCircle" : kind === "warn" ? "alert" : kind === "ok" ? "check" : "info"
    property bool closable: true
    property bool strip: false        // full-width strip under the toolbar (app.css .banner); else an inset card
    signal action()
    signal closed()

    readonly property color col: kind === "danger" ? Theme.c.danger : kind === "warn" ? Theme.c.warning : kind === "ok" ? Theme.c.success : Theme.accent
    implicitHeight: visible ? Math.max(strip ? 40 : 42, msg.implicitHeight + 16) : 0
    radius: strip ? 0 : Theme.radius.md
    color: Theme.soft(col, Theme.dark ? 0.12 : 0.1)
    border.width: strip ? 0 : 1
    border.color: Theme.soft(col, 0.3)
    Rectangle { visible: b.strip; anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.c.line }

    opacity: visible ? 1 : 0
    transform: Translate { id: tr; y: 0 }
    onVisibleChanged: if (visible && !Theme.reducedMotion) slide.restart()
    NumberAnimation { id: slide; target: tr; property: "y"; from: -6; to: 0; duration: Theme.motion.slow; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.spring }

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
        font.weight: Font.Medium
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
