# SynesthesiaVibe for OBS — plan

Status: **Phase 1 built**, awaiting the user's loopback checklist (BlackHole on macOS, VB-Cable on
Windows) before it is marked done. **Phase 2 (macOS) built and reviewed**: tasks 1–6 are done;
task 7 (packaging) is partly done, with an ad-hoc signed `.plugin`, an unsigned `.pkg` and a zip,
and notarization waits on Developer ID credentials. Phase 2 is **not done**: its *Done when*
checks are deferred (the 1080p60 run, the integrated Done-when run, and the OBS check of the
"(first preset)" dialog fix). D1 and D4 are locked; D5 (Pages) is open and skipped for now. Last
updated 2026-11-23.

## Goal

Let streamers show SynesthesiaVibe in OBS Studio and have it react to the audio they are streaming,
with preset and transition control inside OBS.

Two deliverables, shipped in this order:

1. **Browser Source page.** A web page OBS loads as a Browser Source. Works on every OS today, with
   no native code. Audio input is limited (see Phase 1), so treat it as the quick path and a test
   of whether people want this.
2. **Native OBS plugin.** A source called "SynesthesiaVibe Visualizer" that renders inside OBS and
   listens to any OBS audio source or the whole mix. This is the real integration.

## What we build on

The core is the public repo **`lee5783/synesthesia-vibe`** (MIT, latest release `v0.7.0`). It
ships:

| Release asset | Used by |
|---|---|
| `synesthesia-vibe-web-<ver>.zip`: WebGL 2 / WASM dist with ES-module wrappers (`SynesthesiaCanvas`, `AudioInput`, `ResourceLoader`, `loadSynesthesiaModule`) | Phase 1 |
| `SynesthesiaVibeCore-<ver>.xcframework.zip` (iOS + macOS static frameworks) | Optional for macOS; see D4 |
| Source tree (CMake, `include/synesthesia-vibe/synesthesia-vibe.h`) | Phases 2–3, built from source |

These parts of the C API matter here:

- `sv_init_gl(get_proc)`, `sv_create(&config)`, `sv_destroy(ctx)`: setup and teardown. Both need
  the GL context current.
- `sv_render_frame(ctx, dt_seconds, target_fbo)`: renders into a framebuffer the host owns.
  **This is what lets OBS hand us its render target.**
- `sv_resize(ctx, width, height)`
- `sv_push_pcm_float(ctx, interleaved, frames, channels)`: pushes audio in.
- `sv_load_preset`, `sv_prepare_preset` + `sv_commit_preset`: preset loading. `sv_prepare_preset`
  parses and compiles off the render thread, so slow loads don't stall OBS.
- `sv_set_transition_config`, `sv_set_preset_request_callback`: auto-cycling and transitions.

**Threading rule from the core:** one `sv_context` is single-threaded and needs its GL context
current. Only `sv_prepare_preset*` and `sv_validate_preset*` may run on another thread.

## Decisions

| # | Decision | Proposal | State |
|---|---|---|---|
| D1 | License | **GPL-2.0-or-later** for this repo. A plugin that links libobs must be GPL-compatible. The MIT core can be linked into it and keeps its own license. | **Locked (2026-11-23).** GPL-2.0-or-later. `LICENSE` holds the GPL-2.0 text; the "or any later version" grant is stated here and in `docs/decisions.md`. The MIT core (`external/synesthesia-vibe/`, release assets) keeps its own license and `NOTICE`. |
| D2 | Platform order | macOS, then Windows; Linux is optional. macOS is where the core's GL path is validated, and OBS uses OpenGL there. | Proposed |
| D3 | Scope | One source type. No filter, no dock panel, no Syphon / Spout / NDI output. | Proposed |
| D4 | How the plugin gets the core | Git submodule `external/synesthesia-vibe/` pinned to a release tag and built from source by the plugin's CMake. This is one path for all OSes; Windows and Linux have no prebuilt desktop library. The macOS XCFramework is a fallback if building from source is a problem. | **Locked (2026-11-23), confirmed by the Phase 2 spike.** Submodule pinned to `v0.7.0` (`887628f`). The core's CMake is not subdirectory-safe, so it is **not** `add_subdirectory`'d. `plugin/CMakeLists.txt` configures, builds and installs it as a separate `ExternalProject_Add` build (Ninja, vcpkg from `$VCPKG_ROOT`) and links the installed `libSynesthesiaVibeCore.a` as an imported static target. One entry point: `cmake --preset` then build. The XCFramework fallback is not needed. Upstream ask #1 would allow `add_subdirectory` later. |
| D5 | Browser Source hosting | GitHub Pages from this repo, plus instructions for running locally. The web dist is downloaded from the pinned core release by a script and is not committed. | Open; skipped for now (2026-11-23). The page is served locally only, because the core approves its presets for local serving only. |
| D6 | Minimum OBS version | OBS Studio 31 or newer (Qt 6, current plugin template). | Proposed |

## Repository layout

```
PLAN.md                     ← this file
README.md
browser-source/             ← Phase 1: index.html, obs-page.js, fetch-dist.sh, README
plugin/                     ← Phase 2+: CMakeLists.txt, CMakePresets.json, src/, data/locale/en-US.ini
external/synesthesia-vibe/  ← pinned core submodule, never edited here
.github/workflows/          ← Pages deploy (deferred, D5), plugin builds (Phase 4)
docs/                       ← generated by codebase-mapper once code exists
```

The plugin follows the layout of the official `obsproject/obs-plugintemplate`, pinned at commit
`3e7d7ac3b5342cd7d9b88890b9c70b472d1520fc` (2025-12-09). Its `cmake/**`,
`src/plugin-support.c.in` and `src/plugin-support.h` are copied verbatim from that commit and
treated as vendored: no headers added, comments kept.

## Phase 1 — Browser Source (small)

**Tasks**

1. `browser-source/fetch-dist.sh <version>` downloads the web dist zip for the pinned core
   version, checks it against `SHA256SUMS.txt`, and unpacks it into `browser-source/dist/`
   (git-ignored).
2. `index.html` + `obs-page.js`: a full-window canvas with no on-screen UI and a black body
   background.
3. Settings come from the URL:
   - `preset`: one preset, or a comma-separated list.
   - `cycle`: auto-cycle seconds; `0` turns it off.
   - `blend`: transition seconds.
   - `audio`: input device label, or `default`.
   - `fps`: frame-rate cap.
4. Audio comes from the device picked with `audio=` (a loopback device such as BlackHole on macOS
   or VB-Cable on Windows, or a microphone). OBS has to be started with `--enable-media-stream`,
   and the page must be served over HTTPS or localhost.
5. A GitHub Pages workflow publishes `browser-source/` with the dist. **Deferred:** D5 is open, and
   Phase 1 ships local serving only.
6. `browser-source/README.md`: OBS setup steps (source URL, size, launch flag, loopback routing).

**Done when** a 1920×1080 Browser Source in OBS shows the visualizer reacting to music played
through a loopback device on macOS and on Windows, and cycles presets from the URL settings.

**Known limit:** the page cannot hear OBS's own audio mix, which is why Phase 2 exists.

## Phase 2 — Native plugin, macOS (large)

**Spike first (time-box: 2 days). Done 2026-11-23**; see *Spike results* below the questions.
Answer these before building features; each answer goes into this file:

- **Building the core:** does the core's CMake build as a subdirectory with desktop tools and
  samples turned off? If not, file an upstream request (see *Upstream asks*).
  - Answer (2026-11-23), submodule: the core is the submodule `external/synesthesia-vibe/`
    (HTTPS URL), pinned to tag `v0.7.0` at commit `887628f`. It has no nested submodules, so
    `git submodule update --init` is enough; `--recursive` is harmless. `v0.7.0` is a lightweight
    tag, so `git submodule status` labels the commit `v0.5.0-19-g887628f`; use
    `git -C external/synesthesia-vibe describe --tags` to see `v0.7.0`. The subdirectory question
    is answered in the next sub-bullet.
  - Answer (2026-11-23), subdirectory: **no.** `add_subdirectory(external/synesthesia-vibe)` with
    `SYNESTHESIA_VIBE_GL_DEV=ON`, `SYNESTHESIA_VIBE_BUILD_DESKTOP_TOOLS=OFF`,
    `SYNESTHESIA_VIBE_BACKEND=gl` and the vcpkg toolchain fails in three ways on CMake 4.4.5:
    - vcpkg reads the manifest from the parent's root, so nothing is installed and
      `find_package(projectM-Eval)` fails. Passing `VCPKG_MANIFEST_DIR` fixes that, but the
      manifest then also builds `glfw3` and `gtest`, which only the desktop tools use.
    - The core `FORCE`-sets `CMAKE_CXX_FLAGS`, `CMAKE_C_FLAGS` and their `_RELEASE` variants to `""`
      in the cache, which wipes the parent's flags (`-DPROBE_PARENT_FLAG` was `''` afterwards).
    - Its `${CMAKE_SOURCE_DIR}` paths resolve against the parent, so generation stops with six
      "Cannot find source file" errors (`vendor/glad/src/gl.c`, `src/audio/audiobuf.cpp`, …) and six
      targets with no sources.

    `cmake_minimum_required(VERSION 3.25)` and `cmake_policy(SET CMP0091 OLD)` are accepted. The
    policy line gives a deprecation warning only.
  - Answer (2026-11-23), working build: configure the core on its own with the build dir outside
    the submodule, then build one target and install. The submodule stays clean, because vcpkg's
    `vcpkg_installed/` defaults to the build dir. Run from the repo root:

    ```
    cmake -S external/synesthesia-vibe -B build/core-macos-arm64 -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
      -DVCPKG_TARGET_TRIPLET=arm64-osx \
      -DVCPKG_OVERLAY_TRIPLETS="$PWD/external/synesthesia-vibe/triplets" \
      -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 \
      -DCMAKE_BUILD_TYPE=Release -DSYNESTHESIA_VIBE_GL_DEV=ON -DSYNESTHESIA_VIBE_BACKEND=gl \
      -DSYNESTHESIA_VIBE_BUILD_DESKTOP_TOOLS=OFF -DCMAKE_INSTALL_PREFIX="$PWD/build/core-install"
    cmake --build build/core-macos-arm64 --target SynesthesiaVibeCore
    cmake --install build/core-macos-arm64
    ```

    These are the `macos-arm64` preset's cache variables plus desktop tools off and an install
    prefix. Timings: configure takes 9 s with a warm vcpkg binary cache and about 93 s cold. The
    build takes 10 s.

    The prefix holds `lib/libSynesthesiaVibeCore.a`: 4.6 MB, arm64 only, with projectM-Eval and
    ns-eel2 merged in. It also holds `include/synesthesia-vibe/synesthesia-vibe.h` and the
    Objective-C headers under `include/SynesthesiaVibeCore/`. There is no CMake package config, so
    the consumer declares an imported static target. Its link set is the core's: Foundation,
    AVFoundation, CoreAudio, OpenGL, AppKit, CoreVideo, IOSurface, QuartzCore and libc++. A C
    program linked against the prefix ran `sv_validate_preset_memory` and got `SV_OK`
    (minos 12.0).

    The plugin drives this with `ExternalProject_Add` from `plugin/CMakeLists.txt`, and that works
    with the template's Xcode generator. It uses the arguments above, an explicit
    `CMAKE_GENERATOR Ninja` because the template's outer generator is Xcode, an install prefix
    inside the plugin build dir and `BUILD_BYPRODUCTS <prefix>/lib/libSynesthesiaVibeCore.a`.
    Upstream ask #1 is confirmed.
- **Rendering into OBS:** inside `video_render`, between `gs_texrender_begin` and
  `gs_texrender_end`, read the bound draw framebuffer and pass it to `sv_render_frame`. Does the
  frame look right, and does OBS still draw its other sources correctly afterwards?
  - Answer (2026-11-23): **yes, it works, but only with one piece of GL state restored and one
    orientation fix.** Verified on OBS 33.0.3 (OpenGL renderer, GL 4.1 on Metal, Apple M1). The
    source renders Rovastar - Fractopia into a 1920×1080 `GS_RGBA` texrender, and an image, a
    colour and a text source in the same scene draw correctly. That held for more than 3 minutes
    and through 8 program-scene switches. The OBS log showed no GL errors and the core reported
    `gl_errors 0`.
  - The render path:
    - `create` runs `sv_init_gl` / `sv_create` inside `obs_enter_graphics()`, with a get-proc
      that is `dlsym` on `dlopen("/System/Library/Frameworks/OpenGL.framework/OpenGL")`.
    - `video_tick` adds up the frame delta.
    - `video_render` renders the core once per tick:
      `gs_texrender_reset` → `gs_texrender_begin` →
      `glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING)` → `sv_render_frame(ctx, dt, fbo)` →
      `gs_texrender_end`. Then it draws the texture with `OBS_EFFECT_DEFAULT` / `Draw` and
      `gs_draw_sprite`.
    - Rendering once per tick matters because a source can be drawn several times per frame
      (preview, program, screenshots).
  - **Without any save/restore, OBS breaks.** The core leaves its own program bound with
    `glUseProgram`, but `libobs-opengl` caches `cur_program` and skips `glUseProgram` when it
    thinks the right program is already bound. OBS then sets uniforms on the core's program and
    draws with it:
    - The log fills with `glUniformMatrix4fv failed … GL_INVALID_OPERATION`,
      `glActiveTexture failed` and `device_load_texture (GL) failed`, two sets per frame.
    - With the visualizer alone in a freshly loaded scene, the visualizer comes out as a black
      quad over a quarter of the canvas, and the rest stays empty.
    - With other sources in the scene the per-frame errors went away, because the draw order
      happened to resync the cache, but they came back in bursts on screenshots and when items
      were shown or hidden. Whether it breaks depends on draw order.

    Saving `GL_CURRENT_PROGRAM` before every core call (`sv_init_gl`/`sv_create`, preset loads,
    `sv_render_frame`, `sv_destroy`) and restoring it afterwards removed every error and every
    artefact seen. The full audit then replaced this with a wider guard (see *GL state*
    below).
  - **Orientation:** the core renders with GL's bottom-left origin, but OBS treats row 0 of a
    texrender texture as the top, so the frame came out upside down. Drawing with
    `gs_draw_sprite(tex, GS_FLIP_V, 0, 0)` fixes it. This was checked with a probe preset whose
    composite shader paints a white square in MilkDrop's top-left corner. Horizontal orientation
    was already correct.
  - **Colour and alpha:**
    - The core writes display-encoded values. Keep `GL_FRAMEBUFFER_SRGB` off during
      `sv_render_frame`. Then draw as `obs_source_draw` does: the `_srgb` texture setter plus
      framebuffer sRGB when `gs_get_linear_srgb()` is set, the plain setter otherwise.
    - The probe's 50% red gradient read back as 128, so colours are not encoded twice.
    - The sprite is drawn with blending off, because MilkDrop frames are meant to be opaque.
  - Cost: about 4–7 ms of OBS render time per frame (2 ms without the visualizer, 6–9 ms with
    it) at a 30 fps canvas. That fits a 60 fps budget, but 60 fps was not measured.
    - When the source is not shown, the accumulated delta is passed in one call when it comes
      back.
    - Audio was a synthetic tone pushed with `sv_push_pcm_float`; real capture is Phase 2
      work.
- **GL state:** list every piece of GL state the core changes that OBS's GL device tracks
  (program, VAO, blend, viewport, bound textures/samplers, framebuffer), and decide between
  saving and restoring it, or rendering in a separate GL context that shares textures with OBS.
  - Answer (2026-11-23): **save and restore, in OBS's own context.** No shared context. Verified
    on OBS 33.0.3 (OpenGL renderer, GL 4.1 on Metal, Apple M1) against core `v0.7.0` and the
    libobs 31.1.1 source.
  - The audit: a temporary build snapshotted 250 GL items before and after every core call:
    bindings, framebuffers, viewport, scissor, blend, depth/stencil/cull, colour mask, sRGB, pixel
    store, and the texture and sampler bindings on units 0–31. The table below lists what the core
    changed in four presets (Fractopia, glowsticks v2 02 with 3D noise textures, Jester's Calling
    2, swarm) and how OBS's GL device treats each item.

    | State | Changed by | OBS caches or assumes it? | Guarded |
    |---|---|---|---|
    | Program | every call | yes: `cur_program`, skips `glUseProgram` | yes |
    | VAO (and its element buffer) | `sv_create`, `sv_render_frame` | binds a VAO per draw, but creating an index buffer writes into whichever VAO is bound | yes |
    | `GL_ARRAY_BUFFER` | `sv_create`, `sv_render_frame` | no, bound before each use | yes (cheap) |
    | Draw framebuffer | `sv_create` (to 0) | yes: `cur_fbo`, skips the bind | yes |
    | Read framebuffer | `sv_render_frame` | no, saved and restored with `glGet` | yes (cheap) |
    | Viewport | `sv_create` | yes: `cur_viewport` feeds viewport push/pop | yes |
    | Blend enable, factors, equation | `sv_render_frame` (factors vary by preset) | yes: `cur_blend_state`; `gs_reset_blend_state` skips the calls | yes |
    | Active texture unit | `sv_render_frame` (always leaves `GL_TEXTURE0` active) | no, but texture create/unmap bind on the active unit | yes |
    | 2D/3D texture on units 0–7 | `sv_render_frame` (the core binds, then zeroes) | yes: `cur_textures[8]`, skips the bind | yes |
    | Textures on units 8+ | `sv_render_frame` | OBS never uses them | no |
    | Sampler objects | bound, then reset to 0 by the core | OBS never uses them and assumes 0 | no |
    | Pixel store | set to 1, then reset to 4 by the core | OBS assumes the defaults | no |
    | Scissor/depth/cull/stencil, colour mask | disabled or all-true; no net change | OBS sets them per use or keeps them off | no |
    | Clear colour | `sv_create` | OBS sets it before every clear | no |

    `sv_load_preset` and `sv_destroy` changed nothing. `GL_FRAMEBUFFER_SRGB` is not touched by the
    core; the plugin turns it off around `sv_render_frame` (see *Rendering into OBS*).
  - The guard (`gl_state_guard` in `plugin/src/visualizer-source.cpp`) wraps each core call on
    its own: `sv_init_gl` + `sv_create`, `sv_load_preset`, `sv_render_frame` and `sv_destroy`.
    It saves the items marked "yes" and restores them. It does not wrap OBS's own
    `gs_texrender_begin`/`gs_texrender_end`.
  - Proof:
    - The diff taken outside the guard was empty on more than 7200 checked core calls over a
      4-minute run. The run had scene switches, the visualizer shown and hidden, the text, image
      and colour sources toggled, and a second visualizer instance created and removed. The one
      exception was the clear colour after one `sv_create`.
    - The final build, with the audit logging removed, ran the same script with 0 GL error lines
      in the OBS log, `gl_errors 0` from the core, every source drawn correctly in
      `GetSourceScreenshot` captures, and no memory leaks on quit.
  - Cost:
    - One save+restore measured 19.6–20.3 µs over 1200 calls. That is 0.12% of a 16.7 ms frame.
    - OBS render time with the visualizer was 6.9–10.3 ms against a 2.8 ms baseline in this
      session, which matches the program-only guard within GetStats noise.
  - Why not a shared context: nothing failed that it would fix, and it costs more.
    - FBOs are not shared between contexts, so the core could not render straight into the
      texrender. It would render into a shared texture, with fences or `glFlush` between the
      two contexts.
    - It needs CGL context creation in OBS's share group, and the equivalent work on each OS.
  - Hazards the audit found beyond the program bug:
    - `sv_create` runs on the UI thread and leaves the draw framebuffer at 0, while OBS's cache
      still names its own FBO.
    - The blend state left by `sv_render_frame` would make `gs_reset_blend_state` skip
      re-enabling blending for the next source. The first build only escaped this because the
      plugin pushes and pops OBS's blend state around its own sprite draw.
    - The core zeroes the 2D/3D bindings on the units it used, under OBS's texture cache.
- **Loading GL functions:** the core ships its own glad loader and OBS has its own. On macOS,
  two-level symbol namespaces keep them apart. On Linux, the core's symbols must be hidden
  (`-fvisibility=hidden` or a version script).
  - Answer (2026-11-23): on macOS the two loaders are isolated, and the plugin now exports only
    its module entry points.
    - OBS's loader is not visible at all. `libobs-opengl.dylib` in OBS 33.0.3 holds 103 glad
      symbols (glad1: `gladLoadGL`, `glad_gl*`, `GLAD_GL_*`), every one `non-external (was a
      private external)`. OBS builds glad with hidden visibility, so nothing can bind to it.
    - Before the change the plugin exported 4,506 symbols, all from the core's static library:
      2,522 `glad_gl*`, 631 `GLAD_GL_*`, `gladLoadGL`/`gladLoadGLUserPtr`, the 26 `sv_*`, plus
      projectM-Eval, ns-eel2, stb and libc++ template code. `-fvisibility=hidden` from the
      template reaches only the plugin's own sources, not the prebuilt `.a`.
    - That was still safe for glad. The plugin is `TWOLEVEL`, OBS opens modules with
      `RTLD_LAZY | RTLD_FIRST | RTLD_LOCAL`, and the core's glad pointers have a `glad_` prefix,
      so they cannot collide with the GL framework's `gl*` functions the plugin calls directly
      (bound as `OpenGL/_glUseProgram` and so on).
    - The real leak was C++ weak definitions. Two-level namespaces don't cover these: dyld
      coalesces them across every loaded image. The plugin had `WEAK_DEFINES` and 492
      `weak-def-coalesce` binds, mostly core functions such as `BdUniformIntReimpl` and
      `EvaluatePerVertexEquations`. Another plugin linking a different core version could have
      swapped them silently.
    - Fix: `plugin/exported-symbols.txt` (Xcode `EXPORTED_SYMBOLS_FILE`, set in
      `plugin/CMakeLists.txt`) lists the 7 `obs_module_*` entry points the plugin defines.
      `load_module_exports` needs 3 of them (`obs_module_load`, `obs_module_set_pointer`,
      `obs_module_ver`) and looks up 8 optional ones. After the change, `nm -gU` and
      `dyld_info -exports` list exactly those 7. `WEAK_DEFINES` is gone, and the only weak binds
      left are libc++'s `operator new`/`delete` and the `std::length_error` typeinfo. The binary
      shrank from 2.29 MB to 1.94 MB because `-dead_strip` now drops the unused core API.
    - The GL path is unchanged: the same `otool -L`, the same GL and `dlopen`/`dlsym` imports, and
      `sv_init_gl` still gets the plugin's `dlsym` loader on the OpenGL framework. A `dlopen`
      probe with OBS's flags finds all 7 entry points and none of the core symbols. OBS 33.0.3
      loads this build and renders with it: renderer check passed, `sv_create` succeeded, Fractopia
      animating beside the other sources, `gl_errors 0` and no GL error lines in the OBS log.
    - The list must name only symbols the plugin defines: listing an undefined optional entry
      point fails the link. A wildcard is no good either, because `_obs_module_*` also re-exports
      the libobs imports `obs_module_get_config_path` and `obs_module_load_locale`. If an optional
      entry point is added later (`obs_module_post_load`, `obs_module_name`,
      `obs_module_description`, `obs_module_author`), add it to the list. Otherwise it is hidden
      without any warning and OBS never calls it.
  - Linux: `os_dlopen` uses `RTLD_LAZY`, which is local, and OBS builds its glad with hidden
    visibility there too. So today nothing in OBS exports `glad_gl*`. But ELF default-visibility
    symbols can be preempted: a library in global scope that exports `glad_gl*` would take over
    the plugin's references to them. The fix also belongs in the plugin's link step. Use a version
    script (`{ global: obs_module_load; …; local: *; };`, passed with `-Wl,--version-script`) that
    lists the same 7 names, or use `-Wl,--exclude-libs,ALL`. `-fvisibility=hidden` alone does not
    reach the prebuilt archive. Upstream ask #3 is therefore not needed (see *Upstream asks*).
- **Renderer check:** call `gs_get_device_type()` at load. If OBS isn't using OpenGL (for example
  the experimental Metal renderer on macOS), log a clear message and register nothing.
  - Answer (2026-11-23): the check runs in `obs_module_load`. The OBS frontend creates the
    graphics subsystem (`ResetVideo`) before it loads modules, so `obs_enter_graphics()` /
    `gs_get_device_type()` / `gs_get_device_name()` / `obs_leave_graphics()` work at load time and
    the check doesn't need to move to the first `create`. Any value other than `GS_DEVICE_OPENGL`
    refuses. That includes -1, which libobs returns when there is no graphics context. On refusal
    the module still returns `true`, so OBS loads it cleanly, and the source is never registered.
  - What the user sees on a non-OpenGL renderer: "SynesthesiaVibe Visualizer" is missing from the
    Add Source list. The OBS log has one warning:
    `[synesthesia-vibe-obs] SynesthesiaVibe Visualizer needs the OpenGL renderer; current renderer
    is <name> (device type <n>). The source is not registered.` The name and number come from
    libobs; the libobs 31.1.1 headers define only `GS_DEVICE_OPENGL` (1) and
    `GS_DEVICE_DIRECT3D_11` (2). There is no dialog. Scene collections that already hold the source
    were not tested under refusal. On OpenGL the log reads
    `renderer check passed (OpenGL, device type 1)`.
  - Apple's GL-on-Metal driver ("OpenGL 4.1 Metal - 90.5" in the OBS log) is OBS's OpenGL renderer
    and passes the check.
  - Verified on OBS 33.0.3 with the OpenGL renderer: the pass line is logged and
    `GetInputKindList` includes `synesthesia_vibe_visualizer`. The refusal branch was run from a
    scratch build that forced the device type and name through environment variables. The log
    shows the warning above, the module still loads, and `GetInputKindList` no longer lists the
    source. The user's renderer setting was not changed. A real check under OBS's Metal
    renderer is left for the user: switch Settings → Advanced → Renderer to Metal, restart, look
    for the warning, then switch back.

**Spike results (2026-11-23)**

All five questions are answered; none is left open. Verified on an Apple M1 with OBS 33.0.3
(OpenGL renderer, GL 4.1 on Metal), libobs 31.1.1 from the template buildspec, core `v0.7.0`,
Xcode 27.2 and CMake 4.4.5.

| Question | Answer |
|---|---|
| Building the core | Not as a subdirectory. A separate `ExternalProject_Add` build plus install works, from one `cmake --preset` entry point (D4). Upstream ask #1 confirmed. |
| Rendering into OBS | Yes. `sv_render_frame` into the texrender FBO inside `video_render`, once per tick, drawn with `GS_FLIP_V`. Other sources draw correctly. Adds about 4–7 ms of OBS render time per frame at 30 fps. |
| GL state | Save and restore in OBS's own context (`gl_state_guard`, about 20 µs a call). No shared context. Upstream ask #2 confirmed. |
| Loading GL functions | Isolated on macOS. The plugin exports only its 7 `obs_module_*` entry points. Linux needs a version script in the plugin; upstream ask #3 is optional. |
| Renderer check | `gs_get_device_type()` in `obs_module_load`; anything but OpenGL logs a warning and registers no source. |

- **Time against the 2-day box:** about 80 minutes of agent work (70 over the seven spike steps,
  about 8 for the final load check, this summary and cleanup), inside a 7-hour wall-clock span.
  About 5.4 hours of that span was a pause while the user fixed the obs-websocket login. Each
  question took one pass, with no retry. Agent time is not a person-day measure, but the spike
  finished well inside the box.
- **Ready for the Phase 2 tasks:**
  - Task 1, the CMake project, exists: `plugin/` builds libobs and the core and links an arm64,
    ad-hoc-signed `.plugin` bundle (macOS only, no `OS_MACOS` guards yet).
  - Task 2 has a working start: create/destroy under `obs_enter_graphics()`, the tick/render split,
    the GL state guard, the renderer check and the export list.
  - Stand-ins to replace: a synthetic tone instead of audio capture, a fixed 1920×1080 size, one
    `preset_path` setting with no properties UI, and a synchronous `sv_load_preset` inside
    `video_render`. The spike's 10 s debug log line was removed in the review fix pass.
- **Follow-ups the spike found** (fold into the tasks below):
  - **Opaque draw.** Some presets (swarm) write alpha 0, and the sprite is drawn with blending off,
    so screenshots, filters and nested scenes see the visualizer as transparent. Draw with an opaque
    technique (for example `OBS_EFFECT_OPAQUE`) or force alpha to 1.
  - **Clamp `dt` after hidden or inactive.** Off air, the source renders only when something draws
    it, and the whole gap goes to the core as one step (5 s in a test). A screenshot of the
    off-air source came back black. Clamp or reset the delta on `show`/`activate`.
  - **1080p60 is unmeasured.** The user's OBS profile is 1080×1920 at 30 fps and was not changed.
    The core always renders 1920×1080, so 4–7 ms per frame fits a 16.7 ms budget on paper. Measure
    it with a dedicated profile before calling Phase 2 done.
  - **`sv_context` race between tick and render.** The spike's `video_tick` pushes the synthetic
    tone with `sv_push_pcm_float` and writes `pending_seconds` / `frame_pending`, and it runs on
    the graphics thread without the graphics lock. `video_render` can run at the same time on
    another thread that holds only the graphics lock (obs-websocket `GetSourceScreenshot` does
    this) and calls `sv_render_frame` on the same context. That breaks the core's
    single-threaded rule. Nothing crashed in the spike, but it is a real race. Fix it in task 3:
    every `sv_*` call moves into `video_render`, and the tick side writes only atomics or the
    ring. Verify with TSan, or a debugger, on a repro that requests screenshots of the source in
    a loop while it renders on air.
  - **Export-list catch.** `plugin/exported-symbols.txt` must name every `obs_module_*` entry
    point the plugin defines. A new optional one that is not listed is hidden with no warning.
  - **Linux version script** (Phase 4): `-Wl,--version-script` listing the same names, or
    `-Wl,--exclude-libs,ALL`.
  - **No in-app notice on renderer refusal**, only the log line. A dialog needs the frontend API
    (`ENABLE_FRONTEND_API`); decide in Phase 2. The real Metal renderer was not exercised: that
    check is left to the user.
  - Also: presets are copied into the installed bundle by hand, so packaging must decide how they
    ship under their local-only terms. The build is arm64 only; universal is task 7. Re-run the GL
    state audit on every core bump.

**Tasks after the spike**

1. **CMake project** from the plugin template; links libobs and the core submodule. **Done.**
2. **Source lifecycle:** **Done.**
   - `create` and `destroy` run inside `obs_enter_graphics()` and call `sv_init_gl` /
     `sv_create` / `sv_destroy`.
   - `video_tick` accumulates the frame delta; `video_render` passes it to `sv_render_frame`.
   - `video_render` renders, then draws the result with OBS's default effect.
   - `get_width` / `get_height` report the size from the source settings.
3. **Audio:** **Done.**
   - A source-settings list lets the user pick any OBS audio source
     (`obs_source_add_audio_capture_callback`) or the whole mix (`audio_output_connect` on a
     chosen track).
   - The callback runs on OBS's audio thread. It interleaves OBS's planar float samples and
     writes them into a lock-free ring buffer.
   - `video_render` drains the ring into `sv_push_pcm_float` and consumes the accumulated `dt`,
     under the graphics context, right before `sv_render_frame`. `video_tick` only writes
     atomics; it never calls `sv_*` (see the race follow-up above).
   - Callbacks are removed before the source is destroyed, and when the user picks a different
     audio source.
4. **Presets:** **Done.**
   - Settings: a preset folder (path picker), the current preset, a random-order toggle, and the
     auto-cycle and transition times (`sv_set_transition_config`).
   - Loading happens off the render thread with `sv_prepare_preset`, then `sv_commit_preset`
     in `video_render`, under the graphics context.
5. **Hotkeys:** next preset, previous preset, random preset, and pause auto-cycle
   (`obs_hotkey_register_source`). **Done.**
6. **Resolution:** a width × height setting (default 1920×1080) that calls `sv_resize` when it
   changes; an optional render-scale setting to save GPU. **Done.**
7. **Packaging:** a signed and notarized `.plugin` bundle, plus install steps for
   `~/Library/Application Support/obs-studio/plugins/`. **Partly done:** an ad-hoc signed,
   arm64-only `.plugin`, an unsigned `.pkg` and a zip are built and inspected, and the install
   steps are in `plugin/README.md`. Notarization is documented (`docs/integrations.md`) and waits
   on Developer ID credentials. The pkg install into OBS is not verified yet.

**Done when** on macOS (Apple Silicon, OBS 31+):

- The source renders at 1080p60 without stalling OBS's renderer.
- It reacts to a chosen OBS audio source and to the whole mix.
- Changing preset or transition settings takes effect without restarting OBS.
- Removing the source, or quitting OBS, leaks nothing and crashes nothing.
- Other sources in the same scene still draw correctly (the GL-state check).

**Done-when state (2026-11-23):** not met. The user deferred these checks; each was verified per
task on intermediate builds, but not on the final integrated build:

- **Deferred:** the 1080p60 measurement in a dedicated profile (P2-012).
- **Deferred:** the Done-when run on the integrated build (P2-011): the GL-state check, both audio
  paths, live settings, removal and quit with no leaks, and the race stress test.
- **Deferred:** the OBS check of the "(first preset)" fix that stops the properties dialog from
  saving `preset` when it opens (P2-010b). The fix is built and reviewed.

## Phase 3 — Native plugin, Windows (large, blocked)

**Blocked until** the core's Windows GL build is verified again. That is pending in the core
repo; ask there before starting.

1. **Hidden GL context:** OBS uses Direct3D 11 on Windows, so the plugin makes its own
   window-less WGL context (a hidden window + `wglCreateContextAttribsARB`) on a thread it owns,
   or on the graphics thread with careful context switching.
2. **First version, CPU copy:** render into the plugin's own framebuffer, read the frame back
   through two alternating pixel buffers (`glReadPixels`), and upload it with
   `gs_texture_set_image` into a dynamic OBS texture. Simple and correct, but costs a frame of
   latency and some bandwidth.
3. **Faster version, GPU sharing:** share a D3D11 texture with GL using `WGL_NV_DX_interop2`,
   getting the D3D11 device from `gs_get_device_obj()`. Fall back to the CPU copy when the
   extension is missing.
4. Package a Windows installer or zip for `%ProgramData%\obs-studio\plugins\`.

**Done when** the Phase 2 checks pass on Windows 10/11 with NVIDIA, AMD and Intel GPUs. The CPU
copy is acceptable for the first release.

## Phase 4 — Release and Linux (medium)

1. CI builds the plugin for each OS on a `v*` tag and attaches the artifacts to a GitHub Release.
2. Linux (optional): OBS uses OpenGL/EGL there, so the macOS path should mostly carry over.
   Ship it only after a real test on Ubuntu with an X11 and a Wayland session.
3. Publish on the OBS forums resource page.
4. Run `codebase-mapper` to generate `docs/` and fill the routing table in `CLAUDE.md`.

## Risks

| Risk | Impact | Mitigation |
|---|---|---|
| The core leaves GL state that OBS's GL device doesn't expect | Other sources draw wrong, or OBS crashes | Confirmed by the spike (program, VAO, framebuffers, viewport, blend, active unit, textures on units 0–7). Mitigated: `gl_state_guard` saves and restores exactly that state around every core call, at about 20 µs a call. Re-audit when the core version is bumped (upstream ask #2) |
| The core's CMake can't be embedded in the plugin build | The plugin can't build or link the core | Retired by the spike: `add_subdirectory` fails, but a separate `ExternalProject_Add` build plus install works from one `cmake --preset` entry point (D4). Needs `VCPKG_ROOT` and Ninja; the first configure takes about 3 minutes. Upstream ask #1 |
| Xcode 27 and the macOS 26 SDK are newer than the template and libobs 31.1.1 | The build fails | Retired by the spike: libobs 31.1.1 builds cleanly. The core builds with about 105 deprecation warnings, so it is never built with `-Werror` |
| Audio thread → render thread handoff | Crackle-free audio but a desynced or frozen visualizer, or a data race | Single-producer/single-consumer ring buffer; never call `sv_*` from the audio callback. Not exercised by the spike, which pushed a synthetic tone from `video_tick`; that also races `video_render` on the same `sv_context` (task 3 moves every `sv_*` call into `video_render`) |
| OBS on Metal (macOS) or a non-GL renderer | The plugin can't render | Done in the spike: the renderer check at load refuses anything but OpenGL with a log warning and registers no source. Apple's GL-on-Metal driver passes. The real Metal renderer was not exercised, and there is no in-app notice yet |
| Presets that write alpha 0 (swarm) | The visualizer shows transparent in screenshots, filters and nested scenes | Found by the spike. Draw with an opaque technique or force alpha to 1 (Phase 2) |
| Duplicate glad symbols on Linux | Wrong GL entry points, crash at load | The plugin's link exports only its `obs_module_*` entry points: on macOS through `plugin/exported-symbols.txt` (done and loaded in OBS 33.0.3; it also stops cross-image weak-symbol coalescing), and on Linux, when Phase 4 gets there, through a version script or `--exclude-libs,ALL` |
| Slow preset compiles stall OBS | Dropped frames on a live stream | `sv_prepare_preset` off-thread, commit in `video_render` under the graphics context. The spike still loads with a synchronous `sv_load_preset` inside `video_render` |
| GPL vs the MIT core and vendored code | Can't publish | D1 is settled (GPL-2.0-or-later); check the core's `NOTICE` for vendored licenses |
| WebGL 2 / getUserMedia inside OBS's embedded Chromium | Phase 1 page is blank or silent | Test early on the oldest supported OBS; document the launch flag |

## Upstream asks (core repo)

Open these on `lee5783/synesthesia-vibe` only if the Phase 2 spike confirms each is needed.

Status after the spike: #1 and #2 are confirmed, #3 is optional, and none blocks Phase 2. **None
is filed.** The user owns the core repo and files them; each becomes `synesthesia-vibe#<n>` here
once filed. The draft text for each is inline below.

1. A CMake option to build only the library (no samples, desktop tools or tests) when the core is
   used as a subdirectory. The spike confirmed this is needed (see *Building the core* above).
   Draft, not filed: "Make the library-only build usable via `add_subdirectory`:
   - use `CMAKE_CURRENT_SOURCE_DIR` or `PROJECT_SOURCE_DIR` instead of `CMAKE_SOURCE_DIR`;
   - don't `FORCE` the global `CMAKE_*_FLAGS` cache entries; set per-target options instead;
   - guard top-level-only settings with `PROJECT_IS_TOP_LEVEL`;
   - move `glfw3` and `gtest` into a vcpkg manifest feature tied to
     `SYNESTHESIA_VIBE_BUILD_DESKTOP_TOOLS`;
   - optionally install an exported CMake package config for `SynesthesiaVibeCore`."
2. A documented list of the GL state `sv_render_frame` changes, or a flag to restore it. The
   spike confirmed this is warranted (see *GL state* above). The plugin works around it with
   its own guard, so it is not blocking. Draft, not filed:
   "Document the GL state the `sv_*` calls leave changed, so hosts that share a context (OBS,
   game engines) can save exactly that:
   - Measured at `v0.7.0`:
     - `sv_create` leaves the program, VAO, `GL_ARRAY_BUFFER`, the draw framebuffer (0), the
       viewport and the clear colour changed.
     - `sv_render_frame` leaves the program, VAO, `GL_ARRAY_BUFFER`, the read framebuffer, blend
       enable and factors (preset-dependent), `GL_ACTIVE_TEXTURE` (always `GL_TEXTURE0`) and the
       unit-0 2D texture changed. It also zeroes the 2D/3D binding and sampler on every unit a
       preset used, rather than restoring the host's.
     - `sv_load_preset` and `sv_destroy` leave nothing changed.
   - Either list this in the header, or add an opt-in `sv_config` flag that saves and restores
     that state in the core.
   - Keep the guarantee that the core sets every piece of state it relies on, on every call. Hosts
     will change state between calls."
3. Hidden symbol visibility for the static library on Linux. The spike found this is **not
   needed**: the plugin hides the core at its own link step (see *Loading GL functions* above).
   It stays as an optional hygiene request, not a blocker. Draft, not filed: "Build
   `SynesthesiaVibeCore` with `C_VISIBILITY_PRESET`/`CXX_VISIBILITY_PRESET hidden` and
   `VISIBILITY_INLINES_HIDDEN`. Mark the public `sv_*` functions with an `SV_API` macro set to
   default visibility. Hosts that link the static library into a shared object then export only
   the `sv_*` API, not glad, projectM-Eval, ns-eel2, stb or C++ weak definitions, and don't need
   an export list or version script. At `v0.7.0` the library exports 2,522 `glad_gl*` symbols and
   the header has no visibility macro."
5. A host-settable folder for random textures. Found in Phase 2: the core's random-texture picker
   scans only `<resource_path>/textures`, and the plugin's release ships no presets or textures,
   so presets that ask for a random texture get none. The plugin documents the limit in
   `plugin/README.md`. Not blocking. Drafted, not filed: "Let the host
   choose where random textures come from:
   - make the random-texture picker also scan `texture_search_path` (or add an `sv_config` field
     for a random-texture folder) instead of only `<resource_path>/textures`;
   - allow changing that folder after `sv_create` (for example `sv_set_texture_search_path`), so a
     host can follow the user's preset folder without re-creating the context;
   - keep the current behaviour when the host sets nothing."
   Asks #4 and #6 (per-context PCM buffer, public reset of stale audio) are drafted in
   `docs/concerns.md`.

## Evidence and testing

- **Browser Source:** screenshots of the page in Chrome, plus one of OBS showing the source.
- **Plugin:** OBS's release build can't be automated, so capture the OBS window with the OS
  screenshot tool and keep the relevant lines from the OBS log. A short clip only when motion is
  the point.
- No unit tests unless asked. If a test is requested, the audio ring buffer is the first candidate.

## Out of scope

Syphon / Spout / NDI output, OBS filters and dock panels, a Metal renderer, mobile OBS
alternatives, and editing presets inside OBS.
