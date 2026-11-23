# P2-003

## Subtasks executed
- `visualizer_video_render` now draws the texrender texture with
  `obs_get_base_effect(OBS_EFFECT_OPAQUE)` and its `Draw` technique, instead of
  `OBS_EFFECT_DEFAULT`.
- Kept unchanged: the `_srgb` texture setter plus framebuffer sRGB when `gs_get_linear_srgb()` is
  set, `GS_FLIP_V`, and the blend push / disable / pop.
- Captured before (installed P2-002 build) and after (this build) with swarm, a deterministic
  orientation/sRGB probe preset and Fractopia. Ran a 30 s stress smoke on the final build.

## Files changed
- `plugin/src/visualizer-source.cpp`: one line, `OBS_EFFECT_DEFAULT` → `OBS_EFFECT_OPAQUE`. The
  working-tree diff also holds the still-uncommitted P2-002 changes.
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/input-dev.md` (P2-003
  section appended)
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/output-P2-003.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-003/P2-003-opaque-draw.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-003/P2-003-alpha-probe.py`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-003/P2-003-*.png`,
  `P2-003-capture.mjs`, `P2-003-cleanup.mjs`, `P2-003-probe-orientation.milk` (git-ignored local
  evidence)
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/session.log` (log lines)

## What changed and why
The core's texrender keeps whatever alpha a preset writes. Swarm writes 0 everywhere, and
`default.effect`'s `Draw` copied that alpha onto the canvas, so a nested scene or a colour source
underneath showed through. Before the fix, 77% of the visualizer area was pure magenta in the test
scene. `opaque.effect`'s `Draw` (`PSDraw`) has the same `image` parameter, the same linear/clamp
sampler and the same vertex shader as `default.effect`. It returns `float4(rgb, 1.0)`, so only
alpha changes. The sRGB path is untouched because the `image` param binding and the framebuffer
sRGB toggle are the same calls on a different effect. `opaque.effect` is a libobs base effect
loaded at startup (`obs.c`: `obs->video.opaque_effect`), so no new resource is needed.

## Verification
- `cmake --build --preset macos-arm64` (from `plugin/`): `** BUILD SUCCEEDED **`, no warning from
  `plugin/src`. `nm -gU` lists exactly the 7 exports, and `lipo -info` reports arm64.
- Installed per `docs/integrations.md` (ditto, dev presets/textures, ad-hoc re-sign, verify OK).
  Installed `__text` matches the build.
- OBS 33.0.3 (OpenGL), collection `synesthesia-spike`:
  - swarm visualizer source: min alpha **0 → 255** (all 2073600 pixels);
  - swarm nested over magenta: magenta pixels in the visualizer area **902965 → 0**;
  - probe: R = 26/128/230 at rows 10/50/90%, blue right half, white top-left, both before and after.
    The full-frame diff before against after is **0 pixels**: no double encode, no orientation
    change;
  - 30 s stress smoke: 580/580 screenshots OK, render time 6.0–6.8 ms, skipped frames flat;
  - **0 GL error lines** in both session logs, and `Number of memory leaks: 0` on both quits.
- `Untitled` restored and the test scenes/input removed (re-checked by reloading the collection).
  OBS reopened with the plugin installed.

## Evidence
- `evidence/P2-003/P2-003-opaque-draw.md`: measurements, stress summary, log excerpt, setup and
  cleanup, probe preset.
- Swarm: `P2-003-before-swarm-visualizer.png` (alpha 0) / `P2-003-after-swarm-visualizer.png`
  (opaque, viewed); `P2-003-before-swarm-nest.png` (magenta through the visualizer, viewed) /
  `P2-003-after-swarm-nest.png` (visualizer opaque over magenta, viewed).
- Colour/orientation: `P2-003-before-probe-visualizer.png` / `P2-003-after-probe-visualizer.png`
  (pixel-identical, viewed).
- Fractopia: `P2-003-before-fractopia-visualizer.png` / `P2-003-after-fractopia-visualizer.png`
  (viewed; hue differs only because the preset animates), `P2-003-after-fractopia-nest.png`.
- Other sources: `P2-003-before-scene.png` / `P2-003-after-scene.png` (image, colour and text
  draw the same, viewed).
- Tooling: `P2-003-alpha-probe.py` (`alpha` / `probe` / `magenta` modes),
  `P2-003-capture.mjs <out-prefix> <preset dir> <probe path>`, `P2-003-cleanup.mjs`.
- PNGs stay at native size (1920×1080 / 1080×1920) so the alpha measurement is on the real
  pixels. They are git-ignored, so the 1280 px downscale rule for committed images does not apply.

## Known limitations / follow-ups
- Alpha is now always 1.0. If a future setting wants a transparent visualizer (presets as an
  overlay), it would switch back to `OBS_EFFECT_DEFAULT` or a premultiplied path. Out of scope here.
- `OBS_EFFECT_OPAQUE` also has `DrawSrgbDecompress` / `DrawMultiply` / tonemap techniques for
  HDR or non-sRGB canvases. Only `Draw` is used, which matches the SDR sRGB canvas tested. HDR
  colour spaces are not covered (not in Phase 2 scope).
- No unit test added (none requested).
- Doc gaps for document-writer:
  - `docs/architecture.md`, plugin frame step 4: `OBS_EFFECT_DEFAULT` → `OBS_EFFECT_OPAQUE` (forces
    alpha 1.0);
  - `.claude/how-x-works/plugin-render-frame.md`, lines 57–61: same change;
  - `docs/concerns.md`: if it lists "alpha-0 presets show through", mark it fixed.

## Handoff for downstream review
- Acceptance criteria addressed: swarm fully opaque on the visualizer screenshot and on a scene
  nesting the visualizer's scene over a colour source (min alpha 255, 0 magenta pixels).
  Fractopia's orientation and colours are unchanged, shown by the pixel-identical deterministic
  probe (R=128 at 50%). `GS_FLIP_V`, blend push/pop and the sRGB handling are kept. 0 GL errors.
- Areas to scrutinize: that `opaque.effect`'s `image` param name and sampler match
  `default.effect` (checked in `plugin/.deps/obs-studio-31.1.1/libobs/data/opaque.effect`).
- How to run: with the visualizer in `synesthesia-spike`,
  `OBS_WS_CREDS=<creds file> node P2-003-capture.mjs <out-prefix> "<bundle>/Contents/Resources/presets" <probe .milk>`,
  then `python3 -I P2-003-alpha-probe.py alpha <png> [x0,y0,x1,y1]`, `probe <png>` or
  `magenta <png> 0,0,1080,1080`. Finish with `node P2-003-cleanup.mjs`.
