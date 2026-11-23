# Testing Patterns

**Analysis Date:** 2026-11-23

## Test Framework

**Runner:**
- None. The repo has no test suite, no test target and no test files. Do not add tests unless the
  user asks (root `CLAUDE.md`). If asked, the pure functions are the first candidates:
  `readSettings`, `resolvePresets`, `findMatch`, `chooseDevice` in `browser-source/obs-page.js`;
  in Phase 2, the audio ring buffer.

**Assertion Library:**
- Not used.

**Run Commands:**
- Not applicable. Changes are verified by running the real thing and keeping evidence (below).

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

**Build checks:**
- `cmake --preset macos-arm64 && cmake --build --preset macos-arm64` succeeds with no warnings in
  `plugin/`.
- `lipo -info` → arm64; `codesign -dvv` → ad-hoc; `codesign --verify` on the installed copy.
- `nm -gU <bundle>/Contents/MacOS/synesthesia-vibe-obs` lists exactly the 7 names in
  `plugin/exported-symbols.txt`.

**OBS runtime checks** (install as in `docs/integrations.md`, then start OBS):
- OBS log lines: `renderer check passed (OpenGL, device type 1)`, `plugin loaded successfully`,
  `sv_init_gl and sv_create succeeded (1920x1080)`, `preset loaded: …`, no GL error lines
  (`glUniformMatrix4fv failed …` means GL state leaked), `plugin unloaded` and
  `Number of memory leaks: 0` on quit.
- obs-websocket: `GetInputKindList` includes `synesthesia_vibe_visualizer`; `GetSourceScreenshot`
  of the visualizer and of the whole scene (other sources must still draw correctly);
  `GetStats` for render time.
- The renderer-refusal branch was exercised with a scratch build that forced the device type. A
  real Metal-renderer run is left to the user.
- GL state: the SPK-006 audit snapshotted about 250 GL items around every core call with a
  temporary instrumented build, not kept in the repo. Its table is in `PLAN.md` (*GL state*). The
  committed evidence file records the active unit as `GL_TEXTURE7 → GL_TEXTURE0`; the 7 came from
  the guard of that time, not OBS. The core always leaves `GL_TEXTURE0` active.

## Mocking

**Framework:** None.

**What to Mock:**
- Nothing in code. The only stand-ins are environmental: Chrome's fake audio device and fake UI,
  CDP permission overrides, and the plugin's synthetic tone (`push_synthetic_tone`) until Phase 2
  audio capture lands.

**What NOT to Mock:**
- OBS itself: plugin claims are only accepted from a real OBS run.

## Fixtures and Factories

**Test Data:**
- The core's 10 `mvp-presets` and 23 textures (fetched, not committed).
- A synthetic beat WAV (kick, sweep, hats) generated by a committed evidence helper
  (`develop/evidence/BS-005/bs-005-make-beat-wav.py` in the Phase 1 session).
- A probe preset that paints a white square in MilkDrop's top-left corner and a 50% red gradient,
  used to check orientation and sRGB (spike only, not kept).

**Location:**
- Session scratchpads and evidence folders; nothing under the repo's source tree.

## Coverage

**Requirements:** None enforced.

## Test Types

**Unit Tests:**
- Not used.

**Integration Tests:**
- Manual / scripted runs in Chrome and OBS, as above.

**E2E Tests:**
- Not used.

---

*Testing analysis: 2026-11-23*
