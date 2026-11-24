# Codebase Concerns

**Analysis Date:** 2026-11-23

`plugin/` holds the Phase 2 macOS plugin: tasks 1–6 are built and reviewed, packaging is done in CI:
tag builds sign with Developer ID and notarize (first run `v0.1.0-beta.1`, 2026-11-24). Its OBS checks (P2-011, P2-012, P2-010b) passed on
2026-11-23; the pkg install into OBS is still open (*Open checks*). Phase 4 added CI, a
draft-prerelease release pipeline and an experimental Linux x86_64 build that has never run in
OBS. Gotchas with a quick workaround are in
`.claude/COMMON-ISSUES.md`. Phases, risks and the filed-or-not state of upstream asks #1–#3 and
#5 are in `PLAN.md`; #4, #6 and #7 are drafted below.

## Fixed in Phase 2

Listed so nobody chases them again. Each was verified per task in OBS 33.0.3 and again together
on the final build in the P2-011 run (2026-11-23).

- **`sv_context` race between `video_tick` and an off-thread `video_render`.** The tick writes only
  atomics; every `sv_*` call runs in `video_render` or under `obs_enter_graphics`. A scratch
  re-entrancy detector counted 3 overlapping core calls in 60 s on the spike and 0 in 22,450 calls
  over a 5-minute screenshot stress on the fixed build (`docs/testing.md`).
- **Off-air frame came back black.** `show` / `activate` reset the pending delta, and one step is
  capped at 0.1 s.
- **Presets that write alpha 0 showed as transparent.** The sprite is drawn with
  `OBS_EFFECT_OPAQUE` (alpha forced to 1.0).
- **macOS / arm64 / Xcode-only build was unenforced.** `plugin/CMakeLists.txt` refuses any other
  host, generator or architecture at configure, before the template runs.
- **Spike stand-ins** (synthetic tone, fixed 1920×1080, one `preset_path` setting, synchronous
  `sv_load_preset` in `video_render`): replaced by audio capture, size and render-scale settings,
  the preset folder, and prepare-on-worker / commit-in-render.

## Fixed in Phase 4

- **Build-number seed overrode CI numbers.** The seed in `plugin/CMakeLists.txt` now runs only
  when `CI` is not set, so CI builds are numbered with `GITHUB_RUN_ID`. The macOS CI job asserts
  `CFBundleVersion` equals the run id. The local first-configure fix still applies.
- **No CI, no automated packaging check.** `.github/workflows/` builds, packages and inspects both
  OSes on every PR (`docs/testing.md`, *CI checks*).
- **Linux export control** (the `PLAN.md` *Duplicate glad symbols on Linux* risk): a version
  script plus `--exclude-libs,ALL`; CI checks that only the 7 `obs_module_*` names are exported.

## Tech Debt

**Plugin builds only on macOS arm64 (Xcode) and Linux x86_64 (Ninja):**
- Issue: the configure guard allows exactly those two combinations and refuses everything else,
  including the template's `macos` (universal) and `windows-*` presets. The Apple settings sit
  under `if(OS_MACOS)`, and `else()` means Linux only because the guard forbids everything else.
- Files: `plugin/CMakeLists.txt`, `plugin/CMakePresets.json`, `plugin/packaging/macos/distribution.in`.
- Fix approach:
  - **Universal macOS binary (C-025, a follow-up outside Phase 4):** a second `x64-osx` core build
    plus `lipo`, the guard relaxed, and `hostArchitectures` in
    `plugin/packaging/macos/distribution.in` reverted to `arm64,x86_64`. The CI inspection's
    arm64-only checks change with it.
  - **Windows (Phase 3):** revisit the guard and every `if(OS_MACOS) … else()` block.

**Small Linux and build follow-ups (Gate 2 review m4–m7):**
- **m4, duplicated archive paths:** `libprojectM_ns-eel2.a` and `libprojectM_eval.a` are spelled
  twice in `plugin/CMakeLists.txt`, once in `SYNESTHESIA_VIBE_CORE_DEPENDENCY_LIBRARIES`
  (`BUILD_BYPRODUCTS`) and once in the imported targets' `IMPORTED_LOCATION`. One variable per
  archive would keep them in step.
- **m5, platform log in the GL header:** `opengl_log_platform` (the windowing-platform log) is
  declared in `plugin/src/opengl-loader.hpp`, so `plugin-main.cpp` includes the platform GL
  headers only to call it. It is not a GL-loading concern. Revisit only if the header grows.
- **m6, unreachable `libGL.so.1` fallback** (`plugin/src/opengl-loader-linux.cpp`): the `.so` has
  `NEEDED libOpenGL.so.0`, so the first `dlopen` always succeeds once the plugin is loaded.
  Harmless; note it if the C-011 loader choice is revisited after P4-008.
- **m7, the Linux guard checks only the host:** it reads the host processor, not the target, so
  an x86_64 host cross-compiling with a toolchain file passes. Edge case, out of scope.

**`visualizer-source.cpp` holds three subsystems:**
- Issue: about 1,290 lines with presets / auto-cycle, audio attachment and GL / render in one file.
- Files: `plugin/src/visualizer-source.cpp`.
- Fix approach: split the preset scheduler and the audio attachment into their own files, as
  `audio-ring.*` already is, at the start of Phase 3 (D3D11 fallback work touches the render
  side anyway).

**Core ExternalProject runs on every build:**
- Issue: `BUILD_ALWAYS TRUE` makes the core's build and install steps run on every plugin build
  (cheap Ninja no-op, but visible in Xcode output). It exists so a submodule bump rebuilds
  `libSynesthesiaVibeCore.a`.
- Files: `plugin/CMakeLists.txt`.
- Impact: whether the Xcode generator then **relinks** the plugin against a changed IMPORTED static
  library is unverified (the core has not changed yet).
- Fix approach: after the first real core bump, check that the plugin binary's timestamp moves. If
  not, add a dependency on the `.a`.

**Build-number seed:** resolved in Phase 4 (*Fixed in Phase 4*). One residual trap: a
`.CMakeBuildNumber` restored into a CI workspace would make the template increment it instead of
using the run id. No cache includes `plugin/cmake/`; keep it that way.

**Template packaging defects shipped as-is:**
- Quoted receipt id: the vendored `create-package.cmake.in` passes `--identifier
  '@MACOS_BUNDLEID@'` and `--version '@CMAKE_PROJECT_VERSION@'` through `execute_process` (no
  shell), so the pkg receipt is `'io.github.lee5783.synesthesia-vibe-obs'` and the version `'0.1.0'`,
  quotes included. Upgrades still match. Users must type the quotes for `pkgutil --forget`
  (`plugin/README.md`, *Uninstall*).
- dSYM in the payload: the Release pkg installs `synesthesia-vibe-obs.plugin.dSYM` (about 1.4 MB)
  next to the bundle. OBS ignores it; `plugin/README.md` removes it on uninstall.
- Installer title and `CFBundleDisplayName` are `synesthesia-vibe-obs`, not "SynesthesiaVibe
  Visualizer". `CFBundleVersion` is the local build counter.
- Files: `plugin/cmake/macos/resources/create-package.cmake.in` (vendored, do not edit).
- Fix approach: a project-owned `create-package.cmake` configured to the same build-dir path, the
  same override pattern as `plugin/packaging/macos/distribution.in`. Then update
  `plugin/README.md` (Option A, Uninstall steps 2–3) back.

**`distribution.in` override depends on configure order:**
- Issue: `plugin/CMakeLists.txt` overwrites `build_macos_arm64/distribution` after the template's
  `configure_file` inside `set_target_properties_plugin`. A template bump that moves that step to
  generate time or renames the output silently undoes the arm64-only restriction.
- Files: `plugin/CMakeLists.txt`, `plugin/packaging/macos/distribution.in`.
- Fix approach: run `grep hostArchitectures build_macos_arm64/distribution` after every configure
  that precedes a release (`docs/integrations.md`, *Packaging*).

**Only `en-US.ini`:**
- Files: `plugin/data/locale/en-US.ini`. OBS logs a benign `Failed to load '<locale>' text` line on
  other UI locales and falls back to en-US. All 35 UI strings are keys, so a new locale is one file.

**`fetch-dist.sh` rough edges:**
- Files: `browser-source/fetch-dist.sh`.
- `curl` has no `--connect-timeout` or `--retry`; a stalled connection hangs the script.
- The 404 message says "check that release v… exists" even when the missing file is the source
  tarball.
- The ~4 MB tarball is downloaded on every run, even when only the dist changed.
- A `SIGKILL` mid-install can leave `browser-source/.dist-stage.*` / `.dist-old.*`, which are not
  git-ignored.

## Known Limitations (plugin behaviour)

**All visualizers in one OBS share one PCM buffer (C-006):**
- Symptom: a visualizer on "None", or on a different source or mix track, reacts to audio another
  instance feeds. A new instance pushes one silent block into the shared buffer.
- Cause: `sv_push_pcm_float(ctx, …)` stores into the core's process-global
  `pcmLeftLpb` / `pcmRightLpb` (`external/synesthesia-vibe/src/audio/audiobuf.cpp`), ignoring
  `ctx`. Cannot be fixed in the plugin.
- Current handling: documented for users (`plugin/README.md`). Upstream ask #4 draft below.

**Stale PCM needs a silence push:**
- The core never expires stale PCM through its public API (`DrainAudioBufIfExpired` needs
  `SetAudioDrainClock`, which nothing installs). Without help, the levels freeze at their last
  values when audio stops. `push_captured_audio` therefore pushes one block of 2304 zeros after
  0.5 s without captured frames.
- After an off-air gap, the ring still holds up to ~1.5 s captured before it filled, so the first
  render pushes those "newest" frames for one frame (harmless).
- Upstream ask #6 draft below.

**Random textures get no texture:**
- The core's random-texture picker scans only `<resource path>/textures` inside the bundle, and the
  release ships no `textures/` folder (C-003). Presets with `rand…` samplers (for example
  "Tiger No.5 (Random texture flow)") render without one. Named textures are found next to the
  preset.
- `texture_search_path` must be set at `sv_create`, so a plugin-side setting would need a source
  re-create. The chosen fix is upstream ask #5 (host-settable random-texture folder), drafted in
  `PLAN.md`. Documented for users in `plugin/README.md`.

**Presets list and feedback:**
- The "Preset" list shows the last manual choice; it does not follow auto-cycle or hotkeys. With
  `preset` empty it shows "(first preset)" while another preset may be playing.
- A failed prepare only logs a warning; the list still shows the broken file as selected. No
  status line exists.
- The folder is not watched. New files appear the next time the dialog opens or the settings
  change. A cycle request that arrives while the folder is empty stays latched (in the core and
  in `cycle_requested`) until the next pick or settings change (N5).
- Random order can pick a broken file again after one other preset has played; only the last
  resolved path is excluded. Sequential order skips it for a full lap.
- A failed commit (`SV_ERR_GL` only) clears `preset_target`, so the next pick commits as a hard
  cut, not only a new source's first preset (N4).

**Transition timing:**
- After a cycle or blend change, the first interval follows the timer already armed for the
  current preset (core behaviour, no re-arm API). Measured: 12.0 s, then 6.0 s after 10/2 → 5/1.
- Turning auto-cycle back on after the armed time has passed fires one request at once.
- Resume after pause changes the preset at once if the interval is overdue (deliberate). A cycle
  request already accepted when pause is pressed still commits once.
- Hotkey presses faster than a prepare coalesce: the newest command wins. Two Previous presses
  during one prepare go back two steps.
- Hotkeys act on hidden sources; the preset commits on that source's next render.

**Settings the dialog rewrites on open:**
- OBS writes a list setting when the dialog opens if its stored value matches no item, which marks
  the dialog dirty and applies the change live. A folder-only source no longer does (P2-010b,
  confirmed in OBS on 2026-11-23). Remaining triggers:
  - `render_scale` is clamped to 50–100 but not snapped to 50 / 75 / 100. A value such as 60, set
    through the websocket, renders at 60 %; opening the dialog writes 100 (N2).
  - `audio_mix_track` outside 1–6 clamps on read; the dialog shows and writes Track 1.
  - A stored `preset` whose file was later deleted or renamed: opening the dialog switches the
    source to the first preset (N3).
- Fix approach: snap `render_scale` to the list on read; leave the rest documented. Test scripts
  must avoid these states (`.claude/common-issues/verification-tooling.md`).

**Resize and size:**
- Every resize restarts the feedback image: the core clears its frame targets and motion history,
  so the picture rebuilds for about a second.
- The texsize mode is fixed at create. `sv_create` classifies each axis as snapped (multiple of
  16) or exact and keeps that for later resizes: a source created at 1920×1080 and resized to
  1080×1920 has texsize 1088×1920. Output geometry is identical; cosmetic only.
- Width / height and render scale are separate atomics; an update that changes both can cause one
  intermediate resize (not observed).

**Audio routing:**
- "Whole mix" is the stream mix of one track: muted and "Monitor only" sources are not in it, and
  only what is routed to that track is. By design.
- Connecting to a mix makes libobs mix that track while the visualizer exists (a track nothing
  used is otherwise skipped). Render time is unchanged.
- A failed `audio_output_connect` or resampler creation is logged once and not retried until the
  settings change.
- Audio/visual sync is not adjustable (C-024): the audio pipeline is buffered relative to video by
  tens of ms, plus the core's analysis window.
- A source deleted and re-created gets a new UUID, so the saved setting shows "(missing)".

**Timing:**
- One core step is capped at 0.1 s (`max_step_seconds`), so on a canvas below 10 fps the
  animation runs slow (N8).
- The flag and the delta are two separate exchanges; a tick between them gives one repeat frame
  (no lost or doubled time).

**Diagnostics:**
- Log lines do not name the source instance, so with several visualizers they are ambiguous.
- A failed `os_task_queue_create` is not logged; presets would then never load, silently (N6).
  `os_task_queue_queue_task(NULL, …)` is null-safe.

**Teardown window shared with libobs's sidechain (N1):**
- If the chosen audio source's last reference drops on another thread while its audio thread is
  inside `audio_capture`, `obs_weak_source_get_source` returns NULL. Detach then skips the remove
  and destroys the resampler (or `destroy` frees the ring) while that last callback may still
  run. `obs_source_destroy` frees `audio_cb_list` right after the refcount reaches 0, so the window
  is a few instructions. `compressor-filter.c` has the same window. Never hit in any run.
- Files: `plugin/src/visualizer-source.cpp` (`detach_audio_source`).

## Security Considerations

**Signing keys:**
- Release pkgs and plugins are Developer ID signed and notarized, so users need no Gatekeeper
  override. Local builds stay ad-hoc with an unsigned pkg.
- Risk: the Developer ID keys sit in the `macos-signing` environment's secrets.
- Current mitigation: the environment accepts only `v*` tags, and only the macOS job of a signed
  tag build reads it. The `.p12` should hold only the two Developer ID identities, each with its
  own key, so a leak cannot sign as the account's other certificates.

**CI and release pipeline:**
- Risk: a pushed tag produces release binaries, and a fork PR runs repo workflows.
- Current mitigation:
  - Only the release job has `contents: write`. Only the macOS job of a signed tag build reads
    secrets, from the `macos-signing` environment.
  - Fork PRs run under `pull_request`, with a read-only token, `persist-credentials: false`, and
    caches scoped to the PR, which a tag run cannot read.
  - Event values reach scripts only through `env:`. The buildspec `name` and `version` are
    validated before they reach `GITHUB_ENV`.
  - Every release is a draft that a person publishes; final tags need `RELEASE_FINAL_ALLOWED`.
- Keep it that way: switching to `pull_request_target` or reading secrets on a PR or `master`
  path of the shared build workflow would undo most of this (`docs/integrations.md`, *CI/CD & Deployment*).

**User input in the plugin:**
- The folder path only selects files through `os_opendir`; it reaches only `sv_prepare_preset` and
  the log. Preset parsing is the core's.

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
  credentials for the local obs-websocket, signing identities, team IDs or notary profiles, or
  absolute paths in session artifacts.

## Performance Bottlenecks

**Preset commit on the graphics thread:**
- `sv_commit_preset` builds the GL programs and must run on the GL thread. Measured on an M1:
  0.9–3 ms for most presets, up to **17.8 ms** for glowsticks. That is inside the 33 ms budget at
  30 fps but over the 16.7 ms budget at 60 fps.
- At 1080p60 (P2-012, 2026-11-23) commits reached 20.7 ms (24 commits in 5 minutes, auto-cycle
  every 10 s with a 2.7 s blend), yet only 1 frame in 17 446 was skipped and `activeFps` held 60.
  A heavy commit costs at most an occasional frame; it does not stall the renderer.
- A blend renders two presets at once. One unsampled 150 s run right after an OBS launch showed
  +6 skipped frames during blends; five later windows showed 0.
- Improvement path: inherent to the core's prepare / commit split. Low priority after P2-012; if
  it ever matters, an upstream ask (incremental or cached program builds), not a plugin fix.

**Creating a visualizer skips frames:**
- `sv_init_gl` / `sv_create` and the first compile run in `create` under the graphics lock: about
  1.5–2.4 skipped frames per create (10 creates: 29 → 44 skipped). Steady state shows 0.

**`destroy` waits for an in-flight prepare:**
- `os_task_queue_destroy` joins the worker, so removing a source blocks libobs's destruction thread
  until the current prepare finishes (274–441 ms with pathological presets). It never holds the
  graphics lock, so rendering continues.

**Plugin render cost:**
- Median `averageFrameRenderTime` about 9–10 ms at the test canvas (30 fps), attached or not; the
  spike measured 4–7 ms with the synthetic tone. `gl_state_guard` adds about 20 µs per wrapped call.
- At 1080p60, 1920×1080 at 100 % (P2-012): `averageFrameRenderTime` 3.75–8.60 ms (median
  4.69 ms) against the 16.7 ms budget; render scale reduction was not needed.
- `GetStats` on the OpenGL backend measures CPU-side submission, not GPU fill: it reads about 7 ms
  at 1080p, 4K and 8K at every render scale. Render scale's saving shows only when the GPU is
  saturated (8192×8192 at 100 %: 47 ms / 21 fps → 7 ms / 30 fps at 50 %).
- 8192×8192 at 100 % is accepted but too heavy for an M1 at 30 fps. Point users at render scale
  for large sizes (`plugin/README.md`).
- Several instances each render the core and their own texrender; cost adds up (C-025, unmeasured).

**Most of the core builds without optimisation (C-029):**
- The core's top-level `CMakeLists.txt` `FORCE`-sets `CMAKE_CXX_FLAGS_RELEASE` and
  `CMAKE_C_FLAGS_RELEASE` (and `CMAKE_CXX_FLAGS` / `CMAKE_C_FLAGS`) to `""`. Its own targets add
  only `-std=c++17 -ffp-contract=off`.
- In the macOS inner build (`plugin/build_macos_arm64/synesthesia-vibe-core/build/build.ninja`),
  **65 of 75 compile units have no `-O` flag**; only 10 get `-O3`. So the core is mostly `-O0` in
  the plugin's Release builds, on Linux too.
- It feeds the heavy-commit cost (up to 20.7 ms at 1080p60 in P2-012), which P2-012 showed costs
  at most an occasional frame.
- No plugin-side workaround is applied. Injecting `-O2` through `CMAKE_PROJECT_INCLUDE` would be
  possible, but it is the user's call. The fix belongs upstream: ask #7 below.

**Several Browser Sources:**
- Each one has its own WebGL 2 renderer and audio capture. Reuse one source across scenes.

## Fragile Areas

**`gl_state_guard` must match what the core changes:**
- Files: `plugin/src/visualizer-source.cpp`, audit table in `PLAN.md` (*GL state*).
- Why fragile: OBS caches GL state and skips binds; any state the core changes that the guard
  misses corrupts the next OBS draw (`glUniformMatrix4fv … GL_INVALID_OPERATION`, black or
  misplaced quads, lost blending). Scissor, colour mask and clear colour are **not** guarded:
  `sv_render_frame` and `sv_resize` both disable scissor and set the colour mask to all-true. That
  is "no net change" only because OBS's GL device sets them on each use. The SPK-006 audit
  covered `sv_create`, `sv_load_preset`, `sv_render_frame` and `sv_destroy`; `sv_commit_preset`,
  `sv_resize` and `sv_set_transition_config` are guarded but were never audited state by state;
  the integrated P2-011 run (2026-11-23) showed other sources drawing correctly with 0 GL errors
  on the final build, and a follow-up run showed the same with filters on the visualizer and a
  source projector, a multiview and a program projector open. The unit range
  follows `GS_MAX_TEXTURES` (8).
- Safe modification: wrap every new GL-touching `sv_*` call in `with_core_gl_state`; re-run the
  audit on every core bump; guarding scissor and colour mask costs two `glGet`s.
- Evidence caveat: the committed SPK-006 state-diff evidence records the active unit as
  `GL_TEXTURE7 → GL_TEXTURE0`. The 7 came from the guard of that time, not OBS. The core always
  leaves `GL_TEXTURE0` active; the starting unit is whatever the host had active.

**Cycle-latch and pause bookkeeping:**
- Files: `plugin/src/visualizer-source.cpp` (`take_preset_job`, `switch_to_cycle_job`,
  `clear_cycle_request`, `preset_requested`, `toggle_auto_cycle_pause`, `previous_from_history`).
- Why fragile: the core asks for a preset once and latches until a load succeeds or fails. If the
  plugin ever drops a request without clearing the latch on the core's side (a later commit or a
  failed prepare), auto-cycle stops for good. `cycle_requested` must be cleared at exactly the
  three sites that match a core latch release.
- Safe modification: route every new way of changing the preset through the generation /
  worker path; never set the core's `preset_duration` to 0 to pause (it arms the core's
  disabled-timer default). A scripted race test (pick at the moment the timer fires) would need a
  debug hook.

**`plugin/exported-symbols.txt` and `plugin/exported-symbols.map`:**
- Why fragile: a new optional `obs_module_*` entry point that is not listed is hidden with no
  warning and OBS never calls it; listing one that is not defined fails the link. There are two
  lists (macOS with a leading `_`, Linux without), and they must stay in step.
- Safe modification: update both in the same change that adds the entry point. CI diffs each
  built binary against its list and also requires exactly 7 names, so a new entry point also
  means updating the count in `build-plugin.yaml`.

**Renderer check:**
- Files: `plugin/src/plugin-main.cpp`.
- Gaps: on refusal there is only a log line (a dialog needs `ENABLE_FRONTEND_API`); the value
  OBS's real Metal renderer returns from `gs_get_device_type()` was never observed (any value but
  OpenGL is refused, so correctness holds); loading a scene collection that already holds the
  source under refusal is untested.

**Skipped frames around profile switches and source creation (low priority):**
- In the P2-011 run, `renderSkippedFrames` rose by 13 (3 → 16) across the switch back from the
  1080p60 profile, which resets video, plus six create/remove cycles; `create` holds the graphics
  lock for `sv_init_gl` / `sv_create` (*Creating a visualizer skips frames* above). Not
  attributed to either; no GL error or visible fault came with it. An observation, not a bug.

**Preset name matching:**
- Files: `browser-source/obs-page.js` (`findMatch`).
- A `preset` or `audio` item made only of `+` or spaces becomes the empty key, which
  substring-matches every candidate and picks the first, with a warning. Harmless; treating an
  empty key as "no match" is a one-line fix.

## Dependencies at Risk

**libobs 31.1.1 vs the OBS that users run:**
- The plugin builds against libobs 31.1.1 (template buildspec) and is checked only in OBS 33.0.3.
  `plugin/README.md` says "OBS 31 or later" on the strength of the build target. D6 (minimum OBS
  31) is still proposed. Bump `plugin/buildspec.json` only together with D6.

**obs-plugintemplate packaging internals:**
- The `distribution.in` override and the quoted receipt id both depend on the vendored template's
  current `helpers.cmake` / `create-package.cmake.in`. Re-check both after a template bump.

**Core build on new SDKs:**
- About 105 deprecation warnings on the macOS 26 SDK; never build the core with `-Werror`. The
  core's CMake is not subdirectory-safe (upstream ask #1 in `PLAN.md`).

**The core on Linux:**
- The core's README says Windows and Linux are not release targets. It has no Linux preset or
  overlay triplet.
- At `v0.7.0` it builds under GCC 13 on Ubuntu 24.04 with no source change, using the plugin's
  ExternalProject settings (`docs/integrations.md`, *Linux build and package*).
- Its manifest still builds `glfw3` and `gtest`, which pull in X11 dev packages and autotools.
  Only the core's desktop tools use them (upstream ask #1).
- A future core bump may break the Linux build first. The stop rule is unchanged: never edit the
  core here; draft an upstream ask instead.

**Linux OBS floor and the PPA (C-009):**
- The Linux plugin builds against the PPA's current libobs, **32.2.0** at Phase 4, not the
  buildspec's 31.1.1. So Linux needs OBS 32.2.0+, against OBS 31+ on macOS (D6, proposed).
- The `.deb`'s `Depends: obs-studio` is **unversioned**, so apt does not enforce that floor;
  `plugin/README.md` tells users to check their OBS version.
- The PPA keeps only its current version, so a later rebuild can raise the floor silently. CI
  records it in each run summary (*Linux libobs*). The README and the notes for any release that
  ships a `.deb` must name the floor from the run that built it.
- Fix approach (not done): a versioned dependency, for example
  `CPACK_DEBIAN_PACKAGE_DEPENDS "obs-studio (>= <floor>)"` from the recorded version.
- Launchpad's PPA CDN returns 503s on and off. CI retries and refuses the distro OBS, but a long
  outage still fails the Linux job (`.claude/common-issues/plugin-build.md`).

**GitHub runner images and pins:**
- `macos-26` drops older Xcode patch versions over time, so `/Applications/Xcode_26.6.app` in
  `build-plugin.yaml` will stop existing; bump it when the job fails at `xcode-select`.
- Actions are pinned by SHA with no Dependabot, so updates are manual (`docs/integrations.md`,
  *Pinned actions*).
- `CORE_VERSION` in `release.yaml` must follow every submodule bump, or the release fails at
  *Validate core version*.

## Missing Critical Features

**Phase 1 sign-off:**
- The loopback checklist (BlackHole on macOS, VB-Cable on Windows) and the real microphone path
  inside OBS are pending with the user. Phase 1 is not done until they report back.

**Notarization (Phase 2 task 7):**
- Done in CI: tag builds sign and notarize, and `stapler` staples both the pkg and the `.plugin`
  (first run `v0.1.0-beta.1`, 2026-11-24; `docs/integrations.md`, *Signing and notarization*).

**Linux in OBS (P4-008):**
- The Linux `.so` and `.deb` are built and checked in CI (exports, PIC, `ldd`, `dlopen`, package
  contents) but have **never run in OBS**, on X11 or Wayland. The user owns this test on Ubuntu
  24.04.
- Until it passes, the `.deb` is only a workflow artifact, never a release asset, and
  `plugin/README.md` calls Linux experimental and untested.
- Not covered by the design either:
  - **Flatpak OBS** (Flathub) is unsupported: it cannot see a `/usr` install and would need a
    Flatpak extension.
  - Other distributions, other Ubuntu versions and aarch64 are unsupported (C-028).
  - If the test shows a GLX context, revisit the EGL-first loader (C-011).

**Plugin UI:**
- No "now playing" or failed-preset status line; no in-app notice on renderer refusal.

**Hosting, universal binary, Windows:**
- GitHub Pages (D5) is open and skipped: presets are approved for local serving only. The
  universal binary stayed out of Phase 4 and is a follow-up (C-025, *Tech Debt* above). Windows is
  Phase 3, and "each OS" in CI excludes it until then.

## Open Checks

Phase 2's OBS checks passed on 2026-11-23: the P2-011 Done-when run on the integrated build
(including the TC-M03 smoke load of the Phase 4 loader split), the P2-012 1080p60 measurement and
the P2-010b dialog check (`docs/testing.md`, *Done-when run, 2026-11-23*). Still open:

1. **P2-013 / TC-M04, pkg install into OBS: medium for the README, low for the code.** The
   2026-11-23 runs used a dev install (`ditto`). Install Options A and B from a notarized
   release, OBS loading the quarantined zip bundle, `pkgutil --pkgs` / `--forget` against a real receipt (and whether a
   `CurrentUserHomeDirectory` receipt needs `--volume ~`) are unverified end to end.
2. **Linux in OBS (P4-008):** X11 and Wayland on Ubuntu 24.04 (*Missing Critical Features*).
3. **Phase 1 loopback checklist:** with the user (*Missing Critical Features*).

How to run them: `docs/testing.md`.

## Upstream Ask Drafts (core repo)

Drafted, not filed. The user owns the core repo and files them; each becomes
`synesthesia-vibe#<n>` once filed. Asks #1–#3 and #5 are in `PLAN.md`, *Upstream asks*; #4, #6
and #7 are here.

**#4 — Per-context PCM buffer and a sample-rate parameter:**
"`sv_push_pcm_float(ctx, …)` stores samples in the process-global `pcmLeftLpb` / `pcmRightLpb`
(`src/audio/audiobuf.cpp`), so two contexts in one process analyse each other's audio. Hosts that
run several visualizers with different inputs (OBS sources) cannot isolate them.
- Move the PCM ring and its mutex into `sv_context`, so each context analyses only what was
  pushed to it.
- Add a sample-rate argument (or an `sv_config` field) instead of labelling every push as 44,100 Hz
  (`TARGET_SAMPLE_RATE`). Hosts running at 48 kHz then need no resampler of their own."

**#6 — Public reset or expiry of stale audio:**
"When a host stops pushing PCM, the analysis keeps the last samples forever:
`DrainAudioBufIfExpired` only works when `SetAudioDrainClock` is installed, and the public API
installs none. Hosts must push silence themselves to make levels fall to 0.
- Either add `sv_reset_audio(ctx)` that clears the context's PCM and analysis history,
- or let the core expire PCM older than a set age on its own render clock (an `sv_config` field
  for the age, off by default)."

**#7 — Optimisation flags per target instead of cleared global flags (C-029):**
"The top-level `CMakeLists.txt` `FORCE`-sets `CMAKE_CXX_FLAGS`, `CMAKE_CXX_FLAGS_RELEASE`,
`CMAKE_C_FLAGS` and `CMAKE_C_FLAGS_RELEASE` to `""`, and the library's own targets add only
`-std=c++17 -ffp-contract=off`. So a Release build compiles most of `SynesthesiaVibeCore` with no
optimisation: in one macOS arm64 build, 65 of 75 compile units had no `-O` flag. A host that
builds the library from source can't fix this without patching it.
- Give the library targets their optimisation level explicitly (for example
  `target_compile_options(… PRIVATE $<$<CONFIG:Release>:-O2>)` next to `-ffp-contract=off`),
  instead of relying on the cleared cache flags.
- Or stop `FORCE`-clearing `CMAKE_<LANG>_FLAGS_<CONFIG>`, so `CMAKE_BUILD_TYPE=Release` gets the
  toolchain's defaults. This overlaps ask #1, which asks not to `FORCE` the global flags.
- Keep whatever the cleared flags protect (for example float reproducibility) as explicit
  per-target options."

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
- The pkg install into OBS (TC-M04, *Open Checks*). Priority: Medium.
- The Linux build in OBS on X11 and Wayland (P4-008). Priority: High before Linux ships.
- The plain `ubuntu-x86_64` preset (CI uses `ubuntu-ci-x86_64`). Priority: Low.
- Real microphone and desktop-audio sources (10 ms chunks; only a Media Source was tested), the
  real Metal renderer, scene collections under refusal, relink after a core bump, two instances'
  combined cost. Priority: Medium.

---

*Concerns audit: 2026-11-23*
