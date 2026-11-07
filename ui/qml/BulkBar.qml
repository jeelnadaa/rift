// Bulk-action bar shown when several downloads are selected. Slides down 6 px.
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: bb
    property var win
    readonly property var sel: App.selection
    visible: sel.length > 1
    implicitHeight: visible ? 44 : 0
    color: Theme.accentSoft
    clip: true
    Behavior on implicitHeight { NumberAnimation { duration: Theme.motion.base; easing.type: Easing.OutCubic } }
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.soft(Theme.accent, 0.25) }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 12
        spacing: 6
        Text {
            text: qsTr("%n selected", "", bb.sel.length)
            color: Theme.accent
            font.family: Theme.fontUi
            font.pixelSize: Theme.type.body
            font.weight: Font.Bold
        }
        Item { Layout.fillWidth: true }
        Btn { small: true; iconName: "play"; text: qsTr("Resume"); tip: qsTr("Resume all selected"); onClicked: App.resume(bb.sel) }
        Btn { small: true; iconName: "pause"; text: qsTr("Pause"); tip: qsTr("Pause all selected"); onClicked: App.pause(bb.sel) }
        Btn { small: true; iconName: "tag"; text: qsTr("Category"); tip: qsTr("Put the selected downloads in a category"); onClicked: bb.win.openContextMenu() }
        Btn { small: true; iconName: "speed"; text: qsTr("Limits"); tip: qsTr("Speed limits for the selected downloads"); onClicked: bb.win.openLimits(bb.sel) }
        Btn { small: true; iconName: "copy"; tip: qsTr("Copy links"); onClicked: App.copyLinks(bb.sel) }
        Btn { small: true; iconName: "trash"; variant: "ghost"; tint: Theme.c.danger; tip: qsTr("Remove selected"); kbd: "Del"; onClicked: bb.win.openRemove(bb.sel) }
        Btn { small: true; iconName: "x"; variant: "ghost"; tip: qsTr("Clear selection"); kbd: "Esc"; onClicked: App.select([], "") }
    }
}
