import glob, os, sys
sys.path.insert(0, sys.argv[2])
from importlib import util
spec = util.spec_from_file_location('probe', os.path.join(sys.argv[2], 'P2-003-alpha-probe.py'))
probe = util.module_from_spec(spec); spec.loader.exec_module(probe)
d = sys.argv[1]
groups = {}
for f in sorted(glob.glob(os.path.join(d, '*.png'))):
    groups.setdefault(os.path.basename(f).rsplit('-', 1)[0], []).append(f)
for tag, files in groups.items():
    frames = [probe.decode_png(f) for f in files]
    diffs = []
    for a, b in zip(frames, frames[1:]):
        wa, ha, ca, pa = a[0], a[1], a[2], a[3]
        pb = b[3]
        total = 0; n = 0
        for ra, rb in zip(pa, pb):
            for i in range(0, len(ra), ca):
                total += abs(ra[i] - rb[i]) + abs(ra[i+1] - rb[i+1]) + abs(ra[i+2] - rb[i+2]); n += 3
        diffs.append(total / n)
    print(f'{tag:14s} mean abs diff between consecutive frames (0-255): {sum(diffs)/len(diffs):6.2f}  per pair: ' + ' '.join(f'{x:.1f}' for x in diffs))
