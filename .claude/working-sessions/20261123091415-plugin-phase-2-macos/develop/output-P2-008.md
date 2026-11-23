# P2-008

## Subtasks executed

- **Registration.** `register_hotkeys` runs at the end of `create`. It registers four source
  hotkeys with `obs_hotkey_register_source`:
  - `SynesthesiaVibe.NextPreset`
  - `SynesthesiaVibe.PreviousPreset`
  - `SynesthesiaVibe.RandomPreset`
  - `SynesthesiaVibe.PauseAutoCycle`

  The descriptions come from the new `en-US.ini` keys `Hotkey.NextPreset`, `Hotkey.PreviousPreset`,
  `Hotkey.RandomPreset` and `Hotkey.PauseAutoCycle`. libobs loads and saves the bindings with the
  source.
- **Callbacks (hotkey thread).** They act only on `pressed`. None of them calls `sv_*`.
  - **Next, previous, random:** `request_preset_command` stores the command in `pending_command`
    under `preset_mutex`, bumps `preset_generation` like a manual pick, logs `hotkey: <cmd> preset`
    and queues the P2-006 worker.
  - **Pause:** `toggle_auto_cycle_pause` flips `cycle_paused` and logs it. On resume, it also
    serves a deferred core request.
- **Worker.**
  - `take_preset_job` takes the command along with the pick, which keeps manual-first priority,
    and clears the slot. For previous, it resolves the history target under the lock.
  - `command_preset_path` chooses the file:
    - next = the auto-cycle choice for the current mode (sequential, or random with no repeat);
    - random = always random with no repeat;
    - previous = the history entry, else the previous file in sorted order.
  - The sorted-order fallback reuses `sequential_preset_file` through a new defaulted
    `backward` parameter.
  - Command jobs never no-op: a single-file folder re-commits.
  - They commit through the existing render path with `blend_seconds`, and log
    `hotkey picked the <cmd> preset: <path>[ (history)| (sorted order)]`.
- **History (C-014).**
  - `remember_committed_preset` runs on the render thread after `sv_commit_preset` returns
    `SV_OK`. It appends the path unless it equals the last entry, and caps the history at 50.
  - `previous_from_history` pops entries equal to the current target or the last resolved path,
    so a just-failed file is skipped too, and returns the new last entry.
  - The history is cleared when the preset folder changes. `request_preset` also drops a pending
    command when a settings pick supersedes it.
- **Pause (C-013).**
  - While paused, `preset_requested` does not serve the core request. It records
    `cycle_deferred` and logs `auto-cycle request deferred while paused`.
  - The core latch stays set, so the core asks nothing more until a load.
  - Manual picks and hotkeys still commit, which clears the latch. The source stays paused.
  - Resume turns a deferred request into `cycle_requested` and queues the worker.
  - The three places that cleared `cycle_requested` now call `clear_cycle_request`, which also
    clears `cycle_deferred`.
  - Pause is not persisted; it lives only in the instance.
- **Teardown.** `unregister_hotkeys` runs first in `destroy`. libobs releases source hotkeys only
  in `obs_context_data_free`, after `info.destroy` (`obs_source_destroy_defer`). Callbacks run under
  the hotkeys mutex, which `obs_hotkey_unregister` also takes.

## Files changed

- `plugin/src/visualizer-source.cpp`. The working-tree diff also holds the uncommitted
  P2-002…P2-007 changes.
- `plugin/data/locale/en-US.ini` (four `Hotkey.*` keys).
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/input-dev.md` (P2-008
  section appended).
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/output-P2-008.md`.
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-008/P2-008-hotkeys.md`
  (tracked).
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-008/` holds
  git-ignored local evidence:
  - the two `*.png` screenshots;
  - `P2-008-plugin-log.txt` and the `P2-008-*-run*.txt` script outputs;
  - `P2-008-hotkeys.mjs`, `P2-008-control.mjs` and `P2-008-ws-session.mjs`.
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/session.log` (log lines).

## What changed and why

**Hotkeys reuse the manual-pick path.** A press works like a settings pick: bump the generation,
let the worker prepare, let `video_render` commit. All the P2-006/P2-007 guarantees therefore
apply unchanged:
- no `sv_*` call off the render or worker thread;
- an in-flight prepare is superseded by a newer press;
- the press takes priority over a cycle request;
- the cycle latch is cleared on handover, commit or prepare failure.

**Pause works by deferral, not by changing the transition config.** The plan and the P2-007
handoff suggested also forcing `preset_duration = 0` while paused, and the first build did that.
In OBS this went wrong: a manual pick while paused armed the core timer with the core's
disabled-timer default (16 s plus up to 10 s random, `frame.cpp:356-359`,
`context.h:70-71`), and no cycle came in the 12 s after resume. Keeping the user's config and only
deferring the request gives the C-013 behaviour:
- no auto change while paused;
- manual picks still work;
- the timer always follows the user's interval;
- resume serves an overdue request at once.

This is a deliberate deviation from the plan text.

**Teardown safety.** P2-008's plan asked to unregister in `destroy` only if libobs does not
already free the hotkeys there. libobs frees them later, after the plugin data is gone, so
`destroy` unregisters them first.

## Verification

- `cmake --build --preset macos-arm64` (from `plugin/`): `** BUILD SUCCEEDED **`. The build runs
  with `-Werror`, and `plugin/src` gave no diagnostics.
  - `nm -gU` lists 7 exports, unchanged.
  - New libobs imports: `obs_hotkey_register_source` and `obs_hotkey_unregister`.
- **`sv_*` reachability:**
  - The hotkey callbacks call only `request_preset_command` / `toggle_auto_cycle_pause`, which use
    std, `obs_log` and `os_task_queue_queue_task`.
  - `sv_prepare_preset` stays worker-only. `sv_commit_preset` and `sv_set_transition_config` stay
    in `render_core_frame`.
- **OBS 33.0.3**, `synesthesia-spike`, program scene, 30 fps, websocket only. Full write-up in
  `evidence/P2-008/P2-008-hotkeys.md`.
  - **C-029:** `TriggerHotkeyByName` with `contextName` reaches source hotkeys. Every trigger
    returned ok and logged.
  - **GetHotkeyList** shows the four names, and eight with two instances.
  - **Next, previous, random:**
    - Next follows sorted order and skips a broken file on the next press.
    - Previous walks the history (Tiger ← swarm), then falls back to sorted order (Jester →
      Fractopia → Goody).
    - Random never repeated the current preset in 8 presses. Random-mode previous walks the
      history.
  - **Pause:**
    - Paused for 30 s (more than 6 cycles at 4 s): 0 auto commits, and one deferred line per
      core request.
    - A manual Next while paused committed, and the source stayed paused.
    - Resume served the deferred request at once, then kept a 4.5 s cadence.
    - A 1 s pause/resume left the cadence unchanged.
  - **Two instances:** while A was paused, B kept cycling every ~4.5 s and took a hotkey Next. A
    deferred only, and resuming A served its own request.
  - **Removal during hotkeys:** 10 sources cycling at 1 s, each removed in the middle of a burst of
    24 triggers. All 250 requests were ok, 240 callbacks ran, there was no crash and nothing was
    left over.
  - **Frames:** 0 skipped frames in every hotkey window. The 240-press control on one source gave
    44 → 44.
  - **Leaks and GL errors:** both quits printed `plugin unloaded` and
    `Number of memory leaks: 0`, with 0 GL error lines and 0 `preset failed to commit`.
- **Cleanup:**
  - The spike visualizer's settings are restored exactly, and the temporary inputs removed.
  - `Untitled` is restored, and OBS was quit with 0 leaks and relaunched.
  - The P2-008 build stays installed, and `codesign --verify` passes.

## Evidence

- `evidence/P2-008/P2-008-hotkeys.md`: the C-029 result, GetHotkeyList, log excerpts for every
  case, the pause timeline (including the rejected first design), the two-instance timeline,
  the removal stress test, the frame table, leaks, GL errors and cleanup.
- `evidence/P2-008/P2-008-plugin-log.txt`: every plugin log line from both OBS sessions (paths
  scrubbed).
- `evidence/P2-008/P2-008-before-next.png` and `P2-008-after-3-next.png` (960 px, viewed): swarm,
  then Unchained after the hotkey Next presses.
- Scripts and raw outputs: `P2-008-hotkeys.mjs`, `P2-008-control.mjs`, `P2-008-ws-session.mjs` and
  `P2-008-*-run*.txt`.

## Known limitations / follow-ups

- **Creating a visualizer skips frames.** It costs about 1.5–2.4 skipped frames per create (10
  creates with no hotkeys: 29 → 44). `sv_init_gl`/`sv_create` and the first compile run in `create`
  on the graphics thread. This is existing behaviour; P2-011 should measure it, and
  `docs/concerns.md` should record it.
- **Presses that arrive faster than a prepare coalesce.** The newest command wins, and a
  superseded prepare is dropped. Sequential Next still advances one step per press, because the
  cursor moves. A double Previous during one prepare goes back two steps, because the cursor
  entry is popped. A burst of 24 near-simultaneous presses gives one commit.
- **Resume can change the preset immediately.** If the core's timer expired while paused, resume
  changes the preset at once. This is deliberate: the interval is already overdue.
- **Pause does not cancel an accepted request.** A cycle request that was already accepted when
  pause is pressed (worker preparing) still commits once.
- **Random can pick a broken file again** once one other preset has played. This is the P2-007
  limitation, now also reachable through the random hotkey.
- **Hotkeys act on hidden sources too.** The prepared preset waits and commits on the next render
  of that source. The slideshow sibling ignores presses while hidden; P2-010 can decide.
- **History follows the commit, not the target.** Previous pressed within a frame of an unfinished
  handover re-commits the preset that is showing. This is a harmless edge case.
- **No unit test** was added (none requested). The riskiest parts are the latch and deferral
  bookkeeping (`clear_cycle_request`, the deferral in `preset_requested`, `toggle_auto_cycle_pause`)
  and `previous_from_history`.
- **Doc gaps for document-writer:**
  - `docs/architecture.md`: the hotkey path (callback → command slot and generation → worker →
    render commit); the deferral design for pause and why a zero duration is avoided; the
    unregister-first teardown.
  - `.claude/how-x-works/plugin-render-frame.md`: `remember_committed_preset` after a successful
    commit.
  - `plugin/README.md` (P2-014):
    - binding the four hotkeys per source (Settings → Hotkeys, under the source name);
    - previous walks the last 50 changes, then sorted order;
    - pause is per source and resets on restart;
    - resume may change the preset at once.
  - `docs/concerns.md`: frames skipped on create.

## Handoff for downstream review

- **Acceptance criteria addressed:**
  - `GetHotkeyList` lists the four names;
  - `TriggerHotkeyByName` with `contextName` works, so no manual key press was needed (C-029);
  - each hotkey produces its log line and preset change;
  - pause stops auto-cycle until resume, and next/previous while paused switch and stay paused;
  - the history walk follows C-014;
  - the instances are independent;
  - removal under fire is safe;
  - 0 GL errors, 0 leaks, 0 skipped frames attributable to hotkeys.
- **Areas to scrutinize:**
  - `preset_requested` (the deferral branch) and `toggle_auto_cycle_pause`, compared with the core
    latch rules (`frame.cpp` `BdDeliverPresetRequest` / `BdConsumePresetLoadFailure`,
    `preset.cpp` `BdClearPresetRequestLatch`);
  - `clear_cycle_request` at the three sites;
  - `take_preset_job` calling `previous_from_history` under the lock, which reads the worker-only
    `preset_cursor` on the worker thread;
  - `sequential_preset_file` with `backward`, the step `count - 1` modulo arithmetic;
  - `unregister_hotkeys` as the first statement of `destroy`;
  - the deviation from the plan's "set `preset_duration` ≤ 0" (see *What changed and why*).
- **How to run:** see *Files* in `evidence/P2-008/P2-008-hotkeys.md`. The scripts need the P2-006
  fixture root (`presets-a`, `presets-b`), plus `presets-clean` beside the shots directory.
