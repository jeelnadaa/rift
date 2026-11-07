"""Narration: one file per line with word timings (edge-tts, Microsoft neural voice).
python brag-output/work/voice/tts.py  ->  voice/lineN.mp3 + voice/lines.json"""
import asyncio, json, os, sys
import edge_tts

HERE = os.path.dirname(os.path.abspath(__file__))
VOICE = sys.argv[1] if len(sys.argv) > 1 else "en-US-AndrewMultilingualNeural"
LINES = [
    "Torrents in one app. Direct links in another. And speeds that lie.",
    "This is Rift.",
    "One fast, native app for both.",
    "Every file splits into eight lanes, then snaps back together.",
    "The speed you see is the speed you get.",
    "Right-click any link, and your browser hands it straight over.",
    "Free on Windows, Mac and Linux. Everything stays on your device.",
    "Torrents. Direct links. One Rift.",
]

async def one(i, text, tries=4):
    for attempt in range(tries):  # the service occasionally returns no audio; retry
        try:
            c = edge_tts.Communicate(text, VOICE, rate="-6%", pitch="-2Hz", boundary="WordBoundary")
            words, audio = [], bytearray()
            async for ch in c.stream():
                if ch["type"] == "audio": audio += ch["data"]
                elif ch["type"] == "WordBoundary":
                    words.append({"w": ch["text"], "t": ch["offset"] / 1e7, "d": ch["duration"] / 1e7})
            break
        except edge_tts.exceptions.NoAudioReceived:
            if attempt == tries - 1: raise
            await asyncio.sleep(1.5)
    open(os.path.join(HERE, f"line{i}.mp3"), "wb").write(audio)
    return {"i": i, "text": text, "words": words}

async def main():
    out = [await one(i, t) for i, t in enumerate(LINES)]
    json.dump({"voice": VOICE, "lines": out}, open(os.path.join(HERE, "lines.json"), "w"), indent=1)

asyncio.run(main())
