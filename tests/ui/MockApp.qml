// Mock of the C++ Controller ("App" in QML) for the UI harness. Same property and method names.
// `settings` and `list` are set from C++ to the real Settings and DownloadFilter objects.
import QtQuick

QtObject {
    id: app
    signal ticked()
    signal clipboardChanged()
    signal toast(string text, string kind, string action)
    signal requestAdd(string text, var meta)
    signal requestAddTorrent(string path)
    signal requestPickFiles(string rowId)
    signal showWindow()
    signal quitRequested()

    property QtObject settings
    property QtObject list
    readonly property string version: "0.0.0-harness"
    readonly property string engineVersions: "libtorrent (mock) · libcurl (mock) · Qt " + Qt.application.version
    readonly property bool portable: false
    readonly property string dataDir: "/tmp/rift-harness"
    property real downSpeed: 6.2e6
    property real upSpeed: 2.2e5
    property int activeCount: 3
    property int totalCount: 6
    property int downloadingCount: 2
    property real overallProgress: 0.5
    property int dhtNodes: 312
    property int listenPort: 51413
    property string netState: "firewalled"
    property real freeBytes: 120e9
    property real sessionDown: 3.4e9
    property real sessionUp: 0.8e9
    property var speedHistory: {
        var a = []
        for (var i = 0; i < 120; ++i) a.push({ d: 4e6 + 2e6 * Math.sin(i / 9), u: 2e5 + 1e5 * Math.cos(i / 7) })
        return a
    }
    property var counts: ({ all: 6, downloading: 2, seeding: 1, completed: 2, paused: 1, active: 3, inactive: 3,
                            stalled: 0, checking: 0, error: 1, "k:bt": 3, "k:http": 3 })
    property var categories: [{ name: "Video", count: 3 }, { name: "Archives", count: 2 }, { name: "Documents", count: 1 }]
    property var tags: [{ name: "linux", count: 3 }]
    property var sites: [{ name: "example.com", count: 3 }, { name: "tracker.example.org", count: 3 }]
    property var selection: []
    property string current: ""
    property var detail: ({})
    property var files: []
    property var peers: []
    property var trackers: []
    property var pieces: []
    property string detailTab: "overview"
    property string clipboardOffer: "magnet:?xt=urn:btih:0123456789abcdef0123456789abcdef01234567"
    readonly property bool trayAvailable: true

    function selectFirst() { select(["t:aaaa"], "t:aaaa") }
    function select(ids, cur) {
        selection = ids
        current = cur && cur.length ? cur : (ids.length === 1 ? ids[0] : "")
        var p = []
        for (var i = 0; i < 120; ++i) p.push((i * 37) % 256)
        detail = current.length ? {
            id: current, kind: current.charAt(0) === "t" ? "bt" : "http", name: "ubuntu-26.04-desktop-amd64.iso",
            state: "downloading", size: 4.2e9, done: 1.8e9, progress: 0.42, down: 5.4e6, up: 1e5, eta: 420,
            ratio: 0.1, seeds: 12, peers: 30, seedsTotal: 200, peersTotal: 90, conns: 6, category: "Video",
            tags: ["linux"], site: "example.com", path: "/tmp/x", error: "", added: Date.now() - 3600000,
            completed: 0, availability: 3.2, isPrivate: false, sequential: false, segs: [], source: "",
            resumable: true, queuePos: 0, hashV1: "0123456789abcdef0123456789abcdef01234567", hashV2: "",
            comment: "Mock torrent", creator: "mktorrent", downloaded: 1.8e9, uploaded: 2e8, numFiles: 2,
            numPieces: 2000, pieceLength: 2097152, savePath: "/tmp/x", tracker: "udp://tracker.example.org:1337",
            magnet: "magnet:?xt=urn:btih:0123", connections: [
                { start: 0, cursor: 5e8, end: 1e9, speed: 1.2e6 }, { start: 1e9, cursor: 1.4e9, end: 2e9, speed: 9e5 }],
            speedNow: 5.4e6, firstConnSpeed: 1.2e6, url: "https://example.com/a.iso", finalUrl: "https://cdn.example.com/a.iso",
            referer: "https://example.com", retries: 1 } : ({})
        files = [{ index: 0, path: "ubuntu/ubuntu.iso", size: 4.1e9, done: 1.7e9, priority: 4, progress: 0.41 },
                 { index: 1, path: "ubuntu/SHA256SUMS", size: 1200, done: 1200, priority: 7, progress: 1 }]
        peers = [{ ip: "203.0.113.5", port: 6881, client: "qBittorrent 5.1", flags: "D", progress: 0.8, down: 4e5,
                   up: 1e4, utp: true, encrypted: true, seed: false }]
        trackers = [{ url: "udp://tracker.example.org:1337", status: "working", message: "", tier: 0, seeds: 200,
                      peers: 90, downloaded: 1000 }]
        pieces = p
    }

    function addLinks(text, opts) { toast("Added", "ok", ""); return 1 }
    function inspectTorrent(p) {
        return { ok: true, name: "Mock torrent", hash: "abc", size: 4.2e9, comment: "", creator: "", isPrivate: true,
                 exists: false, files: [{ index: 0, path: "a/b.mkv", size: 4e9 }, { index: 1, path: "a/c.srt", size: 2e5 }] }
    }
    function addTorrentFile(p, opts) { return true }
    function pickInfo(id) {
        return { id: id, pending: true, ready: true, name: "Sintel", size: 4.2e9, savePath: "D:/Downloads",
                 files: [{ index: 0, path: "Sintel/sintel-4k.mkv", size: 4e9 }, { index: 1, path: "Sintel/sintel.en.srt", size: 2e5 },
                         { index: 2, path: "Sintel/poster.jpg", size: 1.9e8 }] }
    }
    function confirmPick(id, pr) { return true }
    function cancelPick(id) {}
    function classify(text) {
        var lines = text.split(/[\r\n]+/).filter(function (l) { return l.trim().length })
        var m = 0, u = 0, bad = 0, names = []
        for (var i = 0; i < lines.length; ++i) {
            if (lines[i].indexOf("magnet:") === 0) { m++; names.push("magnet") }
            else if (/^https?:/.test(lines[i])) { u++; names.push(lines[i].split("/").pop()) }
            else bad++
        }
        return { magnets: m, urls: u, torrents: 0, invalid: bad, names: names, category: u ? "Archives" : "" }
    }
    function defaultFolderFor(c) { return settings ? settings.savePath + (c ? "/" + c : "") : "" }
    function pause(ids) {} function resume(ids) {} function togglePause(ids) {} function forceStart(ids) {}
    function remove(ids, del) { toast("Removed", "info", "undo") } function undoRemove() {}
    function recheck(ids) {} function reannounce(ids) {} function restart(ids) {} function queueMove(ids, op) {}
    function setSequential(ids, on) {} function setFirstLast(ids, on) {} function setFilePriority(id, i, p) {}
    function setCategory(ids, c) {} function toggleTag(ids, t) {} function setLimits(ids, d, u) {}
    function setConnections(ids, n) {} function moveTo(ids, f) {} function addTracker(id, url) {}
    function openFolder(id) {} function openFile(id) {} function openFileAt(id, p) {}
    function setDarkFrame(dark) {}
    function extensionDownloads() {
        return { release: true, chromiumFile: "Rift-1.0.0-browser-chromium.zip", firefoxFile: "Rift-1.0.0-browser-firefox.zip",
                 chromium: "https://github.com/jeelnadaa/rift/releases/download/v1.0.0/Rift-1.0.0-browser-chromium.zip",
                 firefox: "https://github.com/jeelnadaa/rift/releases/download/v1.0.0/Rift-1.0.0-browser-firefox.zip",
                 page: "https://github.com/jeelnadaa/rift/releases/tag/v1.0.0" }
    }
    function linkFor(id) { return "magnet:?xt=urn:btih:0123" } function copyLinks(ids) {} function copyText(t) {}
    function exportTorrent(id, p) { return true } function verifyChecksum(id, e) {}
    function pauseAll() {} function resumeAll() {}
    function setAltSpeed(on) { settings.set("altSpeed", on) }
    function fmtBytes(b) {
        var u = ["B", "KB", "MB", "GB", "TB"], i = 0
        while (b >= 1000 && i < u.length - 1) { b /= 1000; ++i }
        return (i === 0 ? Math.round(b) : b.toFixed(b < 10 ? 2 : 1)) + " " + u[i]
    }
    function fmtSpeed(b) { return fmtBytes(b) + "/s" }
    function fmtEta(s) { return s < 0 ? "∞" : Math.floor(s / 60) + "m " + Math.round(s % 60) + "s" }
    function fmtDate(ms) { return new Date(ms).toLocaleString() }
    function fmtRelative(ms) { return "1 hour ago" }
    function urlToPath(u) { return String(u).replace(/^file:\/\/\/?/, "") }
    function pathToUrl(p) { return "file:///" + p }
    function checkClipboard() {} function dismissClipboard() { clipboardOffer = ""; clipboardChanged() }
    function registerHandlers() { return true } function isDefaultHandler() { return false }
    function history(q) { return [{ kind: "http", name: "old.zip", size: 1e6, source: "https://example.com/old.zip", path: "/tmp/old.zip", completedAt: Date.now() - 86400000 }] }
    function clearHistory() {}
    function dailyStats(days) {
        var a = []
        for (var i = 0; i < days; ++i) a.push({ date: "2026-10-" + (i + 1), down: 1e9 * (1 + Math.sin(i)), up: 2e8 })
        return a
    }
    function saveCategory(n, p) {} function removeCategory(n) {}
    function diagnostics() { return "harness" } function quit() { quitRequested() } function notify(t, b) {}
}
