// Details for the current download. Torrent: Overview · Files · Peers · Trackers · Speed.
// HTTP: Overview · Connections · Speed. The tab underline springs in from 20 % width.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: dp
    property var win
    readonly property var d: App.detail
    readonly property bool has: d.id !== undefined
    readonly property bool bt: d.kind === "bt"
    readonly property var tabs: bt
        ? [["overview", qsTr("Overview")], ["files", qsTr("Files")], ["peers", qsTr("Peers")], ["trackers", qsTr("Trackers")], ["speed", qsTr("Speed")]]
        : [["overview", qsTr("Overview")], ["connections", qsTr("Connections")], ["speed", qsTr("Speed")]]
    readonly property string tab: {
        for (var i = 0; i < tabs.length; ++i) if (tabs[i][0] === App.detailTab) return App.detailTab
        return "overview"
    }
    color: Theme.c.surface

    Rectangle { width: parent.width; height: 1; color: Theme.c.line }

    // Nothing selected
    Column {
        visible: !dp.has
        anchors.centerIn: parent
        spacing: 8
        Icon { anchors.horizontalCenter: parent.horizontalCenter; name: "info"; size: 22; color: Theme.c.text3 }
        Text {
            text: App.selection.length > 1 ? qsTr("%n downloads selected", "", App.selection.length) : qsTr("Select a download to see its details")
            color: Theme.c.text3
            font.family: Theme.fontUi
            font.pixelSize: Theme.type.body
        }
    }

    ColumnLayout {
        visible: dp.has
        anchors.fill: parent
        anchors.topMargin: 1
        spacing: 0

        // Tabs + quick actions
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.size.tab
            Row {
                id: tabRow
                anchors.left: parent.left
                anchors.leftMargin: 10
                height: parent.height
                Repeater {
                    model: dp.tabs
                    delegate: Item {
                        id: t
                        required property var modelData
                        readonly property bool on: dp.tab === modelData[0]
                        width: tl.implicitWidth + 24
                        height: parent.height
                        Text {
                            id: tl
                            anchors.centerIn: parent
                            text: t.modelData[1]
                            color: t.on ? Theme.c.text : tm.containsMouse ? Theme.c.text2 : Theme.c.text3
                            font.family: Theme.fontUi
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }
                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.horizontalCenter: parent.horizontalCenter
                            height: 2
                            radius: 1
                            color: Theme.accent
                            width: t.on ? parent.width - 16 : (parent.width - 16) * 0.2
                            opacity: t.on ? 1 : 0
                            Behavior on width { NumberAnimation { duration: Theme.motion.base; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.spring } }
                            Behavior on opacity { NumberAnimation { duration: Theme.motion.fast } }
                        }
                        MouseArea {
                            id: tm
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: App.detailTab = t.modelData[0]
                        }
                        Accessible.role: Accessible.PageTab
                        Accessible.name: modelData[1]
                    }
                }
            }
            Row {
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2
                Btn { small: true; variant: "ghost"; iconName: "folderOpen"; tip: qsTr("Show in folder"); onClicked: App.openFolder(dp.d.id) }
                Btn { small: true; variant: "ghost"; iconName: "copy"; tip: dp.bt ? qsTr("Copy magnet link") : qsTr("Copy link"); onClicked: App.copyLinks([dp.d.id]) }
                Btn { small: true; variant: "ghost"; iconName: "chevD"; tip: qsTr("Hide details"); kbd: "Ctrl+I"; onClicked: App.settings.set("detailsVisible", false) }
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.c.lineSoft }
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: ["overview", "files", "peers", "trackers", "connections", "speed"].indexOf(dp.tab)

            // ---- Overview
            ScrollView {
                id: ov
                contentWidth: availableWidth
                clip: true
                ColumnLayout {
                    width: ov.availableWidth
                    spacing: 14
                    Item { Layout.preferredHeight: 2 }

                    Banner {
                        Layout.fillWidth: true
                        Layout.leftMargin: Theme.size.pad + 4
                        Layout.rightMargin: Theme.size.pad + 4
                        visible: dp.d.state === "error"
                        kind: "danger"
                        text: dp.d.error || qsTr("This download stopped because of an error.")
                        actionText: qsTr("Retry")
                        closable: false
                        onAction: App.resume([dp.d.id])
                    }
                    Banner {
                        Layout.fillWidth: true
                        Layout.leftMargin: Theme.size.pad + 4
                        Layout.rightMargin: Theme.size.pad + 4
                        visible: dp.d.state === "stalled"
                        kind: "warn"
                        closable: false
                        text: dp.bt
                              ? (dp.d.seedsTotal === 0 ? qsTr("Stalled: no one has the full file right now. It will continue when a seed comes online.")
                                 : App.netState === "firewalled" ? qsTr("Stalled: peers can't reach you. Allowing incoming connections on port %1 usually fixes this.").arg(App.listenPort)
                                 : qsTr("Stalled: connected peers aren't sending data. Rift keeps looking for better ones."))
                              : qsTr("Stalled: the server stopped sending data. Rift retries automatically.")
                        actionText: dp.bt ? qsTr("Ask trackers") : qsTr("Retry now")
                        onAction: dp.bt ? App.reannounce([dp.d.id]) : App.resume([dp.d.id])
                    }

                    // Name + progress
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: Theme.size.pad + 4
                        Layout.rightMargin: Theme.size.pad + 4
                        spacing: 8
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Kind { kind: dp.d.kind || "bt" }
                            Text {
                                Layout.fillWidth: true
                                text: dp.d.name || ""
                                color: Theme.c.text
                                font.family: Theme.fontUi
                                font.pixelSize: Theme.type.title
                                font.weight: Font.Bold
                                elide: Text.ElideMiddle
                            }
                            Badge { state_: dp.d.state || "" }
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            Text {
                                text: (Math.floor((dp.d.progress || 0) * 1000) / 10).toFixed(1) + "%"
                                color: Theme.c.text
                                font.family: Theme.fontMono; font.features: Theme.numFeatures
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                            }
                            Text {
                                Layout.fillWidth: true
                                text: App.fmtBytes(dp.d.done || 0) + qsTr(" of ") + (dp.d.size > 0 ? App.fmtBytes(dp.d.size) : qsTr("unknown size"))
                                color: Theme.c.text3
                                font.family: Theme.fontMono; font.features: Theme.numFeatures
                                font.pixelSize: 12
                            }
                            Text {
                                visible: dp.d.state === "downloading" && dp.d.eta >= 0
                                text: qsTr("%1 left").arg(App.fmtEta(dp.d.eta || 0))
                                color: Theme.c.text2
                                font.family: Theme.fontMono; font.features: Theme.numFeatures
                                font.pixelSize: 12
                            }
                        }
                        PieceStrip {
                            Layout.fillWidth: true
                            visible: App.pieces.length > 0
                            pieces: App.pieces
                            fill: dp.bt ? Theme.accent : Theme.c.success
                        }
                        SegBar {
                            Layout.fillWidth: true
                            visible: !dp.bt && (dp.d.connections || []).length > 0
                            tall: true
                            size: dp.d.size || 0
                            connections: dp.d.connections || []
                        }
                        Text {
                            visible: !dp.bt && dp.d.speedNow > 0 && dp.d.firstConnSpeed > 0 && (dp.d.connections || []).length > 1
                            text: qsTr("%1 connections · %2× faster than one connection")
                                  .arg((dp.d.connections || []).length)
                                  .arg((dp.d.speedNow / dp.d.firstConnSpeed).toFixed(1))
                            color: Theme.c.success
                            font.family: Theme.fontUi
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }
                    }

                    // Three columns of facts
                    GridLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: Theme.size.pad + 4
                        Layout.rightMargin: Theme.size.pad + 4
                        columns: ov.availableWidth > 900 ? 3 : ov.availableWidth > 560 ? 2 : 1
                        columnSpacing: 28
                        rowSpacing: 14

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            spacing: 2
                            Cap { text: qsTr("Transfer"); Layout.bottomMargin: 6 }
                            KV { Layout.fillWidth: true; key: qsTr("Down"); value: App.fmtSpeed(dp.d.down || 0); mono: true; valueColor: Theme.c.down }
                            KV { Layout.fillWidth: true; key: qsTr("Up"); value: dp.bt ? App.fmtSpeed(dp.d.up || 0) : ""; mono: true; valueColor: Theme.c.up }
                            KV { Layout.fillWidth: true; key: qsTr("Downloaded"); value: dp.bt ? App.fmtBytes(dp.d.downloaded || 0) : ""; mono: true }
                            KV { Layout.fillWidth: true; key: qsTr("Uploaded"); value: dp.bt ? App.fmtBytes(dp.d.uploaded || 0) : ""; mono: true }
                            KV { Layout.fillWidth: true; key: qsTr("Ratio"); value: dp.bt ? (dp.d.ratio || 0).toFixed(2) : ""; mono: true }
                            KV { Layout.fillWidth: true; key: qsTr("Retries"); value: !dp.bt && dp.d.retries > 0 ? String(dp.d.retries) : ""; mono: true }
                            KV { Layout.fillWidth: true; key: qsTr("Resumable"); value: !dp.bt ? (dp.d.resumable ? qsTr("Yes") : qsTr("No, the server doesn't support ranges")) : "" }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            spacing: 2
                            Cap { text: dp.bt ? qsTr("Swarm") : qsTr("Source"); Layout.bottomMargin: 6 }
                            KV { Layout.fillWidth: true; key: qsTr("Seeds"); value: dp.bt ? dp.d.seeds + (dp.d.seedsTotal >= 0 ? qsTr(" of ") + dp.d.seedsTotal : "") : ""; mono: true }
                            KV { Layout.fillWidth: true; key: qsTr("Peers"); value: dp.bt ? dp.d.peers + (dp.d.peersTotal >= 0 ? qsTr(" of ") + dp.d.peersTotal : "") : ""; mono: true }
                            KV { Layout.fillWidth: true; key: qsTr("Availability"); value: dp.bt ? (dp.d.availability || 0).toFixed(2) : ""; mono: true }
                            KV { Layout.fillWidth: true; key: qsTr("Tracker"); value: dp.bt ? (dp.d.tracker || "") : ""; mono: true; copyable: true }
                            KV { Layout.fillWidth: true; key: qsTr("URL"); value: !dp.bt ? (dp.d.url || "") : ""; mono: true; copyable: true }
                            KV { Layout.fillWidth: true; key: qsTr("Final URL"); value: !dp.bt && dp.d.finalUrl !== dp.d.url ? (dp.d.finalUrl || "") : ""; mono: true; copyable: true }
                            KV { Layout.fillWidth: true; key: qsTr("Referrer"); value: !dp.bt ? (dp.d.referer || "") : ""; mono: true; copyable: true }
                            KV { Layout.fillWidth: true; key: qsTr("Connections"); value: !dp.bt ? String((dp.d.connections || []).length) : ""; mono: true }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignTop
                            spacing: 2
                            Cap { text: qsTr("Info"); Layout.bottomMargin: 6 }
                            KV { Layout.fillWidth: true; key: qsTr("Saved to"); value: dp.d.savePath || dp.d.path || ""; mono: true; copyable: true }
                            KV { Layout.fillWidth: true; key: qsTr("Category"); value: dp.d.category || "" }
                            KV { Layout.fillWidth: true; key: qsTr("Added"); value: dp.d.added > 0 ? App.fmtDate(dp.d.added) : "" }
                            KV { Layout.fillWidth: true; key: qsTr("Completed"); value: dp.d.completed > 0 ? App.fmtDate(dp.d.completed) : "" }
                            KV { Layout.fillWidth: true; key: qsTr("Info hash v1"); value: dp.d.hashV1 || ""; mono: true; copyable: true }
                            KV { Layout.fillWidth: true; key: qsTr("Info hash v2"); value: dp.d.hashV2 || ""; mono: true; copyable: true }
                            KV { Layout.fillWidth: true; key: qsTr("Pieces"); value: dp.bt && dp.d.numPieces > 0 ? qsTr("%1 × %2").arg(dp.d.numPieces).arg(App.fmtBytes(dp.d.pieceLength)) : ""; mono: true }
                            KV { Layout.fillWidth: true; key: qsTr("Private"); value: dp.bt && dp.d.isPrivate ? qsTr("Yes (DHT and PEX off)") : "" }
                            KV { Layout.fillWidth: true; key: qsTr("Created by"); value: dp.d.creator || "" }
                            KV { Layout.fillWidth: true; key: qsTr("Comment"); value: dp.d.comment || "" }
                            KV { Layout.fillWidth: true; key: qsTr("Checksum"); value: dp.d.checksum || ""; mono: true; copyable: true }
                        }
                    }
                    Item { Layout.preferredHeight: 10 }
                }
            }

            // ---- Files (torrent)
            ListView {
                id: files
                clip: true
                model: App.files
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {}
                header: Rectangle {
                    width: files.width
                    height: 34
                    color: Theme.c.surface
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.size.pad + 4
                        anchors.rightMargin: Theme.size.pad + 4
                        spacing: 12
                        Cap { text: qsTr("File"); Layout.fillWidth: true; Layout.leftMargin: 28 }
                        Cap { text: qsTr("Size"); Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight }
                        Cap { text: qsTr("Progress"); Layout.preferredWidth: 120 }
                        Cap { text: qsTr("Priority"); Layout.preferredWidth: 120 }
                    }
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.c.lineSoft }
                }
                delegate: Rectangle {
                    id: fr
                    required property var modelData
                    width: files.width
                    height: 36
                    color: fm.containsMouse ? Theme.c.hover : "transparent"
                    MouseArea { id: fm; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.size.pad + 4
                        anchors.rightMargin: Theme.size.pad + 4
                        spacing: 12
                        Check {
                            checked: fr.modelData.priority > 0
                            tip: checked ? qsTr("Skip this file") : qsTr("Download this file")
                            onToggled: App.setFilePriority(dp.d.id, fr.modelData.index, checked ? 4 : 0)
                        }
                        Text {
                            Layout.fillWidth: true
                            text: fr.modelData.path
                            elide: Text.ElideMiddle
                            color: fr.modelData.priority > 0 ? Theme.c.text : Theme.c.text3
                            font.family: Theme.fontUi
                            font.pixelSize: 13
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: fr.modelData.progress >= 1 ? Qt.PointingHandCursor : Qt.ArrowCursor
                                onDoubleClicked: if (fr.modelData.progress >= 1) App.openFileAt(dp.d.id, fr.modelData.path)
                            }
                        }
                        Text {
                            Layout.preferredWidth: 80
                            horizontalAlignment: Text.AlignRight
                            text: App.fmtBytes(fr.modelData.size)
                            color: Theme.c.text2
                            font.family: Theme.fontMono; font.features: Theme.numFeatures
                            font.pixelSize: 12
                        }
                        Prog {
                            Layout.preferredWidth: 120
                            value: fr.modelData.progress
                            fill: fr.modelData.progress >= 1 ? Theme.c.success : Theme.accent
                        }
                        Select {
                            Layout.preferredWidth: 120
                            small: true
                            options: [[0, qsTr("Skip")], [1, qsTr("Low")], [4, qsTr("Normal")], [7, qsTr("High")]]
                            value: fr.modelData.priority === 0 ? 0 : fr.modelData.priority < 4 ? 1 : fr.modelData.priority < 7 ? 4 : 7
                            tip: qsTr("Priority for this file")
                            onActivated: (v) => App.setFilePriority(dp.d.id, fr.modelData.index, v)
                        }
                    }
                }
            }

            // ---- Peers
            ListView {
                id: peers
                clip: true
                model: App.peers
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {}
                header: Rectangle {
                    width: peers.width
                    height: 34
                    color: Theme.c.surface
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.size.pad + 4
                        anchors.rightMargin: Theme.size.pad + 4
                        spacing: 12
                        Cap { text: qsTr("Address"); Layout.preferredWidth: 200 }
                        Cap { text: qsTr("Client"); Layout.fillWidth: true }
                        Cap { text: qsTr("Flags"); Layout.preferredWidth: 70 }
                        Cap { text: qsTr("Has"); Layout.preferredWidth: 90 }
                        Cap { text: qsTr("Down"); Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight }
                        Cap { text: qsTr("Up"); Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight }
                    }
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.c.lineSoft }
                }
                delegate: Item {
                    id: pr
                    required property var modelData
                    width: peers.width
                    height: 32
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.size.pad + 4
                        anchors.rightMargin: Theme.size.pad + 4
                        spacing: 12
                        Text { Layout.preferredWidth: 200; text: pr.modelData.ip + ":" + pr.modelData.port; color: Theme.c.text; font.family: Theme.fontMono; font.features: Theme.numFeatures; font.pixelSize: 12; elide: Text.ElideRight }
                        Text { Layout.fillWidth: true; text: pr.modelData.client; color: Theme.c.text2; font.family: Theme.fontUi; font.pixelSize: 13; elide: Text.ElideRight }
                        Row {
                            Layout.preferredWidth: 70
                            spacing: 4
                            Icon { visible: pr.modelData.encrypted; name: "lock"; size: 13; color: Theme.c.success }
                            Icon { visible: pr.modelData.utp; name: "zap"; size: 13; color: Theme.c.info }
                            Icon { visible: pr.modelData.seed; name: "seeds"; size: 13; color: Theme.accent }
                        }
                        Prog { Layout.preferredWidth: 90; value: pr.modelData.progress; fill: Theme.c.violet }
                        Text { Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight; text: pr.modelData.down > 0 ? App.fmtSpeed(pr.modelData.down) : "—"; color: Theme.c.down; font.family: Theme.fontMono; font.features: Theme.numFeatures; font.pixelSize: 12 }
                        Text { Layout.preferredWidth: 80; horizontalAlignment: Text.AlignRight; text: pr.modelData.up > 0 ? App.fmtSpeed(pr.modelData.up) : "—"; color: Theme.c.up; font.family: Theme.fontMono; font.features: Theme.numFeatures; font.pixelSize: 12 }
                    }
                }
                Text {
                    anchors.centerIn: parent
                    visible: peers.count === 0
                    text: qsTr("No peers connected yet")
                    color: Theme.c.text3
                    font.family: Theme.fontUi
                    font.pixelSize: Theme.type.body
                }
            }

            // ---- Trackers
            ColumnLayout {
                spacing: 0
                ListView {
                    id: trackers
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: App.trackers
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Item {
                        id: tr
                        required property var modelData
                        width: trackers.width
                        height: 40
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.size.pad + 4
                            anchors.rightMargin: Theme.size.pad + 4
                            spacing: 12
                            Rectangle {
                                width: 8; height: 8; radius: 4
                                color: tr.modelData.status === "working" ? Theme.c.success : tr.modelData.status === "error" ? Theme.c.danger : Theme.c.text3
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                Text { Layout.fillWidth: true; text: tr.modelData.url; elide: Text.ElideMiddle; color: Theme.c.text; font.family: Theme.fontMono; font.features: Theme.numFeatures; font.pixelSize: 12 }
                                Text { Layout.fillWidth: true; visible: text.length > 0; text: tr.modelData.message; elide: Text.ElideRight; color: Theme.c.text3; font.family: Theme.fontUi; font.pixelSize: 12 }
                            }
                            Text { text: qsTr("tier %1").arg(tr.modelData.tier); color: Theme.c.text3; font.family: Theme.fontMono; font.features: Theme.numFeatures; font.pixelSize: 12 }
                            Text {
                                Layout.preferredWidth: 110
                                horizontalAlignment: Text.AlignRight
                                text: tr.modelData.seeds >= 0 ? tr.modelData.seeds + " · " + tr.modelData.peers : "—"
                                color: Theme.c.text2
                                font.family: Theme.fontMono; font.features: Theme.numFeatures
                                font.pixelSize: 12
                            }
                        }
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.c.lineSoft }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Layout.margins: 12
                    spacing: 8
                    Field {
                        id: trackerUrl
                        Layout.fillWidth: true
                        iconName: "server"
                        placeholderText: qsTr("Add a tracker URL (udp://, http://)")
                        onAccepted: addTr.clicked()
                    }
                    Btn {
                        id: addTr
                        text: qsTr("Add tracker")
                        iconName: "plus"
                        enabled: trackerUrl.text.trim().length > 0
                        tip: qsTr("Add this tracker to the torrent")
                        onClicked: { App.addTracker(dp.d.id, trackerUrl.text.trim()); trackerUrl.text = "" }
                    }
                    Btn {
                        iconName: "refresh"
                        text: qsTr("Reannounce")
                        tip: qsTr("Ask all trackers for peers now")
                        onClicked: App.reannounce([dp.d.id])
                    }
                }
            }

            // ---- Connections (HTTP)
            ListView {
                id: conns
                clip: true
                model: dp.d.connections || []
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {}
                delegate: Item {
                    id: cr
                    required property var modelData
                    required property int index
                    width: conns.width
                    height: 40
                    readonly property real len: Math.max(1, modelData.end - modelData.start)
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.size.pad + 4
                        anchors.rightMargin: Theme.size.pad + 4
                        spacing: 12
                        Rectangle { width: 10; height: 10; radius: 3; color: Theme.connectionPalette[cr.index % Theme.connectionPalette.length] }
                        Text { text: "#" + (cr.index + 1); color: Theme.c.text2; font.family: Theme.fontMono; font.features: Theme.numFeatures; font.pixelSize: 12; Layout.preferredWidth: 30 }
                        Text {
                            Layout.preferredWidth: 220
                            text: App.fmtBytes(cr.modelData.start) + " – " + App.fmtBytes(cr.modelData.end)
                            color: Theme.c.text3; font.family: Theme.fontMono; font.features: Theme.numFeatures; font.pixelSize: 12
                        }
                        Prog {
                            Layout.fillWidth: true
                            value: (cr.modelData.cursor - cr.modelData.start) / cr.len
                            fill: Theme.connectionPalette[cr.index % Theme.connectionPalette.length]
                            active: cr.modelData.speed > 0
                        }
                        Text {
                            Layout.preferredWidth: 90
                            horizontalAlignment: Text.AlignRight
                            text: App.fmtSpeed(cr.modelData.speed)
                            color: Theme.c.down; font.family: Theme.fontMono; font.features: Theme.numFeatures; font.pixelSize: 12
                        }
                    }
                }
                Text {
                    anchors.centerIn: parent
                    visible: conns.count === 0
                    text: dp.d.state === "completed" ? qsTr("Finished. Connections are closed.") : qsTr("No open connections")
                    color: Theme.c.text3
                    font.family: Theme.fontUi
                    font.pixelSize: Theme.type.body
                }
            }

            // ---- Speed
            ColumnLayout {
                spacing: 8
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.size.pad + 4
                    Layout.rightMargin: Theme.size.pad + 4
                    Layout.topMargin: 12
                    spacing: 16
                    Row { spacing: 6; Rectangle { width: 10; height: 3; color: Theme.c.down; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: qsTr("Download"); color: Theme.c.text2; font.family: Theme.fontUi; font.pixelSize: 12 } }
                    Row { spacing: 6; Rectangle { width: 10; height: 3; color: Theme.c.up; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: qsTr("Upload"); color: Theme.c.text2; font.family: Theme.fontUi; font.pixelSize: 12 } }
                    Item { Layout.fillWidth: true }
                    Text { text: qsTr("All downloads, last 2 minutes"); color: Theme.c.text3; font.family: Theme.fontUi; font.pixelSize: 12 }
                }
                SpeedGraph {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.leftMargin: Theme.size.pad + 4
                    Layout.rightMargin: Theme.size.pad + 4
                    Layout.bottomMargin: 12
                    samples: App.speedHistory
                }
            }
        }
    }
}
