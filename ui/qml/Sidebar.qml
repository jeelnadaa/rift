// Sidebar: section tabs (Transfers · Search · RSS · Stats), then Status, Type, Categories, Tags, Sites.
// Rows dragged from the list can be dropped onto categories and tags.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: side
    property var win
    property string page: "transfers"
    readonly property string filter: App.list.filter
    readonly property var counts: App.counts
    color: Theme.c.sidebar

    function n(key) { return counts[key] === undefined ? 0 : counts[key] }
    function pick(key) {
        App.list.filter = key
        App.settings.set("lastFilter", key)
        win.page = "transfers"
    }

    Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.c.line }

    ColumnLayout {
        anchors.fill: parent
        anchors.rightMargin: 1
        spacing: 0

        // Brand
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            Layout.leftMargin: 18
            Layout.rightMargin: 12
            spacing: 10
            Logo { size: 22; Layout.alignment: Qt.AlignVCenter }
            Text {
                text: "Rift"
                color: Theme.c.text
                font.family: Theme.fontUi
                font.pixelSize: 19
                font.weight: Font.ExtraBold
                font.letterSpacing: -0.5
                font.variableAxes: Theme.displayAxes
            }
            Item { Layout.fillWidth: true }
            Btn {
                iconName: "chevL"
                variant: "ghost"
                small: true
                tip: qsTr("Hide sidebar")
                kbd: "Ctrl+B"
                onClicked: App.settings.set("sidebarVisible", false)
            }
        }

        // Section tabs
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            Layout.preferredHeight: 34
            radius: Theme.radius.sm + 1
            color: Theme.c.sunken
            RowLayout {
                anchors.fill: parent
                anchors.margins: 3
                spacing: 2
                Repeater {
                    model: [["transfers", qsTr("Transfers"), "download"], ["search", qsTr("Search"), "search"],
                            ["rss", qsTr("RSS"), "rss"], ["stats", qsTr("Stats"), "chart"]]
                    delegate: Rectangle {
                        id: tab
                        required property var modelData
                        readonly property bool on: side.win.page === modelData[0]
                                                   || (modelData[0] === "transfers" && side.win.page === "history")
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: Theme.radius.sm - 1
                        color: on ? Theme.c.raised : tm.containsMouse ? Theme.c.hover : "transparent"
                        border.width: on ? 1 : 0
                        border.color: Theme.c.line
                        scale: tm.pressed ? 0.95 : 1
                        Behavior on scale { NumberAnimation { duration: Theme.motion.press } }
                        Behavior on color { ColorAnimation { duration: Theme.motion.fast } }
                        transform: Translate { y: tm.containsMouse && !tm.pressed && !tab.on ? -1 : 0 }
                        Icon {
                            anchors.centerIn: parent
                            name: tab.modelData[2]
                            color: tab.on ? Theme.accent : Theme.c.text3
                            size: 15
                        }
                        MouseArea {
                            id: tm
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: side.win.page = tab.modelData[0]
                        }
                        Accessible.role: Accessible.PageTab
                        Accessible.name: modelData[1]
                        Tip { visible: tm.containsMouse; text: tab.modelData[1] }
                    }
                }
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: 8
            contentWidth: availableWidth
            clip: true

            ColumnLayout {
                width: parent.width
                spacing: 1

                Cap { text: qsTr("Status"); Layout.leftMargin: 20; Layout.topMargin: 8; Layout.bottomMargin: 4 }
                Repeater {
                    model: [["all", qsTr("All"), "layers", qsTr("Everything in Rift")],
                            ["downloading", qsTr("Downloading"), "download", qsTr("Transferring data now or waiting for peers")],
                            ["seeding", qsTr("Seeding"), "upload", qsTr("Finished torrents sharing with others")],
                            ["completed", qsTr("Completed"), "check", qsTr("Finished downloads")],
                            ["paused", qsTr("Paused"), "pause", qsTr("Paused by you")],
                            ["active", qsTr("Active"), "bolt", qsTr("Moving data right now")],
                            ["inactive", qsTr("Inactive"), "clock", qsTr("Not moving data right now")],
                            ["stalled", qsTr("Stalled"), "alert", qsTr("Downloading but nothing is arriving")],
                            ["checking", qsTr("Checking"), "recheck", qsTr("Verifying data on disk")],
                            ["error", qsTr("Errors"), "alertCircle", qsTr("Something went wrong; select one to see why")]]
                    delegate: SideItem {
                        required property var modelData
                        Layout.fillWidth: true
                        visible: modelData[0] === "all" || modelData[0] === "downloading" || modelData[0] === "completed"
                                 || modelData[0] === "seeding" || modelData[0] === "paused" || side.n(modelData[0]) > 0
                        iconName: modelData[2]
                        text: modelData[1]
                        tip: modelData[3]
                        count: side.n(modelData[0])
                        selected: side.win.page === "transfers" && side.filter === modelData[0]
                        onClicked: side.pick(modelData[0])
                    }
                }

                Cap { text: qsTr("Type"); Layout.leftMargin: 20; Layout.topMargin: 14; Layout.bottomMargin: 4 }
                SideItem {
                    Layout.fillWidth: true
                    iconName: "magnet"; text: qsTr("Torrents"); tip: qsTr("BitTorrent downloads and magnet links")
                    count: side.n("k:bt"); selected: side.win.page === "transfers" && side.filter === "k:bt"
                    onClicked: side.pick("k:bt")
                }
                SideItem {
                    Layout.fillWidth: true
                    iconName: "link"; text: qsTr("Direct links"); tip: qsTr("HTTP, HTTPS and FTP downloads")
                    count: side.n("k:http"); selected: side.win.page === "transfers" && side.filter === "k:http"
                    onClicked: side.pick("k:http")
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 14
                    Layout.leftMargin: 20
                    Layout.rightMargin: 12
                    Cap { text: qsTr("Categories"); Layout.fillWidth: true }
                    Btn {
                        iconName: "plus"; variant: "ghost"; small: true
                        implicitHeight: 22
                        tip: qsTr("New category")
                        onClicked: side.win.prompt(qsTr("New category"), qsTr("Name"), "", function (v) { if (v.length) App.saveCategory(v, "") })
                    }
                }
                Repeater {
                    model: App.categories
                    delegate: SideItem {
                        id: catItem
                        required property var modelData
                        Layout.fillWidth: true
                        dot: Theme.tagColor(modelData.name)
                        text: modelData.name
                        tip: qsTr("Show downloads in %1. Drop downloads here to move them.").arg(modelData.name)
                        count: modelData.count
                        selected: side.win.page === "transfers" && side.filter === "cat:" + modelData.name
                        dropTarget: catDrop.containsDrag
                        onClicked: side.pick("cat:" + modelData.name)
                        onRightClicked: (x, y) => { catMenu.target = modelData.name; catMenu.popup() }
                        DropArea {
                            id: catDrop
                            anchors.fill: parent
                            keys: ["rift/ids"]
                            onDropped: (drop) => App.setCategory(App.selection, catItem.modelData.name)
                        }
                    }
                }

                Cap {
                    visible: App.tags.length > 0
                    text: qsTr("Tags"); Layout.leftMargin: 20; Layout.topMargin: 14; Layout.bottomMargin: 4
                }
                Repeater {
                    model: App.tags
                    delegate: SideItem {
                        id: tagItem
                        required property var modelData
                        Layout.fillWidth: true
                        dot: Theme.tagColor(modelData.name)
                        text: "#" + modelData.name
                        tip: qsTr("Show downloads tagged %1. Drop downloads here to tag them.").arg(modelData.name)
                        count: modelData.count
                        selected: side.win.page === "transfers" && side.filter === "tag:" + modelData.name
                        dropTarget: tagDrop.containsDrag
                        onClicked: side.pick("tag:" + modelData.name)
                        DropArea {
                            id: tagDrop
                            anchors.fill: parent
                            keys: ["rift/ids"]
                            onDropped: (drop) => App.toggleTag(App.selection, tagItem.modelData.name)
                        }
                    }
                }

                Cap {
                    visible: App.sites.length > 0
                    text: qsTr("Trackers & sites"); Layout.leftMargin: 20; Layout.topMargin: 14; Layout.bottomMargin: 4
                }
                Repeater {
                    model: App.sites
                    delegate: SideItem {
                        required property var modelData
                        Layout.fillWidth: true
                        iconName: "globe"
                        text: modelData.name
                        tip: qsTr("Downloads from %1").arg(modelData.name)
                        count: modelData.count
                        selected: side.win.page === "transfers" && side.filter === "site:" + modelData.name
                        onClicked: side.pick("site:" + modelData.name)
                    }
                }

                Cap { text: qsTr("Library"); Layout.leftMargin: 20; Layout.topMargin: 14; Layout.bottomMargin: 4 }
                SideItem {
                    Layout.fillWidth: true
                    iconName: "clock"; text: qsTr("History"); tip: qsTr("Everything you finished, even after removing it from the list")
                    selected: side.win.page === "history"
                    onClicked: side.win.page = "history"
                }
                Item { Layout.preferredHeight: 12 }
            }
        }

        // Footer: overall speed sparkline + settings
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.c.lineSoft
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            spacing: 6
            Btn {
                iconName: "settings"; variant: "ghost"
                text: qsTr("Settings")
                tip: qsTr("Preferences for downloads, BitTorrent, the browser and the look")
                kbd: "Ctrl+,"
                onClicked: side.win.openSettings("")
            }
            Item { Layout.fillWidth: true }
            Btn {
                iconName: Theme.dark ? "sun" : "moon"; variant: "ghost"; small: true
                tip: Theme.dark ? qsTr("Switch to Stone (light)") : qsTr("Switch to Obsidian (dark)")
                onClicked: App.settings.set("theme", Theme.dark ? "stone" : "obsidian")
            }
            Btn {
                iconName: "info"; variant: "ghost"; small: true
                tip: qsTr("About Rift")
                onClicked: side.win.openAbout()
            }
        }
    }

    RMenu {
        id: catMenu
        property string target: ""
        RMenuItem {
            text: qsTr("Set folder…"); iconName: "folder"
            onTriggered: side.win.prompt(qsTr("Folder for %1").arg(catMenu.target), qsTr("Folder path"),
                                         App.defaultFolderFor(catMenu.target), function (v) { App.saveCategory(catMenu.target, v) })
        }
        RMenuItem {
            text: qsTr("Remove category"); iconName: "trash"; danger: true
            onTriggered: App.removeCategory(catMenu.target)
        }
    }
}
