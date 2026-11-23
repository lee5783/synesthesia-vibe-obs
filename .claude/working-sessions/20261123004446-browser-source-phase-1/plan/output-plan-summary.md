# Implementation plan summary

> **Revision 2 (after Gate 1, 2026-11-23).** Applies `output-plan-gate-1-decisions.md`:
> - Scope is now Phase 1 plus the Phase 2 macOS spike.
> - GitHub Pages is dropped (BS-006 removed; D5 stays open).
> - D1 is locked as GPL-2.0-or-later, with a LICENSE file in this run.
> - All of C-004…C-020's recommended defaults are accepted.
> - The spike tasks (SPK-*) wait for a **second Gate 1** on the spike concerns before any spike
>   code is written.

## Goal

**Part A: Phase 1.** A static Browser Source page under `browser-source/` that OBS loads at
1920×1080, **served locally**:
- It renders SynesthesiaVibe full-window with no on-screen UI.
- It reacts to an audio input device picked by label (a loopback device or a microphone).
- It takes its preset list, auto-cycle, blend time, device and frame-rate cap from URL parameters.
- Around it: a fetch script that pulls the pinned core web dist (checked against the release
  checksum) plus the presets, and an OBS setup README.

**Part B: D1.** License the repo GPL-2.0-or-later: a root `LICENSE`, the D1 row locked in
`PLAN.md`, and a new `docs/decisions.md`.

**Part C: Phase 2 spike** (time-box 2 days, macOS, OBS 33.0.3 on this Mac). Answer each
"Spike first" question in `PLAN.md` with evidence and write the answers back into `PLAN.md`:
- the core as a pinned submodule, and whether it builds as a subdirectory
- a minimal `plugin/` on the obs-plugintemplate layout
- `sv_render_frame` inside `video_render`
- the GL-state audit
- glad isolation
- the renderer check

## Scope

**In scope**

- `browser-source/fetch-dist.sh`: download, verify and unpack the core web dist, then fetch the
  presets, textures and the core's LICENSE/NOTICE from the same tag's source tarball, generate
  `manifest.json`, and put it all in the git-ignored `browser-source/dist/`.
- `browser-source/index.html` + `browser-source/obs-page.js`: plain ES modules, no bundler, no npm.
- URL settings `preset`, `cycle`, `blend`, `audio` (including `none`), `fps`.
- Audio input from a device chosen by label through `getUserMedia`.
- `browser-source/README.md`: OBS setup for **local serving only** (URL, size, launch flag,
  loopback routing, OBS settings).
- An agent OBS check on this Mac, plus a manual checklist the user runs (macOS BlackHole +
  Windows VB-Cable). Phase 1 is done only after the user reports back.
- Root `LICENSE` (canonical GPL-2.0 text, fetched), the D1 row in `PLAN.md`, `docs/decisions.md`
  and its row in `docs/README.md`.
- Phase 2 spike: the `external/synesthesia-vibe/` submodule pinned to `v0.7.0`, a minimal
  `plugin/` CMake project, a spike source, and the answers written into `PLAN.md`.
- Reviews and doc passes for both parts.

**Out of scope**

- The GitHub Pages workflow and site (D5 stays open).
- Phase 2 feature tasks after the spike (audio capture, settings UI, hotkeys, packaging,
  notarization).
- Phases 3–4, the root `README.md`, and core edits (a gap becomes a `synesthesia-vibe#<n>`
  request, drafted but not filed without the user's go-ahead).
- Unit tests, code comments, on-screen controls in the page, and hearing OBS's own mix from the
  page (the known Phase 1 limit).

## Key decisions

- **Build on `SynesthesiaCanvas`, not raw `SynesthesiaContext`.** It owns the rAF loop, the fps cap,
  resize, visibility pause and WebGL context-loss recovery. The page only wires settings, presets
  and audio.
- **Auto-cycle uses the engine's timer.** It is the same pattern the core's web sample uses:
  `setTransitionConfig({ softCutSeconds: blend, presetDuration: cycle, presetDurationRand: 0, hardCutsEnabled: false })`
  and an `onPresetRequest` handler that loads the next preset with
  `request.suggestedBlendSeconds`. `cycle=0` → `presetDuration: 0` (off). The page needs no
  `setInterval` of its own.
- **Device by label.** The dist has no device picker. `AudioInput.fromMicrophone(target, constraints)`
  spreads `constraints` into the `audio` constraints, so the page resolves the label with
  `navigator.mediaDevices.enumerateDevices()` and passes `{ deviceId: { exact: id } }`. Labels
  are empty until permission is granted, so the page first opens a throwaway `getUserMedia`
  stream to unlock them. **No core change is needed.**
- **Download with `curl`, not `gh`,** so the script runs unauthenticated in CI. Verify with
  `shasum -a 256` (macOS) or `sha256sum` (Linux).
- **Presets are not in the web dist** (C-001/C-002, resolved). The script fetches the 10
  `resources/mvp-presets/*.milk` and `resources/beatdrop/textures/*` from the pinned tag's source
  tarball into `browser-source/dist/resources/`, which is git-ignored, never committed and never
  published. There is no `manifest=` parameter, since Pages is skipped.
- **Errors go to the console only** (no on-screen UI), per C-009 (default accepted).
- **D1:** GPL-2.0-or-later. The `LICENSE` text is fetched from gnu.org, not typed. The "or later"
  grant is stated in `PLAN.md` D1 and `docs/decisions.md` (C-021). The MIT core keeps its own
  licence.
- **The spike builds the core outside the plugin's CMake tree** as a recommendation, to be
  confirmed by SPK-002 (see *Phase 2 spike facts*). The core's CMake is not subdirectory-safe as
  shipped. Its `macos-arm64` preset plus `cmake --install` produces one merged static library and
  the header, which the plugin imports.
- **Spike code lives in `plugin/`** as the start of the real plugin, built on the pinned
  obs-plugintemplate layout, arm64-only, OBS 31.1.1 headers, tested on OBS 33.0.3 (S-001…S-004,
  pending the second Gate 1).
- Role split: the developer writes `LICENSE` and edits `PLAN.md`. `docs/decisions.md` and
  `docs/README.md` go to the document-writer, because `developer.md` reserves `docs/**` for that
  role. That is why D1 is two tasks (LIC-001, LIC-002).

## Web dist API facts (v0.7.0, verified from the zip — do not re-derive)

**Zip layout.** `synesthesia-vibe-web-0.7.0.zip` (sha256 `e9046312774c824ecbf627a1c0b75cb01d69ae9f79c591c4da07f9c042da5bc6`)
is **flat**, with no top-level folder. It holds 15 files, about 1.1 MB:

- `synesthesia-vibe.js` (barrel)
- `synesthesia-module.js`
- `synesthesia-canvas.js`
- `synesthesia-context.js`
- `resource-loader.js`
- `audio-input.js`
- `audio-input-worklet.js`
- `wav-feeder.js`
- `capture-session.js`
- `synesthesia-sync-group.js`
- `synesthesia-error.js`
- `webgl-context.js`
- `synesthesia-vibe-wasm.js`
- `synesthesia-vibe-wasm.wasm`
- `VERSION` (`0.7.0`)

It has **no README, no `.d.ts`, no LICENSE/NOTICE and no presets**. The wasm glue resolves the
`.wasm` relative to its own `import.meta.url`, so serving `dist/` as-is works. No COOP/COEP
headers are needed.

**Release URLs** (no auth):
`https://github.com/lee5783/synesthesia-vibe/releases/download/v0.7.0/synesthesia-vibe-web-0.7.0.zip`
and `.../v0.7.0/SHA256SUMS.txt`. The checksums file uses the format `<sha256>  <filename>`, one
line per asset. The tag carries a `v`; the asset name does not.

**Barrel exports** (`dist/synesthesia-vibe.js`): `AudioInput`, `CaptureSession`, `ResourceLoader`,
`SynesthesiaCanvas`, `PreparedPreset`, `SynesthesiaContext`, `SynesthesiaError`,
`SynesthesiaSyncGroup`, `loadSynesthesiaModule`, `WavFeeder`. Import it with a relative path
(`./dist/synesthesia-vibe.js`) or an import map. The core sample maps the bare specifier
`synesthesia-vibe`.

**`loadSynesthesiaModule(options = {})`** → `Promise<Module>`. It dynamically imports the wasm
glue next to itself.

**`ResourceLoader`**

- `new ResourceLoader(module)`.
- `await loader.load(manifestUrl, onProgress?)` fetches a JSON manifest:
  `{ "presets": [...], "textures": [...], "audio": [...] }`. Paths are relative to the manifest URL
  and each path segment is URL-encoded by the loader. Every file is written into the wasm MEMFS at
  `/synesthesiavibe/<category>/<basename>`.
- It returns `{ presets: [fsPath...sorted], textures: [...], audio: [...], bytes }`.
- `loader.presets()` lists `/synesthesiavibe/presets/*.milk`, sorted. `writeFile(category, name, data)`
  is also available.
- Statics: `ROOT_DIRECTORY="/synesthesiavibe"`, `PRESETS_DIRECTORY`, `TEXTURES_DIRECTORY`,
  `PRESET_EXTENSION=".milk"`, and `configuration()` →
  `{ resourcePath: "/synesthesiavibe", cachePath: "/synesthesiavibe/cache" }`. The canvas merges
  this configuration in by default.
- A fetch failure throws `SynesthesiaError(SV_ERR_IO, "<url>: HTTP <status>")`.

**`SynesthesiaCanvas`**

- `await SynesthesiaCanvas.create(canvas, module, options)`. `options` takes:
  - `configuration` (merged over `ResourceLoader.configuration()`). Keys: `maxFps`, `fpsHint`,
    `meshX/Y`, `texsizeX/Y`, `rngSeed`, `maxLuminance`, `flashLimit`, `deterministic`, …
  - `onError(SynesthesiaError)` (defaults to `console.error`)
  - `onContextCreated(context)`
  - `onBeforeRender(context, dt)`
  - `syncGroup`, `syncScaling`

  It creates a WebGL2 context with `alpha:false, antialias:false, preserveDrawingBuffer:false` and
  starts its own rAF loop.
- **fps cap:** `configuration.maxFps` (`0` or anything not finite = uncapped). The canvas paces rAF
  with it and also passes it to `sv_create`. It is set at create time only; the canvas has no
  runtime setter, which is fine for a URL-driven page.
- **Backing size:** `clientWidth/Height × min(devicePixelRatio, 2)`, kept in sync by a
  `ResizeObserver`. The canvas needs a CSS size (100vw × 100vh, `display:block`). OBS's DPR is 1,
  which gives 1920×1080.
- **Pausing:** the canvas pauses itself while `document.visibilityState === "hidden"` and resumes
  when the page is visible again. It also recovers from `webglcontextlost` / `webglcontextrestored`
  and reloads the last preset.
- `await view.loadPreset(fsPath, blendSeconds = 0)` prepares, waits one rAF, then commits. The
  latest call wins (earlier calls resolve `null`). It returns `{ prepareMs, commitMs }` or `null`.
  It throws `SynesthesiaError` on a parse or shader failure. `loadPresetText(text, blend)` also
  exists.
- `view.setTransitionConfig(partial)` merges over the defaults
  `{ softCutSeconds: 2.7, presetDuration: 16, presetDurationRand: 10, hardCutsEnabled: false, hardCutLoudness: 2.5, hardCutHalflife: 60 }`.
- `view.setBlendPattern(n)` takes `SynesthesiaContext.BLEND_PATTERN_RANDOM` (`-1`, the default) or
  an index into `SynesthesiaContext.BLEND_PATTERN_NAMES`. There are 19 patterns: Fade,
  Directional Wipe, Plasma, Radial, Clock, Spiral, Diamond, Nuclear Clock, Square, Checkerboard
  Wipe, Curtain, Bubble, Kaleidoscope, Moebius Strip, Star, Disco Floor, Fire, Whirlpool Drain,
  Julia Fractal.
- `view.onPresetRequest = ({ reason: "timer" | "hardCut", suggestedBlendSeconds }) => …` is
  delivered after a render once the engine's timer fires. The host picks and loads the next
  preset; the sample uses blend `0` on `hardCut`.
- Also: `pause()`, `resume()`, `setPlaybackRate(r)`, `onDebugInfo` (fields include `display_fps`,
  `bass/mid/treb`, `preset_name`, `last_error`), `setDebugInterval(s)`, `presetInfo()`, `close()`,
  and the getters `context`, `isPaused`, `isContextLost`.

**`AudioInput`**

- `static isMicrophoneSupported()` = AudioWorklet + `isSecureContext` + `getUserMedia`.
- `await AudioInput.fromMicrophone(target, constraints = {})` calls `getUserMedia({ audio: {
  channelCount: 2, echoCancellation: false, noiseSuppression: false, autoGainControl: false,
  ...constraints } })`. That spread is how the page passes `deviceId: { exact }`.
- The audio runs through an AudioWorklet that resamples to 44.1 kHz stereo and posts 512-frame
  blocks, which the wrapper sends to `context.pushPcm`. The output goes to a gain-0 sink, so it is
  inaudible.
- `target` may be the `SynesthesiaCanvas` itself (the wrapper reads `.context`, so it survives a
  context restore) or a function returning a context.
- On failure it throws `SynesthesiaError(SV_ERR_IO, "microphone unavailable: <DOMException message>")`.
  The original `DOMException` name (`NotAllowedError`, `NotFoundError`, `OverconstrainedError`) is
  **not preserved** in the wrapper's error, so the page cannot tell the cause from it (C-006).
- When the track ends (device unplugged) the input stops itself.
- Members: `isRunning`, `framesPushed`, `inputSampleRate`, `stop()`.
- It calls `audioContext.resume()`, but browser autoplay policy may leave the context suspended
  without a user gesture (desktop Chrome; OBS's CEF is expected to allow it). See C-008.
- Also: `fromMediaElement(target, el)`, `fromDisplayCapture(target)` (Chromium tab audio; not
  usable in OBS), and `WavFeeder`.

**`SynesthesiaError`** has `statusCode` with the values `SV_OK` 0, `SV_ERR_IO` 1, `SV_ERR_PARSE` 2,
`SV_ERR_SHADER` 3, `SV_ERR_GL` 4 and `SV_ERR_ARG` 5, and a `.status` name.

**Reference integration (core `samples/web/main.js` at v0.7.0)**

1. `module = await loadSynesthesiaModule()`
2. `await new ResourceLoader(module).load("resources/manifest.json")`
3. `view = await SynesthesiaCanvas.create(canvas, module, { configuration: { maxFps }, onError })`
4. `view.setTransitionConfig(...)`, `view.setBlendPattern(-1)`, `view.onPresetRequest = ...`
5. `view.loadPreset(presets[0], 0)`
6. `AudioInput.fromMicrophone(() => view.context)`

Sample defaults: blend 2 s, auto interval 30 s (auto off by default), fps 60.

**Presets in the pinned core tag** (`v0.7.0`): `resources/mvp-presets/` holds 10 `.milk` files,
which the README calls "the 10 presets the library is validated against". `resources/beatdrop/textures/`
holds 23 images. The 10 presets are:

- Eo.S. + Geiss - glowsticks v2 02 (Relief Mix)
- Eo.S. + Phat - CAT Scan (Nirvana flux)
- Eo.S. - dark side of the moon (plus a few more hits and a pill)
- Esotic & Rozzer - Now And Later
- Goody - Need - Transcendance remix
- Rovastar - Fractopia (Upspoken Mix)
- Rovastar - Jester's Calling 2
- Stahlregen & Flexi + Geiss - Tiger No.5 (Random texture flow)
- Unchained - Fuzzy Sciences
- shifter - swarm

The file names contain spaces, `+`, `&`, `'`, `.` and parentheses. **None contain a comma**, so
splitting the `preset` list on commas is safe for these. The core's
`build-web-sample.sh` states: *"Local serving only: the bundled presets and textures are approved
for sample artifacts, not for public hosting."* Its NOTICE says the presets come from BeatDrop
under a notice that ShaderToy-converted presets are CC-BY-NC-SA 3.0.

## Phase 2 spike facts (verified 2026-11-23 — do not re-derive)

**This Mac**

| Item | Value |
|---|---|
| OS / arch | macOS 26.7.2, arm64 (Apple M1) |
| Xcode | Xcode 27.2 (27B52), Apple clang 21.0.0 |
| CMake / Ninja | CMake 4.4.5, Ninja 1.13.2 |
| vcpkg | `VCPKG_ROOT=$HOME/vcpkg`, checked out at `5f96cd15fd`, which is **exactly the core's vcpkg baseline**. `arm64-osx` already installed. There is no `vcpkg` on PATH; use `$VCPKG_ROOT/vcpkg` |
| Code signing | 5 valid codesigning identities present (not needed locally; see below) |
| OBS | 33.0.3 at `/Applications/OBS.app`. It ships `libobs.framework`, `libobs-opengl.dylib` **and** `libobs-metal.dylib`, but **no headers** |
| OBS renderer | The user's `global.ini` has `Renderer=OpenGL`. The latest OBS log shows `Loading up OpenGL on adapter Apple Apple M1` and `OpenGL loaded successfully, version 4.1 Metal - 90.5, shading language 4.10`, which is Apple's GL-over-Metal driver. **OBS is on OpenGL here.** Metal can be selected because `libobs-metal.dylib` ships. Whether a fresh OBS 33 install defaults to Metal is unverified; SPK-004's renderer check covers it either way |
| OBS entitlements | `com.apple.security.cs.disable-library-validation` and `allow-unsigned-executable-memory`. A locally built, ad-hoc-signed plugin can load without a Developer ID (S-002) |
| User plugin dir | `~/Library/Application Support/obs-studio/plugins/` does not exist yet |

**Core build (`lee5783/synesthesia-vibe` at `v0.7.0`)**

- The repo has no nested submodules. `vendor/` (glad, hlslparser, stb) is committed. Dependencies
  come through vcpkg (`vcpkg.json`): `projectm-eval` and `glm`, plus `glfw3` and `gtest` for desktop
  tools. The baseline is `5f96cd15fd`.
- Root `CMakeLists.txt` (`cmake_minimum_required(VERSION 3.25)`, 693 lines):
  - It needs `-DSYNESTHESIA_VIBE_GL_DEV=ON`. Without it, it expects the DX9 tree and stops with
    `FATAL_ERROR` off Windows.
  - `SYNESTHESIA_VIBE_BUILD_DESKTOP_TOOLS` defaults ON on desktop (GLFW hosts, GoogleTest, tools)
    and can be set OFF.
  - `SYNESTHESIA_VIBE_BACKEND` is `gl` (desktop GL 3.3 Core, used on macOS) or `gles`.
  - **It is not subdirectory-safe as shipped:**
    - It uses `${CMAKE_SOURCE_DIR}` throughout (`src/`, `vendor/glad`, `vendor/stb`,
      `vendor/hlslparser`, `include/`), including in `src/synesthesia-vibe/CMakeLists.txt`.
    - It `FORCE`-overwrites the cache entries `CMAKE_CXX_FLAGS`, `CMAKE_C_FLAGS` and the `*_RELEASE`
      flags with `""`, which would clobber the parent's flags.
    - It calls `find_package(projectM-Eval REQUIRED)`, so it needs the vcpkg toolchain.
    - It sets `cmake_policy(SET CMP0091 OLD)`.
  - Upstream ask #1 in `PLAN.md` is therefore almost certainly needed. SPK-002 confirms it
    empirically.
- The `macos-arm64` preset uses Ninja, the vcpkg toolchain, triplet `arm64-osx`,
  `CMAKE_OSX_DEPLOYMENT_TARGET=12.0`, Release and `SYNESTHESIA_VIBE_GL_DEV=ON`. Its binary dir is
  `build/macos-arm64` **inside the source tree**, so it would dirty the submodule. Override it with
  `-B <outside dir>`.
- Output: target `SynesthesiaVibeCore` (STATIC). On Apple, a POST_BUILD `libtool -static` merges
  `projectM::ns-eel2` and `projectM::Eval` into it, so a **single `libSynesthesiaVibeCore.a`** comes
  out. `install(TARGETS …)` places the library under `lib/` and the `include/` tree under `include/`,
  giving a self-contained prefix for the plugin.
- glad: `vendor/glad` is glad2 2.0.8, GL 3.3 Core. `gl.c` is compiled **as C++** and its
  declarations are `extern "C"` (C-linkage `glad_gl*` symbols). `src/render/gl/gl_loader.cpp`
  calls `gladLoadGLUserPtr(…, &getProc)`, using the host's `sv_gl_get_proc_fn`. glad's function
  pointers are process-global inside whatever image links them.

**C API (`include/synesthesia-vibe/synesthesia-vibe.h`, 195 lines)**

- Setup and teardown:
  - `sv_status sv_init_gl(sv_gl_get_proc_fn)`, where `typedef void* (*sv_gl_get_proc_fn)(const char*)`.
    The plugin must supply one; OBS exposes none. On macOS, `dlsym` on the OpenGL framework image
    works, as does `CFBundleGetFunctionPointerForName` on `com.apple.opengl`.
  - `sv_context* sv_create(const sv_config*)`. `sv_config` fields: `width`, `height`, `mesh_x/y`,
    `texsize_x/y`, `fps_hint`, `resource_path`, `texture_search_path`, `rng_seed`, `deterministic`,
    the `pin_date` fields, the `pin_mouse` fields, `cache_path`, `max_fps`, `max_luminance`,
    `flash_limit`.
  - `sv_destroy`.
- Presets:
  - `sv_load_preset`, `sv_load_preset_memory`.
  - `sv_prepare_preset*` runs on any thread. `sv_commit_preset` runs on the GL thread.
    `sv_prepared_preset_free` runs on any thread.
  - `sv_validate_preset*` runs on any thread once an `sv_create` has returned.
- Rendering:
  - `void sv_render_frame(sv_context*, double dt_seconds, uint32_t target_fbo)`
  - `sv_resize(ctx, w, h)` takes backing pixels.
  - `sv_pause`, `sv_resume`, `sv_set_playback_rate`.
- Audio and diagnostics:
  - `sv_push_pcm_float(ctx, interleaved, frames, channels)`
  - `sv_get_audio_levels`, `sv_get_preset_info` (any thread), `sv_last_error`.
  - `sv_set_debug_callback` + `sv_debug_info` (fps, levels, preset name, GL errors and more),
    `sv_set_debug_interval`.
- Cycling: `sv_set_transition_config` (NULL restores defaults), `sv_set_blend_pattern`,
  `sv_set_preset_request_callback`. The engine never picks a preset; it asks the host from inside
  `sv_render_frame`, after the frame is presented.
- **Thread rule (header, verbatim intent):** `sv_init_gl`, `sv_create`, `sv_load_preset*`,
  `sv_commit_preset`, `sv_resize`, `sv_render_frame`, the debug setters and `sv_destroy` must run
  on the thread whose GL context is current.
- **The header says nothing about GL state.** It does not document what `sv_render_frame` leaves
  bound or changed.

**GL calls in the core** (census over `src/render/gl` + `src/synesthesia-vibe` at `v0.7.0`)

- State-changing calls by count: `glTexParameteri` 19, `glBindTexture` 15, `glDisable` 10,
  `glActiveTexture` 10, `glPixelStorei` 6, `glVertexAttribPointer`/`glEnableVertexAttribArray` 5,
  `glBindFramebuffer` 5, `glBindBuffer` 5, `glBindVertexArray` 4, `glEnable` 3,
  `glBlendEquation` 3, `glViewport` 2, `glColorMask` 2, `glBlendFuncSeparate` 2,
  `glBindSampler` 2, `glUseProgram` 1, `glPointSize` 1, `glClearColor` 1, `glBlendFunc` 1.
- Toggles: `GL_BLEND` (both ways), and `GL_CULL_FACE`, `GL_DEPTH_TEST`, `GL_SCISSOR_TEST`
  (disabled).
- **The core does no save/restore** of GL state. The only queries are `GL_CURRENT_PROGRAM` (once)
  and `GL_MAX_TEXTURE_SIZE`. SPK-006 must cover, at minimum: program, VAO, `GL_ARRAY_BUFFER` and
  element buffer, active texture unit plus the 2D binding and sampler on each unit, draw/read
  framebuffer, viewport, blend enable/func/equation, cull/depth/scissor enables, color mask,
  pixel-store alignment, clear color and point size. Compare these against what OBS's
  `libobs-opengl` `gs_device` caches (the current program/shaders, vertex and index buffer,
  textures[] and samplers[] per unit, render target/zstencil/FBO, viewport, blend state, and the
  VAO it binds once).

**obs-plugintemplate (`obsproject/obs-plugintemplate`, `master`, last push 2025-12-09)**

- Layout:
  - `CMakeLists.txt`
  - `CMakePresets.json`
  - `buildspec.json`
  - `cmake/{common,macos,windows,linux}/…`
  - `src/plugin-main.c`, `src/plugin-support.{c.in,h}`
  - `data/locale/en-US.ini`
  - `.github/{actions,scripts,workflows}`
  - `build-aux/` (format scripts)
  - `.clang-format`, `.gersemirc`
- `CMakeLists.txt`: `cmake_minimum_required(VERSION 3.28...3.30)`, `add_library(<name> MODULE)`,
  `find_package(libobs REQUIRED)` → `OBS::libobs`. The `ENABLE_FRONTEND_API` and `ENABLE_QT`
  options are OFF by default.
- `CMakePresets.json` `macos` preset: Xcode generator, `CMAKE_OSX_DEPLOYMENT_TARGET=12.0`,
  `CMAKE_OSX_ARCHITECTURES=arm64;x86_64` (universal), and `CODESIGN_IDENTITY`/`CODESIGN_TEAM` from
  the environment.
- **How it gets libobs:** at configure time, `cmake/common/buildspec_common.cmake` downloads the
  obs-studio **source** tag, prebuilt obs-deps and Qt6 (sha256-pinned in `buildspec.json`) into
  `.deps/`. It then configures obs-studio with plugins and frontend off, builds
  `obs-frontend-api`/libobs and installs the `Development` component into `.deps/`.
  `buildspec.json` pins obs-studio **31.1.1**, obs-deps `2025-07-11` and Qt6 `2025-07-11`. This
  matches D6 (OBS ≥ 31), and a plugin built against 31.1.1 is expected to load in 33.0.3.

## Risks

- **Preset licensing.** This is no longer a publishing risk because Pages is skipped. The presets
  stay local-only and git-ignored. Credits go in the README.
- **The core is not subdirectory-safe** (`CMAKE_SOURCE_DIR`, forced flag cache, vcpkg). Expect the
  spike to answer "no" and fall back to building the core separately. An upstream ask follows.
- **Universal vs arm64.** The template's macOS preset is universal, but the core's vcpkg build is
  one triplet per arch. The spike goes arm64-only (S-004).
- **GL state.** The core neither saves nor restores state, and OBS's `gs_device` caches state, so a
  mismatch can make other sources draw wrong. SPK-006 is the decisive check.
- **Two glad copies.** OBS's `libobs-opengl` has its own GL loader. Two-level namespaces should keep
  them apart on macOS; SPK-007 verifies it with `nm`/`otool`.
- **Time-box.** The spike is 2 days. If a question cannot be answered inside it, the answer is
  recorded as "open" with what was learned, rather than overrunning (`HARNESS.md` stuck capture).
- **WebGL2 / getUserMedia inside OBS's CEF.** OBS 33.0.3 is installed here, so the developer can
  test early. It needs browser-source hardware acceleration on and the `--enable-media-stream`
  launch flag, and on macOS OBS needs microphone permission (TCC).
- **No loopback device on this machine,** so the "Done when" (loopback on macOS and Windows) can
  only be partly verified by an agent. The user must run the final check (C-003).
- **Autoplay policy.** In desktop Chrome the AudioContext may stay suspended without a gesture.
  Evidence capture must use Chrome flags. OBS is expected to allow autoplay, but this is unverified.
- **`+` and `&` in preset names collide with URL query encoding** (`+` decodes to a space). Name
  matching has to be lenient (C-005).
- **Redistributing the dist without its notices.** The zip has no LICENSE/NOTICE, and the MIT and
  Emscripten licences require the notice to travel with copies (C-010).

## Impact

There is no existing application code. New or edited paths:

- Phase 1:
  - `browser-source/fetch-dist.sh`
  - `browser-source/index.html`
  - `browser-source/obs-page.js`
  - `browser-source/README.md`
  - `browser-source/dist/**` (generated, git-ignored)
- D1:
  - `LICENSE` (new)
  - `PLAN.md` (D1 row, status line)
  - `docs/decisions.md` (new)
  - `docs/README.md`
- Spike:
  - `.gitmodules` + `external/synesthesia-vibe/` (submodule at `v0.7.0`)
  - `plugin/**` (template-derived CMake, `src/`, `data/`)
  - `.gitignore` (plugin build dirs and `.deps/`, if not covered by `build/`)
  - `PLAN.md` (spike answers, D4, risks, upstream asks)
- `docs/**` after the doc passes.

User-facing surface:
- Phase 1: the local Browser Source URL and its query parameters.
- Spike: a "SynesthesiaVibe Visualizer" source on this Mac only, not distributed.

## References

- Task breakdown: [output-plan-tasks.md](output-plan-tasks.md)
- Test scenarios (high level): [output-plan-test-cases.md](output-plan-test-cases.md)
- Open questions and edge cases: [output-plan-concerns.md](output-plan-concerns.md)
- Analyzer inputs: none this run (no design reference; empty CodeGraph index). Requirements:
  [../inputs/requirements.md](../inputs/requirements.md)
