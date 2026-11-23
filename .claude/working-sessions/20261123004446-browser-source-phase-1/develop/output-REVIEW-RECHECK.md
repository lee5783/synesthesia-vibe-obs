# REVIEW-RECHECK

## Subtasks executed

- Read the current OBS state over the websocket: collection `Untitled`, plugin not installed.
- Quit OBS gracefully. Reinstalled the REVIEW-FIX bundle from `plugin/build_macos_arm64` as is,
  plus the 10 presets and 23 textures. Ad-hoc signed and verified it, then checked the exports.
  Ran `open -a OBS`.
- Websocket run of about 142 s:
  - switched to `synesthesia-spike` and recorded the program scene (`BS-008`);
  - set `Scene` as the program scene and `preset_path` to Fractopia;
  - checked that the text, colour and image sources were enabled (they all were);
  - took scene and visualizer screenshots 4 s apart;
  - did 4 program-scene switches and 2 hide/show cycles of the visualizer item;
  - sampled `GetStats` during a soak of about 2 minutes;
  - restored `preset_path` to `""`, the program scene to `BS-008` and the collection to
    `Untitled`, and read each one back.
- Quit gracefully and checked the unload and leak lines.
- S-012 cleanup: removed only the plugin bundle, relaunched with plain `open -a OBS`, and
  confirmed from the log and the websocket that the plugin is absent and the websocket is up.

## Files changed

- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/input-dev.md` (REVIEW-RECHECK section appended)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/output-REVIEW-RECHECK.md` (this file)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/REVIEW-RECHECK/review-recheck-report.md`
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/REVIEW-RECHECK/REVIEW-RECHECK-*.png` (6, git-ignored)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/REVIEW-RECHECK/REVIEW-RECHECK-obs-log.txt` (git-ignored)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/REVIEW-RECHECK/REVIEW-RECHECK-websocket-run.txt` (git-ignored)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/REVIEW-RECHECK/REVIEW-RECHECK-cleanup.txt` (git-ignored)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/session.log` (appended)
- Outside the repo:
  - The plugin bundle was installed, then removed.
  - In `synesthesia-spike`, `preset_path` and the program scene were changed and then restored.
- Not touched:
  - `plugin/` source and `plugin/build_*`;
  - `external/`;
  - OBS config files;
  - `docs/`.

## What changed and why

No source changed. This run checks the REVIEW-FIX build in OBS (Gate 1.5 evidence for items 2,
6 and 7). All pass criteria are met:

| Criterion | Result |
|---|---|
| `renderer check passed (OpenGL, device type 1)` | pass |
| `plugin loaded successfully (version 0.1.0)` | pass |
| `sv_init_gl and sv_create succeeded (1920x1080)` | pass |
| 0 GL error lines (`failed` / `GL_INVALID`) | pass: no GL lines. The 6 grep hits are 5 stock-module lines (aja, decklink) and the plugin's `en-GB` locale fallback, which was already in SPK-008. |
| No `frame N, … fps` lines (B2) | pass: 0 in the log, and 0 `^frame %u` strings in the binary |
| Visuals correct next to the text, colour and image sources (N1) | pass: 4 scene screenshots match the SPK-008 composition, and the preset animates across switches, hide/show cycles and the soak |
| `plugin unloaded`, `Number of memory leaks: 0` | pass |
| Cleanup: plugin absent, websocket up | pass |

Performance on air: 6.63–7.66 ms average render time, 30.0 fps, 1 skipped frame out of 4870.

## Verification

- `codesign --verify` returned ok.
- `nm -gU` lists the 7 `_obs_module_*` names.
- `strings | grep -c '^frame %u'` returns 0.
- Every websocket request returned ok. The restored values read back as `preset_path=""`,
  program scene `BS-008` and collection `Untitled`.
- The log greps are in `REVIEW-RECHECK-obs-log.txt`.
- No build was run (code frozen). No existing test suite covers this area.

## Evidence

- `evidence/REVIEW-RECHECK/review-recheck-report.md` (tracked): the procedure, the criteria
  table, the screenshot descriptions and the cleanup.
- `evidence/REVIEW-RECHECK/REVIEW-RECHECK-scene-t0.png` and `-scene-t4s.png`: the whole scene,
  4 s apart, with Fractopia in two different phases next to the colour, image and text sources.
- `evidence/REVIEW-RECHECK/REVIEW-RECHECK-scene-after-cycles.png`: the scene after 4 switches
  and 2 hide/show cycles, still animating and composed correctly.
- `evidence/REVIEW-RECHECK/REVIEW-RECHECK-scene-end.png`: the scene at the end of the soak.
- `evidence/REVIEW-RECHECK/REVIEW-RECHECK-visualizer-t0.png` and `-visualizer-t4s.png`: the
  source alone, two different frames.
- `evidence/REVIEW-RECHECK/REVIEW-RECHECK-obs-log.txt`: the trimmed OBS log, with the home
  directory, LAN address and port scrubbed.
- `evidence/REVIEW-RECHECK/REVIEW-RECHECK-websocket-run.txt`: the run output.
- `evidence/REVIEW-RECHECK/REVIEW-RECHECK-cleanup.txt`: the removal, the relaunch log lines and
  the websocket check.

## Known limitations / follow-ups

- **N1 shows no visible difference, as REVIEW-FIX predicted.** The output looks like SPK-008,
  which is the expected result. No GL-state diff was instrumented, because the code is frozen.
- **Each preset change logs `preset loaded` twice when the source goes on air with a new path in
  the same moment.** The default preset queued at create loads on the first render, then the
  explicit path loads. Here both are Fractopia, so it is harmless. Phase 2 could skip reloading
  a path that is already loaded.
- **The plugin ships only `en-US.ini`.** OBS logs a locale fallback line on non-en-US systems.
  Doc gap: note it in `docs/concerns.md`, or add locales in Phase 2.
- `osascript … quit` returned `User canceled (-128)` both times, but OBS exited cleanly within
  2 s.
- Unchanged from SPK-008:
  - the `synesthesia-spike` collection still exists for the user to delete;
  - the empty `plugins/` folder is left in place;
  - the user should turn websocket authentication back on.

## Handoff for downstream review

- **Acceptance criteria addressed:** the reinstall, load, render and cleanup checks for
  REVIEW-FIX items 2 (B2), 6 (N3/N4) and 7 (N1). All pass.
- **Areas to scrutinize:** none in code. The source was frozen and not rebuilt.
- **How to re-run:** use the install commands in `output-SPK-007.md` *Known limitations*, then
  run the websocket steps listed in the evidence report.
