# REVIEW-RECHECK — OBS runtime re-check of the review fixes

Date: 2026-11-23. OBS 33.0.3, obs-websocket 5.7.6, macOS arm64. Build: the REVIEW-FIX bundle in
`plugin/build_macos_arm64` (binary 08:36, newer than both plugin sources at 08:35), not rebuilt.
Driven over the local obs-websocket only, no GUI automation, no OBS config files edited.

## Install

- Graceful quit (`osascript … quit`), then `ditto` of the bundle into the OBS plugins folder,
  10 presets and 23 textures copied, `codesign --force --sign -`, `codesign --verify` → ok.
- `nm -gU` → exactly the 7 `_obs_module_*` exports (`free_locale`, `get_string`, `load`,
  `set_locale`, `set_pointer`, `unload`, `ver`).
- `strings | grep -c '^frame %u'` on the installed binary → 0 (B2 format string gone).
- `open -a OBS`; the websocket was up 2 s later.

## Run (websocket script, ~142 s)

| Step | Result |
|---|---|
| `SetCurrentSceneCollection synesthesia-spike` | ok; program scene before = `BS-008` |
| `Scene` items | visualizer, `SPK-005 Image`, `SPK-005 Colour`, `SPK-005 Text` — all already enabled, nothing toggled |
| `SetCurrentProgramScene Scene` | ok |
| `SetInputSettings preset_path=Fractopia` | ok (was `""`) |
| `GetSourceScreenshot` scene + visualizer at t0 and t0+4 s | ok, different frames |
| 4 program-scene switches `Scene` ↔ `SPK-005 Other` | 4 × ok |
| 2 hide/show cycles of the visualizer item | 4 × ok |
| Scene screenshot after the cycles, and at the end of the soak | ok, still animating |
| `GetStats` (6 samples, on air) | 6.63–7.66 ms average render, 30.0 fps, 1 skipped of 4870 frames, CPU 2.2–3.2 % |
| Restore | `preset_path` → `""` (read back `""`), program scene → `BS-008` (read back), collection → `Untitled` (read back) |

Full run output: `REVIEW-RECHECK-websocket-run.txt` (git-ignored, scratch paths elided).

## Log criteria (`REVIEW-RECHECK-obs-log.txt`, git-ignored, home dir and LAN address scrubbed)

| Criterion | Result |
|---|---|
| `renderer check passed (OpenGL, device type 1)` (N4 guard path) | **pass** |
| `plugin loaded successfully (version 0.1.0)`, listed in Loaded Modules | **pass** |
| `sv_init_gl and sv_create succeeded (1920x1080)` (N3 guard path, resources found) | **pass** |
| GL error lines (`failed` / `GL_INVALID`) from the plugin | **pass — 0.** The grep hits 6 lines, all non-GL: `aja`, `aja-output-ui`, `decklink` init, 2 decklink locale lines, and `Failed to load 'en-GB' text for module: 'synesthesia-vibe-obs'` — the plugin ships only `en-US.ini`, OBS falls back; the same line is in the SPK-008 logs, so not a regression |
| `frame N, … fps` lines (B2) | **pass — 0** |
| Visuals correct beside text/colour sources (N1) | **pass** — see screenshots |
| `plugin unloaded` on quit | **pass** |
| `Number of memory leaks: 0` | **pass** |

Other plugin lines: `preset loaded: …Fractopia…` three times. 08:39:54.994 is the default preset
queued at create (the default is Fractopia), loaded on the first on-air render. 08:39:55.024 is
the explicit `preset_path`. 08:42:03 is the reset to `""`, which falls back to the default. The
same reset behaviour is in the SPK-008 log. All loads report OK, no warnings.

## Screenshots (git-ignored)

- `REVIEW-RECHECK-scene-t0.png` — whole `Scene` (portrait, 720 px): Fractopia (violet/olive
  phase) inside the visualizer frame, green colour source over its corner, the red/yellow/blue
  image source and the `SPK-005 text source` text, all drawn correctly.
- `REVIEW-RECHECK-scene-t4s.png` — same scene 4 s later, a different frame (white/blue phase).
- `REVIEW-RECHECK-scene-after-cycles.png` — after 4 scene switches and 2 hide/show cycles:
  cyan phase, composition unchanged.
- `REVIEW-RECHECK-scene-end.png` — end of the ~2 min soak: yellow phase, composition unchanged.
- `REVIEW-RECHECK-visualizer-t0.png`, `REVIEW-RECHECK-visualizer-t4s.png` — the visualizer source
  alone (960 px), two different frames.

The composition matches `evidence/SPK-008/SPK-008-scene.png` (before the N1 change): no texture
or colour corruption in the other sources, no black or stale visualizer frames.

## Cleanup (S-012) — `REVIEW-RECHECK-cleanup.txt` (git-ignored)

- Graceful quit; log shows `plugin unloaded` and `Number of memory leaks: 0`.
- Checked the bundle id (`io.github.lee5783.synesthesia-vibe-obs`), then removed only
  `~/Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin`. The
  `plugins/` folder is left, empty.
- Relaunched with plain `open -a OBS`. New log: 0 lines containing `synesthesia`;
  obs-websocket `Module loaded` and `Server started successfully`.
- Websocket `GetVersion` ok (OBS 33.0.3, obs-websocket 5.7.6); `GetInputKindList` 16 kinds,
  `synesthesia_vibe_visualizer` absent; current collection `Untitled`. OBS is left running.

## Notes

- `osascript -e 'tell application "OBS" to quit'` returned `User canceled (-128)` both times,
  but OBS exited within 2 s and both logs end in a normal shutdown with 0 leaks.
