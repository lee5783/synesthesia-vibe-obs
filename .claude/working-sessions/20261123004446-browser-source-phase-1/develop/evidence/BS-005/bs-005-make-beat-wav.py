import math
import random
import struct
import sys
import wave

path = sys.argv[1]
rate = 48000
seconds = 30
random.seed(5)
frames = bytearray()
for n in range(rate * seconds):
    t = n / rate
    beat = t % 0.5
    kick = math.sin(2 * math.pi * (50 + 120 * math.exp(-beat * 30)) * beat) * math.exp(-beat * 8)
    sweep = 0.25 * math.sin(2 * math.pi * (100 + 1900 * ((t % 4) / 4)) * t)
    hat_t = (t + 0.25) % 0.5
    hat = random.uniform(-1, 1) * 0.3 * math.exp(-hat_t * 60)
    value = max(-1.0, min(1.0, 0.7 * kick + sweep + hat))
    sample = int(value * 30000)
    frames += struct.pack("<hh", sample, sample)
with wave.open(path, "wb") as out:
    out.setnchannels(2)
    out.setsampwidth(2)
    out.setframerate(rate)
    out.writeframes(bytes(frames))
