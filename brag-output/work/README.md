# How the launch video is made

Everything here is source. Run it again to rebuild `../brag.mp4` exactly.

| Step | Command (from the repo root) | Output |
|---|---|---|
| 1. Real UI footage | build `rift_ui_film` (`bash scripts/ui-harness.sh` builds `build-ui/`), then for each take: `QT_QPA_PLATFORM=offscreen QT_SCALE_FACTOR=2 build-ui/rift_ui_film <take> brag-output/work/frames/<take> <seconds>` with `list 4`, `lanes 5`, `add 4.5`, `stats 4`, `light 3` | `frames/<take>/NNNN.png` (2880×1620, deterministic) |
| 2. Narration | `python brag-output/work/voice/tts.py` (Microsoft neural voice via `edge-tts`) | `voice/lineN.mp3`, `voice/lines.json` (word timings) |
| 3. Music, effects, mix | `python brag-output/work/audio/score.py` | `audio/mix.wav` |
| 4. Picture + mux | `python brag-output/work/render.py final` (Playwright + Chrome + ffmpeg) | `../brag.mp4`, `../brag.jpg` |

Check single frames with `python brag-output/work/render.py stills 5.6 10.9` (→ `stills/`).

- `tests/ui/film.cpp` records the real QML app with Qt's animation clock stepped at 30 fps, so every
  spring, stagger and fade in the footage is the app's own. The data is fictional and public (open movies, Linux ISOs).
- `compositor/index.html` draws every frame as a pure function of time over that footage: kinetic type,
  captions synced to the voice's word timings, and the transitions.
- `frames/` and `stills/` are regenerable and large (~240 MB), so git ignores them (`work/.gitignore`).
  The voice files are committed, because they come from an online service.
