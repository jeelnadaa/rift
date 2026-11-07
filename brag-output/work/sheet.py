"""Contact sheet of stills: python sheet.py out.png t1 t2 ..."""
import sys
from PIL import Image, ImageDraw
ts = sys.argv[2:]
W, H = 640, 360
cols = 3
rows = (len(ts) + cols - 1) // cols
s = Image.new("RGB", (W * cols, H * rows), (40, 40, 40))
d = ImageDraw.Draw(s)
for i, t in enumerate(ts):
    im = Image.open("stills/t_%05.2f.png" % float(t)).convert("RGB").resize((W, H))
    s.paste(im, ((i % cols) * W, (i // cols) * H))
    d.text(((i % cols) * W + 8, (i // cols) * H + 6), t, fill=(255, 255, 0))
s.save(sys.argv[1])
