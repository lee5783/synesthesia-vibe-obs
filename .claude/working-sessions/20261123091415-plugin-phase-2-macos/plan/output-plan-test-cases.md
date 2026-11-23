# Test cases

Scenario-level expectations for manual QA in OBS on macOS (Apple Silicon). The agent drives OBS
through obs-websocket only, in a separate test scene collection. The audio fixture is a looping
Media Source playing a generated WAV, on track 1 (C-019). "No GL errors" means the OBS log has no
`gl…failed` / `device_load_texture (GL) failed` lines.

| ID | Component / area | Preconditions | Expected |
|----|--------------------|---------------|----------|
| TC-001 | Build (`plugin/CMakeLists.txt`) | Clean build dir and no `.CMakeBuildNumber` | `macos-arm64` configures and builds on the first try. The `.plugin` is arm64. `nm -gU` lists exactly the names in `exported-symbols.txt`. A non-arm64, non-Xcode or non-macOS configure fails at configure with a clear message |
| TC-002 | Module load / renderer check | Plugin installed, OBS on OpenGL | Log shows `renderer check passed (OpenGL, device type 1)` and `plugin loaded successfully`. `GetInputKindList` includes `synesthesia_vibe_visualizer` |
| TC-003 | Renderer refusal (user checklist) | OBS switched by the user to the Metal renderer | Log has the "needs the OpenGL renderer" warning. The source is absent from Add Source. OBS runs normally. The user switches back afterwards |
| TC-004 | Source lifecycle: create | New visualizer added with defaults | Log `sv_init_gl and sv_create succeeded (1920x1080)`. Source size 1920×1080. With no preset folder (C-004), opaque output plus one warning, and the properties show the hint |
| TC-005 | Thread safety: screenshot stress | Visualizer on air plus image, colour and text sources; P2-002 stress script with the scratch re-entrancy detector | 5 min at about 20 `GetSourceScreenshot`/s with item toggles: no crash, 0 overlap reports, no GL errors, render time within noise of the spike |
| TC-006 | Off-air resume (`dt` reset/cap) | Visualizer hidden for 10 s or more | First screenshot after showing it is a normal frame, not black. Animation resumes without a visible jump |
| TC-007 | Opaque draw | Preset swarm loaded; the visualizer's scene nested over a colour source | Screenshot shows the visualizer fully opaque (minimum alpha 255). Fractopia's orientation and colours are unchanged |
| TC-008 | Audio: chosen source | Audio source = the Media Source fixture | Debug levels (scratch build) and visuals react while unmuted. Muting the Media Source gives silence-level output within about 1 s (C-008) |
| TC-009 | Audio: switch or none | Audio attached | Switching to another source or "None" logs detach then attach. Only one producer is active. No crash |
| TC-010 | Audio: source deleted / late creation | Chosen Media Source removed while running; scene collection reloaded with the visualizer listed before its audio source | Removal: no crash, one "audio source gone" log, silence. Reload: audio attaches within about 1 s of the source appearing (C-005) |
| TC-011 | Audio: whole mix | Audio = Whole mix, track 1; Media Source on track 1 | Visuals react. Moving the Media Source off track 1, or choosing track 2, gives silence. Switching back reacts again |
| TC-012 | Audio: mic / desktop audio (user checklist) | User grants mic and/or Screen Recording permission | A mic or macOS Audio Capture source chosen in the list drives the visuals |
| TC-013 | Presets: folder and list | Preset folder set to a local folder of the core's mvp presets (dev copy, C-003) | The list shows the `.milk` files sorted. Changing the folder refreshes the list. Picking a preset applies it live, with `prepared` then `committed` in the log |
| TC-014 | Presets: off-thread prepare | Heavy preset switched repeatedly | No render-time spike or skipped frames in `GetStats` while switching. Render stays responsive |
| TC-015 | Presets: bad file | A malformed `.milk` in the folder, then selected | Warning with the core's error. The current preset keeps rendering. No crash |
| TC-016 | Presets: destroy during prepare | Script: change preset, then remove the source at once, ×20 | No crash. Quit shows `Number of memory leaks: 0` |
| TC-017 | Auto-cycle and blend | cycle 10 s, blend 2 s, then changed live via `SetInputSettings` | Commits about every `cycle + blend` s. New values apply without restarting OBS (the first interval may follow the old timer). cycle 0 stops changes |
| TC-018 | Random order | Random on, folder with 2+ presets | No immediate repeat across 20 changes. "Previous" walks back through history (C-014) |
| TC-019 | Hotkeys | Visualizer present | `GetHotkeyList` shows next, previous, random and pause. Triggering each (websocket or bound key, C-029) switches as expected. Pause stops auto-cycle until resumed, and manual switches still work while paused |
| TC-020 | Resolution | Width/height set to 1080×1920 | Scene item source size is 1080×1920 and the frame fills it. 10 scripted size changes produce no GL errors |
| TC-021 | Render scale | Scale 50 % vs 100 % | Lower render time at 50 %. The image still fills the item (upscaled) |
| TC-022 | GL-state check (other sources) | Visualizer plus image, colour and text sources; scene switches, show/hide, a second visualizer added and removed | Every source draws correctly in screenshots of the whole scene. No GL errors |
| TC-023 | Settings UI | Fresh source | `GetInputDefaultSettings` has a default for every setting. The properties show the Audio, Presets and Output groups. Every label comes from `en-US.ini` (no raw keys) |
| TC-024 | Teardown and quit | Source with audio attached, auto-cycle on | Removing the source and then quitting OBS: no crash, `plugin unloaded`, `Number of memory leaks: 0` |
| TC-025 | 1080p60 (after C-001) | Dedicated 1920×1080 at 60 fps profile; visualizer at 100 % with audio and auto-cycle | 5 min: `activeFps` about 60, `renderSkippedFrames` flat, `averageFrameRenderTime` well under 16.7 ms. The user's profile is restored afterwards |
| TC-026 | Packaging | Release build | The `.pkg` holds only the `.plugin` (plus dSYM) and no presets. The zip opens to the `.plugin`. `codesign --verify --deep --strict` passes. An installer run into the user's home domain loads in OBS |
| TC-027 | Install guide | `plugin/README.md` and the packaged build | A reader can install, add the source, set a preset folder, pick audio, bind hotkeys and uninstall by following the file alone |
| TC-028 | Multiple instances (known limit) | Two visualizers with different audio sources | Both run without GL errors. Both react to the combined audio (C-006, documented). Render time is reported |
