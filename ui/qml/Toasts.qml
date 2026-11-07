// Toast stack (bottom-right). Each toast rises with a spring; a 10 s drain bar pauses on hover.
import QtQuick

Item {
    id: host
    signal undo()
    width: 360

    function show(text, kind, action) {
        toastModel.append({ text: text, kind: kind || "info", action: action || "" })
        if (toastModel.count > 4) toastModel.remove(0)
    }

    ListModel { id: toastModel }

    Column {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        spacing: 8
        Repeater {
            model: toastModel
            delegate: Rectangle {
                id: t
                required property int index
                required property string text
                required property string kind
                required property string action
                // Toasts are inverted (app.css .toast): ink background, window-coloured text. Status colours
                // come from the opposite palette so they stay readable on the ink.
                readonly property QtObject inv: Theme.dark ? Theme.stone : Theme.obsidian
                readonly property color col: kind === "danger" ? inv.danger : kind === "ok" ? inv.success : Theme.accents[Theme.accentName][Theme.dark ? 0 : 1]
                width: host.width
                height: Math.max(48, label.implicitHeight + 24)
                radius: 10
                color: Theme.c.text
                clip: true
                property real remaining: 1

                Rectangle {
                    z: -1
                    anchors.fill: parent
                    anchors.topMargin: 8
                    anchors.margins: -2
                    radius: parent.radius + 2
                    color: Theme.c.shadow
                    opacity: 0.3
                }

                transform: Translate { id: rise; y: 0 }
                Component.onCompleted: if (!Theme.reducedMotion) enterAnim.start()
                ParallelAnimation {
                    id: enterAnim
                    NumberAnimation { target: rise; property: "y"; from: 14; to: 0; duration: Theme.motion.slow; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.spring }
                    NumberAnimation { target: t; property: "opacity"; from: 0; to: 1; duration: Theme.motion.base }
                }

                NumberAnimation on remaining {
                    id: drain
                    from: 1; to: 0
                    duration: 10000
                    paused: hover.hovered
                    onFinished: toastModel.remove(t.index)
                }

                Rectangle {
                    width: 3
                    height: parent.height
                    color: t.col
                }
                Icon {
                    id: ic
                    x: 14
                    anchors.verticalCenter: parent.verticalCenter
                    name: t.kind === "danger" ? "alertCircle" : t.kind === "ok" ? "check" : "info"
                    color: t.col
                }
                Text {
                    id: label
                    anchors.left: ic.right
                    anchors.leftMargin: 10
                    anchors.right: acts.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: t.text
                    wrapMode: Text.Wrap
                    color: Theme.c.window
                    font.family: Theme.fontUi
                    font.weight: Font.DemiBold
                    font.pixelSize: 14
                }
                Row {
                    id: acts
                    anchors.right: parent.right
                    anchors.rightMargin: 6
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2
                    Btn {
                        visible: t.action === "undo"
                        text: qsTr("Undo")
                        small: true
                        variant: "ghost"
                        tint: t.col
                        tip: qsTr("Bring the removed downloads back")
                        kbd: "Ctrl+Z"
                        onClicked: { host.undo(); toastModel.remove(t.index) }
                    }
                    Btn {
                        iconName: "x"
                        small: true
                        variant: "ghost"
                        tint: Theme.c.window
                        tip: qsTr("Dismiss")
                        onClicked: toastModel.remove(t.index)
                    }
                }
                Rectangle {
                    anchors.bottom: parent.bottom
                    height: 2
                    width: parent.width * t.remaining
                    color: Theme.soft(t.col, 0.5)
                }
                HoverHandler { id: hover }
            }
        }
    }
}
