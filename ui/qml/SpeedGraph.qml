// Speed graph: download (filled) and upload (line) from App.speedHistory ([{d,u}]).
import QtQuick

Canvas {
    id: g
    property var samples: []
    property color downColor: Theme.c.down
    property color upColor: Theme.c.up
    implicitHeight: 120
    implicitWidth: 400
    onSamplesChanged: requestPaint()
    onWidthChanged: requestPaint()
    onDownColorChanged: requestPaint()
    Accessible.role: Accessible.Chart
    Accessible.name: qsTr("Speed over the last minutes")

    readonly property real peak: {
        var m = 1024
        for (var i = 0; i < samples.length; ++i) m = Math.max(m, samples[i].d, samples[i].u)
        return m * 1.15
    }

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        var n = samples.length
        // grid
        ctx.strokeStyle = Theme.c.lineSoft
        ctx.lineWidth = 1
        for (var k = 1; k < 4; ++k) {
            var gy = Math.round(height * k / 4) + 0.5
            ctx.beginPath(); ctx.moveTo(0, gy); ctx.lineTo(width, gy); ctx.stroke()
        }
        if (n < 2) return
        var step = width / (Math.max(n, 60) - 1)
        var x0 = width - (n - 1) * step
        function y(v) { return height - 2 - (height - 4) * v / peak }
        // download area
        ctx.beginPath()
        ctx.moveTo(x0, height)
        for (var i = 0; i < n; ++i) ctx.lineTo(x0 + i * step, y(samples[i].d))
        ctx.lineTo(x0 + (n - 1) * step, height)
        ctx.closePath()
        var grad = ctx.createLinearGradient(0, 0, 0, height)
        grad.addColorStop(0, Qt.rgba(downColor.r, downColor.g, downColor.b, 0.32))
        grad.addColorStop(1, Qt.rgba(downColor.r, downColor.g, downColor.b, 0.02))
        ctx.fillStyle = grad
        ctx.fill()
        ctx.beginPath()
        for (i = 0; i < n; ++i) { var px = x0 + i * step, py = y(samples[i].d); if (i === 0) ctx.moveTo(px, py); else ctx.lineTo(px, py) }
        ctx.strokeStyle = downColor
        ctx.lineWidth = 1.75
        ctx.stroke()
        // upload line
        ctx.beginPath()
        for (i = 0; i < n; ++i) { px = x0 + i * step; py = y(samples[i].u); if (i === 0) ctx.moveTo(px, py); else ctx.lineTo(px, py) }
        ctx.strokeStyle = upColor
        ctx.lineWidth = 1.5
        ctx.setLineDash([4, 3])
        ctx.stroke()
    }
}
