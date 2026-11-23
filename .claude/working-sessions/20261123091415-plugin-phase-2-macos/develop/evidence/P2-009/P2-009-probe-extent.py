import sys
from alpha_probe import decode_png, pixel
for path in sys.argv[1:]:
    img = decode_png(path)
    w, h = img[0], img[1]
    white_x = next(x for x in range(w) if pixel(img, x, h // 20)[:3] != (255, 255, 255))
    white_y = next(y for y in range(h) if pixel(img, w // 20, y)[:3] != (255, 255, 255))
    blue_x = next(x for x in range(w) if pixel(img, x, h // 2)[2] > 128)
    print(f'{path.split("/")[-1]}: {w}x{h} white square ends x={white_x} ({white_x / w:.3f}) y={white_y} ({white_y / h:.3f}); blue starts x={blue_x} ({blue_x / w:.3f})')
