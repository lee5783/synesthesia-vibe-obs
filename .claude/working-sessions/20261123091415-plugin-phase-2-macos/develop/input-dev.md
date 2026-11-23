# Developer input

## P2-001 — Build guards

### Task

Plan task P2-001 from `../plan/output-plan-tasks.md`: in `plugin/CMakeLists.txt`, stop configure
early with one clear `FATAL_ERROR` unless the host is macOS, the generator is Xcode and
`CMAKE_OSX_ARCHITECTURES` is exactly `arm64`; the message points to the `macos-arm64` preset. Keep
the `.CMakeBuildNumber` seed guard. Recheck a from-scratch configure and build. P2-001b
(universal) is not selected: C-016 accepted arm64 only.

### Skills (using)

- None. The orchestrator listed no extra skills, and no installed skill matches a CMake configure
  guard (`shader-programming` is GL state; `cpp-*` are manual activation only).

### Rules (read)

- Root `CLAUDE.md`: no code comments, sibling style (the vendored template's
  `message(FATAL_ERROR "..." "...")` shape), no unit tests, public repo (no machine paths),
  never edit `plugin/cmake/**` (verbatim template) or the core submodule.
- `.claude/harness/developer.md` (role file, read in full).
- `../inputs/requirements.md`
- `../plan/output-plan-gate-1-decisions.md` (C-004 … C-029 recommended defaults accepted)
- `../plan/output-plan-summary.md`
- `../plan/output-plan-tasks.md` (P2-001 row and section)
- `../plan/output-plan-concerns.md` (C-016: arm64 only for Phase 2, universal deferred)
- `docs/structure.md`, `docs/integrations.md`, `docs/concerns.md` ("macOS / arm64 / Xcode only")
- `.claude/common-issues/plugin-build.md` (build-number seed; from-scratch recheck recipe)

### Interpretation notes

- The guard goes right after `cmake_minimum_required`, before the build-number seed and the
  template bootstrap. A refused configure then writes nothing into the source tree and does not
  start the 3-minute libobs download/build. `CMAKE_HOST_SYSTEM_NAME`, `CMAKE_GENERATOR` and a
  cache-set `CMAKE_OSX_ARCHITECTURES` are all known before `project()` (the template's
  `osconfig.cmake` already relies on `CMAKE_HOST_SYSTEM_NAME` at that point).
- An unset `CMAKE_OSX_ARCHITECTURES` is refused too ("exactly `arm64`"); the message echoes the
  detected host, generator and architectures.
- `CMakePresets.json` is not changed: the task does not ask for it.

## P2-002 — Thread-race fix, delta reset and cap

### Task

Plan task P2-002 from `../plan/output-plan-tasks.md`: move every `sv_*` call and the `dt`
consumption into `video_render` (graphics context held); `video_tick` only writes atomics that
`video_render` takes with `exchange`; the synthetic tone moves into the render path until P2-004
deletes it; reset the pending delta on `show`/`activate` and cap one `sv_render_frame` step at
0.1 s (C-011). Verify per C-009 with a scratch-only re-entrancy detector and a 5-minute
`GetSourceScreenshot` stress loop.

### Skills (using)

- `shader-programming` (delegated; the change touches the GL render path and the
  `with_core_gl_state` guard, though no shader code changes).

### Rules (read)

- Root `CLAUDE.md`: C++17, no code comments, sibling style (`visualizer-source.cpp`), no unit
  tests, one `sv_context` single-threaded with GL current, public repo (no machine paths).
- `.claude/harness/developer.md` (role file), `.claude/harness/HARNESS.md` (stuck capture).
- `../plan/output-plan-gate-1-decisions.md` (standing rules: websocket only, never edit OBS
  config, test collection then restore, C-026/C-027 defaults).
- `../plan/output-plan-summary.md`, `../plan/output-plan-tasks.md` (threading contract rules and
  the P2-002 section), `../plan/output-plan-test-cases.md` (TC-005, TC-006).
- `../plan/output-plan-concerns.md` (C-009, C-011, C-026, C-027).
- `output-P2-001.md`.
- `docs/architecture.md` (plugin frame, threading), `docs/concerns.md` (race, off-air gap,
  TSan note), `.claude/how-x-works/plugin-render-frame.md`,
  `.claude/common-issues/plugin-runtime.md`.

### Interpretation notes

- Pending delta is `std::atomic<uint64_t>` nanoseconds plus `std::atomic<bool>` frame flag.
  `video_render` exchanges the flag first, then the delta, so a second render in one frame neither
  re-renders the core nor double-counts the delta.
- `show`/`activate` reset only the delta (the flag stays), so a render right after showing steps
  the core by 0 s and redraws instead of jumping.
- The cap (0.1 s) applies to the delta passed to one `sv_render_frame`; the excess is dropped, not
  carried over.
- The re-entrancy detector lives only in a scratch build (applied as a temporary patch, then
  reverted); the final source has none of it.

## P2-003 — Opaque draw

### Task

Plan task P2-003 from `../plan/output-plan-tasks.md`: draw the visualizer texrender with
`obs_get_base_effect(OBS_EFFECT_OPAQUE)` and its `Draw` technique, so presets that write alpha 0
(swarm) are fully opaque on the canvas. Keep the sRGB handling (`_srgb` setter and framebuffer
sRGB when `gs_get_linear_srgb()` is set), `GS_FLIP_V` and the blend push/pop.

### Skills (using)

- `shader-programming` (delegated; the change swaps the effect the visualizer texture is drawn
  with — the pixel shader becomes libobs's `opaque.effect` `PSDraw`).

### Rules (read)

- Root `CLAUDE.md`: no code comments, sibling style (`visualizer-source.cpp`), no unit tests,
  public repo (no machine paths).
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-decisions.md` (standing rules: websocket only, never edit OBS
  config, test collection then restore, C-027 osascript quit).
- `../plan/output-plan-tasks.md` (P2-003 row and section), `../plan/output-plan-test-cases.md`
  (TC-007).
- `output-P2-002.md` (current render path, install/reload procedure, evidence scripts).
- `docs/architecture.md` (plugin frame step 4: sRGB handling, `GS_FLIP_V`),
  `.claude/common-issues/plugin-runtime.md`.
- libobs 31.1.1 source in `plugin/.deps`: `libobs/data/opaque.effect` (`Draw` = `PSDraw`, returns
  `float4(rgb, 1.0)`, same `image` param and sampler as `default.effect`), `obs.c`
  `obs_get_base_effect`, `obs-source.c` `obs_source_draw` (sRGB pattern the plugin mirrors).

### Interpretation notes

- Only the effect lookup changes. `opaque.effect`'s `Draw` samples `image` exactly like
  `default.effect`'s `Draw` and only forces alpha to 1.0, so colour, orientation and the sRGB path
  are untouched by construction; the screenshots check it.

## P2-004 — Audio from a chosen OBS source

### Task

Plan task P2-004 from `../plan/output-plan-tasks.md`: a lock-free SPSC ring of interleaved
stereo float frames; an "Audio source" list property (None + every source with
`OBS_SOURCE_AUDIO`, stored by UUID, name as label, "(missing)" for a stored UUID that is gone);
a weak reference resolved lazily in `video_tick`, throttled to about 1 s (compressor-filter
sidechain pattern); attach/detach with `obs_source_add/remove_audio_capture_callback`; resample
to 44.1 kHz interleaved stereo float with `audio_resampler_*` before the ring; drain the newest
≤ 2304 frames into `sv_push_pcm_float` in `video_render` right before `sv_render_frame`; muted
means silence (C-008); delete the synthetic tone. Whole mix is P2-005, presets P2-006.

### Skills (using)

- None. The orchestrator listed no extra skills. The change is audio plumbing and threading; the
  GL path is not touched (`sv_push_pcm_float` does not touch GL), so `shader-programming` does
  not apply. `cpp-*` are manual activation only.

### Rules (read)

- Root `CLAUDE.md`: C++17, no code comments, sibling style (`visualizer-source.cpp`), new file
  header, no unit tests, public repo (no machine paths), never call `sv_*` from the audio callback.
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-decisions.md` (standing rules), `../plan/output-plan-summary.md`
  (audio decisions), `../plan/output-plan-tasks.md` (P2-004 section),
  `../plan/output-plan-test-cases.md` (TC-008, TC-009, TC-010, TC-024),
  `../plan/output-plan-concerns.md` (C-005, C-006, C-007, C-008, C-010, C-019, C-020, C-024).
- `output-P2-002.md`, `output-P2-003.md` (install/reload/capture procedure, scripts).
- `docs/structure.md` (new plugin files: `plugin/src/<kebab-name>.cpp/.hpp` + `target_sources`;
  strings in `en-US.ini`), `docs/conventions.md` (logging: once per event at INFO, warnings for
  failures, nothing per frame), `docs/integrations.md` (build and `ditto` install),
  `.claude/common-issues/plugin-runtime.md`, `.claude/common-issues/verification-tooling.md`.
- libobs 31.1.1 in `plugin/.deps`: `plugins/obs-filters/compressor-filter.c` (sidechain weak ref,
  throttled resolve in tick, detach in update/destroy), `libobs/obs-source.c`
  (`source_signal_audio_data` runs callbacks under `audio_cb_mutex`; remove takes the same mutex;
  destroy frees `audio_cb_list`; `obs_source_remove` only sets `removed`, the UUID stays resolvable
  until destroy), `libobs/media-io/audio-resampler.h`.
- Core: `sv_push_pcm_float` labels samples 44.1 kHz and keeps the newest 2304
  (`SAMPLE_SIZE_LPB`), process-global (C-006).

### Interpretation notes

- New files `plugin/src/audio-ring.hpp` / `.cpp` (class `audio_ring`), 65536-frame capacity
  (~1.5 s at 44.1 kHz, power of two for masking). Producer writes what fits and drops the rest;
  consumer takes everything and keeps only the newest ≤ 2304 frames.
- One `std::mutex` per visualizer serialises attach (tick), detach (update, destroy, loss) and the
  resampler's lifetime, so at most one capture callback is ever registered: strictly single
  producer. The callback never takes that mutex (lock order: ours → `audio_cb_mutex`).
- The resampler is created per attachment from `audio_output_get_info(obs_get_audio())` and
  destroyed after detach. Muted: resample as usual, write the same number of zero frames.
- Loss detection: tick (throttled 1 s) checks the attached weak ref for expiry or `removed`, logs
  once, detaches and goes back to resolving the UUID. Resolution skips removed sources.
- No `get_defaults` yet (P2-010); the empty string means "None".
- C-010 debug-level log lives only in a scratch build, applied by a script to a copy.

## P2-005 — Audio from the whole mix

### Task

Plan task P2-005 from `../plan/output-plan-tasks.md`: add a "Whole mix" entry to the "Audio
source" list and a "Mix track" setting (1–6, default 1) shown only when "Whole mix" is selected
(modified callback). Connect with `audio_output_connect(obs_get_audio(), track - 1,
{44100, AUDIO_FORMAT_FLOAT, SPEAKERS_STEREO}, cb, data)`, the callback writing the interleaved
stereo `data[0]` straight into the P2-004 ring; disconnect with `audio_output_disconnect` on change
and in `destroy`. Only one producer at a time.

### Skills (using)

- None. The orchestrator listed no extra skills; the change is audio plumbing and settings, no GL
  state (`shader-programming` does not apply). `cpp-*` are manual activation only.

### Rules (read)

- Root `CLAUDE.md`: C++17, no code comments, sibling style (`visualizer-source.cpp`), no unit
  tests, public repo (no machine paths), never call `sv_*` from audio callbacks.
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-decisions.md` (standing rules), `../plan/output-plan-summary.md`,
  `../plan/output-plan-tasks.md` (P2-005 section, threading contract), 
  `../plan/output-plan-test-cases.md` (TC-011), `../plan/output-plan-concerns.md` (C-006, C-007,
  C-008, C-010, C-019).
- `output-P2-004.md` (ring, `audio_mutex` attach/detach design, silence push, scripts and fixture
  in `evidence/P2-004/`).
- libobs 31.1.1 in `plugin/.deps`: `libobs/media-io/audio-io.h/.c` (`audio_convert_info` field
  order, `audio_output_connect`/`disconnect` under `input_mutex`, `do_audio_output` calls inputs
  under the same mutex), `libobs/obs.c` (`obs_shutdown`: `stop_audio` nulls `obs->audio.audio`
  before `obs_free_data`, so a late `destroy` disconnect is a no-op).

### Interpretation notes

- The "Whole mix" entry is a sentinel value in the same `audio_source` string setting (it cannot
  collide with a UUID); a new int setting `audio_mix_track` holds the track (1–6).
- `info.get_defaults` is added now with only the track default (1); P2-010 extends it. The track
  is also clamped to 1–6 when read.
- Connect and disconnect happen in `update` (and `destroy`) under the existing `audio_mutex`, after
  any source detach: lock order stays ours → libobs (`input_mutex`). The mix callback takes no lock
  of ours and calls no `sv_*`. `video_tick`'s source resolution is skipped in mix mode.
- A track change while "Whole mix" is selected disconnects the old mix index then connects the new
  one; a track change in source mode only stores the value.

## P2-006

Plan task P2-006 from `../plan/output-plan-tasks.md`: preset folder picker (`OBS_PATH_DIRECTORY`),
preset list refreshed through a modified callback, a per-source `os_task_queue` worker running
`sv_prepare_preset` with stale results dropped, `sv_commit_preset` in `video_render` inside the GL
state guard, worker drained before `sv_destroy`; remove `preset_path` and the synchronous
`sv_load_preset`; settings changes reload the preset only when the folder or preset changed.
Auto-cycle, random order and blend are P2-007.

### Skills (using)

- `shader-programming` (orchestrator-listed): loaded. Its content is generic shader authoring; the
  part that applies here is GL state discipline. `sv_commit_preset` binds GL programs, so it runs
  inside `with_core_gl_state` on the graphics thread, like the old `sv_load_preset`.
- `cpp-*`: manual activation only, not used.

### Rules (read)

- Root `CLAUDE.md`: C++17, no code comments, sibling style (`visualizer-source.cpp`), no unit
  tests, public repo (no machine paths).
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-decisions.md` (C-003: no presets bundled, dev-only copy is fine;
  standing websocket/OBS rules), `../plan/output-plan-summary.md`, `../plan/output-plan-tasks.md`
  (P2-006 section), `../plan/output-plan-test-cases.md` (TC-004, TC-013–TC-016),
  `../plan/output-plan-concerns.md` (C-004, C-021).
- `output-P2-004.md`, `output-P2-005.md` (update change detection, `audio_mutex`, skipped frames
  from the synchronous preset load, scripts).
- Core header `external/synesthesia-vibe/include/synesthesia-vibe/synesthesia-vibe.h`
  (prepare/commit/free threading, SV_ERR_GL leaves handle and context unchanged); core
  `gl_pipeline.cpp` / `disk_textures.cpp` (textures: `<resource_path>/textures`, then
  `texture_search_path`, then the preset's folder; random textures only from
  `<resource_path>/textures`).
- libobs 31.1.1: `util/task.h`/`task.c` (`os_task_queue_destroy` runs every queued task before the
  stop task, then joins), `plugins/image-source/obs-slideshow.c` (folder scan with
  `os_opendir`/`os_readdir`).
- `.claude/common-issues/plugin-runtime.md`, `verification-tooling.md`, `docs/integrations.md`
  (install with `ditto`).

### Interpretation notes

- `preset` stores the file name; the path is `<preset_folder>/<preset>`. When `preset` is empty or
  not in the folder, the first `.milk` in sorted order is used, so choosing a folder shows
  something at once. The folder modified callback refills the list and points `preset` at the
  first entry when the stored one is not listed.
- Folder scanning happens on the worker, not in `update` (which OBS defers to the graphics thread
  for video sources), so no I/O lands on the render path.
- `update` queues a request only when the folder or preset string changed; the worker also skips a
  path that is already the current target.
- Manual picks commit with a hard cut (blend 0), as the old `sv_load_preset(…, 0.0f)` did; P2-007
  brings `blend_seconds`.
- An empty/unset/missing folder: one warning per change, info text in the properties, the current
  preset (if any) keeps rendering; a new source renders whatever the core draws with no preset.

## P2-007 — Auto-cycle, random order and blend

### Task

Plan task P2-007 from `../plan/output-plan-tasks.md`: `random_order`, `cycle_seconds` (0 = off) and
`blend_seconds` settings; `sv_set_transition_config` applied from `video_render` when they change
(`soft_cut_seconds` = blend, `preset_duration` = cycle, `preset_duration_rand` = 0, hard cuts off per
C-022); `sv_set_preset_request_callback` set once after `sv_create`; a request picks the next
preset (sequential, or random never repeating the current per C-014), prepares it on the P2-006
worker and commits with `suggested_blend_seconds`; manual picks commit with the blend setting.
Hotkeys and pause are P2-008.

### Skills (using)

- None. The orchestrator listed no extra skills. `shader-programming` is not needed: no new GL work;
  the one new core call (`sv_set_transition_config`) is wrapped in `with_core_gl_state` per the plan
  rule, like the existing calls. `cpp-*` are manual activation only.

### Rules (read)

- Root `CLAUDE.md`: no code comments, sibling style (`visualizer-source.cpp`), no unit tests,
  public repo (no machine paths).
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-decisions.md` (standing websocket/OBS rules; C-004…C-029 defaults).
- `../plan/output-plan-summary.md`, `../plan/output-plan-tasks.md` (P2-007 and P2-008 sections),
  `../plan/output-plan-test-cases.md` (TC-017, TC-018), `../plan/output-plan-concerns.md` (C-013,
  C-014, C-022).
- `output-P2-006.md` (worker, generation counter, `preset_target`, commit path, scripts).
- Core header `external/synesthesia-vibe/include/synesthesia-vibe/synesthesia-vibe.h`
  (`sv_transition_config`, request callback, latch) and core `frame.cpp` / `context.cpp` /
  `preset.cpp` / `render/core/frame_driver.cpp` (latch atomics; a prepare failure on any thread
  sets `requestLoadFailed` when latched; a commit clears the latch and re-arms the timer at
  `now + soft_cut + duration + rand`).

### Interpretation notes

- Timer semantics (core): the next request fires `soft_cut_seconds + preset_duration` after a
  commit, and the commit lands one frame after the prepare finishes, so commits are about
  `cycle + blend` apart. The timer armed for the current preset is kept when the config changes.
- The request callback does no I/O: it records "cycle requested" under `preset_mutex` and queues the
  P2-006 worker. A manual pick that is pending wins over a cycle request; the plugin tracks whether
  the core's latch is still unserved so it never sticks (a manual pick that turns out to be a no-op
  serves the cycle instead).
- Sequential order walks a cursor (last preset resolved, including a broken one), so a broken file
  is skipped on the next request. Random excludes the current preset (and the last broken one when
  3+ files exist).
- With a single preset, a cycle request re-commits it (blend into itself) so the latch clears.
- Defaults (core values, C-022): cycle 16 s, blend 2.7 s, random off. P2-010 owns the full defaults
  pass.
- Pause (C-013, P2-008) hooks into the request callback and the transition-config apply.

## P2-008 — Hotkeys

### Task

Plan task P2-008 from `../plan/output-plan-tasks.md`: four source hotkeys registered with
`obs_hotkey_register_source` (next, previous, random, pause/resume auto-cycle). Previous walks a
history of the last 50 commits (C-014). Pause is a toggle, not persisted, ignores core requests
while paused, and manual switching still works (C-013). Hotkey callbacks never call `sv_*`: they
write a mutex-guarded slot, the P2-006 worker prepares, `video_render` commits and applies the
transition config.

### Skills (using)

- None. The orchestrator listed no extra skills. No GL work (the hotkey path reuses the existing
  `sv_prepare_preset` worker and the render-thread commit/config calls). `cpp-*` are manual
  activation only.

### Rules (read)

- Root `CLAUDE.md`: no code comments, sibling style (`visualizer-source.cpp`), no unit tests,
  public repo.
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-decisions.md` (standing OBS/websocket rules).
- `../plan/output-plan-tasks.md` (P2-008 row and section, threading contract),
  `../plan/output-plan-test-cases.md` (TC-019), `../plan/output-plan-concerns.md` (C-013, C-014,
  C-029).
- `output-P2-007.md` (pause hook points, `take_preset_job` priority, latch mirror).
- libobs 31.1.1 in `plugin/.deps`: `plugins/image-source/obs-slideshow.c` (source hotkey pattern),
  `libobs/obs-hotkey.h`, `libobs/obs-hotkey.c` (callbacks run under the hotkeys mutex;
  `obs_hotkey_unregister` takes it), `libobs/obs-source.c` (`obs_source_destroy_defer` calls
  `info.destroy` before `obs_context_data_free` releases source hotkeys).

### Interpretation notes

- libobs releases source hotkeys only in `obs_context_data_free`, after `info.destroy` has freed
  the plugin data, so a press in that window would reach freed memory. `destroy` therefore
  unregisters the four hotkeys first; callbacks run under the hotkeys mutex, so none is running
  or can start once `obs_hotkey_unregister` returns.
- A hotkey bumps the P2-006 generation (like a manual pick) and stores a command; the worker
  serves it first. "Next" follows the mode (sequential next or random no-repeat); "random" is
  always random no-repeat; "previous" walks the commit history, else the previous file in sorted
  order. Hotkey picks commit with `blend_seconds` and never no-op (a single-file folder re-commits).
- Pause forces `preset_duration = 0` in `apply_transition_config`. A core request that still
  fires while paused (the already-armed timer, C-013) is deferred, not served; the core latch stays
  set, so resume serves it, otherwise cycling would never restart without a manual pick.
- History is cleared when the preset folder changes.
- **Revised during the task:**
  - The first build also forced `preset_duration = 0` while paused, as the plan suggested.
  - A manual pick while paused then armed the core timer with the core's disabled-timer default
    (16 s plus up to 10 s random, `frame.cpp:356-359`, `context.h:70-71`). Cycling therefore did
    not follow the user's interval after resume.
  - The final build leaves the transition config alone and pauses by deferral only.

## P2-009 — Resolution and render scale

### Task

Plan task P2-009 from `../plan/output-plan-tasks.md`: `width` × `height` settings (default
1920×1080, limits 128–8192 per side, C-012) feeding `get_width`/`get_height` and `sv_resize`
(called in `video_render`, inside `with_core_gl_state`); a `render_scale` setting (100 / 75 / 50 %)
that sizes the texrender and the core, while the sprite is drawn at the full source size.

### Skills (using)

- `shader-programming` (orchestrator-listed; GL state, render targets, the flipped sprite draw).
  No shader source is written: the change sizes render targets and the sprite only.
- `cpp-*` are manual activation only and were not loaded.

### Rules (read)

- Root `CLAUDE.md`: no code comments, sibling style (`visualizer-source.cpp`), no unit tests,
  public repo, locale keys for every user-visible string.
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-decisions.md` (standing OBS/websocket rules).
- `../plan/output-plan-summary.md` (`sv_resize` must run on the GL thread and was not in the
  spike's GL audit, so it goes inside the guard; P2-011 re-checks).
- `../plan/output-plan-tasks.md` (P2-009 row and section, threading contract),
  `../plan/output-plan-test-cases.md` (TC-020, TC-021, TC-022), `../plan/output-plan-concerns.md`
  (C-012).
- `output-P2-008.md` (current state; frames skipped on create).
- `docs/architecture.md` (texrender, `GS_FLIP_V`, opaque draw), `.claude/common-issues/plugin-runtime.md`
  (GL state cache desync; scissor and colour mask not guarded).
- Core: `context.cpp` `sv_resize`, `gl_pipeline.cpp` `BdGlPipelineResize`, `frame_targets.cpp`,
  `framebuffer.cpp`, `texture.cpp`, `blur_chain.cpp`, `motion_interpolator.cpp` (what state a
  resize touches).

### Interpretation notes

- `sv_resize` touches framebuffer bindings (`GL_FRAMEBUFFER` → 0), the 2D texture binding on the
  active unit, the viewport, and (through `ClearFramebuffer`) scissor off, colour mask all-on and
  the clear colour. The first three are in the guard; the last three are the same writes
  `sv_render_frame` already makes every frame (`blend_state.cpp`), so the resize adds no new class
  of state. libobs-opengl does not cache scissor enable, colour mask or clear colour across draws.
- The resize runs in `render_core_frame` before `gs_texrender_begin`, so the guard restores OBS's
  own target and the texrender binds its new-size buffer afterwards.
- Width and height are stored together in one 8-byte atomic so a render never sees a half-written
  pair; render scale is a separate atomic. Out-of-range values are clamped (sizes 128–8192, scale
  50–100).
- `sv_create` uses the scaled initial size. One log line per resize (render thread), none per
  frame.

## P2-010 — Settings pass

### Task

Plan task P2-010 from `../plan/output-plan-tasks.md`: one coherent properties layout with groups,
`get_defaults` for every setting, every user-visible string in `plugin/data/locale/en-US.ini`, the
export list rechecked with `nm -gU`. Layout, defaults and strings only; behaviour unchanged. No
new features ("now playing" line, failed-pick status stay follow-ups).

### Skills (using)

- None. The orchestrator listed none beyond the role file; the change is properties, defaults and
  locale strings (no GL state, render targets or shaders), so `shader-programming` does not match.
  `cpp-*` are manual activation only and were not loaded.

### Rules (read)

- Root `CLAUDE.md`: no code comments, sibling style (`visualizer-source.cpp`), no unit tests,
  public repo, every user-visible string a locale key.
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-decisions.md` (standing OBS/websocket rules).
- `../plan/output-plan-tasks.md` (P2-010 row and section, rules every plugin task follows),
  `../plan/output-plan-test-cases.md` (TC-004, TC-023), `../plan/output-plan-concerns.md`
  (C-004, C-012, C-021, C-022).
- `output-P2-004.md` … `output-P2-009.md` (settings each added; P2-010 notes: incomplete
  `get_defaults`, Mix track plain spinner, `max_cycle_seconds` long long → int).
- libobs 31.1.1 `obs-properties.h`/`.c`: `obs_properties_add_group`, `OBS_GROUP_NORMAL`;
  `obs_properties_get` searches groups recursively and `obs_property_modified` passes the topmost
  properties, so the existing modified callbacks keep working across groups.
- Siblings for style: `obs-transitions/transition-stinger.c` (group built from its own
  `obs_properties_create`), `obs-ffmpeg` (unit suffixes), `rtmp-services` locale (a quoted value
  with a leading space survives the ini parser).

### Interpretation notes

- Groups, in order: Presets (folder, preset, hint), Transitions (cycle, blend, random order),
  Audio (source, mix track), Output (width, height, render scale). This follows the delegation's
  example; TC-023's Audio, Presets and Output groups are all present.
- Group property names (`presets_group`, …) hold no settings (`OBS_GROUP_NORMAL`).
- Unit suffixes (` s`, ` px`) move to locale keys so no user-visible text stays in code.
- Mix track becomes an int list "Track 1"–"Track 6"; the stored value and its clamping are
  unchanged, so saved values load as before.
- Log lines and the renderer-refused log in `plugin-main.cpp` are logs, not UI, and stay in code.

## P2-010b — Properties dialog no longer writes `preset` on open

### Task

User-approved follow-up to P2-010: opening a visualizer's properties dialog with a preset folder
set and `preset` empty made OBS's properties view select and store the first file (the stored
value matched no item). Fix: a first list entry "(first preset)" with value `""`, so an empty
`preset` matches an item and opening the dialog changes nothing. An empty `preset` still means
"the worker uses the first preset".

### Skills (using)

- None. The orchestrator listed none; the change is a properties list entry and a locale key (no
  GL state, render targets or shaders). `cpp-*` are manual activation only.

### Rules (read)

- Root `CLAUDE.md`: no code comments, sibling style, no unit tests, public repo.
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-decisions.md` (websocket only, test collection, restore `Untitled`).
- `output-P2-010.md` (bug analysis; never remove an input or switch collection while its
  properties dialog is open), `output-P2-006.md` (list fill, modified callback, worker fallback).
- `.claude/common-issues/verification-tooling.md` (the dialog wedge entry).

### Interpretation notes

- The entry is added only when the folder has presets, so the hint/visibility logic is unchanged
  (the list stays hidden for an empty folder).
- Its value `""` reuses the existing worker path (`listed_or_first` falls back to the first file).
- The label follows the `AudioSource.None`/`AudioSource.Missing` pattern: a locale key, first item.

## P2-013 — Packaging (Release build, pkg, zip, ad-hoc signing)

### Task

Release build of the plugin, the template's `cmake --install` step (pkgbuild + productbuild) into
a staging prefix, a `ditto` zip of the `.plugin`, signing per C-002 (ad-hoc `.plugin`, unsigned
`.pkg`, zip; notarization commands recorded, not run), no presets or textures (C-003), and an
arm64-only `hostArchitectures` through a project-owned copy of `distribution.in` (C-016).

### Skills (using)

- None. The orchestrator listed none; packaging is CMake install plus macOS packaging tools (no GL
  state, render targets or shaders). `cpp-*` are manual activation only.

### Rules (read)

- Root `CLAUDE.md`: no code comments, never edit vendored template files, public repo (no team
  IDs, identity names or absolute paths in repo files), new-file header.
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-decisions.md` (C-002 ad-hoc now / notarize later, C-003 none
  bundled), `../plan/output-plan-summary.md`, `../plan/output-plan-tasks.md` (P2-013),
  `../plan/output-plan-concerns.md` (C-002, C-003, C-016, C-017).
- `docs/integrations.md`, `docs/structure.md`.
- Template (read only): `plugin/cmake/macos/helpers.cmake`,
  `plugin/cmake/macos/resources/create-package.cmake.in`, `distribution.in`,
  `installer-macos.pkgproj.in` (not referenced by the CMake helpers), `plugin/cmake/macos/xcode.cmake`,
  `plugin/cmake/macos/defaults.cmake`.
- `.claude/COMMON-ISSUES.md` → `common-issues/plugin-build.md`.

### Interpretation notes

- `defaults.cmake` sets the cached `CMAKE_INSTALL_PREFIX` to the real user plugins folder, so every
  `cmake --install` in this task passes `--prefix` to a staging dir under the git-ignored build dir.
- `helpers.cmake` configures `${CMAKE_CURRENT_BINARY_DIR}/distribution` from the vendored file and
  `create-package.cmake` reads that path at install time. `plugin/CMakeLists.txt` configures the
  project-owned copy to the same output path after `set_target_properties_plugin`, so the later
  write wins and the vendored file stays verbatim.
- The project-owned copy lives in a new `plugin/packaging/macos/` folder (per-OS segment mirrors the
  template's `cmake/macos/`); `plugin/cmake/**` is template-only and `plugin/data/**` is globbed into
  the bundle, so neither can hold it.
- Only `hostArchitectures` changes in the copy; everything else matches the vendored file.
- No packaging script (C-017); commands go to the handoff for the doc pass.

## P2-014 — User install steps (`plugin/README.md`)

### Task

Write `plugin/README.md`, the user-facing sibling of `browser-source/README.md`: requirements,
install from the pkg or zip with the Gatekeeper workaround while unnotarized, adding the source,
every setting by group, hotkeys, presets folder and licence terms, the shared-audio limit,
troubleshooting and uninstall. Developer build commands stay in `docs/integrations.md` (C-017),
linked only.

### Skills (using)

- `project-documentation` — shape only (README structure, cross-links). No `docs/` writes.
- No area skill: no GL state, render targets or shaders are touched. `cpp-*` are manual activation
  only.

### Rules (read)

- Root `CLAUDE.md`: public repo (no absolute machine paths beyond `~/Library/...`, no identities,
  no private repos), root `README.md` untouched, README carries no file header.
- `.claude/harness/developer.md` (role file).
- `../plan/output-plan-gate-1-decisions.md` (C-002, C-003, C-006), `../plan/output-plan-tasks.md`
  (P2-014 row and section), `../plan/output-plan-concerns.md` (C-003…C-017, C-022).
- `output-P2-002.md` … `output-P2-010b.md` (README doc-gap items).
- `plugin/data/locale/en-US.ini` (exact UI labels), `plugin/src/visualizer-source.cpp`,
  `plugin/src/plugin-main.cpp`, `plugin/buildspec.json`, `plugin/CMakePresets.json`,
  `plugin/cmake/macos/resources/distribution.in` (read only) for names, defaults, ranges, hotkeys
  and log lines.
- `browser-source/README.md` (tone and structure sibling), `docs/concerns.md` (renderer check),
  `docs/stack.md`.

### Interpretation notes

- P2-013 runs concurrently: the bundle name `synesthesia-vibe-obs.plugin` is taken from the build
  output; pkg and zip file names are written as "see the release".
- Settings, defaults, ranges and log lines are quoted from source, not from the handoffs.

## REVIEW-FIX — README fixes S1, S2, N4 (Gate 2 follow-up)

### Task

Fix should-fix findings S1 and S2 from `../review/output-review-findings.md`, both in
`plugin/README.md`: (S1) the pkg also installs `synesthesia-vibe-obs.plugin.dSYM` into the plugins
folder; mention it in Option A and add it to the Uninstall `rm -rf`. (S2) the receipt id carries
literal quotes (template bug from P2-013); give the exact working command
`pkgutil --forget "'io.github.lee5783.synesthesia-vibe-obs'"` and explain briefly. Also correct the
Blend time row per nit N4 if it is a one-sentence fix. README text only: no code or CMake changes.
Verify S2 from the P2-013 pkg (expanded in the scratchpad) without installing it or running
`pkgutil --forget`; do not touch OBS; do not commit.

### Skills (using)

- None. The orchestrator listed none, and no installed skill matches a README text fix.

### Rules (read)

- Root `CLAUDE.md`: public repo (no absolute machine paths beyond `~/Library/...`), README carries
  no file header, no `docs/` writes.
- `.claude/harness/developer.md` (role file).
- `../review/output-review-findings.md` (S1, S2, N4), `output-P2-013.md` (payload listing, receipt
  id, template-bug note), `output-P2-014.md`, `plugin/README.md`.

### Interpretation notes

- N4 is fixed in the same table cell, one sentence, naming the `preset failed to commit` log line
  the Troubleshooting section already uses.
- The template is not overridden (the alternative fix in S1/S2); README text only, as delegated.
