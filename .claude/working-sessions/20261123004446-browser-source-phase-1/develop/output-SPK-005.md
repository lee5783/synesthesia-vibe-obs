# SPK-005

## Subtasks executed
- `create`, inside `obs_enter_graphics()`:
  - `sv_init_gl` with a get-proc that is `dlsym` on `dlopen("/System/Library/Frameworks/OpenGL.framework/OpenGL")`.
  - `sv_create` at 1920×1080, with `resource_path` = the module data path (bundle `Contents/Resources`)
    and `cache_path` = `obs_module_config_path("cache")`, created with `os_mkdirs`.
  - A debug callback every 10 s.
  - `gs_texrender_create(GS_RGBA, GS_ZS_NONE)`.
- Presets:
  - `update` queues the preset path. The `preset_path` setting is used if set; otherwise
    `obs_module_file("presets/Rovastar - Fractopia (Upspoken Mix).milk")`.
  - `video_render` loads the queued preset on the graphics thread.
- `video_tick` adds up the delta, marks a frame as pending and pushes a synthetic stereo tone at
  44.1 kHz through `sv_push_pcm_float`: 60 Hz plus 880 Hz, pulsing at 2 Hz.
- `video_render`:
  1. Once per tick: `gs_texrender_reset` → `gs_texrender_begin(1920, 1080)` → framebuffer sRGB off →
     `glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING)` → `sv_render_frame(ctx, dt, fbo)` → framebuffer
     sRGB restored → `gs_texrender_end`.
  2. Draw the texture with `obs_get_base_effect(OBS_EFFECT_DEFAULT)`, technique `Draw`,
     `gs_draw_sprite(tex, GS_FLIP_V, 0, 0)`, blending off. sRGB is handled the way
     `obs_source_draw` handles it.
- `destroy`, inside graphics: `sv_destroy` and `gs_texrender_destroy`.
- First run with no GL save/restore. I recorded what broke (see below), then added a minimal
  `GL_CURRENT_PROGRAM` save/restore (`gl_program_guard`) around every core GL call and flagged it
  for SPK-006.
- Orientation probe: a preset in the scratchpad with a gradient composite shader showed the frame
  upside down, so I added `GS_FLIP_V` and re-checked.
- Removed the SPK-003 `core linked (...)` spike line, and the core include, from
  `plugin-main.cpp`.
- OBS test, websocket only, in the `synesthesia-spike` collection:
  - Added `SPK-005 Image` (a PNG I generated in the scratchpad), `SPK-005 Colour` and
    `SPK-005 Text`.
  - Added a second scene, `SPK-005 Other`, and made 8 program-scene switches over 60 s.
  - Took `GetSourceScreenshot` captures and sampled `GetStats`.
  - Restored `Untitled` at the end.
- Wrote the "Rendering into OBS" answer into `PLAN.md`.

## Files changed
- `plugin/src/visualizer-source.cpp`: core lifecycle, texrender rendering, preset queue, synthetic tone, program guard, debug log.
- `plugin/src/plugin-main.cpp`: removed the `core linked` spike line and the now-unused core include.
- `plugin/CMakeLists.txt`: `target_compile_definitions(... PRIVATE GL_SILENCE_DEPRECATION)`. The
  plugin includes `<OpenGL/gl3.h>`, and the template builds with `-Werror`.
- `PLAN.md`: answer under the *Rendering into OBS* spike bullet.
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/input-dev.md`: SPK-005 section appended.
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/output-SPK-005.md`
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/SPK-005/`: see *Evidence*.
- Outside the repo:
  - The installed `synesthesia-vibe-obs.plugin` was replaced with this build, plus the 10 mvp
    presets copied into `Contents/Resources/presets/` and the 23 textures into
    `Contents/Resources/textures/`, then ad-hoc re-signed (S-009, never committed).
  - The `synesthesia-spike` collection now holds the three test sources and the `SPK-005 Other`
    scene.
  - The plugin config dir `plugin_config/synesthesia-vibe-obs/cache` was created.
- Not touched: `external/synesthesia-vibe/` (clean), OBS config files, the OBS profile and video
  settings, `docs/`.

## What changed and why
The visualizer source now drives the core for real and draws its frame in OBS. That answers the
spike's Q2: rendering with `sv_render_frame` into the texrender FBO inside `video_render` works.

There is one catch. The core leaves its own GL program bound, and `libobs-opengl` caches
`cur_program` and skips `glUseProgram` when it thinks its program is still bound. On a fresh start
with the visualizer alone, this caused GL errors on every frame:
- `glUniformMatrix4fv failed … GL_INVALID_OPERATION`, `glActiveTexture failed` and
  `device_load_texture (GL) failed`.
- 6951 such lines in a 40 s run.
- The visualizer drew as a black quad over a quarter of the canvas.

With other sources in the scene, the draw order happened to resync the cache. The errors fell to
bursts on screenshots and when items were shown or hidden.

Saving and restoring `GL_CURRENT_PROGRAM` around every core call removed all of it: 0 GL error
lines in the following runs. The frame also came out upside down, because the core uses GL's
bottom-left origin and OBS treats texrender row 0 as the top. `GS_FLIP_V` on the sprite fixes it.

## Verification
- Build: `cd plugin && VCPKG_ROOT=$HOME/vcpkg cmake --preset macos-arm64` exited 0;
  `cmake --build --preset macos-arm64` gave `** BUILD SUCCEEDED **` 3 times (initial, program
  guard, flip). No warnings or errors in `plugin/src`, and the template builds with `-Werror`.
- Install and reload, each time:
  1. `osascript … quit` (it prints the known benign `User canceled (-128)`).
  2. `ditto` the bundle into the user plugin folder.
  3. Copy the presets and textures.
  4. `codesign --force --sign -`, then `codesign --verify`, which reported "signature ok".
  5. `open -a OBS`. The websocket was up in 3 s.
- OBS log, final build:
  - `renderer check passed (OpenGL, device type 1)`
  - `sv_init_gl and sv_create succeeded (1920x1080)`
  - `preset loaded: …/Contents/Resources/presets/Rovastar - Fractopia (Upspoken Mix).milk`
  - `frame N, 30.0 fps, preset "Rovastar - Fractopia (Upspoken Mix)", gl_errors 0, last_error ""`
    every 10 s for about 3.5 minutes.
  - 0 GL error lines.
  - On quit: `plugin unloaded` and `Number of memory leaks: 0`.
  - The collection switch back to `Untitled` destroyed the source with no errors.
- Scene check: across the 60 s run with 8 switches, every screenshot showed all four sources
  drawn correctly.
- `GetStats` (the user's profile is 1080×1920 at 30 fps; it was not changed):
  - 5.9–9.4 ms average render time with the visualizer, against a 2.0–2.1 ms baseline with it
    disabled.
  - Skipped render frames stayed at 6/4899, all from start-up.
- No existing test suite covers this area.

## Evidence
- `evidence/SPK-005/SPK-005-obs-scene-a.png`, `SPK-005-obs-scene-b.png`: `GetSourceScreenshot` of
  the spike scene, 3 s apart. Fractopia renders with visibly different frames, so it animates. The
  image (red corner top-left, so its orientation shows), the green colour source overlapping the
  visualizer corner (z-order) and the white text all draw correctly. The canvas is portrait
  1080×1920, downscaled to 720×1280. Transparent canvas areas are shown as dark grey.
- `SPK-005-before-fix-visualizer-alone.png`: the first build on a fresh start, visualizer alone.
  It draws only a black quad, the program-cache break.
- `SPK-005-after-fix-visualizer-alone.png`: the same situation with the program guard. The
  visualizer renders correctly. This shot is from before the flip fix.
- `SPK-005-orientation-before-flip.png`, `SPK-005-orientation-after-flip.png`: the probe preset
  should show a white square top-left, black at the top grading to red at the bottom, and blue on
  the right half. Before the fix it is upside down; after `GS_FLIP_V` it is correct. The 50% row
  reads R=128, so sRGB is not applied twice.
- `SPK-005-scene-switches.png`: the `SPK-005 Other` scene, then the main scene after each of 4
  returns during the 60 s switch run.
- `SPK-005-obs-log.txt`: log excerpts covering the error spam without the guard, its partial
  self-healing, the clean runs with the guard, the switch run, and the error counts. Paths and
  the adapter name are scrubbed.
- `SPK-005-stats.txt`: the frame-time note.
- All captures come from obs-websocket `GetSourceScreenshot`, which renders the scene through
  OBS's own pipeline. I did not capture the OBS window itself, because GUI automation is not
  allowed.

## Known limitations / follow-ups
- **Flagged for SPK-006:** `gl_program_guard` in `visualizer-source.cpp` is a minimal
  save/restore of `GL_CURRENT_PROGRAM` only. OBS also caches the bound texture per unit, samplers,
  the VAO, blend state, viewport and the FBO. Nothing visibly broke from those in this test, but
  no audit has been done. SPK-006 decides between a full save/restore and a shared context, and
  should replace or extend the guard.
- 1080p60 was not measured. The user's OBS profile is 1080×1920 base, 720×1280 output, 30 fps,
  and changing it would have modified the profile. The core always renders a fixed 1920×1080
  texrender, so its cost does not depend on the canvas. The measured 4–7 ms per frame fits a
  16.7 ms budget on paper. If the user wants the number, they can set 1920×1080 at 60 fps (or let
  an agent use a dedicated spike profile) and re-sample `GetStats`.
- When the source is hidden or its scene is inactive, the accumulated delta (5 s in the test)
  goes to the core in a single `sv_render_frame` when it comes back. The core's step budget
  absorbs it with no visible problem, but Phase 2 may want to clamp or reset the delta on
  `show`/`activate`.
- Spike instrumentation:
  - The debug-report log line every 10 s (`frame …, fps …, gl_errors …`) should drop to
    `LOG_DEBUG` or go away before review.
  - The synthetic tone stands in for audio capture.
  - The `preset_path` setting has no properties UI.
- The preset is loaded inside `video_render` on the graphics thread, from a mutex-guarded path
  that `update` queues. `sv_create`/`sv_destroy` run on the calling thread under
  `obs_enter_graphics()`, as `PLAN.md` asks.
- The 10 presets and 23 textures were copied into the installed bundle by hand, in the reload
  script in the session scratchpad. No CMake install step exists. Phase 2 packaging must decide
  how presets ship, given their local-only terms.
- The sprite is drawn with blending off, so the visualizer is always opaque. Transparency or
  blend-mode support would need the core's alpha to mean something.
- The `SPK-005 Image` source in the spike collection points at a PNG in the session scratchpad.
  It will show nothing once the scratchpad is cleared. This is harmless.
- Doc gaps for document-writer:
  - `docs/architecture.md`: the render path (tick and render split, once-per-tick texrender,
    flip, sRGB handling, program guard).
  - `docs/concerns.md`: the program cache desync, the partial guard, and unaudited state.
  - `docs/integrations.md`: the preset/texture copy into the installed bundle and the ad-hoc
    re-sign after modifying the bundle.
  - `.claude/common-issues/`: "OBS GL errors `glUniformMatrix4fv … GL_INVALID_OPERATION` after
    core calls = program-cache desync".

## Handoff for downstream review
- Acceptance criteria addressed:
  - The visualizer plus image, text and colour sources draw correctly for more than 60 s and
    through scene switches.
  - The artefacts seen before the guard are recorded with screenshots and logs, not hidden.
  - The orientation fix is recorded.
  - The `PLAN.md` answer is written.
  - The `core linked` line is removed.
- Areas to scrutinize:
  - `render_core_frame`: whether `gs_texrender_reset` before every pending tick is right, given
    that `video_render` can be called several times per tick.
  - That `gl_program_guard` restores after `gs_texrender_end`. It is scope-based, so the restore
    happens at function exit.
  - The sRGB handling against `obs_source_draw`.
  - Opaque drawing via `gs_blend_state_push` + `gs_enable_blending(false)`.
  - The `static` sample buffer in `push_synthetic_tone`, which is shared by all instances on the
    graphics thread.
  - `sv_status gl_status` stays `SV_ERR_GL` if `sv_init_gl` is never reached. It always is today.
- How to run:
  1. Build as in SPK-003.
  2. Quit OBS.
  3. `ditto` the bundle into the user plugin folder.
  4. Copy `external/synesthesia-vibe/resources/mvp-presets/*.milk` into
     `Contents/Resources/presets/` and `resources/beatdrop/textures/*` into
     `Contents/Resources/textures/` of the installed bundle.
  5. Run `codesign --force --sign -` on the installed bundle.
  6. Relaunch OBS and add the source.
