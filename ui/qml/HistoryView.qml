// History: everything that finished, kept even after removal from the list. Searchable.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: hv
    property var win
    property var items: []
    color: Theme.c.surface
    function reload() { items = App.history(search.text) }
    onVisibleChanged: if (visible) reload()
    Component.onCompleted: reload()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 28
            Layout.rightMargin: 20
            Layout.topMargin: 22
            Layout.bottomMargin: 14
            spacing: 10
            Text {
                Layout.fillWidth: true
                text: qsTr("History")
                color: Theme.c.text
                font.family: Theme.fontUi
                font.pixelSize: Theme.type.page
                font.weight: Font.Bold
                font.letterSpacing: -0.8
            }
            Field { id: search; Layout.preferredWidth: 260; iconName: "search"; placeholderText: qsTr("Search history"); onTextEdited: hv.reload() }
            Btn {
                text: qsTr("Clear")
                iconName: "trash"
                enabled: hv.items.length > 0
                tip: qsTr("Forget the history (files are not touched)")
                onClicked: { App.clearHistory(); hv.reload() }
            }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.c.line }
        ListView {
            id: lv
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: hv.items
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}
            delegate: Rectangle {
                id: hr
                required property var modelData
                width: lv.width
                height: Theme.size.row
                color: hm.containsMouse ? Theme.c.hover : "transparent"
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.c.lineSoft }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 28
                    anchors.rightMargin: 20
                    spacing: 12
                    Kind { kind: hr.modelData.kind }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { Layout.fillWidth: true; text: hr.modelData.name; elide: Text.ElideMiddle; color: Theme.c.text; font.family: Theme.fontUi; font.pixelSize: Theme.type.body; font.weight: Font.DemiBold }
                        Text { Layout.fillWidth: true; text: hr.modelData.path; elide: Text.ElideMiddle; color: Theme.c.text3; font.family: Theme.fontMono; font.pixelSize: 11 }
                    }
                    Text { text: App.fmtBytes(hr.modelData.size); color: Theme.c.text2; font.family: Theme.fontMono; font.pixelSize: Theme.type.num }
                    Text { Layout.preferredWidth: 130; horizontalAlignment: Text.AlignRight; text: App.fmtRelative(hr.modelData.completedAt); color: Theme.c.text3; font.family: Theme.fontUi; font.pixelSize: 13 }
                    Btn { small: true; variant: "ghost"; iconName: "copy"; tip: qsTr("Copy the source link"); onClicked: App.copyText(hr.modelData.source) }
                    Btn { small: true; variant: "ghost"; iconName: "download"; tip: qsTr("Download again"); onClicked: hv.win.openAdd(hr.modelData.source, {}) }
                }
                MouseArea { id: hm; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
            }
            Column {
                anchors.centerIn: parent
                visible: lv.count === 0
                spacing: 8
                Icon { anchors.horizontalCenter: parent.horizontalCenter; name: "clock"; size: 26; color: Theme.c.text3 }
                Text {
                    text: search.text.length ? qsTr("Nothing in history matches") : qsTr("Finished downloads show up here")
                    color: Theme.c.text3
                    font.family: Theme.fontUi
                    font.pixelSize: Theme.type.body
                }
            }
        }
    }
}
