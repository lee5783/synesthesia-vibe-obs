# P2-010

## Subtasks executed

- **Grouped properties layout.** Four `OBS_GROUP_NORMAL` groups in a stable order: Presets,
  Transitions, Audio, Output. Each group is built from its own `obs_properties_create()`, as in
  `transition-stinger.c`.
- **`get_defaults` covers all 10 settings.** It now also sets `preset_folder`, `preset` and
  `audio_source` (all `""`) and `random_order` (`false`). The order follows the layout.
- **Every user-visible string is in `en-US.ini`.** That adds the group titles, the unit suffixes
  (` s`, ` px`, which were code literals) and the Mix track items. The ini keys follow the
  layout order.
- **Mix track is a list.** It was an int spinner and is now "Track 1"–"Track 6" (P2-005 note).
  The stored int and its 1–6 clamp are unchanged.
- **`int` casts.** `max_cycle_seconds` and `min_output_size`/`max_output_size` are passed to
  `obs_properties_add_int` through `static_cast<int>` (P2-007 note).
- **Export list rechecked** with `nm -gU`: 7 of 7 match `plugin/exported-symbols.txt`, and no new
  entry point was added.
- **No leftover spike setting.** `preset_path` was removed in P2-006. Saved values are ignored
  (C-021).

## Files changed

- `plugin/src/visualizer-source.cpp`: group and Mix track label constants,
  `visualizer_get_properties`, `visualizer_get_defaults`.
- `plugin/data/locale/en-US.ini`: 10 new keys (`Group.*`, `Unit.*`, `MixTrack.Track1`–`6`), all
  keys reordered to follow the layout.
- `.claude/common-issues/verification-tooling.md`, `.claude/COMMON-ISSUES.md`: new entry for the
  test-harness wedge hit during cleanup. This is the stuck-task exception in the role file.
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/input-dev.md`: P2-010
  section appended.
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-010/*`.

## What changed and why

- **Layout.** The flat 14-row list is now four titled groups:
  - **Presets:** folder, preset, no-folder hint.
  - **Transitions:** cycle, blend, random order.
  - **Audio:** source, mix track.
  - **Output:** width, height, render scale.

  The order follows the delegation's example. TC-023's Audio, Presets and Output groups are all
  there.
- **Callbacks unchanged.** The existing modified callbacks (`audio_source_modified`,
  `preset_folder_modified`) need no change. libobs passes them the topmost properties, and
  `obs_properties_get` searches groups recursively (`obs-properties.c:296-322, 806-818`).
- **Defaults.** `GetInputDefaultSettings` now lists every setting.
- **Strings.** No user-visible text is left in code. The quoted leading space in `Unit.*`
  survives OBS's ini parser; this was checked with OBS.app's own `text_lookup`.
- **Behaviour.** No setting key, stored type, range, clamp or `update` path changed. The empty
  and `false` defaults match what `obs_data_get_*` already returned.

## Verification

- `cmake --build --preset macos-arm64` (from `plugin/`): `** BUILD SUCCEEDED **`, with `-Werror
  -Wshorten-64-to-32` and 0 diagnostics from `plugin/src`.
  - It was rebuilt after rewrapping four lines to 120 columns. `otool -t` gives identical
    `__text`, so the installed build is the same machine code.
- `nm -gU` on the installed bundle lists 7 names, and they diff clean against
  `exported-symbols.txt`.
- `codesign --verify` passes on the installed bundle.
- **OBS 33.0.3**, `synesthesia-spike`, websocket only:
  - **Defaults:** `GetInputDefaultSettings` lists all 10 settings with defaults.
  - **Fresh source:** created with no settings, it is 1920×1080, logs the C-004 warning and
    shows the hint.
  - **Live settings:** a configured source still connects the mix, commits a preset, follows a
    track change, follows a cycle change and resizes on a scale change.
  - **Lists inside groups:** `GetInputPropertiesListPropertyItems` resolves the Mix track,
    render scale, audio source and preset lists.
  - **Old settings:** the spike collection's old visualizer loads its saved settings unchanged.
- The OBS log for the session has **0 GL error lines**.
- **Leak count not obtained** for this build; see the Evidence section. The run stopped at an OBS
  safe-mode prompt that needs a click.

## Evidence

All under `evidence/P2-010/`; `P2-010-settings.md` is the write-up.

- `P2-010-defaults.txt`: `GetInputDefaultSettings` before (6 keys) and after (10 keys), plus the
  spike visualizer's saved settings.
- `P2-010-props-before.png`: the flat P2-009 dialog.
- `P2-010-props-fresh.png`: a fresh source with the Presets group, the C-004 hint and the
  Transitions header.
- `P2-010-props-folder.png`: Presets with folder and preset list; the path is redacted.
- `P2-010-fresh-defaults.png`, `P2-010-folder-instance.png`, `P2-010-spike-viz-reloaded.png`:
  source screenshots.
- `P2-010-string-audit.txt`: every UI call takes `obs_module_text`, the remaining literals are
  classified, and 34 keys are used with 34 in the ini.
- `P2-010-locale-lookup.txt`: every installed key resolved by OBS.app `text_lookup`.
- `P2-010-build-nm.txt`: the build tail, `nm -gU`, the export diff and `codesign`.
- `P2-010-plugin-log.txt`: the plugin's log lines, 0 GL errors.
- `P2-010-ws-run.txt`: the websocket run output.
- `P2-010-crash-summary.txt`, `P2-010-incident-*.png`: the cleanup incident.
- `P2-010-settings.mjs`, `P2-010-lookup-check.c`: the driver and the lookup checker.
- **Not captured:**
  - **The lower groups in the dialog.** The dialog is taller than its window and can't be
    scrolled without GUI automation. The lower groups were checked through the properties API
    instead.
  - **The leak count.** The cleanup removed `P2-010 Folder` while its properties dialog was open.
    The dialog had already written `preset` on open, so it held the source behind an
    "unsaved changes" prompt. The collection switch then failed ("Source Cleanup Error … will
    now exit").
    - `osascript` quit could not get past the modals.
    - A SIGINT closed OBS, and the plugin's destroy ran. Qt then aborted while deleting the
      dialog's widgets, with 0 plugin frames in the crash report.
    - The relaunch stopped at "Run in Safe Mode?", and `--disable-shutdown-check` is ignored by
      33.0.3. The websocket is down until someone clicks, so the run stopped there.

## Known limitations / follow-ups

- **Needs the user now:**
  - Click **"Run in Normal Mode"** in the OBS prompt.
  - Then confirm `GetSceneCollectionList` shows `Untitled`. On disk, `user.ini` already says
    `SceneCollection=Untitled`, and neither collection file holds a P2-010 input.
  - The leak check for this build (quit, then `Number of memory leaks: 0`) moves to P2-011, which
    quits OBS on the integrated build anyway.
- **Properties dialog writes `preset` on open.** When a folder is set and `preset` is empty,
  OBS's properties view selects the first file and writes it, because the stored value is not
  among the items. This has been true since P2-006 and is harmless to rendering (it is the
  implicit first file), but it marks the dialog dirty. Two possible fixes, both behaviour changes
  and so out of scope here:
  - a "(first file)" item with value `""`;
  - writing the implicit pick back in `update`.

  Test scripts must avoid it; see the new common-issues entry.
- **Not built, as instructed:**
  - a "now playing" status line, because the Preset list does not follow auto-cycle (P2-007);
  - a status line for a failed pick, because the list keeps showing a broken file as selected
    (P2-006).
- **Hotkeys while hidden.** Presses still work while the source is hidden; the slideshow sibling
  ignores them (P2-008 note). Behaviour was left unchanged.
- **Mix track list and out-of-range values.** A saved value outside 1–6 (only possible through
  the websocket) clamps on read as before. The dialog would show and write Track 1.
- **Doc gaps for document-writer:**
  - `docs/architecture.md` / `docs/conventions.md`: the group layout and its order; the
    `Group.*` / `Unit.*` / `MixTrack.Track<n>` key conventions; units as locale keys.
  - `plugin/README.md` (P2-014): the settings by group.
  - `docs/testing.md`: never remove an input or switch collection while its properties dialog
    may be open. OBS 33.0.3 ignores `--disable-shutdown-check`.
- `docs/integrations.md` still describes the spike-era install with bundled presets and a
  `preset_path` default. That is stale since P2-006 and C-003, and belongs to P2-016.

## Handoff for downstream review

- **Acceptance criteria addressed:**
  - one `get_properties` with a stable, grouped order;
  - `get_defaults` for every setting;
  - every user-visible string in `en-US.ini`;
  - no leftover spike setting;
  - the export list matches `nm -gU`;
  - no warning from the plugin's sources.
- **Areas to scrutinize:**
  - `mix_track_labels` is sized by `MAX_AUDIO_MIXES` with six literal entries. If libobs ever
    raised the macro, the extra entries would be null.
  - The group property names (`presets_group`, …) must never collide with a setting key.
  - The Mix track change from spinner to list.
- **How to run relevant tests:** build as above. Then, on the running OBS:
  - `GetInputDefaultSettings {"inputKind":"synesthesia_vibe_visualizer"}`;
  - `GetInputPropertiesListPropertyItems` for `audio_mix_track` / `render_scale` on any
    visualizer.

  Open the properties dialog only on a source with `preset` set, or with no folder.
