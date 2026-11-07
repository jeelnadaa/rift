// Remove downloads. Files are kept unless the user explicitly ticks "Also delete files".
import QtQuick
import QtQuick.Layouts

RDialog {
    id: dlg
    property var ids: []
    title: ids.length === 1 ? qsTr("Remove download?") : qsTr("Remove %1 downloads?").arg(ids.length)
    subtitle: ids.length === 1 && App.detail.name ? App.detail.name : ""
    iconName: "trash"
    dialogWidth: 460

    function show(list) {
        ids = list.slice()
        del.checked = false
        open()
    }

    Text {
        Layout.fillWidth: true
        wrapMode: Text.Wrap
        text: del.checked ? qsTr("The downloaded files will be deleted from disk. This can't be undone.")
                          : qsTr("Removes it from the list. Downloaded files stay where they are, and you can undo for a few seconds.")
        color: del.checked ? Theme.c.danger : Theme.c.text2
        font.family: Theme.fontUi
        font.pixelSize: Theme.type.body
    }
    Check { id: del; text: qsTr("Also delete files"); tip: qsTr("Delete the downloaded data too") }

    footerItems: [
        Item { Layout.fillWidth: true },
        Btn { text: qsTr("Cancel"); tip: qsTr("Keep everything"); kbd: "Esc"; onClicked: dlg.close() },
        Btn {
            variant: "danger"
            iconName: "trash"
            text: del.checked ? qsTr("Remove and delete") : qsTr("Remove")
            tip: del.checked ? qsTr("Remove and delete the files") : qsTr("Remove from the list, keep files")
            onClicked: { App.remove(dlg.ids, del.checked); dlg.close() }
        }
    ]
}
