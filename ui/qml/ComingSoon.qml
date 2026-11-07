// Placeholder page for sections planned after 1.0 (Search, RSS).
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: cs
    property string title: ""
    property string iconName: "sparkle"
    property string body: ""
    color: Theme.c.surface

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(420, parent.width - 48)
        spacing: 12
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 72; height: 72
            color: Theme.accentSoft
            topLeftRadius: Theme.radius.big; bottomRightRadius: Theme.radius.big
            topRightRadius: Theme.radius.xs; bottomLeftRadius: Theme.radius.xs
            Icon { anchors.centerIn: parent; name: cs.iconName; size: 28; color: Theme.accent }
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: cs.title
            color: Theme.c.text
            font.family: Theme.fontUi
            font.pixelSize: Theme.type.section
            font.weight: Font.Bold
            font.variableAxes: Theme.displayAxes
        }
        Text {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: cs.body
            color: Theme.c.text2
            font.family: Theme.fontUi
            font.pixelSize: Theme.type.body
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Coming in a later version")
            color: Theme.accent
            font.family: Theme.fontSerif
            font.italic: true
            font.pixelSize: 18
        }
    }
}
