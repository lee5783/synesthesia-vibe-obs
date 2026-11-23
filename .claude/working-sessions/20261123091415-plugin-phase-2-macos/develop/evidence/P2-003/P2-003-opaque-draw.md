# P2-003 — Opaque draw evidence

OBS 33.0.3 (OpenGL renderer), libobs 31.1.1, canvas 1080×1920 at 30 fps, test collection
`synesthesia-spike`. "Before" is the installed P2-002 build. "After" is the same source with only
`OBS_EFFECT_DEFAULT` → `OBS_EFFECT_OPAQUE` in `visualizer_video_render`. All captures come from
obs-websocket `GetSourceScreenshot` (PNG, RGBA, native size), taken by `P2-003-capture.mjs`.

## Test setup (temporary, removed afterwards)

- Scene `P2-003 Viz`: one scene item of the existing `SynesthesiaVibe Visualizer` input, default
  transform, so it covers (0,0)–(1080,1080) of the canvas.
- Scene `P2-003 Nest`: a full-canvas `color_source_v3` `P2-003 Backdrop` (pure magenta
  `#FF00FF`), with `P2-003 Viz` nested on top. Magenta inside the visualizer area means the
  visualizer is not opaque.
- Presets set via `SetInputSettings` `preset_path`: `shifter - swarm.milk` (writes alpha 0), then
  the orientation/sRGB probe `P2-003-probe-orientation.milk`, then `""` (default Fractopia).
- `P2-003-cleanup.mjs` removed both scenes and the backdrop, reset `preset_path` to `""` and
  switched back to `Untitled`. A later reload of `synesthesia-spike` confirmed the collection is
  back to its three original scenes and five inputs.

## Alpha (`P2-003-alpha-probe.py`, pure-Python PNG decode, run with `python3 -I`)

| Capture | Before (P2-002) | After (P2-003) |
|---|---|---|
| swarm, visualizer source (1920×1080) | `min_alpha=0 max_alpha=0`, 2073600/2073600 pixels below 255 | `min_alpha=255 max_alpha=255`, 0 below 255 |
| swarm, `P2-003 Nest`, region (0,0)–(1080,1080) | `pure_magenta_pixels=902965/1166400` | `pure_magenta_pixels=0/1166400`, `min_alpha=255` |
| Fractopia, visualizer source | `min_alpha=255` | `min_alpha=255` |
| Fractopia, `P2-003 Nest`, same region | — | `pure_magenta_pixels=0/1166400` |
| probe, visualizer source | `min_alpha=255` | `min_alpha=255` |

## Colour and orientation (deterministic probe preset)

The probe's comp shader writes `ret = (uv.y, 0, 0)`, blue for `uv.x > 0.5` and white for the
top-left quarter-square. So the expected image is: white square top-left, black at the top
grading to red at the bottom, blue right half. A 50% row reading R=128 means the sRGB path stores
and re-encodes once. A double encode would read about R=188.

| Sample | Before | After |
|---|---|---|
| row 10%, x=37% | (26, 0, 0, 255) | (26, 0, 0, 255) |
| row 50%, x=37% | (128, 0, 0, 255) | (128, 0, 0, 255) |
| row 90%, x=37% | (230, 0, 0, 255) | (230, 0, 0, 255) |
| right half, x=75% | (0, 0, 255, 255) | (0, 0, 255, 255) |
| top-left, 10%/10% | (255, 255, 255, 255) | (255, 255, 255, 255) |

Full-frame diff of `before-probe-visualizer.png` against `after-probe-visualizer.png`: **0 of
2073600 RGB pixels differ, max channel delta 0** (alpha included). The opaque draw changes nothing
but alpha for a preset that already writes alpha 1, and colour and orientation are unchanged.

Fractopia's colour cycles over time, so its before/after frames differ in hue (cyan vs pink).
That is animation, not a colour change. The probe is the colour check.

## Other sources

`P2-003-before-scene.png` and `P2-003-after-scene.png` (the spike's `Scene`): the image source
(red corner top-left), the green colour source overlapping the visualizer corner and the white
text all draw the same in both. Blending for the sources drawn after the visualizer is intact.

## Smoke stress (P2-002's `P2-002-stress.mjs`, 30 s, final build)

```
{"summary":{"elapsedS":31.109,"sent":580,"ok":580,"failed":0,"ratePerS":18.64,
 "latencyMs":{"p50":11,"p95":20,"max":53},"otherItemToggles":57,"visualizerHideShowCycles":2}
"statsBefore":{"averageFrameRenderTime":6.768372,"renderSkippedFrames":10,"renderTotalFrames":4415}
"statsAfter":{"averageFrameRenderTime":5.976466,"renderSkippedFrames":10,"renderTotalFrames":5348}}
```

Render time stays in P2-002's 5.4–8.9 ms range. `renderSkippedFrames` is flat (the 10 come from
startup).

## OBS log (final build session; paths scrubbed)

GL error lines (`GL_INVALID|(GL) failed|failed (GL)|gl.*failed`): **0** in the P2-002 "before"
session log and **0** in the P2-003 session log.

```
09:52:44.774: [synesthesia-vibe-obs] renderer check passed (OpenGL, device type 1)
09:52:44.774: [synesthesia-vibe-obs] plugin loaded successfully (version 0.1.0)
09:52:44.984: [synesthesia-vibe-obs] sv_init_gl and sv_create succeeded (1920x1080)
09:52:45.092: [synesthesia-vibe-obs] preset loaded: <bundle>/presets/Rovastar - Fractopia (Upspoken Mix).milk
09:53:10.596: [synesthesia-vibe-obs] preset loaded: <bundle>/presets/shifter - swarm.milk
09:53:15.318: [synesthesia-vibe-obs] preset loaded: <scratch>/probe-orientation.milk
09:53:19.326: [synesthesia-vibe-obs] preset loaded: <bundle>/presets/Rovastar - Fractopia (Upspoken Mix).milk
09:55:52.023: Switched to scene collection 'Untitled' (Untitled.json)
09:56:12.808: ==== Shutting down ==================================================
09:56:12.979: [synesthesia-vibe-obs] plugin unloaded
09:56:13.065: Number of memory leaks: 0
```

The P2-002 session quit before the reinstall also logged `Number of memory leaks: 0`.

## Install

`cmake --build --preset macos-arm64` → `ditto` into the plugins folder → presets and textures
re-copied (dev only, C-003) → `codesign --force --sign -` → `codesign --verify` OK. The installed
`__text` matches the build output (`otool -t` diff empty); the whole files differ only in the
signature. OBS was quit with osascript (C-027) and reopened on `Untitled`, and the plugin stays
installed.

## Probe preset (`P2-003-probe-orientation.milk`, git-ignored, reproduced here)

```
MILKDROP_PRESET_VERSION=201
PSVERSION=2
PSVERSION_WARP=2
PSVERSION_COMP=2
[preset00]
fWaveAlpha=0.000000
fDecay=1.000000
comp_1=`shader_body
comp_2=`{
comp_3=`    float3 c = float3(uv.y, 0, 0);
comp_4=`    if (uv.x > 0.5) c = float3(0, 0, 1);
comp_5=`    if (uv.x < 0.25 && uv.y < 0.25) c = float3(1, 1, 1);
comp_6=`    ret = c;
comp_7=`}
```
