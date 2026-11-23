import sys
from alpha_probe import decode_png, pixel
a, b = decode_png(sys.argv[1]), decode_png(sys.argv[2])
regions = {'image': (60, 700, 460, 1000), 'colour': (760, 450, 1000, 630), 'text': (60, 1100, 939, 1192), 'background': (0, 1200, 1080, 1920)}
for name, (x0, y0, x1, y1) in regions.items():
    diff = 0
    maxd = 0
    for y in range(y0, y1):
        for x in range(x0, x1):
            d = max(abs(i - j) for i, j in zip(pixel(a, x, y), pixel(b, x, y)))
            diff += d > 0
            maxd = max(maxd, d)
    print(f'{name} ({x0},{y0})-({x1},{y1}): {diff} of {(x1 - x0) * (y1 - y0)} pixels differ, max channel delta {maxd}')
