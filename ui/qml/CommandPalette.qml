// Command palette (Ctrl+K): every action by name, fuzzy-ish filtering, arrow keys + Enter.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Popup {
    id: cp
    property var win
    property int cur: 0
    readonly property var sel: App.selection

    readonly property var commands: [
        { n: qsTr("Add link or magnet"), i: "link", k: "Ctrl+N", run: function () { win.openAdd("", {}) } },
        { n: qsTr("Open torrent file"), i: "fileTorrent", k: "Ctrl+O", run: function () { win.openTorrentPicker() } },
        { n: qsTr("Add many links"), i: "list", k: "", run: function () { win.openAdd("", { batch: true }) } },
        { n: qsTr("Paste from clipboard"), i: "copy", k: "Ctrl+V", run: function () { win.pasteAdd() } },
        { n: qsTr("Resume selected"), i: "play", k: "Ctrl+R", needs: true, run: function () { App.resume(sel) } },
        { n: qsTr("Pause selected"), i: "pause", k: "Ctrl+P", needs: true, run: function () { App.pause(sel) } },
        { n: qsTr("Remove selected"), i: "trash", k: "Del", needs: true, run: function () { win.openRemove(sel) } },
        { n: qsTr("Force recheck"), i: "recheck", k: "", needs: true, run: function () { App.recheck(sel) } },
        { n: qsTr("Copy links"), i: "copy", k: "Ctrl+C", needs: true, run: function () { App.copyLinks(sel) } },
        { n: qsTr("Show in folder"), i: "folderOpen", k: "", needs: true, run: function () { App.openFolder(sel[0]) } },
        { n: qsTr("Resume all"), i: "play", k: "Ctrl+Shift+R", run: function () { App.resumeAll() } },
        { n: qsTr("Pause all"), i: "pause", k: "Ctrl+Shift+P", run: function () { App.pauseAll() } },
        { n: qsTr("Toggle alternative speed"), i: "turtle", k: "Ctrl+T", run: function () { App.setAltSpeed(!App.settings.altSpeed) } },
        { n: qsTr("Speed limits…"), i: "speed", k: "", run: function () { win.openLimits([]) } },
        { n: qsTr("Select all"), i: "check", k: "Ctrl+A", run: function () { App.select(App.list.allIds(), "") } },
        { n: qsTr("Show: All"), i: "layers", k: "", run: function () { App.list.filter = "all"; win.page = "transfers" } },
        { n: qsTr("Show: Downloading"), i: "download", k: "", run: function () { App.list.filter = "downloading"; win.page = "transfers" } },
        { n: qsTr("Show: Completed"), i: "check", k: "", run: function () { App.list.filter = "completed"; win.page = "transfers" } },
        { n: qsTr("Show: Errors"), i: "alertCircle", k: "", run: function () { App.list.filter = "error"; win.page = "transfers" } },
        { n: qsTr("Statistics"), i: "chart", k: "", run: function () { win.page = "stats" } },
        { n: qsTr("History"), i: "clock", k: "", run: function () { win.page = "history" } },
        { n: qsTr("Toggle sidebar"), i: "window", k: "Ctrl+B", run: function () { App.settings.set("sidebarVisible", !App.settings.sidebarVisible) } },
        { n: qsTr("Toggle details"), i: "info", k: "Ctrl+I", run: function () { App.settings.set("detailsVisible", !App.settings.detailsVisible) } },
        { n: qsTr("Theme: Stone (light)"), i: "sun", k: "", run: function () { App.settings.set("theme", "stone") } },
        { n: qsTr("Theme: Obsidian (dark)"), i: "moon", k: "", run: function () { App.settings.set("theme", "obsidian") } },
        { n: qsTr("Theme: follow system"), i: "window", k: "", run: function () { App.settings.set("theme", "system") } },
        { n: qsTr("Settings"), i: "settings", k: "Ctrl+,", run: function () { win.openSettings("") } },
        { n: qsTr("Browser integration settings"), i: "puzzle", k: "", run: function () { win.openSettings("browser") } },
        { n: qsTr("About Rift"), i: "info", k: "", run: function () { win.openAbout() } },
        { n: qsTr("Quit Rift"), i: "power", k: "Ctrl+Q", run: function () { App.quit() } }
    ]
    readonly property var matches: {
        var q = query.text.trim().toLowerCase()
        var out = []
        for (var i = 0; i < commands.length; ++i) {
            var c = commands[i]
            if (c.needs && sel.length === 0) continue
            if (!q.length) { out.push(c); continue }
            var name = c.n.toLowerCase()
            // all query characters must appear in order
            var j = 0
            for (var k = 0; k < name.length && j < q.length; ++k) if (name[k] === q[j]) ++j
            if (j === q.length) out.push(c)
        }
        return out
    }
    onMatchesChanged: cur = 0

    function show() { query.text = ""; cur = 0; open(); query.forceActiveFocus() }
    function run(i) {
        var c = matches[i]
        if (!c) return
        close()
        c.run()
    }

    parent: Overlay.overlay
    modal: true
    focus: true
    x: Math.round((parent.width - width) / 2)
    y: Math.round(parent.height * 0.14)
    width: Math.min(580, parent.width - 48)
    height: Math.min(460, 60 + list.contentHeight + 12)
    padding: 0
    Overlay.modal: Rectangle { color: Theme.c.overlay }
    background: Rectangle {
        color: Theme.c.raised
        topLeftRadius: Theme.radius.big; bottomRightRadius: Theme.radius.big
        topRightRadius: Theme.radius.xs; bottomLeftRadius: Theme.radius.xs
        border.color: Theme.dark ? Qt.rgba(1, 1, 1, 0.07) : Theme.c.line
    }
    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.motion.fast }
            NumberAnimation { property: "scale"; from: Theme.reducedMotion ? 1 : 0.97; to: 1; duration: Theme.motion.base; easing.type: Easing.OutBack }
        }
    }
    exit: Transition { NumberAnimation { property: "opacity"; to: 0; duration: Theme.motion.fast } }

    contentItem: ColumnLayout {
        spacing: 0
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            Icon { x: 20; anchors.verticalCenter: parent.verticalCenter; name: "cmd"; color: Theme.accent; size: 18 }
            TextInput {
                id: query
                anchors.left: parent.left
                anchors.leftMargin: 50
                anchors.right: parent.right
                anchors.rightMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                color: Theme.c.text
                font.family: Theme.fontUi
                font.pixelSize: 16
                selectionColor: Theme.selection
                Accessible.name: qsTr("Type a command")
                Text {
                    visible: !query.text.length
                    text: qsTr("Type a command…")
                    color: Theme.c.text3
                    font: query.font
                }
                Keys.onDownPressed: cp.cur = Math.min(cp.matches.length - 1, cp.cur + 1)
                Keys.onUpPressed: cp.cur = Math.max(0, cp.cur - 1)
                Keys.onReturnPressed: cp.run(cp.cur)
                Keys.onEnterPressed: cp.run(cp.cur)
                Keys.onEscapePressed: cp.close()
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.c.lineSoft }
        }
        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 6
            clip: true
            model: cp.matches
            currentIndex: cp.cur
            boundsBehavior: Flickable.StopAtBounds
            onCurrentIndexChanged: positionViewAtIndex(currentIndex, ListView.Contain)
            delegate: Rectangle {
                id: row
                required property var modelData
                required property int index
                width: list.width
                height: 38
                radius: Theme.radius.sm
                color: index === cp.cur ? Theme.selection : "transparent"
                Icon { x: 12; anchors.verticalCenter: parent.verticalCenter; name: row.modelData.i; color: row.index === cp.cur ? Theme.accent : Theme.c.text2 }
                Text {
                    x: 40
                    anchors.verticalCenter: parent.verticalCenter
                    text: row.modelData.n
                    color: Theme.c.text
                    font.family: Theme.fontUi
                    font.pixelSize: 14
                    font.weight: row.index === cp.cur ? Font.DemiBold : Font.Normal
                }
                Kbd {
                    visible: row.modelData.k.length > 0
                    anchors.right: parent.right
                    anchors.rightMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    text: row.modelData.k
                }
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: cp.cur = row.index
                    onClicked: cp.run(row.index)
                }
            }
            Text {
                anchors.centerIn: parent
                visible: list.count === 0
                text: qsTr("No command matches")
                color: Theme.c.text3
                font.family: Theme.fontUi
                font.pixelSize: Theme.type.body
            }
        }
    }
}
