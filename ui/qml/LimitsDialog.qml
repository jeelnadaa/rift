// Speed limits: global (ids empty) or per-download. 0 = unlimited. Values are KB/s.
import QtQuick
import QtQuick.Layouts

RDialog {
    id: dlg
    property var ids: []
    readonly property bool isGlobal: ids.length === 0
    title: isGlobal ? qsTr("Speed limits") : ids.length === 1 ? qsTr("Limits for this download") : qsTr("Limits for %1 downloads").arg(ids.length)
    subtitle: isGlobal ? qsTr("Shared by torrents and direct links") : ""
    iconName: "speed"
    dialogWidth: 480

    function show(list) {
        ids = list.slice()
        if (isGlobal) {
            down.value = App.settings.downLimit; up.value = App.settings.upLimit
            altDown.value = App.settings.altDownLimit; altUp.value = App.settings.altUpLimit
        } else {
            down.value = 0; up.value = 0; conns.value = App.settings.httpConnections
        }
        open()
    }
    function apply() {
        if (isGlobal) {
            App.settings.set("downLimit", down.value); App.settings.set("upLimit", up.value)
            App.settings.set("altDownLimit", altDown.value); App.settings.set("altUpLimit", altUp.value)
        } else {
            App.setLimits(ids, down.value, up.value)
            if (App.detail.kind === "http" || ids.length > 1) App.setConnections(ids, conns.value)
        }
        close()
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 2
        columnSpacing: 14
        rowSpacing: 10
        Cap { text: dlg.isGlobal ? qsTr("Normal") : qsTr("Limits"); Layout.columnSpan: 2 }
        Text { text: qsTr("Download"); color: Theme.c.text2; font.family: Theme.fontUi; font.pixelSize: 13 }
        NumField { id: down; suffix: "KB/s"; zeroText: qsTr("Unlimited"); to: 10000000; step: 100; tip: qsTr("0 means unlimited") }
        Text { text: qsTr("Upload"); color: Theme.c.text2; font.family: Theme.fontUi; font.pixelSize: 13 }
        NumField { id: up; suffix: "KB/s"; zeroText: qsTr("Unlimited"); to: 10000000; step: 50; tip: qsTr("0 means unlimited") }

        Cap { visible: dlg.isGlobal; text: qsTr("Alternative (turtle)"); Layout.columnSpan: 2; Layout.topMargin: 8 }
        Text { visible: dlg.isGlobal; text: qsTr("Download"); color: Theme.c.text2; font.family: Theme.fontUi; font.pixelSize: 13 }
        NumField { id: altDown; visible: dlg.isGlobal; suffix: "KB/s"; zeroText: qsTr("Unlimited"); to: 10000000; step: 100; tip: qsTr("Used while Alt speed is on") }
        Text { visible: dlg.isGlobal; text: qsTr("Upload"); color: Theme.c.text2; font.family: Theme.fontUi; font.pixelSize: 13 }
        NumField { id: altUp; visible: dlg.isGlobal; suffix: "KB/s"; zeroText: qsTr("Unlimited"); to: 10000000; step: 50; tip: qsTr("Used while Alt speed is on") }

        Text { visible: !dlg.isGlobal; text: qsTr("Connections"); color: Theme.c.text2; font.family: Theme.fontUi; font.pixelSize: 13 }
        NumField { id: conns; visible: !dlg.isGlobal; from: 1; to: 32; tip: qsTr("Most parallel connections for direct links") }
    }

    footerItems: [
        Item { Layout.fillWidth: true },
        Btn { text: qsTr("Cancel"); tip: qsTr("Keep the current limits"); kbd: "Esc"; onClicked: dlg.close() },
        Btn { variant: "primary"; text: qsTr("Apply"); iconName: "check"; tip: qsTr("Use these limits"); onClicked: dlg.apply() }
    ]
}
