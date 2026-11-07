// Status bar: ↓ total of limit · ↑ total · connection health · DHT · free disk · active / total.
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: sb
    property var win
    implicitHeight: Theme.size.statusbar
    color: Theme.c.sidebar
    Rectangle { width: parent.width; height: 1; color: Theme.c.line }

    readonly property real downLimit: App.settings.altSpeed ? App.settings.altDownLimit : App.settings.downLimit
    readonly property real upLimit: App.settings.altSpeed ? App.settings.altUpLimit : App.settings.upLimit
    readonly property color health: App.netState === "ok" ? Theme.c.success
                                  : App.netState === "firewalled" ? Theme.c.warning
                                  : App.netState === "offline" ? Theme.c.danger : Theme.c.text3

    component Cell: Item {
        id: cell
        property string iconName: ""
        property color iconColor: Theme.c.text3
        property string text: ""
        property string tip: ""
        property bool clickable: false
        signal clicked()
        implicitWidth: row.implicitWidth + 16
        Layout.fillHeight: true
        Rectangle {
            anchors.fill: parent
            anchors.topMargin: 4
            anchors.bottomMargin: 4
            radius: Theme.radius.xs
            color: cm.containsMouse && cell.clickable ? Theme.c.hover : "transparent"
        }
        Row {
            id: row
            anchors.centerIn: parent
            spacing: 6
            Icon { visible: cell.iconName.length > 0; name: cell.iconName; size: 13; color: cell.iconColor; anchors.verticalCenter: parent.verticalCenter }
            Text {
                text: cell.text
                color: Theme.c.text2
                font.family: Theme.fontMono
                font.pixelSize: 11
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        MouseArea {
            id: cm
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: cell.clickable ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: cell.clicked()
        }
        Tip { visible: cm.containsMouse && cell.tip.length > 0; text: cell.tip; y: -height - 6 }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 0
        Cell {
            iconName: "arrowD"; iconColor: Theme.c.down
            text: App.fmtSpeed(App.downSpeed) + (sb.downLimit > 0 ? qsTr(" of ") + App.fmtSpeed(sb.downLimit * 1024) : "")
            tip: qsTr("Download speed. Click to set limits.")
            clickable: true
            onClicked: sb.win.openLimits([])
        }
        Cell {
            iconName: "arrowUp2"; iconColor: Theme.c.up
            text: App.fmtSpeed(App.upSpeed) + (sb.upLimit > 0 ? qsTr(" of ") + App.fmtSpeed(sb.upLimit * 1024) : "")
            tip: qsTr("Upload speed. Click to set limits.")
            clickable: true
            onClicked: sb.win.openLimits([])
        }
        Cell {
            visible: App.settings.altSpeed
            iconName: "turtle"; iconColor: Theme.c.warning
            text: qsTr("Alt speed")
            tip: qsTr("Alternative speed limits are on. Click to turn them off.")
            clickable: true
            onClicked: App.setAltSpeed(false)
        }
        Item { Layout.fillWidth: true }
        Cell {
            iconName: "dot3v"; iconColor: sb.health
            text: App.netState === "ok" ? qsTr("Connected") : App.netState === "firewalled" ? qsTr("Firewalled")
                : App.netState === "offline" ? qsTr("Offline") : qsTr("Starting")
            tip: App.netState === "firewalled"
                 ? qsTr("No incoming connections reach port %1. Downloads still work, but slower. Allow it in your router or firewall.").arg(App.listenPort)
                 : qsTr("Listening on port %1").arg(App.listenPort)
        }
        Cell {
            iconName: "globe"
            text: qsTr("DHT %1").arg(App.dhtNodes)
            tip: qsTr("Distributed hash table nodes: how many peers Rift can ask for other peers")
        }
        Cell {
            visible: App.freeBytes >= 0
            iconName: "disk"
            iconColor: App.freeBytes >= 0 && App.freeBytes < App.settings.minFreeSpaceGB * 1073741824 ? Theme.c.danger : Theme.c.text3
            text: qsTr("%1 free").arg(App.fmtBytes(App.freeBytes))
            tip: qsTr("Free space in the download folder")
        }
        Cell {
            iconName: "bolt"
            text: App.activeCount + " / " + App.totalCount
            tip: qsTr("Active / total downloads")
        }
    }
}
