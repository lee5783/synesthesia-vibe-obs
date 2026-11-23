# P2-009 evidence: resolution and render scale

OBS 33.0.3 (OpenGL 4.1 Metal, Apple M1, 16 GB), canvas 1080×1920 at 30 fps (unchanged; output
720×1280). Collection `synesthesia-spike`, program scene `Scene`. Everything was driven through
obs-websocket (`CreateInput`, `SetInputSettings`, `GetSceneItemTransform`, `GetSourceScreenshot`,
`GetStats`, `SetSceneItemEnabled`, `RemoveInput`). There was no GUI automation, and no OBS
config, profile or canvas was changed. The test input `P2-009 Viz` (swarm, auto-cycle off) was
added to `Scene` next to the collection's own visualizer, image, colour, text and browser sources.
Paths are scrubbed to `<scratch>`.

## Defaults

`GetInputDefaultSettings`:
`{"audio_mix_track":1,"blend_seconds":2.7,"cycle_seconds":16,"height":1080,"render_scale":100,"width":1920}`.
A new source reports `sourceWidth 1920, sourceHeight 1080`, and the create line reads
`sv_init_gl and sv_create succeeded (1920x1080)`.

## Size changes (`P2-009-sizes-run.txt`)

| `SetInputSettings` | `GetSceneItemTransform` source size |
|---|---|
| 1080×1920 | 1080×1920 |
| 1280×720 | 1280×720 |
| 128×128 (lower limit) | 128×128 |
| 8192×128 (upper limit, width) | 8192×128 |
| 128×8192 (upper limit, height) | 128×8192 |
| 9000×50 (out of range) | 8192×128 (clamped) |
| 50×9000 (out of range) | 128×8192 (clamped) |
| 1920×1080 | 1920×1080 |
| 8192×8192 at 50 % and 100 % (`P2-009-max-run.txt`) | 8192×8192 |

- After each change the transform updated within 1.5 s and stayed at scale 1.0, with no restart
  and no re-add.
- **10 scripted changes, 400 ms apart:** 1920×1080 → 1080×1920 → 720×1280 → 2560×1440 → 640×360 →
  1080×1080 → 3840×2160 → 400×1600 → 1600×400 → 1920×1080. The last transform reads 1920×1080.

## One `sv_resize` per change (`P2-009-plugin-log.txt`)

```
11:54:45.091: core resized to 1080x1920 (source 1080x1920, render scale 100 %, 4.8 ms)
11:54:46.888: core resized to 1280x720 (source 1280x720, render scale 100 %, 3.9 ms)
11:54:48.588: core resized to 128x128 (source 128x128, render scale 100 %, 4.0 ms)
11:54:50.097: core resized to 8192x128 (source 8192x128, render scale 100 %, 10.3 ms)
11:54:51.592: core resized to 128x8192 (source 128x8192, render scale 100 %, 7.5 ms)
...
11:55:16.155: core resized to 540x960 (source 1080x1920, render scale 50 %)
11:55:20.321: core resized to 1440x810 (source 1920x1080, render scale 75 %)
11:59:39.959: core resized to 4096x4096 (source 8192x8192, render scale 50 %, 4.0 ms)
11:59:45.027: core resized to 8192x8192 (source 8192x8192, render scale 100 %, 4.9 ms)
```

- The session made 65 real size or scale changes, and the log has 65 `core resized` lines.
- Setting a value that is already in effect logs nothing. The first step of each scale run and the
  repeated clamps show this.
- The 10-change burst gives exactly 10 lines, 400 ms apart.
- A resize costs 3.9–10.3 ms on the graphics thread, once per change.

## Orientation and no stretch (probe preset, `P2-009-probe-run.txt`)

The probe from P2-003 (`probe-orientation.milk`) draws a white top-left quarter square, a red ramp
from black at the top to red at the bottom, and a blue right half.

| Shot (source size, scale) | row 10 / 50 / 90 % R | right half | top-left | white square ends | blue starts |
|---|---|---|---|---|---|
| 1080×1920, 100 % | 26 / 128 / 230 | (0,0,255) | white | x 0.250, y 0.250 | x 0.500 |
| 1080×1920, 50 % | 26 / 128 / 230 | (0,0,255) | white | x 0.249, y 0.249 | x 0.500 |
| 1920×1080, 100 % | 26 / 128 / 230 | (0,0,255) | white | x 0.250, y 0.250 | x 0.500 |
| 1920×1080, 75 % | 26 / 128 / 230 | (0,0,255) | white | x 0.249, y 0.249 | x 0.500 |

- These values are identical to P2-003, so `GS_FLIP_V` and sRGB still hold.
- The texture maps exactly onto the full source size at every scale. The 0.001 offset at 50 % and
  75 % is the bilinear upscale edge.

**Resized vs created at size (`P2-009-aspect-run.txt`, `P2-009-aspect-extent.txt`).** A second
instance, `P2-009 Native`, was created directly at 1080×1920 and compared with `P2-009 Viz`, which
was created at 1920×1080 and then resized. Probe `probe-aspect.milk` draws a disk where the comp
`rad < 0.4`.

| Shot | resized instance | native instance |
|---|---|---|
| 1080×1920 | disk 432×768, centre (540,960) | disk 432×768, centre (540,960) |
| 1080×1920 at 50 % | disk 432×768, centre (540,960) | disk 432×768, centre (540,960) |
| 1920×1080 | disk 768×432, centre (960,540) | disk 768×432, centre (960,540) |

- A resized context renders the same geometry as one created at that size.
- The swarm screenshots at 1080×1920 (`swarm-portrait-resized/native.png`) show the same shape;
  only the colours differ, and they animate.
- So the portrait look is the core's look for that aspect, not a plugin stretch.

## Render scale and `GetStats`

**GPU-bound case (`P2-009-gpu-bound-run.txt`).** Source 8192×8192, swarm, two rounds, sampled
every 3 s:

| Scale | core size | `averageFrameRenderTime` | `activeFps` | skipped frames per 12 s |
|---|---|---|---|---|
| 100 % | 8192×8192 | 46.9–48.6 ms (round 2: 43.3–48.6) | 20.3–23.2 | +112 / +97 |
| 75 % | 6144×6144 | 6.5–9.9 ms | 30.0 | 0 |
| 50 % | 4096×4096 | 6.7–8.9 ms | 30.0 | +2 (on the switch) / +1 |

Render scale is what keeps a large source within the frame budget.

**Not GPU-bound (`P2-009-scale-run.txt`, `-scale-4k-run.txt`, `-scale-8k-run.txt`,
`-settle-run.txt`).** At source sizes 1920×1080, 3840×2160 and 7680×4320, GetStats reads about
7 ms at every scale, with 30 fps and 0 skipped frames.
- At 100 % the 1920×1080 and 3840×2160 means are the same (7.05 vs 7.06 ms). On the OpenGL
  backend `averageFrameRenderTime` therefore measures CPU-side submission, not GPU fill cost.
- The per-run means put 50 % slightly *higher* (8.2–8.9 ms). The settle run shows why. A periodic
  hump of 8–9.7 ms lasts about 10 s every 35–40 s whatever the scale (t=15–24 s at 100 %,
  t=52–64 s at 50 %, t=88–94 s at 100 %, t=121–124 s at 75 %). It aliased into the short 50 %
  windows.
- Outside the hump every scale reads 6.8–7.3 ms.
- So the 50 % saving does not show in GetStats until the GPU is the bottleneck. The acceptance
  line "50 % lowers GetStats render time" is met only under GPU load.

**Screenshots at full source size.** `P2-009-swarm-1920x1080-50.png` is 1920×1080 from a 960×540
core, and `P2-009-swarm-3840x2160-50.png` is a 3840×2160 source from a 1920×1080 core (shot at
1920 wide). Both fill the frame, softer than 100 % as expected from the upscale.

## GL state (`P2-009-region-diff.txt`)

`Scene` was captured with `P2-009 Viz` hidden before any resize, and again after all 65 resizes
(including 8192×8192 and the bursts):

```
image (60,700)-(460,1000): 0 of 120000 pixels differ, max channel delta 0
colour (760,450)-(1000,630): 0 of 43200 pixels differ, max channel delta 0
text (60,1100)-(939,1192): 0 of 80868 pixels differ, max channel delta 0
background (0,1200)-(1080,1920): 0 of 777600 pixels differ, max channel delta 0
```

- The collection's own visualizer animates and was left out of the diff. It kept drawing
  correctly in every scene shot (`scene-viz-off-after.png`, `scene-viz-on-after.png`).
- OBS log for the whole session: 0 lines matching `GL_INVALID|device_*failed|gl*failed`, and 0
  `preset failed to commit`.
- `sv_resize` writes framebuffer bindings, the 2D texture binding, the viewport, scissor, colour
  mask and clear colour.
  - The guard restores the first three.
  - `sv_render_frame` already writes the last three every frame (`blend_state.cpp`).
  - libobs-opengl sets those three directly on each use, with no cache (`gl-subsystem.c`).

## Leaks and cleanup (`P2-009-cleanup-run.txt`)

- `P2-009 Viz` and `P2-009 Native` were removed. `GetInputList` shows only the collection's five
  original inputs.
- The collection is back on `Untitled`. The canvas still reads 1080×1920 at 30 fps.
- OBS quit: `[synesthesia-vibe-obs] plugin unloaded`, `Number of memory leaks: 0`.
- OBS was relaunched and is on `Untitled`. The P2-009 build stays installed, and
  `codesign --verify` passes.

## Files

- `P2-009-resolution.mjs` takes a phase: `setup`, `sizes`, `probe`, `scale [w h]`, `max`, `after`
  or `cleanup`.
  - `P2-009-settle.mjs` takes a JSON list of `[settings, seconds]`.
  - `P2-009-aspect.mjs` runs the resized-vs-native comparison.
  - `P2-009-ws-session.mjs` is the connection helper. Rename it to `ws-session.mjs` first.
- `P2-009-probe-extent.py`, `P2-009-disk-extent.py` and `P2-009-diff-regions.py` import
  `decode_png`/`pixel` from the P2-003 sampler. Copy `../P2-003/P2-003-alpha-probe.py` beside
  them as `alpha_probe.py` and run them with `python3 -I`.
- `probe-orientation.milk` and `probe-aspect.milk` are the probe presets.
- The `P2-009-*-run.txt` files hold the script outputs, and `P2-009-plugin-log.txt` holds every
  plugin log line.
- To run: `OBS_WS_CREDS=<creds file> node P2-009-resolution.mjs <phase> <fixture root> <probe dir>
  <shots dir>`. The fixture root holds `presets-a` (P2-006).
