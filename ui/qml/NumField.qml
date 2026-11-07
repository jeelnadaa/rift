// Numeric input with a unit suffix and small stepper buttons. Emits edited(value) on commit.
import QtQuick
import QtQuick.Controls.Basic

Item {
    id: n
    property real value: 0
    property real from: 0
    property real to: 1000000
    property real step: 1
    property int decimals: 0
    property string suffix: ""
    property string tip: ""
    property string zeroText: ""      // e.g. "Unlimited" shown when the value is 0
    signal edited(real value)

    implicitWidth: 150
    implicitHeight: Theme.size.control
    Accessible.role: Accessible.SpinBox
    Accessible.name: tip

    function commit(v) {
        var x = Math.max(from, Math.min(to, v))
        x = decimals > 0 ? Number(x.toFixed(decimals)) : Math.round(x)
        if (x !== value) { value = x; edited(x) }
        input.text = display()
    }
    function display() { return value === 0 && zeroText.length ? zeroText : value.toFixed(decimals) }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radius.sm
        color: Theme.c.input
        border.width: input.activeFocus ? 1.5 : 1
        border.color: input.activeFocus ? Theme.accent : Theme.c.line
    }
    TextInput {
        id: input
        anchors.left: parent.left
        anchors.leftMargin: 11
        anchors.right: unit.left
        anchors.rightMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        text: n.display()
        color: Theme.c.text
        font.family: Theme.fontMono
        font.pixelSize: Theme.type.num
        selectByMouse: true
        selectionColor: Theme.selection
        clip: true
        onActiveFocusChanged: if (activeFocus) { if (n.value === 0 && n.zeroText.length) text = "0"; selectAll() } else n.commit(Number(text) || 0)
        onAccepted: { n.commit(Number(text) || 0); focus = false }
        Keys.onUpPressed: n.commit(n.value + n.step)
        Keys.onDownPressed: n.commit(n.value - n.step)
    }
    Text {
        id: unit
        anchors.right: steps.left
        anchors.rightMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        text: n.value === 0 && n.zeroText.length && !input.activeFocus ? "" : n.suffix
        color: Theme.c.text3
        font.family: Theme.fontUi
        font.pixelSize: 12.5
    }
    Column {
        id: steps
        anchors.right: parent.right
        anchors.rightMargin: 3
        anchors.verticalCenter: parent.verticalCenter
        Repeater {
            model: [["chevU", 1], ["chevD", -1]]
            delegate: Rectangle {
                id: st
                required property var modelData
                width: 18
                height: (n.height - 8) / 2
                radius: 3
                color: sm.pressed ? Theme.c.pressed : sm.containsMouse ? Theme.c.hover : "transparent"
                Icon { anchors.centerIn: parent; name: st.modelData[0]; size: 11; color: Theme.c.text3 }
                MouseArea {
                    id: sm
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: n.commit(n.value + st.modelData[1] * n.step)
                }
            }
        }
    }
    HoverHandler { id: hov }
    Tip { visible: hov.hovered && n.tip.length > 0 && !input.activeFocus; text: n.tip }
}
