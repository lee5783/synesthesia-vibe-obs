# Plugin render frame — from OBS tick to the drawn sprite

> Part of the how-x-works corpus — see [`../HOW-X-WORKS.md`](../HOW-X-WORKS.md) for the index.
> Layer overview and the thread table: `docs/architecture.md`. Limits: `docs/concerns.md`.
> Feeding flows: [`plugin-audio-path.md`](plugin-audio-path.md),
> [`plugin-preset-loading.md`](plugin-preset-loading.md).

How one OBS frame turns into a SynesthesiaVibe frame inside OBS's scene (macOS, OpenGL renderer).
All citations are `plugin/src/visualizer-source.cpp` unless noted.

## 0. Setup (once per source)

- `visualizer_create` (`:1098-1142`):
  1. `configure_output_size` (`:925-933`) stores width, height (clamped 128–8192) and render scale
     (clamped 50–100) in atomics; `core_target_size` (`:913-918`) gives the scaled size.
  2. `obs_enter_graphics()`, then inside `with_core_gl_state`: `sv_init_gl(opengl_get_proc)` and
     `sv_create(&config)` at the scaled size (resource path = the bundle's data dir, cache =
     `obs_module_config_path("cache")`). A `GS_RGBA` texrender is created, then
     `obs_leave_graphics()`.
  3. On success: the preset worker (`os_task_queue_create`), `core_size`, and
     `sv_set_preset_request_callback(…, preset_requested, …)`.
  4. `visualizer_update` (`:1074-1082`) and `register_hotkeys` (`:597-610`).
- `opengl_get_proc` lives in the per-OS loader (`plugin/src/opengl-loader-macos.cpp:18-22`,
  `plugin/src/opengl-loader-linux.cpp:54-58`). On macOS it is `dlsym` on the OpenGL framework
  opened `RTLD_LOCAL`. On Linux it is `eglGetProcAddress`, falling back to `dlsym` on
  `libOpenGL.so.0`, then `libGL.so.1` (`docs/architecture.md`, *Core GL loader*).
- Failure: `context` stays null, a `core setup failed` warning is logged, and every later render
  returns early. Creating a source costs about 1.5–2.4 skipped OBS frames (compile under the
  graphics lock).

## 1. `video_tick` — record that time passed

`visualizer_video_tick` (`:1177-1185`) runs on the graphics thread **without** the graphics lock.
It only:

- adds `seconds` (as nanoseconds, skipping non-positive deltas) to the atomic
  `pending_nanoseconds` with `fetch_add`;
- sets the atomic `frame_pending`;
- calls `maintain_audio_source` (`:773-788`), which `try_lock`s `audio_mutex` and at most once a
  second resolves or re-checks the chosen audio source (never blocks the graphics thread).

No `sv_*` call happens here. For video sources libobs also runs a deferred `update` inside this
tick; `update` only copies settings and queues work.

`show` / `activate` both call `visualizer_reset_pending_delta` (`:1187-1191`), which zeroes the
delta (not the flag), so a source coming back on air continues instead of jumping.

## 2. `video_render` — render the core at most once per tick

`visualizer_video_render` (`:1223-1256`) runs with the graphics lock held, on the graphics thread
or on another thread that took the lock (obs-websocket `GetSourceScreenshot`). It returns if there
is no context or texrender. Then `frame_pending.exchange(false)` (`:1229`): only the draw that wins
the flag calls `render_core_frame` with `take_pending_seconds` (`:1193-1197`), which takes the
delta with `exchange(0)` and caps it at `max_step_seconds` = 0.1 s, dropping the excess. Later
draws in the same frame (preview, program, projectors, screenshots) reuse the texture.

The flag is taken before the delta. A tick landing between the two exchanges gives this render
that tick's delta too and the next render about 0 s: a repeat frame, never lost or doubled time.

## 3. `render_core_frame` — core into the texrender FBO

`render_core_frame` (`:1199-1221`), in order:

1. `apply_transition_config` (`:882-904`): if `transition_dirty` is set under `preset_mutex`, copy
   cycle / blend / random and call `sv_set_transition_config` inside `with_core_gl_state`
   (`preset_duration_rand` 0, hard cuts off). Logs `auto-cycle: every … s, blend … s, … order` or
   `auto-cycle: off, …`.
2. `commit_ready_preset` (`:846-880`): under `preset_mutex`, take the `ready_preset` handle, its
   path and blend, and clear the cycle latch mirror if there was one. Outside the lock,
   `sv_commit_preset(ctx, prepared, blend)` inside the guard, then free the handle. Success →
   `remember_committed_preset` (`:835-844`, history of 50) and `preset committed: … (blend …, …
   ms)`. Failure → `preset failed to commit` and `preset_target` cleared.
3. `resize_core` (`:935-952`): if the scaled target differs from `core_size`, `sv_resize` inside
   the guard and log `core resized to …`. It runs before `gs_texrender_begin`, so the guard
   restores OBS's target and the texrender then binds at the new size.
4. `gs_texrender_reset` → `gs_texrender_begin(texrender, core_width, core_height)` at the
   **scaled** size; returns on failure (`:1205-1207`).
5. Save and disable framebuffer sRGB: the core writes display-encoded values (`:1209-1210`).
6. Read `GL_DRAW_FRAMEBUFFER_BINDING`, the texrender's FBO (`:1212-1213`).
7. `push_captured_audio` (`:818-833`): drain the newest ≤ 2304 frames from the ring into
   `sv_push_pcm_float`, or push one block of zeros after 0.5 s without audio. Outside the guard
   (no GL). Details: [`plugin-audio-path.md`](plugin-audio-path.md).
8. `sv_render_frame(ctx, seconds, fbo)` inside the guard (`:1215-1217`). If the core's auto-cycle
   timer has expired, the core calls `preset_requested` (`:513-533`) from inside this call; the
   callback only sets flags under `preset_mutex` and queues the worker
   ([`plugin-preset-loading.md`](plugin-preset-loading.md)).
9. Restore sRGB, `gs_texrender_end` (`:1219-1220`).

## 4. The GL guard around each core call

`with_core_gl_state` (`:210-214`) builds a `gl_state_guard` (`:138-208`) before the call and
destroys it after: it saves program, VAO, `GL_ARRAY_BUFFER`, draw/read FBO, viewport, blend
enable/factors/equations, the active unit and 2D/3D textures on units 0–7, and restores them (units
first, then the active unit, then the rest). This keeps `libobs-opengl`'s state cache true; without
it OBS draws with the core's program (`.claude/common-issues/plugin-runtime.md`). Scissor, colour
mask and clear colour are not saved; `sv_render_frame` and `sv_resize` both write them.

## 5. Draw — opaque flipped sprite at the source size

Back in `visualizer_video_render` (`:1232-1255`): get the texrender texture (return if none); read
the source `size`; set framebuffer sRGB to `gs_get_linear_srgb()`; push blend state, disable
blending; bind the texture to `obs_get_base_effect(OBS_EFFECT_OPAQUE)`'s `image` with
`gs_effect_set_texture_srgb` (linear sRGB) or `gs_effect_set_texture`; loop the `Draw` technique
with `gs_draw_sprite(texture, GS_FLIP_V, size.width, size.height)` (the core's origin is
bottom-left); pop blend state and restore sRGB.

Outcome: an opaque sprite (alpha forced to 1.0, so alpha-0 presets no longer show through) that
covers the full source size. A render scale below 100 % is upscaled by the effect's linear
sampler; the scene layout does not change with the scale. `get_width` / `get_height`
(`:1167-1175`) report the source size, so the scene item follows a size change live.

## 6. Teardown

`visualizer_destroy` (`:1144-1165`), on libobs's destruction task thread:

1. `unregister_hotkeys` (`:612-618`) first, so no hotkey callback runs afterwards.
2. Under `audio_mutex`: `detach_audio_source`, `disconnect_audio_mix`.
3. `stop_preset_worker` (`:635-649`): `preset_closing`, `os_task_queue_destroy` (drains and joins;
   an in-flight prepare is dropped; up to about 0.4 s), free `ready_preset`. Before the graphics
   lock, so rendering never waits on it.
4. Under graphics: `sv_destroy` inside the guard if a context exists, destroy the texrender.

Safe when create failed.

**Analysis date:** 2026-11-23 (core `v0.7.0`, libobs 31.1.1, OBS 33.0.3).
