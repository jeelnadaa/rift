// Keyboard key cap.
import QtQuick

Rectangle {
    property string text: ""
    implicitHeight: 20
    implicitWidth: Math.max(20, t.implicitWidth + 10)
    radius: 5
    color: Theme.c.sunken
    border.color: Theme.c.line
    Text {
        id: t
        anchors.centerIn: parent
        text: parent.text
        color: Theme.c.text2
        font.family: Theme.fontMono
        font.pixelSize: 11
        font.weight: Font.DemiBold
    }
}
