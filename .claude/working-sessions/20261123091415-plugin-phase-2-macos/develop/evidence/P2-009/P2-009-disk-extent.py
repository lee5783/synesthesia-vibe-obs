import sys
from alpha_probe import decode_png, pixel
for path in sys.argv[1:]:
    img = decode_png(path)
    w, h = img[0], img[1]
    row = [x for x in range(w) if pixel(img, x, h // 2)[0] > 128]
    col = [y for y in range(h) if pixel(img, w // 2, y)[0] > 128]
    dw = row[-1] - row[0] + 1 if row else 0
    dh = col[-1] - col[0] + 1 if col else 0
    print(f'{path.split("/")[-1]}: {w}x{h} disk {dw}x{dh} px (ratio {dw / dh if dh else 0:.3f}) centre ({(row[0] + row[-1]) / 2 if row else -1:.0f},{(col[0] + col[-1]) / 2 if col else -1:.0f})')
