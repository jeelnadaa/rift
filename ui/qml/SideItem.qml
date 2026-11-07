// Sidebar item: content slides 4 px right on hover, icon scales 1.12, count shifts left.
// The selected item gets the selection fill, an accent rule and a soft accent shadow.
import QtQuick

Item {
    id: s
    property string iconName: ""
    property string text: ""
    property string tip: ""
    property int count: -1
    property bool selected: false
    property color dot: "transparent"   // color dot instead of an icon (categories, tags)
    property bool dropTarget: false
    signal clicked()
    signal rightClicked(real x, real y)

    implicitHeight: Theme.size.sideItem
    implicitWidth: 200
    Accessible.role: Accessible.Button
    Accessible.name: text
    Accessible.selected: selected

    Rectangle {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        radius: Theme.radius.sm
        color: s.selected ? Theme.selection : s.dropTarget ? Theme.accentSoft : ma.pressed ? Theme.c.pressed : ma.containsMouse ? Theme.c.hover : "transparent"
        border.width: s.dropTarget ? 1.5 : 0
        border.color: Theme.accent
        Behavior on color { ColorAnimation { duration: Theme.motion.fast } }
        scale: ma.pressed ? 0.98 : 1
        Behavior on scale { NumberAnimation { duration: Theme.motion.press } }

        Rectangle {
            visible: s.selected
            x: 0
            width: 3
            height: parent.height - 14
            anchors.verticalCenter: parent.verticalCenter
            radius: 2
            color: Theme.accent
        }

        Item {
            anchors.fill: parent
            anchors.leftMargin: 12 + (ma.containsMouse && !s.selected ? 4 : 0)
            anchors.rightMargin: 10 + (ma.containsMouse ? 2 : 0)
            Behavior on anchors.leftMargin { NumberAnimation { duration: Theme.motion.base; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.spring } }
            Behavior on anchors.rightMargin { NumberAnimation { duration: Theme.motion.base; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.standard } }

            Item {
                id: lead
                width: 16
                height: 16
                anchors.verticalCenter: parent.verticalCenter
                Icon {
                    visible: s.dot.a === 0
                    anchors.centerIn: parent
                    name: s.iconName
                    color: s.selected ? Theme.accent : Theme.c.text2
                    scale: ma.containsMouse ? 1.12 : 1
                    Behavior on scale { NumberAnimation { duration: Theme.motion.base; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.spring } }
                }
                Rectangle {
                    visible: s.dot.a > 0
                    anchors.centerIn: parent
                    width: 9; height: 9; radius: 3
                    color: s.dot
                    scale: ma.containsMouse ? 1.15 : 1
                    Behavior on scale { NumberAnimation { duration: Theme.motion.base; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.spring } }
                }
            }
            Text {
                anchors.left: lead.right
                anchors.leftMargin: 10
                anchors.right: cnt.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                text: s.text
                elide: Text.ElideRight
                color: s.selected ? Theme.c.text : Theme.c.text2
                font.family: Theme.fontUi
                font.pixelSize: Theme.type.body
                font.weight: s.selected ? Font.DemiBold : Theme.weightNav
            }
            Text {
                id: cnt
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                visible: s.count >= 0
                text: s.count
                color: s.selected ? Theme.accent : Theme.c.text3
                font.family: Theme.fontMono; font.features: Theme.numFeatures
                font.pixelSize: 12
            }
        }
    }
    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        cursorShape: Qt.PointingHandCursor
        onClicked: (ev) => ev.button === Qt.RightButton ? s.rightClicked(ev.x, ev.y) : s.clicked()
    }
    Tip { visible: ma.containsMouse && s.tip.length > 0; text: s.tip }
}
