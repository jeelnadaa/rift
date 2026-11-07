// The Rift mark: a slim disc cut once on a diagonal, halves slid apart. Left = ink, right = accent.
import QtQuick

Item {
    id: root
    property color ink: Theme.c.text
    property color accent: Theme.accent
    property real size: 22
    implicitHeight: size
    implicitWidth: size * 38 / 64

    readonly property string svg: "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"13 0 38 64\">"
        + "<path transform=\"translate(-1.1 -4.6)\" fill=\"" + Theme.hex(ink) + "\" d=\"M39.24 6.66A17 28 0 1 0 24.76 57.34Z\"/>"
        + "<path transform=\"translate(1.1 4.6)\" fill=\"" + Theme.hex(accent) + "\" d=\"M39.24 6.66A17 28 0 1 1 24.76 57.34Z\"/></svg>"

    Image {
        anchors.fill: parent
        sourceSize: Qt.size(Math.ceil(root.implicitWidth * 3), Math.ceil(root.size * 3))
        smooth: true
        mipmap: true
        source: "data:image/svg+xml;utf8," + encodeURIComponent(root.svg)
    }
}
