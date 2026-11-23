# Plugin preset loading — settings, auto-cycle and hotkeys to a committed preset

> Part of the how-x-works corpus — see [`../HOW-X-WORKS.md`](../HOW-X-WORKS.md) for the index.
> Where it ends: [`plugin-render-frame.md`](plugin-render-frame.md), step 3.2. Thread table:
> `docs/architecture.md`. Limits: `docs/concerns.md`.

How a preset change travels from one of three triggers (a setting, the core's auto-cycle timer, a
hotkey) through the worker thread to a commit on the render thread. Citations are
`plugin/src/visualizer-source.cpp` unless noted.

## 0. The pieces

- **Worker**: one `os_task_queue` per source (`preset_worker`), created in `visualizer_create`.
  Every trigger queues the same task, `prepare_requested_preset`.
- **Under `preset_mutex`**: the request (`preset_folder`, `preset_file`, `pending_command`),
  `preset_generation` / `preset_handled_generation`, `preset_target` (last path handed to
  render), the `ready_preset` slot (handle, path, blend), the cycle latch mirror
  (`cycle_requested`, `cycle_deferred`, `cycle_paused`, `cycle_blend`), the transition settings and
  `preset_history`.
- **Worker-only**: `preset_cursor` (last resolved path), `preset_random`.
- The lock is never held across `sv_prepare_preset`, `sv_commit_preset` or a free.

## 1. Trigger: a setting

`visualizer_update` → `request_preset` (`:494-511`):

1. Read `preset_folder` and `preset` (the file name, or `""` for "(first preset)").
2. Under the lock: identical to the stored request (after the first) → return. A folder change
   clears `preset_history`. Store the request, clear `pending_command`, bump `preset_generation`.
3. Queue `prepare_requested_preset`.

`configure_auto_cycle` (`:620-633`) in the same `update` stores cycle (0–3600 s), blend (0–30 s) and
random order and sets `transition_dirty`; the render applies it (`apply_transition_config`).

## 2. Trigger: the core's auto-cycle timer

The core asks after `blend + cycle` seconds by calling `preset_requested` (`:513-533`) from inside
`sv_render_frame` (render thread):

- Under the lock: return if closing; `cycle_requested = !paused`, `cycle_deferred = paused`, store
  the suggested blend.
- Paused → log `auto-cycle request deferred while paused (…)`, queue nothing.
- Otherwise log `auto-cycle requested a preset (timer|hard cut, blend … s)` and queue the worker.

The core latches the request until a load succeeds or fails, so the plugin must answer it or
clear it on a failure (step 5).

## 3. Trigger: a hotkey

Hotkey callbacks (`:573-595`) run on OBS's hotkey thread:

- Next / Previous / Random → `request_preset_command` (`:535-546`): under the lock set
  `pending_command` and bump the generation; log `hotkey: … preset`; queue the worker.
- Pause → `toggle_auto_cycle_pause` (`:548-571`): flip `cycle_paused`. On resume with a deferred
  request, turn it into a live one (`cycle_requested = true`) and queue the worker, so an overdue
  change happens at once. The transition config is not touched.

## 4. Worker: take a job, pick a file

`prepare_requested_preset` (`:422-492`):

1. `take_preset_job` (`:317-342`), under the lock: return if closing. A **pick** is pending when
   `preset_handled_generation != preset_generation`; otherwise serve a cycle request only if
   `cycle_request_servable` (`:283-286`: latched and no ready slot). Copy the latest request
   (bursts coalesce: superseded requests are never prepared), choose the blend (cycle → suggested
   blend; a pick with no previous target → 0; a pick → the "Blend time" setting), take the hotkey
   command, and for Previous resolve `previous_from_history` (`:294-301`). Mark the generation
   handled.
2. `list_preset_files` (`:227-243`): non-hidden `.milk` files, case-insensitive sort. Empty →
   `log_missing_presets` (`:261-269`, "not set" / "not found" / "has no .milk presets") for a pick;
   return. The committed preset keeps rendering.
3. Resolve the path:
   - hotkey command → `command_preset_path` (`:406-420`): Previous = history entry, else sorted
     order backwards; Random = `random_preset_file`; Next = `next_preset_file`. Logs `hotkey picked
     the … preset: … [(history)|(sorted order)]`.
   - setting pick → `listed_or_first` (`:256-259`): the named file if listed, else the first. If
     that equals `preset_target`, the pick is a no-op: `switch_to_cycle_job` (`:344-355`) turns it
     into the waiting cycle job, if any; otherwise return.
   - cycle job → `next_preset_file` (`:399-404`): sequential (`sequential_preset_file`, `:357-375`,
     from the cursor, skipping the current target) or random (`random_preset_file`, `:377-397`, never
     the target or the cursor). Logs `auto-cycle picked the next|random preset: …`.
   - Every branch updates `preset_cursor`.
4. `sv_prepare_preset(ctx, path, &prepared, error, …)` (the only `sv_*` call on this thread).
   Failure → `preset failed to prepare: … (<core error>)`, `clear_cycle_request` under the lock,
   return.

## 5. Worker: publish to the ready slot

Under the lock (`:470-485`):

- Superseded (closing, or a newer generation arrived) → the new handle is freed after the lock;
  log `preset prepared but superseded, dropped`.
- Otherwise swap it into `ready_preset` with its path and blend, set `preset_target`, and
  `clear_cycle_request`. Any replaced handle is freed after the lock. Log `preset prepared: …
  (… ms)`.

`cycle_requested` is cleared at exactly the three places the core releases its latch: a
non-superseded prepare success (`:483`), a prepare failure (`:466`) and the render taking the slot
(`:857-858`). A superseded cycle prepare leaves it set; the superseding pick's commit answers the
core.

## 6. Render: commit

`commit_ready_preset` (`:846-880`) at the start of `render_core_frame`: take the slot under the lock,
`sv_commit_preset` inside `with_core_gl_state`, free the handle, then on success
`remember_committed_preset` (history of 50, no consecutive duplicates) and `preset committed: …
(blend … s, … ms)`. On failure `preset_target` is cleared, so the next pick commits as a hard cut.
Commit costs 0.9–3 ms for most presets and up to 17.8 ms for heavy ones (GL program build); at
1080p60 (P2-012) commits reached 20.7 ms with only 1 skipped frame in 17 446.

## 7. Teardown

`stop_preset_worker` (`:635-649`) sets `preset_closing` (every later trigger returns early),
`os_task_queue_destroy` drains and joins (queued tasks see `preset_closing` and return; an in-flight
prepare finishes, is dropped as superseded and freed), then frees `ready_preset`. It runs before
`sv_destroy`, because the core forbids destroying a context during a prepare.

## Edge cases

- The Preset list does not follow auto-cycle or hotkeys.
- A request that arrives while the folder is empty stays latched until the next pick or settings
  change.
- A hotkey press on a hidden source commits on that source's next render.
- Pause never sets the core's `preset_duration` to 0: a manual pick would then arm the core's
  disabled-timer default (16 s plus up to 10 s random).

**Analysis date:** 2026-11-23 (core `v0.7.0`, libobs 31.1.1, OBS 33.0.3).
