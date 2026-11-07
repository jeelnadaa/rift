// One setting: label + help on the left, the control on the right (children go to the right side).
import QtQuick
import QtQuick.Layouts

Item {
    id: sr
    property string label: ""
    property string help: ""
    property bool stacked: false        // control goes under the text (for wide controls)
    default property alias control: slot.data
    Layout.fillWidth: true
    implicitHeight: stacked ? textCol.implicitHeight + slot.childrenRect.height + 26
                            : Math.max(textCol.implicitHeight, slot.childrenRect.height) + 20

    ColumnLayout {
        id: textCol
        anchors.left: parent.left
        anchors.right: sr.stacked ? parent.right : slot.left
        anchors.rightMargin: sr.stacked ? 0 : 20
        y: 10
        spacing: 3
        Text {
            Layout.fillWidth: true
            text: sr.label
            color: Theme.c.text
            font.family: Theme.fontUi
            font.pixelSize: Theme.type.body
            font.weight: Font.DemiBold
            wrapMode: Text.Wrap
        }
        Text {
            Layout.fillWidth: true
            visible: sr.help.length > 0
            text: sr.help
            color: Theme.c.text3
            font.family: Theme.fontUi
            font.pixelSize: 12.5
            wrapMode: Text.Wrap
            lineHeight: 1.15
        }
    }
    Item {
        id: slot
        anchors.right: parent.right
        anchors.left: sr.stacked ? parent.left : undefined
        y: sr.stacked ? textCol.y + textCol.implicitHeight + 8 : Math.max(10, (sr.height - childrenRect.height) / 2)
        width: sr.stacked ? parent.width : childrenRect.width
        height: childrenRect.height
    }
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.c.lineSoft }
}
