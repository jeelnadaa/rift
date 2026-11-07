# AGENTS.md: rules for anyone (human or AI) working on Rift

Read this file first, then **PROGRESS.md**, then the parts of **PLAN.md** and **DESIGN.md** that matter for your task.

## Resume protocol (most important)

1. `PROGRESS.md` is the single source of truth for what is done and what is next. Open it before writing code.
2. Pick the first unchecked item under **Next up** (or the item the user names). Do it fully, including tests and docs.
3. When an item is done: tick it in `PROGRESS.md`, add a line to the **Log** with the date and what changed, then commit.
4. Commit small and often (`git commit`, never `git push` unless the user asks). An interrupted session must lose at most one item of work.
5. Never delete unchecked items. If you decide not to do something, move it to **Deferred** with the reason.

## Project shape

| Path | What it is |
|---|---|
| `src/core/` | Pure C++20, no Qt, no libcurl: segment map, speed meter, units, path safety, URL/filename helpers, version. Unit-tested with `tests/core_tests.cpp`. |
| `src/http/` | `rift-get`, the HTTP/FTP engine on libcurl (multi interface, segmentation, resume). No Qt. |
| `src/engine/` | `TorrentEngine`, the only code that includes libtorrent headers. |
| `src/app/` | Qt app: bootstrap, controller exposed to QML, models, storage (QtSql/SQLite), settings, tray, single instance, IPC. |
| `src/nativehost/` | `rift-native-host`, the browser Native Messaging bridge (stdin/stdout → local socket). |
| `ui/qml/` | All QML. `Theme.qml` holds every color, size and duration. |
| `ui/icons/` | Icon SVG paths live in `ui/qml/Icons.js` (24×24, 1.75 stroke), generated from `design/app/app.js`. |
| `browser/` | One MV3 extension source; `browser/build.mjs` emits Chromium and Firefox builds. |
| `packaging/` | Inno Setup (Windows), Info.plist (macOS), desktop file + AppImage/deb/rpm (Linux), native-messaging manifests. |
| `.github/workflows/release.yml` | Every push to `main` builds all platforms and publishes the next `vX.Y.Z` release. |
| `rift-static/` | Public website (Cloudflare). Not part of the app build. |
| `design/` | Design prototype and tokens. **The app must match it.** |

## Rules

1. **Design is law.** Colors, sizes, radii, motion and copy come from `DESIGN.md` / `design/tokens.json` / `ui/qml/Theme.qml`. Never hard-code a color in a view. Every interactive control has a tooltip (`ToolTip` via the `Tip` attached helper) and an accessible name.
2. **Engines never block the UI.** libtorrent calls are async; the HTTP engine runs on its own thread. The UI reads snapshots on a timer (500 ms, 1000 ms when hidden).
3. **Only `src/engine/` includes libtorrent. Only `src/http/` includes curl.** The UI talks to `Controller`.
4. **Pure logic goes in `src/core/`** and gets a test in `tests/core_tests.cpp`. Run `scripts/test-core.sh` (needs only a C++20 compiler) before committing changes there.
5. **No network listeners** besides the BitTorrent port. Browser integration uses Native Messaging + the user-only local socket.
6. **Never lose user data.** Removing without "delete files" must never delete files. Paths from torrents/servers go through `core::sanitizeFileName`.
7. **Strings** in QML use `qsTr()`.
8. **Cross-platform:** OS-specific code stays behind `#ifdef Q_OS_*` in `src/app/platform.cpp`.
9. **Versioning is automatic.** Do not hand-edit version numbers. CI computes the next version from the latest `v*` tag (`scripts/next-version.py`) and passes `-DRIFT_VERSION=` to CMake.
10. **Release asset names are a contract** with `rift-static/download.html`. If you change one, change both:
    - `Rift-<v>-windows-x64-setup.exe`, `Rift-<v>-windows-x64-portable.zip`
    - `Rift-<v>-macos-arm64.dmg`, `Rift-<v>-macos-x64.dmg`, `Rift-<v>-macos-arm64.zip`, `Rift-<v>-macos-x64.zip`
    - `Rift-<v>-linux-x86_64.AppImage`, `rift_<v>_amd64.deb`, `rift-<v>.x86_64.rpm`, `Rift-<v>-linux-x86_64.tar.gz`
    - `Rift-<v>-browser-chromium.zip`, `Rift-<v>-browser-firefox.zip`, `SHA256SUMS`

## Build

```bash
# deps: Qt 6.8 (Quick, QuickControls2, Sql, Network, Svg, Widgets), libtorrent-rasterbar 2.0, libcurl
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Core-only tests without Qt: `bash scripts/test-core.sh`.
Extension: `node browser/build.mjs` → `dist/browser/`; tests: `node browser/test.mjs`.

## Definition of done (per item)

- Builds on all three OSes in CI; no new warnings in touched files.
- Matches the design in both themes and both densities; has hover/press motion and a tooltip.
- Has empty/error states where relevant.
- Tests added where the logic is testable; `PROGRESS.md` updated; committed.
