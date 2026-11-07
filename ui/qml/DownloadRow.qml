// One row in the download list. Hover: 2 px inset rule. Selected: 3 px accent rule + selection fill.
import QtQuick
import QtQuick.Layouts

Item {
    id: r
    required property int index
    required property string rid
    required property string kind
    required property string name
    required property var model
    required property string category
    required property string error
    required property var tags
    required property real size
    required property real done
    required property real progress
    required property real down
    required property real up
    required property real eta
    required property int seeds
    required property int peers
    required property int seedsTotal
    required property int peersTotal
    required property int conns
    required property var segs
    required property real ratio

    property var cols
    property bool selected: false
    property bool hovered: ma.containsMouse
    signal pressed(var mouse)
    signal doubleClicked()
    signal menu(real x, real y)

    readonly property string st: r.model.state
    readonly property color stCol: Theme.stateColor(st)
    readonly property bool dim: Theme.isDim(st)
    implicitHeight: Theme.size.row
    Accessible.role: Accessible.Row
    Accessible.name: name + ", " + Theme.stateLabel(st) + ", " + Math.round(progress * 100) + "%"
    Accessible.selected: selected

    Rectangle {
        anchors.fill: parent
        color: r.selected ? Theme.selection : r.hovered ? Theme.c.hover : "transparent"
        Behavior on color { ColorAnimation { duration: Theme.motion.fast } }
    }
    Rectangle {
        width: r.selected ? 3 : 2
        height: parent.height
        color: r.selected ? Theme.accent : Theme.c.line
        visible: r.selected || r.hovered
    }
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.c.lineSoft }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 12
        opacity: r.dim ? 0.62 : 1

        // Name
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 160
            Icon {
                id: fileIcon
                anchors.verticalCenter: parent.verticalCenter
                name: r.kind === "bt" ? "magnet" : {
                    "Video": "film", "Music": "music", "Archives": "archive", "Programs": "app",
                    "Documents": "doc", "Disc images": "disc", "Images": "eye" }[r.category] || "file"
                color: r.st === "error" ? Theme.c.danger : r.kind === "bt" ? Theme.kindTorrent : Theme.c.kindHttp
                size: 18
            }
            Column {
                anchors.left: fileIcon.right
                anchors.leftMargin: 10
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 3
                Text {
                    width: parent.width
                    text: r.name
                    elide: Text.ElideMiddle
                    color: Theme.c.text
                    font.family: Theme.fontUi
                    font.pixelSize: Theme.type.body
                    font.weight: Font.DemiBold
                }
                Row {
                    spacing: 6
                    visible: !Theme.compact || r.error.length > 0 || r.tags.length > 0
                    height: visible ? 17 : 0
                    Kind { kind: r.kind; anchors.verticalCenter: parent.verticalCenter }
                    Repeater {
                        model: r.tags
                        delegate: Text {
                            required property string modelData
                            text: "#" + modelData
                            color: Theme.tagColor(modelData)
                            font.family: Theme.fontUi
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    Text {
                        visible: r.error.length > 0
                        text: r.error
                        color: Theme.c.danger
                        font.family: Theme.fontUi
                        font.pixelSize: 12
                        elide: Text.ElideRight
                        width: Math.min(implicitWidth, 320)
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }
        // Size
        Text {
            visible: r.cols.size
            Layout.preferredWidth: 84
            horizontalAlignment: Text.AlignRight
            text: r.size > 0 ? App.fmtBytes(r.size) : "—"
            color: Theme.c.text2
            font.family: Theme.fontMono
            font.pixelSize: Theme.type.num
        }
        // Progress
        Column {
            visible: r.cols.progress
            Layout.preferredWidth: 150
            spacing: 5
            Text {
                text: r.progress >= 1 ? "100%" : (Math.floor(r.progress * 1000) / 10).toFixed(1) + "%"
                color: Theme.c.text2
                font.family: Theme.fontMono
                font.pixelSize: 12
            }
            Prog {
                visible: r.kind === "bt" || r.segs.length === 0
                width: parent.width
                value: r.st === "metadata" || r.st === "probing" ? -1 : r.progress
                fill: r.stCol
                active: r.st === "downloading"
            }
            SegBar {
                visible: r.kind === "http" && r.segs.length > 0
                width: parent.width
                value: r.progress
                size: 1
                connections: r.segs.map(function (g) { return { start: g.s, cursor: g.c, end: g.e } })
            }
        }
        // Status
        Item {
            visible: r.cols.state
            Layout.preferredWidth: 118
            Layout.fillHeight: true
            Badge { state_: r.st; anchors.verticalCenter: parent.verticalCenter }
        }
        // Down / Up
        Text {
            visible: r.cols.down
            Layout.preferredWidth: 86
            horizontalAlignment: Text.AlignRight
            text: r.down > 0 ? App.fmtSpeed(r.down) : "—"
            color: r.down > 0 ? Theme.c.down : Theme.c.text3
            font.family: Theme.fontMono
            font.pixelSize: Theme.type.num
        }
        Text {
            visible: r.cols.up
            Layout.preferredWidth: 80
            horizontalAlignment: Text.AlignRight
            text: r.up > 0 ? App.fmtSpeed(r.up) : "—"
            color: r.up > 0 ? Theme.c.up : Theme.c.text3
            font.family: Theme.fontMono
            font.pixelSize: Theme.type.num
        }
        // ETA
        Text {
            visible: r.cols.eta
            Layout.preferredWidth: 70
            horizontalAlignment: Text.AlignRight
            text: r.st === "downloading" && r.eta >= 0 ? App.fmtEta(r.eta) : r.st === "seeding" ? "×" + r.ratio.toFixed(2) : "—"
            color: Theme.c.text2
            font.family: Theme.fontMono
            font.pixelSize: Theme.type.num
        }
        // Seeds/peers or connections
        Text {
            visible: r.cols.peers
            Layout.preferredWidth: 76
            horizontalAlignment: Text.AlignRight
            text: r.kind === "bt" ? r.seeds + "·" + r.peers : (r.conns > 0 ? qsTr("%n conn", "", r.conns) : "—")
            color: Theme.c.text3
            font.family: Theme.fontMono
            font.pixelSize: 12
        }
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: (m) => r.pressed(m)
        onDoubleClicked: (m) => { if (m.button === Qt.LeftButton) r.doubleClicked() }
        onClicked: (m) => { if (m.button === Qt.RightButton) r.menu(m.x, m.y) }
        drag.target: dragProxy
        drag.threshold: 12
        onReleased: { if (dragProxy.Drag.active) dragProxy.Drag.drop(); dragProxy.x = 0; dragProxy.y = 0 }
    }
    Item {
        id: dragProxy
        Drag.active: ma.drag.active
        Drag.keys: ["rift/ids"]
        Drag.hotSpot.x: 0
        Drag.hotSpot.y: 0
        Rectangle {
            visible: ma.drag.active
            width: dragLabel.implicitWidth + 24
            height: 28
            radius: Theme.radius.sm
            color: Theme.accent
            Text {
                id: dragLabel
                anchors.centerIn: parent
                text: App.selection.length > 1 ? qsTr("%n downloads", "", App.selection.length) : r.name
                color: Theme.onAccent
                font.family: Theme.fontUi
                font.pixelSize: 13
                font.weight: Font.DemiBold
                elide: Text.ElideMiddle
                width: Math.min(implicitWidth, 260)
            }
        }
    }
}
