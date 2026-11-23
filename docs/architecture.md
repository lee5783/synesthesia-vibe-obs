# Architecture

**Analysis Date:** 2026-11-23

## Pattern Overview

**Overall:** Two thin host adapters over one external renderer. The SynesthesiaVibe core (pinned,
never edited here) does all the visual work; this repo only hosts it inside OBS.

**Key Characteristics:**
- **Browser Source page** (`browser-source/`): a static ES-module page that drives the core's web
  dist (`SynesthesiaCanvas`) in OBS's embedded Chromium. No build step, no server code.
- **Native plugin** (`plugin/`): a libobs input source that renders the core with `sv_render_frame`
  into an OBS texrender, inside OBS's own OpenGL context, and draws the result as an opaque
  sprite. It feeds the core from an OBS audio source or the audio mix, loads presets from a
  user-chosen folder on a worker thread, and auto-cycles them.
- The two deliverables share no code. Both read settings on load/update and log every decision.

## Layers

**Core (external):**
- Purpose: MilkDrop-preset rendering, audio analysis, preset transitions and the auto-cycle timer.
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
- Purpose: register the OBS source type and bridge OBS's lifecycle, threads, audio and GL state to
  the core.
- Contains:
  - `plugin/src/plugin-main.cpp`: module entry, renderer check.
  - `plugin/src/visualizer-source.cpp`: source callbacks, properties, the GL guard, the render
    path, the preset worker and auto-cycle, hotkeys, audio attachment.
  - `plugin/src/audio-ring.hpp` / `.cpp`: the single-producer / single-consumer PCM ring
    (`audio_ring`).
  - `plugin/src/opengl-loader.hpp` plus one of `opengl-loader-macos.cpp` /
    `opengl-loader-linux.cpp`: the GL include, the core's GL get-proc and the platform log line.
- Depends on: libobs (`OBS::libobs`), the imported static target `SynesthesiaVibe::Core`. On Linux
  also `SynesthesiaVibe::NsEel2`, `SynesthesiaVibe::ProjectMEval` and GLVND (`OpenGL::OpenGL`,
  `OpenGL::EGL`).

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

**Plugin module load** (`plugin/src/plugin-main.cpp:19-49`):

1. OBS creates the graphics subsystem before loading modules, so `obs_module_load` can enter the
   graphics context.
2. `renderer_is_opengl()` reads `gs_get_device_type()`. Anything other than `GS_DEVICE_OPENGL`
   logs one warning and returns without registering; `obs_module_load` still returns `true`, so
   OBS loads the module cleanly and the source is simply absent from Add Source.
3. On OpenGL, `opengl_log_platform()` logs `windowing platform: X11 (EGL)` / `Wayland (EGL)` /
   `unknown` on Linux (from `obs_get_nix_platform()`); on macOS it does nothing.
4. Then `visualizer_source_register()` (`visualizer-source.cpp:1260-1278`) registers
   `synesthesia_vibe_visualizer` (`OBS_SOURCE_VIDEO | OBS_SOURCE_CUSTOM_DRAW`), with `show` and
   `activate` both mapped to `visualizer_reset_pending_delta`.

**Plugin source creation** (`visualizer_create`, `visualizer-source.cpp:1098-1142`):

1. `configure_output_size` reads width, height and render scale; `core_target_size` gives the
   scaled core size.
2. Under `obs_enter_graphics()` and inside `with_core_gl_state`: `sv_init_gl(opengl_get_proc)` and
   `sv_create(&config)` (scaled size, resource path = the bundle's data dir, cache =
   `obs_module_config_path("cache")`). A `GS_RGBA` texrender is created in the same block.
3. On success: `os_task_queue_create()` starts the preset worker, and
   `sv_set_preset_request_callback` installs `preset_requested`. On failure: `core setup failed …`
   is logged, `context` stays null and every render returns early.
4. `visualizer_update` applies the settings (audio, auto-cycle, preset request, size), then
   `register_hotkeys` registers the four source hotkeys.

`sv_create` and the first compile run on the thread that creates the source, holding the graphics
lock, so creating a visualizer costs OBS about 1.5–2.4 skipped frames (`docs/concerns.md`).

**Plugin frame** (`visualizer-source.cpp:1177-1256`):

1. `visualizer_video_tick` (`:1177-1185`, graphics thread, no graphics lock) only adds the frame
   delta to the atomic `pending_nanoseconds` (`fetch_add`), sets the atomic `frame_pending`, and
   calls `maintain_audio_source` (a `try_lock` on `audio_mutex`, at most one check per second). It
   never calls `sv_*`.
2. `visualizer_video_render` (`:1223-1256`, graphics lock held) takes `frame_pending` with
   `exchange(false)`. Only the render that wins the flag calls `render_core_frame`, with
   `take_pending_seconds` (`:1193-1197`): `exchange(0)` on the delta, capped at
   `max_step_seconds` (0.1 s); anything above the cap is dropped. OBS may draw the source several
   times per frame (preview, program, projectors, websocket screenshots); later draws reuse the
   texture and never step the core again.
3. `render_core_frame` (`:1199-1221`), in order:
   1. `apply_transition_config` (`:882-904`): if the transition settings changed,
      `sv_set_transition_config` inside the guard (`preset_duration_rand` 0, hard cuts off).
   2. `commit_ready_preset` (`:846-880`): takes the ready prepared preset, if any, and runs
      `sv_commit_preset(ctx, prepared, blend)` inside the guard; on success
      `remember_committed_preset` appends it to the history.
   3. `resize_core` (`:935-952`): if the scaled target size changed, `sv_resize` inside the guard.
   4. `gs_texrender_reset` → `gs_texrender_begin` at the **scaled core size** (`GS_RGBA`) →
      framebuffer sRGB off → read `GL_DRAW_FRAMEBUFFER_BINDING`.
   5. `push_captured_audio` (`:818-833`) drains the ring into `sv_push_pcm_float` (not GL, so
      outside the guard).
   6. `sv_render_frame(ctx, seconds, fbo)` inside the guard. The core may call
      `preset_requested` from inside this call.
   7. Restore sRGB → `gs_texrender_end`.
4. The texture is drawn with `obs_get_base_effect(OBS_EFFECT_OPAQUE)` / `Draw` (forces alpha 1.0),
   `gs_draw_sprite(tex, GS_FLIP_V, width, height)` at the **source size** (the core renders
   bottom-left-origin; OBS treats row 0 as the top), blending off inside a blend-state push/pop,
   and the `_srgb` texture setter plus framebuffer sRGB when `gs_get_linear_srgb()` is set, as
   `obs_source_draw` does. A render scale below 100 % is upscaled by the effect's linear sampler.

`show` and `activate` zero the pending delta, so a source coming back on air (or a screenshot of
a hidden source) continues from its last frame instead of jumping by the whole off-air gap. The
0.1 s cap also slows the animation on a canvas below 10 fps.

A step-by-step trace is in `.claude/how-x-works/plugin-render-frame.md`.

**Plugin audio path** (`visualizer-source.cpp:651-833`, `plugin/src/audio-ring.cpp`):

1. The "Audio source" setting stores a source **UUID**, `""` for None, or `whole_mix`.
   `visualizer_update` → `select_audio_source` (`:790-809`) under `audio_mutex` always removes
   both producers first (detach the source, then disconnect the mix), then connects the mix if
   "Whole mix" is selected. Re-applying identical settings does nothing; a track change only
   reconnects while "Whole mix" is selected.
2. **Chosen source.** `maintain_audio_source` (`:773-788`), called from the tick, resolves the
   UUID lazily every 1 s (`resolve_audio_source`, `:745-756`), so a collection where the
   visualizer loads before its audio source attaches about 1 s later. `attach_audio_source`
   (`:716-738`) creates an `audio_resampler` from OBS's output format to 44.1 kHz float stereo,
   stores a weak reference and calls `obs_source_add_audio_capture_callback`. While attached, the
   same 1 s check (`check_attached_audio_source`, `:758-771`) detaches a removed source and logs
   `audio source gone`.
3. **Producer, chosen source.** `audio_capture` (`:651-664`) runs on that source's audio thread
   under libobs's `audio_cb_mutex`. It resamples and writes the ring; a muted source writes
   zeros (`write_silence`). It never calls `sv_*` and never takes a plugin mutex.
4. **Producer, whole mix.** `connect_audio_mix` (`:677-687`) calls `audio_output_connect` on
   `obs_get_audio()` for the chosen track, asking libobs for 44.1 kHz float stereo.
   `mix_capture` (`:666-670`) runs on the audio output thread under libobs's `input_mutex` and
   only writes the ring. The mix is the stream mix of that track: muted and "Monitor only" sources
   are not in it.
5. **Ring.** `audio_ring` (`plugin/src/audio-ring.cpp`) holds 65,536 interleaved stereo frames
   (power of two, masked indices). `store` (`:42-62`) drops new frames when full;
   `read_newest` (`:64-82`) copies at most the newest `max_frames` and marks everything read.
   Each side loads its own index relaxed and the other side's with acquire, and copies before its
   release store.
6. **Consumer.** `push_captured_audio` (`:818-833`) in `render_core_frame` reads the newest 2304
   frames (`core_audio_frames`, what the core keeps) and calls `sv_push_pcm_float`. After 0.5 s
   with no captured frames (`audio_silence_after_ns`) it pushes one block of 2304 zeros, because
   the core never expires stale PCM through its public API. That covers None, a lost source and a
   paused or stopped source.

Exactly one producer is registered at a time: every attach, detach, connect and disconnect runs
under the per-visualizer `audio_mutex`, and `obs_source_remove_audio_capture_callback` /
`audio_output_disconnect` take the same libobs mutex the callback runs under, so the callback is
not running once they return.

**Plugin preset path** (`visualizer-source.cpp:221-533`):

1. `visualizer_update` → `request_preset` (`:494-511`) copies `preset_folder` and `preset` under
   `preset_mutex`, bumps `preset_generation` and queues `prepare_requested_preset` on the worker.
   Identical settings are a no-op; a folder change clears the history. No file I/O happens here.
2. `prepare_requested_preset` (`:422-492`, worker) takes the latest request with
   `take_preset_job` (`:317-342`), so a burst of requests coalesces into one prepare. It lists
   the folder (`list_preset_files`, `:227-243`: non-hidden `.milk` files, case-insensitive sort)
   and resolves the file: `""` or a file not in the folder means the first file
   (`listed_or_first`, `:256-259`). An empty, missing or unset folder logs one warning
   (`log_missing_presets`) and keeps the committed preset rendering.
3. `sv_prepare_preset` runs on the worker. On success the handle goes into the `ready_preset`
   slot with its path and blend, unless a newer generation arrived (then it is freed and logged
   `superseded, dropped`). `preset_target` (the last path handed to render) makes re-applying
   the same effective preset a no-op.
4. The next `render_core_frame` commits it (`commit_ready_preset`). The first preset a source
   shows, and any pick after a failed commit, commits with blend 0; manual picks otherwise use
   the "Blend time" setting.

**Plugin auto-cycle** (core timer, plugin choice):

1. `configure_auto_cycle` (`:620-633`) stores cycle seconds, blend seconds and random order and
   marks `transition_dirty`; `apply_transition_config` hands them to the core on the next render.
2. The core asks for a preset after `blend + cycle` seconds by calling `preset_requested`
   (`:513-533`) from inside `sv_render_frame`. The callback only sets `cycle_requested` and the
   suggested blend under `preset_mutex`, logs, and queues the worker. The core latches the
   request until a load succeeds or fails.
3. The worker serves a cycle request only if no manual pick is pending
   (`cycle_request_servable`): manual picks win. A manual pick that turns out to be a no-op while
   a cycle request waits becomes the cycle job (`switch_to_cycle_job`, `:344-355`).
4. `cycle_requested` mirrors the core latch and is cleared exactly where the core clears it
   (`clear_cycle_request`, `:288-292`): a non-superseded prepare success, a prepare failure, and
   the render taking the slot. A request that arrives while the folder is empty stays latched
   until the next pick or settings change.
5. Sequential order walks the sorted list from `preset_cursor`; random order never picks the
   current target or the cursor (`random_preset_file`, `:377-397`). A broken file is skipped on the
   next request.

**Plugin hotkeys** (`visualizer-source.cpp:535-618`):

- `register_hotkeys` registers four source hotkeys (`SynesthesiaVibe.NextPreset`,
  `.PreviousPreset`, `.RandomPreset`, `.PauseAutoCycle`); OBS lists them under the source name.
- Next / previous / random go through `request_preset_command` (`:535-546`): set
  `pending_command`, bump the generation, queue the worker. They reuse the manual-pick path, so
  every preset-path guarantee applies. Previous walks `preset_history` (last 50 commits,
  `max_preset_history`), then falls back to sorted order.
- Pause (`toggle_auto_cycle_pause`, `:548-571`) **defers** core requests instead of changing the
  transition config: while paused, `preset_requested` sets `cycle_deferred` and queues nothing;
  resume turns a deferred request into a live one and queues it, so an overdue change happens at
  once. A zero `preset_duration` is avoided on purpose: a manual pick while paused would arm the
  core's disabled-timer default (16 s plus up to 10 s random). Pause is per source and not saved.
- Hotkeys also act on hidden sources; the prepared preset commits on that source's next render.

**Plugin resize and render scale:**
- `configure_output_size` (`:925-933`) clamps width and height to 128–8192 and the render scale to
  50–100 into atomics (`size`, `render_scale`). `get_width` / `get_height` report the size, so
  the scene item follows live.
- `resize_core` calls `sv_resize` with the scaled size (`scaled_dimension`, `:906-911`) on the
  render thread, inside the guard and before `gs_texrender_begin`. The sprite always covers the
  full source size, so the layout never changes with the scale.

**Plugin teardown** (`visualizer_destroy`, `:1144-1165`, libobs destruction task thread):

1. `unregister_hotkeys` first: libobs runs hotkey callbacks under its hotkeys mutex, and
   unregister takes that mutex, so no callback runs afterwards. libobs's own unregister happens
   only after `info.destroy`.
2. Under `audio_mutex`: detach the source, disconnect the mix.
3. `stop_preset_worker` (`:635-649`): set `preset_closing`, `os_task_queue_destroy` (drains and
   joins; an in-flight prepare finishes and is dropped, up to about 0.4 s for a pathological
   preset), free `ready_preset`. This happens before `obs_enter_graphics`, so the wait never holds
   the graphics lock.
4. Under graphics: `sv_destroy` inside the guard, destroy the texrender.

**State Management:**
- Page: all state in closures inside `main` (playlist index, settings). Nothing persists.
- Plugin: one `visualizer_source` per instance (`visualizer-source.cpp:90-136`):
  - render-thread only: `context`, `texrender`, `core_size`;
  - atomics: `size`, `render_scale`, `pending_nanoseconds`, `frame_pending`;
  - under `preset_mutex`: folder/file request and generations, `preset_target`, the
    `ready_preset` slot, the cycle latch mirror and pause flags, transition settings,
    `pending_command`, `preset_history`;
  - worker-only: `preset_cursor`, `preset_random`;
  - under `audio_mutex`: the chosen UUID, mix index and connection, weak source, resampler;
  - the SPSC `audio_ring` between one producer and the render thread.

## Threading

One `sv_context` is single-threaded and needs the GL context current. Only `sv_prepare_preset*`
/ `sv_validate_preset*` may run elsewhere. The plugin maps OBS's threads onto that rule:

| Thread | Runs | Touches |
|---|---|---|
| Render (`video_render`, graphics lock held; the graphics thread, or a websocket thread for screenshots) | `render_core_frame`, the preset-request callback | every GL `sv_*` call, `sv_push_pcm_float`, the ring's consumer side |
| Graphics thread (`video_tick`, no graphics lock) | tick; the **deferred `update`** for video sources | atomics; `try_lock` on `audio_mutex`; `update` copies settings under the mutexes and queues the worker |
| Preset worker (`os_task_queue`) | `prepare_requested_preset` | folder scan, `sv_prepare_preset`, the ready slot under `preset_mutex` |
| Source audio thread / audio output thread | `audio_capture` / `mix_capture` | the resampler and the ring's producer side only |
| Hotkey thread | the four hotkey callbacks | `preset_mutex`, queue the worker |
| UI thread | `create` (calls `update` directly), `get_properties` and its modified callbacks | `create` holds the graphics lock for `sv_create`; `get_properties` reads the UUID under `audio_mutex` and lists the folder |
| libobs destruction task thread | `destroy` | as in *Plugin teardown* |

For video sources libobs defers `update` into `obs_source_video_tick`, so a settings change runs
`update` on the graphics thread, not the UI thread. Nothing in `update` depends on which thread
it runs on.

**Locks:** `preset_mutex` and `audio_mutex` are never nested and never held across
`sv_prepare_preset`, `sv_commit_preset`, `sv_render_frame` or a free. The lock order is always
`audio_mutex` → libobs (`audio_cb_mutex`, `input_mutex`, `sources_mutex`); the audio callbacks
take none of the plugin's mutexes.

## Key Abstractions

**`gl_state_guard` / `with_core_gl_state`** (`plugin/src/visualizer-source.cpp:138-214`):
- Purpose: OBS's `libobs-opengl` caches GL state (current program, FBO, viewport, blend state,
  textures per unit) and skips redundant binds. The core changes that state. Every core GL call
  is therefore wrapped: `with_core_gl_state([&] { sv_…; })`.
- Saves and restores: program, VAO (and its element buffer), `GL_ARRAY_BUFFER`, draw and read
  framebuffer, viewport, blend enable / factors / equations, the active texture unit, and the
  2D/3D texture bindings on units `0 … GS_MAX_TEXTURES-1` (0–7). Restore order: units, then the
  active unit, then the rest. The constructor re-activates the host's unit before the core runs.
- Wrapped calls: `sv_init_gl` + `sv_create`, `sv_set_transition_config`, `sv_commit_preset`,
  `sv_resize`, `sv_render_frame`, `sv_destroy`. Not wrapped: `sv_push_pcm_float` (no GL), OBS's own
  `gs_texrender_begin` / `end`.
- Not guarded: scissor, colour mask and clear colour. `sv_render_frame` and `sv_resize` both write
  them (scissor off, colour mask all-true); this is no net change only because OBS's GL device
  sets them on each use (`docs/concerns.md`).
- Rule: **any new `sv_*` call that touches GL goes through `with_core_gl_state`.** Re-run the GL
  state audit when the core is bumped, and extend the guard if the core starts changing new state.
  The audit table is in `PLAN.md`, *GL state*. Note on the SPK-006 evidence it came from: the
  evidence file records the active unit as `GL_TEXTURE7 → GL_TEXTURE0`; the 7 was an artifact of
  the guard at the time, not OBS. The core always leaves `GL_TEXTURE0` active, and the starting
  unit is whatever the host had active.

**`audio_ring`** (`plugin/src/audio-ring.hpp`, `plugin/src/audio-ring.cpp`):
- Lock-free SPSC ring of interleaved stereo floats (`channels = 2`), capacity rounded up to a
  power of two. `write`, `write_silence` (producer), `read_newest` (consumer). Non-copyable. Only
  one producer may be registered at a time; the plugin enforces this with `audio_mutex`.

**Preset worker and ready slot** (`take_preset_job`, `prepare_requested_preset`,
`commit_ready_preset`):
- A generation counter turns every pick (setting, hotkey) into "prepare the latest request";
  the worker prepares, the render thread commits. Use this path for any new way of changing
  the preset: bump the generation under `preset_mutex` and queue `prepare_requested_preset`.

**Core GL loader** (`opengl_get_proc`, one definition per OS):
- `plugin/src/opengl-loader.hpp` is the shared header and the only platform `#if` in
  `plugin/src/`. It pulls in `<OpenGL/gl3.h>` on Apple, otherwise `<GL/gl.h>` + `<GL/glext.h>`
  (with `GL_GLEXT_PROTOTYPES`, so `gl_state_guard`'s direct `gl*` calls get prototypes and resolve
  against GLVND `libOpenGL.so.0` at link time). `plugin/CMakeLists.txt` compiles exactly one of
  the two loader files.
- macOS (`opengl-loader-macos.cpp`): `dlsym` on `dlopen(".../OpenGL.framework/OpenGL",
  RTLD_LAZY | RTLD_LOCAL)`, opened once in a function-local static. `opengl_log_platform` is
  empty.
- Linux (`opengl-loader-linux.cpp`):
  - `eglGetProcAddress` first. Recent OBS creates its GL context through EGL on both X11 and
    Wayland, and Mesa and NVIDIA expose core functions there (`EGL_KHR_get_all_proc_addresses`).
  - Then `dlsym` on `libOpenGL.so.0`, then on `libGL.so.1`. Both handles are opened once, in a
    thread-safe static initializer, and never closed.
  - GLVND may hand back stubs from `eglGetProcAddress`. That does no harm, because the core loads
    through glad 2 (`gladLoadGLUserPtr`), which picks functions from the version and extension
    strings.
  - `EGL_NO_X11` keeps Xlib out of `<EGL/eglplatform.h>`.
  - The `libGL.so.1` fallback is unreachable in practice: the `.so` already has
    `NEEDED libOpenGL.so.0` (`docs/concerns.md`).
- Call path: `sv_init_gl(opengl_get_proc)` → `render_gl::LoadGlFunctions` → `gladLoadGLUserPtr`.
  The core's glad pointers stay private to the plugin on both OSes.

**Export lists** (`plugin/exported-symbols.txt` on macOS, `plugin/exported-symbols.map` on Linux):
- The plugin exports exactly its 7 `obs_module_*` entry points. This hides the core's glad, eval
  and C++ weak definitions, which dyld would otherwise coalesce across every loaded image, and
  which on Linux would clash with OBS's own glad (the *Duplicate glad symbols* risk in `PLAN.md`).
- macOS: Xcode's `EXPORTED_SYMBOLS_FILE` (names with a leading `_`).
- Linux: `-Wl,--version-script` (`global:` the 7 names, `local: *;`), plus
  `-Wl,--exclude-libs,ALL` so nothing from the static archives is exported, plus
  `-Wl,--no-undefined`, which turns a missing library into a link error instead of a load failure
  inside OBS. CI confirms that `nm -D --defined-only` shows only the 7 names and the
  linker-reserved `_init`, `_fini`, `_edata`, `_end` and `__bss_start`.

**Core link on Linux** (`plugin/CMakeLists.txt`, Linux branch):
- On Apple the core's own build merges projectM-Eval and ns-eel2 into `libSynesthesiaVibeCore.a`
  (`xcrun libtool`). On Linux it does not: the core installs `libprojectM_ns-eel2.a` and
  `libprojectM_eval.a` next to it, and the plugin links all three.
- The archives depend on each other in a cycle. The core needs ns-eel2, ns-eel2 needs eval, and
  ns-eel2 also needs `NSEEL_HOSTSTUB_EnterMutex` / `LeaveMutex`. Those are defined in the core
  (`external/synesthesia-vibe/src/platform/nseel_hoststub.cpp`, merged into the core archive).
- GNU ld scans a plain archive list once, left to right. When it scans the core, nothing has asked
  for the stubs yet, so it skips that object. ns-eel2 then needs them, and ld never goes back:
  `undefined reference to 'NSEEL_HOSTSTUB_EnterMutex'`. Apple's ld64 resolves within one merged
  archive, so the problem never shows on macOS.
- The fix: the two archives are imported static targets (`SynesthesiaVibe::NsEel2`,
  `SynesthesiaVibe::ProjectMEval`), linked as
  `$<LINK_GROUP:RESCAN,SynesthesiaVibe::Core,SynesthesiaVibe::NsEel2,SynesthesiaVibe::ProjectMEval>`.
  On GNU that is `-Wl,--start-group … -Wl,--end-group`, which rescans until nothing new resolves.
  It needs CMake ≥ 3.24; the minimum here is 3.28.
- CMake replaces every non-group use of a library that belongs to a group with the group. So the
  shared `target_link_libraries(… PRIVATE SynesthesiaVibe::Core)` stays.
- Reordering the list cannot express a cycle, and listing the core twice depends on CMake not
  de-duplicating it (CMP0156). The group names the cycle explicitly.
- The core is also built with `-DCMAKE_POSITION_INDEPENDENT_CODE=ON`, because a `.so` on x86_64
  cannot take non-PIC objects. CI rejects any `TEXTREL`.

**Properties** (`visualizer_get_properties`, `visualizer-source.cpp:1000-1072`):
- Four `OBS_GROUP_NORMAL` groups in this order: **Presets** (folder path, preset list, no-folder
  hint), **Transitions** (cycle seconds, blend seconds, random order), **Audio** (source list,
  mix track list), **Output** (width, height, render scale list). Defaults for every key are in
  `visualizer_get_defaults` (`:1084-1096`).
- `preset_folder_modified` (`:984-998`) rebuilds the preset list: "(first preset)" with value
  `""`, then one entry per file. With no files it hides the list and shows the hint.
  `audio_source_modified` (`:977-982`) shows "Mix track" only for "Whole mix". A stored audio UUID
  that no longer resolves is listed as "(missing)". libobs passes the topmost properties to
  modified callbacks, and `obs_properties_get` searches groups recursively.

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
- Triggers: OBS's module scan of `~/Library/Application Support/obs-studio/plugins/` (macOS) or
  `/usr/lib/x86_64-linux-gnu/obs-plugins/` (Linux).

**Source callbacks:**
- Location: `visualizer_source_register` (`visualizer-source.cpp:1260-1278`): `get_name`,
  `create`, `destroy`, `update`, `get_defaults`, `get_properties`, `get_width`, `get_height`,
  `video_tick`, `video_render`, `show`, `activate`.

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
- Plugin:
  - A failed `sv_init_gl` / `sv_create` logs `core setup failed (…)` and leaves `context` null;
    `video_render` then returns early. Destroy is safe when create failed.
  - A failed prepare logs `preset failed to prepare: <path> (<core error>)` and the current preset
    keeps rendering. A failed commit logs `preset failed to commit: …` and clears `preset_target`.
  - A missing audio source logs once, then retries every second; a resampler or mix-connect
    failure logs a warning and leaves the visualizer silent.
- `fetch-dist.sh`: `die` prints one `fetch-dist: …` line and exits 1; the old `dist/` stays intact.

## Cross-Cutting Concerns

**Logging:**
- Page: `console.info` / `warn` / `error`, every line prefixed `obs-page:` (`LOG_PREFIX`). One
  `obs-page: settings …` line summarises what the page understood.
- Plugin: `obs_log` (template `plugin-support.h`), prefixed with the plugin name. One line per
  event (module load, source creation, preset prepared/committed, auto-cycle request, hotkey,
  audio attach/detach, resize); nothing per frame. Lines do not name the source instance.

**Validation:**
- Page: user text from `preset=` / `audio=` only **selects** among manifest paths or device
  labels. It is never inserted into the DOM and never passed raw to `loadPreset`.
- Plugin: the folder path only selects files through `os_opendir`; it reaches only
  `sv_prepare_preset` and the log.

**Licensing boundary:**
- The plugin links libobs, so the repo is GPL-2.0-or-later; the MIT core and its notices keep
  their own licence (`docs/decisions.md`).

---

*Architecture analysis: 2026-11-23*
*Update when major patterns change*
