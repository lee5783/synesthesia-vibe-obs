# Codebase Concerns

**Analysis Date:** 2026-11-23

`plugin/` is the Phase 2 spike carried forward as the start of the real plugin, so several items
below are known stand-ins with a planned fix in `PLAN.md` (*Tasks after the spike*). Gotchas
with a quick workaround are in `.claude/COMMON-ISSUES.md`.

## Tech Debt

**Plugin build is macOS / arm64 / Xcode only, and nothing enforces it:**
- Issue: the triplet (`arm64-osx`), `CMAKE_OSX_ARCHITECTURES=arm64`, the Apple framework list and
  `XCODE_ATTRIBUTE_EXPORTED_SYMBOLS_FILE` are unconditional. There are no `OS_MACOS` guards.
- Files: `plugin/CMakeLists.txt`, `plugin/CMakePresets.json`.
- Impact: only the `macos-arm64` preset works. The template's `macos` / `macos-ci` presets are
  universal and fail at link (the core is arm64 only); `windows-*` / `ubuntu-*` fail at the triplet
  and frameworks. A non-Xcode generator silently drops the export list, which brings back
  cross-image weak-symbol coalescing.
- Fix approach: a `FATAL_ERROR` unless macOS + arm64 + Xcode, or `if(OS_MACOS)` guards with a clear
  message (Phase 2 task 1). Universal needs a second `x64-osx` core build plus `lipo` (task 7).
  Linux needs `-Wl,--version-script` with the same 7 names, or `-Wl,--exclude-libs,ALL` (Phase 4).

**Core ExternalProject runs on every build:**
- Issue: `BUILD_ALWAYS TRUE` makes the core's build and install steps run on every plugin build
  (cheap Ninja no-op, but visible in Xcode output). It exists so a submodule bump rebuilds
  `libSynesthesiaVibeCore.a`.
- Files: `plugin/CMakeLists.txt`.
- Impact: whether the Xcode generator then **relinks** the plugin against a changed IMPORTED static
  library is unverified (the core never changed during the spike).
- Fix approach: after the first real core bump, check that the plugin binary's timestamp moves. If
  not, add a dependency on the `.a`.

**Build-number seed overrides CI numbers:**
- Issue: `plugin/CMakeLists.txt` writes `0` into `cmake/.CMakeBuildNumber` when it is missing, to
  avoid the template's empty-number first-configure failure. That bypasses the template's
  `GITHUB_RUN_ID` branch on CI.
- Files: `plugin/CMakeLists.txt`, `plugin/cmake/common/buildnumber.cmake` (vendored).
- Fix approach: revisit in Phase 4 when CI exists (e.g. skip the seed when `CI` is set).

**Spike stand-ins in the plugin:**
- Issue: a synthetic tone instead of audio capture; a fixed 1920×1080 size; one `preset_path`
  setting with no properties UI; a synchronous `sv_load_preset` inside `video_render`.
- Files: `plugin/src/visualizer-source.cpp`.
- Impact: the source does not react to OBS audio; slow preset compiles would stall OBS's renderer.
  A new path set while the source goes on air loads twice (default preset first, then the new
  one).
- Fix approach: Phase 2 tasks 3–6 (audio ring, `sv_prepare_preset` + commit in `video_render`,
  `sv_resize`, properties). Skip reloading a path that is already loaded.

**Presets are copied into the plugin bundle by hand:**
- Issue: no install step ships presets or textures; a fresh build plus `ditto` shows nothing until
  they are copied (`docs/integrations.md`).
- Fix approach: Phase 2 packaging decides how presets ship, given their local-only terms
  (`docs/decisions.md`).

**Only `en-US.ini`:**
- Files: `plugin/data/locale/en-US.ini`. OBS logs a benign `Failed to load '<locale>' text` line on
  other UI locales and falls back to en-US. Add locales in Phase 2 if wanted.

**`fetch-dist.sh` rough edges:**
- Files: `browser-source/fetch-dist.sh`.
- `curl` has no `--connect-timeout` or `--retry`; a stalled connection hangs the script.
- The 404 message says "check that release v… exists" even when the missing file is the source
  tarball.
- The ~4 MB tarball is downloaded on every run, even when only the dist changed.
- A `SIGKILL` mid-install can leave `browser-source/.dist-stage.*` / `.dist-old.*`, which are not
  git-ignored.

## Known Bugs

**`sv_context` race between `video_tick` and an off-thread `video_render`:**
- Symptoms: none observed, but one context can be touched from two threads at once, breaking the
  core's single-threaded rule.
- Files: `plugin/src/visualizer-source.cpp` (`visualizer_video_tick`, `render_core_frame`).
- Trigger: `video_tick` runs on the graphics thread without the graphics lock and calls
  `sv_push_pcm_float` and writes `pending_seconds` / `frame_pending`. `video_render` can run at the
  same time on a thread holding only the graphics lock (obs-websocket `GetSourceScreenshot` does
  this) and calls `sv_render_frame`.
- Workaround: none in code. Phase 2 task 3 moves every `sv_*` call and the `dt` consumption into
  `video_render`; the tick side writes only atomics, exchanged atomically so repeated renders in
  one frame do not both consume the delta. Verify with TSan or a debugger on a repro that requests
  screenshots of the source in a loop while it renders on air.

**Presets that write alpha 0 show as transparent:**
- Symptoms: some presets (swarm) appear transparent in screenshots, filters and nested scenes; a
  plain RGB stream output still shows them.
- Files: `plugin/src/visualizer-source.cpp` (`visualizer_video_render`, blending off,
  `OBS_EFFECT_DEFAULT`).
- Fix approach: draw with `OBS_EFFECT_OPAQUE` or force alpha to 1 (Phase 2).

**Whole off-air gap passed as one step:**
- Symptoms: when the source returns after being hidden or inactive, the accumulated delta (5 s in a
  test) goes to the core in one `sv_render_frame`; a screenshot of the off-air source came back
  black.
- Fix approach: clamp or reset the delta on `show` / `activate`, together with the race fix above.

## Security Considerations

**Supply chain of the web dist and presets:**
- Risk: the zip's checksum comes from the same release, so it catches corruption, not a
  compromised release. The source tarball has no checksum at all (accepted, C-010); GitHub archive
  bytes are not guaranteed stable. `unzip` would create symlink entries as-is.
- Files: `browser-source/fetch-dist.sh`.
- Current mitigation: HTTPS, checksum before staging, explicit member extraction under a validated
  root, symlinks and dotfiles filtered when copying resources.
- Recommendations: an upstream release asset with presets and a checksum. A one-line stderr notice
  when `FETCH_DIST_BASE_URL` / `FETCH_DIST_ARCHIVE_URL` is set, since an override turns
  verification into self-consistency.

**Local serving and debugging exposure:**
- Risk: `python3 -m http.server` without `--bind 127.0.0.1` serves the presets to the network.
  `--remote-debugging-port` lets any local program control OBS's browser.
- Current mitigation: both are documented in `browser-source/README.md`.

**Public repo hygiene:**
- Never commit `.vscode/` (editor settings can hold absolute machine paths; it is git-ignored),
  credentials for the local obs-websocket, or absolute paths in session artifacts.

## Performance Bottlenecks

**Plugin render cost:**
- Measured: about 4–7 ms of OBS render time per frame at a 30 fps canvas; `gl_state_guard` adds
  about 20 µs per wrapped call. **1080p60 is unmeasured.**
- Improvement path: measure with a dedicated 1920×1080 / 60 fps profile before Phase 2 is done; the
  render-scale setting (task 6) if needed.

**Synthetic tone while off-air:**
- The stand-in pushes about 44k frames/s per instance even when the source is not shown. Goes away
  with Phase 2 audio.

**Several Browser Sources:**
- Each one has its own WebGL 2 renderer and audio capture. Reuse one source across scenes.

## Fragile Areas

**`gl_state_guard` must match what the core changes:**
- Files: `plugin/src/visualizer-source.cpp`, audit table in `PLAN.md` (*GL state*).
- Why fragile: OBS caches GL state and skips binds; any state the core changes that the guard
  misses corrupts the next OBS draw (`glUniformMatrix4fv … GL_INVALID_OPERATION`, black or
  misplaced quads, lost blending). Scissor and colour mask are **not** guarded: the core disables
  scissor and sets the colour mask to all-true, which is "no net change" only because OBS had them
  that way in the scenes tested. Projector, multiview and filter paths were not exercised. The unit
  range follows `GS_MAX_TEXTURES` (8).
- Safe modification: wrap every new GL-touching `sv_*` call in `with_core_gl_state`; re-run the
  audit on every core bump; guarding scissor and colour mask costs two `glGet`s.
- Evidence caveat: the committed SPK-006 state-diff evidence records the active unit as
  `GL_TEXTURE7 → GL_TEXTURE0`. The 7 came from the guard of that time, not OBS. The core always
  leaves `GL_TEXTURE0` active; the starting unit is whatever the host had active.

**`plugin/exported-symbols.txt`:**
- Why fragile: a new optional `obs_module_*` entry point that is not listed is hidden with no
  warning and OBS never calls it; listing one that is not defined fails the link.
- Safe modification: update the list in the same change that adds the entry point; check with
  `nm -gU`.

**Renderer check:**
- Files: `plugin/src/plugin-main.cpp`.
- Gaps: on refusal there is only a log line (a dialog needs `ENABLE_FRONTEND_API`); the value
  OBS's real Metal renderer returns from `gs_get_device_type()` was never observed (any value but
  OpenGL is refused, so correctness holds); loading a scene collection that already holds the
  source under refusal is untested.

**Preset name matching:**
- Files: `browser-source/obs-page.js` (`findMatch`).
- A `preset` or `audio` item made only of `+` or spaces becomes the empty key, which
  substring-matches every candidate and picks the first, with a warning. Harmless; treating an
  empty key as "no match" is a one-line fix.

## Dependencies at Risk

**libobs 31.1.1 vs the OBS that users run:**
- The plugin builds against libobs 31.1.1 (template buildspec) and is checked in OBS 33.0.3. D6
  (minimum OBS 31) is still proposed. Bump `plugin/buildspec.json` only together with D6.

**Core build on new SDKs:**
- About 105 deprecation warnings on the macOS 26 SDK; never build the core with `-Werror`. The
  core's CMake is not subdirectory-safe (upstream ask #1 in `PLAN.md`).

## Missing Critical Features

**Phase 1 sign-off:**
- The loopback checklist (BlackHole on macOS, VB-Cable on Windows) and the real microphone path
  inside OBS are pending with the user. Phase 1 is not done until they report back.

**Hosting:**
- GitHub Pages (D5) is open and skipped: presets are approved for local serving only.

**Native audio, settings, hotkeys, packaging, Windows:**
- Phase 2 tasks 3–7 and Phase 3.

## Test Coverage Gaps

**`fetch-dist.sh` off macOS:**
- Not run with GNU tar or GNU `sha256sum` (Linux), or under Git Bash / WSL (Windows). Priority:
  Medium.

**Browser Source in OBS:**
- Audio after hide/show; the flag-specific error name when `--enable-media-stream` is missing (the
  README's `NotAllowedError` wording is unconfirmed); "Shutdown source when not visible" reopening
  the audio device (reasoned, not observed); Windows and Linux entirely. Priority: Medium (user
  checklist).

**Plugin:**
- 1080p60, the real Metal renderer, scene collections under refusal, the S1 race repro, relink
  after a core bump. Priority: High for the race before Phase 2 audio lands.

---

*Concerns audit: 2026-11-23*
