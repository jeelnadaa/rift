// Piece availability strip: N buckets 0..255 drawn on a canvas (cheap at any width).
import QtQuick

Canvas {
    id: c
    property var pieces: []
    property color fill: Theme.accent
    property color empty: Theme.c.pieceMissing
    implicitHeight: 18
    implicitWidth: 300
    onPiecesChanged: requestPaint()
    onFillChanged: requestPaint()
    onWidthChanged: requestPaint()
    Accessible.role: Accessible.Graphic
    Accessible.name: qsTr("Pieces")

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        var n = pieces.length
        ctx.fillStyle = empty
        ctx.fillRect(0, 0, width, height)
        if (n === 0) return
        var w = width / n
        for (var i = 0; i < n; ++i) {
            var v = pieces[i] / 255
            if (v <= 0) continue
            ctx.globalAlpha = 0.25 + 0.75 * v
            ctx.fillStyle = fill
            ctx.fillRect(Math.floor(i * w), 0, Math.ceil(w), height)
        }
        ctx.globalAlpha = 1
    }
}
