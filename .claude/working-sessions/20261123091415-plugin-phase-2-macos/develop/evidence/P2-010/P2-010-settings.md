# P2-010 evidence: settings pass

OBS 33.0.3, `synesthesia-spike` collection, user canvas 1080×1920 at 30 fps (unchanged), websocket
only. Driver: `P2-010-settings.mjs` (phases `setup`, `open-fresh`, `folder`, `cleanup`). The run
logs are in `P2-010-ws-run.txt`. `<scratch>` is the session scratchpad.

## Defaults (TC-023)

`GetInputDefaultSettings` for `synesthesia_vibe_visualizer`. The full before/after is in
`P2-010-defaults.txt`.

- Before, on the P2-009 build, 6 of 10 settings had a default:
  `{"audio_mix_track":1,"blend_seconds":2.7,"cycle_seconds":16,"height":1080,"render_scale":100,"width":1920}`
- After, all 10 settings have one:
  `{"audio_mix_track":1,"audio_source":"","blend_seconds":2.7,"cycle_seconds":16,"height":1080,"preset":"","preset_folder":"","random_order":false,"render_scale":100,"width":1920}`

`preset_hint` is an info-text property, not a setting. The four group names hold no settings.

## Fresh source with defaults (TC-004, C-004)

`CreateInput` was called with no settings.

- `GetInputSettings` returns `{}`, and the source is 1920×1080.
- Log: `sv_init_gl and sv_create succeeded (1920x1080)`, then
  `preset folder not set; choose a folder of .milk presets`, then
  `auto-cycle: every 16 s, blend 2.7 s, sequential order`.
- `P2-010-fresh-defaults.png` shows the core's output with no preset (same as before this task).
- `P2-010-props-fresh.png` shows the **Presets** group: "Preset folder" with Browse and the
  warning-styled hint "Choose a folder of .milk presets." The **Transitions** group header starts
  below it.

## Layout

The groups, in order:

| Group (`Group.*`) | Properties |
|---|---|
| Presets | Preset folder (path, directory), Preset (list, hidden while empty), hint (info, warning; shown while the list is empty) |
| Transitions | Change preset every (0 = off) (int, 0–3600, ` s`), Blend time (float, 0–30, step 0.1, ` s`), Random order (bool) |
| Audio | Audio source (list: None, Whole mix, audio sources, "(missing)"), Mix track (list: Track 1–Track 6; shown only for Whole mix) |
| Output | Width, Height (int, 128–8192, ` px`), Render scale (list: 100 % (full), 75 %, 50 %) |

The dialog is 720×612 pt and the websocket cannot scroll it. The screenshots therefore show the
preview, the Presets group and the Transitions header. The rest was checked through the
properties API on the live build:

- `GetInputPropertiesListPropertyItems` on `audio_mix_track` returns `Track 1=1 … Track 6=6`. On
  `render_scale` it returns `100 % (full)=100, 75 %=75, 50 %=50`. On `audio_source` it returns
  `None="", Whole mix="whole_mix", BS-008 Browser=<uuid>`. On `preset` it returns the 10 files of
  the folder. libobs found each of these inside its group (`obs_properties_get` searches groups).
- `P2-010-locale-lookup.txt` resolves every key of the installed `en-US.ini` through OBS.app's own
  `text_lookup`. The unit values keep their leading space (`Unit.Seconds -> [ s]`,
  `Unit.Pixels -> [ px]`). The ` s` suffix rendered in `P2-010-props-before.png`. Only its source
  moved, from a code literal to the locale.

## Live settings still apply (behaviour unchanged)

`P2-010 Folder` was created with a preset folder, `whole_mix` and track 2. Then
`SetInputSettings` set track 3, cycle 5 s and render scale 75 %:

```
audio mix connected: track 2 (44100 Hz stereo)
preset prepared: <scratch>/p2-008/presets-clean/Eo.S. + Geiss - glowsticks v2 02 (Relief Mix).milk (202.0 ms)
preset committed: … (blend 0.0 s, 9.2 ms)
audio mix disconnected: track 2
audio mix connected: track 3 (44100 Hz stereo)
auto-cycle: every 5 s, blend 2.7 s, sequential order
core resized to 1440x810 (source 1920x1080, render scale 75 %, 6.3 ms)
auto-cycle picked the next preset: …/Eo.S. + Phat - CAT Scan (Nirvana flux).milk
```

Screenshots: `P2-010-folder-instance.png` and `P2-010-props-folder.png` (folder path redacted).

## Existing saved settings still load

The spike collection's own `SynesthesiaVibe Visualizer` was saved by older builds. Its settings
`{"audio_source":"","preset_path":""}` load unchanged, and the spike-era `preset_path` is ignored
(C-021). Its lists resolve as above, and it renders (`P2-010-spike-viz-reloaded.png`).
`P2-010-props-before.png` is the same source's dialog on the P2-009 build: flat, with no groups.

## Strings

`P2-010-string-audit.txt`:

- Every label, list item, group title, hint and unit suffix goes through `obs_module_text`.
- The other literals in `plugin/src` are log text, ids or effect names.
- 34 keys are used and 34 are in the ini, with none missing and none unused.

## Build and exports

`P2-010-build-nm.txt`:

- `BUILD SUCCEEDED` with `-Werror -Wshorten-64-to-32` and 0 diagnostics from `plugin/src`.
- `nm -gU` lists exactly the 7 names in `plugin/exported-symbols.txt`.
- `codesign --verify` passes.

## GL errors and leaks

- The session log `2026-11-23 12-15-13` has **0 GL error lines** (`P2-010-plugin-log.txt`).
- **No leak count for the P2-010 build.** OBS did not shut down cleanly; see the incident below.
  The session before the install (P2-009 build, same day 12-08-34) ended with
  `Number of memory leaks: 0`.

## Incident during cleanup (test harness, not the plugin)

1. The cleanup ran `RemoveInput` on `P2-010 Folder` while its properties dialog was open. Opening
   the dialog had already written `preset` = first file, because OBS's properties view writes a
   list whose stored value (`""`) is not among its items. This list behaviour dates from P2-006.
2. The removal asked the dialog to close. The dialog found unsaved changes and showed
   "There are unsaved changes" (`P2-010-incident-unsaved-changes-prompt.png`), still holding a
   reference to the source.
3. `SetCurrentSceneCollection` → `Untitled` then failed:
   `Not all sources were cleared when clearing scene data: - P2-010 Folder`. OBS showed
   "Source Cleanup Error … will now exit" (`P2-010-incident-source-leak-alert.png`). Saving stays
   disabled on that path (`disableSaving` is not decremented), so no collection file was
   overwritten.
4. `osascript` quit returned -128 and OBS stayed up. A SIGINT (OBS's own graceful shutdown path)
   closed it. During `Shutting down` the plugin's destroy ran (`audio mix disconnected: track 3`,
   `preset worker stopped`). Then Qt aborted while deleting the dialog's widgets
   (`P2-010-crash-summary.txt`: SIGABRT in `QObjectPrivate::deleteChildren`, 0 frames from
   `synesthesia-vibe-obs` in any thread).
5. The relaunch (with `--disable-shutdown-check`, which OBS 33.0.3 ignores) stopped at
   "OBS Studio did not shut down properly. Run in Safe Mode?"
   (`P2-010-incident-safe-mode-prompt.png`). The websocket stays down until someone clicks a
   button, and GUI automation is not allowed, so the run stopped there.

The trap and the workaround are recorded in `.claude/common-issues/verification-tooling.md`.

## State left behind

- The P2-010 build is installed and passes `codesign --verify`.
- OBS is running and waiting at the safe-mode prompt.
- Before the incident, the test scene `P2-010` was removed and `P2-010 Fresh` destroyed.
  `P2-010 Folder` was removed from its scene and is not in the saved collection.
- On disk (read only):
  - `user.ini` has `SceneCollection=Untitled`, written at 12:21:03 during the shutdown.
  - `Untitled.json` was last written at 12:15:41, when the run switched away from it.
  - `synesthesiaspike.json` holds no `P2-010` source or scene. Its sources are the visualizer,
    SPK-005 Image/Colour/Text, BS-008 Browser and the scenes Scene, SPK-005 Other and BS-008.
- After "Run in Normal Mode", OBS should start on `Untitled` with the P2-010 build loaded.
