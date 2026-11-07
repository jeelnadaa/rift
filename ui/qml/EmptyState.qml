// Empty list: first launch (drop zone + ways to add) or "no matches" for the current filter.
// The art floats 6 px on a 5 s loop (stops with reduced motion).
import QtQuick
import QtQuick.Layouts

Item {
    id: e
    property var win
    property bool filtered: false

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(440, parent.width - 48)
        spacing: 14

        Rectangle {
            id: art
            Layout.alignment: Qt.AlignHCenter
            width: 96
            height: 96
            color: Theme.c.raised
            border.color: Theme.c.line
            topLeftRadius: Theme.radius.big
            bottomRightRadius: Theme.radius.big
            topRightRadius: Theme.radius.xs
            bottomLeftRadius: Theme.radius.xs
            Logo { anchors.centerIn: parent; size: 46; visible: !e.filtered }
            Icon { anchors.centerIn: parent; name: "search"; size: 34; color: Theme.c.text3; visible: e.filtered }
            transform: Translate { id: fl; y: 0 }
            SequentialAnimation {
                running: !Theme.reducedMotion && e.visible
                loops: Animation.Infinite
                NumberAnimation { target: fl; property: "y"; from: 0; to: -6; duration: 2500; easing.type: Easing.InOutSine }
                NumberAnimation { target: fl; property: "y"; from: -6; to: 0; duration: 2500; easing.type: Easing.InOutSine }
            }
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: e.filtered ? qsTr("Nothing matches") : qsTr("Drop anything here")
            color: Theme.c.text
            font.family: Theme.fontUi
            font.pixelSize: 20
            font.weight: Font.Bold
            font.variableAxes: Theme.displayAxes
            font.letterSpacing: -0.4
        }
        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: e.filtered
                  ? qsTr("No download fits this view or search. Try another filter, or clear the search.")
                  : qsTr("Links, magnets and .torrent files. Paste with Ctrl+V, drag them onto the window, or send them from your browser.")
            color: Theme.c.text2
            font.family: Theme.fontUi
            font.pixelSize: Theme.type.body
            lineHeight: 1.25
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 6
            spacing: 8
            Btn {
                visible: !e.filtered
                variant: "primary"
                iconName: "plus"
                text: qsTr("Add link")
                tip: qsTr("Add a link or magnet")
                kbd: "Ctrl+N"
                onClicked: e.win.openAdd("", {})
            }
            Btn {
                visible: !e.filtered
                iconName: "fileTorrent"
                text: qsTr("Open torrent")
                tip: qsTr("Open a .torrent file")
                kbd: "Ctrl+O"
                onClicked: e.win.openTorrentPicker()
            }
            Btn {
                visible: e.filtered
                iconName: "x"
                text: qsTr("Show everything")
                tip: qsTr("Clear the filter and the search")
                onClicked: { App.list.filter = "all"; App.list.query = "" }
            }
        }
        RowLayout {
            visible: !e.filtered
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: 10
            spacing: 18
            Repeater {
                model: [["Ctrl+V", qsTr("Paste")], ["Ctrl+K", qsTr("Commands")], ["Ctrl+,", qsTr("Settings")]]
                delegate: Row {
                    required property var modelData
                    spacing: 6
                    Kbd { text: modelData[0] }
                    Text {
                        text: modelData[1]
                        color: Theme.c.text3
                        font.family: Theme.fontUi
                        font.pixelSize: 12
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }
    }
}
