// About Rift: version, engines, links.
import QtQuick
import QtQuick.Layouts

RDialog {
    id: dlg
    dialogWidth: 440
    showFooter: true

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 10
        Logo { size: 54; Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 6 }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Rift"
            color: Theme.c.text
            font.family: Theme.fontUi
            font.pixelSize: 32
            font.weight: Font.ExtraBold
            font.letterSpacing: -1.6
            font.variableAxes: Theme.displayAxes
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Version %1").arg(App.version)
            color: Theme.c.text2
            font.family: Theme.fontMono; font.features: Theme.numFeatures
            font.pixelSize: 13
        }
        Text {
            // Update status under the version; the controls live in Settings, General.
            readonly property var u: App.updater
            Layout.alignment: Qt.AlignHCenter
            visible: text.length > 0
            text: u.state === "ready" ? qsTr("Rift %1 installs when Rift restarts.").arg(u.latest)
                : u.state === "available" ? qsTr("Rift %1 is out.").arg(u.latest)
                : u.state === "downloading" ? qsTr("Downloading Rift %1… %2%").arg(u.latest).arg(Math.round(u.progress * 100))
                : u.state === "uptodate" ? qsTr("Up to date")
                : u.state === "checking" ? qsTr("Checking for updates…")
                : ""
            color: u.state === "ready" || u.state === "available" ? Theme.accent : Theme.c.text3
            font.family: Theme.fontUi
            font.pixelSize: 12
            font.weight: Font.Medium
        }
        Text {
            Layout.fillWidth: true
            Layout.topMargin: 4
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: qsTr("Torrents and direct links in one calm, fast window.")
            color: Theme.c.text2
            font.family: Theme.fontSerif
            font.italic: true
            font.pixelSize: 19
        }
        Text {
            Layout.fillWidth: true
            Layout.topMargin: 6
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: App.engineVersions
            color: Theme.c.text3
            font.family: Theme.fontMono; font.features: Theme.numFeatures
            font.pixelSize: 11
        }
    }

    footerItems: [
        Btn { text: qsTr("Website"); iconName: "external"; tip: qsTr("Open rift.solarquack.in"); onClicked: Qt.openUrlExternally("https://rift.solarquack.in") },
        Btn { text: qsTr("Source"); iconName: "terminal"; tip: qsTr("Open the source code on GitHub"); onClicked: Qt.openUrlExternally("https://github.com/jeelnadaa/rift") },
        Item { Layout.fillWidth: true },
        Btn { variant: "primary"; text: qsTr("Close"); tip: qsTr("Close"); kbd: "Esc"; onClicked: dlg.close() }
    ]
}
