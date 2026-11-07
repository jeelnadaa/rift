// Dialog with the Rift corner (14 4 14 4): scrim fades, sheet scales from .965 and rises 8 px.
// Put body items as children; put footer buttons in `footerItems`.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Popup {
    id: d
    property string title: ""
    property string subtitle: ""
    property string iconName: ""
    property int dialogWidth: 560
    property alias footerItems: footerRow.data
    property bool showFooter: true
    default property alias body: bodyCol.data

    parent: Overlay.overlay
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    anchors.centerIn: parent
    width: Math.min(dialogWidth, (parent ? parent.width : dialogWidth) - 48)
    height: Math.min(implicitHeight, (parent ? parent.height : 800) - 48)
    padding: 0

    Overlay.modal: Rectangle {
        color: Theme.c.overlay
        Behavior on opacity { NumberAnimation { duration: Theme.motion.base } }
    }

    background: Rectangle {
        color: Theme.c.raised
        topLeftRadius: Theme.radius.big
        bottomRightRadius: Theme.radius.big
        topRightRadius: Theme.radius.xs
        bottomLeftRadius: Theme.radius.xs
        border.color: Theme.dark ? Qt.rgba(1, 1, 1, 0.07) : Theme.c.line
        Rectangle {
            z: -1
            anchors.fill: parent
            anchors.topMargin: 14
            anchors.margins: -6
            radius: 18
            color: Theme.c.shadow
            opacity: 0.45
        }
    }

    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: Theme.motion.base }
            NumberAnimation { property: "scale"; from: Theme.reducedMotion ? 1 : 0.965; to: 1; duration: Theme.motion.slow; easing.type: Easing.BezierSpline; easing.bezierCurve: Theme.motion.spring }
        }
    }
    exit: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; to: 0; duration: Theme.motion.fast }
            NumberAnimation { property: "scale"; to: Theme.reducedMotion ? 1 : 0.98; duration: Theme.motion.fast }
        }
    }

    contentItem: ColumnLayout {
        spacing: 0
        // Header
        Item {
            Layout.fillWidth: true
            implicitHeight: d.title.length ? Math.max(60, head.implicitHeight + 32) : 0
            visible: d.title.length > 0
            RowLayout {
                id: head
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 12
                spacing: 12
                Rectangle {
                    visible: d.iconName.length > 0
                    width: 34; height: 34
                    radius: Theme.radius.md
                    color: Theme.accentSoft
                    Icon { anchors.centerIn: parent; name: d.iconName; color: Theme.accent; size: 18 }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text {
                        text: d.title
                        color: Theme.c.text
                        font.family: Theme.fontUi
                        font.pixelSize: 17
                        font.weight: Font.Bold
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    Text {
                        visible: d.subtitle.length > 0
                        text: d.subtitle
                        color: Theme.c.text3
                        font.family: Theme.fontUi
                        font.pixelSize: 13
                        Layout.fillWidth: true
                        elide: Text.ElideMiddle
                    }
                }
                Btn {
                    iconName: "x"
                    variant: "ghost"
                    small: true
                    tip: qsTr("Close")
                    kbd: "Esc"
                    onClicked: d.close()
                }
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.c.lineSoft }
        }
        // Body
        ScrollView {
            id: scroller
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: bodyCol.implicitHeight + 40
            contentWidth: availableWidth
            clip: true
            ColumnLayout {
                id: bodyCol
                x: 20
                y: 20
                width: scroller.availableWidth - 40
                spacing: 14
            }
        }
        // Footer
        Rectangle {
            visible: d.showFooter
            Layout.fillWidth: true
            implicitHeight: 60
            color: Theme.c.surface
            bottomRightRadius: Theme.radius.big
            bottomLeftRadius: Theme.radius.xs
            Rectangle { width: parent.width; height: 1; color: Theme.c.lineSoft }
            RowLayout {
                id: footerRow
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 16
                spacing: 8
            }
        }
    }
}
