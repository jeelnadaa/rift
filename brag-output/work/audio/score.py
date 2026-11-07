"""Rift launch video soundtrack: music + effects + narration, one mix.
120 BPM (beat 0.5 s), D minor, 29 s. Every event sits on the beat grid counted from frame 0.
    python brag-output/work/audio/score.py   ->  audio/mix.wav (48 kHz stereo)
"""
import json, os, subprocess
import numpy as np
from scipy import signal

HERE = os.path.dirname(os.path.abspath(__file__))
VOICE_DIR = os.path.join(HERE, "..", "voice")
SR = 48000
DUR = 29.0
N = int(SR * DUR)
BEAT = 0.5
rng = np.random.default_rng(7)

# Narration line start times (seconds). Chosen so key words land on beats: "Rift" 5.5, "One Rift" 27.0.
LINE_STARTS = [0.25, 5.06, 6.5, 9.0, 13.5, 16.0, 20.0, 24.5]
DROP = 5.5          # impact + full groove
BREAK = 24.5        # outro breakdown
FINAL = 27.0        # last hit
CUTS = [6.5, 9.0, 13.5, 16.0, 20.0, 24.5]


def t_axis(n):
    return np.arange(n) / SR


def midi(m):
    return 440.0 * 2 ** ((m - 69) / 12)


def env_ad(n, a, d):
    t = t_axis(n)
    e = np.where(t < a, t / max(a, 1e-6), np.exp(-(t - a) / max(d, 1e-6)))
    return e


def lp(x, fc, order=2):
    sos = signal.butter(order, min(fc, SR * 0.45), "low", fs=SR, output="sos")
    return signal.sosfilt(sos, x)


def hp(x, fc, order=2):
    sos = signal.butter(order, fc, "high", fs=SR, output="sos")
    return signal.sosfilt(sos, x)


def bp(x, lo, hi, order=2):
    sos = signal.butter(order, [lo, min(hi, SR * 0.45)], "band", fs=SR, output="sos")
    return signal.sosfilt(sos, x)


def saw(f, n, phase=0.0):
    t = t_axis(n)
    return 2 * ((f * t + phase) % 1.0) - 1


def place(buf, x, at, gain=1.0, pan=0.0):
    i = int(round(at * SR))
    if i >= buf.shape[1]:
        return
    j = min(buf.shape[1], i + len(x))
    l, r = np.cos((pan + 1) * np.pi / 4), np.sin((pan + 1) * np.pi / 4)
    buf[0, i:j] += x[: j - i] * gain * l * 1.414
    buf[1, i:j] += x[: j - i] * gain * r * 1.414


def reverb_ir(seconds=2.4, decay=2.2, bright=6000):
    n = int(SR * seconds)
    t = t_axis(n)
    ir = np.stack([rng.standard_normal(n), rng.standard_normal(n)]) * np.exp(-t * decay)
    ir = np.stack([lp(ir[0], bright), lp(ir[1], bright)])
    ir[:, : int(0.012 * SR)] *= np.linspace(0, 1, int(0.012 * SR))
    return ir / np.sqrt((ir ** 2).sum(axis=1, keepdims=True))


def apply_reverb(x, ir, wet):
    out = np.stack([signal.fftconvolve(x[0], ir[0])[: x.shape[1]], signal.fftconvolve(x[1], ir[1])[: x.shape[1]]])
    return x + out * wet


# ---------------------------------------------------------------- instruments
def kick(level=1.0):
    n = int(0.45 * SR)
    t = t_axis(n)
    f = 46 + 110 * np.exp(-t * 32)
    ph = 2 * np.pi * np.cumsum(f) / SR
    body = np.sin(ph) * np.exp(-t * 7.5)
    click = hp(rng.standard_normal(n), 2500) * np.exp(-t * 300) * 0.25
    return np.tanh((body + click) * 1.4) * level


def hat(open_=False):
    n = int((0.18 if open_ else 0.05) * SR)
    t = t_axis(n)
    x = hp(rng.standard_normal(n), 7000, 4) * np.exp(-t * (18 if open_ else 70))
    return x * 0.5


def clap():
    n = int(0.5 * SR)
    t = t_axis(n)
    x = np.zeros(n)
    for k, off in enumerate([0, 0.011, 0.023]):
        i = int(off * SR)
        x[i:] += rng.standard_normal(n - i) * np.exp(-t[: n - i] * (60 if k < 2 else 14))
    return bp(x, 900, 5200) * 0.55


def pluck(m, dur=0.22, bright=1.0):
    n = int(dur * 1.8 * SR)
    t = t_axis(n)
    f = midi(m)
    x = 0.6 * saw(f, n) + 0.4 * saw(f * 1.005, n, 0.3)
    x = lp(x, 900 + 3800 * bright) * np.exp(-t * (9 / dur * 0.25)) * env_ad(n, 0.003, dur)
    return x * 0.32


def pad_chord(notes, dur, cutoff=1400, level=0.13):
    n = int(dur * SR)
    t = t_axis(n)
    x = np.zeros(n)
    for m in notes:
        for det in (-0.08, 0.0, 0.07):
            x += saw(midi(m + det), n, rng.random())
    x = lp(x / (len(notes) * 3), cutoff, 2)
    a = np.minimum(1, t / 0.35)
    r = np.minimum(1, (dur - t) / 0.4).clip(0, 1)
    return x * a * r * level


def bass_note(m, dur, level=0.42):
    n = int(dur * SR)
    t = t_axis(n)
    f = midi(m)
    x = 0.7 * np.sin(2 * np.pi * f * t) + 0.3 * lp(saw(f, n), 380)
    e = np.minimum(1, t / 0.005) * np.exp(-t * 3.2) * np.minimum(1, (dur - t) / 0.02).clip(0, 1)
    return np.tanh(x * 1.6) * e * level


def impact():
    n = int(2.6 * SR)
    t = t_axis(n)
    f = 30 + 70 * np.exp(-t * 6)
    boom = np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t * 1.6)
    noise = lp(rng.standard_normal(n), 2600) * np.exp(-t * 5.5) * 0.5
    return np.tanh((boom + noise) * 1.5) * 0.9


def riser(seconds, top=7000):
    n = int(seconds * SR)
    t = t_axis(n)
    p = t / seconds
    noise = rng.standard_normal(n)
    out = np.zeros(n)
    hop = 1024
    for i in range(0, n, hop):  # sweep a band upward block by block
        c = 300 + (top - 300) * p[i] ** 2
        seg = bp(noise[i:i + hop * 2], c * 0.7, c * 1.3)[:hop]
        out[i:i + len(seg)] += seg
    tone = np.sin(2 * np.pi * np.cumsum(midi(50) * (1 + p * 1.0)) / SR) * 0.15
    return (out * 0.9 + tone) * p ** 2.2 * 0.5


def whoosh(seconds=0.5, rev=False):
    n = int(seconds * SR)
    t = t_axis(n)
    p = t / seconds
    noise = rng.standard_normal(n)
    out = np.zeros(n)
    hop = 512
    for i in range(0, n, hop):
        c = 600 + 5000 * (p[i] if not rev else 1 - p[i])
        seg = bp(noise[i:i + hop * 2], c * 0.6, c * 1.4)[:hop]
        out[i:i + len(seg)] += seg
    shape = np.sin(np.pi * p) ** 2
    return out * shape * 0.22


def tick(m=86):
    n = int(0.06 * SR)
    t = t_axis(n)
    return np.sin(2 * np.pi * midi(m) * t) * np.exp(-t * 90) * 0.18


def stab(notes, level=0.3):
    n = int(0.9 * SR)
    t = t_axis(n)
    x = sum(saw(midi(m), n) for m in notes) / len(notes)
    return lp(x, 2200) * np.exp(-t * 5) * env_ad(n, 0.002, 0.5) * level


# ---------------------------------------------------------------- arrangement
music = np.zeros((2, N))
drums = np.zeros((2, N))
fx = np.zeros((2, N))

D2, F2, A2, Bb1, C2 = 38, 41, 45, 34, 36
CHORDS = {  # by bar (2 s): root for bass, notes for pad/arp (D minor: i  VI  III  VII)
    "Dm": (38, [62, 65, 69, 74]), "Bb": (34, [58, 62, 65, 70]), "F": (41, [60, 65, 69, 72]), "C": (36, [60, 64, 67, 72]),
}
PROG = ["Dm", "Bb", "F", "C"]


def chord_at(t):
    if t < 6.0:
        return "Dm"
    return PROG[int((t - 6.0) // 2.0) % 4]


# Hook (0–5.5): heartbeat, drone, ticking, stabs on each phrase, glitch on "lie", riser into the drop.
drone_n = int(5.6 * SR)
dt = t_axis(drone_n)
drone = (np.sin(2 * np.pi * midi(26) * dt) * 0.5 + lp(saw(midi(38), drone_n), 260) * 0.25) * np.minimum(1, dt / 1.2)
place(music, drone * 0.55, 0.0)
for b in np.arange(0, 5.5, 1.0):
    place(drums, kick(0.55), b)
for s in np.arange(0.0, 5.5, 0.125):
    place(drums, hat() * (0.35 if (s * 8) % 2 else 0.6), s, pan=0.3 if (s * 8) % 2 else -0.2)
place(music, stab([50, 57, 62], 0.33), 0.5, pan=-0.5)   # "Torrents"
place(music, stab([53, 60, 65], 0.33), 2.0, pan=0.5)    # "Direct links"
for k, s in enumerate(np.arange(3.5, 4.5, 0.0625)):      # lying counter: fast ticks
    place(fx, tick(86 + (k * 5) % 12), s, 0.9, pan=((k % 3) - 1) * 0.4)
place(music, stab([50, 56, 62], 0.38), 4.0)             # "lie" (flat 5: a sour stab)
place(fx, riser(1.5), DROP - 1.5, 0.9)

# Drop (5.5): impact, then groove 6.0–24.5.
place(fx, impact(), DROP, 1.0)
place(music, pad_chord([50, 57, 62, 65, 69], 1.6, 2600, 0.2), DROP)
for b in np.arange(6.0, BREAK, BEAT):
    place(drums, kick(0.95), b)
for b in np.arange(7.0, BREAK, 1.0):  # clap on 2 and 4
    if b >= 9.0:
        place(drums, clap(), b + 0.0, 0.8)
for s in np.arange(6.0, BREAK, 0.25):
    place(drums, hat(open_=(s * 4) % 2 == 1) * (0.55 if (s * 4) % 2 == 1 else 0.4), s, pan=0.25)
for bar in np.arange(6.0, BREAK, 2.0):
    name = chord_at(bar + 0.01)
    root, notes = CHORDS[name]
    place(music, pad_chord(notes, min(2.0, BREAK - bar), 1500, 0.12), bar)
    for s in np.arange(bar, min(bar + 2.0, BREAK), 0.5):  # off-beat bass
        place(music, bass_note(root, 0.24), s + 0.25)
    arp = [notes[0], notes[1], notes[2], notes[3], notes[2] + 12, notes[3], notes[1] + 12, notes[2]]
    for k, s in enumerate(np.arange(bar, min(bar + 2.0, BREAK), 0.125)):
        if bar < 9.0 and k % 2:
            continue  # sparser before the lanes section
        place(music, pluck(arp[k % 8], 0.2, 0.4 + 0.6 * min(1, (s - 6) / 10)), s, 0.8, pan=-0.35 if k % 2 else 0.35)

# Lanes section: eight rising blips, one per lane, on 16ths.
for k in range(8):
    place(fx, tick(74 + [0, 3, 5, 7, 10, 12, 15, 17][k]), 9.75 + k * 0.125, 0.8, pan=-0.7 + k * 0.2)

# Scene transitions: soft whooshes ending on each cut.
for c in CUTS:
    place(fx, whoosh(0.5), c - 0.42, 0.9, pan=0.0)

# Browser click (16.0 + right-click and menu pick).
place(fx, tick(93) * 1.2, 17.0)
place(fx, tick(98) * 1.2, 18.0)

# Breakdown (24.5–27): pad holds Bb → C, arp filtered, riser; final hit on 27.0; tail to 29.
place(music, pad_chord(CHORDS["Bb"][1], 1.5, 1100, 0.16), BREAK)
place(music, pad_chord(CHORDS["C"][1], 1.0, 1500, 0.17), 26.0)
for k, s in enumerate(np.arange(BREAK, FINAL, 0.25)):
    place(music, pluck([62, 65, 70, 74][k % 4], 0.25, 0.25), s, 0.6, pan=-0.4 if k % 2 else 0.4)
place(fx, riser(1.5, 5000), FINAL - 1.5, 0.7)
place(fx, impact() * 0.8, FINAL)
place(drums, kick(1.0), FINAL)
place(music, pad_chord([50, 57, 62, 64, 69, 74], 2.0, 3000, 0.22), FINAL)
place(music, bass_note(38, 1.6, 0.5), FINAL)

# ---------------------------------------------------------------- sidechain + reverb
kick_times = list(np.arange(6.0, BREAK, BEAT)) + [FINAL]
pump = np.ones(N)
tt = t_axis(int(0.4 * SR))
curve = 1 - 0.55 * np.exp(-tt * 9)
for k in kick_times:
    i = int(k * SR)
    j = min(N, i + len(curve))
    pump[i:j] = np.minimum(pump[i:j], curve[: j - i])
music *= pump

ir = reverb_ir()
music = apply_reverb(music, ir, 0.28)
fx = apply_reverb(fx, ir, 0.35)
drums = drums + apply_reverb(drums * 0.0 + drums * (1.0), ir, 0.08) * 0.0  # drums dry, tight

# ---------------------------------------------------------------- narration
voice = np.zeros(N)
meta = json.load(open(os.path.join(VOICE_DIR, "lines.json")))
for i, start in enumerate(LINE_STARTS):
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", os.path.join(VOICE_DIR, f"line{i}.mp3"), "-f", "f32le", "-ac", "1",
                          "-ar", str(SR), "-"], capture_output=True, check=True).stdout
    v = np.frombuffer(raw, dtype=np.float32).astype(np.float64)
    a = int(start * SR)
    b = min(N, a + len(v))
    voice[a:b] += v[: b - a]
# Voice chain: gentle high-pass, presence lift, light compression, a touch of room.
voice = hp(voice, 85)
voice = voice + 0.18 * bp(voice, 2500, 6000)
env = np.abs(signal.hilbert(voice))
env = lp(env, 12, 1)
thr = 0.12
gain = np.where(env > thr, (thr + (env - thr) / 2.5) / np.maximum(env, 1e-9), 1.0)
voice = voice * gain
voice = voice / (np.max(np.abs(voice)) + 1e-9) * 0.9
voice_st = np.stack([voice, voice])
voice_st = apply_reverb(voice_st, reverb_ir(0.9, 6.5, 5000), 0.06)

# Duck music + fx under the voice (-8 dB), smooth attack/release.
venv = lp(np.abs(voice), 6, 1)
venv = np.clip(venv / 0.08, 0, 1)
duck = 1 - 0.68 * venv
bed = (music * 0.9 + drums * 0.75 + fx * 0.8) * duck

mix = bed * 0.42 + voice_st * 0.95
# Fade the tail.
fade = np.ones(N)
fs = int((DUR - 1.2) * SR)
fade[fs:] = np.linspace(1, 0, N - fs) ** 1.5
mix *= fade
mix = np.tanh(mix * 1.1) / np.tanh(1.1)
mix /= np.max(np.abs(mix)) + 1e-9
mix *= 0.89

out = os.path.join(HERE, "mix.wav")
import soundfile as sf
sf.write(out, mix.T.astype(np.float32), SR, subtype="PCM_24")
sf.write(os.path.join(HERE, "music_only.wav"), (bed / (np.max(np.abs(bed)) + 1e-9) * 0.89).T.astype(np.float32), SR, subtype="PCM_24")
print("wrote", out)

# Balance report: voice vs bed during speech, and the bed alone between lines.
sp = np.abs(voice) > 0.02
sp = lp(sp.astype(float), 4, 1) > 0.3
def db(x): return 20 * np.log10(np.sqrt(np.mean(x ** 2)) + 1e-12)
print("voice in speech %.1f dB | bed in speech %.1f dB | bed between lines %.1f dB"
      % (db(voice_st[0][sp] * 0.95), db(bed[0][sp] * 0.42), db(bed[0][~sp] * 0.42)))
