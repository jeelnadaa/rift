// Choose which files of a torrent to download. Opens right after a magnet or .torrent is added (setting
// "pickFiles"); nothing downloads until Download is pressed. For magnets the file list arrives from peers, so
// the dialog waits for it. "Later" keeps the torrent waiting (its row says "Choose files"); "Don't add" removes it.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

RDialog {
    id: dlg
    property string rowId: ""
    property var info: ({})
    property var picked: []
    property string filter: ""
    readonly property var files: info.files || []
    readonly property bool ready: !!info.ready
    readonly property int pickedCount: { var n = 0; for (var i = 0; i < picked.length; ++i) if (picked[i]) ++n; return n }
    readonly property real pickedSize: {
        var s = 0
        for (var i = 0; i < files.length; ++i) if (picked[i]) s += files[i].size
        return s
    }
    // Indices shown in the list (all, or those matching the filter).
    readonly property var shown: {
        var out = [], f = filter.trim().toLowerCase()
        for (var i = 0; i < files.length; ++i) if (!f || files[i].path.toLowerCase().indexOf(f) >= 0) out.push(i)
        return out
    }

    title: info.name || qsTr("Choose files")
    subtitle: !ready ? qsTr("Getting the file list from peers…")
            : files.length === 1 ? qsTr("%1 · 1 file").arg(App.fmtBytes(info.size || 0))
            : qsTr("%1 · %2 files").arg(App.fmtBytes(info.size || 0)).arg(files.length)
    iconName: "fileTorrent"
    dialogWidth: 760

    function show(id) {
        rowId = id
        info = ({})
        picked = []
        filter = ""
        refresh()
        open()
    }
    function refresh() {
        var i2 = App.pickInfo(rowId)
        var wasReady = ready
        info = i2
        if (i2.ready && !wasReady) {
            var a = []
            for (var i = 0; i < (i2.files || []).length; ++i) a.push(true)
            picked = a
        }
    }
    function setShown(on) {
        var a = picked.slice()
        for (var k = 0; k < shown.length; ++k) a[shown[k]] = on
        picked = a
    }
    function shownPicked() { var n = 0; for (var k = 0; k < shown.length; ++k) if (picked[shown[k]]) ++n; return n }
    function submit() {
        var pr = []
        for (var i = 0; i < picked.length; ++i) pr.push(picked[i] ? 4 : 0)
        if (App.confirmPick(rowId, pr)) close()
    }

    // Magnets: poll until the file list has arrived.
    Connections {
        target: App
        enabled: dlg.visible && !dlg.ready
        function onTicked() { dlg.refresh() }
    }

    // Waiting for metadata
    ColumnLayout {
        visible: !dlg.ready
        Layout.fillWidth: true
        Layout.topMargin: 18
        Layout.bottomMargin: 18
        spacing: 12
        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 34; implicitHeight: 34
            running: dlg.visible && !dlg.ready
            Accessible.name: qsTr("Getting the file list")
        }
        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: qsTr("A magnet link only carries the torrent's fingerprint. Rift is asking peers for the file list; nothing is downloaded until you choose.")
            color: Theme.c.text3
            font.family: Theme.fontUi
            font.pixelSize: 13
        }
    }

    // File list
    ColumnLayout {
        visible: dlg.ready
        Layout.fillWidth: true
        spacing: 8
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Check {
                tip: qsTr("Select all or none")
                Accessible.name: qsTr("Select all files")
                tristate: true
                checkState: dlg.shownPicked() === 0 ? Qt.Unchecked : dlg.shownPicked() === dlg.shown.length ? Qt.Checked : Qt.PartiallyChecked
                nextCheckState: function () { return dlg.shownPicked() === dlg.shown.length ? Qt.Unchecked : Qt.Checked }
                onClicked: dlg.setShown(dlg.shownPicked() !== dlg.shown.length)
            }
            Cap { text: qsTr("Files") }
            Field {
                Layout.fillWidth: true
                visible: dlg.files.length > 8
                iconName: "search"
                placeholderText: qsTr("Filter files, e.g. .mkv")
                tip: qsTr("Show only files whose path contains this text")
                onTextEdited: dlg.filter = text
            }
            Item { Layout.fillWidth: true; visible: dlg.files.length <= 8 }
            Text {
                text: qsTr("%1 of %2 selected").arg(App.fmtBytes(dlg.pickedSize)).arg(App.fmtBytes(dlg.info.size || 0))
                color: Theme.c.text3
                font.family: Theme.fontMono; font.features: Theme.numFeatures
                font.pixelSize: 12
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(300, Math.max(40, dlg.shown.length * 32 + 8))
            radius: Theme.radius.md
            color: Theme.c.input
            border.color: Theme.c.line
            ListView {
                id: fl
                anchors.fill: parent
                anchors.margins: 4
                clip: true
                model: dlg.shown
                boundsBehavior: Flickable.StopAtBounds
                ScrollBar.vertical: ScrollBar {}
                delegate: Rectangle {
                    id: fd
                    required property var modelData   // file index
                    readonly property var file: dlg.files[modelData] || ({})
                    readonly property bool on: !!dlg.picked[modelData]
                    width: fl.width
                    height: 32
                    radius: Theme.radius.sm - 2
                    color: fdm.containsMouse ? Theme.c.hover : "transparent"
                    Behavior on color { ColorAnimation { duration: Theme.motion.fast } }
                    MouseArea {
                        id: fdm
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: { var a = dlg.picked.slice(); a[fd.modelData] = !fd.on; dlg.picked = a }
                    }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 10
                        spacing: 10
                        Check {
                            checked: fd.on
                            tip: checked ? qsTr("Skip this file") : qsTr("Download this file")
                            Accessible.name: fd.file.path || ""
                            onToggled: { var a = dlg.picked.slice(); a[fd.modelData] = checked; dlg.picked = a }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: fd.file.path || ""
                            elide: Text.ElideMiddle
                            color: fd.on ? Theme.c.text : Theme.c.text3
                            font.family: Theme.fontUi
                            font.pixelSize: 13
                        }
                        Text {
                            text: App.fmtBytes(fd.file.size || 0)
                            color: Theme.c.text3
                            font.family: Theme.fontMono; font.features: Theme.numFeatures
                            font.pixelSize: 12
                        }
                    }
                }
            }
            Text {
                anchors.centerIn: parent
                visible: dlg.shown.length === 0
                text: qsTr("No file matches “%1”").arg(dlg.filter)
                color: Theme.c.text3
                font.family: Theme.fontUi
                font.pixelSize: 13
            }
        }
        Text {
            visible: !!dlg.info.savePath
            Layout.fillWidth: true
            text: qsTr("Saving to %1").arg(dlg.info.savePath || "")
            elide: Text.ElideMiddle
            color: Theme.c.text3
            font.family: Theme.fontUi
            font.pixelSize: 12
        }
    }

    footerItems: [
        Text {
            text: dlg.ready && App.freeBytes >= 0 ? qsTr("%1 free on disk").arg(App.fmtBytes(App.freeBytes)) : ""
            color: App.freeBytes >= 0 && App.freeBytes < dlg.pickedSize ? Theme.c.danger : Theme.c.text3
            font.family: Theme.fontMono; font.features: Theme.numFeatures
            font.pixelSize: 12
        },
        Item { Layout.fillWidth: true },
        Btn {
            text: qsTr("Don't add")
            tip: qsTr("Remove this torrent from the list. Nothing has been downloaded.")
            onClicked: { App.cancelPick(dlg.rowId); dlg.close() }
        },
        Btn {
            text: qsTr("Later")
            tip: qsTr("Keep it waiting. Double-click its row to choose files later.")
            kbd: "Esc"
            onClicked: dlg.close()
        },
        Btn {
            variant: "primary"
            iconName: "download"
            text: dlg.ready && dlg.pickedCount < dlg.files.length ? qsTr("Download %1").arg(App.fmtBytes(dlg.pickedSize)) : qsTr("Download")
            enabled: dlg.ready && dlg.pickedCount > 0
            tip: dlg.pickedCount > 0 ? qsTr("Start downloading the selected files") : qsTr("Select at least one file")
            onClicked: dlg.submit()
        }
    ]
}
