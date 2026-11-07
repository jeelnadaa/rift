"""Render the Rift launch film.
  python brag-output/work/render.py stills 0.5 2.2 5.6 ...   -> work/stills/t_XX.XX.png
  python brag-output/work/render.py video                    -> work/video_noaudio.mp4
  python brag-output/work/render.py final                    -> brag-output/brag.mp4 (+ brag.jpg poster as frame 0)
Every frame is a pure function of time: render(t) sets the page, waits for its images, then we screenshot.
"""
import json, os, subprocess, sys
import numpy as np
from PIL import Image
from playwright.sync_api import sync_playwright

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.dirname(HERE)
FPS, DUR = 30, 29.0
POSTER_T = 28.3


def ensure_grain():
    p = os.path.join(HERE, "compositor", "grain.png")
    if not os.path.exists(p):
        rng = np.random.default_rng(3)
        g = (rng.normal(128, 46, (512, 512))).clip(0, 255).astype(np.uint8)
        Image.fromarray(g, "L").save(p)


def frames_count():
    d = os.path.join(HERE, "frames")
    return {k: len(os.listdir(os.path.join(d, k))) for k in os.listdir(d) if os.path.isdir(os.path.join(d, k))}


def open_page(pw):
    browser = pw.chromium.launch(channel="chrome", args=["--allow-file-access-from-files", "--disable-web-security",
                                                         "--force-color-profile=srgb", "--hide-scrollbars"])
    page = browser.new_page(viewport={"width": 1920, "height": 1080}, device_scale_factor=1)
    page.goto("file:///" + os.path.join(HERE, "compositor", "index.html").replace("\\", "/"))
    fonts = page.evaluate("ready()")
    bad = [f for f in fonts if not f.endswith("loaded")]
    if bad:
        raise SystemExit("fonts not loaded: %s" % bad)
    lines = json.load(open(os.path.join(HERE, "voice", "lines.json")))["lines"]
    page.evaluate("([l, f]) => setup(l, f)", [lines, frames_count()])
    return browser, page


def shot(page, t):
    page.evaluate("t => render(t)", t)
    return page.screenshot(type="png", full_page=False)


def main():
    ensure_grain()
    mode = sys.argv[1] if len(sys.argv) > 1 else "stills"
    with sync_playwright() as pw:
        browser, page = open_page(pw)
        if mode == "stills":
            os.makedirs(os.path.join(HERE, "stills"), exist_ok=True)
            for a in sys.argv[2:]:
                t = float(a)
                open(os.path.join(HERE, "stills", "t_%05.2f.png" % t), "wb").write(shot(page, t))
            print("stills done")
        else:
            n = int(round(DUR * FPS))
            dst = os.path.join(HERE, "video_noaudio.mp4")
            ff = subprocess.Popen(["ffmpeg", "-y", "-v", "error", "-f", "image2pipe", "-framerate", str(FPS), "-i", "-",
                                   "-c:v", "libx264", "-preset", "slow", "-crf", "14", "-pix_fmt", "yuv420p",
                                   "-x264-params", "keyint=60", dst], stdin=subprocess.PIPE)
            for i in range(n):
                t = i / FPS
                if i == 0:
                    t = POSTER_T  # poster baked in as frame 0
                ff.stdin.write(shot(page, t))
                if i % 60 == 0:
                    print("frame", i, "/", n, flush=True)
            ff.stdin.close()
            ff.wait()
            open(os.path.join(OUT, "brag.jpg"), "wb").write(page.screenshot(type="jpeg", quality=92) if page.evaluate("t => render(t)", POSTER_T) else b"")
            print("video done")
        browser.close()
    if mode in ("video", "final"):
        mix = os.path.join(HERE, "audio", "mix.wav")
        subprocess.run(["ffmpeg", "-y", "-v", "error", "-i", os.path.join(HERE, "video_noaudio.mp4"), "-i", mix,
                        "-c:v", "copy", "-c:a", "aac", "-b:a", "256k", "-ar", "48000", "-shortest", "-movflags", "+faststart",
                        os.path.join(OUT, "brag.mp4")], check=True)
        print("wrote", os.path.join(OUT, "brag.mp4"))


if __name__ == "__main__":
    main()
