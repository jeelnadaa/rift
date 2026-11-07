// The main list: sticky sortable header, multi-select (click, Ctrl, Shift, Ctrl+A), keyboard, drag rows
// onto categories/tags, empty states. When the filter/sort/search changes, rows fade up with a stagger.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

FocusScope {
    id: dl
    property var win
    property int anchorRow: -1
    readonly property var sel: App.selection
    readonly property var cols: ({
        size: width > 760, progress: width > 560, state: width > 640, down: true,
        up: width > 900, eta: width > 1000, peers: width > 1100 })

    function isSelected(id) { return sel.indexOf(id) >= 0 }
    function selectRow(row, mods) {
        var id = App.list.idAt(row)
        if (!id.length) return
        if (mods & Qt.ShiftModifier && anchorRow >= 0) {
            var range = App.list.idsBetween(anchorRow, row)
            if (mods & Qt.ControlModifier) {
                var merged = sel.slice()
                for (var i = 0; i < range.length; ++i) if (merged.indexOf(range[i]) < 0) merged.push(range[i])
                App.select(merged, id)
            } else App.select(range, id)
        } else if (mods & (Qt.ControlModifier | Qt.MetaModifier)) {
            var s = sel.slice()
            var k = s.indexOf(id)
            if (k >= 0) s.splice(k, 1); else s.push(id)
            App.select(s, k >= 0 ? (s.length ? s[s.length - 1] : "") : id)
            anchorRow = row
        } else {
            App.select([id], id)
            anchorRow = row
        }
        view.currentIndex = row
    }
    function moveBy(d, mods) {
        var row = Math.max(0, Math.min(view.count - 1, (view.currentIndex < 0 ? -1 : view.currentIndex) + d))
        selectRow(row, mods & Qt.ShiftModifier)
        view.positionViewAtIndex(row, ListView.Contain)
    }

    // Replay the fade-up when what the list shows changes (not on data refresh).
    property int generation: 0
    Connections {
        target: App.list
        function onChanged() { dl.generation++ }
    }

    Rectangle { anchors.fill: parent; color: Theme.c.surface }

    // Header
    Rectangle {
        id: header
        width: parent.width
        height: Theme.size.header
        color: Theme.c.surface
        z: 2
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.c.line }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 12
            Repeater {
                model: [["name", qsTr("Name"), -1, true, Text.AlignLeft],
                        ["size", qsTr("Size"), 84, dl.cols.size, Text.AlignRight],
                        ["progress", qsTr("Progress"), 150, dl.cols.progress, Text.AlignLeft],
                        ["state", qsTr("Status"), 118, dl.cols.state, Text.AlignLeft],
                        ["down", qsTr("Down"), 86, dl.cols.down, Text.AlignRight],
                        ["up", qsTr("Up"), 80, dl.cols.up, Text.AlignRight],
                        ["eta", qsTr("ETA"), 70, dl.cols.eta, Text.AlignRight],
                        ["peers", qsTr("Peers"), 76, dl.cols.peers, Text.AlignRight]]
                delegate: Item {
                    id: hc
                    required property var modelData
                    readonly property bool sorted: App.list.sortKey === modelData[0]
                    visible: modelData[3]
                    Layout.fillWidth: modelData[2] < 0
                    Layout.minimumWidth: modelData[2] < 0 ? 160 : modelData[2]
                    Layout.preferredWidth: modelData[2] < 0 ? 200 : modelData[2]
                    Layout.fillHeight: true
                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.right: hc.modelData[4] === Text.AlignRight ? parent.right : undefined
                        anchors.left: hc.modelData[4] === Text.AlignRight ? undefined : parent.left
                        anchors.leftMargin: hc.modelData[0] === "name" ? 28 : 0
                        spacing: 4
                        Cap {
                            text: hc.modelData[1]
                            color: hc.sorted ? Theme.c.text : hm.containsMouse ? Theme.c.text2 : Theme.c.text3
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Icon {
                            visible: hc.sorted
                            name: App.list.ascending ? "chevU" : "chevD"
                            size: 12
                            color: Theme.accent
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }
                    MouseArea {
                        id: hm
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (App.list.sortKey === hc.modelData[0]) App.list.ascending = !App.list.ascending
                            else { App.list.sortKey = hc.modelData[0]; App.list.ascending = hc.modelData[0] === "name" }
                            App.settings.set("sortKey", App.list.sortKey)
                            App.settings.set("sortAsc", App.list.ascending)
                        }
                    }
                    Tip { visible: hm.containsMouse; text: qsTr("Sort by %1").arg(hc.modelData[1].toLowerCase()) }
                }
            }
        }
    }

    ListView {
        id: view
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        clip: true
        focus: true
        interactive: false     // desktop: wheel + scrollbar; dragging rows starts a drag-and-drop instead
        model: App.list
        currentIndex: -1
        highlightFollowsCurrentItem: false
        boundsBehavior: Flickable.StopAtBounds
        reuseItems: true
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            contentItem: Rectangle { implicitWidth: 6; radius: 3; color: Theme.c.text3; opacity: 0.45 }
        }
        WheelHandler {
            onWheel: (ev) => view.contentY = Math.max(0, Math.min(view.contentHeight - view.height,
                                                                   view.contentY - ev.angleDelta.y))
        }

        delegate: DownloadRow {
            id: row
            width: view.width
            cols: dl.cols
            selected: dl.sel.indexOf(rid) >= 0
            onPressed: (m) => {
                dl.forceActiveFocus()
                if (m.button === Qt.RightButton) {
                    if (!dl.isSelected(rid)) dl.selectRow(index, 0)
                } else if (!(dl.isSelected(rid) && !(m.modifiers & (Qt.ShiftModifier | Qt.ControlModifier)) && dl.sel.length > 1)) {
                    dl.selectRow(index, m.modifiers)
                }
            }
            onDoubleClicked: row.st === "completed" || row.st === "seeding" ? App.openFile(rid) : App.togglePause([rid])
            onMenu: (x, y) => dl.win.openContextMenu()

            // fade-up on list change, staggered 22 ms
            property int gen: dl.generation
            onGenChanged: if (!Theme.reducedMotion && index < 24) appear.restart()
            transform: Translate { id: lift; y: 0 }
            SequentialAnimation {
                id: appear
                PropertyAction { target: row; property: "opacity"; value: 0 }
                PropertyAction { target: lift; property: "y"; value: 6 }
                PauseAnimation { duration: Math.min(row.index, 24) * Theme.motion.rowStagger }
                ParallelAnimation {
                    NumberAnimation { target: row; property: "opacity"; to: 1; duration: Theme.motion.base }
                    NumberAnimation { target: lift; property: "y"; to: 0; duration: Theme.motion.slow; easing.type: Easing.OutCubic }
                }
            }
        }

        Keys.onPressed: (ev) => {
            if (ev.key === Qt.Key_Down) { dl.moveBy(1, ev.modifiers); ev.accepted = true }
            else if (ev.key === Qt.Key_Up) { dl.moveBy(-1, ev.modifiers); ev.accepted = true }
            else if (ev.key === Qt.Key_Home) { dl.selectRow(0, 0); view.positionViewAtBeginning(); ev.accepted = true }
            else if (ev.key === Qt.Key_End) { dl.selectRow(view.count - 1, 0); view.positionViewAtEnd(); ev.accepted = true }
            else if (ev.key === Qt.Key_A && (ev.modifiers & Qt.ControlModifier)) { App.select(App.list.allIds(), App.current); ev.accepted = true }
            else if (ev.key === Qt.Key_Space && dl.sel.length) { App.togglePause(dl.sel); ev.accepted = true }
            else if ((ev.key === Qt.Key_Return || ev.key === Qt.Key_Enter) && App.current.length) { App.openFile(App.current); ev.accepted = true }
            else if (ev.key === Qt.Key_Escape && dl.sel.length) { App.select([], ""); ev.accepted = true }
            else if (ev.key === Qt.Key_Menu && dl.sel.length) { dl.win.openContextMenu(); ev.accepted = true }
        }
    }

    // Click on empty space clears the selection
    MouseArea {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.topMargin: view.contentHeight - view.contentY
        visible: anchors.topMargin < height
        acceptedButtons: Qt.LeftButton
        onClicked: { App.select([], ""); dl.forceActiveFocus() }
    }

    EmptyState {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: view.count === 0
        win: dl.win
        filtered: App.totalCount > 0
    }
}
