# Rift

A fast, clean download manager for Windows, macOS and Linux. Rift combines a BitTorrent client built on
[libtorrent](https://libtorrent.org) with its own accelerated multi-connection HTTP/FTP downloader. A browser
extension adds **Download with Rift** to Chrome, Edge, Brave and Firefox.

- **Torrents:** magnets, `.torrent` files and URLs; v1, v2 and hybrid torrents; sequential download; first and last pieces first; per-file priorities; queueing; per-torrent limits; recheck; reannounce; move storage. DHT, PEX, LSD, uTP and encryption.
- **HTTP/FTP (`rift-get`):** dynamic segmentation with work stealing, adaptive connection count, stall detection with retry and backoff, crash-safe resume (`If-Range`, `.riftpart` files), checksum verification (SHA-256, SHA-1, MD5), and batch patterns such as `file[001-120].zip`.
- **One list for everything:** filters by status, type, category, tag and site, plus text or regex search, sorting, multi-select with a bulk bar, and a details panel (overview, files, peers, trackers, connections).
- **Honest speeds:** measured speeds only, totals that add up, and one bandwidth arbiter for torrents and HTTP together.
- **Desktop integration:** tray icon, notifications, clipboard watcher, drag and drop, magnet and `.torrent` file associations, prevent sleep while downloading, single instance, portable mode.
- **Design:** Stone (light) and Obsidian (dark) themes, compact and comfortable densities, spring motion, bundled fonts. See [DESIGN.md](DESIGN.md).
- **Private by default:** the only network listener is the BitTorrent port. The browser bridge uses Native Messaging and a user-only local socket.

## Download

Every push to `main` publishes a new release on the GitHub **Releases** page:

| Platform | Files |
|---|---|
| Windows 10/11 x64 | `Rift-<v>-windows-x64-setup.exe`, `Rift-<v>-windows-x64-portable.zip` |
| macOS 12+ Apple silicon | `Rift-<v>-macos-arm64.dmg`, `Rift-<v>-macos-arm64.zip` |
| macOS 12+ Intel | `Rift-<v>-macos-x64.dmg`, `Rift-<v>-macos-x64.zip` |
| Linux x86_64 | `Rift-<v>-linux-x86_64.AppImage`, `rift_<v>_amd64.deb`, `rift-<v>.x86_64.rpm`, `Rift-<v>-linux-x86_64.tar.gz` |
| Browser extension | `Rift-<v>-browser-chromium.zip`, `Rift-<v>-browser-firefox.zip` |

Check your file against `SHA256SUMS`, which is attached to every release.

> The builds are **not code-signed yet**. On Windows, SmartScreen may ask once (choose *More info → Run anyway*).
> On macOS the app is ad-hoc signed but not notarized: right-click Rift in Applications and choose **Open** the first time.

## Build from source

You need CMake 3.21 or later, Ninja, a C++20 compiler, **Qt 6.8+** (Quick, QuickControls2, Sql, Network, Svg, Widgets),
**libtorrent-rasterbar 2.0** and **libcurl**.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

Where to get the dependencies:

- **Windows (MSYS2 UCRT64):** `pacman -S mingw-w64-ucrt-x86_64-{toolchain,cmake,ninja,qt6-base,qt6-declarative,qt6-svg,qt6-tools,libtorrent-rasterbar,curl,boost}`. CI uses MSVC with vcpkg instead.
- **macOS:** `brew install qt libtorrent-rasterbar curl ninja cmake`
- **Ubuntu/Debian:** `apt install qt6-base-dev qt6-declarative-dev qt6-svg-dev libtorrent-rasterbar-dev libcurl4-openssl-dev ninja-build cmake`

### Faster checks

| Command | What it checks | Needs |
|---|---|---|
| `bash scripts/test-core.sh` | `src/core` unit tests | a C++20 compiler |
| `bash scripts/ui-harness.sh` | Loads the real QML against a mock backend and fails on any QML warning | Qt only (`-DRIFT_UI_ONLY=ON`) |
| `node browser/test.mjs` | Extension logic and both built manifests | Node 18+ |
| `python3 tests/test_next_version.py` | Release version rules | Python 3 |

To see the UI without libtorrent or curl, run `build-ui/rift_ui_harness --show`. To save screenshots of
both themes, run `build-ui/rift_ui_harness --shots shots/`.

### Browser extension

```bash
node browser/build.mjs        # → dist/browser/chromium and dist/browser/firefox
```

Load `dist/browser/chromium` as an unpacked extension (Chromium ID `llhjnffjocckdmklgdcpnllflgcacppo`), or
`dist/browser/firefox/manifest.json` as a temporary add-on (ID `browser@rift.app`). Rift writes the
native-messaging manifests for host `app.rift.native` each time it starts, and the Windows installer registers them too.

## Releases and versions

`.github/workflows/release.yml` runs on every push to `main`:

1. `scripts/next-version.py` takes the latest `vX.Y.Z` tag and bumps the patch number. The first release is `1.0.0`.
   To bump the major or minor version, edit `VERSION` (for example `1.1`). Never hand-edit version numbers anywhere else.
2. It builds Windows x64 (MSVC + vcpkg, Inno Setup), macOS arm64 and x64 (Homebrew, macdeployqt, DMG), Linux x86_64
   (AppImage via linuxdeploy, DEB/RPM via fpm), and the browser extension zips.
3. It checks every asset name against the contract in [AGENTS.md](AGENTS.md), writes `SHA256SUMS`, and publishes
   release `vX.Y.Z` marked as latest.

`.github/workflows/ci.yml` runs the fast checks on pull requests.

## Repository layout

| Path | What it is |
|---|---|
| `src/core/` | Pure C++20 logic (segments, speed, units, paths, names), unit-tested |
| `src/http/` | `rift-get`, the libcurl HTTP/FTP engine |
| `src/engine/` | `TorrentEngine`, the only code that includes libtorrent |
| `src/app/` | Qt app: controller, model, storage (SQLite), settings, tray, IPC, platform code |
| `src/nativehost/` | `rift-native-host`, the Native Messaging bridge |
| `ui/qml/` | All QML; `Theme.qml` holds the design tokens |
| `ui/fonts/` | Bundled fonts (SIL Open Font License) |
| `browser/` | MV3 extension source, build script and tests |
| `packaging/` | Windows (Inno Setup, rc, manifest), macOS (Info.plist), Linux (desktop file, metainfo) |
| `tests/` | Core tests, UI harness, version script tests |
| `design/` | Design prototype and tokens. The app follows them. |
| `rift-static/` | Public website (Cloudflare) |

Contributors and coding agents: read [AGENTS.md](AGENTS.md) first, then [PROGRESS.md](PROGRESS.md).

## License

MIT. See [LICENSE](LICENSE). Qt is used under the LGPL (dynamically linked). libtorrent is BSD-licensed and libcurl uses the curl license.
