# P2-009

## Subtasks executed

- **Settings:**
  - `width` and `height` are ints, default 1920×1080, clamped to 128–8192 per side (C-012).
  - `render_scale` is an int list: 100 % (full, default), 75 % and 50 %. Values are clamped to
    50–100.
  - `get_defaults` sets all three. The properties show Width and Height with a ` px` suffix, then
    Render scale, after the preset settings. P2-010 owns the final grouping.
- **Storage:**
  - `configure_output_size` runs from `update` (UI thread) and once in `create` before
    `sv_create`.
  - It stores width and height together in one `std::atomic<output_size>`, an 8-byte struct, so a
    render never reads a half-written pair. The build has no libatomic calls, so it is lock-free.
  - The render scale is stored in a separate atomic.
- **`get_width` / `get_height`** return the stored size.
- **Render (graphics thread):**
  - `render_core_frame` calls `resize_core` first.
  - `resize_core` computes the scaled core size (`lround(size × scale / 100)`, at least 1).
  - When that size differs from the last one, it calls `sv_resize` inside `with_core_gl_state`,
    before `gs_texrender_begin`, and logs
    `core resized to WxH (source WxH, render scale N %, X ms)` once.
  - `gs_texrender_begin` uses the scaled size.
  - The sprite is drawn at the full source size:
    `gs_draw_sprite(texture, GS_FLIP_V, size.width, size.height)`.
- **Create:** `sv_create` gets the initial scaled size, and `core_size` is recorded. The create
  log line prints that size.
- **Locale:** `Width`, `Height`, `RenderScale`, `RenderScale.Full`, `RenderScale.ThreeQuarters`
  and `RenderScale.Half`.

## Files changed

- `plugin/src/visualizer-source.cpp`. The working-tree diff also holds the uncommitted
  P2-002…P2-008 changes.
- `plugin/data/locale/en-US.ini` (six keys).
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/input-dev.md` (P2-009
  section appended).
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/output-P2-009.md`.
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-009/P2-009-resolution.md`
  (tracked).
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-009/P2-009-*.py`
  (tracked: the probe extent, disk extent and region diff samplers).
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-009/` also
  holds git-ignored local evidence:
  - the screenshots (`*.png`, at most 1280 px);
  - the scripts (`*.mjs`);
  - the probe presets (`*.milk`);
  - the run outputs and plugin log (`*.txt`).
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/session.log` (log lines).

## What changed and why

**Size settings.** The output size was a 1920×1080 constant. It is now a setting, which follows
PLAN item 6 and C-012. Streamers on a portrait canvas set 1080×1920, and the scene item's source
size follows live.

**Render scale.** The scale sizes only the core and the texrender. The sprite still covers the
full source, so the scene layout never changes with the scale; the image is upscaled with the
opaque effect's linear sampler.

**Where `sv_resize` runs.** It runs on the graphics thread inside the guard, as the threading
contract and the plan summary require: it was not in the spike audit, and P2-011 re-checks it. It
runs before `gs_texrender_begin`:
- the guard restores OBS's own target;
- the texrender then binds its buffer at the new size.

**GL state `sv_resize` touches** (core `FrameTargets`, `Framebuffer`, `Texture`, `BlurChain`,
`MotionInterpolator`):
- `GL_FRAMEBUFFER` bindings, the active unit's 2D texture and the viewport. All three are guarded.
- Scissor off, colour mask on and the clear colour (`ClearFramebuffer`). `sv_render_frame` already
  writes these every frame (`blend_state.cpp`), and libobs-opengl sets them on each use without a
  cache.

So the resize adds no new state class, and the OBS check confirms it.

## Verification

- `cmake --build --preset macos-arm64` (from `plugin/`): `** BUILD SUCCEEDED **`. The build runs
  with `-Werror`, and `plugin/src` gave no diagnostics.
  - `nm -gU` lists 7 exports, unchanged.
  - `nm -u` shows no `__atomic_*` imports.
- **`sv_*` reachability:**
  - `sv_resize` is called only from `resize_core`, which is called only from `render_core_frame`.
  - `update` only stores atomics.
- **OBS 33.0.3**, `synesthesia-spike`, program scene, canvas 1080×1920 at 30 fps (unchanged),
  websocket only. The full write-up is in `evidence/P2-009/P2-009-resolution.md`.
  - **Defaults:** `GetInputDefaultSettings` has `width 1920`, `height 1080`, `render_scale 100`.
    A new source is 1920×1080.
  - **Transforms:** `GetSceneItemTransform` follows every change live, with no restart:
    - 1080×1920, 1280×720, 128×128, 8192×128, 128×8192 and 8192×8192;
    - out of range, 9000×50 → 8192×128 and 50×9000 → 128×8192.
  - **Resize logging:** 65 real changes gave 65 `core resized` lines. A no-op set gave none. The
    10-change burst at 400 ms gave 10 lines. A resize takes 3.9–10.3 ms.
  - **Orientation (probe):**
    - At 1080×1920 and 1920×1080, at 100 %, 75 % and 50 %, the ramp reads R 26/128/230, the right
      half is blue and the top-left is white. These are the P2-003 values.
    - The white square ends at 0.25 and blue starts at 0.50 of the full source size.
    - A resized instance and an instance created at the target size give the same geometry
      (aspect probe disk 432×768 at the same centre, at 100 % and 50 %).
  - **Render scale (GetStats):**
    - At 8192×8192, 100 % reads 43–49 ms at 20–23 fps, skipping about 100 frames per 12 s. 75 %
      and 50 % read 6.5–9.9 ms at 30 fps with no skips. Two rounds gave the same result.
    - At 1920×1080, 3840×2160 and 7680×4320, GetStats is flat at about 7 ms at every scale. See
      the limitations below.
  - **GL state:**
    - The image, colour and text regions and the background of `Scene` are bit-identical before
      and after all the resizes.
    - The collection's own visualizer kept drawing.
    - The OBS log has 0 GL error lines and 0 commit failures.
  - **Leaks:** quit printed `plugin unloaded` and `Number of memory leaks: 0`.
- **Cleanup:**
  - The test inputs are removed and the collection is back on `Untitled`.
  - OBS was relaunched on `Untitled`.
  - The P2-009 build stays installed, and `codesign --verify` passes.

## Evidence

- `evidence/P2-009/P2-009-resolution.md`: the transforms table, the resize log excerpt and count,
  the probe tables, the resized-vs-native comparison, the GetStats tables with the hump analysis,
  the region diff, leaks and cleanup.
- Screenshots (viewed):
  - `P2-009-swarm-1920x1080.png`, `-1080x1920.png`, `-1280x720.png`: full-frame swarm at each
    size.
  - `P2-009-swarm-1920x1080-50.png` and `P2-009-swarm-3840x2160-50.png`: 50 % at full source size,
    upscaled, filling the frame.
  - `P2-009-probe-1080x1920-100.png`, `-50.png`, `P2-009-probe-1920x1080-75.png`: orientation.
  - `P2-009-swarm-portrait-resized.png` / `-native.png` and
    `P2-009-aspect-1080x1920-50-resized.png` / `-native.png`: resized vs created at size.
  - `P2-009-scene-viz-off-before.png` / `-after.png` / `P2-009-scene-viz-on-after.png`: the
    whole-scene GL check.
  - `P2-009-swarm-8192x8192-100-thumb.png`: the max size.
- `P2-009-plugin-log.txt` (paths scrubbed), the `P2-009-*-run.txt` outputs, the scripts and the
  probe presets.

## Known limitations / follow-ups

- **GetStats does not show the render-scale saving unless the GPU is saturated.** On the OpenGL
  backend, `averageFrameRenderTime` measures CPU-side submission. At 1920×1080, 3840×2160 and
  7680×4320 it reads about 7 ms at every scale, and 100 % shows the same value at 1080p and 4K.
  - Short windows also alias a periodic 8–9.7 ms hump: about 10 s every 35–40 s, whatever the
    scale.
  - The saving is clear once the GPU is the bottleneck (8192×8192: 47 ms / 21 fps → 7 ms /
    30 fps).
  - So TC-021's "lower render time at 50 %" holds only under GPU load. P2-012 (1080p60) may want
    GL timer queries in a scratch build if GPU time must be quantified at normal sizes.
- **A resize restarts the feedback image.** The core clears its frame targets and motion history,
  so the image rebuilds for about a second. Every resize gives this brief visual reset, which is
  expected core behaviour.
- **The texsize mode depends on the size at create.** `sv_create` gets `texsize = width/height`.
  The core classifies each axis as snapped (a multiple of 16) or exact at create, and keeps that
  mode for later resizes. A source created at 1920×1080 and resized to 1080×1920 has texsize
  1088×1920, against 1080×1920 for one created at that size. The output geometry is identical (the
  aspect probe matched to the pixel), so this is cosmetic. Passing explicit texsizes would change
  core behaviour and was left alone.
- **8192×8192 at 100 % is accepted but too heavy on an M1** at 30 fps (21 fps). The limits follow
  C-012. The README (P2-014) should point at render scale for large sizes.
- **Width/height and the render scale are separate atomics.** An update that changes both, landing
  between the two loads on the render thread, could give one intermediate resize (two log lines).
  This was not observed: 65 changes gave 65 lines.
- **Observed, not P2-009:** the collection's own spike visualizer, with no folder and cycle 16 s,
  logged `auto-cycle requested a preset (timer …)`. This is the existing P2-007 behaviour.
- **No unit test** was added (none requested). `scaled_dimension` and the clamping are the only
  arithmetic.
- **Doc gaps for document-writer:**
  - `docs/architecture.md`: render step 3 now begins with `resize_core` (scaled size, `sv_resize`
    in the guard, before `gs_texrender_begin`). The texrender is the scaled size, not 1920×1080.
    The sprite is drawn at the source size (`gs_draw_sprite(tex, GS_FLIP_V, w, h)`), so step 4's
    `0, 0` is out of date. Width/height are in an atomic pair.
  - `.claude/how-x-works/plugin-render-frame.md`: the same render-step change.
  - `docs/concerns.md`: GetStats on GL does not reflect GPU fill. A resize resets the feedback
    image. The texsize mode is fixed at create.
  - `.claude/common-issues/plugin-runtime.md`: `sv_resize` writes the same unguarded scissor,
    colour-mask and clear-colour state as `sv_render_frame`.
  - `plugin/README.md` (P2-014): set 1080×1920 for a portrait canvas (C-012), the 128–8192 limits,
    and render scale as the performance knob for large sizes.

## Handoff for downstream review

- **Acceptance criteria addressed:**
  - 1080×1920 gives a 1080×1920 item (`GetSceneItemTransform`) that fills it, with the same
    geometry as a native-size context.
  - 50 % lowers GetStats render time under GPU load and still fills the item. At normal sizes
    GetStats is flat (see the limitations).
  - No GL errors across the 10 scripted changes or the 65 changes in total.
  - One resize log line per change, applied live.
  - 0 leaks.
- **Areas to scrutinize:**
  - `resize_core` placement before `gs_texrender_reset`/`begin` in `render_core_frame`.
  - `std::atomic<output_size>`: 8 bytes, trivially copyable, no libatomic import.
  - `configure_output_size` is called twice on create (once before `sv_create`, once through
    `visualizer_update`). The second call is idempotent.
  - `core_size` is render-thread only, and is set in `create` under `obs_enter_graphics`.
  - The guard does not cover scissor, colour mask or clear colour for `sv_resize`. See *What
    changed and why*.
- **How to run:** see *Files* in `evidence/P2-009/P2-009-resolution.md`.
