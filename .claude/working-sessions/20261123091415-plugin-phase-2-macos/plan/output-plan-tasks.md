# Tasks

Phase 2 — native plugin, macOS (Apple Silicon, OBS 31+, checked on OBS 33.0.3). All plugin code
lives in `plugin/`; the core submodule is never edited. Each row is one developer pass. Detailed
acceptance criteria and evidence for each task follow the table.

| ID | Description | Specialist | Dependencies | Order | Status | Notes |
|----|-------------|------------|--------------|-------|--------|-------|
| P2-001 | Build guards: fail configure clearly unless macOS + Xcode generator + arm64-only, and recheck a from-scratch configure and build | developer | Gate 1 | 1 | pending | PLAN item 1 remainder; `docs/concerns.md` "macOS / arm64 / Xcode only". Universal per C-016 |
| P2-002 | Thread-race fix: every `sv_*` call and the `dt` consumption move into `video_render`; `video_tick` writes only atomics; reset the delta on `show`/`activate` and cap a single step | developer | Gate 1 | 2 | pending | Spike follow-ups "race" and "clamp dt". Policy per C-011; verification per C-009 |
| P2-003 | Opaque draw: draw the texrender with `OBS_EFFECT_OPAQUE` so alpha-0 presets show opaque | developer | P2-002 | 3 | pending | Spike follow-up "opaque draw" |
| P2-004 | Audio, chosen source: SPSC lock-free PCM ring, an "Audio source" list setting, `obs_source_add_audio_capture_callback` on the chosen source, resample to the core's rate, drain into `sv_push_pcm_float` in `video_render`; remove the synthetic tone | developer | P2-002 | 4 | pending | PLAN item 3. C-005, C-006, C-007, C-008, C-019, C-020 |
| P2-005 | Audio, whole mix: "Whole mix" entry plus a track setting, `audio_output_connect` / `audio_output_disconnect` on `obs_get_audio()` | developer | P2-004 | 5 | pending | PLAN item 3 |
| P2-006 | Presets: folder picker, current-preset list, off-thread `sv_prepare_preset` on a worker, `sv_commit_preset` in `video_render`; remove `preset_path` and the synchronous `sv_load_preset` | developer | P2-002 | 6 | pending | PLAN item 4 (part 1). C-003, C-004, C-021 |
| P2-007 | Auto-cycle and transitions: random-order toggle, cycle and blend seconds through `sv_set_transition_config`, next preset chosen in the preset request callback | developer | P2-006 | 7 | pending | PLAN item 4 (part 2). C-013, C-014, C-022 |
| P2-008 | Hotkeys: next, previous, random, pause/resume auto-cycle (`obs_hotkey_register_source`), consumed in `video_render` | developer | P2-007 | 8 | pending | PLAN item 5. C-013, C-014, C-029 |
| P2-009 | Resolution: width × height settings (default 1920×1080) → `get_width`/`get_height` and `sv_resize`; render-scale setting | developer | P2-003 | 9 | pending | PLAN item 6. C-012 |
| P2-010 | Settings pass: one coherent properties layout, `get_defaults`, every string in `en-US.ini`, export list rechecked | developer | P2-005, P2-008, P2-009 | 10 | pending | Integration of P2-004…P2-009 |
| P2-011 | Done-when verification run on the integrated build at the user's 30 fps canvas: GL-state check, both audio paths, live settings, source removal, quit with no leaks, the race stress test | developer | P2-010 | 11 | pending | PLAN *Done when* items 2–5 |
| P2-012 | 1080p60 measurement in a dedicated profile | developer | P2-011, C-001 approval | 12 | pending | PLAN *Done when* item 1. **Blocked on C-001** |
| P2-013 | Packaging: Release build, `.pkg` through the template's install step, a zip of the `.plugin`, signing per C-002; no presets inside | developer | P2-010 | 13 | pending | PLAN item 7. C-002, C-003, C-016. Notarization blocked until credentials exist |
| P2-014 | User install steps: `plugin/README.md` (install, add the source, settings, hotkeys, presets folder, troubleshooting, uninstall) | developer | P2-013 | 14 | pending | PLAN item 7. C-017 |
| P2-015 | PR-style review of P2-001…P2-014 against this plan, `CLAUDE.md` and the GL/threading rules | code-reviewer | P2-001…P2-014 | 15 | pending | P2-012 may still be blocked; review what landed |
| P2-016 | Doc pass: `docs/` pages, `.claude/how-x-works/plugin-render-frame.md`, common issues, the `PLAN.md` Phase 2 status and upstream ask #4 draft | document-writer | P2-015 | 16 | pending | C-006 upstream draft; C-028 |

**Specialist values:** `developer` | `code-reviewer` | `document-writer`
**Status:** `pending` | `in_progress` | `done`

Order is the suggested serial order. P2-002 → P2-003 → P2-009, P2-002 → P2-004 → P2-005 and
P2-002 → P2-006 → P2-007 → P2-008 are independent chains, but they all edit
`plugin/src/visualizer-source.cpp`, so run them one after another, not in parallel.

---

## Rules every plugin task follows

- **Threading contract (after P2-002):**
  - Every `sv_*` call that needs the GL context runs in `video_render` (graphics context held),
    or in `create`/`destroy` inside `obs_enter_graphics()`, and always inside
    `with_core_gl_state`.
  - `sv_prepare_preset` runs only on the preset worker. `sv_prepared_preset_free` may run anywhere.
  - `video_tick`, `update`, the audio callbacks and the hotkey callbacks never call `sv_*`. They
    write atomics, a mutex-guarded slot or the ring, and `video_render` applies the result.
  - `update` runs on the UI thread and must not call `sv_*`.
- **No code comments, no unit tests,** sibling style (`visualizer-source.cpp`), the standard file
  header on every new file. New files go in `plugin/src/<kebab-name>.cpp/.hpp` and are added to
  `target_sources` in `plugin/CMakeLists.txt` (`docs/structure.md`). `plugin/cmake/**` stays
  verbatim.
- **Strings:** every user-visible string is a key in `plugin/data/locale/en-US.ini`, read with
  `obs_module_text`.
- **Logging:** `obs_log`, one line per decision (source created, audio source attached or
  detached, preset committed, setting applied). Nothing per frame.
- **Export list:** a new `obs_module_*` entry point goes into `plugin/exported-symbols.txt` in
  the same change. Check with `nm -gU`.
- **OBS testing:**
  - Use only the local obs-websocket, in the `synesthesia-spike` scene collection (or a new
    `synesthesia-phase2` collection). Never use GUI or accessibility automation. If the websocket
    is unavailable, or rejects auth, stop and ask the user.
  - Never edit OBS config files. Never change the user's profile, canvas or fps (P2-012 runs only
    after C-001 is approved).
  - Install with `ditto` plus an ad-hoc re-sign as in `docs/integrations.md`. Quit OBS first and
    poll `pgrep -x OBS` (`common-issues/verification-tooling.md`).
  - Evidence goes in `<root>/develop/evidence/<TASK-ID>/`. Only `.md` and `.py` files there are
    committed, so the report quotes the log lines that matter.
- **Test audio fixture (C-019):**
  - A generated WAV (beats plus a tone sweep, about 30 s, 48 kHz stereo) is played by an OBS
    **Media Source** (`ffmpeg_source`, `is_local_file`, `looping: true`). The Media Source sits
    visible in the active test scene, because an inactive media source does not play.
  - It is routed to track 1 (`SetInputAudioTracks`), with monitoring off.
  - It needs no macOS permission. Mute it with `SetInputMute` to get a silent baseline.
  - The generator is a session evidence helper (`.py`, committed under evidence). The WAV is not
    committed. The Phase 1 helper `bs-005-make-beat-wav.py` can be reused.
- **Audio-reaction evidence (C-010):** a scratch build adds a temporary `sv_set_debug_callback`
  that logs bass, mid and treb once a second. It runs muted and then unmuted, and the levels are
  compared. Screenshots from `GetSourceScreenshot` back this up. The instrumentation is removed
  before review.

---

## Task details

### P2-001 — Build guards

**What:**
- In `plugin/CMakeLists.txt`, stop configure early with one clear `FATAL_ERROR` unless all three
  hold: the host is macOS, the generator is Xcode, and `CMAKE_OSX_ARCHITECTURES` is exactly
  `arm64`. The message points to the `macos-arm64` preset.
- If the user picks universal in C-016, this task instead allows `arm64;x86_64` and adds the second
  core build (`x64-osx`) plus `lipo`. That is a larger pass; split it into P2-001b if chosen.
- Keep the `.CMakeBuildNumber` seed guard.

**Acceptance:**
- `rm -rf plugin/build_macos_arm64 plugin/cmake/.CMakeBuildNumber`, then
  `cmake --preset macos-arm64 && cmake --build --preset macos-arm64` passes on the first try, with
  no warnings from `plugin/src`.
- `cmake --preset macos` (the template's universal preset) fails at configure with the new
  message, not at link.

**Evidence:** both configure outputs (tail), `lipo -info` (arm64), `nm -gU` (exactly the 7 names).

### P2-002 — Thread-race fix, delta reset and cap

**What:**
- `visualizer_source` keeps the pending delta as an atomic, for example nanoseconds in
  `std::atomic<uint64_t>`, plus an atomic frame-pending flag.
- `video_tick` only adds to the delta and sets the flag. `video_render` takes both with
  `exchange`, so a second render in the same frame (projector, screenshot) does not render the core
  again, and no delta is lost or counted twice.
- The synthetic tone moves into the render path as a temporary stand-in, driven by the consumed
  delta. P2-004 deletes it.
- Add `show` and `activate` callbacks that reset the pending delta. Cap the delta passed to one
  `sv_render_frame` at the C-011 value.
- `visualizer_update` keeps writing only the mutex-guarded slot.

**Acceptance:**
- No `sv_*` call is reachable from `video_tick`, `update` or any non-graphics callback (code
  review plus grep).
- **Stress repro:** with the visualizer on air (program scene) next to an image, a colour and a
  text source, a script calls `GetSourceScreenshot` of the visualizer about 20 times a second for
  5 minutes while toggling scene items. OBS does not crash, the OBS log has 0 GL error lines, and
  the temporary re-entrancy detector (C-009) reports 0 overlapping core calls. The detector is an
  atomic in-flight counter around the `with_core_gl_state` calls, logging if it is entered while
  non-zero, in a scratch build only.
- **Off-air:** with the visualizer hidden for 10 s or more, a `GetSourceScreenshot` of it is not
  black, unlike the spike.

**Evidence:** the stress-script summary (request count, failures), OBS log excerpt, before/after
off-air screenshots, `GetStats` (render time unchanged within noise of the spike's 6.9–10.3 ms).

### P2-003 — Opaque draw

**What:**
- Draw with `obs_get_base_effect(OBS_EFFECT_OPAQUE)` and its `Draw` technique (it sets alpha to
  1.0). Keep the sRGB handling as it is: the `_srgb` setter and framebuffer sRGB when
  `gs_get_linear_srgb()` is set.
- Keep `GS_FLIP_V` and the blend push/pop.

**Acceptance:**
- With the swarm preset (writes alpha 0), `GetSourceScreenshot` of the visualizer and of a scene
  that nests the visualizer's scene over a colour source shows the visualizer fully opaque.
- Fractopia still looks identical to before: same orientation, and the colours do not look
  double-encoded.

**Evidence:** screenshots before and after, plus an alpha probe of the PNG (a short `.py` that
reports the minimum alpha), OBS log without GL errors.

### P2-004 — Audio from a chosen OBS source

**What:**
- **Ring:** a single-producer/single-consumer lock-free ring of interleaved stereo float frames.
  Suggested home: a new `plugin/src/audio-ring.hpp`/`.cpp`, or inside the source file if small;
  follow `docs/structure.md`.
  - Capacity about 1 s at 44.1 kHz.
  - When full, the producer drops the new frames; it never blocks.
  - The consumer in `video_render` drains everything and pushes only the newest ≤ 2304 frames
    (the core keeps only that many, `SAMPLE_SIZE_LPB`), so a backlog after off-air is dropped.
- **Setting:** an "Audio source" list (`OBS_COMBO_TYPE_LIST`, `OBS_COMBO_FORMAT_STRING`).
  - Entries: "None", then every source with `OBS_SOURCE_AUDIO` in its output flags, from
    `obs_enum_sources`.
  - Stored by the C-005 key. The source's display name is the list label.
  - "Whole mix" is added by P2-005.
- **Capture:**
  - Follow the sidechain pattern in libobs `plugins/obs-filters/compressor-filter.c`: keep a weak
    reference, and resolve lazily in `video_tick`, throttled, because the chosen source may be
    created after the visualizer while a scene collection loads.
  - Attach with `obs_source_add_audio_capture_callback`. Detach with
    `obs_source_remove_audio_capture_callback` when the setting changes and in `destroy`.
  - Removal takes the source's `audio_cb_mutex`, so after it returns the callback is not running.
    `destroy` must detach before freeing the ring.
- **Callback (audio or source thread):**
  - Convert OBS's planar float (OBS rate, OBS speaker layout) to 44.1 kHz interleaved stereo with
    libobs `audio_resampler_create` / `audio_resampler_resample` (C-007), then write into the ring.
  - Muted → per C-008.
  - No `sv_*`, no allocation per callback after setup.
- **Render:** drain the ring into `sv_push_pcm_float(ctx, data, frames, 2)` right before
  `sv_render_frame`, under the graphics context.
- Delete `push_synthetic_tone` and its constants.

**Acceptance:**
- With the Media Source fixture chosen, the debug-callback bass/mid/treb are clearly above the
  muted baseline. Muted, they fall to about 0 within about 1 s, which is the core's drain
  behaviour.
- Switching to "None" or to another source detaches the old callback (log line) and attaches the
  new one.
- Deleting the chosen Media Source while the visualizer runs causes no crash; the log says the
  audio source is gone. Re-creating a source with the same identity, where C-005 allows it,
  re-attaches.
- Loading a scene collection where the visualizer comes before its audio source attaches within
  the throttle interval.
- Removing the visualizer, then quitting: no crash, `Number of memory leaks: 0`.

**Evidence:** OBS log lines (attach/detach, levels muted vs unmuted), screenshots muted vs
unmuted, the websocket script.

### P2-005 — Audio from the whole mix

**What:**
- Add a "Whole mix" entry to the audio list and a "Mix track" setting (1–6, default 1). Show it
  only when "Whole mix" is selected, through a modified callback.
- Connect with `audio_output_connect(obs_get_audio(), track - 1, &conversion, cb, data)`, where
  `conversion = {44100, AUDIO_FORMAT_FLOAT, SPEAKERS_STEREO}`. libobs resamples, and the callback
  gets interleaved stereo in `data[0]` straight into the ring.
- Disconnect with `audio_output_disconnect` (same mix index, callback and param) on change and in
  `destroy`. It takes `input_mutex`, so the callback is not running afterwards.

**Acceptance:**
- With the Media Source on track 1 and "Whole mix / track 1", the levels react.
- Moving the Media Source off track 1 (`SetInputAudioTracks`), or choosing track 2, gives the
  silent baseline.
- Switching between whole mix and a chosen source never double-feeds: only one producer is
  attached at a time (log lines).
- Mix is the stream mix: a source set to "monitor only" does not reach it (document, don't fix).

**Evidence:** log lines, level comparison per track, websocket script.

### P2-006 — Preset folder, list, off-thread prepare and commit

**What:**
- **Settings:**
  - `preset_folder`: `obs_properties_add_path(…, OBS_PATH_DIRECTORY, …)`.
  - `preset`: a list of the `.milk` files in that folder, sorted, labelled by file name. Refreshed
    when the folder changes, through `obs_property_set_modified_callback`, returning `true`.
  - An info text when the folder is empty or unset (C-004).
  - See `plugins/image-source/obs-slideshow.c` for the sibling pattern.
- **Worker:**
  - One per source, for example libobs `os_task_queue_create` from `util/task.h`. It runs
    `sv_prepare_preset(ctx, path, &prepared, err, len)`.
  - It hands the handle to the render side through a mutex-guarded slot, tagged with a generation
    number. A stale result, from a request superseded before it finished, is freed with
    `sv_prepared_preset_free` and never committed.
- **Render:** `video_render` takes a ready handle and runs `sv_commit_preset(ctx, h, blend)` inside
  `with_core_gl_state`, then `sv_prepared_preset_free(h)`. On `SV_ERR_GL` the handle and context
  are unchanged; log and free it.
- **Destroy:** stop and drain the worker (`os_task_queue_wait` + destroy) **before** `sv_destroy`,
  because the core forbids destroying a context during a prepare. Free any pending handle.
- Skip re-preparing a path that is already current (spike debt "loads twice").
- Remove `setting_preset_path`, `default_preset_file`, `load_pending_preset` and the synchronous
  `sv_load_preset` (C-021).
- Textures: the core already searches the preset's own folder. No new texture setting unless C-003
  says otherwise.

**Acceptance:**
- Choosing another preset in the properties applies it without restarting OBS. The log shows
  `preset prepared …` on the worker and `preset committed …` from render.
- Changing the folder refreshes the list.
- An empty or unset folder (C-004) shows the info text, logs one warning, renders the C-004
  fallback, and never crashes.
- A preset that fails to parse logs `sv_last_error` / the prepare error and keeps the current
  preset.
- `GetStats` `averageFrameRenderTime` and `renderSkippedFrames` show no spike while switching
  presets repeatedly. Compare with a heavy preset, for example glowsticks v2 02.
- Deleting the source while a prepare is in flight (switch the preset, then remove the source at
  once, 20 times in a script) causes no crash and no leak on quit.

**Evidence:** log lines with timings, `GetStats` series during switches, screenshots of two
presets, the delete-during-prepare script result.

### P2-007 — Auto-cycle, random order, transitions

**What:**
- **Settings:** `random_order` (bool), `cycle_seconds` (0 = off), `blend_seconds`.
- **Applying them:** when the values change (`update` sets a dirty flag), `video_render` calls
  `sv_set_transition_config` with `preset_duration = cycle_seconds` (≤ 0 → never) and
  `soft_cut_seconds = blend_seconds`. The other fields follow C-022.
- **Request callback:** `sv_set_preset_request_callback` is set once after `sv_create`.
  - The callback runs inside `sv_render_frame` on the render thread. It picks the next preset,
    sequential or random per C-014, and queues a prepare.
  - The commit then uses `request->suggested_blend_seconds`.
  - The callback must not call `sv_resize`, `sv_render_frame` or `sv_destroy` (core header).
  - The request stays latched until a load succeeds or fails, so a slow prepare is not re-asked.
  - A failed prepare restarts the timer (core behaviour); the next request skips that file.
- A manual pick (P2-006, or a hotkey in P2-008) commits with `blend_seconds`.

**Acceptance:**
- With cycle 10 s and blend 2 s, the log shows commits about every `cycle + blend` seconds
  (`soft_cut_seconds` is the lead-in, per the core header).
- Cycle 0 stops the changes.
- Changing either value through `SetInputSettings` takes effect without restarting OBS. Note that
  the core keeps the timer already armed for the current preset, so the first interval after the
  change may follow the old value.
- Random order never repeats the same preset twice in a row when the folder holds 2 or more.

**Evidence:** log timestamps of commits for two settings, the websocket script.

### P2-008 — Hotkeys

**What:**
- In `create`, register four source hotkeys with `obs_hotkey_register_source`: next, previous,
  random, and pause/resume auto-cycle. Names and descriptions come from `en-US.ini`. libobs saves
  and loads the bindings with the source.
- Callbacks (hotkey thread) act only on `pressed == true`. They set an atomic command, and
  `video_render` applies it.
- Pause per C-013: ignore preset requests while paused and set `preset_duration` ≤ 0, then
  restore it on resume.
- Next and previous follow C-014.
- Unregister in `destroy` only if libobs does not already free source hotkeys on source destroy.
  Check `obs_source_destroy` and the hotkey context release.

**Acceptance:**
- `GetHotkeyList` lists the four names.
- `TriggerHotkeyByName` (with `contextName` = the source name) for each one produces the expected
  log lines and preset change.
- Pause stops auto-cycle until resumed. Next or previous while paused still switches and stays
  paused.
- If `TriggerHotkeyByName` cannot target source hotkeys in the installed obs-websocket, record
  that and ask the user to press bound keys (manual check).

**Evidence:** `GetHotkeyList` excerpt, log lines per trigger.

### P2-009 — Resolution and render scale

**What:**
- **Settings:** `width`, `height` (int, default 1920×1080, limits per C-012) and `render_scale`
  (a list such as 100 / 75 / 50 %, default 100).
- `get_width` / `get_height` return the settings, stored as atomics.
- **Render:**
  - When the scaled size changes, `video_render` calls `sv_resize(ctx, sw, sh)` inside
    `with_core_gl_state`, where `sw`/`sh` are width and height times the scale, rounded and at
    least 1.
  - `gs_texrender_begin` uses the scaled size.
  - The sprite is drawn at `width × height` with `gs_draw_sprite(tex, GS_FLIP_V, width, height)`.
- `sv_create` uses the initial scaled size.
- Log one line per resize.

**Acceptance:**
- Setting 1080×1920 makes the scene item's source size 1080×1920 (`GetSceneItemTransform`), and
  the frame fills it with the preset's aspect-correct look.
- 50 % scale lowers `GetStats` render time against 100 % and still fills the item.
- No GL errors across 10 scripted size changes.

**Evidence:** transforms, screenshots at 1920×1080, 1080×1920 and 50 %, `GetStats` per scale, log.

### P2-010 — Settings pass

**What:**
- One `get_properties` with a stable order. Suggested groups: Audio (source, mix track), Presets
  (folder, preset, random, cycle, blend), Output (width, height, scale).
- `get_defaults` for every setting.
- Every string in `en-US.ini`.
- No leftover spike setting.
- `plugin/exported-symbols.txt` matches the `obs_module_*` functions defined.

**Acceptance:**
- A fresh source shows all settings with their defaults; there is nothing to check visually beyond
  `GetInputSettings` / `GetInputDefaultSettings`.
- `nm -gU` matches the list.
- No warning from the plugin's own sources.

**Evidence:** `GetInputDefaultSettings` output, `nm -gU`, build log tail.

### P2-011 — Done-when verification (except 1080p60)

On the integrated build, at the user's current canvas:

- **GL-state check:** visualizer next to image, colour and text sources, with scene switches,
  show/hide, a second visualizer instance added and removed, and screenshots. All sources draw
  correctly, and the log has 0 GL error lines.
- Both audio paths react (P2-004, P2-005 criteria) on the same build.
- Preset and transition changes apply live (P2-006, P2-007).
- Remove the source while audio is attached and a prepare is in flight. Then quit OBS
  (osascript quit, per C-027). Expect no crash, `plugin unloaded` and `Number of memory leaks: 0`.
- Repeat the P2-002 stress repro on the final build.

**Evidence:** one consolidated report with log excerpts, screenshots and `GetStats`.

### P2-012 — 1080p60 measurement (blocked on C-001)

- Only after the user approves C-001: a dedicated profile at 1920×1080 base and output, 60 fps.
  Visualizer at 1920×1080 and 100 % scale, the audio fixture attached, auto-cycle on (10 s), run
  for 5 minutes.
- Pass: `activeFps` holds about 60, `renderSkippedFrames` does not grow, and
  `averageFrameRenderTime` stays well under 16.7 ms. Record the value.
- Switch back to the user's original profile afterwards. Confirm `GetProfileList` shows it
  current.
- If it fails, measure 75 % and 50 % scale and report.

**Evidence:** `GetStats` samples (every 10 s), the profile switch log, the report.

### P2-013 — Packaging

**What:**
- **Build:** a Release build of the plugin (`cmake --build --preset macos-arm64 --config Release`,
  or the existing RelWithDebInfo if the template's package step needs it).
- **Package:** run the template's install step into a staging prefix under the build dir
  (`cmake --install … --prefix <build>/package-staging --config Release`). Its
  `create-package.cmake` runs `pkgbuild` + `productbuild`. The `distribution.in` domain is
  `enable_currentUserHome`, so the `.pkg` installs into
  `~/Library/Application Support/obs-studio/plugins/`. That is the only path OBS scans on Apple
  Silicon (`frontend/widgets/OBSBasic.cpp`).
- **Zip:** also make a zip of the `.plugin` with `ditto -c -k --keepParent`.
- **Signing per C-002:**
  - Default: an ad-hoc `.plugin` and an unsigned `.pkg`.
  - If a Developer ID Application and Installer identity plus a notarytool keychain profile are
    provided, sign with hardened runtime and secure timestamp (already on for non-Debug in
    `cmake/macos/xcode.cmake`), `productsign` the pkg, `xcrun notarytool submit --wait`, and
    `xcrun stapler staple`.
  - Credentials stay outside the repo and are referenced only by environment variables
    (`CODESIGN_IDENT`, `CODESIGN_TEAM`, already wired in `CMakePresets.json`).
- **Architecture:** `distribution.in` declares `hostArchitectures="arm64,x86_64"` while the bundle
  is arm64 only. Handle it per C-016 without editing the vendored file.
- **Contents:** no presets or textures (C-003). The bundle carries `Resources/locale/en-US.ini`
  only.
- Where commands live: per C-017 (documented, no new script by default).

**Acceptance:**
- `.pkg` and `.zip` produced.
- `pkgutil --payload-files` lists only the `.plugin` (plus `.dSYM` for Release).
- `codesign -dvv` reports the expected signature, and `codesign --verify --deep --strict` passes.
- `spctl -a -vv -t install` is recorded. Expect a rejection while unnotarized.
- With the user's approval, an install through
  `installer -pkg … -target CurrentUserHomeDirectory` puts the plugin where OBS loads it. OBS shows
  the source, and `renderer check passed` is logged.

**Evidence:** command outputs, OBS log after a pkg install.

### P2-014 — User install steps

- `plugin/README.md`, the sibling of `browser-source/README.md`, with the same shape. It covers:
  - requirements (Apple Silicon, OBS 31+, OpenGL renderer);
  - install from the pkg or zip, and the Gatekeeper note while unnotarized;
  - adding "SynesthesiaVibe Visualizer";
  - every setting;
  - hotkeys (Settings → Hotkeys);
  - where to get presets and their licence terms (C-003);
  - the multi-instance audio limit (C-006);
  - troubleshooting: the renderer refusal log line, an empty preset folder, no audio reaction;
  - uninstall.
- No root `README.md` change. No machine paths beyond `~/Library/...`.

**Evidence:** the file; a reviewer can follow it on the packaged build.

### P2-015 — Review

PR-style review of all changes. Focus on:

- the threading contract: no `sv_*` outside the allowed threads; atomics and memory order;
  callback detach ordering in `destroy`;
- every core GL call inside `with_core_gl_state`;
- the worker-vs-destroy ordering;
- the ring's SPSC correctness;
- the export list;
- no comments added, headers present, strings localised;
- no secrets or paths in committed files.

Missing unit tests are not a defect (`CLAUDE.md`).

### P2-016 — Doc pass

- Update `docs/architecture.md`: frame flow, threading, audio, presets.
- Update `docs/structure.md` (new files), `docs/integrations.md` (packaging, signing, install),
  `docs/concerns.md` (close fixed items, add the C-006 limit and notarization status) and
  `docs/testing.md` (audio fixture, re-entrancy repro, 1080p60 method).
- Update `.claude/how-x-works/plugin-render-frame.md`. Add `.claude/how-x-works/plugin-audio-capture.md`
  if the flow warrants it.
- Add common-issue entries for gotchas hit during develop.
- With the user's approval at the gate (C-028): update the `PLAN.md` Phase 2 status, task ticks
  and risks, and add the upstream ask #4 draft (per-context PCM buffer and a sample-rate
  parameter).
