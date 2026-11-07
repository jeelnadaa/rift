// One icon from the design set, tinted with any color (rendered from SVG, crisp at any DPI).
import QtQuick
import "Icons.js" as Icons

Item {
    id: root
    property string name
    property color color: Theme.c.text2
    property real size: Theme.size.icon
    property real stroke: 1.75
    implicitWidth: size
    implicitHeight: size

    Image {
        anchors.fill: parent
        sourceSize: Qt.size(Math.ceil(root.size * 2), Math.ceil(root.size * 2))
        fillMode: Image.PreserveAspectFit
        smooth: true
        mipmap: true
        opacity: root.color.a
        source: root.name.length ? "data:image/svg+xml;utf8," + encodeURIComponent(Icons.svg(root.name, Theme.hex(root.color), root.stroke)) : ""
    }
}
