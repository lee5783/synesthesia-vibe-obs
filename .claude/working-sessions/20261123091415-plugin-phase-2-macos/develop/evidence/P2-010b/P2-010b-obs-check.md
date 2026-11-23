# P2-010b — OBS check

Run on 2026-11-23 in session B of the P2-011 run (`../P2-011/P2-011-done-when.md`). The build was
`4c260b5` plus the scratch levels log, which changes no settings or properties code. The steps
follow the ordered list in `../../output-P2-010b.md`.

**Result: pass.** Opening the properties dialog on a folder-only source no longer writes
`preset`.

| Step | Result |
|---|---|
| Folder-only source | `RT Viz Dialog` created with only `preset_folder` set, then hidden. Settings: `{"preset_folder":"<presets>"}`. |
| Open the dialog | `OpenInputPropertiesDialog`, then `GetInputSettings` 1.5 s later: still `{"preset_folder":"<presets>"}`. Before the fix, `preset` became the first file name at this point. |
| Screenshot | `P2-010b-dialog-first-preset.png` (`screencapture -l <window id>`, no GUI automation): the Preset list shows "(first preset)" selected. |
| Close | The user clicked Cancel. The window was gone from the OBS window list, and the settings were still unchanged. Only after that were the source removed and the collection switched. |
| Never-opened source | `RT Viz Settings` with `preset` unset, then a named file, then `""`. The log shows `prepared` and `committed` for the first file, then the named file (Fractopia), then the first file again (`P2-010b-settings-run.txt`, and `../P2-011/P2-011-session-b-plugin-log.txt` 19:02:14 to 19:02:21). |
| GL errors | 0 lines. |

## Files

- `P2-010b-dialog-run.txt`, `P2-010b-settings-run.txt`: run output (sanitised paths).
- `P2-010b-dialog-first-preset.png`: the dialog with "(first preset)".
- The script is `../P2-011/P2-011-session-b.mjs`, phases `dialog` and `settings`.
