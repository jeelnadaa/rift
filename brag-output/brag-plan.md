# Rift launch video: plan

**What it is:** Rift is a free, native download manager for Windows, macOS and Linux. One app runs a tuned
BitTorrent engine *and* an accelerated multi-connection HTTP downloader.
**Who it's for:** anyone who juggles a torrent client and a separate download manager, or who doesn't trust
the speed numbers they're shown.
**What sets it apart:** both engines in one calm native window; every HTTP file split into eight lanes;
honest, measured speeds; a browser extension that hands links over with no network port; nothing leaves the device.
**Most impressive claim:** "Eight lanes. One file." (site copy), shown live in the real segment bar.
**Visual hook:** two apps torn apart by a rift, then a speed counter that won't stop lying, then a slash
(Rift's mark) cuts through it all.
**Real UI shown:** the actual QML app, recorded frame by frame by `tests/ui/film.cpp` with Qt's animation
clock stepped at 30 fps, at 2× scale. Fictional, public data only: open movies (Big Buck Bunny, Sintel,
Tears of Steel) and Linux ISOs, saving to `D:/Downloads`. No personal paths or names.
**Tone:** cinematic and polished, Obsidian theme, cobalt accent. Fast, confident, not shouty.
**Share caption:** see `share-copy.txt`.

## Identity
- Colors: Obsidian `#050608` window, `#ECEEF3` text, cobalt `#4C7DFF`, signal `#C38BEA`, success `#4CC89A`, info `#5EC4C4`.
- Type: Bricolage Grotesque (display axes wdth 75–85, opsz 96), Instrument Serif italic for emphasis,
  JetBrains Mono for numbers and labels. The app's own bundled fonts.
- Motion: the app's spring curve `cubic-bezier(.2,1.3,.3,1)` and standard `(.2,.8,.2,1)`; the Rift
  corner (14 4 14 4) on every card.

## Sound
120 BPM (beat 0.5 s, bar 2 s), D minor, synthesized in Python as one piece: tense filtered pulse under the hook,
an impact and full drop on "Rift" (5.5 s), four-on-the-floor with sub bass, a pluck arpeggio and a pad, a
breakdown for the outro and one final hit on "One Rift". Effects (risers, whooshes, ticks) are pitched to D and
sit under the music. The music ducks under the narration. Narration: Microsoft neural voice
*Andrew (Multilingual)*, slowed 6 %, one file per line, each line starting on the beat grid.

## Storyboard (29 s, 30 fps; every cut on a beat)

| # | Time | Narration | Picture |
|---|---|---|---|
| 1 Hook | 0.0–5.0 | "Torrents in one app. Direct links in another. And speeds that lie." | Black. "Torrents" slams in left inside a ghost window, "Direct links" right in another; a crack opens between them. A giant mono speed counter jitters through lies while *lie* stamps across it. Everything collapses into a slash. |
| 2 Reveal | 5.0–6.5 | "This is Rift." | The slash (Rift's mark) tears the frame on the downbeat; the wordmark slams in at display width with the logo; light streaks. |
| 3 Both | 6.5–9.0 | "One fast, native app for both." | The real window rises in 3D from the empty state as torrent and HTTP rows stagger in. TORRENT / HTTP chips fly out on *both*. |
| 4 Lanes | 9.0–13.5 | "Every file splits into eight lanes, then snaps back together." | Camera dives into the real segment bar: 8 colored lanes fill live; beams race out and converge into one file. Real copy: "8 connections · 6.4× faster than one connection". |
| 5 Honest | 13.5–16.0 | "The speed you see is the speed you get." | Real Stats page; the 118 MB/s tile zooms to full frame, mono digits settle; kicker from the site: "The number is the number." |
| 6 Browser | 16.0–20.0 | "Right-click any link, and your browser hands it straight over." | A plain browser page, cursor right-clicks a link → "Download with Rift" → cut to the real Add dialog springing up, Ctrl+Enter, "Added" toast. |
| 7 Everywhere | 20.0–24.5 | "Free on Windows, Mac and Linux. Everything stays on your device." | Windows / macOS / Linux slam in sync; the real UI wipes Obsidian → Stone → Obsidian along the slash; a lock line and "Every detail, on your device." |
| 8 Outro | 24.5–29.0 | "Torrents. Direct links. One Rift." | The hook's words return and fuse into the logo; `rift.solarquack.in` and "Free · Windows · macOS · Linux". |

Subtitles: custom kinetic captions (Bricolage 600, word-by-word rise synced to the voice's word timings, key
words in Instrument Serif italic + cobalt). Hidden in the hook and outro where the giant type *is* the caption.
