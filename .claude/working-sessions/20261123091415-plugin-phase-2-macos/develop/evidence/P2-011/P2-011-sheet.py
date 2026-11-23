import sys
from PIL import Image, ImageDraw
out, names = sys.argv[1], sys.argv[2:]
tiles = []
for n in names:
    im = Image.open(f"shots/{n}.png").convert("RGBA")
    bg = Image.new("RGBA", im.size, (90, 90, 90, 255)); bg.alpha_composite(im)
    bg.thumbnail((270, 480)); tiles.append((n, bg.convert("RGB")))
W = sum(t.width for _, t in tiles) + 10 * (len(tiles) + 1); H = max(t.height for _, t in tiles) + 40
sheet = Image.new("RGB", (W, H), (30, 30, 30)); d = ImageDraw.Draw(sheet); x = 10
for n, t in tiles:
    sheet.paste(t, (x, 30)); d.text((x, 8), n, fill=(255, 255, 255)); x += t.width + 10
sheet.save(out)
