// Section caption: small, uppercase, tracked.
import QtQuick

Text {
    color: Theme.c.text3
    font.family: Theme.fontUi
    font.pixelSize: Theme.type.caption
    font.weight: Font.Bold
    font.letterSpacing: 0.9
    font.capitalization: Font.AllUppercase
    elide: Text.ElideRight
}
