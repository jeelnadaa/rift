# PROGRESS.md

Working log and checklist for Rift. **Agents: read `AGENTS.md` first, then continue from the first unchecked item below.**

Legend: `[x]` done · `[ ]` not done · `[~]` partly done (see note)

## Release 1.0 checklist

### Foundation
- [x] Repo layout, `.gitignore`, `CMakeLists.txt`, dependency detection (Qt 6.8, libtorrent 2.0, libcurl)
- [x] Version injected by CI (`RIFT_VERSION`), `scripts/next-version.py` + test
- [x] `src/core`: segment map (work stealing), speed meter (current + smoothed, zero within 2 s), units, ETA
- [x] `src/core`: path sanitizer, Content-Disposition / URL file name, batch pattern `[001-120]`, file-type category
- [x] `tests/core_tests.cpp` + `scripts/test-core.sh` (runs with plain g++/clang)

### Engines
- [x] `rift-get` HTTP engine: probe (`Range: bytes=0-`), single-connection fallback, dynamic segmentation, adaptive connections, stall detection + retry/backoff, `If-Range` resume, positional writes into `.riftpart`, rename on finish, per-download limit, global limit
- [x] HTTP: checksum verify (SHA-256/SHA-1/MD5 via QCryptographicHash in the app layer)
- [x] TorrentEngine (libtorrent): session settings mapper, add magnet/file/URL, pause/resume/remove(+files), force start/recheck/reannounce, sequential, first/last pieces, file priorities, per-torrent limits, queue up/down/top/bottom, move storage, resume data in SQLite, alerts → events, session stats (DHT nodes, port status).

### App
- [x] Storage: SQLite schema + migrations (downloads, http_segments, resume_data, categories, tags, history, settings)
- [x] Settings: typed store with defaults; applied live to both engines
- [x] Controller exposed to QML (actions + selection + details)
- [x] Unified download list model + filter/sort proxy (status, type, category, tag, tracker/site, text/regex)
- [x] Single instance + argument forwarding (local socket, user-only)
- [x] CLI: `rift <magnet|file|url>`, `--version`, `--portable`, `--minimized`
- [x] Portable mode (`portable.flag` → `Data/`)
- [x] Tray icon: show/hide, pause/resume all, alt speed, quit; speeds in tooltip; close to tray
- [x] Notifications (download complete / error) via tray
- [x] Clipboard watcher (magnet / direct link) → banner
- [x] Drag & drop of files, magnets, links
- [x] Open file / open folder / copy magnet/link
- [x] Prevent sleep while downloading (Windows/macOS/Linux)
- [x] File associations: magnet + .torrent (installer/Info.plist/desktop file)

### UI (QML, matches design/app)
- [x] Theme.qml (Stone/Obsidian, accents, densities, motion), fonts bundled
- [x] Icon component (design icon set), Tip tooltip helper, buttons/inputs/switch/checkbox/select/segmented/menu primitives with motion
- [x] Custom title area + toolbar (Add split button, resume/pause/remove, queue, alt speed, filter box with regex, command palette, settings)
- [x] Sidebar: nav-top (Transfers/Search/RSS/Stats), status filters, type, categories, tags, sites with counts; collapsible (Ctrl B)
- [x] Download list: kind chip, name, size, progress (bar or HTTP segment bar), status badge, down, up, ETA, seeds/peers or connections; sorting; multi-select; context menu; row stagger; empty states
- [x] Bulk bar for multi-select
- [x] Details panel: Overview, Files (priorities), Peers, Trackers (torrent) · Overview, Connections (HTTP); resizable, collapsible
- [x] Status bar: speeds with limit context, connection status, DHT, free disk, alt speed, counts
- [x] Add link/magnet dialog (multi-line, batch pattern, folder, category, connections, start paused)
- [x] Add torrent dialog (file tree with checkboxes, sizes, save path, sequential, start paused)
- [x] Remove dialog (keep / delete files, undo toast when files kept)
- [x] Settings window: General, Appearance, Downloads, Connection, Speed, BitTorrent, HTTP downloader, Browser, Notifications, Advanced
- [x] Command palette (Ctrl K)
- [x] Stats page (session totals + live speed graph)
- [x] About dialog
- [x] Toasts and banners
- [x] Keyboard shortcuts (Ctrl L/O/K/F/B/A/,, Space, Del, Enter, Esc)

### Browser integration
- [x] `rift-native-host` (Native Messaging, size-limited JSON, forwards to the app socket)
- [x] Native messaging manifests (Chrome/Edge/Brave/Firefox) written by the app on start + by installers
- [x] MV3 extension: context menu "Download with Rift", take-over by type/size with bypass, popup in the design language, options page; Chromium + Firefox builds; tests

### Packaging and release
- [x] Windows: windeployqt bundle, Inno Setup installer (per-user/all-users, custom dir, shortcuts, magnet/.torrent, native host registry), portable ZIP with `portable.flag`
- [x] macOS: `Rift.app` with Info.plist (magnet/.torrent), macdeployqt, DMG + ZIP, arm64 and x64 builds
- [x] Linux: AppImage (linuxdeploy + Qt plugin), DEB, RPM, tar.gz, desktop file + MIME
- [x] `.github/workflows/release.yml`: on push to `main` → next version → build 4 targets + extension → SHA256SUMS → GitHub release `vX.Y.Z` (latest)
- [x] `.github/workflows/ci.yml`: build + tests on pull requests
- [x] `rift-static/download.html` points at the new asset names (macOS arm64 + x64)

### Docs
- [x] README.md (features, build, release, layout)
- [x] Implementation notes (below) where the build differs from DESIGN.md / PLAN.md

## Deferred (after 1.0, with reason)

Tracked so nothing from PLAN.md is forgotten. Not started.

- RSS feeds and auto-download rules (PLAN 4.6), Search plugins with QuickJS (4.12): larger subsystems; sidebar shows them as "coming soon" pages.
- Rules engine, watch folders, run-on-complete (4.6).
- Streaming server and built-in player (4.9).
- Bandwidth scheduler grid, VPN kill switch, interface binding UI, blocklists (4.4/4.5): libtorrent supports them; settings keys reserved.
- Create torrent dialog (4.11).
- Metalink, FTP-specific UI, mirrors, keychain credentials (4.14 P1-HTTP).
- Auto-update with minisign signatures (12.4), code signing/notarization (needs certificates from the owner).
- Linux arm64 and Windows arm64 builds.
- Import from other clients, backup/restore, mini mode, first-run wizard (4.13/4.16).

## Implementation notes (differences from DESIGN.md / PLAN.md)

- macOS ships two native builds (arm64, x64) instead of one universal app. Homebrew's libtorrent is single-arch, so this keeps CI simple.
- No code signing or notarization yet: Windows is unsigned and macOS is ad-hoc signed. The website says so honestly.
- `ContextMenu.qml` is called `RowMenu.qml` because Qt 6.9+ ships a `ContextMenu` type.
- Fractional type sizes from the design (9.5, 12.5) are rounded to whole pixels, because QML `font.pixelSize` is an int.
- RSS, Search, rules and streaming are shown as "coming soon" pages (see Deferred).

## Log

- 2026-11-07: Started implementation. `src/core`, the extension and the release scripts are tested locally.
- 2026-11-07: Core, HTTP engine, torrent engine, storage, settings, model, IPC, native host, the full QML UI, and packaging are committed.
- 2026-11-07: With Qt 6.12 (D:\Qt) installed, added the `RIFT_UI_ONLY` build and `rift_ui_harness`, which loads the real QML against a mock backend. Fixed every QML warning it found. Screenshots checked in both themes.
- 2026-11-07: Bundled the design fonts and matched the design's type axes, spring motion, press squash, icon nudges and spacing.
- 2026-11-07: Extension finished (Chromium + Firefox builds, 10 tests). Added `scripts/next-version.py` (5 tests), `release.yml` (Windows, macOS arm64/x64, Linux, extension, SHA256SUMS, latest release) and `ci.yml`.
- 2026-11-07: The website download page now uses the real asset names (macOS arm64/x64, Linux x86_64 only) and honest signing notes. README.md written.

- 2026-11-07: Full local build with MSYS2 UCRT64 (GCC 16.2, Qt 6.12, libtorrent 2.1.2, curl 8.23): no warnings (2.1 deprecations silenced for torrent.cpp only), ctest passes, `Rift.exe --smoke-test` passes. Launched the real app: DHT connects, a second launch forwards a URL into the running app's Add dialog.
- 2026-11-07: Get extension downloads the release zip from GitHub, website domain is rift.solarquack.in, launch video in `brag-output/` (web copy on the site's home page). Release CI: Windows caches vcpkg builds (actions/cache, files backend), the Linux AppImage smoke test reports missing libraries, and Run workflow takes a `platforms` input (e.g. `linux`) for quick partial builds that don't publish.
- 2026-11-07: CI skips pushes that only touch docs (`*.md`), `rift-static/`, `brag-output/` or `wrangler.jsonc`; every build job uses a compiler cache (ccache, sccache on Windows).
- 2026-11-07: Linux CI segfaulted in `QFontDatabase::addApplicationFont` (Qt 6.8 fontconfig backend, Ubuntu 22.04; backtrace via `scripts/ci-crash-report.sh`). On Linux the bundled fonts now go to fontconfig directly (`platform::registerAppFontsWithSystem`, before the app object exists).

### Resume note
- 1.0 checklist complete. Local full build: put `C:\msys64\ucrt64\bin` first on PATH, then
  `cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64 && cmake --build build`, run `build/Rift.exe --portable`.
- Next: watch the first GitHub release run and fix anything platform-specific it reports.
