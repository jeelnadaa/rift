# PROGRESS.md

Working log and checklist for Rift. **Agents: read `AGENTS.md` first, then continue from the first unchecked item below.**

Legend: `[x]` done · `[ ]` not done · `[~]` partly done (see note)

## Release 1.0 checklist

### Foundation
- [ ] Repo layout, `.gitignore`, `CMakeLists.txt`, dependency detection (Qt 6.8, libtorrent 2.0, libcurl)
- [ ] Version injected by CI (`RIFT_VERSION`), `scripts/next-version.py` + test
- [ ] `src/core`: segment map (work stealing), speed meter (current + smoothed, zero within 2 s), units, ETA
- [ ] `src/core`: path sanitizer, Content-Disposition / URL file name, batch pattern `[001-120]`, file-type category
- [ ] `tests/core_tests.cpp` + `scripts/test-core.sh` (runs with plain g++/clang)

### Engines
- [ ] `rift-get` HTTP engine: probe (`Range: bytes=0-`), single-connection fallback, dynamic segmentation, adaptive connections, stall detection + retry/backoff, `If-Range` resume, positional writes into `.riftpart`, rename on finish, per-download limit, global limit
- [ ] HTTP: checksum verify (SHA-256/SHA-1/MD5 via QCryptographicHash in the app layer)
- [ ] TorrentEngine (libtorrent): session settings mapper, add magnet/file/URL, pause/resume/remove(+files), force start/recheck/reannounce, sequential, first/last pieces, file priorities, per-torrent limits, queue up/down/top/bottom, move storage, resume data in SQLite, alerts → events, session stats (DHT nodes, port status)

### App
- [ ] Storage: SQLite schema + migrations (downloads, http_segments, resume_data, categories, tags, history, settings)
- [ ] Settings: typed store with defaults; applied live to both engines
- [ ] Controller exposed to QML (actions + selection + details)
- [ ] Unified download list model + filter/sort proxy (status, type, category, tag, tracker/site, text/regex)
- [ ] Single instance + argument forwarding (local socket, user-only)
- [ ] CLI: `rift <magnet|file|url>`, `--version`, `--portable`, `--minimized`
- [ ] Portable mode (`portable.flag` → `Data/`)
- [ ] Tray icon: show/hide, pause/resume all, alt speed, quit; speeds in tooltip; close to tray
- [ ] Notifications (download complete / error) via tray
- [ ] Clipboard watcher (magnet / direct link) → banner
- [ ] Drag & drop of files, magnets, links
- [ ] Open file / open folder / copy magnet/link
- [ ] Prevent sleep while downloading (Windows/macOS/Linux)
- [ ] File associations: magnet + .torrent (installer/Info.plist/desktop file)

### UI (QML, matches design/app)
- [ ] Theme.qml (Stone/Obsidian, accents, densities, motion), fonts bundled
- [ ] Icon component (design icon set), Tip tooltip helper, buttons/inputs/switch/checkbox/select/segmented/menu primitives with motion
- [ ] Custom title area + toolbar (Add split button, resume/pause/remove, queue, alt speed, filter box with regex, command palette, settings)
- [ ] Sidebar: nav-top (Transfers/Search/RSS/Stats), status filters, type, categories, tags, sites with counts; collapsible (Ctrl B)
- [ ] Download list: kind chip, name, size, progress (bar or HTTP segment bar), status badge, down, up, ETA, seeds/peers or connections; sorting; multi-select; context menu; row stagger; empty states
- [ ] Bulk bar for multi-select
- [ ] Details panel: Overview, Files (priorities), Peers, Trackers (torrent) · Overview, Connections (HTTP); resizable, collapsible
- [ ] Status bar: speeds with limit context, connection status, DHT, free disk, alt speed, counts
- [ ] Add link/magnet dialog (multi-line, batch pattern, folder, category, connections, start paused)
- [ ] Add torrent dialog (file tree with checkboxes, sizes, save path, sequential, start paused)
- [ ] Remove dialog (keep / delete files, undo toast when files kept)
- [ ] Settings window: General, Appearance, Downloads, Connection, Speed, BitTorrent, HTTP downloader, Browser, Notifications, Advanced
- [ ] Command palette (Ctrl K)
- [ ] Stats page (session totals + live speed graph)
- [ ] About dialog
- [ ] Toasts and banners
- [ ] Keyboard shortcuts (Ctrl L/O/K/F/B/A/,, Space, Del, Enter, Esc)

### Browser integration
- [ ] `rift-native-host` (Native Messaging, size-limited JSON, forwards to the app socket)
- [ ] Native messaging manifests (Chrome/Edge/Brave/Firefox) written by the app on start + by installers
- [ ] MV3 extension: context menu "Download with Rift", take-over by type/size with bypass, popup in the design language, options page; Chromium + Firefox builds; tests

### Packaging and release
- [ ] Windows: windeployqt bundle, Inno Setup installer (per-user/all-users, custom dir, shortcuts, magnet/.torrent, native host registry), portable ZIP with `portable.flag`
- [ ] macOS: `Rift.app` with Info.plist (magnet/.torrent), macdeployqt, DMG + ZIP, arm64 and x64 builds
- [ ] Linux: AppImage (linuxdeploy + Qt plugin), DEB, RPM, tar.gz, desktop file + MIME
- [ ] `.github/workflows/release.yml`: on push to `main` → next version → build 4 targets + extension → SHA256SUMS → GitHub release `vX.Y.Z` (latest)
- [ ] `.github/workflows/ci.yml`: build + tests on pull requests
- [ ] `rift-static/download.html` points at the new asset names (macOS arm64 + x64)

### Docs
- [ ] README.md (features, build, release, layout)
- [ ] DESIGN.md / PLAN.md notes where implementation differs

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

## Log

- 2026-11-07: Started implementation. Local machine has no Qt/libtorrent toolchain (installing one was not allowed), so Qt/libtorrent code is verified by CI; `src/core`, the extension and the release scripts are tested locally.
