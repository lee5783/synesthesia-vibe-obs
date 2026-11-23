# Codebase Concerns

**Analysis Date:** 2026-11-23

`plugin/` holds the Phase 2 macOS plugin: tasks 1–6 are built and reviewed, packaging is partly
done (ad-hoc pkg and zip, no notarization), and three verification runs are deferred (see
*Deferred checks*). Gotchas with a quick workaround are in `.claude/COMMON-ISSUES.md`. Phases,
risks and the filed-or-not state of upstream asks #1–#3 and #5 are in `PLAN.md`.

## Fixed in Phase 2

Listed so nobody chases them again. Each was verified per task in OBS 33.0.3; the integrated
re-check is part of the deferred P2-011 run.

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

## Tech Debt

**Plugin builds only on macOS / arm64 / Xcode:**
- Issue: the triplet (`arm64-osx`), `CMAKE_OSX_ARCHITECTURES=arm64`, the Apple framework list,
  `XCODE_ATTRIBUTE_EXPORTED_SYMBOLS_FILE` and the OpenGL framework path are unconditional. The
  configure guard makes this explicit and refuses everything else, including the template's
  `windows-*` / `ubuntu-*` presets.
- Files: `plugin/CMakeLists.txt`, `plugin/CMakePresets.json`, `plugin/packaging/macos/distribution.in`.
- Fix approach: universal needs a second `x64-osx` core build plus `lipo`, and reverting
  `hostArchitectures` in `plugin/packaging/macos/distribution.in` to `arm64,x86_64` (Phase 4,
  C-016). Windows (Phase 3) and Linux (Phase 4) relax the guard per OS. Linux also needs
  `-Wl,--version-script` with the same 7 names, or `-Wl,--exclude-libs,ALL`.

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

**Build-number seed overrides CI numbers:**
- Issue: `plugin/CMakeLists.txt` writes `0` into `cmake/.CMakeBuildNumber` when it is missing, to
  avoid the template's empty-number first-configure failure. That bypasses the template's
  `GITHUB_RUN_ID` branch on CI.
- Files: `plugin/CMakeLists.txt`, `plugin/cmake/common/buildnumber.cmake` (vendored).
- Fix approach: revisit in Phase 4 when CI exists (e.g. skip the seed when `CI` is set).

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
  the dialog dirty and applies the change live. Remaining triggers after P2-010b:
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

**Unsigned pkg, ad-hoc plugin:**
- Risk: users must override Gatekeeper to install, which trains them to click through warnings.
- Current mitigation: `plugin/README.md` explains the per-macOS-version override; the CLI
  `installer -target CurrentUserHomeDirectory` needs no admin rights. Notarization is documented
  and waits on Developer ID credentials (C-002).

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
  30 fps but over the 16.7 ms budget at 60 fps, so a heavy preset change will likely skip a frame
  at 60 fps.
- A blend renders two presets at once. One unsampled 150 s run right after an OBS launch showed
  +6 skipped frames during blends; five later windows showed 0.
- Improvement path: inherent to the core's prepare / commit split. If P2-012 confirms it, an
  upstream ask (incremental or cached program builds) or a documented limit; not a plugin fix.

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
  **1080p60 is unmeasured** (P2-012 deferred).
- `GetStats` on the OpenGL backend measures CPU-side submission, not GPU fill: it reads about 7 ms
  at 1080p, 4K and 8K at every render scale. Render scale's saving shows only when the GPU is
  saturated (8192×8192 at 100 %: 47 ms / 21 fps → 7 ms / 30 fps at 50 %).
- 8192×8192 at 100 % is accepted but too heavy for an M1 at 30 fps. Point users at render scale
  for large sizes (`plugin/README.md`).
- Several instances each render the core and their own texrender; cost adds up (C-025, unmeasured).

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
  `sv_resize` and `sv_set_transition_config` are guarded but were never audited. Projector,
  multiview and filter paths were not exercised. The unit range follows `GS_MAX_TEXTURES` (8).
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
- The plugin builds against libobs 31.1.1 (template buildspec) and is checked only in OBS 33.0.3.
  `plugin/README.md` says "OBS 31 or later" on the strength of the build target. D6 (minimum OBS
  31) is still proposed. Bump `plugin/buildspec.json` only together with D6.

**obs-plugintemplate packaging internals:**
- The `distribution.in` override and the quoted receipt id both depend on the vendored template's
  current `helpers.cmake` / `create-package.cmake.in`. Re-check both after a template bump.

**Core build on new SDKs:**
- About 105 deprecation warnings on the macOS 26 SDK; never build the core with `-Werror`. The
  core's CMake is not subdirectory-safe (upstream ask #1 in `PLAN.md`).

## Missing Critical Features

**Phase 1 sign-off:**
- The loopback checklist (BlackHole on macOS, VB-Cable on Windows) and the real microphone path
  inside OBS are pending with the user. Phase 1 is not done until they report back.

**Notarization (Phase 2 task 7):**
- The release is ad-hoc signed and unnotarized. The commands are in `docs/integrations.md`; they
  run once the user supplies Developer ID credentials out of band.

**Plugin UI:**
- No "now playing" or failed-preset status line; no in-app notice on renderer refusal.

**Hosting, universal binary, Windows:**
- GitHub Pages (D5) is open and skipped: presets are approved for local serving only. Universal
  binary is Phase 4 (C-016). Windows is Phase 3.

## Deferred Checks

The user deferred these OBS runs; Phase 2's *Done when* is not met until they pass. Ordered by how
much the code makes them matter (Gate 2 review):

1. **P2-012, 1080p60: matters most.** The 17.8 ms commit is over the 60 fps budget, and blends
   render two presets. *Done when* item 1 ("renders at 1080p60 without stalling OBS's renderer")
   is the criterion most likely to fail or need a caveat.
2. **P2-011, GL-state check on the integrated build: high.** `sv_commit_preset`, `sv_resize` and
   `sv_set_transition_config` sit outside the spike audit; scissor, colour mask and clear colour
   stay unguarded. Per-task runs showed 0 GL errors and bit-identical other sources. Never run on
   the final build: projectors, multiview, filters, and a second instance added and removed next
   to image, colour and text sources.
3. **P2-011, race stress plus combined teardown on the final build: medium-high.** Each piece was
   stress-tested on its own (removal under prepare ×20, removal under hotkey fire, removal with
   audio attached). Not run: removal while audio is attached **and** a prepare is in flight, then
   quit; the 5-minute screenshot stress on the final build.
4. **P2-011, leak check on a normal quit: medium.** The last clean `Number of memory leaks: 0` was
   the P2-009 build. P2-010 / P2-010b changed only properties and defaults, but P2-010's session
   ended in a SIGINT and a Qt abort (no plugin frames).
5. **P2-013 pkg install into OBS: medium for the README, low for the code.** Install Option A, the
   Gatekeeper steps, quarantine on the zip, `pkgutil --pkgs` / `--forget` against a real receipt
   (and whether a `CurrentUserHomeDirectory` receipt needs `--volume ~`) are unverified end to end.
6. **P2-010b dialog check: low code risk.** The fix follows OBS's own `count && idx == -1` rule
   (checked in the 31.1.1 source), but the screenshot and the `GetInputSettings` check are still
   mandatory before it counts as done.

How to run each: `docs/testing.md`, *Planned, not yet run*.

## Upstream Ask Drafts (core repo)

Drafted, not filed. The user owns the core repo and files them; each becomes
`synesthesia-vibe#<n>` once filed. Asks #1–#3 and #5 are in `PLAN.md`, *Upstream asks*.

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
- Everything in *Deferred Checks*. Priority: High for P2-012 and the P2-011 GL-state check.
- Real microphone and desktop-audio sources (10 ms chunks; only a Media Source was tested), the
  real Metal renderer, scene collections under refusal, relink after a core bump, two instances'
  combined cost. Priority: Medium.

---

*Concerns audit: 2026-11-23*
