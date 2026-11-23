# Testing Patterns

**Analysis Date:** 2026-11-23

## Test Framework

**Runner:**
- None. The repo has no test suite, no test target and no test files. Do not add tests unless the
  user asks (root `CLAUDE.md`). If asked, the pure functions are the first candidates:
  `readSettings`, `resolvePresets`, `findMatch`, `chooseDevice` in `browser-source/obs-page.js`;
  in the plugin, `audio_ring` (`plugin/src/audio-ring.cpp`), then `scaled_dimension`,
  `sequential_preset_file` / `random_preset_file` and the cycle-latch bookkeeping in
  `plugin/src/visualizer-source.cpp`.

**Assertion Library:**
- Not used.

**Run Commands:**
- No test command. Changes are verified by running the real thing and keeping evidence (below).
  The automated part is CI: every PR and every push to `master` runs workflow **CI**, whose build
  and package checks fail the run on a regression (*CI checks* below).

## Test File Organization

**Location:**
- No tests. Verification evidence lives with the vibe-flow session that produced it:
  `.claude/working-sessions/<timestamp>-<slug>/develop/evidence/<TASK-ID>/`.

**Naming:**
- Evidence reports `<task-id>-<topic>-report.md`; screenshots and console captures named after the
  case (`…-t5s.png`, `…-console.txt`).

**Structure:**
- Only `.md` and `.py` files under `develop/evidence/` are committed. Screenshots, logs and helper
  `.mjs` / `.js` scripts there are git-ignored (`.gitignore`), so the committed reports must quote
  the lines that matter.

## CI checks (automated evidence)

GitHub Actions runs these on every PR, every push to `master` and every `v*` tag
(`.github/workflows/build-plugin.yaml`; layout in `docs/integrations.md`, *CI/CD & Deployment*).
They are inline shell steps, not a test suite: nothing is committed as a probe program, and no
OBS or Xvfb runs in CI (C-027). Each failure prints an `::error::` naming the problem and fails
the job. A green run is the build-and-package evidence for a change; behaviour inside OBS still
needs the runs below.

**macOS arm64 job, *Inspect package*:**
- The pkg's own Distribution (`pkgutil --expand`) has `hostArchitectures="arm64"`.
- `lipo -info` shows a non-fat arm64 binary.
- `nm -gU` lists exactly the 7 names in `plugin/exported-symbols.txt` (diff and count).
- `CFBundleVersion` equals `GITHUB_RUN_ID` (the CI build number).
- `codesign -dvv` shows `Signature=adhoc`, and `codesign --verify --deep --strict` passes.
- The pkg payload and the zip hold no `.milk`, presets or textures. The zip holds nothing outside
  the bundle and no `._*` entries.

**Linux x86_64 job, *Record libobs version*, *Check plugin binary*, *Inspect package*:**
- **OBS origin:**
  - the installed `obs-studio` comes from the PPA (`obsproject` in its version);
  - `obs-config.h` is found through `dpkg-query`;
  - the libobs API version goes to the log and the run summary.
- **Exports** (`nm -D --defined-only`):
  - the `obs_module_*` names equal the 7 in `plugin/exported-symbols.map`;
  - nothing else is defined except `_init`, `_fini`, `_edata`, `_end` and `__bss_start`;
  - nothing matches `glad`, `^sv_`, `projectm`, `nseel` or `^_Z`.
- **Link:** `readelf -d` has no `TEXTREL` (all objects are PIC), and `ldd` reports nothing `not
  found`.
- **`dlopen` probe:** inline `python3 -I` with ctypes and `RTLD_NOW | RTLD_LOCAL`. Every function
  import must bind at load, and `obs_module_load`, `obs_module_unload`, `obs_module_ver` and
  `obs_module_set_pointer` must resolve. It needs no display.
- **`.deb` control** (`dpkg-deb -I`): `Maintainer` is non-empty, and `Depends` names the OBS
  package.
- **`.deb` contents** (`dpkg-deb -c`):
  - `./usr/lib/x86_64-linux-gnu/obs-plugins/synesthesia-vibe-obs.so` and
    `./usr/share/obs/obs-plugins/synesthesia-vibe-obs/locale/en-US.ini` are present;
  - there are no presets or textures;
  - the `.so` extracted from the `.deb` still exports the 7 names.

**Both jobs:**
- The plugin sources build with warnings as errors (Xcode 26.6, GCC 13).
- *Set up environment* rejects a bad `config` input and a buildspec `name` or `version` of the
  wrong shape.

**Release workflow** (`.github/workflows/release.yaml`):
- *Validate tag* runs before anything builds.
- *Validate core version* checks that `CORE_VERSION` matches the submodule.
- `sha256sum -c` checks `SHA256SUMS.txt` before upload.
- The tag rules were checked locally against 16 tag/variable cases and once live with
  `v0.1.0-beta.0` (Phase 4 session, `develop/evidence/P4-006-release-logic.md`).

**Running a workflow step locally:**
- `develop/evidence/P4-002-workflow-step.py` in the Phase 4 session extracts one step's `run:`
  script from a workflow file, so it can be run with `bash -eo pipefail` against a local build.
- `actionlint` with shellcheck is the static check for workflow edits (0 errors at Phase 4).

**Not covered by CI:** anything inside OBS, the Linux plugin in a real OBS session (P4-008), the
pkg install, notarization.

## How the Browser Source is verified

**Headless Chrome over CDP:**
- Serve the page: `python3 -m http.server <port> --bind 127.0.0.1 --directory browser-source`.
- A small Node script starts Chrome with `--headless=new --remote-debugging-port=<port>
  --window-size=1920,1080 --use-gl=angle --use-angle=metal` and a throwaway profile, opens the URL
  over the DevTools protocol, captures console lines and timed screenshots, and can inject a
  click, key or `Runtime.evaluate`.
- Audio without a real device:
  `--use-fake-ui-for-media-stream --use-fake-device-for-media-stream
  --use-file-for-fake-audio-capture=<wav> --disable-features=AudioServiceSandbox
  --autoplay-policy=no-user-gesture-required`. On macOS the fake WAV is silent without
  `--disable-features=AudioServiceSandbox` (`.claude/common-issues/browser-source.md`).
- Permission-denied cases use CDP `Browser.setPermission` with `microphone: denied`.
- Audio reaction is measured, not eyeballed: a probe wraps the dist's `pushPcm` / `render` and
  samples `audioLevels()` to show PCM is flowing and the engine reacts.
- What a pass looks like: the expected `obs-page:` lines (settings, preset, preset request timing
  of `cycle + blend`, audio decision) and no uncaught errors.

**Inside OBS:**
- OBS is driven only through the local **obs-websocket** (no GUI or accessibility automation), in a
  separate `synesthesia-spike` scene collection: `CreateInput` for a `browser_source`,
  `SetSceneItemEnabled` for hide/show, `GetSourceScreenshot` for images.
- The page console inside OBS is read over CDP after starting OBS with
  `--remote-debugging-port=9222` (`http://localhost:9222/json/list`).
- The OBS log (`[obs-browser: '<source>'] Error: …`, `Browser Hardware Acceleration: true`, the
  macOS microphone permission line) confirms the environment.

**User checklist:**
- Loopback audio (BlackHole on macOS, VB-Cable on Windows), Windows and the real microphone path
  need a person. Phase 1 is done only after the user runs that checklist.

## How the plugin is verified

Plugin claims are accepted only from a real OBS run (OBS 33.0.3, OpenGL renderer, Apple M1 so
far), driven by scripts over obs-websocket, with the OBS log and screenshots as evidence. The
Phase 2 session's per-task write-ups are under
`.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/<task>/`.

**Build checks** (every change; CI repeats the binary checks on both OSes):
- `cmake --preset macos-arm64 && cmake --build --preset macos-arm64` succeeds in `plugin/` with
  no warning from `plugin/src` (the plugin builds with `-Werror`; the ~105 core warnings are
  expected). A change to Linux-only code is first compiled by the CI Linux job; there is no local
  Linux build here.
- `lipo -info` → arm64; `codesign -dvv` → ad-hoc; `codesign --verify` on the installed copy.
- `nm -gU <bundle>/Contents/MacOS/synesthesia-vibe-obs` lists exactly the 7 names in
  `plugin/exported-symbols.txt`.
- **`sv_*` reachability:** for every changed path, trace which thread can reach each `sv_*` call
  (grep plus the call sites, or `codegraph_explore`). Only `video_render`, the worker
  (`sv_prepare_preset`) and create/destroy under `obs_enter_graphics` may reach one.
- Packaging changes: the inspection commands in `docs/integrations.md`, *Packaging*.

**OBS rules for every scripted run:**
- obs-websocket only; no GUI or accessibility automation, no edits to OBS config files.
  Credentials stay outside the repo (scripts read a local creds file path from `OBS_WS_CREDS`).
- Work in the `synesthesia-spike` scene collection and restore the user's collection at the end.
  Restore any visualizer settings the run changed.
- **Never remove an input, and never switch scene collection, while that input's properties
  dialog may be open.** A dirty dialog keeps the source alive and wedges OBS until a person
  answers its prompts (`.claude/common-issues/verification-tooling.md`). Ask the user to close a
  dialog opened with `OpenInputPropertiesDialog` before any removal.
- **Test removals in the program scene.** `RemoveInput` on a source in a scene that is not
  rendering returns OK but leaves the source alive: libobs prunes removed items only in
  `scene_video_render`. A removal test there proves nothing.
- Quit OBS with `osascript -e 'tell application "OBS" to quit'`, ignore its -128 status and wait
  for the process to exit before copying a new build. Every quit must log `plugin unloaded` and
  `Number of memory leaks: 0`. A changed 1080p60 profile is created only with the user's approval
  and never while streaming or recording.

**OBS runtime checks** (dev install as in `docs/integrations.md`, then start OBS):
- OBS log lines: `renderer check passed (OpenGL, device type 1)`, `plugin loaded successfully`,
  `sv_init_gl and sv_create succeeded (…)`, `preset prepared` / `preset committed`, no GL error
  lines (`glUniformMatrix4fv failed …` means GL state leaked), `plugin unloaded` and
  `Number of memory leaks: 0` on quit.
- obs-websocket requests used: `GetInputKindList` (`synesthesia_vibe_visualizer` present),
  `CreateInput` / `SetInputSettings` / `GetInputSettings` / `GetInputDefaultSettings`,
  `SetSceneItemEnabled` (hide/show), `GetSourceScreenshot` of the visualizer and of the whole
  scene (image, colour and text sources must still draw), `GetStats` (`averageFrameRenderTime`,
  `renderSkippedFrames`, `activeFps`), `TriggerHotkeyByName` with `contextName` (reaches source
  hotkeys), `GetHotkeyList`, `SetInputAudioTracks`, `OpenInputPropertiesDialog`.
- Properties dialog captures: open it with `OpenInputPropertiesDialog` and capture the window with
  `screencapture -l <window id>` (the OS screenshot tool, not GUI automation).
- **Pixel measurements instead of eyeballing:** a small committed Python helper reads the PNG and
  reports min alpha, magenta pixels under the visualizer, or probe colours at fixed rows
  (`develop/evidence/P2-003/P2-003-alpha-probe.py`); another compares consecutive frames for
  motion (`develop/evidence/P2-004/P2-004-motion.py`).
- **Timing from the log:** auto-cycle intervals are measured from the `preset committed`
  timestamps (expect `cycle + blend` plus about one frame plus the prepare); commit and prepare
  costs are in the log lines themselves.
- `GetStats` on the OpenGL backend measures CPU-side submission. It does not show GPU fill savings
  (render scale) until the GPU is saturated; test that at a very large size (8192×8192).

**Scratch-only instrumentation** (never committed into `plugin/src`):
- Some claims need code that must not ship. A committed Python patcher applies it to a **copy** of
  `visualizer-source.cpp`, the copy is built and installed for the run, and the clean build is
  reinstalled afterwards. The reviewer confirms the instrumentation is absent from the source.
  - **Re-entrancy detector** (`develop/evidence/P2-002/P2-002-add-detector.py`): counts
    overlapping core calls across threads. The race repro is a 5-minute screenshot loop
    (`GetSourceScreenshot` at about 19/s) with item toggles and hide/show while the source is on
    air. Pass: 0 overlaps. TSan is not available against release OBS.
  - **Audio levels log** (`develop/evidence/P2-011/P2-011-add-audio-debug.py` in the Phase 2
    session): logs the core's bass / mid / treble levels, so "reacts" and "silent" are numbers,
    not screenshots. It anchors on `remember_committed_preset`. The older
    `P2-004-add-audio-debug.py` no longer applies: its `load_pending_preset` anchor is gone.
- A forced-device-type scratch build exercised the renderer-refusal branch. A real Metal-renderer
  run is left to the user.
- GL state: the SPK-006 audit snapshotted about 250 GL items around every core call with a
  temporary instrumented build. Its table is in `PLAN.md` (*GL state*). The committed evidence file
  records the active unit as `GL_TEXTURE7 → GL_TEXTURE0`; the 7 came from the guard of that time,
  not OBS. The core always leaves `GL_TEXTURE0` active. `sv_commit_preset`, `sv_resize` and
  `sv_set_transition_config` were not part of that audit; the integrated GL-state check on the
  final build passed in the P2-011 run (below).

**Done-when run, 2026-11-23** (P2-011, P2-012, P2-010b; production build of `4c260b5`, OBS
33.0.3; reports under `develop/evidence/P2-011/`, `P2-012/` and `P2-010b/` in the Phase 2
session). All passed, with 0 GL error lines and `Number of memory leaks: 0` on every quit:
- **P2-011 Done-when run:** the load lines (TC-M03); GL state correct next to image, colour and
  text sources across a scene switch, hide/show and a second instance added and removed; live
  preset and transition changes; a 300 s race stress (5882 of 5882 screenshots,
  `renderSkippedFrames` flat at 2); 6 removals with audio attached and a prepare queued; quit
  150 ms after a preset request. Both audio paths reacted and went silent when they should
  (scratch levels build, bass max 4.95 direct and 4.11 / 8.43 on the mix).
- **1080p60** (P2-012): a dedicated `synesthesia-1080p60` profile (1920×1080, 60 fps) created over
  the websocket, the visualizer at 1920×1080 and 100 %, the audio fixture attached, auto-cycle at
  10 s, 30 `GetStats` samples over 5 minutes. `activeFps` 60 in every sample,
  `averageFrameRenderTime` 3.75–8.60 ms (median 4.69 ms), 1 skipped frame in 17 446. 24 commits
  in the window, the slowest 20.7 ms. 75 % and 50 % render scale were not needed. The user's
  profile was restored and checked with `GetProfileList`.
- **P2-010b dialog check:** a visualizer with only `preset_folder` set; `OpenInputPropertiesDialog`;
  1.5 s later `GetInputSettings` still showed only `preset_folder`, with "(first preset)" selected
  in a `screencapture -l` shot. The user closed the dialog before the source was removed.
- **Projectors, multiview and filters** (a follow-up P2-011 session): colour correction, crop and
  sharpen filters on the visualizer and a colour filter on the image drew correctly. A source
  projector, a multiview and a program projector, all windowed, showed the visualizer animating.
  60 s of scene switches and hide/show with all three open: 0 skipped frames in 1969, average render
  time about 10.6 ms. Opening projectors needs the user to close them; OBS has no websocket request
  for that, and windows can end up behind other apps.

**Still open** (see `docs/concerns.md`, *Open checks*): the pkg install into OBS (TC-M04, P2-013),
notarization (needs Developer ID credentials), Linux in OBS (P4-008, below) and the Phase 1
loopback checklist.

## Mocking

**Framework:** None.

**What to Mock:**
- Nothing in code. The only stand-ins are environmental: Chrome's fake audio device and fake UI,
  CDP permission overrides, and an OBS Media Source playing a generated WAV instead of a real
  microphone or desktop capture.

**What NOT to Mock:**
- OBS itself: plugin claims are only accepted from a real OBS run.

## Fixtures and Factories

**Test Data:**
- The core's 10 `mvp-presets` and 23 textures (fetched, not committed). For the plugin they go
  into a scratch preset folder with the textures beside the `.milk` files
  (`docs/integrations.md`, *Dev-only preset folder*).
- Extra scratch folders for the preset tests: a second small folder, an empty folder, a folder
  with a deliberately broken `.milk`, and "heavy" presets with about 400 ms prepares
  (`develop/evidence/P2-006/P2-006-make-heavy.py`).
- **Plugin audio fixture:** a 48 kHz WAV of 6 s beat then 6 s silence, looping, generated by
  `develop/evidence/P2-004/P2-004-make-fixture.py` and played by an OBS **Media Source** (C-019).
  Its beat/silence halves give "reacts" and "silent" in one run; muting it, moving it to another
  mix track (`SetInputAudioTracks` on the test input only) or removing it covers the other cases.
  A Media Source delivers about 12 chunks a second; mic and desktop sources deliver about 10 ms
  chunks and are on the user checklist.
- A synthetic beat WAV (kick, sweep, hats) for the Browser Source
  (`develop/evidence/BS-005/bs-005-make-beat-wav.py` in the Phase 1 session).
- A probe preset that paints known colours at known rows, a white top-left square and a blue
  right half, used to check orientation and sRGB pixel-exactly (`P2-003-probe-orientation.milk`,
  local evidence, git-ignored).

**Location:**
- Session scratchpads and evidence folders; nothing under the repo's source tree.

## Coverage

**Requirements:** None enforced.

## Test Types

**Unit Tests:**
- Not used.

**Integration Tests:**
- CI build-and-package checks (*CI checks*) on every PR.
- Manual / scripted runs in Chrome and OBS, as above. The helper `.mjs` scripts drive one phase
  per invocation (`setup`, the cases, `cleanup`) and are git-ignored; the committed `.md` reports
  quote the log lines and numbers that matter.

**User checklist (plugin):**
- Real microphone and desktop-audio sources, Gatekeeper dialogs for the unnotarized pkg and zip,
  the real Metal renderer, and OBS UI paths not exercised by the websocket (Renderer setting,
  Hotkeys settings page). These need a person.
- **Linux in OBS (P4-008):** the CI `.deb` on Ubuntu 24.04 with OBS from the PPA, once in an X11
  session and once in a Wayland session. Check the `windowing platform:` log line, that the
  source renders and reacts to audio, and that other sources still draw. The Linux package goes
  onto releases only after this passes.

**E2E Tests:**
- Not used.

---

*Testing analysis: 2026-11-23*
