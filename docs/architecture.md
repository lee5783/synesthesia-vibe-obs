# Architecture

**Analysis Date:** 2026-11-23

## Pattern Overview

**Overall:** Two thin host adapters over one external renderer. The SynesthesiaVibe core (pinned,
never edited here) does all the visual work; this repo only hosts it inside OBS.

**Key Characteristics:**
- **Browser Source page** (`browser-source/`): a static ES-module page that drives the core's web
  dist (`SynesthesiaCanvas`) in OBS's embedded Chromium. No build step, no server code.
- **Native plugin** (`plugin/`): a libobs input source that renders the core with `sv_render_frame`
  into an OBS texrender, inside OBS's own OpenGL context, and draws the result as a sprite.
- The two deliverables share no code. Both read settings once per load/update and log every
  decision.

## Layers

**Core (external):**
- Purpose: MilkDrop-preset rendering, audio analysis, preset transitions.
- Location: `external/synesthesia-vibe/` (C API `include/synesthesia-vibe/synesthesia-vibe.h`) and
  the downloaded web dist in `browser-source/dist/`.
- Depends on: OpenGL / WebGL 2.
- Used by: both adapters. Changes go upstream as `synesthesia-vibe#<n>`.

**Browser Source adapter:**
- Purpose: turn URL parameters into a playlist, a renderer and an audio input.
- Contains: `browser-source/obs-page.js` (all logic), `browser-source/index.html` (canvas
  `#visualizer`, black body), `browser-source/fetch-dist.sh` (installs the dist and resources).
- Depends on: `./dist/synesthesia-vibe.js` and `./dist/resources/manifest.json`.

**Plugin adapter:**
- Purpose: register the OBS source type and bridge OBS's lifecycle, threads and GL state to the
  core.
- Contains: `plugin/src/plugin-main.cpp` (module entry, renderer check),
  `plugin/src/visualizer-source.cpp` (source callbacks, GL guard, render path).
- Depends on: libobs (`OBS::libobs`), the imported static target `SynesthesiaVibe::Core`.

## Data Flow

**Browser Source startup** (`main`, `browser-source/obs-page.js:307-321`):

1. `readSettings(location.search)` parses `preset`, `cycle` (≥ 0, default 30), `blend` (clamped
   0–10, default 2), `fps` (≥ 0, default 60, `0` = uncapped) and `audio` (`default`, `none` or a
   label). Bad values warn and fall back.
2. `importDist()` dynamically imports `./dist/synesthesia-vibe.js`. A dynamic import (not a static
   one) lets a missing `dist/` produce the readable `could not load …, run
   browser-source/fetch-dist.sh` error.
3. `loadSynesthesiaModule()`, then `ResourceLoader.load(manifest.json)` → preset paths.
4. `resolvePresets` maps the `preset=` list onto manifest paths (see *Validation*). One preset
   forces `cycle` to 0.
5. `SynesthesiaCanvas.create(canvas, module, { configuration: { maxFps } })`.
6. `startCycling` sets `presetDuration: cycle`, `softCutSeconds: blend`, no hard cuts, random blend
   pattern, and answers `onPresetRequest` with the next playlist entry. The engine timer drives
   cycling, so presets change every `cycle + blend` seconds.
7. `loadNext(0)` loads the first preset with no blend. `createPlaylist` tries each entry once per
   request and throws `no preset in the list could be loaded` only after a full failed pass.
8. `startAudio` runs only after the first preset: `audio=none` skips it; otherwise a
   `getUserMedia` call unlocks device labels, `chooseDevice` picks the input, the page waits for a
   click or key only if a probe `AudioContext` starts suspended (OBS 33's CEF starts `running`),
   then `AudioInput.fromMicrophone`. A 1 s `isRunning` poll reports an unplugged device once; the
   page never retries.

A step-by-step trace is in `.claude/how-x-works/browser-source-startup.md`.

**Plugin module load** (`plugin/src/plugin-main.cpp:18-47`):

1. OBS creates the graphics subsystem before loading modules, so `obs_module_load` can enter the
   graphics context.
2. `renderer_is_opengl()` reads `gs_get_device_type()`. Anything other than `GS_DEVICE_OPENGL`
   logs one warning and returns without registering; `obs_module_load` still returns `true`, so
   OBS loads the module cleanly and the source is simply absent from Add Source.
3. On OpenGL, `visualizer_source_register()` registers `synesthesia_vibe_visualizer`
   (`OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW`).

**Plugin frame** (`plugin/src/visualizer-source.cpp:263-326`):

1. `video_tick` (graphics thread, no graphics lock) adds the frame delta to `pending_seconds`,
   sets `frame_pending`, and pushes a synthetic tone with `sv_push_pcm_float` (spike stand-in for
   audio capture).
2. `video_render` (graphics context held) calls `render_core_frame` only when `frame_pending` is
   set, so the core renders **once per tick** even though OBS may draw the source several times
   per frame (preview, program, projectors, screenshots).
3. `render_core_frame` loads any queued preset, then `gs_texrender_reset` → `gs_texrender_begin`
   (1920×1080, `GS_RGBA`) → framebuffer sRGB off → read `GL_DRAW_FRAMEBUFFER_BINDING` →
   `sv_render_frame(ctx, pending_seconds, fbo)` inside the GL guard → restore sRGB →
   `gs_texrender_end`, and resets the pending delta.
4. The texture is drawn with `OBS_EFFECT_DEFAULT` / `Draw`, `gs_draw_sprite(tex, GS_FLIP_V, 0, 0)`
   (the core renders bottom-left-origin; OBS treats row 0 as the top), blending off inside a
   blend-state push/pop, and the `_srgb` texture setter plus framebuffer sRGB when
   `gs_get_linear_srgb()` is set, as `obs_source_draw` does.

A step-by-step trace is in `.claude/how-x-works/plugin-render-frame.md`.

**State Management:**
- Page: all state in closures inside `main` (playlist index, settings). Nothing persists.
- Plugin: one `visualizer_source` per instance (`visualizer-source.cpp:35-44`): the
  `sv_context`, the texrender, the pending delta and flag, tone phase, and a mutex-guarded
  `pending_preset` path that `update` writes and the render path consumes.

## Key Abstractions

**`gl_state_guard` / `with_core_gl_state`** (`plugin/src/visualizer-source.cpp:46-122`):
- Purpose: OBS's `libobs-opengl` caches GL state (current program, FBO, viewport, blend state,
  textures per unit) and skips redundant binds. The core changes that state. Every core GL call
  is therefore wrapped: `with_core_gl_state([&] { sv_…; })`.
- Saves and restores: program, VAO (and its element buffer), `GL_ARRAY_BUFFER`, draw and read
  framebuffer, viewport, blend enable / factors / equations, the active texture unit, and the
  2D/3D texture bindings on units `0 … GS_MAX_TEXTURES-1` (0–7). Restore order: units, then the
  active unit, then the rest. The constructor re-activates the host's unit before the core runs.
- Wrapped calls: `sv_init_gl` + `sv_create`, `sv_load_preset`, `sv_render_frame`, `sv_destroy`.
  Not wrapped: OBS's own `gs_texrender_begin` / `end`.
- Rule: **any new `sv_*` call that touches GL goes through `with_core_gl_state`.** Re-run the GL
  state audit when the core is bumped, and extend the guard if the core starts changing new state.
  The audit table is in `PLAN.md`, *GL state*. Note on the SPK-006 evidence it came from: the
  evidence file records the active unit as `GL_TEXTURE7 → GL_TEXTURE0`; the 7 was an artifact of
  the guard at the time, not OBS. The core always leaves `GL_TEXTURE0` active, and the starting
  unit is whatever the host had active.

**Core GL loader** (`opengl_get_proc`, `visualizer-source.cpp:124-128`):
- `sv_init_gl` gets a get-proc that is `dlsym` on `dlopen(".../OpenGL.framework/OpenGL",
  RTLD_LAZY | RTLD_LOCAL)`. The core's glad pointers stay private to the plugin.

**Export list** (`plugin/exported-symbols.txt`):
- The plugin exports exactly its 7 `obs_module_*` entry points. This hides the core's glad, eval
  and C++ weak definitions, which dyld would otherwise coalesce across every loaded image.

**Playlist / matcher** (`browser-source/obs-page.js:116-189`):
- `matchKey` normalises (lowercase, `+` → space, collapse spaces). `findMatch` takes an exact key,
  else the first substring match with a warning if several match. Shared by `preset=` and `audio=`.

## Entry Points

**Browser Source page:**
- Location: `browser-source/index.html` → `<script type="module" src="./obs-page.js">`.
- Triggers: OBS (or a browser) loading `http://localhost:8080/?…`. A URL change reloads the page.
- Responsibilities: `main()`; any rejection reaches `main().catch(reportError)`.

**Plugin module:**
- Location: `plugin/src/plugin-main.cpp` (`OBS_DECLARE_MODULE`, `obs_module_load`,
  `obs_module_unload`).
- Triggers: OBS's module scan of `~/Library/Application Support/obs-studio/plugins/`.

**Source callbacks:**
- Location: `visualizer_source_register` (`visualizer-source.cpp:330-345`).
- `create` / `destroy` run `sv_*` under `obs_enter_graphics()` (UI thread); `update` only queues
  the preset path; `video_tick` / `video_render` as above.

**Dist installer:**
- Location: `browser-source/fetch-dist.sh [version]`. Downloads, verifies, stages into
  `browser-source/.dist-stage.*`, then swaps `dist/` with two renames; a trap restores the old
  `dist/` on failure.

## Error Handling

**Strategy:** degrade, log, keep rendering.

**Patterns:**
- Page: `stage(description, action)` (`obs-page.js:88-94`) rethrows any failure as
  `new Error(description, { cause })`, so every user-facing error names the step that failed.
  Fatal steps propagate to `main().catch(reportError)`; preset failures are caught per entry in
  the playlist; audio failures leave visuals running silent.
- Plugin: a failed `sv_init_gl` / `sv_create` logs `core setup failed (…)` and leaves `context`
  null; `video_render` then returns early. A failed preset load logs a warning with
  `sv_last_error`. Destroy is safe when create failed.
- `fetch-dist.sh`: `die` prints one `fetch-dist: …` line and exits 1; the old `dist/` stays intact.

## Cross-Cutting Concerns

**Logging:**
- Page: `console.info` / `warn` / `error`, every line prefixed `obs-page:` (`LOG_PREFIX`). One
  `obs-page: settings …` line summarises what the page understood.
- Plugin: `obs_log` (template `plugin-support.h`), prefixed with the plugin name. One line per
  module load, source creation and preset load; nothing per frame.

**Validation:**
- User text from `preset=` / `audio=` only **selects** among manifest paths or device labels. It is
  never inserted into the DOM and never passed raw to `loadPreset`.

**Threading:**
- One `sv_context` is single-threaded and needs the GL context current. OBS calls a source from
  the UI, graphics and audio threads. Only `sv_prepare_preset*` / `sv_validate_preset*` may run
  elsewhere. The spike's `video_tick` breaks this rule against an off-thread `video_render`
  (see `docs/concerns.md`); the Phase 2 design moves every `sv_*` call into `video_render`, with
  the tick and audio sides writing only atomics or a lock-free ring.

**Licensing boundary:**
- The plugin links libobs, so the repo is GPL-2.0-or-later; the MIT core and its notices keep
  their own licence (`docs/decisions.md`).

---

*Architecture analysis: 2026-11-23*
*Update when major patterns change*
