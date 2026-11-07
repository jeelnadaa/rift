// Text input with a focus ring and an optional leading icon.
import QtQuick
import QtQuick.Controls.Basic

TextField {
    id: f
    property string iconName: ""
    property string tip: ""
    property bool invalid: false
    implicitHeight: Theme.size.control
    implicitWidth: 220
    leftPadding: iconName.length ? 32 : 11
    rightPadding: 11
    color: Theme.c.text
    placeholderTextColor: Theme.c.text3
    selectionColor: Theme.selection
    selectedTextColor: Theme.c.text
    font.family: Theme.fontUi
    font.pixelSize: Theme.type.body
    verticalAlignment: TextInput.AlignVCenter
    selectByMouse: true
    hoverEnabled: true
    Accessible.name: placeholderText.length ? placeholderText : tip
    // Long values (paths) show their beginning when the field isn't being edited.
    onTextChanged: if (!activeFocus) cursorPosition = 0
    onActiveFocusChanged: if (!activeFocus) cursorPosition = 0

    background: Rectangle {
        radius: Theme.radius.sm
        color: Theme.c.input
        border.width: f.activeFocus ? 1.5 : 1
        border.color: f.invalid ? Theme.c.danger : f.activeFocus ? Theme.accent : f.hovered ? Qt.tint(Theme.c.line, Theme.c.pressed) : Theme.c.line
        Behavior on border.color { ColorAnimation { duration: Theme.motion.fast } }
        Rectangle {
            anchors.fill: parent
            anchors.margins: -3
            radius: parent.radius + 3
            color: "transparent"
            border.width: 3
            border.color: Theme.soft(f.invalid ? Theme.c.danger : Theme.accent, 0.22)
            opacity: f.activeFocus ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: Theme.motion.base } }
        }
    }

    Icon {
        visible: f.iconName.length > 0
        name: f.iconName
        color: f.activeFocus ? Theme.accent : Theme.c.text3
        size: 15
        x: 11
        anchors.verticalCenter: parent.verticalCenter
    }
    Tip { visible: f.hovered && f.tip.length > 0 && !f.activeFocus; text: f.tip }
}
