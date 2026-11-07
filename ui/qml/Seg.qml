// Segmented control with a sliding thumb. options: [[value, label, icon?], ...]
import QtQuick

Rectangle {
    id: seg
    property var options: []
    property var value
    property bool small: false
    signal activated(var value)

    implicitHeight: small ? Theme.size.controlSmall : Theme.size.control
    implicitWidth: row.implicitWidth + 6
    radius: Theme.radius.sm + 1
    color: Theme.c.sunken
    border.color: Theme.c.lineSoft
    border.width: 1

    readonly property int index: {
        for (var i = 0; i < options.length; ++i)
            if (options[i][0] === value) return i
        return -1
    }

    Rectangle {
        id: thumb
        readonly property Item target: seg.index >= 0 && seg.index < rep.count ? rep.itemAt(seg.index) : null
        visible: target !== null
        x: target ? row.x + target.x : 3
        y: 3
        width: target ? target.width : 0
        height: seg.height - 6
        radius: Theme.radius.sm - 1
        color: Theme.c.raised
        border.color: Theme.c.line
        Behavior on x { NumberAnimation { duration: Theme.motion.base; easing.type: Easing.OutBack } }
        Behavior on width { NumberAnimation { duration: Theme.motion.base; easing.type: Easing.OutCubic } }
    }

    Row {
        id: row
        x: 3
        anchors.verticalCenter: parent.verticalCenter
        Repeater {
            id: rep
            model: seg.options
            delegate: Item {
                id: opt
                required property var modelData
                required property int index
                readonly property bool active: seg.index === index
                readonly property string iconName: modelData.length > 2 && modelData[2] ? modelData[2] : ""
                implicitWidth: lbl.implicitWidth + (iconName.length ? 20 : 0) + (seg.small ? 18 : 24)
                implicitHeight: seg.height - 6
                Accessible.role: Accessible.RadioButton
                Accessible.name: modelData[1]
                Accessible.checked: active
                Row {
                    anchors.centerIn: parent
                    spacing: 6
                    Icon {
                        visible: opt.iconName.length > 0
                        name: opt.iconName
                        size: 14
                        color: opt.active ? Theme.c.text : Theme.c.text3
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        id: lbl
                        text: opt.modelData[1]
                        color: opt.active ? Theme.c.text : (ma.containsMouse ? Theme.c.text2 : Theme.c.text3)
                        font.family: Theme.fontUi
                        font.pixelSize: seg.small ? 12.5 : 13.5
                        font.weight: Font.DemiBold
                        anchors.verticalCenter: parent.verticalCenter
                        Behavior on color { ColorAnimation { duration: Theme.motion.fast } }
                    }
                }
                MouseArea {
                    id: ma
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        seg.value = opt.modelData[0]
                        seg.activated(opt.modelData[0])
                    }
                }
            }
        }
    }
}
