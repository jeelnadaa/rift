// Key/value row for details. Values can be monospaced and copied with a click.
import QtQuick

Item {
    id: kv
    property string key: ""
    property string value: ""
    property bool mono: false
    property bool copyable: false
    property color valueColor: Theme.c.text
    implicitHeight: Math.max(24, v.implicitHeight + 6)
    implicitWidth: 300
    visible: value.length > 0

    Text {
        id: k
        width: Math.min(130, parent.width * 0.4)
        anchors.verticalCenter: parent.verticalCenter
        text: kv.key
        color: Theme.c.text3
        font.family: Theme.fontUi
        font.pixelSize: 13
        elide: Text.ElideRight
    }
    Text {
        id: v
        anchors.left: k.right
        anchors.leftMargin: 10
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        text: kv.value
        color: ma.containsMouse && kv.copyable ? Theme.accent : kv.valueColor
        font.family: kv.mono ? Theme.fontMono : Theme.fontUi
        font.pixelSize: kv.mono ? 12.5 : 13.5
        elide: Text.ElideMiddle
        maximumLineCount: 1
    }
    MouseArea {
        id: ma
        anchors.fill: v
        enabled: kv.copyable
        hoverEnabled: true
        cursorShape: kv.copyable ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: App.copyText(kv.value)
    }
    Tip { visible: ma.containsMouse && kv.copyable; text: qsTr("Click to copy") }
}
