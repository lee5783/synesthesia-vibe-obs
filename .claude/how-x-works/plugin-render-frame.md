# Plugin render frame — from OBS tick to the drawn sprite

> Part of the how-x-works corpus — see [`../HOW-X-WORKS.md`](../HOW-X-WORKS.md) for the index.
> Layer overview: `docs/architecture.md`. Stand-ins and the open race: `docs/concerns.md`.

How one OBS frame turns into a SynesthesiaVibe frame inside OBS's scene (spike state, macOS,
OpenGL renderer). All citations are `plugin/src/visualizer-source.cpp` unless noted.

## 0. Setup (once per source)

- `visualizer_create` (`:202-238`) enters OBS's graphics context, then inside
  `with_core_gl_state` calls `sv_init_gl(opengl_get_proc)` and `sv_create(&config)` (1920×1080,
  resource path = the bundle's data dir, cache = `obs_module_config_path("cache")`). It creates a
  `GS_RGBA` texrender, leaves the context, and queues the preset path through `visualizer_update`.
- `opengl_get_proc` (`:124-128`) is `dlsym` on the OpenGL framework opened `RTLD_LOCAL`.
- Failure: `context` stays null, a `core setup failed` warning is logged, and every later render
  returns early.

## 1. `video_tick` — accumulate

`visualizer_video_tick` (`:263-269`) runs on the graphics thread **without** the graphics lock:
adds `seconds` to `pending_seconds`, sets `frame_pending`, and pushes the synthetic tone via
`push_synthetic_tone` → `sv_push_pcm_float` (`:146-169`).

Edge case: this `sv_*` call can race an off-thread `video_render` (obs-websocket
`GetSourceScreenshot`). Phase 2 moves it into `video_render` (`docs/concerns.md`).

## 2. `video_render` — render the core at most once per tick

`visualizer_video_render` (`:295-326`) runs with the graphics context held. It returns if there is
no context or texrender, and calls `render_core_frame` only if `frame_pending` is set. OBS may draw
the source several times in a frame (preview, program, projectors, screenshots); only the first
draw after a tick renders the core, later ones reuse the texture.

## 3. `render_core_frame` — core into the texrender FBO

`render_core_frame` (`:271-293`):

1. `load_pending_preset` (`:171-188`) swaps out the mutex-guarded path and, if one is queued, runs
   `sv_load_preset` inside `with_core_gl_state` (synchronous; stand-in for prepare/commit).
2. `gs_texrender_reset` → `gs_texrender_begin(texrender, 1920, 1080)`; returns on failure.
3. Saves and disables framebuffer sRGB: the core writes display-encoded values.
4. Reads `GL_DRAW_FRAMEBUFFER_BINDING` — the texrender's FBO — and calls
   `sv_render_frame(ctx, pending_seconds, fbo)` inside `with_core_gl_state`.
5. Restores sRGB, `gs_texrender_end`, resets `pending_seconds` and `frame_pending`.

## 4. The GL guard around each core call

`with_core_gl_state` (`:118-122`) builds a `gl_state_guard` (`:46-116`) before the call and
destroys it after: it saves program, VAO, `GL_ARRAY_BUFFER`, draw/read FBO, viewport, blend
enable/factors/equations, the active unit and 2D/3D textures on units 0–7, and restores them (units
first, then the active unit, then the rest). This keeps `libobs-opengl`'s state cache true; without
it OBS draws with the core's program (`.claude/common-issues/plugin-runtime.md`).

## 5. Draw — flipped sprite with OBS's default effect

Back in `visualizer_video_render`: get the texrender texture; set framebuffer sRGB to
`gs_get_linear_srgb()`; push blend state, disable blending; bind the texture to
`OBS_EFFECT_DEFAULT`'s `image` with `gs_effect_set_texture_srgb` (linear sRGB) or
`gs_effect_set_texture`; loop the `Draw` technique with `gs_draw_sprite(texture, GS_FLIP_V, 0, 0)`
(the core's origin is bottom-left); pop blend state and restore sRGB.

Outcome: a 1920×1080 opaque sprite in the scene, sized by `get_width` / `get_height`. Edge case:
presets that write alpha 0 come out transparent for screenshots and filters (`docs/concerns.md`).

## 6. Teardown

`visualizer_destroy` (`:240-251`) enters graphics, calls `sv_destroy` inside the guard if a
context exists, destroys the texrender, leaves graphics. Safe when create failed.

**Analysis date:** 2026-11-23 (core `v0.7.0`, libobs 31.1.1, OBS 33.0.3).
