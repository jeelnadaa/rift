// HTTP segment bar: done map in the background, one colored lane per live connection.
// connections: [{start, cursor, end, speed}], size: total bytes, pieces: optional 0..255 buckets.
import QtQuick

Item {
    id: s
    property var connections: []
    property var pieces: []
    property real size: 0
    property real value: 0
    property bool tall: false
    implicitHeight: tall ? 14 : 5
    implicitWidth: 120

    Rectangle {
        anchors.fill: parent
        radius: s.tall ? 3 : height / 2
        color: Theme.c.pieceMissing
        clip: true

        // Done bytes, either from the bucket map or as one bar.
        Row {
            anchors.fill: parent
            visible: s.pieces.length > 0
            Repeater {
                model: s.pieces.length
                delegate: Rectangle {
                    required property int index
                    width: s.width / s.pieces.length
                    height: s.height
                    color: Theme.c.success
                    opacity: (s.pieces[index] || 0) / 255
                }
            }
        }
        Rectangle {
            visible: s.pieces.length === 0
            height: parent.height
            width: parent.width * Math.min(1, s.value)
            color: Theme.c.success
            opacity: 0.85
            Behavior on width { NumberAnimation { duration: Theme.motion.slow; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.standard } }
        }
        Repeater {
            model: s.size > 0 ? s.connections : []
            delegate: Rectangle {
                required property var modelData
                required property int index
                x: s.width * modelData.start / s.size
                width: Math.max(2, s.width * (modelData.cursor - modelData.start) / s.size)
                height: s.height
                color: Theme.connectionPalette[index % Theme.connectionPalette.length]
                Behavior on width { NumberAnimation { duration: Theme.motion.slow; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.standard } }
                Rectangle {
                    anchors.right: parent.right
                    width: 2
                    height: parent.height
                    color: Theme.c.text
                    opacity: 0.6
                }
            }
        }
    }
}
