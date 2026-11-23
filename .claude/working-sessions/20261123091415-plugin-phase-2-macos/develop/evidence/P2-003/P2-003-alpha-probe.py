import struct
import sys
import zlib


def decode_png(path):
    data = open(path, 'rb').read()
    assert data[:8] == b'\x89PNG\r\n\x1a\n', 'not a PNG'
    pos = 8
    idat = b''
    while pos < len(data):
        length, kind = struct.unpack('>I4s', data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + length]
        if kind == b'IHDR':
            width, height, depth, color_type = struct.unpack('>IIBB', body[:10])
        elif kind == b'IDAT':
            idat += body
        pos += 12 + length
    assert depth == 8 and color_type in (2, 6), f'unsupported depth {depth} colour type {color_type}'
    channels = 4 if color_type == 6 else 3
    raw = zlib.decompress(idat)
    stride = width * channels
    rows = []
    previous = bytearray(stride)
    pos = 0
    for _ in range(height):
        kind = raw[pos]
        line = bytearray(raw[pos + 1:pos + 1 + stride])
        pos += 1 + stride
        for i in range(stride):
            left = line[i - channels] if i >= channels else 0
            up = previous[i]
            corner = previous[i - channels] if i >= channels else 0
            if kind == 1:
                line[i] = (line[i] + left) & 255
            elif kind == 2:
                line[i] = (line[i] + up) & 255
            elif kind == 3:
                line[i] = (line[i] + (left + up) // 2) & 255
            elif kind == 4:
                p = left + up - corner
                pa, pb, pc = abs(p - left), abs(p - up), abs(p - corner)
                predictor = left if pa <= pb and pa <= pc else (up if pb <= pc else corner)
                line[i] = (line[i] + predictor) & 255
        rows.append(line)
        previous = line
    return width, height, channels, rows


def pixel(image, x, y):
    width, height, channels, rows = image
    line = rows[y]
    values = tuple(line[x * channels:(x + 1) * channels])
    return values if channels == 4 else values + (255,)


def report_alpha(path, region):
    image = decode_png(path)
    width, height, channels, rows = image
    x0, y0, x1, y1 = region if region else (0, 0, width, height)
    alphas = [pixel(image, x, y)[3] for y in range(y0, y1) for x in range(x0, x1)]
    below = sum(1 for a in alphas if a < 255)
    print(f'{path}: {width}x{height} channels={channels} region=({x0},{y0})-({x1},{y1}) '
          f'min_alpha={min(alphas)} max_alpha={max(alphas)} pixels_below_255={below}/{len(alphas)}')


def report_probe(path):
    image = decode_png(path)
    width, height = image[0], image[1]
    for fraction in (0.1, 0.5, 0.9):
        y = int(height * fraction)
        print(f'{path}: row {fraction:.0%} ramp(x=37%) rgba={pixel(image, int(width * 0.37), y)} '
              f'right(x=75%) rgba={pixel(image, int(width * 0.75), y)}')
    print(f'{path}: top-left(x=10%,y=10%) rgba={pixel(image, int(width * 0.1), int(height * 0.1))}')


def report_magenta(path, region):
    image = decode_png(path)
    x0, y0, x1, y1 = region
    hits = sum(1 for y in range(y0, y1) for x in range(x0, x1) if pixel(image, x, y)[:3] == (255, 0, 255))
    print(f'{path}: region=({x0},{y0})-({x1},{y1}) pure_magenta_pixels={hits}/{(x1 - x0) * (y1 - y0)}')


if __name__ == '__main__':
    mode, path = sys.argv[1], sys.argv[2]
    region = tuple(int(v) for v in sys.argv[3].split(',')) if len(sys.argv) > 3 else None
    if mode == 'alpha':
        report_alpha(path, region)
    elif mode == 'probe':
        report_probe(path)
    elif mode == 'magenta':
        report_magenta(path, region)
