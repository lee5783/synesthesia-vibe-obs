# P2-014 claim check: `plugin/README.md`

No OBS run (as instructed). Line numbers are approximate (±2), taken from the working tree on 2026-11-23. Each claim was checked against source, locale, build output or a
develop handoff. Line numbers are `plugin/src/visualizer-source.cpp` unless noted.

## Verified against code / locale

| README claim | Source |
|---|---|
| Source name "SynesthesiaVibe Visualizer" | `en-US.ini` `SynesthesiaVibeVisualizer`; `visualizer_get_name` (963-966) |
| Groups Presets / Transitions / Audio / Output, in that order | `visualizer_get_properties` (1010-1067), `Group.*` keys |
| Preset folder: directory picker, default empty | 1014-1016, 1095 |
| Only `.milk` directly in the folder, case-insensitive extension, dot-files skipped, sorted case-insensitively | `is_preset_file` (230-234), `list_preset_files` (236-252: `os_readdir`, no recursion, `astrcmpi` sort) |
| List label = file name without `.milk` | `preset_label` (254-258) |
| "(first preset)" first entry, value `""`, shown only when the folder has files; default `""` | `preset_folder_modified` (993-1006), 1096; `listed_or_first` (265-268) |
| Hint "Choose a folder of .milk presets." replaces the list when empty | 1004-1005, `PresetFolder.Hint`, `OBS_TEXT_INFO_WARNING` |
| Empty-folder log lines | `log_missing_presets` (271-278) |
| Cycle: label "Change preset every (0 = off)", 0–3600 s, step 1, default 16 | 75-76, 1027-1030, 1097, clamp 631 |
| Blend: label "Blend time", 0–30 s, step 0.1, default 2.7 | 77-78, 1031-1033, 1098, clamp 632 |
| Blend applies to list picks and hotkeys; first preset of a new source is a hard cut | `take_preset_job` blend selection (340-342: `preset_target.empty() ? 0` else `blend_seconds`) |
| Random order default off; next hotkey honours it; random never repeats the preset on screen (2+ presets) | 1099; `job.random` (343) used by `next_preset_file`; `random_preset_file` excludes `job.target` (386-406) |
| Sequential wraps | `sequential_preset_file` (`index % count`, 366-383) |
| Period = cycle + blend; first interval after a change follows the old timer; re-enabling can fire at once | `output-P2-007.md` (measured 12.0 s for 10/2), core behaviour |
| Hard cuts and random duration off, so the cycle time is exact | `apply_transition_config` (903-904) |
| Audio source: None (default, `""`), Whole mix, then every source with `OBS_SOURCE_AUDIO` by name, stored by UUID | 1039-1045, 1100, `add_audio_source_entry` (968-973), `audio_source_available` (749-752) |
| "(missing)" when the stored UUID is not listed | 1069-1079, `AudioSource.Missing` |
| Muted source = silence | `audio_capture` `write_silence` when `muted` (669-670) |
| Rechecks every second until found; log lines quoted | `audio_check_interval_ns` (72), `resolve_audio_source` / `check_attached_audio_source` (754-780) |
| Silence settles within ~0.5 s | `audio_silence_after_ns` (73), `push_captured_audio` (827-843) |
| Mix track: Track 1–6, default 1, visible only with Whole mix | 1046-1050, 1101, `audio_source_modified` (986-991) |
| Mix connect/fail log lines | `connect_audio_mix` (686-696) |
| Width/Height 128–8192 px, default 1920×1080 | 37-40, 1054-1061, 1102-1103, clamp 937 |
| Render scale 100 % (full) default, 75 %, 50 %; sprite drawn at full source size | 41-43, 1062-1066, 1104; `gs_draw_sprite(..., size.width, size.height)` in `visualizer_video_render` |
| Four hotkeys, exact labels | 63-66, `register_hotkeys` (606-618), `Hotkey.*` keys |
| Previous: history of last 50, then sorted order | `max_preset_history` (83), `previous_from_history` (303-310), `command_preset_path` (414-428) |
| Pause: deferral, manual picks still work, resume serves an overdue request; not persisted | `preset_requested` (522-541), `toggle_auto_cycle_pause` (556-578); state lives only in the source struct |
| Log prefix `[synesthesia-vibe-obs]` | `plugin/build_macos_arm64/plugin-support.c` (`PLUGIN_NAME`, `"[%s] %s"`) |
| Renderer refusal message (verbatim) and "renderer check passed" | `plugin/src/plugin-main.cpp` 17-36 |
| "plugin loaded successfully (version %s)", version 0.1.0 | `plugin-main.cpp` 45; `plugin/buildspec.json` |
| Example log lines' formats | 692, 745, 880, 909, 1142 |
| Bundle name `synesthesia-vibe-obs.plugin` | `plugin/build_macos_arm64/RelWithDebInfo/synesthesia-vibe-obs.plugin` |
| pkg installs for the current user only | `plugin/cmake/macos/resources/distribution.in` line 8 (`enable_currentUserHome="true"`, others false) |
| arm64 only | `plugin/CMakeLists.txt` 13-20 guard |
| Core presets/textures locations, 10 presets, flat textures folder | `external/synesthesia-vibe/resources/mvp-presets/` (10 files), `resources/beatdrop/textures/` (23 files, no subfolders) |

## Verified against handoffs / plan (observed in OBS by earlier tasks)

- Textures found next to the preset; random textures get none: `output-P2-006.md`.
- No-folder source shows a grey wave and mesh: `output-P2-006.md`.
- Folder not watched: `output-P2-006.md`.
- List does not follow auto-cycle/hotkeys: `output-P2-007.md`, `output-P2-010b.md`.
- Whole mix excludes muted and Monitor Only sources and is per track: `output-P2-005.md`.
- Shared PCM buffer across visualizers (observed): `output-P2-004.md`, `output-P2-005.md`, C-006.
- 8192×8192 at 100 % ≈ 21 fps vs 30 fps at 50 % on an M1; resize resets the image ~1 s:
  `output-P2-009.md`.
- Hotkeys work while hidden, presses coalesce, accepted request survives pause:
  `output-P2-008.md`.
- Creating a visualizer skips frames: `output-P2-008.md`.
- Hidden source stops rendering and resumes without a jump: `output-P2-002.md` (delta reset on
  show/activate, 0.1 s cap).
- Tested on OBS 33.0.3, Apple M1, OpenGL: develop evidence logs, `docs/stack.md`.
- Decisions: C-002 (unnotarized), C-003 (no presets bundled), C-006, C-012, C-013, C-014, C-017.

## Not verified in this task

- Packaging artifact names (P2-013 concurrent): written as "the file name from the release".
- Installer behaviour in practice (the pkg had not been built when this was written), including
  whether the installer shows an "Install for me only" choice and the pkg receipt identifier
  (README tells the user to find it with `pkgutil --pkgs | grep -i synesthesia`).
- Gatekeeper wording and steps (macOS 15 "Open Anyway" in Privacy & Security; macOS 14
  right-click → Open) and whether a quarantined zip-installed bundle is actually refused by OBS:
  standard macOS behaviour, not exercised here.
- OBS UI paths not exercised this run: Settings → Advanced → Video → Renderer and the Metal option
  name; Settings → Hotkeys listing source hotkeys under the source name; Add Existing; Browse;
  Edit → Advanced Audio Properties → Audio Monitoring.
- The renderer refusal on a real Metal renderer was never observed (`docs/concerns.md`, renderer
  check); the message text is from code.
- Behaviour of saved visualizer sources after uninstall or under renderer refusal (untested, so the
  README avoids specifics).
- "OBS 31 or later": built against libobs 31.1.1 and checked only on 33.0.3.

## Mechanical checks

- Every `en-US.ini` label appears in the README (Track 2–5 via "Track 1–Track 6"); `Unit.*` keys are
  suffixes.
- No absolute machine paths (only `~/Library/...` and `~/Downloads/<placeholder>`).
- All in-page anchors resolve; relative links `../browser-source/README.md`,
  `../docs/integrations.md`, `../LICENSE` exist.
