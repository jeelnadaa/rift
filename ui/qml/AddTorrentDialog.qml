// Add a .torrent file: preview its files, pick which to download, choose folder and options.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

RDialog {
    id: dlg
    property var win
    property string path: ""
    property var info: ({})
    property var picked: []
    property bool folderTouched: false
    readonly property int pickedCount: { var n = 0; for (var i = 0; i < picked.length; ++i) if (picked[i]) ++n; return n }
    readonly property real pickedSize: {
        var s = 0
        var f = info.files || []
        for (var i = 0; i < f.length; ++i) if (picked[i]) s += f[i].size
        return s
    }

    title: info.ok ? info.name : qsTr("Open torrent")
    subtitle: info.ok ? qsTr("%1 · %n file(s)", "", (info.files || []).length).arg(App.fmtBytes(info.size)) : ""
    iconName: "fileTorrent"
    dialogWidth: 760

    function show(p) {
        path = p
        info = App.inspectTorrent(p)
        var f = info.files || []
        var arr = []
        for (var i = 0; i < f.length; ++i) arr.push(true)
        picked = arr
        folderTouched = false
        category.value = ""
        folder.text = App.defaultFolderFor("")
        paused.checked = App.settings.startPaused
        sequential.checked = App.settings.sequentialDefault
        open()
    }
    function setAll(on) { var a = []; for (var i = 0; i < picked.length; ++i) a.push(on); picked = a }
    function submit() {
        var pr = []
        for (var i = 0; i < picked.length; ++i) pr.push(picked[i] ? 4 : 0)
        var opts = { category: category.value, paused: paused.checked, sequential: sequential.checked, priorities: pr }
        if (folderTouched) opts.savePath = folder.text
        if (App.addTorrentFile(path, opts)) close()
    }

    Banner {
        Layout.fillWidth: true
        visible: !dlg.info.ok
        kind: "danger"
        closable: false
        text: dlg.info.error || qsTr("This file isn't a valid torrent.")
    }
    Banner {
        Layout.fillWidth: true
        visible: !!dlg.info.exists
        kind: "info"
        closable: false
        text: qsTr("This torrent is already in your list. Adding it again merges its trackers.")
    }
    Banner {
        Layout.fillWidth: true
        visible: !!dlg.info.isPrivate
        kind: "warn"
        closable: false
        text: qsTr("Private torrent: peers only come from its trackers (DHT and PEX are off).")
    }

    // File list
    ColumnLayout {
        visible: !!dlg.info.ok
        Layout.fillWidth: true
        spacing: 6
        RowLayout {
            Layout.fillWidth: true
            Check {
                tip: qsTr("Select all or none")
                tristate: true
                checkState: dlg.pickedCount === 0 ? Qt.Unchecked : dlg.pickedCount === dlg.picked.length ? Qt.Checked : Qt.PartiallyChecked
                nextCheckState: function () { return dlg.pickedCount === dlg.picked.length ? Qt.Unchecked : Qt.Checked }
                onClicked: dlg.setAll(dlg.pickedCount !== dlg.picked.length)
            }
            Cap { text: qsTr("Files"); Layout.fillWidth: true }
            Text {
                text: qsTr("%1 of %2 selected").arg(App.fmtBytes(dlg.pickedSize)).arg(App.fmtBytes(dlg.info.size || 0))
                color: Theme.c.text3
                font.family: Theme.fontMono; font.features: Theme.numFeatures
                font.pixelSize: 12
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(260, Math.max(40, (dlg.info.files || []).length * 32 + 8))
            radius: Theme.radius.md
            color: Theme.c.input
            border.color: Theme.c.line
            ListView {
                id: fl
                anchors.fill: parent
                anchors.margins: 4
                clip: true
                model: dlg.info.files || []
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {}
                delegate: Rectangle {
                    id: fd
                    required property var modelData
                    required property int index
                    width: fl.width
                    height: 32
                    radius: Theme.radius.sm - 2
                    color: fdm.containsMouse ? Theme.c.hover : "transparent"
                    MouseArea { id: fdm; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 10
                        spacing: 10
                        Check {
                            checked: !!dlg.picked[fd.index]
                            tip: checked ? qsTr("Skip this file") : qsTr("Download this file")
                            onToggled: { var a = dlg.picked.slice(); a[fd.index] = checked; dlg.picked = a }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: fd.modelData.path
                            elide: Text.ElideMiddle
                            color: dlg.picked[fd.index] ? Theme.c.text : Theme.c.text3
                            font.family: Theme.fontUi
                            font.pixelSize: 13
                        }
                        Text {
                            text: App.fmtBytes(fd.modelData.size)
                            color: Theme.c.text3
                            font.family: Theme.fontMono; font.features: Theme.numFeatures
                            font.pixelSize: 12
                        }
                    }
                }
            }
        }
    }

    GridLayout {
        visible: !!dlg.info.ok
        Layout.fillWidth: true
        columns: 2
        columnSpacing: 14
        rowSpacing: 10
        Text { text: qsTr("Save to"); color: Theme.c.text2; font.family: Theme.fontUi; font.pixelSize: 13 }
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Field { id: folder; Layout.fillWidth: true; iconName: "folder"; tip: qsTr("Folder for the downloaded files"); onTextEdited: dlg.folderTouched = true }
            Btn { text: qsTr("Browse…"); tip: qsTr("Choose a folder"); onClicked: dlg.win.pickFolder(function (p) { folder.text = p; dlg.folderTouched = true }) }
        }
        Text { text: qsTr("Category"); color: Theme.c.text2; font.family: Theme.fontUi; font.pixelSize: 13 }
        Select {
            id: category
            Layout.preferredWidth: 220
            tip: qsTr("Category for this torrent")
            options: {
                var o = [["", qsTr("None")]]
                for (var i = 0; i < App.categories.length; ++i) o.push([App.categories[i].name, App.categories[i].name])
                return o
            }
            value: ""
            onActivated: if (!dlg.folderTouched) folder.text = App.defaultFolderFor(value)
        }
    }
    RowLayout {
        visible: !!dlg.info.ok
        spacing: 22
        Check { id: paused; text: qsTr("Add paused"); tip: qsTr("Add to the list without starting") }
        Check { id: sequential; text: qsTr("Download in order"); tip: qsTr("Fetch pieces from start to end so media can be previewed early") }
    }
    Text {
        visible: !!dlg.info.comment
        Layout.fillWidth: true
        text: dlg.info.comment || ""
        wrapMode: Text.Wrap
        color: Theme.c.text3
        font.family: Theme.fontUi
        font.pixelSize: 12
    }

    footerItems: [
        Text {
            text: App.freeBytes >= 0 ? qsTr("%1 free on disk").arg(App.fmtBytes(App.freeBytes)) : ""
            color: App.freeBytes >= 0 && App.freeBytes < dlg.pickedSize ? Theme.c.danger : Theme.c.text3
            font.family: Theme.fontMono; font.features: Theme.numFeatures
            font.pixelSize: 12
        },
        Item { Layout.fillWidth: true },
        Btn { text: qsTr("Cancel"); tip: qsTr("Close without adding"); kbd: "Esc"; onClicked: dlg.close() },
        Btn {
            variant: "primary"
            iconName: "download"
            text: qsTr("Add torrent")
            enabled: !!dlg.info.ok && dlg.pickedCount > 0
            tip: qsTr("Start downloading the selected files")
            onClicked: dlg.submit()
        }
    ]
}
