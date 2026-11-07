# RIFT: Product and Engineering Plan

> **Name:** Rift
> **One-line pitch:** A fast, clean, modern, fully native download manager for Windows, macOS and Linux: a BitTorrent client built on libtorrent **plus** an accelerated multi-connection HTTP/FTP downloader built on our own segmented engine. One app, runs entirely on your device, installs with everything it needs.
> **Audience for this doc:** the coding agent that will build it. Follow it phase by phase and do not skip acceptance criteria.

### What changed in v3 (this draft)
- **New: accelerated HTTP/HTTPS/FTP downloader** ("IDM-style") with dynamic segmentation, adaptive connection count, mirrors, Metalink, checksum verification, and crash-safe resume. Built as our own module (`rift-get`) on top of libcurl. See Section 4.14.
- **New: browser integration without a network port**, using the browsers' **Native Messaging** (stdio pipe launched by the browser). This keeps the "no control port" rule. See Section 4.15.
- **New: unified bandwidth arbiter** so global speed limits, the scheduler, and data caps apply to torrents and HTTP downloads together (Section 3).
- **Library decision documented** (Section 2.1): keep libtorrent for BitTorrent; write our own HTTP segmentation engine; reasons and alternatives recorded.
- **Optimizations:** resume data in SQLite (WAL) instead of thousands of small files, `async_add_torrent` + staggered startup, Windows defaults to pread/pwrite disk I/O (mmap has known working-set/perf issues on Windows), LTO + PGO release builds, ed25519 (minisign) update signatures, QuickJS-ng for plugins, libmaxminddb for GeoIP.
- **New quality features** (Section 4.16): Mark-of-the-Web on downloads, prevent-sleep while active, shutdown/sleep on completion, metered-network and battery awareness, "game mode" auto-throttle, import from qBittorrent/other clients, and more.
- **New: accurate speed display** (Section 4.17): Rift shows only real, measured speeds, with totals that add up and an honest "acceleration" indicator.
- **Summary tables** of all optimizations (Section 2.2) and all new features (Section 2.3).
- **Fixes:** Section cross-references corrected; "single executable" clarified to "single process, one folder" (LGPL Qt must be dynamically linked).
- Roadmap re-sequenced to fit the HTTP engine (Section 11).

### What changed in v2
- **Renamed** to Rift.
- **Native desktop app only.** No web version, no web UI, no browser-based shell, no Python daemon.
- **Single process, single executable** (`Rift.exe` on Windows) with all dependencies bundled.
- **All remote control removed**: no REST/WebSocket API, no web UI, no headless/daemon mode, no Docker, no browser extension, no phone remote, no qBittorrent/Transmission compatibility, no Prometheus, no webhooks. Rift is controlled only on the device it runs on.
- **Distribution defined**: Windows installer (custom install location) + portable ZIP; macOS DMG + ZIP; Linux AppImage + DEB + RPM + tar.gz.

---

## 0. Ground rules for the agent

1. **Do NOT implement the BitTorrent protocol yourself.** Use **libtorrent-rasterbar 2.0.x** for DHT, PEX, LSD, uTP, trackers, encryption, piece picking, disk I/O, hashing, and v1/v2/hybrid torrents. Put it behind Rift's own `TorrentEngine` interface so the UI never includes libtorrent headers.
1a. **Do NOT implement HTTP, TLS, or FTP yourself.** Use **libcurl** (multi interface) for the transport. **Do** write Rift's own segmentation, scheduling, resume, and disk-writing logic on top of it (`src/http/`, Section 4.14). That logic is where the speed and reliability gains come from.
2. **The UI thread must never block on either engine.** The torrent engine and the HTTP engine each run on their own thread; the UI reads from models updated by batched events.
3. **Native, not web.** The UI is built with Qt 6 (Qt Quick/QML). Do not use Electron, Tauri, a WebView, or any embedded browser for the main UI.
4. **Self-contained by default.** A user must be able to install or unzip Rift on a clean machine and run it with **zero** extra downloads (no Visual C++ redistributable install, no .NET, no Python, no Java, no WebView2, no Qt install).
5. **Build in vertical slices.** At the end of each phase, the app runs, is usable, and passes that phase's acceptance criteria.
6. **Everything user-facing is configurable** through Settings (Section 8) with sensible defaults.
7. **Tests as you go** (Section 14). A feature is not done without tests and a short doc note.
8. **Modular code.** Engine, storage, automation, networking, and UI are separate modules with clear interfaces.
9. **Cross-platform from day one:** Windows 10/11 (x64, arm64 later), macOS 12+ (Apple Silicon + Intel), Linux (x64, arm64). Keep OS-specific code inside `platform/`.
10. **On-device only.** Rift opens no control port to the network. The only listening sockets are the BitTorrent peer port (libtorrent) and an optional localhost-only streaming endpoint (Section 4.9). Browser integration uses Native Messaging (the browser spawns a small host process and talks over stdin/stdout), which needs **no** listening socket.
11. **Legal posture.** Rift is a neutral download client. Do not bundle any indexer, tracker list, or search source that points to infringing content. Search is plugin-based and user-installed. The HTTP downloader **never** circumvents DRM, paywalls, or site access controls, and Rift ships no site-specific "rippers".

---

## 1. Goals and non-goals

### Goals
- Speed equal to or better than qBittorrent/Transmission (same engine, tuned well).
- HTTP download speed equal to or better than IDM/aria2/Free Download Manager on the same link and server.
- One place for all downloads: torrents and direct links share queueing, limits, scheduler, categories, and notifications.
- Very low idle CPU and RAM.
- Modern, clean, keyboard-friendly UI with dark/light themes.
- Rich but discoverable features: automation, streaming, privacy, organization.
- Professional installers on all three OSes with everything bundled.

### Non-goals
- Writing our own protocol stack.
- Any web UI, remote access, or network-exposed control interface.
- Built-in torrent indexer or content catalog.
- Cloud sync or social features.
- Native mobile apps.
- DRM circumvention, stream "ripping" from specific sites, or bundling yt-dlp-style site extractors.

---

## 2. Tech stack (decided)

| Layer | Choice | Why |
|---|---|---|
| Language | **C++20** | Direct libtorrent access, best performance, no runtime to ship |
| Engine | **libtorrent-rasterbar 2.0.x** (static link) | Industry standard, fastest option |
| UI toolkit | **Qt 6.7+ (Qt Quick / QML)** with Qt Widgets only where needed (native file dialogs, tray) | Truly native app, GPU-accelerated modern UI, small and fast, works on all 3 OSes, same family qBittorrent uses |
| Build system | **CMake + Ninja**, dependencies via **vcpkg** (manifest mode, pinned versions) | Reproducible builds on all OSes |
| HTTP/FTP transport | **libcurl 8.x** (static, multi interface; HTTP/1.1, HTTP/2 via nghttp2, FTP/FTPS, SOCKS/HTTP proxy, optional HTTP/3 via ngtcp2 later) | Mature, MIT-style license, used by billions of devices; we write the segmentation engine on top |
| Crypto/TLS | **OpenSSL 3** (static), shared by libtorrent and libcurl | One TLS stack, one set of CA handling. On Windows also load the OS root store (`CURLSSLOPT_NATIVE_CA`) so corporate/AV roots work |
| Hashing | OpenSSL EVP (MD5/SHA-1/SHA-256/SHA-512) + **xxHash** for internal block checks | Checksum verification, no extra lib |
| Signatures | **Monocypher** or libsodium (ed25519, minisign format) | Update manifest signatures; tiny, auditable |
| Storage | **SQLite** (direct sqlite3, WAL mode) | Settings, categories, RSS, history, stats, **resume data** |
| Archive extraction | **libarchive** (static) for zip/tar/7z/rar read support | No external 7-Zip/WinRAR needed |
| Charts | Custom QML Canvas / Qt Graphs for speed graphs | Fast, no web charts |
| GeoIP | Bundled DB-IP Lite country database (MMDB) read with **libmaxminddb**, updatable in-app | Peer country flags offline |
| Scripting (search plugins) | **QuickJS-ng** | Small, embeddable, no external runtime, memory/time limits |
| Secrets | **QtKeychain** (Windows Credential Manager / macOS Keychain / Secret Service) | Stores HTTP/FTP/proxy passwords; never plain text in SQLite |
| Crash reports (opt-in) | **sentry-native** with crashpad backend, self-hostable endpoint | Minidumps without names/paths |
| Installers | **Windows:** Inno Setup or WiX · **macOS:** DMG + notarization · **Linux:** AppImage, DEB, RPM | See Section 12 |
| CI/CD | **GitHub Actions** matrix (windows-latest, macos-latest, ubuntu-latest) | Builds, tests, packages, signs |

> **Why Qt/C++ instead of the earlier Python + Tauri idea?** Python and WebView-based apps need extra runtimes, make "one exe with everything inside" harder, and add startup and memory overhead. Qt/C++ gives a real native app, one process, direct libtorrent calls, and straightforward bundling. Qt is used under LGPLv3 with dynamic linking (so ship Qt DLLs/frameworks next to the executable and include license notices). Rift's own license is the owner's choice; libtorrent is BSD-3.

### 2.1 Build vs. use: library decisions

| Component | Decision | Reasoning |
|---|---|---|
| **BitTorrent protocol** | **Use libtorrent-rasterbar 2.0.x.** Do not write our own. | libtorrent has ~20 years of tuning: uTP congestion control, piece picker, DHT security extensions, v2/hybrid hashing, disk I/O, and hundreds of fixes for broken peers and trackers. A new stack would take years to match it and would be slower and less safe for a long time. qBittorrent, Deluge, and others use it; the speed difference between clients comes from **settings and app design**, which is where Rift competes. |
| How we beat other libtorrent clients | Tuning + architecture | Bulk `post_torrent_updates`, diffed models, SQLite resume store, staggered async startup, auto-scaled connection limits, per-OS disk I/O defaults, profiles, and benchmarks in CI (Section 10). Upstream any libtorrent fixes we find instead of forking. |
| **HTTP/FTP downloader engine** | **Write our own (`rift-get`)** on top of libcurl. | No existing library gives us what we want with a compatible license: **aria2** is GPL-2 (would force Rift to be GPL) and lacks HTTP/2; **libcurl** alone has no segmentation/resume logic; **Qt Network** lacks FTP, fine-grained connection control, and robust proxy/auth support; **Boost.Beast** has no HTTP/2/FTP/proxy auth. Our engine adds: dynamic segment splitting, adaptive connection count, mirror/multi-source scoring, a single-writer disk pipeline, crash-safe resume, and integration with the shared bandwidth arbiter. This is the part we can make better than what exists. |
| HTTP transport (sockets, TLS, HTTP/2, FTP, proxies, auth) | Use libcurl | Protocol correctness and security are not where we add value. |
| Everything else | Use the libraries in the table above | Small, permissive licenses, statically linkable. |

> Keep both engines behind Rift interfaces (`TorrentEngine`, `HttpEngine`) so the UI, automation, and storage layers treat a "download" generically. This also leaves the door open to swap or upgrade an engine (for example libtorrent 2.1) without touching the UI.

### Runtime dependency policy (important)
| Dependency | How it is handled |
|---|---|
| MSVC C++ runtime | Statically linked (`/MT`) so no Visual C++ Redistributable is required |
| libtorrent, Boost, OpenSSL, libcurl, nghttp2, libarchive, SQLite, QuickJS-ng, libmaxminddb | Statically linked into the executable |
| Qt 6 | Bundled alongside the exe (`windeployqt` on Windows, `macdeployqt` on macOS, `linuxdeploy` for AppImage); only required modules, plugins, and QML imports, trimmed to cut size |
| GeoIP DB, icons, fonts, translations | Bundled resources |
| Anything else needed | Must be bundled. If a future feature truly needs an optional component, the app downloads it on demand with a clear prompt, a progress bar, and checksum verification. Nothing needed for core features may require a manual download. |

### 2.2 Optimization and quality improvements (summary)
One checklist of every improvement made over a "default" libtorrent client. Details live in the referenced sections; the agent must implement all of them.

| # | Improvement | What it does | Where |
|---|---|---|---|
| 1 | **Own HTTP engine (`rift-get`) on libcurl** | Dynamic splitting, adaptive connections, mirrors, single-writer disk pipeline, crash-safe resume | 2.1, 4.14, Phase 4 |
| 2 | **Browser integration without a port** | Native Messaging helper (`rift-native-host`) talks over stdin/stdout; no listening socket | 0 (rule 10), 4.15, 9 |
| 3 | **Unified bandwidth arbiter** | One global limit, scheduler, and data cap across torrents and HTTP downloads | 3, 4.4 |
| 4 | **Resume data in SQLite (WAL)** | Batched atomic writes instead of thousands of small files; faster startup, fewer antivirus scans | 3, 6 |
| 5 | **Async, staggered startup** | UI shows the list from SQLite immediately; torrents are added with `async_add_torrent` in batches | 3 |
| 6 | **Per-OS disk I/O defaults** | pread/pwrite on Windows (avoids mmap working-set/perf issues), mmap on 64-bit Linux/macOS, POSIX on network drives; decided by the Phase 0 benchmark | 4.4, 7, Phase 0 |
| 7 | **Bulk, diffed UI updates** | `post_torrent_updates` + changed-rows-only model updates, slower when minimized | 3 |
| 8 | **Accurate speed display** | Measured payload speeds, smoothed + current values, totals that add up, honest acceleration indicator | 4.17 |
| 9 | **LTO + PGO release builds** | Smaller, faster binary; PGO profile from the benchmark suite | Phase 10 |
| 10 | **ed25519 (minisign) update signatures** | Small, auditable verification with an embedded public key | 2, 9, 12.4 |
| 11 | **QuickJS-ng plugin engine** | Small sandboxed JavaScript runtime for search plugins with time/memory caps | 2, 4.12 |
| 12 | **libmaxminddb GeoIP** | Fast offline country lookups for peers | 2 |
| 13 | **OS keychain for secrets** | HTTP/FTP/proxy passwords never stored in plain text | 2, 9 |
| 14 | **One shared TLS stack** | OpenSSL used by both engines, plus the OS root store on Windows | 2 |
| 15 | **"One process, one folder" clarified** | `Rift.exe` + bundled Qt libraries (LGPL), plus the small native-messaging helper | 3 |
| 16 | **Corrected cross-references** | Settings is Section 8; benchmarks run in Phase 10 | 0, 10 |

### 2.3 Feature additions at a glance (beyond the torrent client)
| Feature | Priority | Where |
|---|---|---|
| Accelerated HTTP/HTTPS/FTP downloads, Metalink, mirrors, checksums, batch/pattern add | P0-HTTP / P1-HTTP | 4.14 |
| Browser extensions (Chrome/Edge/Brave/Firefox) with download take-over | P1 | 4.15 |
| Accurate speed display and acceleration indicator | P0 / P1 | 4.17 |
| Mark-of-the-Web + antivirus scan trigger | P1 | 4.16 |
| Prevent sleep; sleep/shut down when all finish | P1 | 4.16 |
| Metered-network, battery, and game/fullscreen awareness | P1 | 4.16 |
| Import from qBittorrent, Transmission, Deluge | P1 | 4.16 |
| Disk-health aware I/O, smart folder rules, notes and "Downloaded from" | P1 | 4.16 |
| Clipboard watcher for direct links, connection inspector, history search, backup/restore | P1 | 4.16 |
| Speed test for auto-tuning, archive/media quick-look, `.meta4` watch folders | P2 | 4.16 |
| Quick-add mini window, auto-pick fastest source (torrent vs HTTP) | P2 | 4.16 |
| Site grabber (lite), non-DRM HLS/DASH | P2-HTTP | 4.14 |

---

## 3. Architecture

Single process, multiple threads. "Single executable" means one `Rift.exe` process shipped in one folder; Qt libraries sit beside it because LGPL requires dynamic linking. The only extra binary is the tiny `rift-native-host` used by browsers for Native Messaging (Section 4.15).

```
┌───────────────────────────────────────────────────────────────┐
│ Rift (single executable)                                      │
│                                                               │
│  UI thread (Qt Quick / QML)                                   │
│   ├─ TorrentListModel   (QAbstractItemModel, diff-updated)    │
│   ├─ FileTreeModel, PeerModel, TrackerModel, FilterModels     │
│   ├─ Controllers (Add, Settings, RSS, Search, Stats, Tray)    │
│   └─ Notifications, tray, shortcuts, command palette          │
│          ▲  batched, queued signals (cross-thread)            │
│          │                                                    │
│  Engine thread                                                │
│   ├─ libtorrent session wrapper                               │
│   ├─ alert loop (wait_for_alert → pop_alerts → events)        │
│   ├─ settings mapper, queue/limit manager                     │
│   └─ resume-data manager                                      │
│                                                               │
│  HTTP engine thread (rift-get)                                │
│   ├─ curl multi handle + event loop (curl_multi_poll)        │
│   ├─ segment scheduler (dynamic split, work stealing)         │
│   ├─ connection controller (adaptive count, per-host caps)    │
│   └─ mirror scorer, resume map, checksum verifier             │
│                                                               │
│  Disk writer thread (HTTP)                                    │
│   └─ coalesced positional writes into preallocated files      │
│                                                               │
│  Bandwidth arbiter (timer, 250 ms)                            │
│   └─ splits global/alt/scheduled limits between both engines  │
│                                                               │
│  Worker pool (QThreadPool)                                    │
│   ├─ RSS fetcher, blocklist loader, GeoIP, archive extraction │
│   ├─ torrent creation, hashing helpers                        │
│   └─ file operations (move, delete, rename)                   │
│                                                               │
│  Services: Scheduler · Automation (watch folders, rules, RSS) │
│            Network guard (VPN bind, kill switch, proxy)       │
│            Streaming server (localhost only) · Stats · Plugins│
│            Storage (SQLite) · Updater                         │
└───────────────────────────────────────────────────────────────┘
```

### Key engine design rules
- **One libtorrent `session`** for the whole app.
- **One alert loop** on the engine thread. Translate alerts to internal events; never poll `status()` per torrent.
- **Use `post_torrent_updates()`** and `state_update_alert` to get changed torrent statuses in bulk. Push only **changed rows** to the UI model, batched (default every 500 ms; 1000+ ms when the window is minimized/hidden; paused entirely when in tray and no tooltip needed except cheap global stats).
- **UI list uses a virtualized view** (`TableView`/`ListView` with delegate recycling) so thousands of torrents stay smooth.
- **Resume data** saved every 5 minutes (only for torrents flagged `need_save_resume`), on pause, on completion, and on clean shutdown. Store it in **SQLite (WAL)** as blobs (`write_resume_data_buf`) rather than thousands of small files: one transaction per batch is atomic, faster on startup, and avoids antivirus scanning many tiny files on Windows. Persist **session state** (DHT nodes, etc.) so startup is quick.
- **Fast startup:** load the torrent list into the UI from SQLite first (names, sizes, last-known state), then add torrents to the session with `async_add_torrent` in batches, staggered so hundreds of torrents don't spike CPU/disk. The UI is usable before the engine finishes loading.
- **Unified bandwidth arbiter:** global and alternative limits, the scheduler, and data caps are owned by the arbiter, not by each engine. Every 250 ms it measures demand from both engines and sets libtorrent's `download_rate_limit`/`upload_rate_limit` and the HTTP engine's token bucket so the combined total respects the user's limit. Priority between them is a setting (default: fair share; option: "HTTP first" for interactive downloads).
- Support **v1, v2, and hybrid** torrents.
- **Crash safety:** write resume data atomically (write temp, then rename). On startup, corrupt resume data for one torrent only forces a recheck of that torrent.
- **Single instance:** a second launch (for example from a magnet link click) forwards its arguments to the running instance over a local IPC channel (`QLocalSocket`/named pipe/Unix socket, user-only permissions) and exits. This is local-only and is not a network API.

---

## 4. Feature list by priority

Legend: **[P0]** required for MVP · **[P1]** v1.0 · **[P2]** after v1.0 · **[P0-HTTP] / [P1-HTTP] / [P2-HTTP]** the same tiers for the HTTP downloader, delivered in Phase 4 (P0-HTTP is required for v1.0, not for the torrent-only MVP)

### 4.1 Adding torrents
- [P0] Add via **magnet link**, **.torrent file**, **.torrent URL** (http/https).
- [P0] **Drag and drop** of files and text into the window.
- [P0] **Clipboard detection**: when the window gains focus and the clipboard holds a magnet link, show a subtle "Add magnet from clipboard?" toast (can be turned off).
- [P0] **Add dialog**: save path, category, tags, file tree with checkboxes and sizes, sequential toggle, start paused, skip hash check, content layout (original / subfolder / no subfolder), remembered last-used options.
- [P0] **Metadata state** for magnets ("Fetching metadata…" with peer count) and cancel.
- [P0] **Register as handler** for `magnet:` links and `.torrent` files (installer sets it; Settings has "Make Rift the default" with a status check).
- [P1] **Bulk add**: paste multiple magnets (one per line) or drop many .torrent files.
- [P1] **Duplicate detection** by info-hash: offer to merge trackers into the existing torrent.
- [P1] **Command line**: `rift add <magnet|file>`, `rift --version`, `rift --portable`, forwarded to the running instance.
- [P1] **Open with Rift** shell integration (Explorer, Finder, Linux file managers).

### 4.2 Core download behavior
- [P0] Start, pause, resume, remove (keep data / delete data options).
- [P0] **Force start** (ignores queue limits).
- [P0] **Force recheck** and **force reannounce**.
- [P0] **Fast resume** (no recheck on restart when resume data is valid).
- [P0] **Selective file download** plus per-file priority (skip / normal / high / max).
- [P0] **Sequential download** and **first/last piece priority**.
- [P0] **Move storage** with progress; **rename** torrent and files.
- [P1] **Pre-allocation** option (sparse vs full).
- [P1] **Incomplete files folder** (download to a fast drive, move on completion).
- [P1] **Append `.!part`** to incomplete files (option).
- [P1] **Disk-space check** before start; pause and notify when free space is below a threshold.
- [P1] **Super seeding** mode.
- [P1] **Per-torrent connection and upload-slot limits**.

### 4.3 Queue and prioritization
- [P0] Max active downloads, uploads, and total (`active_downloads`, `active_seeds`, `active_limit`).
- [P0] **Queue position** with move up/down/top/bottom and drag reordering.
- [P1] **Ignore slow torrents** in the active counts (inactivity thresholds).
- [P1] **Auto-manage** on/off per torrent.

### 4.4 Speed and performance
- [P0] Global download/upload limits and an **alternative speed limits** toggle.
- [P0] **Per-torrent** speed limits.
- [P0] **uTP + TCP** with sensible defaults.
- [P0] **DHT, PEX, LSD** on by default.
- [P0] **UPnP and NAT-PMP** port mapping, port test button, and a connection status indicator.
- [P1] **Bandwidth scheduler**: 7x24 grid where each cell is Normal / Alternative / Paused-all.
- [P1] **Performance profiles**: *Balanced* (default), *Max Speed*, *Low Resource*, *Background*.
- [P1] **Disk I/O tuning**: per-OS defaults chosen by benchmark in Phase 0/10. Starting point: **pread/pwrite (POSIX) disk I/O on Windows** (libtorrent 2.0's mmap backend shows inflated working set and slowdowns there), memory-mapped I/O on 64-bit Linux/macOS, and always POSIX on network drives or where mmap is unsafe; setting to override.
- [P1] **Smart connection limits**: auto-scale to OS file-descriptor limits and available RAM.
- [P1] **Peer turnover** and **choking algorithm** exposed under Advanced.
- [P2] **Auto-tuning wizard** (short test, suggested settings).

### 4.5 Privacy and network safety
- [P0] **Protocol encryption**: Prefer / Require / Disable (default Prefer).
- [P0] **Listen port**: fixed or random per launch.
- [P1] **Network interface binding** (choose the adapter, for example your VPN).
- [P1] **VPN kill switch**: if the bound interface goes down or its IP changes, pause all torrents immediately and notify; resume automatically or manually (setting).
- [P1] **Proxy support**: SOCKS5 (with/without auth), HTTP; options for peer connections, tracker connections, DHT, and hostname lookups.
- [P1] **IP blocklist**: import from file or URL (PeerGuardian `.p2p`, DAT, CIDR), scheduled auto-update.
- [P1] **Anonymous mode**.
- [P1] **Manual ban** and **auto-ban** of peers that repeatedly send bad data.
- [P1] **Private torrent awareness**: badge in the UI; DHT/PEX/LSD are disabled for private torrents.
- [P2] **IPv6** toggle with a leak check.
- [P2] **IP/port visibility test** against a user-configurable endpoint.

### 4.6 Automation (all local)
- [P0] **Watch folders**: any `.torrent` (or `.magnet` text file) dropped into a watched folder is added automatically; per-folder category, save path, start paused, delete-after-add.
- [P1] **Run on add / on completion** with variables: `%N` name, `%L` category, `%G` tags, `%F` content path, `%R` root path, `%D` save path, `%I` info-hash v1, `%J` info-hash v2, `%Z` size, `%T` current tracker. Arguments are passed directly to the process (no shell interpolation).
- [P1] **Rules engine** (if-this-then-that), for example:
  - IF category = "Movies" AND completed THEN move to a chosen folder
  - IF ratio >= 2.0 OR seeding time >= 7 days THEN pause / remove
  - IF free disk < 10 GB THEN pause all downloads
  - IF tracker errors persist > 24 h THEN add tag `dead-tracker`
- [P1] **Seeding limits** (global and per-torrent ratio and time) with action: pause / remove / remove with data / super seed.
- [P1] **RSS feeds**:
  - Feed manager (add, edit, refresh interval, enable/disable)
  - Auto-download rules: must-contain, must-not-contain (regex supported), episode filter (for example `S01E01-;`), smart episode filter (never re-download the same episode), category and save path per rule, ignore repeats for N days
  - **Rule preview** that shows which articles currently match
  - Article list with read/unread and manual download
- [P1] **Auto-extract archives** (zip, rar, 7z, tar variants) on completion via libarchive; options: extract to subfolder, delete archive afterward.
- [P1] **Auto-rename/sort** with templates such as `{category}/{name}`.
- [P2] **Scheduled actions** (for example "02:00 recheck all").

### 4.7 Organization
- [P0] **Categories** with default save path and optional defaults; nested (`Media/Movies`).
- [P0] **Tags** (many per torrent) with colors.
- [P0] **Sidebar filters**: All, Downloading, Seeding, Completed, Paused, Active, Inactive, Stalled, Errored, Checking, Moving; plus Category, Tag, and Tracker (grouped by domain with counts).
- [P0] **Instant search/filter** box (regex toggle).
- [P0] **Sortable, resizable, reorderable columns**, column chooser, remembered layout.
- [P1] **Saved smart views** (filter + sort + columns) shown in the sidebar.
- [P1] **Bulk actions** on multi-selection (pause, resume, category, tags, limits, remove, recheck, move).
- [P1] **Export/import torrent list** (JSON) and **export .torrent**.
- [P2] **Pin/favorite** torrents to the top.

### 4.8 Details panel (per torrent)
- [P0] **Overview**: progress bar with **piece map**, size, downloaded/uploaded, ratio, ETA, speeds, availability, seeds/peers (connected/swarm), save path, hashes, added/completed dates, comment, creator, private flag.
- [P0] **Files**: tree with progress, priority dropdown, open file, open folder, rename.
- [P0] **Peers**: IP, client, flags, progress, speeds, country flag, connection type (TCP/uTP), encryption indicator; actions: add peer, ban peer, copy IP.
- [P0] **Trackers**: URL, status, seeds/peers/downloaded counts, last/next announce, message; add, edit, remove, force reannounce; show DHT / PEX / LSD pseudo-rows.
- [P1] **Web seeds** (HTTP sources) add/remove.
- [P1] **Per-torrent speed graph**.
- [P1] **Piece grid** (have / downloading / missing / prioritized).
- [P1] **Per-torrent event log** (tracker errors, hash fails, file errors).
- [P1] **Copy** actions: magnet link, info-hash, path, name.

### 4.9 Streaming and preview (on-device)
- [P0 for streaming] **Sequential download + piece deadlines** via `set_piece_deadline`.
- [P1] **Local streaming endpoint** bound to `127.0.0.1` only, on a random port with a per-session random token in the URL, supporting HTTP Range, so VLC/mpv/the system player can play while downloading. Started only when the user clicks Stream and shut down when playback stops or Rift closes. It must never bind to non-loopback addresses.
- [P1] **"Stream" button** on video files: opens the default or configured external player.
- [P1] **Built-in video player** (Qt Multimedia) with subtitle auto-detect (`.srt`, `.vtt`, `.ass`).
- [P1] **Buffering indicator** (percentage of the lookahead window ready).
- [P2] **Preview thumbnails** for downloaded videos and images.

### 4.10 Stats and history
- [P1] **Live graph** (5 min / 1 h / 24 h) for global download, upload, and connections.
- [P1] **Persistent statistics**: daily/weekly/monthly downloaded and uploaded totals in SQLite.
- [P1] **Data cap / monthly quota** with warnings and optional auto-pause.
- [P1] **Session stats panel**: DHT nodes, total peers, disk queues, cache hit rate, announces, TCP/uTP split.
- [P1] **History** of completed torrents (name, size, date, ratio) that survives removal.
- [P2] **Per-tracker stats**.

### 4.11 Torrent creation
- [P1] **Create torrent**: file/folder, piece size (auto/manual), trackers, web seeds, comment, private flag, v1/v2/hybrid, start seeding after creation, progress, save `.torrent`, copy magnet.
- [P2] **Saved tracker templates**.

### 4.12 Search (plugin-based, on-device)
- [P1] **Plugin system** for search providers: a small script or module implementing `search(query, category)` with a manifest (name, version, categories, URL). Use an embedded scripting engine (for example QuickJS or embedded Lua) so no external runtime has to be installed.
- [P1] **Search tab**: query, plugin selector, category filter, sortable results (name, size, seeds, leechers, source), Download button, open description page.
- [P1] **Plugin manager**: install from file/URL, enable/disable, update check, sandboxing (timeouts, no filesystem write, only HTTP GET to declared domains).
- [P1] **No plugins bundled.** Show a neutral empty state with docs on writing plugins.

### 4.13 UX polish and small features
- [P0] **Dark / Light / System** theme plus accent color picker.
- [P0] **Compact vs Comfortable** density.
- [P0] **Window state memory** (size, position, splitters, columns, selected filter).
- [P0] **System tray / menu bar icon**: show/hide, pause all/resume all, speeds in tooltip, alt-speed toggle, quit.
- [P0] **Close to tray**, **start minimized**, **launch at login** options.
- [P0] **Status bar**: down/up speed, limits, DHT nodes, connection status (green/yellow/red with a tooltip explaining port-forward status), free disk space, alt-speed toggle.
- [P0] **Native notifications** (Windows toast, macOS Notification Center, Linux freedesktop): complete, error, disk low, VPN drop, RSS match; each toggleable; clicking opens the torrent.
- [P1] **Command palette** (`Ctrl/Cmd+K`): any action, jump to a torrent by name, jump to a category.
- [P1] **Keyboard shortcuts** for everything common (Del, Space, Ctrl+O, Ctrl+V, Ctrl+F, Ctrl+A), cheat sheet (`?`), rebindable.
- [P1] **Right-click context menus** everywhere.
- [P1] **Taskbar / dock progress** indicator.
- [P1] **Completion sounds** (optional).
- [P1] **Undo for removal** (8-second toast) when data is not deleted.
- [P1] **Confirm before deleting data**, with "don't ask again" for removal without data.
- [P1] **ETA smoothing** (moving average; see 4.17).
- [P1] **Human-friendly numbers**: auto units, bytes or bits for speeds (4.17), relative dates with absolute on hover.
- [P1] **Empty states** with hints ("Drop a .torrent here or press Ctrl+V to paste a magnet").
- [P1] **Optimistic UI**: pause/resume reflect instantly and reconcile with the engine.
- [P1] **Open containing folder / open file** everywhere.
- [P1] **"Why is it stalled" tooltip** (no seeds / waiting for metadata / disk full / paused by schedule / paused by kill switch).
- [P1] **Health indicator** per torrent (Healthy / Slow / Dead) from availability.
- [P1] **One-click copy magnet** from the list.
- [P1] **In-app update check** (see Section 12) with stable/beta channel; updates are downloaded, signature-verified, then installed on user confirmation.
- [P1] **Localization framework** (Qt translations) with English first.
- [P1] **Accessibility**: full keyboard navigation, screen-reader names (UI Automation / NSAccessibility / AT-SPI via Qt), contrast-checked themes, honor reduced-motion.
- [P1] **First-run wizard**: download folder, performance profile, port test, optional VPN interface, default-handler prompt.
- [P1] **Diagnostics page**: libtorrent/Qt versions, settings dump, port-test result, DHT status, OS limits, "Copy diagnostics".
- [P2] **Log viewer** with level filter and export.
- [P2] **Mini mode**: small always-on-top window with total speeds and top 3 active torrents.
- [P2] **Disk usage by category** and an **orphaned files cleaner**.
- [P2] **Multiple profiles**.

### 4.14 Accelerated HTTP/HTTPS/FTP downloader (`rift-get`)

**What actually makes it faster (be honest in the UI and docs):** parallel connections cannot exceed the user's line speed. They help when (a) the server or a CDN throttles **per connection**, (b) a single TCP stream is limited by latency and window size (long-distance or lossy links), (c) a connection stalls and another can take over its work, or (d) several mirrors can be used at once. Rift measures the gain and stops adding connections when it stops helping.

**Core behavior**
- [P0-HTTP] Add a download from a URL (paste, drag a link, clipboard detection, command line `rift get <url>`), with an add dialog: file name (from `Content-Disposition`, then URL, editable), save folder, category (auto-picked by file type: Video, Music, Archives, Documents, Programs, Other), connection count (Auto / 1–32), start now / later / scheduled, checksum.
- [P0-HTTP] **Probe first:** one `GET` with `Range: bytes=0-` (not a `HEAD`, many servers lie on `HEAD`) to learn size, `Accept-Ranges`, `ETag`, `Last-Modified`, final URL after redirects, and file name. Keep this connection as segment #1 instead of throwing it away.
- [P0-HTTP] **Single-connection fallback** when the server has no range support or no length (chunked/streamed); show "Server does not support resume" clearly.
- [P0-HTTP] **Pause / resume / restart / remove (keep or delete file)**, survive app crash and reboot.
- [P0-HTTP] **Crash-safe resume:** the segment map (list of completed byte ranges) is persisted to SQLite every 2 s and on pause/exit. On resume, validate with `If-Range: <ETag or Last-Modified>`; if the file changed on the server, ask to restart instead of producing a corrupt file.
- [P0-HTTP] **Unified list:** HTTP downloads appear in the same torrent/download list with a type icon, share filters, categories, tags, queue, notifications, run-on-complete, and the bandwidth arbiter.

**Segmentation engine (the part we write ourselves)**
- **Dynamic segmentation (work stealing):** start with N segments (Auto: 4, ramping up). When a connection finishes its segment, it **splits the largest remaining segment in half** and takes the second half, so all connections stay busy until the end and there is no slow "last segment" tail. Minimum segment size 1 MiB (setting).
- **Adaptive connection count:** add one connection at a time every ~2 s while total throughput improves by > 5 %; stop at the per-host cap (default 8, max 32) or when the gain flattens; drop a connection if it is the slowest by a wide margin for > 10 s and give its range to a faster one.
- **Endgame duplication:** for the last few hundred KiB, race the slow tail on a second connection and keep whichever finishes first.
- **Stall detection:** no bytes for 15 s (setting) → reconnect that segment with exponential backoff and jitter; treat HTTP 429/503 with `Retry-After` politely.
- **Per-host politeness:** global per-host connection cap so several downloads from one server do not hammer it (and do not get the user banned).
- **HTTP/2 and HTTP/3:** HTTP/2 multiplexing puts all ranges on one TCP connection, which removes the parallel-TCP benefit, so for segmented downloads Rift opens **separate connections** by default (`CURLPIPE_NOTHING` per handle), and uses multiplexing only for small files or when the server limits connection count. HTTP/3 (QUIC) is P2 and opt-in.
- **Connection reuse and DNS:** reuse keep-alive connections across segments; cache DNS; try all resolved IPs (happy eyeballs) and spread connections across them when a host has several A/AAAA records.

**Disk pipeline**
- Preallocate the target file (sparse by default; full allocation option) as `name.ext.riftpart`, renamed on completion.
- All network handles hand buffers to **one disk-writer thread** that coalesces adjacent blocks and writes with positional writes (`pwrite` / `WriteFile` with `OVERLAPPED` offsets) in 1–4 MiB chunks. This avoids per-segment temp files and the slow "merging/assembling" step other downloaders have.
- Memory cap for in-flight buffers (default 64 MiB); apply backpressure (`CURL_WRITEFUNC_PAUSE`) when the disk is slower than the network.

**Sources and integrity**
- [P1-HTTP] **Mirrors / multi-source:** add extra URLs for the same file; each mirror is scored by speed and error rate and gets segments proportionally. Verify the mirrors agree on size (and ETag when present).
- [P1-HTTP] **Metalink v4 (`.meta4`) and v3** import: mirrors, sizes, whole-file and per-piece hashes; a bad piece is re-downloaded from another mirror. A metalink that also lists a torrent can use both.
- [P1-HTTP] **Checksum verification:** MD5/SHA-1/SHA-256/SHA-512 typed by the user, read from a sibling `.sha256`/`SHA256SUMS` file, or from `Digest`/`Repr-Digest` headers; green/red result in the details panel.
- [P1-HTTP] **Torrent + HTTP hybrid:** for torrents, let the user add HTTP mirrors as web seeds (BEP 19) from the same UI.

**Protocols, auth, network**
- [P0-HTTP] HTTP/HTTPS with redirects (limit 10), gzip/br decoding off for binary downloads.
- [P1-HTTP] FTP/FTPS (passive/active, resume with `REST`).
- [P1-HTTP] Basic/Digest/NTLM auth; per-site saved credentials in the OS keychain; custom headers, cookies, referer, and user agent per download (normally supplied by the browser extension).
- [P1-HTTP] Uses the same proxy, interface binding, and **VPN kill switch** as the torrent engine.
- [P1-HTTP] **Refresh expired link:** when a signed URL returns 403/410 mid-download, offer "Get a new link" (opens the source page; with the browser extension, the next matching download is linked back to the paused item and resumes at the same offset if the file is identical).

**Convenience**
- [P1-HTTP] **Batch download:** paste many URLs, or a pattern like `https://host/file[001-120].jpg`.
- [P1-HTTP] **Queues and schedules** shared with torrents (start at 02:00, stop at 07:00, max N simultaneous).
- [P1-HTTP] **Per-download speed limit** and connection count, changeable while running.
- [P1-HTTP] **Duplicate detection** (same URL or same final file name in the folder): ask to rename, overwrite, or resume the existing file.
- [P2-HTTP] **Site grabber (lite):** list links on a page that match a filter (extension/regex) and download them; no site-specific scrapers.
- [P2-HTTP] **HLS/DASH (non-DRM only)** download of media playlists the user opens, merged into one file; encrypted/DRM streams are refused.

### 4.15 Browser integration (no network port)
- [P1] **Extensions for Chrome/Edge/Brave (MV3) and Firefox** that offer "Download with Rift" in the context menu and can optionally **take over downloads** by file type and minimum size (with a modifier key to bypass).
- Communication uses **Native Messaging**: the browser launches `rift-native-host` (registered by the installer for the extension IDs only) and exchanges JSON over stdin/stdout. The host forwards the request to the running Rift over the existing user-only single-instance IPC (`QLocalSocket`/named pipe). **No TCP/HTTP port is opened.**
- The extension passes URL, referer, cookies for that URL, user agent, and suggested file name, so downloads behind a login work. Cookies are used only for that download and never persisted beyond its resume needs.
- Optional "media detected on this page" button for direct media files (`.mp4`, `.mp3`, etc.) only; never for DRM streams.
- Store listings ship separately; the installer registers the native host manifest (Windows registry, macOS/Linux JSON manifest paths).

### 4.16 Additional quality features (new in v3)
- [P1] **Mark-of-the-Web:** downloaded files get `Zone.Identifier` on Windows (via `IAttachmentExecute`, which also triggers the installed antivirus scan) and the `com.apple.quarantine` attribute on macOS. Setting to disable for torrents if the user insists.
- [P1] **Prevent sleep while downloading** (`SetThreadExecutionState`, IOPMAssertion, systemd-inhibit/DBus), released when idle.
- [P1] **When all downloads finish:** do nothing / sleep / hibernate / shut down / quit Rift, with a 60-second cancelable countdown.
- [P1] **Metered network awareness:** on Windows metered connections (and NetworkManager metered on Linux) pause or switch to alternative limits; setting.
- [P1] **Battery awareness:** on battery, switch to the *Low Resource* profile or pause; resume on AC.
- [P1] **Game / fullscreen mode:** when a fullscreen app or game is in the foreground, auto-apply alternative limits and lower uTP priority to protect latency.
- [P1] **Import from other clients:** qBittorrent (`BT_backup` fastresume + categories/tags), Transmission, Deluge; import HTTP lists from text/CSV. Lets users switch without rechecking everything.
- [P1] **Disk-health aware I/O:** detect HDD vs SSD per save path and adjust write coalescing and active checking count.
- [P1] **Smart folder rules:** route by file type/extension or source domain to folders (for example `*.iso` → `Downloads/ISO`).
- [P1] **Per-download notes** and a **"Downloaded from"** field (source URL / referer, magnet) kept in history.
- [P2] **Download speed test** (to a user-configured URL) used by the auto-tuning wizard to pick connection counts and limits.
- [P2] **Quick-look preview** of partially downloaded media/archives (list archive contents before completion when the central directory is available).
- [P2] **Watch folder for `.meta4`/URL lists** in addition to `.torrent`/`.magnet`.
- [P1] **Clipboard watcher for direct links:** besides magnets, offer to download copied URLs whose extension matches a user list (`.zip`, `.iso`, `.mp4`, ...); off by default.
- [P1] **Connection inspector** for HTTP downloads: each connection's byte range, speed, server IP, HTTP version, retries; for torrents the existing Peers tab.
- [P1] **History search and re-download:** find any past download (torrent or URL) by name, source, or date and start it again.
- [P1] **Backup and restore:** export all Rift data (settings, categories, rules, RSS, history, resume data, `.torrent` files) to one archive and restore it on another machine.
- [P2] **Quick-add mini window** from the tray: paste a link, pick a category, done.
- [P2] **Pick the fastest source automatically** when a Metalink or page offers both a torrent and HTTP mirrors: start both, keep whichever is faster after a short test (or use both).

### 4.17 Accurate speed display (real numbers only)
Rift always shows the **speed that is actually happening**, never an estimated, "boosted", or inflated number.
- [P0] **Measured, not estimated:** speeds come from bytes really received/sent: libtorrent's `download_payload_rate`/`upload_payload_rate` for torrents, and bytes delivered by libcurl write callbacks for HTTP. Nothing is extrapolated.
- [P0] **Payload by default:** the main number is file data only (what actually ends up on disk). Protocol overhead (headers, TLS, BitTorrent messages, DHT) is shown separately in tooltips and the stats panel, so totals can be matched with the OS network monitor.
- [P0] **Two time windows:** a *current* value (1 s window, updated twice per second) and a *smoothed* value (exponential moving average over ~5 s) used for the list, ETA, and tray, so numbers don't jump around but still respond within a few seconds. Both are exposed; the user can choose which one the list shows.
- [P0] **Consistent totals:** the status-bar total equals the sum of all rows (torrents + HTTP), and equals what the bandwidth arbiter counts against the limit.
- [P0] **Zero means zero:** when nothing arrives, the speed drops to 0 within 2 s (no lingering average) and the "why is it stalled" reason is shown.
- [P1] **Limit context:** when a limit is active, show it next to the speed (for example `4.8 MB/s of 5 MB/s limit`) so users know the speed is capped, not slow.
- [P1] **Per-connection / per-peer speeds** (connection inspector, Peers tab) add up to the row speed.
- [P1] **Honest acceleration indicator (HTTP):** in the details panel, show the measured speed of the first connection and the current total speed (for example "1 connection: 1.2 MB/s → 8 connections: 9.4 MB/s"), only from measured data. If extra connections don't help, it says so and Auto mode backs off.
- [P1] **Average speed and active time** for each finished download (total bytes ÷ time actually downloading, excluding paused time), kept in history.
- [P1] **ETA** = remaining bytes ÷ smoothed speed; shows `∞` / "Unknown" when the speed is 0 or the size is unknown, never a made-up time.
- [P1] **Units:** bytes (KB/s, MB/s) or bits (Mbit/s, matching ISP plans) toggle; binary vs decimal prefixes setting; same units everywhere.
- [P1] **Tests:** unit tests feed known byte streams to the speed meter and check current/smoothed values, the drop to zero, and that row speeds add up to the total within 1 %.

---

## 5. UI screens (content inventory; design is the next deliverable)

1. **Main window**: left sidebar (filters, categories, tags, trackers, saved views) · center torrent list · dockable/resizable details panel · status bar.
2. **Add Torrent dialog** and **Add Download (URL) dialog**
3. **Settings**: General, Downloads, Connection, Speed (with scheduler grid), BitTorrent, Privacy and VPN, Queueing, Automation (watch folders, rules, RSS), Notifications, Appearance, Advanced.
4. **Search tab**
5. **RSS tab** (feeds, articles, rules)
6. **Statistics tab**
7. **Create Torrent dialog**
8. **Command palette overlay**
9. **First-run wizard**
10. **Diagnostics / Logs**
11. **Built-in player window**
12. **Mini mode** (P2)

---

## 6. Data model (SQLite)

Use versioned migrations.

- `torrents` (info_hash_v1, info_hash_v2, name, save_path, category_id, added_at, completed_at, ratio_limit, seed_time_limit, flags, queue_pos)
- `categories` (id, name, parent_id, save_path, defaults_json)
- `tags` (id, name, color) and `torrent_tags`
- `settings` (key, value_json), schema-validated and versioned
- `rss_feeds`, `rss_articles`, `rss_rules`
- `watch_folders`
- `rules` (id, name, enabled, trigger, conditions_json, actions_json)
- `stats_daily` (date, downloaded, uploaded) and `stats_tracker`
- `history`
- `saved_views` (id, name, filter_json, sort_json, columns_json)
- `blocklists`
- `schedule` (day, hour, mode)

- `resume_data` (info_hash, blob, updated_at): libtorrent resume data, written in batched transactions
- `http_downloads` (id, url, final_url, file_path, size, etag, last_modified, state, category_id, connections, speed_limit, checksum_algo, checksum_value, headers_json, referer, added_at, completed_at, queue_pos)
- `http_segments` (download_id, start, end, done_bytes): completed/in-progress ranges for crash-safe resume
- `http_mirrors` (download_id, url, score, errors)
- `site_credentials` (host, username, keychain_ref): password itself lives in the OS keychain

`.torrent` files are kept as files (`torrents/<infohash>.torrent`) so users can find and export them; resume data lives in SQLite (Section 3). Keep a `--export-resume` diagnostic that writes `.fastresume` files for compatibility.

### App data locations
| OS | Location |
|---|---|
| Windows | `%APPDATA%\Rift` (settings, DB), `%LOCALAPPDATA%\Rift` (cache, logs) |
| macOS | `~/Library/Application Support/Rift`, `~/Library/Logs/Rift` |
| Linux | `~/.config/rift`, `~/.local/share/rift`, `~/.cache/rift` |
| Portable mode | `Data/` folder next to the executable, enabled when a `portable.flag` file exists beside it (the portable ZIP ships with it) |

---

## 7. libtorrent configuration (starting defaults)

Build a `SettingsPack` mapper from Rift settings to libtorrent settings. **Apply changes live** where libtorrent allows.

- `listen_interfaces`: `0.0.0.0:<port>,[::]:<port>` (or the bound interface)
- `enable_dht`, `enable_lsd`, `enable_upnp`, `enable_natpmp`: true
- `dht_bootstrap_nodes`: standard public bootstrap nodes (configurable)
- `out_enc_policy` / `in_enc_policy`: `pe_enabled`; `allowed_enc_level`: `pe_both`
- `connections_limit`: 500 global, ~100 per torrent default; auto-scaled by OS limits
- `active_downloads`: 5, `active_seeds`: 5, `active_limit`: 15, `active_checking`: 1
- `max_queued_disk_bytes`: raised from the default for fast links
- `send_buffer_watermark`, `send_buffer_low_watermark`, `send_buffer_watermark_factor`: tuned for high-bandwidth links
- `mixed_mode_algorithm`: peer-proportional so uTP stays polite
- Disk I/O: `posix_disk_io_constructor` on Windows, `mmap_disk_io_constructor` on 64-bit Linux/macOS (see 4.4); `aio_threads`/`hashing_threads` scaled to CPU cores and drive type
- `alert_mask`: only the categories needed (status, error, storage, tracker, peer, port_mapping, stats; piece_progress only while the details panel needs it)
- `auto_manage_startup`: staggered startup so hundreds of torrents don't spike load
- `anonymous_mode`: false by default
- `user_agent` / peer fingerprint: `Rift/x.y.z` with its own peer-ID prefix (never impersonate another client)

Handle all alerts relevant to UX, including: add_torrent, torrent_finished, torrent_error, torrent_paused/resumed, state_changed, state_update, metadata_received/failed, save_resume_data(+failed), storage_moved(+failed), file_error, hash_failed, tracker_error/reply/warning/announce, portmap(+error), external_ip, listen_succeeded/failed, peer_ban, session_stats.

---

## 8. Settings catalog (so nothing is forgotten)

**General:** language, theme, accent, density, start on login, start minimized, close to tray, confirm on exit, check for updates, release channel, default-handler status.
**Downloads:** default save path, incomplete path, category defaults, pre-allocate, `.!part` suffix, auto-delete `.torrent` after add, copy `.torrent` to a folder, content layout default, minimum free space, auto-extract options, run on add/complete.
**Connection:** listen port (fixed/random), UPnP/NAT-PMP, max connections (global/per-torrent), upload slots, proxy, IPv6, interface binding, kill switch.
**Speed:** global limits, alternative limits, scheduler grid, apply limits to uTP / overhead / LAN peers.
**BitTorrent:** DHT, PEX, LSD, encryption mode, anonymous mode, announce-to-all, max active torrents, slow-torrent thresholds, seeding limits and action.
**Privacy:** blocklist sources and update interval, auto-ban, GeoIP toggle, "privacy mode" (don't log names/magnets).
**Automation:** watch folders, RSS options (refresh interval, max articles per feed), rules.
**Streaming:** external player path, built-in player options, lookahead buffer size.
**HTTP downloader:** default connections (Auto/1–32), per-host cap, minimum segment size, stall timeout, retries and backoff, separate connections vs HTTP/2 multiplexing, sparse vs full allocation, in-flight memory cap, default user agent, file-type categories and folders, duplicate handling, auto-verify checksums, bandwidth priority vs torrents.
**Browser integration:** enable, take over downloads (file types, minimum size), bypass modifier key, show "media detected" button, registered browsers status.
**Power and network:** prevent sleep, action when all finished, metered-network behavior, battery behavior, game/fullscreen mode behavior.
**Notifications:** per-event toggles, sounds.
**Advanced:** raw libtorrent settings editor (validated key/value with reset), disk I/O mode, queue sizes, log level, data directory, import/export settings (JSON).

All settings are schema-validated, versioned, import/exportable, and hot-reloaded where possible.

---

## 9. Security requirements (on-device model)

- **No network control surface.** The only listening sockets are the BitTorrent port and, while streaming, a loopback-only HTTP endpoint with a random token. Add a unit test that fails if the streaming server ever binds to a non-loopback address.
- **Single-instance IPC** uses a local socket/named pipe restricted to the current user.
- **Path safety:** sanitize every path from torrents (reject `..`, absolute paths, reserved Windows names, trailing dots/spaces); handle case-collisions and long paths (enable Windows long-path support in the manifest).
- **Run-on-complete commands:** execute without a shell; pass variables as arguments.
- **Plugin sandbox:** embedded script engine, timeouts, memory cap, no file or process access, network limited to declared domains.
- **Update integrity:** updates are fetched over HTTPS and verified against a signature (public key embedded in the app) before installing.
- **Code signing:** Windows Authenticode, macOS Developer ID signing and notarization.
- **Privacy:** no telemetry by default. Crash reports are opt-in and exclude torrent names and paths.
- **Hardened builds:** ASLR/DEP/CFG on Windows, hardened runtime on macOS, PIE and RELRO on Linux.
- **HTTP downloader:** TLS certificate verification is always on (a per-download "ignore certificate errors" requires an explicit, logged user choice); HTTPS → HTTP redirect downgrades warn; redirect limit; file names from servers go through the same path sanitizer; never auto-open/execute downloaded files; Mark-of-the-Web applied (4.16).
- **Native messaging host:** accepts only the registered extension IDs (`allowed_origins`), validates and size-limits every message, can only *enqueue a download for user confirmation or a configured take-over rule*; it cannot read files, change settings, or run commands.
- **Credentials and cookies:** passwords in the OS keychain only; cookies from the browser kept only for the life of that download; both excluded from logs, diagnostics, and crash reports.

---

## 10. Performance targets (measure in Phase 10)

| Metric | Target |
|---|---|
| Cold start to usable UI (500 torrents) | < 1.5 s |
| Idle RAM (100 torrents) | < 150 MB |
| Idle CPU | < 1% |
| UI list with 5,000 torrents | 60 fps scrolling, < 50 ms filter response |
| UI update cost with 500 active torrents | < 3% CPU |
| Download throughput | Within 5% of qBittorrent on the same torrent, machine, and network |
| Graceful shutdown (save resume data, 500 torrents) | < 5 s |
| HTTP: throughput on a per-connection-throttled test server (1 MB/s per connection, 100 MB/s link) | ≥ 90 % of link speed with Auto connections |
| HTTP: throughput vs IDM / aria2 (`-x16 -s16`) on the same public mirror | Equal or better (median of 5 runs) |
| HTTP: CPU at 1 Gbit/s single download | < 10 % of one core |
| HTTP: resume after kill -9 mid-download | Resumes from the last persisted offset (≤ 2 s of data lost), final hash matches |
| HTTP: time from "Download" click to first byte | < 500 ms on a healthy server |
| Windows installer size | Target < 60 MB; installed size < 150 MB |
| Portable ZIP size | Target < 60 MB |

---

## 11. Phased roadmap

### Phase 0: Foundations and build pipeline
- Repo and CMake + vcpkg setup (pinned dependency versions), CI matrix for Windows/macOS/Linux, formatting (clang-format), static analysis (clang-tidy), pre-commit hooks.
- Hello-world Qt Quick window plus a console spike that downloads a Linux ISO magnet with libtorrent, and a second spike that downloads the same ISO over HTTPS with libcurl using 8 ranged connections into one preallocated file.
- **Disk I/O benchmark spike:** compare libtorrent mmap vs POSIX disk I/O on Windows (NVMe, HDD, network share) to lock in the defaults in 4.4.
- **Prove the packaging early:** produce a trivial Windows build that runs on a clean Windows 10 VM with no runtimes installed; same on macOS and a clean Linux VM.
- **Accept:** the spike downloads a file on all three OSes; the packaged hello-world runs on clean machines with zero prerequisites.

### Phase 1: Engine core
- Session wrapper, alert loop on the engine thread, settings mapper, resume data save/load, graceful shutdown, SQLite + migrations, categories/tags storage.
- Engine event bus and batched model updates to the UI thread.
- **Accept:** a headless test harness drives add/pause/resume/remove/recheck; restart resumes without recheck; engine unit and integration tests pass using a local test swarm.

### Phase 2: Minimal UI (MVP)
- Speed meter module (4.17, P0 items) used by every speed shown in the UI.
- Main window: virtualized torrent list, sidebar filters, add dialog, details panel (Overview, Files, Peers, Trackers), status bar, dark/light theme, tray icon, notifications, single-instance forwarding, magnet/.torrent associations (dev installs).
- **Accept:** a non-technical user can add, watch, pause, and finish a download entirely from the UI.

### Phase 3: Management features
- Queueing, limits, alt speeds, per-torrent limits, categories/tags UI, bulk actions, context menus, shortcuts, search/filter, column customization, move storage, rename, recheck, reannounce, force start, sequential and first/last, incomplete folder, disk-space guard.
- **Accept:** every P0 item in 4.1 to 4.8 works with tests.

### Phase 4: HTTP downloader engine (`rift-get`)
- libcurl multi loop on its own thread, probe request, single-connection fallback, dynamic segmentation with work stealing, adaptive connection count, endgame duplication, stall detection and backoff, per-host caps, disk-writer thread with preallocation and coalesced positional writes, SQLite segment map and `If-Range` resume, checksum verification, add dialog, unified list rows and details panel (segments view showing each connection's range and speed), bandwidth arbiter shared with libtorrent.
- Then P1-HTTP: mirrors and scoring, Metalink, FTP/FTPS, auth + keychain, batch/pattern add, duplicate handling, refresh-expired-link flow.
- Engine is a separate static library (`src/http/`) with no Qt dependency in its core, unit-testable on its own.
- **Accept:** against the local test server (Section 14): reaches ≥ 90 % of link speed when the server throttles per connection; resumes correctly after `kill -9` with a matching SHA-256; falls back cleanly when ranges are unsupported; detects a changed file via ETag and refuses to splice it; combined torrent + HTTP speed stays within 5 % of the global limit.

### Phase 5: Network, privacy, and speed
- Interface binding + kill switch, proxy, encryption modes, blocklist, auto-ban, UPnP/NAT-PMP + port test, IPv6, anonymous mode, performance profiles, bandwidth scheduler, connection auto-scaling, GeoIP flags. Proxy, interface binding, kill switch, and scheduler cover HTTP downloads as well.
- **Accept:** kill switch pauses everything (torrents and HTTP) within 2 s of the interface dropping (tested with a virtual interface); scheduler applies the right limits at the right hour.

### Phase 6: Automation
- Watch folders, run-on-add/complete, seeding limits, rules engine, RSS (feeds, rules, preview, smart episode filter), auto-extract, auto-rename/sort, smart folder rules by file type/domain. Rules and hooks apply to HTTP downloads too.
- **Accept:** a test RSS feed fixture triggers a download exactly once per episode; a rule moves a completed torrent and extracts its archive.

### Phase 7: Streaming and media
- Piece deadlines, loopback streaming endpoint, Stream button, external player launch, built-in player with subtitles, buffering indicator.
- **Accept:** a video starts playing in VLC/mpv and the built-in player within seconds on a healthy swarm; seeking re-prioritizes pieces; the endpoint is verified loopback-only.

### Phase 8: Packaging, distribution, and browser integration (all OSes)
- Windows installer, portable ZIP, macOS DMG + ZIP, Linux AppImage + DEB + RPM + tar.gz (Section 12).
- Code signing, auto-update, file associations, uninstall behavior, release pipeline.
- `rift-native-host`, native-messaging manifests registered by every installer, Chrome/Edge/Firefox extensions (4.15).
- **Accept (extra):** with the extension installed, "Download with Rift" on a login-protected file downloads it with the right cookies, and no new listening socket appears (`netstat` check in CI).
- **Accept:** every artifact installs and runs on a clean VM of each target OS with no extra downloads; install to a custom path works; uninstall leaves no stray files (except user data when the user chooses to keep it).

### Phase 9: Stats, creation, search, polish
- Stats and graphs, data caps, history, create-torrent, plugin system and search tab, command palette, first-run wizard, diagnostics, localization framework, accessibility pass, log viewer, import from other clients, power/metered/battery/game-mode features (4.16).
- **Accept:** a new user finishes first-run setup in under 60 seconds; keyboard-only use and screen-reader labels pass an accessibility audit.

### Phase 10: Hardening and release
- Benchmark against Section 10 and tune libtorrent and HTTP engine defaults; enable LTO and **PGO** (profile from the benchmark suite) for release builds.
- Soak test (1,000+ torrents + 200 HTTP downloads, 72 h), memory-leak checks (ASan/Valgrind/Instruments), fuzzing of torrent/magnet/RSS/metalink/HTTP-header/native-messaging parsers, security review, opt-in crash reporting, docs, changelog.
- **Accept:** all targets in Section 10 met; no open P0/P1 bugs.

---

## 12. Packaging, installers, and updates

### 12.1 Windows (primary target)

**Artifacts**
| File | What it is |
|---|---|
| `Rift-<ver>-windows-x64-setup.exe` | Installer (Inno Setup or WiX), everything inside, works offline |
| `Rift-<ver>-windows-x64-portable.zip` | Portable build: unzip and run `Rift.exe`; ships with `portable.flag` so all data stays in a `Data/` folder beside it |
| `Rift-<ver>-windows-arm64-*` | P2: same artifacts for Windows on ARM |

**Installer requirements**
- **Custom install location** (default `%ProgramFiles%\Rift`, or `%LocalAppData%\Programs\Rift` when installing for the current user only).
- **Install scope choice:** current user (no admin needed) or all users (admin).
- Optional checkboxes: desktop shortcut, Start Menu entry, launch at login, associate `.torrent`, handle `magnet:` links, add Windows Firewall rule for Rift, add to PATH for the `rift` CLI.
- **No prerequisites to download:** the C++ runtime is statically linked and Qt DLLs are bundled, so the installer never asks the user to fetch anything.
- **Upgrade in place**, preserving settings; detect a running instance and ask to close it; support silent install (`/SILENT`, `/VERYSILENT`, `/DIR=`).
- **Uninstaller** removes the app, shortcuts, associations, and firewall rule, and asks whether to keep or delete user data.
- Signed with an Authenticode certificate to reduce SmartScreen warnings.
- Windows manifest: long-path aware, per-monitor DPI aware, UTF-8 code page.
- **Folder layout (portable and installed):**
  ```
  Rift/
  ├─ Rift.exe
  ├─ Qt6Core.dll, Qt6Gui.dll, Qt6Quick.dll, ... (only needed modules)
  ├─ platforms/, imageformats/, qml/, multimedia/, tls/ (trimmed plugins)
  ├─ translations/
  ├─ resources/ (GeoIP DB, icons)
  ├─ LICENSES/ (Qt LGPL notices, third-party licenses)
  └─ portable.flag      (portable ZIP only)
  ```
- Optional later: winget, Scoop, and Chocolatey manifests (P2).

### 12.2 macOS
- `Rift-<ver>-macos-universal.dmg` (Apple Silicon + Intel, drag to Applications) and a `.zip`.
- Frameworks bundled inside `Rift.app` via `macdeployqt`; hardened runtime; **Developer ID signed and notarized**; magnet/.torrent associations declared in `Info.plist`; menu bar status item; Dock progress.
- Minimum macOS 12.

### 12.3 Linux
- **AppImage** (primary; everything bundled, runs on most distros without install).
- **DEB** (Debian/Ubuntu) and **RPM** (Fedora/openSUSE) with desktop entry, MIME associations (`application/x-bittorrent`, `x-scheme-handler/magnet`), icons.
- **tar.gz** portable archive (with `portable.flag` option).
- Flatpak (P2).
- Tray support via StatusNotifierItem with a fallback for desktops without one.

### 12.4 Auto-update (on-device, user-confirmed)
- Check on launch and every 24 h (setting); channels: stable / beta.
- Fetch a small signed manifest over HTTPS from a fixed update URL (GitHub Releases or your own host).
- Download the installer/package (with Rift's own HTTP engine, so it resumes), **verify the ed25519 (minisign) signature and SHA-256**, then prompt "Restart to update." Never update silently without consent unless the user enables it.
- Portable builds only notify and open the download page (they do not self-overwrite).
- Linux package-manager installs defer to the package manager; AppImage supports delta updates (P2).

### 12.5 Release pipeline
- Tagged release in CI builds all artifacts on all OSes, runs the full test suite, signs everything, generates checksums (`SHA256SUMS`), publishes release notes, and uploads to the release page and update manifest.
- Smoke-test each artifact in a clean VM or container before publishing.

---

## 13. Repo structure

```
rift/
├─ PLAN.md
├─ CMakeLists.txt
├─ vcpkg.json                  pinned dependency manifest
├─ docs/                       architecture, plugin guide, user guide
├─ src/
│  ├─ app/                     main(), app bootstrap, single-instance IPC, CLI
│  ├─ engine/                  session wrapper, alerts, settings mapper, resume manager
│  ├─ http/                    rift-get: curl loop, segment scheduler, connection controller,
│  │                           disk writer, mirrors, metalink, checksum (no Qt in core)
│  ├─ bandwidth/               bandwidth arbiter shared by both engines
│  ├─ nativehost/              rift-native-host (browser Native Messaging bridge)
│  ├─ models/                  Qt item models (torrents, files, peers, trackers, filters)
│  ├─ storage/                 SQLite, migrations
│  ├─ automation/              watch folders, rules, RSS, hooks, extract
│  ├─ network/                 interface binding, kill switch, proxy, blocklist
│  ├─ streaming/               loopback range server, piece deadline logic
│  ├─ scheduler/               bandwidth schedule, queue helper
│  ├─ stats/
│  ├─ plugins/                 embedded scripting host, plugin manager
│  ├─ updater/
│  └─ platform/                win/, mac/, linux/ (tray, notifications, associations, autostart)
├─ ui/
│  ├─ qml/                     views, components, themes
│  ├─ assets/                  icons, fonts
│  └─ translations/
├─ browser/
│  ├─ chromium/                MV3 extension (Chrome, Edge, Brave)
│  └─ firefox/                 WebExtension
├─ packaging/
│  ├─ windows/                 Inno Setup/WiX scripts, manifest, rc files
│  ├─ macos/                   Info.plist, entitlements, dmg config
│  └─ linux/                   desktop file, AppImage recipe, deb/rpm specs
├─ tests/                      unit, integration (local swarm), UI, perf
└─ .github/workflows/
```

---

## 14. Testing strategy

- **Unit tests** (Qt Test or GoogleTest): settings mapper, rules engine, RSS filters (regex and episode filters), path sanitizer, magnet/URL parser, schedule logic, unit formatting.
- **Integration tests:** a **local private swarm** (tracker + seeder + leecher over loopback, using libtorrent in-process) to test download, resume, recheck, sequential, streaming, move storage, and limits deterministically and offline.
- **HTTP test server** (in-process, loopback, scriptable): per-connection throttling, no-range servers, chunked responses, lying `HEAD`, changing ETag mid-download, 429/503 with `Retry-After`, random connection drops, slow-loris stalls, redirects and HTTPS→HTTP downgrade, expired-link 403, FTP server fixture. Every segmentation and resume test ends with a full-file hash check.
- **Property/fuzz tests for the segment scheduler:** random split/finish/fail sequences must always cover every byte exactly once with no overlap.
- **UI tests:** QML tests (`QtQuickTest`) plus scripted end-to-end flows; screenshot comparison for light and dark themes.
- **Platform tests:** CI on Windows, macOS, Linux; installer smoke tests in clean VMs (install to default and custom path, upgrade, uninstall, portable run).
- **Performance tests:** scripted benchmarks for Section 10 on every release candidate.
- **Chaos tests:** kill the app mid-download, fill the disk, unplug the network interface, remove the download drive, and verify recovery.
- **Security tests:** loopback-only assertion for the streaming server, fuzzing of `.torrent`/magnet/RSS/plugin inputs, path-traversal tests.

---

## 15. Edge cases the agent must handle

- Invalid or corrupt `.torrent`; magnet with no peers; metadata timeout.
- Save path missing, read-only, on a disconnected drive, or full (pause with a clear message; auto-resume when fixed if enabled).
- Windows path length, illegal characters, case-collision on case-insensitive filesystems.
- Two torrents with overlapping files or save paths.
- Listen port in use (fall back and notify).
- No internet at startup (don't mark torrents errored; retry quietly).
- Sleep/resume and clock changes: re-announce and re-check interface state after wake.
- Huge torrents (100k+ files): lazy-load the file tree.
- Moving storage across drives while active.
- Network interface changes (Wi-Fi to Ethernet): rebind sockets.
- Corrupt resume data for one torrent must only force-recheck that torrent.
- Multiple monitors, high DPI, scaling changes at runtime.
- **Never lose user data:** removing a torrent without "delete data" must never delete files.
- Upgrades: settings and database migrate forward safely; keep a backup of the previous DB before migrating.
- HTTP: server size changes between probe and segments; `Content-Length` missing or wrong; server ignores `Range` and returns 200 (must not write that body at the segment's offset); file name with illegal characters or no extension; extremely large files (> 4 GiB on FAT32 targets: warn before start); target folder on a network drive; captive portal returning an HTML page instead of the file (detect content-type/size mismatch and stop).
- HTTP: same file being downloaded twice; user renames or moves the `.riftpart` file while paused; clock skew affecting `Last-Modified`.

---

## 16. Definition of Done (every feature)

- [ ] Works on Windows, macOS, and Linux
- [ ] UI has empty, loading, error, and success states
- [ ] Setting added to the Settings catalog if configurable
- [ ] Keyboard accessible and screen-reader labeled
- [ ] Unit/integration tests added and passing
- [ ] No regression in Section 10 performance targets
- [ ] Strings go through the translation system
- [ ] Short user-facing docs entry
- [ ] Does not introduce any new network listener or external runtime dependency

---

## 17. Naming and branding

- **Name:** Rift. Keep it in one constant (`APP_NAME`) plus the CMake project name for easy changes.
- Executable: `Rift.exe` (Windows), `Rift.app` (macOS), `rift` (Linux).
- App ID: `com.rift.app` (change to your real domain when you have one).
- Before public release, check trademark, domain, and GitHub availability for "Rift".

---

## 18. Next step

**Design system and UI mockups** (separate deliverable): color tokens, typography, spacing, component library (list rows, progress bars, piece map, **HTTP segment bar showing each connection's range**, speed readouts with limit context and the acceleration indicator (4.17), connection inspector, graphs, dialogs, command palette), and high-fidelity screens for every item in Section 5, including the HTTP add dialog and browser-extension popup, in dark and light. Also update `index-v2.html` / `download-v2.html` to describe Rift as torrents + accelerated downloads. Designs should assume a native Qt Quick app (no web-only patterns).
