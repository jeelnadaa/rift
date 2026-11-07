// Statistics: live speed graph, session totals and the last 30 days.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: sv
    color: Theme.c.surface
    property var days: []
    function reload() { days = App.dailyStats(30) }
    onVisibleChanged: if (visible) reload()
    Component.onCompleted: reload()

    readonly property real totalDown: { var s = 0; for (var i = 0; i < days.length; ++i) s += days[i].down; return s }
    readonly property real totalUp: { var s = 0; for (var i = 0; i < days.length; ++i) s += days[i].up; return s }
    readonly property real dayPeak: { var m = 1; for (var i = 0; i < days.length; ++i) m = Math.max(m, days[i].down, days[i].up); return m }

    component Stat: Rectangle {
        id: st
        property string label: ""
        property string value: ""
        property string iconName: ""
        property color tint: Theme.accent
        Layout.fillWidth: true
        implicitHeight: 92
        radius: Theme.radius.lg
        color: Theme.c.raised
        border.color: hov.hovered ? Theme.soft(Theme.accent, 0.35) : Theme.c.line
        transform: Translate { y: hov.hovered ? -2 : 0; Behavior on y { NumberAnimation { duration: Theme.motion.base; easing.type: Easing.OutCubic } } }
        HoverHandler { id: hov }
        Column {
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8
            Row {
                spacing: 6
                Icon { name: st.iconName; size: 14; color: st.tint; anchors.verticalCenter: parent.verticalCenter }
                Cap { text: st.label; anchors.verticalCenter: parent.verticalCenter }
            }
            Text {
                text: st.value
                color: Theme.c.text
                font.family: Theme.fontUi
                font.pixelSize: 26
                font.weight: Font.Bold
                font.letterSpacing: -0.6
            }
        }
    }

    ScrollView {
        id: sc
        anchors.fill: parent
        contentWidth: availableWidth
        ColumnLayout {
            x: 28
            width: sc.availableWidth - 56
            spacing: 18
            Text {
                Layout.topMargin: 24
                text: qsTr("Statistics")
                color: Theme.c.text
                font.family: Theme.fontUi
                font.pixelSize: Theme.type.page
                font.weight: Font.Bold
                font.letterSpacing: -0.8
            }
            GridLayout {
                Layout.fillWidth: true
                columns: sv.width > 900 ? 4 : 2
                columnSpacing: 12
                rowSpacing: 12
                Stat { label: qsTr("Downloading"); value: App.fmtSpeed(App.downSpeed); iconName: "arrowD"; tint: Theme.c.down }
                Stat { label: qsTr("Uploading"); value: App.fmtSpeed(App.upSpeed); iconName: "arrowUp2"; tint: Theme.c.up }
                Stat { label: qsTr("This session"); value: App.fmtBytes(App.sessionDown) + " ↓"; iconName: "clock" }
                Stat { label: qsTr("Last 30 days"); value: App.fmtBytes(sv.totalDown) + " ↓"; iconName: "calendar" }
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 240
                radius: Theme.radius.lg
                color: Theme.c.raised
                border.color: Theme.c.line
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 8
                    RowLayout {
                        Cap { text: qsTr("Speed, last 2 minutes"); Layout.fillWidth: true }
                        Text { text: "↓ " + App.fmtSpeed(App.downSpeed); color: Theme.c.down; font.family: Theme.fontMono; font.pixelSize: 12 }
                        Text { text: "↑ " + App.fmtSpeed(App.upSpeed); color: Theme.c.up; font.family: Theme.fontMono; font.pixelSize: 12 }
                    }
                    SpeedGraph { Layout.fillWidth: true; Layout.fillHeight: true; samples: App.speedHistory }
                }
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 230
                Layout.bottomMargin: 24
                radius: Theme.radius.lg
                color: Theme.c.raised
                border.color: Theme.c.line
                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 10
                    RowLayout {
                        Cap { text: qsTr("Daily traffic, last 30 days"); Layout.fillWidth: true }
                        Text { text: qsTr("%1 ↓ · %2 ↑").arg(App.fmtBytes(sv.totalDown)).arg(App.fmtBytes(sv.totalUp)); color: Theme.c.text3; font.family: Theme.fontMono; font.pixelSize: 12 }
                    }
                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Text {
                            anchors.centerIn: parent
                            visible: sv.days.length === 0
                            text: qsTr("Nothing yet. Daily totals appear after your first download.")
                            color: Theme.c.text3
                            font.family: Theme.fontUi
                            font.pixelSize: Theme.type.body
                        }
                        Row {
                            anchors.fill: parent
                            spacing: 4
                            Repeater {
                                model: sv.days
                                delegate: Item {
                                    id: bar
                                    required property var modelData
                                    width: (parent.width - 29 * 4) / 30
                                    height: parent.height
                                    Rectangle {
                                        anchors.bottom: parent.bottom
                                        width: parent.width * 0.6
                                        height: Math.max(2, parent.height * bar.modelData.down / sv.dayPeak)
                                        radius: 2
                                        color: Theme.c.down
                                    }
                                    Rectangle {
                                        anchors.bottom: parent.bottom
                                        anchors.right: parent.right
                                        width: parent.width * 0.35
                                        height: Math.max(1, parent.height * bar.modelData.up / sv.dayPeak)
                                        radius: 2
                                        color: Theme.c.up
                                        opacity: 0.8
                                    }
                                    HoverHandler { id: bh }
                                    Tip { visible: bh.hovered; text: bar.modelData.date + " · ↓ " + App.fmtBytes(bar.modelData.down) + " · ↑ " + App.fmtBytes(bar.modelData.up) }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
