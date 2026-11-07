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
            font.letterSpacing: -1.4
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Version %1").arg(App.version)
            color: Theme.c.text2
            font.family: Theme.fontMono
            font.pixelSize: 13
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
            font.family: Theme.fontMono
            font.pixelSize: 11.5
        }
    }

    footerItems: [
        Btn { text: qsTr("Website"); iconName: "external"; tip: qsTr("Open rift.app"); onClicked: Qt.openUrlExternally("https://rift.app") },
        Btn { text: qsTr("Source"); iconName: "terminal"; tip: qsTr("Open the source code on GitHub"); onClicked: Qt.openUrlExternally("https://github.com/jeelnadaa/rift") },
        Item { Layout.fillWidth: true },
        Btn { variant: "primary"; text: qsTr("Close"); tip: qsTr("Close"); kbd: "Esc"; onClicked: dlg.close() }
    ]
}
