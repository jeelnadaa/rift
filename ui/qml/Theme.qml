// Rift design tokens (design/tokens.json, DESIGN.md). Every color, size and duration in the UI comes from here.
// Main.qml binds mode/accentName/density/reducedMotion to the user's settings.
pragma Singleton
import QtQuick

QtObject {
    id: theme

    // ---- Mode -------------------------------------------------------------------
    property string mode: "system"                 // "system" | "stone" (light) | "obsidian" (dark)
    readonly property bool dark: mode === "obsidian"
        || (mode === "system" && Qt.styleHints.colorScheme === Qt.ColorScheme.Dark)
    property string density: "compact"             // "compact" | "comfortable"
    readonly property bool compact: density !== "comfortable"
    property bool reducedMotion: false

    // ---- Fonts (bundled; fall back to the system UI font) --------------------------
    readonly property string fontUi: "Bricolage Grotesque"
    readonly property string fontMono: "JetBrains Mono"
    readonly property string fontSerif: "Instrument Serif"

    // ---- Type (DESIGN.md §3). Body defaults (14 px, weight 450, wdth 96) are the app font, set in C++.
    readonly property var numFeatures: ({ "tnum": 1, "zero": 1 })   // numbers never jitter
    readonly property var displayAxes: ({ "wdth": 85, "opsz": 96 })  // page titles, wordmark
    readonly property var statAxes: ({ "wdth": 85, "opsz": 48 })     // big numbers
    readonly property int weightBody: 450
    readonly property int weightNav: 550
    readonly property QtObject type: QtObject {
        readonly property int caption: 12
        readonly property int small: 13
        readonly property int body: 14
        readonly property int num: 13
        readonly property int title: 16
        readonly property int section: 22
        readonly property int page: 28
        readonly property int display: 44
    }

    // ---- Radius / sizes ----------------------------------------------------------------
    readonly property QtObject radius: QtObject {
        readonly property int xs: 4
        readonly property int sm: 7
        readonly property int md: 9
        readonly property int lg: 12
        readonly property int big: 14
        readonly property int small: 4
    }
    readonly property QtObject size: QtObject {
        readonly property int toolbar: 52
        readonly property int statusbar: 30
        readonly property int sidebar: 264
        readonly property int sideItem: theme.compact ? 36 : 40
        readonly property int row: theme.compact ? 46 : 54
        readonly property int header: 40
        readonly property int control: theme.compact ? 32 : 36
        readonly property int controlSmall: theme.compact ? 26 : 30
        readonly property int tab: 42
        readonly property int pad: theme.compact ? 12 : 16       // panel padding
        readonly property int icon: 16
        readonly property int iconSmall: 14
        readonly property int iconLarge: 20
    }

    // ---- Motion ---------------------------------------------------------------------
    readonly property QtObject motion: QtObject {
        readonly property int press: theme.reducedMotion ? 0 : 60
        readonly property int fast: theme.reducedMotion ? 0 : 140
        readonly property int base: theme.reducedMotion ? 0 : 200
        readonly property int slow: theme.reducedMotion ? 120 : 300
        readonly property int emphasized: theme.reducedMotion ? 120 : 440
        readonly property int tooltipDelay: 450
        readonly property int rowStagger: theme.reducedMotion ? 0 : 22
        readonly property var standard: [0.2, 0.8, 0.2, 1, 1, 1]   // cubic-bezier(.2,.8,.2,1)
        readonly property var exit: [0.4, 0, 1, 1, 1, 1]
        readonly property var spring: [0.2, 1.3, 0.3, 1, 1, 1]     // cubic-bezier(.2,1.3,.3,1)
        readonly property real pressScale: theme.reducedMotion ? 1 : 0.965
        readonly property real squashScale: theme.reducedMotion ? 1 : 0.88   // switches/checkboxes on press
    }

    // ---- Palettes ---------------------------------------------------------------------
    readonly property QtObject stone: QtObject {
        readonly property color window: "#EEEBE4"; readonly property color sidebar: "#E6E2D8"
        readonly property color surface: "#F6F4EE"; readonly property color raised: "#FCFBF8"
        readonly property color sunken: "#DEDAD0"; readonly property color input: "#FFFFFF"
        readonly property color overlay: Qt.rgba(23/255, 20/255, 15/255, 0.38)
        readonly property color hover: Qt.rgba(23/255, 20/255, 15/255, 0.05)
        readonly property color pressed: Qt.rgba(23/255, 20/255, 15/255, 0.09)
        readonly property color line: Qt.rgba(23/255, 20/255, 15/255, 0.12)
        readonly property color lineSoft: Qt.rgba(23/255, 20/255, 15/255, 0.065)
        readonly property color lineStrong: "#17140F"
        readonly property color text: "#17140F"; readonly property color text2: "#575045"; readonly property color text3: "#857D70"
        readonly property color signal: "#7F4BB3"; readonly property color success: "#1C7A58"
        readonly property color warning: "#B85F1A"; readonly property color danger: "#B32638"
        readonly property color violet: "#55607F"; readonly property color info: "#1F7F86"
        readonly property color down: "#1C7A58"; readonly property color up: "#1F7F86"
        readonly property color kindHttp: "#1F7F86"
        readonly property color pieceMissing: Qt.rgba(23/255, 20/255, 15/255, 0.08)
        readonly property color shadow: Qt.rgba(40/255, 30/255, 15/255, 0.18)
    }
    readonly property QtObject obsidian: QtObject {
        readonly property color window: "#050608"; readonly property color sidebar: "#08090C"
        readonly property color surface: "#0B0C10"; readonly property color raised: "#12141A"
        readonly property color sunken: "#020304"; readonly property color input: "#090A0E"
        readonly property color overlay: Qt.rgba(0, 0, 0, 0.66)
        readonly property color hover: Qt.rgba(225/255, 232/255, 1, 0.045)
        readonly property color pressed: Qt.rgba(225/255, 232/255, 1, 0.085)
        readonly property color line: Qt.rgba(225/255, 232/255, 1, 0.085)
        readonly property color lineSoft: Qt.rgba(225/255, 232/255, 1, 0.045)
        readonly property color lineStrong: "#ECEEF3"
        readonly property color text: "#ECEEF3"; readonly property color text2: "#A2A7B3"; readonly property color text3: "#6A707C"
        readonly property color signal: "#C38BEA"; readonly property color success: "#4CC89A"
        readonly property color warning: "#F0924A"; readonly property color danger: "#F0596A"
        readonly property color violet: "#9AA3C7"; readonly property color info: "#5EC4C4"
        readonly property color down: "#4CC89A"; readonly property color up: "#5EC4C4"
        readonly property color kindHttp: "#5EC4C4"
        readonly property color pieceMissing: Qt.rgba(225/255, 232/255, 1, 0.06)
        readonly property color shadow: Qt.rgba(0, 0, 0, 0.7)
    }

    readonly property QtObject c: dark ? obsidian : stone

    // ---- Accent ---------------------------------------------------------------------
    property string accentName: "cobalt"
    readonly property var accents: ({
        cobalt: ["#1E46D8", "#4C7DFF"], champagne: ["#8E6119", "#D6B072"], jade: ["#1C7A58", "#4CC89A"],
        teal: ["#1F7F86", "#5EC4C4"], orchid: ["#7F4BB3", "#C38BEA"], ember: ["#B4532A", "#EE8A5E"] })
    readonly property var accentNames: ["cobalt", "champagne", "jade", "teal", "orchid", "ember"]
    readonly property color accent: (accents[accentName] || accents.cobalt)[dark ? 1 : 0]
    readonly property color accentHover: dark ? Qt.lighter(accent, 1.12) : Qt.darker(accent, 1.14)
    readonly property color accentPressed: Qt.darker(accent, dark ? 1.14 : 1.3)
    readonly property color accentSoft: Qt.rgba(accent.r, accent.g, accent.b, dark ? 0.14 : 0.12)
    readonly property color selection: Qt.rgba(accent.r, accent.g, accent.b, dark ? 0.16 : 0.15)
    readonly property color onAccent: luminance(accent) > 0.45 ? "#0B0C10" : "#FFFFFF"
    readonly property color focus: accent
    readonly property color kindTorrent: accent

    function lin(v) { return v <= 0.03928 ? v / 12.92 : Math.pow((v + 0.055) / 1.055, 2.4) }
    function luminance(col) { return 0.2126 * lin(col.r) + 0.7152 * lin(col.g) + 0.0722 * lin(col.b) }
    function soft(col, a) { return Qt.rgba(col.r, col.g, col.b, a === undefined ? (dark ? 0.14 : 0.12) : a) }
    function hex(col) {
        function h(v) { var s = Math.round(v * 255).toString(16); return s.length < 2 ? "0" + s : s }
        return "#" + h(col.r) + h(col.g) + h(col.b)
    }

    readonly property var tagPalette: ["#2F5BE0", "#1F7F86", "#1C7A58", "#7F4BB3", "#2A8A8F", "#B34A6E", "#B85F1A", "#575045"]
    readonly property var connectionPalette: ["#4C7DFF", "#5EC4C4", "#4CC89A", "#C38BEA", "#F0924A", "#D6B072", "#E98AA8", "#B9C07A"]
    function tagColor(name) {
        var h = 0
        for (var i = 0; i < name.length; ++i) h = (h * 31 + name.charCodeAt(i)) % 9973
        return tagPalette[h % tagPalette.length]
    }

    // Download state → color and label
    function stateColor(s) {
        switch (s) {
        case "downloading": case "probing": return accent
        case "seeding": case "completed": return c.success
        case "streaming": return c.signal
        case "stalled": case "choose": return c.warning
        case "checking": case "moving": case "metadata": return c.violet
        case "error": return c.danger
        case "scheduled": return c.info
        default: return c.text3
        }
    }
    function stateLabel(s) {
        switch (s) {
        case "metadata": return qsTr("Fetching metadata")
        case "choose": return qsTr("Choose files")
        case "probing": return qsTr("Connecting")
        case "checking": return qsTr("Checking")
        case "moving": return qsTr("Moving")
        case "downloading": return qsTr("Downloading")
        case "stalled": return qsTr("Stalled")
        case "seeding": return qsTr("Seeding")
        case "completed": return qsTr("Completed")
        case "paused": return qsTr("Paused")
        case "queued": return qsTr("Queued")
        case "error": return qsTr("Error")
        default: return s.length ? s.charAt(0).toUpperCase() + s.slice(1) : ""
        }
    }
    function isLive(s) { return s === "downloading" || s === "probing" || s === "checking" || s === "metadata" }
    function isDim(s) { return s === "paused" || s === "queued" }
}
