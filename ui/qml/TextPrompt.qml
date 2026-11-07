// Small one-field prompt (new category, new tag, folder path…).
import QtQuick
import QtQuick.Layouts

RDialog {
    id: dlg
    property var callback: null
    property string label: ""
    dialogWidth: 420
    iconName: "edit"

    function ask(t, l, value, cb) {
        title = t
        label = l
        field.text = value || ""
        callback = cb
        open()
        field.forceActiveFocus()
        field.selectAll()
    }
    function accept() {
        var cb = callback
        close()
        if (cb) cb(field.text.trim())
    }

    Field {
        id: field
        Layout.fillWidth: true
        placeholderText: dlg.label
        onAccepted: dlg.accept()
    }
    footerItems: [
        Item { Layout.fillWidth: true },
        Btn { text: qsTr("Cancel"); tip: qsTr("Close"); kbd: "Esc"; onClicked: dlg.close() },
        Btn { variant: "primary"; text: qsTr("OK"); tip: qsTr("Save"); kbd: "Enter"; enabled: field.text.trim().length > 0; onClicked: dlg.accept() }
    ]
}
