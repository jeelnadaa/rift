// Toolbar: Add ▾ · resume · pause · remove · queue up/down · alt speed | filter (regex) · palette · view · settings
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: bar
    property var win
    readonly property var sel: App.selection
    readonly property bool hasSel: sel.length > 0
    property alias searchField: search
    implicitHeight: Theme.size.toolbar
    color: Theme.c.surface

    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.c.line }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 6

        Btn {
            visible: !App.settings.sidebarVisible
            iconName: "chevR"; variant: "ghost"
            tip: qsTr("Show sidebar"); kbd: "Ctrl+B"
            onClicked: App.settings.set("sidebarVisible", true)
        }

        // Split "Add" button
        Row {
            spacing: 1
            Btn {
                id: addBtn
                variant: "primary"
                iconName: "plus"
                text: qsTr("Add")
                tip: qsTr("Add a link, magnet or several links at once")
                kbd: "Ctrl+N"
                onClicked: bar.win.openAdd("", {})
            }
            Btn {
                id: addMore
                variant: "primary"
                iconName: "chevD"
                implicitWidth: 28
                tip: qsTr("More ways to add")
                onClicked: addMenu.popup(addMore, 0, addMore.height + 4)
                Rectangle { width: 1; height: parent.height - 12; anchors.verticalCenter: parent.verticalCenter; color: Qt.rgba(1, 1, 1, 0.25) }
            }
        }
        RMenu {
            id: addMenu
            RMenuItem { text: qsTr("Link or magnet…"); iconName: "link"; kbd: "Ctrl+N"; onTriggered: bar.win.openAdd("", {}) }
            RMenuItem { text: qsTr("Torrent file…"); iconName: "fileTorrent"; kbd: "Ctrl+O"; onTriggered: bar.win.openTorrentPicker() }
            RMenuItem { text: qsTr("Many links…"); iconName: "list"; onTriggered: bar.win.openAdd("", { batch: true }) }
            RMenuItem { text: qsTr("Paste from clipboard"); iconName: "copy"; kbd: "Ctrl+V"; onTriggered: bar.win.pasteAdd() }
        }

        Rectangle { width: 1; height: 22; color: Theme.c.line; Layout.leftMargin: 4; Layout.rightMargin: 4 }

        Btn {
            iconName: "play"; variant: "ghost"
            enabled: bar.hasSel
            tip: qsTr("Resume selected"); kbd: "Ctrl+R"
            onClicked: App.resume(bar.sel)
        }
        Btn {
            iconName: "pause"; variant: "ghost"
            enabled: bar.hasSel
            tip: qsTr("Pause selected"); kbd: "Ctrl+P"
            onClicked: App.pause(bar.sel)
        }
        Btn {
            iconName: "trash"; variant: "ghost"
            enabled: bar.hasSel
            tip: qsTr("Remove selected (keeps files unless you choose otherwise)"); kbd: "Del"
            onClicked: bar.win.openRemove(bar.sel)
        }
        Btn {
            iconName: "arrowU"; variant: "ghost"
            enabled: bar.hasSel
            tip: qsTr("Move up in the queue"); kbd: "Ctrl+↑"
            onClicked: App.queueMove(bar.sel, "up")
        }
        Btn {
            iconName: "arrowD"; variant: "ghost"
            enabled: bar.hasSel
            tip: qsTr("Move down in the queue"); kbd: "Ctrl+↓"
            onClicked: App.queueMove(bar.sel, "down")
        }

        Rectangle { width: 1; height: 22; color: Theme.c.line; Layout.leftMargin: 4; Layout.rightMargin: 4 }

        Btn {
            iconName: "turtle"
            variant: "ghost"
            checkable: true
            checked: App.settings.altSpeed
            text: Theme.compact && bar.width < 1100 ? "" : qsTr("Alt speed")
            tip: qsTr("Alternative speed limits (%1 ↓ · %2 ↑)")
                .arg(App.settings.altDownLimit > 0 ? App.fmtSpeed(App.settings.altDownLimit * 1024) : qsTr("unlimited"))
                .arg(App.settings.altUpLimit > 0 ? App.fmtSpeed(App.settings.altUpLimit * 1024) : qsTr("unlimited"))
            kbd: "Ctrl+T"
            onClicked: App.setAltSpeed(checked)
        }

        Item { Layout.fillWidth: true }

        Field {
            id: search
            Layout.preferredWidth: Math.min(320, bar.width * 0.26)
            iconName: "search"
            placeholderText: qsTr("Filter by name, tracker, tag…")
            tip: qsTr("Filter the list. Ctrl+F")
            invalid: App.list.regex && !App.list.regexValid
            rightPadding: 40
            text: App.list.query
            onTextEdited: App.list.query = text
            Keys.onEscapePressed: { text = ""; App.list.query = ""; focus = false }
            Btn {
                anchors.right: parent.right
                anchors.rightMargin: 4
                anchors.verticalCenter: parent.verticalCenter
                implicitHeight: 24
                implicitWidth: 30
                variant: "ghost"
                checkable: true
                checked: App.list.regex
                text: ".*"
                tip: qsTr("Use a regular expression")
                onClicked: App.list.regex = checked
            }
        }
        Btn {
            iconName: "cmd"; variant: "ghost"
            tip: qsTr("Command palette: every action, by name"); kbd: "Ctrl+K"
            onClicked: bar.win.openPalette()
        }
        Btn {
            id: viewBtn
            iconName: "sliders"; variant: "ghost"
            tip: qsTr("View options")
            onClicked: viewMenu.popup(viewBtn, 0, viewBtn.height + 4)
        }
        RMenu {
            id: viewMenu
            RMenuItem { text: qsTr("Sidebar"); checkable: true; checked: App.settings.sidebarVisible; kbd: "Ctrl+B"
                onTriggered: App.settings.set("sidebarVisible", !App.settings.sidebarVisible) }
            RMenuItem { text: qsTr("Details panel"); checkable: true; checked: App.settings.detailsVisible; kbd: "Ctrl+I"
                onTriggered: App.settings.set("detailsVisible", !App.settings.detailsVisible) }
            MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.c.lineSoft } }
            RMenuItem { text: qsTr("Compact rows"); checkable: true; checked: Theme.compact
                onTriggered: App.settings.set("density", "compact") }
            RMenuItem { text: qsTr("Comfortable rows"); checkable: true; checked: !Theme.compact
                onTriggered: App.settings.set("density", "comfortable") }
            MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.c.lineSoft } }
            RMenuItem { text: qsTr("System theme"); checkable: true; checked: App.settings.theme === "system"; onTriggered: App.settings.set("theme", "system") }
            RMenuItem { text: qsTr("Stone (light)"); checkable: true; checked: App.settings.theme === "stone"; onTriggered: App.settings.set("theme", "stone") }
            RMenuItem { text: qsTr("Obsidian (dark)"); checkable: true; checked: App.settings.theme === "obsidian"; onTriggered: App.settings.set("theme", "obsidian") }
        }
    }
}
