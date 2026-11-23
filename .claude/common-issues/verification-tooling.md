# Verification tooling (headless Chrome, OBS scripting)

> Part of the known-issues corpus — see [`../COMMON-ISSUES.md`](../COMMON-ISSUES.md) for the
> index. **Analysis date:** 2026-11-23 (Chrome 157 on macOS, OBS 33.0.3). How verification is done
> overall: `docs/testing.md`.

## Chrome's fake audio file delivers silence on macOS

- **Symptom:** headless Chrome started with `--use-fake-device-for-media-stream
  --use-file-for-fake-audio-capture=<wav>` gives the page a running audio input, but the PCM is all
  zeros and the engine's audio levels stay at 0. Without the file option the default fake beep
  arrives, so it looks like the WAV is the problem.
- **Root cause:** the sandboxed audio service cannot read the WAV file.
- **Fix / workaround:** add `--disable-features=AudioServiceSandbox` to the Chrome flags. Also pass
  `--use-fake-ui-for-media-stream` (no permission prompt) and
  `--autoplay-policy=no-user-gesture-required` (no click needed).
- **How to verify this is still current:** run once with and once without the feature flag and
  compare the PCM peak the page receives.

## `osascript -e 'tell application "OBS" to quit'` returns `User canceled (-128)`

- **Symptom:** the quit command exits non-zero with error -128, so a script treats the quit as
  failed.
- **Root cause:** not investigated. The error is cosmetic: the quit itself proceeds. OBS exited
  within 2 s each time, and its log ended with a normal shutdown and `Number of memory leaks: 0`.
- **Fix / workaround:** ignore the exit status; instead wait until the OBS process is gone (e.g.
  poll `pgrep -x OBS`) before copying a new plugin build or relaunching.
- **How to verify this is still current:** run the quit and check the exit status, the process list
  and the end of the OBS log.

## Removing a source, or switching collection, while its properties dialog is open wedges OBS

- **Symptom:** after `OpenInputPropertiesDialog`, a `RemoveInput` on that source returns OK but the
  input stays in `GetInputList`. The next `SetCurrentSceneCollection` never answers. The OBS log
  shows `Not all sources were cleared when clearing scene data: - <source>`. OBS shows two modals,
  "There are unsaved changes. Do you want to keep them?" and "Source Cleanup Error … OBS Studio
  will now exit". `osascript … quit` then really fails with -128 (the entry above does not apply).
  A SIGINT closes OBS but it aborts in Qt widget teardown (no plugin frames in the crash report).
  The next launch stops at the "did not shut down properly / Run in Safe Mode?" prompt, so the
  websocket stays down until someone clicks "Run in Normal Mode". `--disable-shutdown-check` did
  not skip that prompt on OBS 33.0.3.
- **Root cause:** OBS's properties view writes a setting on open when a list's stored value is
  not among its items ("trigger a settings update if the index was not found",
  `shared/properties-view/properties-view.cpp`, condition `count && idx == -1`). That marks the
  dialog dirty and applies the change live. The removal closes the dialog, which finds unsaved
  changes and waits on the prompt while still holding a reference to the source.
- **Which visualizer states still dirty the dialog** (analysis 2026-11-23):
  - **A folder-only source (folder set, `preset` empty) no longer does (P2-010b).** The Preset
    list starts with "(first preset)" whose value is `""`, so the stored empty value matches
    item 0 (`preset_folder_modified`, `plugin/src/visualizer-source.cpp`). Confirmed in OBS on
    2026-11-23: 1.5 s after `OpenInputPropertiesDialog` the settings were unchanged. A dialog
    made dirty some other way (the triggers below, or an unsaved edit) still wedges OBS on
    removal, so close it first.
  - A stored `preset` whose file was later deleted or renamed: the dialog writes `""` and switches
    the source to the first preset.
  - An off-step `render_scale` (a value such as 60, set through the websocket): the dialog writes
    100. Likewise an `audio_mix_track` outside 1–6: the dialog writes Track 1.
  - An empty folder (no list items) writes nothing.
- **Fix / workaround:** in test scripts, open the properties dialog only on a source whose
  settings all match their list items: `preset` empty or naming a file that exists, `render_scale`
  50 / 75 / 100, `audio_mix_track` 1–6. Never remove an input or switch scene collection while a
  properties dialog could still be open; ask the user to close it first, or quit and relaunch OBS
  instead. If it does wedge, stop and ask the user to answer the prompts. There is no websocket or
  Apple Event way out.
- **How to verify this is still current:** repeat the P2-010b check (`docs/testing.md`,
  *Done-when run, 2026-11-23*): create a visualizer with only `preset_folder` set, open its
  properties dialog, and after about 1 s `GetInputSettings` must still show no `preset` written,
  with "(first preset)" selected. Do **not** remove it while the dialog is open. To reproduce the
  wedge itself, use one of the remaining triggers, e.g. a stored `preset` naming a deleted file,
  then `RemoveInput` and check `GetInputList` after 1 s, and be ready to ask the user to answer
  the prompts.

## `RemoveInput` on a source in a scene that is not rendering leaves it alive

- **Symptom:** `RemoveInput` returns OK, but the source is still in `GetInputList`, its `destroy`
  never runs, and nothing it owns is freed. A removal test (no crash, worker stopped, audio
  detached) then passes without testing anything.
- **Root cause:** libobs marks the source removed, but prunes the scene item only in
  `scene_video_render`. A scene that is not being rendered (not in program or preview) keeps the
  item, and so keeps a reference to the source.
- **Fix / workaround:** run removal tests in the program scene, or make the test scene current
  first. Confirm with the plugin's `preset worker stopped (… ms)` log line, which only `destroy`
  prints.
- **How to verify this is still current:** put a visualizer in a scene that is neither program nor
  preview, `RemoveInput` it, and check `GetInputList` and the log for `preset worker stopped`.
