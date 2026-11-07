// Add links / magnets (one per line, [1-20] batch patterns allowed). Shows what Rift understood as you type.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

RDialog {
    id: dlg
    property var win
    property var meta: ({})
    property bool batch: false
    property bool folderTouched: false
    readonly property var info: App.classify(links.text)
    readonly property int total: (info.magnets || 0) + (info.urls || 0) + (info.torrents || 0)
    readonly property bool singleHttp: info.urls === 1 && total === 1

    title: batch ? qsTr("Add many links") : qsTr("Add download")
    subtitle: meta.referer ? qsTr("From %1").arg(meta.referer) : qsTr("Links, magnets or .torrent URLs, one per line")
    iconName: "plus"
    dialogWidth: 620

    function show(text, m) {
        meta = m || {}
        batch = !!meta.batch
        links.text = text || ""
        fileName.text = meta.filename || ""
        folderTouched = false
        category.value = ""
        folder.text = App.settings.savePath
        connections.value = App.settings.addConnections > 0 ? App.settings.addConnections : App.settings.httpConnections
        paused.checked = App.settings.startPaused
        sequential.checked = App.settings.sequentialDefault
        checksum.text = ""
        open()
        links.forceActiveFocus()
        links.cursorPosition = links.text.length
    }
    function submit() {
        if (total === 0) return
        var opts = { category: category.value, paused: paused.checked, sequential: sequential.checked,
                     connections: connections.value, referer: meta.referer || "", cookies: meta.cookies || "",
                     userAgent: meta.userAgent || "" }
        if (folderTouched) opts.savePath = folder.text
        if (singleHttp && fileName.text.trim().length) opts.fileName = fileName.text.trim()
        if (singleHttp && checksum.text.trim().length) opts.checksum = checksum.text.trim()
        App.addLinks(links.text, opts)
        close()
    }

    onInfoChanged: {
        if (!folderTouched) folder.text = App.defaultFolderFor(category.value.length ? category.value : (info.category || ""))
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 6
        ScrollView {
            Layout.fillWidth: true
            Layout.preferredHeight: dlg.batch ? 200 : 96
            TextArea {
                id: links
                placeholderText: dlg.batch ? qsTr("Paste many links, one per line.\nhttps://example.com/part[01-20].zip expands to 20 links.")
                                           : qsTr("https://… or magnet:?xt=…")
                wrapMode: TextEdit.WrapAnywhere
                color: Theme.c.text
                placeholderTextColor: Theme.c.text3
                selectionColor: Theme.selection
                font.family: Theme.fontMono
                font.pixelSize: 12.5
                padding: 10
                background: Rectangle {
                    radius: Theme.radius.sm
                    color: Theme.c.input
                    border.width: links.activeFocus ? 1.5 : 1
                    border.color: links.activeFocus ? Theme.accent : Theme.c.line
                }
                Keys.onPressed: (ev) => {
                    if ((ev.key === Qt.Key_Return || ev.key === Qt.Key_Enter) && (ev.modifiers & Qt.ControlModifier)) { dlg.submit(); ev.accepted = true }
                }
                Accessible.name: qsTr("Links")
            }
        }
        // What Rift understood
        Flow {
            Layout.fillWidth: true
            spacing: 8
            visible: links.text.trim().length > 0
            Repeater {
                model: [[dlg.info.urls || 0, qsTr("%n direct link(s)", "", dlg.info.urls || 0), "link", Theme.c.kindHttp],
                        [dlg.info.magnets || 0, qsTr("%n magnet(s)", "", dlg.info.magnets || 0), "magnet", Theme.kindTorrent],
                        [dlg.info.torrents || 0, qsTr("%n torrent file(s)", "", dlg.info.torrents || 0), "fileTorrent", Theme.kindTorrent],
                        [dlg.info.invalid || 0, qsTr("%n line(s) not understood", "", dlg.info.invalid || 0), "alert", Theme.c.warning]]
                delegate: Row {
                    required property var modelData
                    visible: modelData[0] > 0
                    spacing: 5
                    Icon { name: modelData[2]; size: 14; color: modelData[3]; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: modelData[1]; color: Theme.c.text2; font.family: Theme.fontUi; font.pixelSize: 12.5; anchors.verticalCenter: parent.verticalCenter }
                }
            }
        }
        Text {
            Layout.fillWidth: true
            visible: (dlg.info.names || []).length > 0 && dlg.total <= 6 && dlg.total > 0
            text: (dlg.info.names || []).slice(0, 6).join("\n")
            color: Theme.c.text3
            font.family: Theme.fontMono
            font.pixelSize: 12
            elide: Text.ElideMiddle
        }
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 2
        columnSpacing: 14
        rowSpacing: 10

        Text { text: qsTr("Save to"); color: Theme.c.text2; font.family: Theme.fontUi; font.pixelSize: 13.5 }
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Field {
                id: folder
                Layout.fillWidth: true
                iconName: "folder"
                tip: qsTr("Folder for the downloaded files")
                onTextEdited: dlg.folderTouched = true
            }
            Btn {
                text: qsTr("Browse…")
                tip: qsTr("Choose a folder")
                onClicked: dlg.win.pickFolder(function (p) { folder.text = p; dlg.folderTouched = true })
            }
        }

        Text { text: qsTr("Category"); color: Theme.c.text2; font.family: Theme.fontUi; font.pixelSize: 13.5 }
        Select {
            id: category
            Layout.preferredWidth: 220
            tip: qsTr("Auto picks a category from the file type")
            options: {
                var o = [["", dlg.info.category && dlg.info.category !== "Other" ? qsTr("Auto (%1)").arg(dlg.info.category) : qsTr("Auto")]]
                for (var i = 0; i < App.categories.length; ++i) o.push([App.categories[i].name, App.categories[i].name])
                return o
            }
            value: ""
            onActivated: if (!dlg.folderTouched) folder.text = App.defaultFolderFor(value)
        }

        Text { visible: dlg.singleHttp; text: qsTr("File name"); color: Theme.c.text2; font.family: Theme.fontUi; font.pixelSize: 13.5 }
        Field {
            id: fileName
            visible: dlg.singleHttp
            Layout.fillWidth: true
            iconName: "file"
            placeholderText: (dlg.info.names || [""])[0] || qsTr("From the server")
            tip: qsTr("Leave empty to use the name the server sends")
        }

        Text { visible: (dlg.info.urls || 0) > 0; text: qsTr("Connections"); color: Theme.c.text2; font.family: Theme.fontUi; font.pixelSize: 13.5 }
        RowLayout {
            visible: (dlg.info.urls || 0) > 0
            spacing: 10
            NumField {
                id: connections
                from: 1; to: 32
                tip: qsTr("Parallel connections per file. Rift adds them gradually and only while they help.")
            }
            Text { text: qsTr("per file"); color: Theme.c.text3; font.family: Theme.fontUi; font.pixelSize: 13 }
        }

        Text { visible: dlg.singleHttp; text: qsTr("Checksum"); color: Theme.c.text2; font.family: Theme.fontUi; font.pixelSize: 13.5 }
        Field {
            id: checksum
            visible: dlg.singleHttp
            Layout.fillWidth: true
            iconName: "hash"
            placeholderText: qsTr("Optional SHA-256 or MD5 to verify when done")
            tip: qsTr("Rift checks the finished file against this")
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 22
        Check { id: paused; text: qsTr("Add paused"); tip: qsTr("Add to the list without starting") }
        Check {
            id: sequential
            visible: (dlg.info.magnets || 0) + (dlg.info.torrents || 0) > 0
            text: qsTr("Download in order")
            tip: qsTr("Fetch pieces from start to end so media can be previewed early")
        }
    }

    footerItems: [
        Text {
            text: qsTr("Ctrl+Enter to add")
            color: Theme.c.text3
            font.family: Theme.fontUi
            font.pixelSize: 12.5
        },
        Item { Layout.fillWidth: true },
        Btn { text: qsTr("Cancel"); tip: qsTr("Close without adding"); kbd: "Esc"; onClicked: dlg.close() },
        Btn {
            variant: "primary"
            iconName: "download"
            text: dlg.total > 1 ? qsTr("Add %1").arg(dlg.total) : qsTr("Add")
            enabled: dlg.total > 0
            tip: qsTr("Start downloading")
            kbd: "Ctrl+Enter"
            onClicked: dlg.submit()
        }
    ]
}
