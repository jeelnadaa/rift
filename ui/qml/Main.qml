// Rift main window. Wires the views to App (the Controller), owns dialogs, shortcuts, drag-and-drop,
// tray behaviour and the window title. `smokeTest()` is called by `Rift --smoke-test` in CI.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: root
    property string page: "transfers"       // transfers | stats | history | search | rss
    property bool quitting: false
    property var folderCallback: null

    width: App.settings.winW
    height: App.settings.winH
    minimumWidth: 880
    minimumHeight: 540
    visible: !StartMinimized
    color: Theme.c.window
    font.family: Theme.fontUi
    title: App.settings.showSpeedInTitle && (App.downSpeed > 0 || App.upSpeed > 0)
           ? "↓ " + App.fmtSpeed(App.downSpeed) + "  ↑ " + App.fmtSpeed(App.upSpeed) + " · Rift" : "Rift"

    // ---- Theme follows settings ------------------------------------------------------------
    Binding { target: Theme; property: "mode"; value: App.settings.theme }
    Binding { target: Theme; property: "accentName"; value: App.settings.accent }
    Binding { target: Theme; property: "density"; value: App.settings.density }
    Binding { target: Theme; property: "reducedMotion"; value: App.settings.reducedMotion }

    Component.onCompleted: {
        if (App.settings.winX >= 0 && App.settings.winY >= 0) { x = App.settings.winX; y = App.settings.winY }
        if (App.settings.winMax && !StartMinimized) showMaximized()
        App.list.filter = App.settings.lastFilter
        App.list.sortKey = App.settings.sortKey
        App.list.ascending = App.settings.sortAsc
    }
    function saveGeometry() {
        App.settings.set("winMax", visibility === Window.Maximized)
        if (visibility !== Window.Maximized && visibility !== Window.Minimized && visible) {
            App.settings.set("winW", width); App.settings.set("winH", height)
            App.settings.set("winX", x); App.settings.set("winY", y)
        }
    }

    onClosing: (close) => {
        saveGeometry()
        if (!quitting && App.settings.closeToTray && App.trayAvailable) {
            close.accepted = false
            hide()
        } else {
            close.accepted = false
            App.quit()
        }
    }
    onActiveChanged: if (active && App.settings.clipboardWatch) App.checkClipboard()

    // ---- Window API used by the views ------------------------------------------------------
    function bringToFront() {
        if (!visible) show()
        if (visibility === Window.Minimized) showNormal()
        raise()
        requestActivate()
    }
    function openAdd(text, meta) { bringToFront(); addDlg.show(text, meta) }
    function openTorrentPicker() { torrentPicker.open() }
    function pasteAdd() {
        var t = clip.read()
        openAdd(t, {})
    }
    function openRemove(ids) { if (ids.length) removeDlg.show(ids) }
    function openLimits(ids) { limitsDlg.show(ids) }
    function prompt(t, label, value, cb) { promptDlg.ask(t, label, value, cb) }
    function openSettings(section) { settingsDlg.show(section) }
    function openAbout() { aboutDlg.open() }
    function openExtension() { extDlg.show() }
    function openPalette() { palette.show() }
    function openContextMenu() { if (App.selection.length) ctxMenu.popup() }
    function pickFolder(cb) { folderCallback = cb; folderPicker.open() }
    function saveTorrent(id, name) { torrentSaver.targetId = id; torrentSaver.selectedFile = App.pathToUrl(App.settings.savePath + "/" + name + ".torrent"); torrentSaver.open() }
    function toast(text, kind, action) { toasts.show(text, kind, action) }

    // Clipboard helper (TextEdit gives us paste without extra C++).
    TextEdit {
        id: clip
        visible: false
        function read() { clear(); paste(); var t = getText(0, length); clear(); return t }
    }

    // ---- App signals ----------------------------------------------------------------------
    Connections {
        target: App
        function onRequestAdd(text, meta) { root.openAdd(text, meta) }
        function onRequestAddTorrent(path) { root.bringToFront(); torrentDlg.show(path) }
        function onShowWindow() { root.bringToFront() }
        function onToast(text, kind, action) { toasts.show(text, kind, action) }
        function onQuitRequested() { root.quitting = true; root.saveGeometry() }
    }

    // ---- Layout ---------------------------------------------------------------------------
    RowLayout {
        anchors.fill: parent
        spacing: 0

        Sidebar {
            id: sidebar
            win: root
            visible: App.settings.sidebarVisible
            Layout.fillHeight: true
            Layout.preferredWidth: Math.max(184, Math.min(360, App.settings.sidebarWidth))
            // resize handle
            MouseArea {
                anchors.right: parent.right
                width: 6
                height: parent.height
                cursorShape: Qt.SplitHCursor
                hoverEnabled: true
                property real startX: 0
                property real startW: 0
                onPressed: (m) => { startX = mapToItem(null, m.x, 0).x; startW = sidebar.width }
                onPositionChanged: (m) => {
                    if (!pressed) return
                    var w = Math.max(184, Math.min(360, startW + mapToItem(null, m.x, 0).x - startX))
                    App.settings.set("sidebarWidth", Math.round(w))
                }
                Rectangle { anchors.fill: parent; color: Theme.accent; opacity: parent.containsMouse || parent.pressed ? 0.35 : 0 }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Toolbar {
                id: toolbar
                win: root
                Layout.fillWidth: true
                visible: root.page === "transfers"
            }
            Rectangle {
                visible: root.page !== "transfers"
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.size.toolbar
                color: Theme.c.surface
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.c.line }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    Btn {
                        iconName: "chevL"; variant: "ghost"; text: qsTr("Transfers")
                        tip: qsTr("Back to the download list")
                        onClicked: root.page = "transfers"
                    }
                    Item { Layout.fillWidth: true }
                    Btn { iconName: "cmd"; variant: "ghost"; tip: qsTr("Command palette"); kbd: "Ctrl+K"; onClicked: root.openPalette() }
                    Btn { iconName: "settings"; variant: "ghost"; tip: qsTr("Settings"); kbd: "Ctrl+,"; onClicked: root.openSettings("") }
                }
            }

            Banner {
                Layout.fillWidth: true
                strip: true
                visible: App.clipboardOffer.length > 0 && root.page === "transfers"
                kind: "info"
                iconName: App.clipboardOffer.indexOf("magnet:") === 0 ? "magnet" : "link"
                text: qsTr("You copied a link: %1").arg(App.clipboardOffer.length > 90 ? App.clipboardOffer.slice(0, 90) + "…" : App.clipboardOffer)
                actionText: qsTr("Download")
                onAction: { var t = App.clipboardOffer; App.dismissClipboard(); root.openAdd(t, {}) }
                onClosed: App.dismissClipboard()
            }
            Banner {
                Layout.fillWidth: true
                strip: true
                visible: App.freeBytes >= 0 && App.settings.minFreeSpaceGB > 0 && App.freeBytes < App.settings.minFreeSpaceGB * 1073741824 && root.page === "transfers"
                kind: "danger"
                closable: false
                text: qsTr("Low disk space: %1 left. Downloads pause below %2 GB.").arg(App.fmtBytes(App.freeBytes)).arg(App.settings.minFreeSpaceGB)
                actionText: qsTr("Settings")
                onAction: root.openSettings("downloads")
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: ["transfers", "stats", "history", "search", "rss"].indexOf(root.page)

                ColumnLayout {
                    spacing: 0
                    BulkBar { win: root; Layout.fillWidth: true }
                    SplitView {
                        id: split
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        orientation: Qt.Vertical
                        handle: Rectangle {
                            implicitHeight: 5
                            color: SplitHandle.hovered || SplitHandle.pressed ? Theme.soft(Theme.accent, 0.4) : "transparent"
                            Behavior on color { ColorAnimation { duration: Theme.motion.fast } }
                        }
                        DownloadList {
                            id: list
                            win: root
                            focus: true
                            SplitView.fillHeight: true
                            SplitView.minimumHeight: 160
                        }
                        DetailsPanel {
                            id: details
                            win: root
                            visible: App.settings.detailsVisible
                            SplitView.preferredHeight: App.settings.detailsHeight
                            SplitView.minimumHeight: 180
                            onHeightChanged: if (split.resizing) saveDetails.restart()
                            Timer { id: saveDetails; interval: 400; onTriggered: App.settings.set("detailsHeight", Math.round(details.height)) }
                        }
                    }
                }
                StatsView {}
                HistoryView { win: root }
                ComingSoon {
                    title: qsTr("Search")
                    iconName: "search"
                    body: qsTr("Search torrent sites from inside Rift with plugins you choose.")
                }
                ComingSoon {
                    title: qsTr("RSS")
                    iconName: "rss"
                    body: qsTr("Follow feeds and download new episodes automatically with rules.")
                }
            }

            StatusBar { win: root; Layout.fillWidth: true }
        }
    }

    // Drop anything: links, magnets, .torrent files, text
    DropArea {
        id: drop
        anchors.fill: parent
        keys: ["text/uri-list", "text/plain"]
        onDropped: (ev) => {
            var torrents = [], lines = []
            if (ev.hasUrls) {
                for (var i = 0; i < ev.urls.length; ++i) {
                    var u = ev.urls[i].toString()
                    if (u.indexOf("file:") === 0) {
                        if (u.toLowerCase().endsWith(".torrent")) torrents.push(u)
                    } else lines.push(u)
                }
            } else if (ev.hasText) lines.push(ev.text)
            if (torrents.length === 1 && lines.length === 0) torrentDlg.show(torrents[0])
            else if (torrents.length > 0) App.addLinks(torrents.map(function (t) { return App.urlToPath(t) }).join("\n"), {})
            if (lines.length) root.openAdd(lines.join("\n"), {})
            ev.acceptProposedAction()
        }
    }
    Rectangle {
        anchors.fill: parent
        anchors.margins: 10
        visible: drop.containsDrag
        color: Theme.soft(Theme.accent, 0.08)
        border.width: 2
        border.color: Theme.accent
        topLeftRadius: Theme.radius.big; bottomRightRadius: Theme.radius.big
        topRightRadius: Theme.radius.xs; bottomLeftRadius: Theme.radius.xs
        Column {
            anchors.centerIn: parent
            spacing: 10
            Icon { anchors.horizontalCenter: parent.horizontalCenter; name: "download"; size: 34; color: Theme.accent }
            Text {
                text: qsTr("Drop to add")
                color: Theme.accent
                font.family: Theme.fontUi
                font.pixelSize: Theme.type.section
                font.weight: Font.Bold
                font.variableAxes: Theme.displayAxes
            }
        }
    }

    Toasts {
        id: toasts
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 18
        anchors.bottomMargin: Theme.size.statusbar + 14
        height: 1
        z: 100
        onUndo: App.undoRemove()
    }

    // ---- Dialogs --------------------------------------------------------------------------
    AddLinkDialog { id: addDlg; win: root }
    AddTorrentDialog { id: torrentDlg; win: root }
    RemoveDialog { id: removeDlg }
    LimitsDialog { id: limitsDlg }
    TextPrompt { id: promptDlg }
    SettingsDialog { id: settingsDlg; win: root }
    CommandPalette { id: palette; win: root }
    AboutDialog { id: aboutDlg }
    ExtensionDialog { id: extDlg }
    RowMenu { id: ctxMenu; win: root }

    FileDialog {
        id: torrentPicker
        title: qsTr("Open torrent")
        nameFilters: [qsTr("Torrent files (*.torrent)"), qsTr("All files (*)")]
        fileMode: FileDialog.OpenFiles
        onAccepted: {
            if (selectedFiles.length === 1) torrentDlg.show(selectedFiles[0].toString())
            else App.addLinks(selectedFiles.map(function (f) { return App.urlToPath(f) }).join("\n"), {})
        }
    }
    FileDialog {
        id: torrentSaver
        property string targetId: ""
        title: qsTr("Export .torrent")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Torrent files (*.torrent)")]
        onAccepted: {
            if (App.exportTorrent(targetId, App.urlToPath(selectedFile))) toasts.show(qsTr("Torrent exported"), "ok", "")
            else toasts.show(qsTr("Could not export the torrent"), "danger", "")
        }
    }
    FolderDialog {
        id: folderPicker
        title: qsTr("Choose a folder")
        currentFolder: App.pathToUrl(App.settings.savePath)
        onAccepted: if (root.folderCallback) root.folderCallback(App.urlToPath(selectedFolder))
    }

    // ---- Shortcuts ------------------------------------------------------------------------
    readonly property bool dialogOpen: addDlg.visible || torrentDlg.visible || removeDlg.visible || limitsDlg.visible
                                       || promptDlg.visible || settingsDlg.visible || palette.visible || aboutDlg.visible
    Shortcut { sequences: [StandardKey.New]; onActivated: root.openAdd("", {}) }
    Shortcut { sequences: [StandardKey.Open]; onActivated: root.openTorrentPicker() }
    Shortcut { sequence: "Ctrl+K"; onActivated: root.openPalette() }
    Shortcut { sequence: "Ctrl+,"; onActivated: root.openSettings("") }
    Shortcut { sequence: "Ctrl+F"; enabled: root.page === "transfers"; onActivated: toolbar.searchField.forceActiveFocus() }
    Shortcut { sequence: "Ctrl+B"; onActivated: App.settings.set("sidebarVisible", !App.settings.sidebarVisible) }
    Shortcut { sequence: "Ctrl+I"; onActivated: App.settings.set("detailsVisible", !App.settings.detailsVisible) }
    Shortcut { sequence: "Ctrl+T"; onActivated: App.setAltSpeed(!App.settings.altSpeed) }
    Shortcut { sequence: "Ctrl+R"; enabled: !root.dialogOpen; onActivated: App.resume(App.selection) }
    Shortcut { sequence: "Ctrl+P"; enabled: !root.dialogOpen; onActivated: App.pause(App.selection) }
    Shortcut { sequence: "Ctrl+Shift+R"; onActivated: App.resumeAll() }
    Shortcut { sequence: "Ctrl+Shift+P"; onActivated: App.pauseAll() }
    Shortcut { sequence: "Ctrl+Up"; enabled: !root.dialogOpen; onActivated: App.queueMove(App.selection, "up") }
    Shortcut { sequence: "Ctrl+Down"; enabled: !root.dialogOpen; onActivated: App.queueMove(App.selection, "down") }
    Shortcut { sequences: [StandardKey.Delete]; enabled: !root.dialogOpen && list.activeFocus; onActivated: root.openRemove(App.selection) }
    Shortcut { sequence: "Ctrl+Z"; enabled: !root.dialogOpen; onActivated: App.undoRemove() }
    Shortcut {
        sequences: [StandardKey.Paste]
        enabled: !root.dialogOpen && !toolbar.searchField.activeFocus
        onActivated: root.pasteAdd()
    }
    Shortcut {
        sequences: [StandardKey.Copy]
        enabled: !root.dialogOpen && list.activeFocus && App.selection.length > 0
        onActivated: App.copyLinks(App.selection)
    }
    Shortcut { sequences: [StandardKey.Quit]; context: Qt.ApplicationShortcut; onActivated: App.quit() }

    // ---- CI smoke test: open every surface once, both themes. Warnings fail the run. ----------
    function smokeTest() {
        var pages = ["stats", "history", "search", "rss", "transfers"]
        for (var i = 0; i < pages.length; ++i) page = pages[i]
        App.settings.set("theme", "obsidian")
        App.settings.set("density", "comfortable")
        addDlg.show("https://example.com/file[1-3].zip\nmagnet:?xt=urn:btih:0123456789abcdef0123456789abcdef01234567&dn=Test", {})
        addDlg.close()
        limitsDlg.show([]); limitsDlg.close()
        removeDlg.show(["h:0"]); removeDlg.close()
        promptDlg.ask("Test", "Name", "x", null); promptDlg.close()
        var secs = settingsDlg.sections
        settingsDlg.show("")
        for (var s = 0; s < secs.length; ++s) settingsDlg.section = secs[s][0]
        settingsDlg.close()
        palette.show(); palette.close()
        aboutDlg.open(); aboutDlg.close()
        extDlg.show(); extDlg.browser = "firefox"; extDlg.close(); extDlg.browser = "chromium"
        toasts.show("Smoke test", "ok", "undo")
        App.settings.set("theme", "stone")
        App.settings.set("density", "compact")
        App.settings.set("theme", "system")
        console.info("smoke-test: ui ok")
        return true
    }
}
