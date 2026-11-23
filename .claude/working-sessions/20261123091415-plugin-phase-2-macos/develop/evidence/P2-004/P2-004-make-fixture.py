import math, struct, sys, wave
path = sys.argv[1]
rate = 48000
beat_seconds = 6
silence_seconds = 6
frames = []
for n in range(rate * (beat_seconds + silence_seconds)):
    t = n / rate
    if t < beat_seconds:
        bt = t % 0.5
        kick = math.exp(-bt * 12) * math.sin(2 * math.pi * (50 + 80 * math.exp(-bt * 30)) * bt)
        ht = (t + 0.25) % 0.5
        hat = math.exp(-ht * 40) * math.sin(2 * math.pi * 7000 * t) * 0.3
        mid = 0.2 * math.sin(2 * math.pi * 440 * t) * (0.5 + 0.5 * math.sin(2 * math.pi * 0.5 * t))
        v = 0.8 * kick + hat + mid
    else:
        v = 0.0
    v = max(-1.0, min(1.0, v))
    s = int(v * 32000)
    frames.append(struct.pack('<hh', s, s))
with wave.open(path, 'wb') as w:
    w.setnchannels(2)
    w.setsampwidth(2)
    w.setframerate(rate)
    w.writeframes(b''.join(frames))
print(path, rate, beat_seconds + silence_seconds)
