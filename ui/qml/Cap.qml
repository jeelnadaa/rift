// Section caption: small, uppercase, tracked.
import QtQuick

Text {
    color: Theme.c.text3
    font.family: Theme.fontMono
    font.features: Theme.numFeatures
    font.pixelSize: Theme.type.caption
    font.weight: Font.DemiBold
    font.letterSpacing: Theme.type.caption * 0.07
    font.capitalization: Font.AllUppercase
    elide: Text.ElideRight
}
