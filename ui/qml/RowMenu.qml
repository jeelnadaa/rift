// Right-click menu for the selected downloads.
import QtQuick
import QtQuick.Controls.Basic

RMenu {
    id: cm
    property var win
    readonly property var sel: App.selection
    readonly property var d: App.detail
    readonly property bool one: sel.length === 1
    readonly property bool bt: one && d.kind === "bt"
    readonly property bool done: one && (d.state === "completed" || d.state === "seeding")

    component Sep: MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.c.lineSoft } }

    RMenuItem { text: qsTr("Open"); iconName: "external"; enabled: cm.done; kbd: "Enter"; onTriggered: App.openFile(cm.sel[0]) }
    RMenuItem { text: qsTr("Show in folder"); iconName: "folderOpen"; enabled: cm.one; onTriggered: App.openFolder(cm.sel[0]) }
    Sep {}
    RMenuItem { text: qsTr("Resume"); iconName: "play"; kbd: "Ctrl+R"; onTriggered: App.resume(cm.sel) }
    RMenuItem { text: qsTr("Pause"); iconName: "pause"; kbd: "Ctrl+P"; onTriggered: App.pause(cm.sel) }
    RMenuItem { text: qsTr("Force start"); iconName: "bolt"; onTriggered: App.forceStart(cm.sel) }
    RMenu {
        title: qsTr("Queue")
        RMenuItem { text: qsTr("Move to top"); iconName: "arrowTop"; onTriggered: App.queueMove(cm.sel, "top") }
        RMenuItem { text: qsTr("Move up"); iconName: "arrowU"; kbd: "Ctrl+↑"; onTriggered: App.queueMove(cm.sel, "up") }
        RMenuItem { text: qsTr("Move down"); iconName: "arrowD"; kbd: "Ctrl+↓"; onTriggered: App.queueMove(cm.sel, "down") }
        RMenuItem { text: qsTr("Move to bottom"); iconName: "arrowBottom"; onTriggered: App.queueMove(cm.sel, "bottom") }
    }
    Sep {}
    RMenu {
        id: catSub
        title: qsTr("Category")
        RMenuItem { text: qsTr("None"); checkable: true; checked: !!(cm.one && !cm.d.category); onTriggered: App.setCategory(cm.sel, "") }
        Instantiator {
            model: App.categories
            delegate: RMenuItem {
                required property var modelData
                text: modelData.name
                checkable: true
                checked: !!(cm.one && cm.d.category === modelData.name)
                onTriggered: App.setCategory(cm.sel, modelData.name)
            }
            onObjectAdded: (i, o) => catSub.insertItem(i + 1, o)
            onObjectRemoved: (i, o) => catSub.removeItem(o)
        }
    }
    RMenu {
        id: tagSub
        title: qsTr("Tags")
        Instantiator {
            model: App.tags
            delegate: RMenuItem {
                required property var modelData
                text: "#" + modelData.name
                checkable: true
                checked: !!(cm.one && cm.d.tags && cm.d.tags.indexOf(modelData.name) >= 0)
                onTriggered: App.toggleTag(cm.sel, modelData.name)
            }
            onObjectAdded: (i, o) => tagSub.insertItem(i, o)
            onObjectRemoved: (i, o) => tagSub.removeItem(o)
        }
        RMenuItem {
            text: qsTr("New tag…"); iconName: "plus"
            onTriggered: cm.win.prompt(qsTr("New tag"), qsTr("Tag"), "", function (v) { if (v.length) App.toggleTag(cm.sel, v) })
        }
    }
    RMenuItem { text: qsTr("Speed limits…"); iconName: "speed"; onTriggered: cm.win.openLimits(cm.sel) }
    RMenuItem {
        text: qsTr("Move files…"); iconName: "move"
        onTriggered: cm.win.pickFolder(function (p) { App.moveTo(cm.sel, p) })
    }
    Sep {}
    RMenuItem { text: qsTr("Download in order"); checkable: true; visible: cm.bt; checked: !!(cm.bt && cm.d.sequential)
        onTriggered: App.setSequential(cm.sel, !cm.d.sequential) }
    RMenuItem { text: qsTr("Force recheck"); iconName: "recheck"; visible: cm.bt || !cm.one; onTriggered: App.recheck(cm.sel) }
    RMenuItem { text: qsTr("Ask trackers for peers"); iconName: "refresh"; visible: cm.bt || !cm.one; onTriggered: App.reannounce(cm.sel) }
    RMenuItem { text: qsTr("Restart from zero"); iconName: "undo"; visible: cm.one && cm.d.kind === "http"; onTriggered: App.restart(cm.sel) }
    RMenuItem { text: qsTr("Copy link"); iconName: "copy"; kbd: "Ctrl+C"; onTriggered: App.copyLinks(cm.sel) }
    RMenuItem {
        text: qsTr("Export .torrent…"); iconName: "fileTorrent"; visible: cm.bt
        onTriggered: cm.win.saveTorrent(cm.sel[0], cm.d.name)
    }
    Sep {}
    RMenuItem { text: qsTr("Remove…"); iconName: "trash"; danger: true; kbd: "Del"; onTriggered: cm.win.openRemove(cm.sel) }
}
