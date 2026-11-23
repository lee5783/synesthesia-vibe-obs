# P2-007

## Subtasks executed

- **Settings:**
  - `cycle_seconds`: int, 0–3600, suffix ` s`; 0 means off.
  - `blend_seconds`: float, 0–30, step 0.1, suffix ` s`.
  - `random_order`: bool.
  - Defaults in `get_defaults` are the core's: 16 s and 2.7 s. `random_order` defaults to false
    implicitly. New `en-US.ini` keys: `CycleSeconds`, `BlendSeconds`, `RandomOrder`.
- **Transition config:**
  - `update` → `configure_auto_cycle` clamps the three values and stores them under
    `preset_mutex`. On a change it sets `transition_dirty`.
  - At the top of `render_core_frame`, `apply_transition_config` calls `sv_set_transition_config`
    inside `with_core_gl_state` with:
    - `soft_cut_seconds` = blend;
    - `preset_duration` = cycle (0 means never);
    - `preset_duration_rand = 0`;
    - hard cuts off, with loudness and half-life at 0, which means the core's defaults (C-022).
  - It logs `auto-cycle: every N s, blend B s, <order> order` or `auto-cycle: off, blend B s`.
- **Request callback:**
  - `sv_set_preset_request_callback(ctx, preset_requested, visualizer)` is set once after the
    worker is created in `create`.
  - The callback (render thread, inside `sv_render_frame`) does no I/O. Under the lock it records
    `cycle_requested` and `cycle_blend = suggested_blend_seconds`, unless the source is closing. It
    then logs the request and queues the P2-006 worker.
- **Worker** (`prepare_requested_preset`, restructured around a `preset_job`):
  - `take_preset_job` gives a pending manual pick (unhandled generation) priority over a cycle
    request. It serves a cycle request only while no prepared handle waits in `ready_preset`.
  - A manual pick that resolves to the preset already current becomes the cycle job
    (`switch_to_cycle_job`) when a cycle request is waiting. Otherwise the core latch would never
    clear.
  - **Next preset:**
    - Sequential walks `preset_cursor`, the last path resolved, including one that failed, and
      wraps. A broken file is therefore skipped on the next request.
    - Random (`std::mt19937`, worker-only) excludes the current preset and the last-resolved path.
      It falls back to excluding only the current preset, then to the only file.
    - With a single preset, the request re-commits it.
  - The handover stores `ready_preset_blend` beside the handle and clears `cycle_requested`. A
    prepare failure also clears it, because the core releases the latch and restarts the timer.
    `commit_ready_preset` clears it when it takes a handle.
  - A next job does not bump the generation, so a manual pick arriving during its prepare
    supersedes it.
  - A cycle request with no presets in the folder is ignored silently. The latch stays set until a
    manual pick loads something, so there is no request loop and no repeated warning.
- **Commit:** `sv_commit_preset(ctx, h, ready_preset_blend)`, logged as
  `preset committed: <path> (blend B s, N ms)`.
- **Manual picks** commit with `blend_seconds`. The exception is the first commit of a source,
  which has nothing committed yet (`preset_target` empty) and therefore commits as a hard cut, so
  creating a source or loading a collection does not blend out of the core's no-preset image.

## Files changed

- `plugin/src/visualizer-source.cpp`. The working-tree diff also holds the uncommitted
  P2-002…P2-006 changes.
- `plugin/data/locale/en-US.ini`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/input-dev.md` (P2-007
  section appended)
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/output-P2-007.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-007/P2-007-cycle.md`
  (tracked)
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-007/`: the
  `*.png` files, `P2-007-plugin-log.txt`, `P2-007-cycle.mjs`, `P2-007-skips.mjs`,
  `P2-007-remove.mjs` and `P2-007-ws-session.mjs`. These are git-ignored local evidence.
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/session.log` (log lines)

## What changed and why

**The core owns the timer, the plugin owns the choice.** The core asks for a preset after
`soft_cut + duration` and latches until a load succeeds or fails. The plugin picks the file and
prepares it off-thread. The commit stays where P2-006 put it: in `video_render`, under the GL
guard.

**No I/O on the render path.** The callback runs inside `sv_render_frame`, so it only flips a
flag and queues the worker. The folder scan and the prepare happen on the worker.

**The latch must never stick.** A host that ignores a request is never asked again until it
loads a preset. So `cycle_requested` mirrors "the core latch is set and nothing queued will clear
it", and is cleared exactly where the core clears its latch: at handover or commit, and on a
prepare failure. The one path that could strand it, a manual pick that turns out to be a no-op
while a cycle request waits, is turned into the cycle job.

**Manual picks win.** A pick already queued or prepared is served first. Its commit clears the
latch and re-arms the timer, so the cycle request it overlapped is not served as a second change.

**Live changes.** `sv_set_transition_config` keeps the timer already armed for the current
preset. After a change, the first interval follows the old values; later intervals follow the new
ones. Measured: 12.0 s, then 6.0 s after switching from 10/2 to 5/1. Turning cycle 0 back on after
the armed time has passed fires one request at once.

**Pause design for P2-008 (C-013).** Pause can return early in `preset_requested` before
`cycle_requested` is set. The latch then stays set, which is harmless, because a manual pick's
commit clears it. `apply_transition_config` is the single place to force `preset_duration = 0`
while paused and restore it on resume, by marking `transition_dirty`. Next and previous can be
queued as manual-style jobs on the same worker; previous needs the C-014 history, which P2-008
adds.

## Verification

- `cmake --build --preset macos-arm64` (from `plugin/`): `** BUILD SUCCEEDED **`. The build runs
  with `-Werror` and gave no diagnostics from `plugin/src`.
  - `nm -gU` lists 7 exports, unchanged.
  - New libobs imports: `obs_properties_add_float`/`add_bool`,
    `obs_property_int_set_suffix`/`float_set_suffix`, `obs_data_get_double`/`get_bool` and
    `obs_data_set_default_double`.
- **`sv_*` reachability:**
  - `sv_set_transition_config` is reached only from `apply_transition_config` ←
    `render_core_frame`, inside `with_core_gl_state`.
  - `sv_set_preset_request_callback` is called only from `create`.
  - `preset_requested` calls only libobs and std functions, never `sv_resize`, `sv_render_frame`
    or `sv_destroy`.
- **OBS 33.0.3**, collection `synesthesia-spike`, program scene, 30 fps. Full write-up in
  `evidence/P2-007/P2-007-cycle.md`:
  - **Cycle 10, blend 2:**
    - commits 12.02–12.24 s apart (`cycle + blend` plus about one frame plus the prepare);
    - the sequential order wraps;
    - `zz broken.milk` gets `preset failed to prepare`, and the next request comes one cycle later
      and moves on to the next file (a 24.1–24.3 s gap).
  - **Live:** 10/2 → 5/1 gave a first interval of 12.0 s (the armed timer), then 6.02–6.09 s.
    Cycle 0 gave no request in 40 s. Back to 10 gave an immediate request. No restart.
  - **Random, cycle 3, blend 0.5, 90 s:** 26 requests, 25 commits and 1 broken skip. No
    consecutive repeat, including across the skip.
  - **A single-preset folder** re-commits every 3.53–3.57 s in both modes.
  - **Manual pick** with blend 6 logs `blend 6.0 s`, with screenshots at 1.2, 3.4 and 5.6 s
    mid-blend. Blend 0 is a hard cut.
  - **GetStats:** 0 skipped frames in five windows (the 90 s baseline, then 150 s, 105 s, 90 s
    and the manual phase while cycling). One earlier unsampled 150 s run right after launch showed
    +6. It was not reproduced, and its cause was not isolated.
  - **Teardown:** 10 temporary visualizers, cycling at 1 s, were removed mid-cycle. No crash, no
    leftovers, and `preset worker stopped` each time.
  - **Persistence:** all five settings came back after a relaunch.
  - **Leaks and GL errors:** both quits printed `Number of memory leaks: 0`, with 0 GL error lines
    and no `preset failed to commit`.
- **Cleanup:**
  - The spike visualizer's settings were restored exactly
    (`{"audio_source":"","preset_path":""}`, `overlay: false`), and the temporary inputs removed.
  - `Untitled` was restored, quit with 0 leaks and reopened.
  - The P2-007 build stays installed, identical to the repo build (`cmp`), and
    `codesign --verify` passes.

## Evidence

- `evidence/P2-007/P2-007-cycle.md`: the interval table, log excerpts for every case, the
  GetStats table, teardown, persistence, leaks, GL errors and cleanup.
- `evidence/P2-007/P2-007-plugin-log.txt`: every plugin log line from both sessions (paths
  scrubbed to `<scratch>`).
- Screenshots (960 px; the dialog is 720 px), all viewed:
  - `P2-007-manual-before-swarm.png`;
  - `P2-007-manual-blend-1s.png`, `-3s.png` and `-5s.png` (mid-blend, swarm into Fractopia);
  - `P2-007-manual-after-fractopia.png`;
  - `P2-007-manual-hard-cut-300ms.png`;
  - `P2-007-props.png`: the dialog with "Change preset every (0 = off)". Blend and Random sit
    below the fold, and scrolling would need GUI automation.
- Scripts: `P2-007-cycle.mjs`, `P2-007-skips.mjs`, `P2-007-remove.mjs` and
  `P2-007-ws-session.mjs`.

## Known limitations / follow-ups

- **The "Preset" list does not follow auto-cycle.** It keeps showing the last manual pick.
  Choosing that same entry again changes no setting, so nothing happens. Choosing it via
  `SetInputSettings` with an unchanged value is also deduplicated by P2-006's `request_preset`.
  P2-010 could add a "now playing" info line, or write the cycled file back. Writing it back would
  need care, because `obs_source_update` re-enters `update`.
- **The first interval after a cycle change follows the old timer.** This is core behaviour, and
  there is no API to re-arm. Documented in TC-017.
- **Turning auto-cycle on can fire at once.** If the armed time already passed while the cycle was
  0, turning it on fires one request immediately.
- **Random order can still pick a broken file again** after one other preset has played. Only
  the last-resolved path is excluded. Sequential mode skips it for a full lap.
- **Run 1 skipped 6 frames.** It came right after an OBS launch, was unsampled and was not
  reproduced in five later windows (0 skipped). P2-011 or P2-012 should keep an eye on skipped
  frames during blends at 60 fps, since a blend renders two presets.
- **Hard-cut fields stay unexposed** (C-022 default). `SV_PRESET_REQUEST_HARD_CUT` is handled the
  same as a timer request, with blend 0 from the core, in case P2-010 exposes them.
- `obs_properties_add_int(…, 0, max_cycle_seconds, 1)` passes a `long long` constant to an `int`
  parameter. The value fits and there is no warning, but P2-010 may want a `static_cast<int>` for
  tidiness.
- No unit test added (none requested). The latch and priority logic in `take_preset_job`,
  `switch_to_cycle_job` and the three `cycle_requested = false` sites is the riskiest part. A
  scripted race test (pick at the moment the timer fires) would need a debug hook. The user can
  decide.
- **Doc gaps for document-writer:**
  - `docs/architecture.md`: the auto-cycle flow. That covers the callback flag, worker job
    priority, the latch mirror (`cycle_requested`), blend carried with the ready slot, and the
    transition config applied in render.
  - `.claude/how-x-works/plugin-render-frame.md`: `apply_transition_config` before
    `commit_ready_preset`, and the request callback firing at the end of `sv_render_frame`.
  - `docs/concerns.md`: the list not following the cycle, and the first interval following the
    old timer.
  - `plugin/README.md` (P2-014): what cycle, blend and random do, that 0 turns cycling off, that
    the real period is cycle + blend, and that a broken preset is skipped.

## Handoff for downstream review

- **Acceptance criteria addressed:**
  - commits about every `cycle + blend` seconds (12.0–12.2 s at 10/2);
  - cycle 0 stops the changes;
  - live changes through `SetInputSettings` work without a restart, with the first interval
    following the old timer as documented;
  - random never repeats the current preset (25 commits);
  - manual picks blend with `blend_seconds`;
  - broken presets are skipped and the latch released;
  - 0 skipped frames in the sampled windows, 0 GL errors and 0 leaks.
- **Areas to scrutinize:**
  - `take_preset_job` and `cycle_request_servable`: the manual-first rule, and refusing a cycle
    job while a handle waits;
  - `switch_to_cycle_job` and the generation check;
  - the three sites that clear `cycle_requested` (handover, prepare failure, the render taking the
    slot), compared with the core's latch rules in `frame.cpp` / `context.cpp` / `preset.cpp`;
  - `preset_cursor` being worker-only, without a lock;
  - `preset_requested` reading `preset_worker` without a lock (set before the first render and
    nulled only in `destroy`, after rendering has stopped);
  - the hard cut on the first commit, keyed on `preset_target.empty()`.
- **How to run** (from a scratch copy of the evidence scripts, with the P2-006 scratch folders and
  the visualizer in `synesthesia-spike`):
  `OBS_WS_CREDS=<creds file> node P2-007-cycle.mjs <phase> <scratch root> <shots dir>`, where
  `<phase>` is `sequential`, `live`, `random`, `one`, `manual` or `cleanup`. Then
  `node P2-007-skips.mjs <scratch root> 90 150` and `node P2-007-remove.mjs <scratch root>`.
  Rename `P2-007-ws-session.mjs` to `ws-session.mjs` first.
