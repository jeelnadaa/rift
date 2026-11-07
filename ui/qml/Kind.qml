// Kind chip: TORRENT (accent) or HTTP (teal).
import QtQuick

Rectangle {
    id: k
    property string kind: "bt"
    readonly property color col: kind === "bt" ? Theme.kindTorrent : Theme.c.kindHttp
    implicitHeight: 17
    implicitWidth: t.implicitWidth + 10
    radius: 4
    color: Theme.soft(col, 0.12)
    border.color: Theme.soft(col, 0.28)
    Text {
        id: t
        anchors.centerIn: parent
        text: k.kind === "bt" ? "TORRENT" : "HTTP"
        color: k.col
        font.family: Theme.fontMono
        font.pixelSize: 10
        font.weight: Font.Bold
        font.letterSpacing: 0.6
    }
}
