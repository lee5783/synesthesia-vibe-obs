# SPK-004

## Subtasks executed
- Confirmed in the libobs 31.1.1 sources (`plugin/.deps/`) that graphics exist at module load time:
  `OBSBasic::OBSInit` calls `ResetVideo()` (line 920) before `obs_load_all_modules2` (line 970).
  `obs_enter_graphics()` is a no-op without graphics, and `gs_get_device_type()` returns -1 there
  (`gs_valid` logs at debug level only). So the check stays in `obs_module_load` and doesn't need
  to move to the first `create`.
- Added `renderer_is_opengl()` to `plugin/src/plugin-main.cpp`, in an anonymous namespace as in
  `visualizer-source.cpp`:
  - It reads `gs_get_device_type()` and `gs_get_device_name()` between
    `obs_enter_graphics()` and `obs_leave_graphics()`.
  - Anything other than `GS_DEVICE_OPENGL`, including -1, gets a `LOG_WARNING` naming the renderer
    and device type, and returns false.
  - On OpenGL it logs a one-line `LOG_INFO` pass.
- `obs_module_load` returns `true` without registering the source when the check fails, so OBS
  still loads the module cleanly.
- Built the plugin, installed it, and verified the OpenGL branch live through obs-websocket (no
  auth, websocket only, no GUI automation).
- Ran the refusal branch from a scratch copy of `plugin/` in the session scratchpad, patched with an
  env-var override. Then reinstalled the repo build and confirmed the OpenGL branch again.
- Recorded the Q5 answer in `PLAN.md` under the renderer-check spike bullet. Added the
  `obs-plugintemplate` pin `3e7d7ac3b5342cd7d9b88890b9c70b472d1520fc` to the plugin-layout
  paragraph (S-006).

## Files changed
- `plugin/src/plugin-main.cpp`: renderer check before registration.
- `PLAN.md`: template SHA plus vendored-files note under *Repository layout*, and the SPK-004
  answer under the *Renderer check* spike bullet.
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/input-dev.md`: SPK-004
  section appended.
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/output-SPK-004.md`
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/SPK-004/`:
  `SPK-004-obs-log-opengl.txt`, `SPK-004-obs-log-forced-non-opengl.txt`, `SPK-004-build.log`.
- Outside the repo:
  - The installed `synesthesia-vibe-obs.plugin` in the user plugin folder was replaced with this
    build. It is cmp-identical to `plugin/build_macos_arm64`.
  - The scratch copy and its build dir are in the session scratchpad. Nothing is committed.
- Not touched: `external/synesthesia-vibe/` (clean), OBS config files, the OBS renderer setting,
  `docs/`.

## What changed and why
PLAN.md Q5 needs the plugin to refuse cleanly when OBS isn't on OpenGL, because the core is
GL-only. The check needs a graphics context, and the frontend has one by module load time. So it
runs once at load, and a non-GL renderer leaves the plugin loaded but registers no source type.
- The user sees the source missing from Add Source, plus one warning line in the OBS log. There is
  no dialog.
- Apple's GL-on-Metal driver ("OpenGL 4.1 Metal - 90.5") is OBS's OpenGL device and passes the
  check.

## Verification
- `cd plugin && cmake --preset macos-arm64`: exit 0.
- `cmake --build --preset macos-arm64`: `** BUILD SUCCEEDED **` (8.5 s incremental), 0 warnings in
  `plugin/src`.
- `nm -u` shows the new imports: `_gs_get_device_type`, `_gs_get_device_name`,
  `_obs_enter_graphics`, `_obs_leave_graphics`.
- `codesign --verify` of the installed bundle: valid.
- OBS reload cycle, three times:
  1. Quit with `osascript ... quit`. AppleScript printed `User canceled (-128)` each time, but OBS
     exited cleanly: the log shows `plugin unloaded` and `Number of memory leaks: 0`.
  2. Reinstall the bundle with `ditto`.
  3. Relaunch with `open -a OBS`. The websocket was up within 2 s.
- OpenGL run (repo build):
  - Log: `[synesthesia-vibe-obs] renderer check passed (OpenGL, device type 1)`, then
    `plugin loaded successfully (version 0.1.0)`.
  - `GetInputKindList`: 17 kinds, including `synesthesia_vibe_visualizer`.
  - `SetCurrentSceneCollection synesthesia-spike`, then `GetInputList`: the existing
    `SynesthesiaVibe Visualizer` input of kind `synesthesia_vibe_visualizer` is there.
  - Restored `Untitled`.
- Forced refusal run (scratch build, launched with
  `open -a OBS --env SV_SPIKE_FORCE_DEVICE_TYPE=3 --env SV_SPIKE_FORCE_DEVICE_NAME=Metal`):
  - Log: `[synesthesia-vibe-obs] SynesthesiaVibe Visualizer needs the OpenGL renderer; current
    renderer is Metal (device type 3). The source is not registered.` The module still appears
    under Loaded Modules.
  - `GetInputKindList`: 16 kinds, without `synesthesia_vibe_visualizer`.
  - The collection stayed on `Untitled` throughout, so no collection holding the source was loaded
    or saved while the type was missing.
- Final state: repo build installed, OBS running on OpenGL with the pass line logged, 17 kinds,
  current collection `Untitled`.
- No existing test suite covers this area.

## Evidence
- `evidence/SPK-004/SPK-004-obs-log-opengl.txt`: OpenGL-branch log excerpts from two launches,
  plus the websocket results (`GetInputKindList` includes the kind, and the spike collection's
  input). Paths and the GPU adapter name are scrubbed.
- `evidence/SPK-004/SPK-004-obs-log-forced-non-opengl.txt`: the scratch override diff, the refusal
  warning in the OBS log, and `GetInputKindList` without the kind.
- `evidence/SPK-004/SPK-004-build.log`: build result, architecture, the new libobs imports, and the
  signature check.
- No screenshot. The change only affects the log and source registration, and both are shown
  through the OBS log and obs-websocket. GUI automation is not allowed.

## Known limitations / follow-ups
- **The real Metal renderer was not exercised (S-008).** The refusal branch ran with forced values
  inside a real OBS. The value OBS's Metal device actually returns from `gs_get_device_type()` is
  unknown: the 31.1.1 headers define only 1 (OpenGL) and 2 (D3D11). Any value other than 1 is
  refused, so this doesn't matter for correctness.
  - Optional user check: Settings → Advanced → Renderer → Metal, restart OBS, look for the
    `needs the OpenGL renderer` warning and confirm the source is missing from Add Source. Then
    switch back to OpenGL and restart.
- Not tested: loading a scene collection that already holds the source while the type is
  unregistered.
- On refusal there is no in-app notice, only the log line. A frontend dialog would need
  `ENABLE_FRONTEND_API`. Out of scope for the spike; Phase 2 can decide.
- The `core linked (...)` line is still SPK-003 spike instrumentation, to be dropped in SPK-005.
  The new `renderer check passed` line is meant to stay.
- The scratch build and its override exist only in the session scratchpad and are not referenced
  from any repo file.
- Doc gaps for document-writer:
  - `docs/architecture.md`: module load order, and the renderer check gating registration.
  - `docs/concerns.md`: no in-app notice on refusal, and the Metal device-type value is unverified.

## Handoff for downstream review
- Acceptance criteria addressed:
  - `gs_get_device_type()` is read at load inside the graphics context.
  - A non-OpenGL renderer gets a clear warning and nothing is registered, while the module still
    loads.
  - The graphics context is confirmed available at load time.
  - The `PLAN.md` answer covers where the check runs and what the user sees, and notes GL-on-Metal.
  - The S-006 template SHA is in `PLAN.md`.
- Areas to scrutinize:
  - `renderer_is_opengl()` in `plugin/src/plugin-main.cpp`: -1 handling, and the null
    `device_name` fallback.
  - Whether returning `true` on refusal is the wanted module semantics. `false` would make OBS log
    "Failed to initialize module".
  - The `PLAN.md` wording.
- How to run:
  1. From `plugin/`: `cmake --preset macos-arm64` and `cmake --build --preset macos-arm64`.
  2. Quit OBS.
  3. `ditto` the bundle from `build_macos_arm64/RelWithDebInfo/` into the user plugin folder.
  4. Relaunch OBS and grep the newest log for `renderer check`.
