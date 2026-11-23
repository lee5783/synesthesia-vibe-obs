# P2-008 hotkeys: evidence

OBS 33.0.3 (OpenGL, Apple M1), canvas 1080×1920 at 30 fps, collection `synesthesia-spike`, program
scene `Scene`. Driven only through obs-websocket (`GetHotkeyList`, `TriggerHotkeyByName` with
`contextName` = the source name, `SetInputSettings`, `CreateInput`, `RemoveInput`, `GetStats`). No
keys were bound in OBS settings, no GUI automation was used, and no OBS config was edited. Paths
are scrubbed to `<scratch>`.

Fixtures:
- `presets-a`: the core's 10 mvp presets plus `zz broken.milk`, which is not a preset.
- `presets-b`: Fractopia and swarm.
- `presets-clean`: `presets-a` without the broken file.

## C-029: `TriggerHotkeyByName` reaches source hotkeys

Yes. Every trigger returned `ok`, and each one produced a `hotkey: …` line from the plugin. No
manual key press was needed.

```
GetHotkeyList one instance ["SynesthesiaVibe.NextPreset","SynesthesiaVibe.PreviousPreset","SynesthesiaVibe.RandomPreset","SynesthesiaVibe.PauseAutoCycle"]
GetHotkeyList two instances [ …the same four names, twice… ]
```

## Next, previous (C-014) and random

The source uses `presets-a` with cycle 0 and blend 0.5. It starts on swarm. The files sort
case-insensitively, so `shifter - swarm` comes before `Stahlregen`.

```
hotkey picked the next preset: Stahlregen & Flexi + Geiss - Tiger No.5 …       committed
hotkey picked the next preset: Unchained - Fuzzy Sciences                      committed
hotkey picked the next preset: zz broken.milk                                  preset failed to prepare
hotkey picked the previous preset: Stahlregen … Tiger No.5 … (history)         committed
hotkey picked the previous preset: shifter - swarm (history)                   committed
hotkey picked the previous preset: Rovastar - Jester's Calling 2 (sorted order) committed
hotkey picked the previous preset: Rovastar - Fractopia (sorted order)         committed
hotkey picked the previous preset: Goody - Need - Transcendance remix (sorted order) committed
```

- **Previous** walks the commit history back. The current preset and a just-failed file are both
  skipped. Once the history is used up, it steps back through the files in sorted order.
- **Random** (8 presses) gave Tiger → dark side → Tiger → Jester → Tiger → dark side →
  [zz broken: failed] → Unchained. It never repeated the current preset.
- **Random order on:**
  - Next is random: [zz broken: failed] → Now And Later → Fractopia → dark side.
  - Previous then walked the history: Fractopia → Now And Later → Unchained.
- Each hotkey commit took 1.5–21 ms in `video_render`, with blend 0.5 s.

## Pause / resume (C-013)

Run on `presets-clean`, cycle 4 s, blend 0.5 s:

```
11:41:44.073  preset committed: … Tiger No.5 …                    (cycle)
11:41:48.621  preset committed: … Unchained …                     (cycle)
11:41:50.500  hotkey: auto-cycle paused
11:41:53.127  auto-cycle request deferred while paused (timer)
11:42:05.505  hotkey: next preset
11:42:05.764  preset committed: … glowsticks v2 02 …              (manual, still paused)
11:42:10.291  auto-cycle request deferred while paused (timer)
11:42:20.510  hotkey: auto-cycle resumed, serving the deferred request
11:42:20.555  preset committed: … CAT Scan …
11:42:25.121  preset committed: … dark side …                     (cycle, +4.57 s)
11:42:29.655  preset committed: … Now And Later …                 (cycle, +4.53 s)
11:42:32.515  hotkey: auto-cycle paused
11:42:33.519  hotkey: auto-cycle resumed                           (no request fired during the 1 s pause)
11:42:34.240  preset committed: … Goody …                         (cycle, +4.59 s, cadence unchanged)
11:42:38.788  preset committed: … Fractopia …
11:42:43.320  preset committed: … Jester …
```

- **While paused** (30 s, more than 6 cycles), auto-cycle committed nothing. Each core request
  logged one "deferred" line. The core latches after a request, so there are no more requests
  until a load.
- **A manual Next while paused** commits, and the source stays paused.
- **Resume** serves the deferred request at once, then the normal cadence continues.

**Design change during the task.** The first build also forced `preset_duration = 0` while paused,
as the plan suggested. A manual pick while paused then armed the core timer with the core's
disabled-timer default (16 s plus up to 10 s random, `frame.cpp:356-359`). The result: no cycle
in the 12 s after resume (`P2-008-pause-run-v1.txt`; excerpt in `P2-008-plugin-log.txt` at
11:37:04–11:37:49). The final build keeps the user's config and relies on the deferral alone.

## Two instances act independently

Instance A is `SynesthesiaVibe Visualizer` (`presets-clean` in the second half). Instance B is the
temporary `P2-008 Viz B` (`presets-b`). Both cycle every 4 s.

```
hotkey: next preset → <B>/Fractopia committed         (trigger on B)
hotkey: next preset → <B>/swarm committed             (trigger on B)
hotkey: next preset → presets-a/Tiger committed       (trigger on A)
hotkey: random preset → <B>/Fractopia committed       (trigger on B)
11:44:09.649  hotkey: auto-cycle paused               (A)
11:44:13.195  <B>/Fractopia committed                 (B cycling)
11:44:13.227  auto-cycle request deferred while paused (A)
11:44:17.762  <B>/swarm committed                     (B cycling)
11:44:22.296  <B>/Fractopia committed                 (B cycling)
11:44:24.696  <B>/swarm committed                     (hotkey Next on B)
11:44:26.656  hotkey: auto-cycle resumed, serving the deferred request (A)
11:44:26.714  <A:clean>/Tiger committed
11:44:29.263  <B>/Fractopia   11:44:31.230 <A>/Unchained   11:44:33.801 <B>/swarm   11:44:35.809 <A>/glowsticks
```

The first attempt (`P2-008-two-run-v1-old-timer.txt`) was inconclusive. Neither source cycled
during A's pause, because both timers had been armed while cycle was 0. That is the P2-007
"first interval follows the old timer" limit, not this change.

## Removing a source while its hotkeys fire

Ten temporary visualizers ran the same loop:
- each cycled at 1 s;
- each was created, then sent a burst of 24 `TriggerHotkeyByName` (next, random, previous,
  pause);
- `RemoveInput` went into the middle of each burst, without waiting.

- All 250 requests returned ok. libobs defers the destroy, so the presses sent after
  `RemoveInput` still reached the source.
- 240 `hotkey:` lines, 10 × `preset worker stopped`, and no crash.
- `GetInputList` afterwards shows only the collection's original five inputs.
- `destroy` unregisters the four hotkeys first. libobs frees source hotkeys only in
  `obs_context_data_free`, which runs after `info.destroy`, so without this a press could reach
  freed memory. The callbacks run under the libobs hotkeys mutex, which `obs_hotkey_unregister`
  also takes.

## Frames, GL errors, leaks

| Window | Skipped frames |
|---|---|
| Walk (40 s, 32 presses) | 2 → 2 |
| Pause (64 s) | 2 → 2 |
| Two instances, during the hotkeys (36 s) | 5 → 5 |
| 10 bursts of 24 presses on one source (22 s), control | 44 → 44 |
| 10 creates + removes with hotkey bursts | 5 → 29 |
| 10 creates + removes, no hotkeys, control | 29 → 44 |

- **Hotkeys add no skipped frames.**
  - The skips come from creating a visualizer: `sv_init_gl`/`sv_create` run in `create` on the
    graphics thread, about 1.5–2.4 frames per create.
  - This is existing behaviour, flagged for P2-011.
- **The 24-press bursts coalesce.** Each press bumps the generation, so a burst gives one commit
  (10 commits and 10 superseded drops for 240 presses).
- **Both sessions ended cleanly:**
  - `plugin unloaded` and `Number of memory leaks: 0`;
  - 0 GL error lines and 0 `preset failed to commit`;
  - the only plugin warnings were the expected `zz broken.milk` prepare failures and
    `preset folder not set` (at create, and when the cleanup restored the spike settings).

## Cleanup

- The spike visualizer is back to `{"audio_source":"","preset_path":""}` (`overlay: false`).
- The temporary inputs are removed.
- The collection is back on `Untitled`; OBS was quit with 0 leaks and relaunched.
- The P2-008 build stays installed, and `codesign --verify` passes.

## Files

- `P2-008-hotkeys.mjs` takes a phase: `walk`, `pause`, `two`, `remove` or `cleanup`.
- `P2-008-control.mjs` takes a mode: `create` or `burst`.
- `P2-008-ws-session.mjs` is the connection helper. Rename it to `ws-session.mjs` first.
- `P2-008-*-run*.txt` hold the script outputs.
- `P2-008-plugin-log.txt` holds every plugin log line from both OBS sessions.
- `P2-008-before-next.png` and `P2-008-after-3-next.png` show swarm, then Unchained after the
  Next presses.

To run: `OBS_WS_CREDS=<creds file> node P2-008-hotkeys.mjs <phase> <fixture root> <shots dir>`.
`presets-clean` must sit beside the shots dir.
