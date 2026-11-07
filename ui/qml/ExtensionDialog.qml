// Browser extension: Rift downloads the extension zip for this exact version from GitHub Releases (as a
// normal Rift download) and walks the user through installing it, for Chromium browsers and Firefox.
import QtQuick
import QtQuick.Layouts

RDialog {
    id: dlg
    title: qsTr("Browser extension")
    subtitle: qsTr("Adds “Download with Rift” to links in your browser")
    iconName: "puzzle"
    dialogWidth: 640

    property var links: ({})
    property string browser: "chromium"          // chromium | firefox
    property var started: ({})                   // browser -> true once its zip was added

    function show() {
        links = App.extensionDownloads()
        started = ({})
        open()
    }
    function fetchZip() {
        var url = links[browser]
        if (!links.release || !url) { Qt.openUrlExternally(links.page); return }
        App.addLinks(url, { category: "Programs", paused: false })
        var s = Object.assign({}, started); s[browser] = true; started = s
    }

    readonly property var steps: browser === "chromium" ? [
        [qsTr("Download the extension"), qsTr("Rift fetches %1 from GitHub Releases into your downloads folder.").arg(links.chromiumFile || "")],
        [qsTr("Unzip it into a folder you'll keep"), qsTr("For example Documents/Rift extension. The browser loads it from there, so don't delete it.")],
        [qsTr("Open the extensions page"), qsTr("Type chrome://extensions in Chrome or Brave, or edge://extensions in Edge. Turn on Developer mode.")],
        [qsTr("Click “Load unpacked”"), qsTr("Pick the folder from step 2. Right-click any link and choose “Download with Rift”.")]
    ] : [
        [qsTr("Download the extension"), qsTr("Rift fetches %1 from GitHub Releases into your downloads folder.").arg(links.firefoxFile || "")],
        [qsTr("Open the debugging page"), qsTr("Type about:debugging#/runtime/this-firefox in the address bar.")],
        [qsTr("Click “Load Temporary Add-on…”"), qsTr("Pick the downloaded zip. Right-click any link and choose “Download with Rift”.")],
        [qsTr("Until it's on Firefox Add-ons"), qsTr("Firefox removes temporary add-ons when it restarts, so repeat step 3 after a restart.")]
    ]
    readonly property string pageUrl: browser === "chromium" ? "chrome://extensions" : "about:debugging#/runtime/this-firefox"

    Seg {
        Layout.fillWidth: true
        options: [["chromium", qsTr("Chrome · Edge · Brave")], ["firefox", qsTr("Firefox")]]
        value: dlg.browser
        onActivated: (v) => dlg.browser = v
    }

    Repeater {
        model: dlg.steps
        delegate: RowLayout {
            id: step
            required property var modelData
            required property int index
            Layout.fillWidth: true
            Layout.topMargin: index === 0 ? 6 : 2
            spacing: 12
            opacity: 0
            Component.onCompleted: appear.start()
            NumberAnimation on opacity { id: appear; running: false; from: 0; to: 1; duration: Theme.motion.slow; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.standard }

            Rectangle {
                Layout.alignment: Qt.AlignTop
                implicitWidth: 26; implicitHeight: 26
                radius: 13
                color: step.index === 0 && dlg.started[dlg.browser] ? Theme.c.success : Theme.accentSoft
                Behavior on color { ColorAnimation { duration: Theme.motion.base } }
                Text {
                    anchors.centerIn: parent
                    text: step.index === 0 && dlg.started[dlg.browser] ? "✓" : String(step.index + 1)
                    color: step.index === 0 && dlg.started[dlg.browser] ? Theme.c.onAccent : Theme.accent
                    font.family: Theme.fontMono; font.features: Theme.numFeatures
                    font.pixelSize: 13; font.weight: Font.DemiBold
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    Layout.fillWidth: true
                    text: step.modelData[0]
                    color: Theme.c.text
                    font.pixelSize: Theme.type.body; font.weight: Font.DemiBold
                    wrapMode: Text.Wrap
                }
                Text {
                    Layout.fillWidth: true
                    text: step.modelData[1]
                    color: Theme.c.text2
                    font.pixelSize: Theme.type.small
                    wrapMode: Text.Wrap
                    lineHeight: 1.15
                }
            }
        }
    }

    Banner {
        Layout.fillWidth: true
        Layout.topMargin: 6
        visible: !dlg.links.release
        kind: "warn"
        closable: false
        text: qsTr("This is a development build, so there is no matching release. Get the extension from the latest release on GitHub.")
    }

    footerItems: [
        Btn {
            text: qsTr("Copy page address")
            iconName: "copy"
            tip: qsTr("Copy %1 to paste into the address bar").arg(dlg.pageUrl)
            onClicked: { App.copyText(dlg.pageUrl); App.toast(qsTr("Copied %1").arg(dlg.pageUrl), "ok", "") }
        },
        Btn {
            text: qsTr("Releases")
            iconName: "external"
            tip: qsTr("Open the release on GitHub")
            onClicked: Qt.openUrlExternally(dlg.links.page)
        },
        Item { Layout.fillWidth: true },
        Btn {
            variant: "primary"
            iconName: dlg.started[dlg.browser] ? "check" : "download"
            text: dlg.started[dlg.browser] ? qsTr("Downloading") : dlg.links.release ? qsTr("Download from GitHub") : qsTr("Open GitHub")
            enabled: !dlg.started[dlg.browser]
            tip: dlg.links.release ? qsTr("Download %1 with Rift").arg(dlg.browser === "chromium" ? dlg.links.chromiumFile : dlg.links.firefoxFile)
                                   : qsTr("Open the latest release in your browser")
            onClicked: dlg.fetchZip()
        }
    ]
}
