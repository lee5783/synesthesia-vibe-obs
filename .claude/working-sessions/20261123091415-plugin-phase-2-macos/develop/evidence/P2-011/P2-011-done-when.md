# P2-011 — Done-when run on the integrated build

Run on 2026-11-23, in the Phase 4 follow-up. Session log:
`.claude/working-sessions/20261123145422-phase-4-release-linux/session.log`.

**Result: pass.** Every check below passed. OBS never crashed and never logged a GL error. Both
quits logged `plugin unloaded` and then `Number of memory leaks: 0`.

## Setup

- **OBS:** 33.0.3 (mac), the user's `Untitled` profile, canvas 1080×1920 at 30 fps, output
  720×1280. No stream, recording or virtual camera was running.
- **Collection:** the test collection `synesthesia-spike`. Its `Scene` holds the visualizer next to
  an image, a colour and a text source; `SPK-005 Other` holds the text source alone.
- **Session A build:** the production plugin built from `4c260b5` (branch
  `feature/phase-4-release-linux`, which carries Phase 2 plus the Phase 4 GL-loader split).
  It exported 7 symbols, was arm64 only and passed `codesign --verify`. Dev install per
  `docs/integrations.md`.
- **Session B build:** the same source plus the scratch levels log, built only for this run.
  - `P2-011-audio-debug.diff`: 19 added lines, logging only.
  - The Phase 2 patch script's `load_pending_preset` anchor no longer exists, so
    `P2-011-add-audio-debug.py` anchors on `remember_committed_preset` instead.
  - The source was restored from git right after the build. The production build was rebuilt
    at the end.
- **Fixtures:**
  - Audio: `RT Beat`, a looping Media Source on `beat.wav` (C-019), with monitoring off.
  - Presets: the core's 10 tested presets plus their textures, in a folder outside the repo.
- **Driver:** obs-websocket only. OBS was quit with `osascript` and relaunched with `open -a OBS`,
  both authorised by the user for this run. The one dialog opened was closed by the user.

## Session A (production build)

| Check | Result |
|---|---|
| Load (TC-M03) | `renderer check passed (OpenGL, device type 1)`, `plugin loaded successfully (version 0.1.0)`, `sv_init_gl and sv_create succeeded (1920x1080)` |
| GL state | Correct in every frame (`P2-011-gl-state-sheet.png`): the visualizer with the image, colour and text sources; a switch to `SPK-005 Other` and back; the visualizer hidden and shown; a second instance (960×540) added and removed. |
| Live presets | A named preset, then `""` (the first file), then a named preset with a 0.5 s blend: each logged `prepared` and then `committed` with the new blend. |
| Live transitions | 5 s cycle, 1 s blend, random order: a request every ~6 s, 7 in 40 s, each a random pick (`P2-011-a-cycle5-run.txt` and the plugin log). |
| Race stress (P2-002 repro, 300 s) | 5882 of 5882 screenshots OK at 19.5 per second (p50 24 ms, p95 34 ms, max 152 ms). 560 other-item toggles and 28 visualizer hide/show cycles. `renderSkippedFrames` stayed at 2 (start and end). Average render time 6.13 ms at the start, 5.47 ms at the end. Memory 286 → 273 MB. |
| Remove with audio attached and a prepare queued | 6 cycles: create a 640×360 source with audio attached, request a second preset, `RemoveInput` at once. Each logged `audio source detached` and `preset worker stopped`; the queued prepare never ran. No crash. |
| Quit | The main visualizer had audio attached and a preset requested 150 ms before the quit. The log shows `plugin unloaded`, then `Number of memory leaks: 0`. No new crash report (the newest `.ips` is from the Phase 2 incident at 12:21). |
| GL errors | 0 lines in the whole session log. |

The 1080p60 measurement (P2-012) ran in this session, between the stress test and the removal
test: `../P2-012/P2-012-1080p60.md`.

**One observation, not attributed.** `renderSkippedFrames` went from 3 to 16 between the end of
P2-012 and the end of the removal test. Two candidates:
- the profile switch back, which resets video;
- the six create/remove cycles, since `create` holds the graphics lock for `sv_init_gl` and
  `sv_create`.

No GL error and no visible fault came with it.

**First run of the transition check was too short.** It turned the cycle on (5 s) 4 s after a
commit made with the cycle off. With the cycle off, the core arms the next change at
load + blend + 16 s + a random 0–10 s, and turning the cycle on does not re-arm it. So no request
fell in the 17 s window, as documented in `docs/concerns.md` (*Transition timing*). The rerun
committed a preset after setting the cycle and showed the 5 s cycle (row above).

## Session B (scratch audio-levels build)

Levels come from the core's debug callback (C-010). Each window is several seconds long, and the
table shows the highest values in it. "Frames per sample" is how many frames the plugin pushed
between two callbacks.

| Window | bass / mid / treb max | frames per sample | peak |
|---|---|---|---|
| Direct `RT Beat`, playing | 4.95 / 3.10 / 1.27 | 27 648 | 0.862 |
| Direct, media paused | 0 / 0 / 0 | 0 | 0 |
| Direct, source muted | 0 / 0 / 0 | 27 617 | 0 |
| Audio source "None", playing | 0 / 0 / 0 | 0 | 0 |
| Whole mix track 1, beat on track 1 | 4.11 / 2.69 / 3.02 | 45 158 | 0.860 |
| Whole mix track 2, beat on track 1 | 0 / 0 / 0 | 45 315 | 0 |
| Whole mix track 2, beat on track 2 | 8.43 / 7.52 / 2.64 | 44 374 | 0.861 |

Both paths react, and each is silent when it should be. The log shows the matching
`audio source attached / detached`, `audio source: none` and `audio mix connected / disconnected:
track N` lines. The P2-010b checks ran in this session too (`../P2-010b/P2-010b-obs-check.md`).

**Cleanup:**
- removed `RT Beat` and the test visualizers;
- restored the main visualizer's settings to their spike-era values
  (`{"audio_source":"","preset_path":""}`);
- switched back to `Untitled`.

The final quit logged `plugin unloaded`, then `Number of memory leaks: 0`, with 0 GL error lines.
The plugin was then uninstalled (C-026) and OBS relaunched, on `Untitled` / `Untitled` at
1080×1920, 30 fps.

## Session C (production build): projectors, multiview and filters

The original P2-011 plan also listed projectors, multiview and filters. Sessions A and B did not
run them, so the user approved a follow-up session on the same production build.

| Check | Result |
|---|---|
| Filters | On the visualizer: colour correction (hue +120, saturation 0.5), crop (200/200/100/100), sharpen. On the image: a colour-multiply filter. All drew correctly, and turning the crop off and on applied live (`P2-011-filters-sheet.png`). |
| Projectors | Windowed source projector on the visualizer, multiview and program projector (`P2-011-projectors-sheet.png`). The source projector animates (mean pixel change 42.6 between captures 1 s apart). The multiview and program projector show the filtered visualizer next to the other sources. |
| Run with projectors open | 60 s of scene switches and visualizer hide/show: 1969 frames, 0 skipped, average render time 10.1 → 10.6 ms (more views drawn). |
| GL errors | 0 lines. |
| Quit | The user chose to quit OBS rather than close the projectors by hand: they were stacked behind other apps' windows, and obs-websocket cannot close a projector. Filters were removed and the collection restored first. The log shows `plugin unloaded`, then `Number of memory leaks: 0`; there was no crash report, and the projectors did not reopen on relaunch. |

`renderSkippedFrames` was already 12 about 15 s after this launch, before any filter or projector
work, across the launch and the switch to the test collection. That fits the unattributed +13 in
session A, which also spanned a video reset: a profile or collection switch can cost frames. No
fault came with it.

## Files

- **Logs:**
  - `P2-011-session-a-plugin-log.txt`, `P2-011-session-b-plugin-log.txt`: the plugin's lines
    plus the profile, collection, video-reset and leak lines from each OBS log.
  - Machine paths are replaced with `<presets>`, `<scratch>` and `~`.
- **Run output:** `P2-011-a-*-run.txt` and `P2-011-b-*-run.txt`.
- **Screenshots:**
  - `P2-011-gl-state-sheet.png`, `P2-011-a01-scene-smoke.png`, `P2-011-a04-scene-back.png`,
    `P2-011-a07-two-visualizers.png`, `P2-011-a11-auto-cycle.png`,
    `P2-011-a14-last-stress-shot.png`;
  - `P2-011-b01-direct-playing.png`, `P2-011-b02-mix-track1.png`.
- **Session C:** `P2-011-session-c-plugin-log.txt`, `P2-011-c-*-run.txt`,
  `P2-011-filters-sheet.png` and `P2-011-projectors-sheet.png`.
- **Scripts:**
  - `P2-011-session-a.mjs` (phases `setup`, `gl`, `live`, `p1080`, `remove`),
    `P2-011-session-b.mjs` (`audio`, `settings`, `dialog`, `cleanup`),
    `P2-011-session-c.mjs` (`filters`, `projectors`, `run`, `cleanup`), `P2-011-cycle5.mjs`;
  - `P2-011-add-audio-debug.py`, `P2-011-sheet.py`.
  - The scripts import `P2-002-ws-session.mjs` from their own folder
    (`../P2-002/P2-002-ws-session.mjs`) and read the websocket URL and password from the file
    named by `OBS_WS_CREDS`.
  - `P2-002-stress.mjs` ran unchanged as `node P2-002-stress.mjs 300 <out-prefix>`.
