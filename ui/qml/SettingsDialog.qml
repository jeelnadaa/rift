// Settings window (960 × 680): section list on the left, settings on the right. Changes apply at once.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Popup {
    id: sd
    property var win
    property string section: "general"
    readonly property var s: App.settings
    readonly property var sections: [
        ["general", qsTr("General"), "home"], ["appearance", qsTr("Appearance"), "sparkle"],
        ["downloads", qsTr("Downloads"), "download"], ["speed", qsTr("Speed"), "speed"],
        ["bittorrent", qsTr("BitTorrent"), "magnet"], ["network", qsTr("Connection"), "wifi"],
        ["http", qsTr("Direct links"), "link"], ["browser", qsTr("Browser"), "puzzle"],
        ["advanced", qsTr("Advanced"), "terminal"]]

    function show(sec) { if (sec && sec.length) section = sec; open() }
    function set(k, v) { App.settings.set(k, v) }

    parent: Overlay.overlay
    modal: true
    focus: true
    anchors.centerIn: parent
    width: Math.min(960, parent ? parent.width - 48 : 960)
    height: Math.min(680, parent ? parent.height - 48 : 680)
    padding: 0
    closePolicy: Popup.CloseOnEscape
    Overlay.modal: Rectangle { color: Theme.c.overlay }
    background: Rectangle {
        color: Theme.c.raised
        topLeftRadius: Theme.radius.big; bottomRightRadius: Theme.radius.big
        topRightRadius: Theme.radius.xs; bottomLeftRadius: Theme.radius.xs
        border.color: Theme.dark ? Qt.rgba(1, 1, 1, 0.07) : Theme.c.line
    }
    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.motion.base }
            NumberAnimation { property: "scale"; from: Theme.reducedMotion ? 1 : 0.965; to: 1; duration: Theme.motion.slow; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.spring }
        }
    }
    exit: Transition { NumberAnimation { property: "opacity"; to: 0; duration: Theme.motion.fast } }

    contentItem: RowLayout {
        spacing: 0
        // Nav
        Rectangle {
            Layout.preferredWidth: 220
            Layout.fillHeight: true
            color: Theme.c.sidebar
            topLeftRadius: Theme.radius.big
            bottomLeftRadius: Theme.radius.xs
            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: 18
                spacing: 1
                Text {
                    Layout.leftMargin: 20
                    Layout.bottomMargin: 12
                    text: qsTr("Settings")
                    color: Theme.c.text
                    font.family: Theme.fontUi
                    font.pixelSize: 20
                    font.weight: Font.Bold
                    font.letterSpacing: -0.4
                }
                Repeater {
                    model: sd.sections
                    delegate: SideItem {
                        required property var modelData
                        Layout.fillWidth: true
                        iconName: modelData[2]
                        text: modelData[1]
                        selected: sd.section === modelData[0]
                        onClicked: sd.section = modelData[0]
                    }
                }
                Item { Layout.fillHeight: true }
                Text {
                    Layout.leftMargin: 20
                    Layout.bottomMargin: 16
                    text: "Rift " + App.version
                    color: Theme.c.text3
                    font.family: Theme.fontMono; font.features: Theme.numFeatures
                    font.pixelSize: 11
                }
            }
            Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.c.line }
        }

        // Pages
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                Layout.leftMargin: 28
                Layout.rightMargin: 14
                Text {
                    Layout.fillWidth: true
                    text: { for (var i = 0; i < sd.sections.length; ++i) if (sd.sections[i][0] === sd.section) return sd.sections[i][1]; return "" }
                    color: Theme.c.text
                    font.family: Theme.fontUi
                    font.pixelSize: Theme.type.section
                    font.weight: Font.Bold
                    font.variableAxes: Theme.displayAxes
                    font.letterSpacing: -0.4
                }
                Btn { iconName: "x"; variant: "ghost"; tip: qsTr("Close"); kbd: "Esc"; onClicked: sd.close() }
            }
            ScrollView {
                id: pageScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                contentWidth: availableWidth
                clip: true
                Loader {
                    width: pageScroll.availableWidth
                    sourceComponent: ({ general: general, appearance: appearance, downloads: downloads, speed: speed,
                                        bittorrent: bittorrent, network: network, http: http, browser: browser,
                                        advanced: advanced })[sd.section] || general
                }
            }
        }
    }

    // ---- Pages ----------------------------------------------------------------------------------
    component PageBody: ColumnLayout {
        x: 28
        width: (parent ? parent.width : 600) - 56
        spacing: 0
    }

    Component {
        id: general
        Item {
            implicitHeight: g.implicitHeight + 24
            PageBody {
                id: g
                SettingRow {
                    label: qsTr("Close button keeps Rift running")
                    help: App.trayAvailable ? qsTr("Rift stays in the system tray and keeps downloading.") : qsTr("No system tray is available on this desktop.")
                    Toggle { checked: sd.s.closeToTray; enabled: App.trayAvailable; tip: qsTr("Minimize to tray on close"); onToggled: sd.set("closeToTray", checked) }
                }
                SettingRow {
                    label: qsTr("Start with my computer")
                    help: qsTr("Open Rift in the background when you sign in.")
                    Toggle { checked: sd.s.launchAtLogin; tip: qsTr("Launch at login"); onToggled: sd.set("launchAtLogin", checked) }
                }
                SettingRow {
                    label: qsTr("Updates")
                    help: !App.updater.canInstall ? App.updater.installNote
                        : sd.s.updateMode === "notify" ? qsTr("Tell me when a new version is out, and update when I say so.")
                        : sd.s.updateMode === "off" ? qsTr("Never check on its own. You can still check by hand.")
                        : qsTr("Download new versions in the background and install them when Rift restarts.")
                    Select {
                        options: [["auto", qsTr("Automatic")], ["notify", qsTr("Ask first")], ["off", qsTr("Off")]]
                        value: sd.s.updateMode; tip: qsTr("How Rift updates itself"); onActivated: (v) => sd.set("updateMode", v)
                    }
                }
                SettingRow {
                    label: qsTr("Version %1").arg(App.version)
                    help: {
                        var u = App.updater
                        switch (u.state) {
                        case "checking": return qsTr("Checking for updates…")
                        case "uptodate": return qsTr("You have the latest version.") + (u.lastChecked > 0 ? " " + qsTr("Checked %1.").arg(App.fmtRelative(u.lastChecked)) : "")
                        case "available": return qsTr("Rift %1 is out.").arg(u.latest)
                        case "downloading": return qsTr("Downloading Rift %1… %2%").arg(u.latest).arg(Math.round(u.progress * 100))
                        case "ready": return qsTr("Rift %1 is ready and installs when Rift restarts.").arg(u.latest)
                        case "error": return u.error
                        case "dev": return qsTr("This is a development build, so it doesn't update itself.")
                        default: return qsTr("Rift checks GitHub Releases for new versions.")
                        }
                    }
                    Row {
                        spacing: 6
                        Btn {
                            visible: App.updater.state === "ready"
                            variant: "primary"; small: true; iconName: "refresh"
                            text: qsTr("Restart now"); tip: qsTr("Quit, install Rift %1 and start again").arg(App.updater.latest)
                            onClicked: App.updater.restartNow()
                        }
                        Btn {
                            visible: App.updater.state === "available" && App.updater.canInstall
                            variant: "primary"; small: true; iconName: "download"
                            text: qsTr("Update"); tip: qsTr("Download and install Rift %1").arg(App.updater.latest)
                            onClicked: App.updater.download()
                        }
                        Btn {
                            visible: App.updater.state === "available" && !App.updater.canInstall
                            small: true; iconName: "external"
                            text: qsTr("Release page"); tip: qsTr("Open the release on GitHub")
                            onClicked: Qt.openUrlExternally(App.updater.pageUrl)
                        }
                        Btn {
                            visible: App.updater.state !== "ready" && App.updater.state !== "dev"
                            enabled: App.updater.state !== "checking" && App.updater.state !== "downloading"
                            small: true; iconName: "refresh"
                            text: qsTr("Check now"); tip: qsTr("Check GitHub for a newer Rift")
                            onClicked: App.updater.check()
                        }
                    }
                }
                SettingRow {
                    label: qsTr("Start minimized")
                    Toggle { checked: sd.s.startMinimized; tip: qsTr("Start hidden in the tray"); onToggled: sd.set("startMinimized", checked) }
                }
                SettingRow {
                    label: qsTr("Watch the clipboard")
                    help: qsTr("Offer to download links and magnets you copy.")
                    Toggle { checked: sd.s.clipboardWatch; tip: qsTr("Clipboard offers"); onToggled: sd.set("clipboardWatch", checked) }
                }
                SettingRow {
                    label: qsTr("Notifications")
                    help: qsTr("Tell me when a download finishes.")
                    Toggle { checked: sd.s.notifications; tip: qsTr("Completion notifications"); onToggled: sd.set("notifications", checked) }
                }
                SettingRow {
                    label: qsTr("Notify about errors")
                    Toggle { checked: sd.s.notifyErrors; tip: qsTr("Error notifications"); onToggled: sd.set("notifyErrors", checked) }
                }
                SettingRow {
                    label: qsTr("Keep the computer awake")
                    help: qsTr("Prevent sleep while something is downloading.")
                    Toggle { checked: sd.s.preventSleep; tip: qsTr("Prevent sleep while downloading"); onToggled: sd.set("preventSleep", checked) }
                }
                SettingRow {
                    label: qsTr("Open magnet links and .torrent files with Rift")
                    help: App.isDefaultHandler() ? qsTr("Rift is the default app for magnet links.") : qsTr("Another app currently opens magnet links.")
                    Btn { text: qsTr("Make default"); iconName: "magnet"; tip: qsTr("Register Rift for magnet: and .torrent"); onClicked: App.registerHandlers() }
                }
            }
        }
    }

    Component {
        id: appearance
        Item {
            implicitHeight: a.implicitHeight + 24
            PageBody {
                id: a
                SettingRow {
                    label: qsTr("Theme")
                    help: qsTr("Stone is warm paper; Obsidian is near-black.")
                    Seg { options: [["system", qsTr("System")], ["stone", qsTr("Stone"), "sun"], ["obsidian", qsTr("Obsidian"), "moon"]]
                          value: sd.s.theme; onActivated: (v) => sd.set("theme", v) }
                }
                SettingRow {
                    label: qsTr("Accent")
                    help: qsTr("Used for selection, progress and the logo's right half.")
                    Row {
                        spacing: 8
                        Repeater {
                            model: Theme.accentNames
                            delegate: Rectangle {
                                id: sw
                                required property string modelData
                                width: 26; height: 26; radius: 13
                                color: Theme.accents[modelData][Theme.dark ? 1 : 0]
                                border.width: sd.s.accent === modelData ? 2 : 0
                                border.color: Theme.c.text
                                scale: swm.containsMouse ? 1.12 : 1
                                Behavior on scale { NumberAnimation { duration: Theme.motion.base; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.spring } }
                                Icon { anchors.centerIn: parent; visible: sd.s.accent === sw.modelData; name: "check"; size: 13; stroke: 2.4; color: "#FFFFFF" }
                                MouseArea { id: swm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: sd.set("accent", sw.modelData) }
                                Tip { visible: swm.containsMouse; text: sw.modelData.charAt(0).toUpperCase() + sw.modelData.slice(1) }
                                Accessible.role: Accessible.RadioButton
                                Accessible.name: modelData
                            }
                        }
                    }
                }
                SettingRow {
                    label: qsTr("Density")
                    Seg { options: [["compact", qsTr("Compact")], ["comfortable", qsTr("Comfortable")]]
                          value: sd.s.density; onActivated: (v) => sd.set("density", v) }
                }
                SettingRow {
                    label: qsTr("Reduce motion")
                    help: qsTr("Turn off movement and loops; keep short fades.")
                    Toggle { checked: sd.s.reducedMotion; tip: qsTr("Reduced motion"); onToggled: sd.set("reducedMotion", checked) }
                }
                SettingRow {
                    label: qsTr("Speed in the window title")
                    Toggle { checked: sd.s.showSpeedInTitle; tip: qsTr("Show ↓ ↑ in the title"); onToggled: sd.set("showSpeedInTitle", checked) }
                }
                SettingRow {
                    label: qsTr("Units")
                    help: qsTr("Bytes per second (MB/s) or bits per second (Mbit/s).")
                    Seg { options: [["bytes", "MB/s"], ["bits", "Mbit/s"]]; value: sd.s.speedUnits; onActivated: (v) => sd.set("speedUnits", v) }
                }
                SettingRow {
                    label: qsTr("Binary sizes")
                    help: qsTr("Show sizes as GiB/MiB (1024) instead of GB/MB (1000).")
                    Toggle { checked: sd.s.binaryUnits; tip: qsTr("Binary units"); onToggled: sd.set("binaryUnits", checked) }
                }
            }
        }
    }

    Component {
        id: downloads
        Item {
            implicitHeight: dbody.implicitHeight + 24
            PageBody {
                id: dbody
                SettingRow {
                    label: qsTr("Save downloads to")
                    stacked: true
                    RowLayout {
                        width: parent.width
                        Field { id: savePath; Layout.fillWidth: true; iconName: "folder"; text: sd.s.savePath; onEditingFinished: sd.set("savePath", text) }
                        Btn { text: qsTr("Browse…"); tip: qsTr("Choose a folder"); onClicked: sd.win.pickFolder(function (p) { sd.set("savePath", p) }) }
                    }
                }
                SettingRow {
                    label: qsTr("A folder per category")
                    help: qsTr("Put files in “%1/Video”, “%1/Music”… when the category has no folder of its own.").arg(sd.s.savePath)
                    Toggle { checked: sd.s.categoryFolders; tip: qsTr("Category subfolders"); onToggled: sd.set("categoryFolders", checked) }
                }
                SettingRow {
                    label: qsTr("Keep unfinished torrents elsewhere")
                    help: qsTr("Move them to the save folder when they finish.")
                    Toggle { checked: sd.s.useIncompletePath; tip: qsTr("Separate folder for incomplete torrents"); onToggled: sd.set("useIncompletePath", checked) }
                }
                SettingRow {
                    visible: sd.s.useIncompletePath
                    label: qsTr("Unfinished torrents go to")
                    stacked: true
                    RowLayout {
                        width: parent.width
                        Field { Layout.fillWidth: true; iconName: "folder"; text: sd.s.incompletePath; onEditingFinished: sd.set("incompletePath", text) }
                        Btn { text: qsTr("Browse…"); tip: qsTr("Choose a folder"); onClicked: sd.win.pickFolder(function (p) { sd.set("incompletePath", p) }) }
                    }
                }
                SettingRow {
                    label: qsTr("Choose files before a torrent downloads")
                    Toggle { checked: sd.s.pickFiles; tip: qsTr("Ask which files to download for every magnet link and .torrent"); onToggled: sd.set("pickFiles", checked) }
                }
                SettingRow {
                    label: qsTr("Add downloads paused")
                    Toggle { checked: sd.s.startPaused; tip: qsTr("Start paused"); onToggled: sd.set("startPaused", checked) }
                }
                SettingRow {
                    label: qsTr("Download torrents in order by default")
                    Toggle { checked: sd.s.sequentialDefault; tip: qsTr("Sequential by default"); onToggled: sd.set("sequentialDefault", checked) }
                }
                SettingRow {
                    label: qsTr("Delete .torrent files after adding")
                    Toggle { checked: sd.s.deleteTorrentAfterAdd; tip: qsTr("Remove the .torrent file once added"); onToggled: sd.set("deleteTorrentAfterAdd", checked) }
                }
                SettingRow {
                    label: qsTr("Pause when free space is below")
                    NumField { value: sd.s.minFreeSpaceGB; to: 1000; suffix: "GB"; tip: qsTr("0 turns this off"); onEdited: (v) => sd.set("minFreeSpaceGB", v) }
                }
            }
        }
    }

    Component {
        id: speed
        Item {
            implicitHeight: spb.implicitHeight + 24
            PageBody {
                id: spb
                SettingRow {
                    label: qsTr("Download limit")
                    help: qsTr("Shared by torrents and direct links. Rift splits it so neither starves.")
                    NumField { value: sd.s.downLimit; to: 10000000; step: 100; suffix: "KB/s"; zeroText: qsTr("Unlimited"); tip: qsTr("0 means unlimited"); onEdited: (v) => sd.set("downLimit", v) }
                }
                SettingRow {
                    label: qsTr("Upload limit")
                    NumField { value: sd.s.upLimit; to: 10000000; step: 50; suffix: "KB/s"; zeroText: qsTr("Unlimited"); tip: qsTr("0 means unlimited"); onEdited: (v) => sd.set("upLimit", v) }
                }
                SettingRow {
                    label: qsTr("Alternative download limit")
                    help: qsTr("Used while the turtle (Alt speed) is on.")
                    NumField { value: sd.s.altDownLimit; to: 10000000; step: 100; suffix: "KB/s"; zeroText: qsTr("Unlimited"); tip: qsTr("Alt speed download"); onEdited: (v) => sd.set("altDownLimit", v) }
                }
                SettingRow {
                    label: qsTr("Alternative upload limit")
                    NumField { value: sd.s.altUpLimit; to: 10000000; step: 50; suffix: "KB/s"; zeroText: qsTr("Unlimited"); tip: qsTr("Alt speed upload"); onEdited: (v) => sd.set("altUpLimit", v) }
                }
            }
        }
    }

    Component {
        id: bittorrent
        Item {
            implicitHeight: btb.implicitHeight + 24
            PageBody {
                id: btb
                SettingRow { label: qsTr("DHT"); help: qsTr("Find peers without trackers."); Toggle { checked: sd.s.dht; tip: qsTr("Distributed hash table"); onToggled: sd.set("dht", checked) } }
                SettingRow { label: qsTr("Peer exchange"); help: qsTr("Learn about peers from other peers."); Toggle { checked: sd.s.pex; tip: qsTr("PEX"); onToggled: sd.set("pex", checked) } }
                SettingRow { label: qsTr("Local peer discovery"); help: qsTr("Find peers on your local network."); Toggle { checked: sd.s.lsd; tip: qsTr("LSD"); onToggled: sd.set("lsd", checked) } }
                SettingRow {
                    label: qsTr("Encryption")
                    Select {
                        options: [[0, qsTr("Prefer encrypted")], [1, qsTr("Require encrypted")], [2, qsTr("Allow unencrypted")]]
                        value: sd.s.encryption; tip: qsTr("Peer connection encryption"); onActivated: (v) => sd.set("encryption", v)
                    }
                }
                SettingRow { label: qsTr("Anonymous mode"); help: qsTr("Hide the client name and version from peers and trackers."); Toggle { checked: sd.s.anonymous; tip: qsTr("Anonymous mode"); onToggled: sd.set("anonymous", checked) } }
                SettingRow { label: qsTr("Queue torrents"); help: qsTr("Only run a few at a time; the rest wait their turn."); Toggle { checked: sd.s.queueing; tip: qsTr("Queueing"); onToggled: sd.set("queueing", checked) } }
                SettingRow { visible: sd.s.queueing; label: qsTr("Active downloads"); NumField { value: sd.s.activeDownloads; from: 1; to: 100; tip: qsTr("Torrents downloading at once"); onEdited: (v) => sd.set("activeDownloads", v) } }
                SettingRow { visible: sd.s.queueing; label: qsTr("Active seeds"); NumField { value: sd.s.activeSeeds; from: 1; to: 100; tip: qsTr("Torrents seeding at once"); onEdited: (v) => sd.set("activeSeeds", v) } }
                SettingRow { visible: sd.s.queueing; label: qsTr("Active in total"); NumField { value: sd.s.activeLimit; from: 1; to: 200; tip: qsTr("Torrents active at once"); onEdited: (v) => sd.set("activeLimit", v) } }
                SettingRow {
                    label: qsTr("Stop seeding at ratio")
                    help: qsTr("0 seeds forever.")
                    NumField { value: sd.s.seedRatioLimit; to: 100; step: 0.1; decimals: 1; tip: qsTr("Upload ÷ download"); onEdited: (v) => sd.set("seedRatioLimit", v) }
                }
                SettingRow {
                    visible: sd.s.seedRatioLimit > 0
                    label: qsTr("When the ratio is reached")
                    Seg { options: [["pause", qsTr("Pause")], ["remove", qsTr("Remove")]]; value: sd.s.seedRatioAction; onActivated: (v) => sd.set("seedRatioAction", v) }
                }
            }
        }
    }

    Component {
        id: network
        Item {
            implicitHeight: nb.implicitHeight + 24
            PageBody {
                id: nb
                SettingRow {
                    label: qsTr("Listening port")
                    help: qsTr("Incoming BitTorrent connections. Currently %1.").arg(App.listenPort)
                    NumField { value: sd.s.listenPort; from: 1024; to: 65535; tip: qsTr("TCP and UDP port"); onEdited: (v) => sd.set("listenPort", v) }
                }
                SettingRow { label: qsTr("UPnP"); help: qsTr("Ask the router to forward the port."); Toggle { checked: sd.s.upnp; tip: qsTr("UPnP port mapping"); onToggled: sd.set("upnp", checked) } }
                SettingRow { label: qsTr("NAT-PMP"); Toggle { checked: sd.s.natpmp; tip: qsTr("NAT-PMP port mapping"); onToggled: sd.set("natpmp", checked) } }
                SettingRow { label: qsTr("Most connections"); NumField { value: sd.s.maxConnections; from: 10; to: 5000; tip: qsTr("Peer connections in total"); onEdited: (v) => sd.set("maxConnections", v) } }
                SettingRow { label: qsTr("Most connections per torrent"); NumField { value: sd.s.maxConnectionsPerTorrent; from: 2; to: 1000; tip: qsTr("Peer connections per torrent"); onEdited: (v) => sd.set("maxConnectionsPerTorrent", v) } }
                SettingRow { label: qsTr("Upload slots"); NumField { value: sd.s.maxUploads; from: 1; to: 500; tip: qsTr("Peers you upload to at once"); onEdited: (v) => sd.set("maxUploads", v) } }
                SettingRow {
                    label: qsTr("Proxy")
                    Select {
                        options: [[0, qsTr("None")], [1, "SOCKS5"], [2, "HTTP"]]
                        value: sd.s.proxyType; tip: qsTr("Proxy for peers, trackers and direct links"); onActivated: (v) => sd.set("proxyType", v)
                    }
                }
                SettingRow {
                    visible: sd.s.proxyType > 0
                    label: qsTr("Proxy address")
                    RowLayout {
                        Field { Layout.preferredWidth: 220; text: sd.s.proxyHost; placeholderText: qsTr("Host"); onEditingFinished: sd.set("proxyHost", text) }
                        NumField { value: sd.s.proxyPort; from: 1; to: 65535; tip: qsTr("Port"); onEdited: (v) => sd.set("proxyPort", v) }
                    }
                }
                SettingRow {
                    label: qsTr("Network interface")
                    help: qsTr("Bind BitTorrent to one interface (for example a VPN). Empty uses all.")
                    Field { Layout.preferredWidth: 220; text: sd.s.networkInterface; placeholderText: qsTr("Any"); onEditingFinished: sd.set("networkInterface", text) }
                }
            }
        }
    }

    Component {
        id: http
        Item {
            implicitHeight: hb.implicitHeight + 24
            PageBody {
                id: hb
                SettingRow {
                    label: qsTr("Connections per file")
                    help: qsTr("Rift opens them gradually and stops adding when they no longer help.")
                    NumField { value: sd.s.httpConnections; from: 1; to: 32; tip: qsTr("Parallel connections"); onEdited: (v) => sd.set("httpConnections", v) }
                }
                SettingRow { label: qsTr("Files at once"); NumField { value: sd.s.httpMaxActive; from: 1; to: 20; tip: qsTr("Direct downloads running together"); onEdited: (v) => sd.set("httpMaxActive", v) } }
                SettingRow { label: qsTr("Smallest piece"); help: qsTr("Rift won't split a file into parts smaller than this."); NumField { value: sd.s.httpMinSegmentMB; from: 1; to: 64; suffix: "MB"; tip: qsTr("Minimum segment"); onEdited: (v) => sd.set("httpMinSegmentMB", v) } }
                SettingRow { label: qsTr("Treat as stalled after"); NumField { value: sd.s.httpStallSeconds; from: 5; to: 300; suffix: "s"; tip: qsTr("Seconds without data before reconnecting"); onEdited: (v) => sd.set("httpStallSeconds", v) } }
                SettingRow { label: qsTr("Retries"); NumField { value: sd.s.httpRetries; from: 0; to: 100; tip: qsTr("Attempts before giving up"); onEdited: (v) => sd.set("httpRetries", v) } }
                SettingRow { label: qsTr("Verify TLS certificates"); help: qsTr("Turn off only for servers you trust."); Toggle { checked: sd.s.httpVerifyTls; tip: qsTr("Certificate checks"); onToggled: sd.set("httpVerifyTls", checked) } }
                SettingRow {
                    label: qsTr("User agent")
                    stacked: true
                    Field { width: parent.width; text: sd.s.httpUserAgent; placeholderText: "Rift/" + App.version; onEditingFinished: sd.set("httpUserAgent", text) }
                }
            }
        }
    }

    Component {
        id: browser
        Item {
            implicitHeight: bb.implicitHeight + 24
            PageBody {
                id: bb
                Banner {
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    kind: "info"
                    closable: false
                    text: qsTr("Install the Rift extension for Chrome, Edge, Brave or Firefox to send downloads here. Rift registers itself with your browsers automatically.")
                    actionText: qsTr("Get extension")
                    onAction: sd.win.openExtension()
                }
                SettingRow { label: qsTr("Accept downloads from the browser"); Toggle { checked: sd.s.browserEnabled; tip: qsTr("Browser integration"); onToggled: sd.set("browserEnabled", checked) } }
                SettingRow {
                    label: qsTr("Take over downloads")
                    help: qsTr("Catch matching downloads in the browser and hand them to Rift.")
                    Toggle { checked: sd.s.browserTakeOver; tip: qsTr("Intercept browser downloads"); onToggled: sd.set("browserTakeOver", checked) }
                }
                SettingRow {
                    label: qsTr("File types to take over")
                    stacked: true
                    Field { width: parent.width; text: sd.s.browserTypes; onEditingFinished: sd.set("browserTypes", text) }
                }
                SettingRow { label: qsTr("Only files bigger than"); NumField { value: sd.s.browserMinMB; to: 100000; suffix: "MB"; tip: qsTr("Smaller files stay in the browser"); onEdited: (v) => sd.set("browserMinMB", v) } }
                SettingRow { label: qsTr("Ask before adding"); help: qsTr("Show the add dialog for each download from the browser."); Toggle { checked: sd.s.browserConfirm; tip: qsTr("Confirm browser downloads"); onToggled: sd.set("browserConfirm", checked) } }
            }
        }
    }

    Component {
        id: advanced
        Item {
            implicitHeight: ab.implicitHeight + 24
            PageBody {
                id: ab
                SettingRow {
                    label: qsTr("Data folder")
                    help: App.portable ? qsTr("Portable mode: everything lives next to Rift.") : App.dataDir
                    Btn { text: qsTr("Open"); iconName: "folderOpen"; tip: qsTr("Open the data folder"); onClicked: Qt.openUrlExternally(App.pathToUrl(App.dataDir)) }
                }
                SettingRow {
                    label: qsTr("Copy diagnostics")
                    help: qsTr("Versions, paths and engine state, for bug reports. No personal data.")
                    Btn { text: qsTr("Copy"); iconName: "copy"; tip: qsTr("Copy diagnostics to the clipboard"); onClicked: { App.copyText(App.diagnostics()); sd.win.toast(qsTr("Diagnostics copied"), "ok", "") } }
                }
                SettingRow {
                    label: qsTr("Export settings")
                    Btn { text: qsTr("Copy as JSON"); iconName: "upload"; tip: qsTr("Copy all settings to the clipboard"); onClicked: { App.copyText(App.settings.exportJson()); sd.win.toast(qsTr("Settings copied as JSON"), "ok", "") } }
                }
                SettingRow {
                    label: qsTr("Import settings")
                    help: qsTr("Paste JSON exported from another Rift.")
                    Btn {
                        text: qsTr("Import…"); iconName: "download"; tip: qsTr("Paste settings JSON")
                        onClicked: sd.win.prompt(qsTr("Import settings"), qsTr("Settings JSON"), "", function (v) {
                            sd.win.toast(App.settings.importJson(v) ? qsTr("Settings imported") : qsTr("That isn't valid settings JSON"), "info", "")
                        })
                    }
                }
                SettingRow {
                    label: qsTr("Reset all settings")
                    help: qsTr("Downloads and history are kept.")
                    Btn { text: qsTr("Reset"); variant: "danger"; iconName: "undo"; tip: qsTr("Restore every default"); onClicked: { App.settings.resetAll(); sd.win.toast(qsTr("Settings reset"), "info", "") } }
                }
                SettingRow {
                    label: qsTr("Engines")
                    help: App.engineVersions
                    Item { width: 1; height: 1 }
                }
            }
        }
    }
}
