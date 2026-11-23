# P2-010b

## Subtasks executed

- **"(first preset)" entry.** `preset_folder_modified` now adds a first list item with the label
  `Preset.First` and the value `""`, but only when the folder holds presets. The file items
  follow it, unchanged.
- **Visibility.** The empty test is now `files.empty()` and no longer the list's item count. The
  extra item would have made the count wrong. The list is still hidden, and the hint shown, for an
  unset, missing or empty folder.
- **Locale.** New key `Preset.First="(first preset)"`, placed right after `Preset`. It follows the
  layout order and the `AudioSource.None` / `AudioSource.Missing` pattern.
- **Build, exports, codesign** rechecked.
- **OBS check not run.** The websocket was unreachable; see *Evidence*.

## Files changed

- `plugin/src/visualizer-source.cpp`: `preset_folder_modified` only.
- `plugin/data/locale/en-US.ini`: one new key.
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/input-dev.md`: P2-010b
  section appended.
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/output-P2-010b.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-010b/P2-010b-build-nm.txt`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/session.log`: log lines.

## What changed and why

**The bug.** OBS's properties view writes a list setting when the dialog opens if the stored value
matches no item. The source is `shared/properties-view/properties-view.cpp`, "trigger a settings
update if the index was not found"; P2-010 has the analysis. With a folder set and `preset` empty,
no item had the value `""`. Just showing the dialog stored the first file name and marked the
dialog dirty. That dirty state is what wedged OBS when a test removed the source.

**The fix.** An empty `preset` now matches the first item, so the view finds an index and writes
nothing.

**What does not change:**

- **Rendering.** The worker resolves `""` through `listed_or_first` to the first file in sorted
  order, as before.
- **Choosing "(first preset)" in the dialog** stores `""`. The worker resolves that to the first
  file. If the first file is already the target, the `preset_target` skip makes it a no-op.
- **Picking a named preset** stores the file name, as before.
- **Setting keys, stored types, defaults and the `update` path** are untouched.

## Verification

- `cmake --build --preset macos-arm64` (from `plugin/`): `** BUILD SUCCEEDED **`. The build uses
  `-Werror`, and no warning or error line came from `plugin/src`.
- `nm -gU` on the built bundle lists 7 names, and they diff clean against
  `plugin/exported-symbols.txt` (comments stripped).
- `codesign --verify` passes on the built bundle.
- The bundle's `Contents/Resources/locale/en-US.ini` holds `Preset.First="(first preset)"`.
- **OBS check pending.**
  - `nc -z 127.0.0.1 4455` was closed on the first try, and again on ten more tries over about
    30 s.
  - The OBS process was running: `OBS --disable-shutdown-check`, about 11 min old, most likely
    still at the crash/safe-mode prompt.
  - As instructed, OBS was not relaunched, quit or killed, and no GUI automation was used.
  - The new build is **not installed** in OBS's plugin folder. The installed copy is still the
    P2-010 build.

## Evidence

- `evidence/P2-010b/P2-010b-build-nm.txt`: the build result, the `nm -gU` exports and their clean
  diff, the bundle locale lines, and `codesign`.
- **Before-fix behaviour** is described from P2-010's evidence (`evidence/P2-010/P2-010-ws-run.txt`,
  `P2-010-crash-summary.txt`). Opening the dialog on a folder-only source changed `preset` from `""`
  to the first file name and left the dialog dirty.
- **Not captured yet:** the after-fix dialog screenshot and the `GetInputSettings` check. The
  websocket was unreachable, as stated under *Verification*. These are mandatory UI evidence and
  stay open until the OBS check below runs.

## Known limitations / follow-ups

- **OBS check to run once the websocket is back** (P2-010b re-delegation, or folded into P2-011).
  Run it in this order:
  1. Confirm `GetSceneCollectionList` shows `Untitled`.
  2. Quit OBS gracefully with `osascript`, ignoring the -128 status, then copy the new build.
  3. Relaunch OBS and switch to `synesthesia-spike`.
  4. Create a visualizer with only `preset_folder` set.
  5. `OpenInputPropertiesDialog`, wait about 1 s, then `GetInputSettings`: `preset` must still be
     `""`. Take a screenshot showing "(first preset)" selected.
  6. Do not remove that source while its dialog is open. Ask the user to close the dialog first.
  7. On a second source, which is never opened in a dialog, check that `SetInputSettings` with
     `preset: ""` and then with a named file both log `prepared` and `committed`.
  8. Check for 0 GL error lines, then restore `Untitled`.
- **Folder change in the dialog.** When the stored file is not in a newly chosen folder, the
  dialog's unmatched-value write now stores `""` ("(first preset)") instead of the first file
  name. The rendered preset is the same first file. The saved value now keeps meaning "first" if
  the folder's contents change later.
- **The list does not follow auto-cycle or hotkeys** (as before, P2-007). With `preset` empty, the
  list shows "(first preset)" while another preset may be playing.
- **The common-issues wedge entry** (`.claude/common-issues/verification-tooling.md`) names the
  folder-only source as the trigger. Once the OBS check confirms the fix, its "Root cause" and
  "How to verify" lines should note the fix: a folder-only source no longer dirties the dialog.
  Removing a source while its dialog is open still wedges OBS when the dialog is dirty for other
  reasons. Left for the orchestrator or document-writer, since it is not confirmed yet.
- **Doc gaps for document-writer:** `plugin/README.md` (P2-014) should mention "(first preset)" as
  the default list entry.
- No unit test added (none requested).

## Handoff for downstream review

- **Acceptance criteria addressed:**
  - An empty `preset` maps to an existing item, so opening the dialog no longer writes a setting.
    This follows from the code; the OBS confirmation is pending.
  - Empty-`preset` behaviour is unchanged: the worker still uses the first preset.
- **Areas to scrutinize:**
  - The `files.empty()` visibility test, which replaced the item count.
  - That `""` is never treated as a real file anywhere else. `request_preset` and the worker only
    pass it through `listed_or_first`.
- **How to run:** build as above, then follow the ordered OBS check under *Known limitations*.
