# SPK-008: PLAN.md diff

Diff of `PLAN.md` from the state SPK-007 left to the state after SPK-008 (`git diff --no-index`, paths relabelled).

```diff
diff --git PLAN.md b/PLAN.md
index 87d2e79..1f733fc 100644
--- PLAN.md
+++ b/PLAN.md
@@ -48,7 +48,7 @@ current. Only `sv_prepare_preset*` and `sv_validate_preset*` may run on another
 | D1 | License | **GPL-2.0-or-later** for this repo. A plugin that links libobs must be GPL-compatible. The MIT core can be linked into it and keeps its own license. | **Locked (2026-11-23).** GPL-2.0-or-later. `LICENSE` holds the GPL-2.0 text; the "or any later version" grant is stated here and in `docs/decisions.md`. The MIT core (`external/synesthesia-vibe/`, release assets) keeps its own license and `NOTICE`. |
 | D2 | Platform order | macOS, then Windows; Linux is optional. macOS is where the core's GL path is validated, and OBS uses OpenGL there. | Proposed |
 | D3 | Scope | One source type. No filter, no dock panel, no Syphon / Spout / NDI output. | Proposed |
-| D4 | How the plugin gets the core | Git submodule `external/synesthesia-vibe/` pinned to a release tag and built from source by the plugin's CMake. This is one path for all OSes; Windows and Linux have no prebuilt desktop library. The macOS XCFramework is a fallback if building from source is a problem. | Proposed. The Phase 2 spike checks it. |
+| D4 | How the plugin gets the core | Git submodule `external/synesthesia-vibe/` pinned to a release tag and built from source by the plugin's CMake. This is one path for all OSes; Windows and Linux have no prebuilt desktop library. The macOS XCFramework is a fallback if building from source is a problem. | **Locked (2026-11-23), confirmed by the Phase 2 spike.** Submodule pinned to `v0.7.0` (`887628f`). The core's CMake is not subdirectory-safe, so it is **not** `add_subdirectory`'d. `plugin/CMakeLists.txt` configures, builds and installs it as a separate `ExternalProject_Add` build (Ninja, vcpkg from `$VCPKG_ROOT`) and links the installed `libSynesthesiaVibeCore.a` as an imported static target. One entry point: `cmake --preset` then build. The XCFramework fallback is not needed. Upstream ask #1 would allow `add_subdirectory` later. |
 | D5 | Browser Source hosting | GitHub Pages from this repo, plus instructions for running locally. The web dist is downloaded from the pinned core release by a script and is not committed. | Proposed |
 | D6 | Minimum OBS version | OBS Studio 31 or newer (Qt 6, current plugin template). | Proposed |
 
@@ -97,8 +97,8 @@ through a loopback device on macOS and on Windows, and cycles presets from the U
 
 ## Phase 2 — Native plugin, macOS (large)
 
-**Spike first (time-box: 2 days).** Answer these before building features; each answer goes
-into this file:
+**Spike first (time-box: 2 days). Done 2026-11-23**; see *Spike results* below the questions.
+Answer these before building features; each answer goes into this file:
 
 - **Building the core:** does the core's CMake build as a subdirectory with desktop tools and
   samples turned off? If not, file an upstream request (see *Upstream asks*).
@@ -150,10 +150,11 @@ into this file:
     program linked against the prefix ran `sv_validate_preset_memory` and got `SV_OK`
     (minos 12.0).
 
-    The plugin drives this with `ExternalProject_Add` from `plugin/CMakeLists.txt` (S-005, set up
-    in the next spike step). It uses the arguments above, an explicit `CMAKE_GENERATOR Ninja`
-    because the template's outer generator is Xcode, an install prefix inside the plugin build dir
-    and `BUILD_BYPRODUCTS <prefix>/lib/libSynesthesiaVibeCore.a`. Upstream ask #1 is confirmed.
+    The plugin drives this with `ExternalProject_Add` from `plugin/CMakeLists.txt`, and that works
+    with the template's Xcode generator. It uses the arguments above, an explicit
+    `CMAKE_GENERATOR Ninja` because the template's outer generator is Xcode, an install prefix
+    inside the plugin build dir and `BUILD_BYPRODUCTS <prefix>/lib/libSynesthesiaVibeCore.a`.
+    Upstream ask #1 is confirmed.
 - **Rendering into OBS:** inside `video_render`, between `gs_texrender_begin` and
   `gs_texrender_end`, read the bound draw framebuffer and pass it to `sv_render_frame`. Does the
   frame look right, and does OBS still draw its other sources correctly afterwards?
@@ -188,8 +189,8 @@ into this file:
 
     Saving `GL_CURRENT_PROGRAM` before every core call (`sv_init_gl`/`sv_create`, preset loads,
     `sv_render_frame`, `sv_destroy`) and restoring it afterwards removed every error and every
-    artefact seen. The spike needed nothing else. SPK-006 then audited the full state and
-    replaced this with a wider guard (see *GL state* below).
+    artefact seen. The full audit then replaced this with a wider guard (see *GL state*
+    below).
   - **Orientation:** the core renders with GL's bottom-left origin, but OBS treats row 0 of a
     texrender texture as the top, so the frame came out upside down. Drawing with
     `gs_draw_sprite(tex, GS_FLIP_V, 0, 0)` fixes it. This was checked with a probe preset whose
@@ -253,18 +254,18 @@ into this file:
   - Cost:
     - One save+restore measured 19.6–20.3 µs over 1200 calls. That is 0.12% of a 16.7 ms frame.
     - OBS render time with the visualizer was 6.9–10.3 ms against a 2.8 ms baseline in this
-      session, which matches SPK-005 within GetStats noise.
+      session, which matches the program-only guard within GetStats noise.
   - Why not a shared context: nothing failed that it would fix, and it costs more.
     - FBOs are not shared between contexts, so the core could not render straight into the
       texrender. It would render into a shared texture, with fences or `glFlush` between the
       two contexts.
     - It needs CGL context creation in OBS's share group, and the equivalent work on each OS.
-  - Hazards the audit found beyond the SPK-005 program bug:
+  - Hazards the audit found beyond the program bug:
     - `sv_create` runs on the UI thread and leaves the draw framebuffer at 0, while OBS's cache
       still names its own FBO.
     - The blend state left by `sv_render_frame` would make `gs_reset_blend_state` skip
-      re-enabling blending for the next source. SPK-005 only escaped this because the plugin
-      pushes and pops OBS's blend state around its own sprite draw.
+      re-enabling blending for the next source. The first build only escaped this because the
+      plugin pushes and pops OBS's blend state around its own sprite draw.
     - The core zeroes the 2D/3D bindings on the units it used, under OBS's texture cache.
 - **Loading GL functions:** the core ships its own glad loader and OBS has its own. On macOS,
   two-level symbol namespaces keep them apart. On Linux, the core's symbols must be hidden
@@ -296,8 +297,9 @@ into this file:
       shrank from 2.29 MB to 1.94 MB because `-dead_strip` now drops the unused core API.
     - The GL path is unchanged: the same `otool -L`, the same GL and `dlopen`/`dlsym` imports, and
       `sv_init_gl` still gets the plugin's `dlsym` loader on the OpenGL framework. A `dlopen`
-      probe with OBS's flags finds all 7 entry points and none of the core symbols. The OBS load
-      and render check is still pending.
+      probe with OBS's flags finds all 7 entry points and none of the core symbols. OBS 33.0.3
+      loads this build and renders with it: renderer check passed, `sv_create` succeeded, Fractopia
+      animating beside the other sources, `gl_errors 0` and no GL error lines in the OBS log.
     - The list must name only symbols the plugin defines: listing an undefined optional entry
       point fails the link. A wildcard is no good either, because `_obs_module_*` also re-exports
       the libobs imports `obs_module_get_config_path` and `obs_module_load_locale`. If an optional
@@ -333,10 +335,58 @@ into this file:
     `GetInputKindList` includes `synesthesia_vibe_visualizer`. The refusal branch was run from a
     scratch build that forced the device type and name through environment variables. The log
     shows the warning above, the module still loads, and `GetInputKindList` no longer lists the
-    source. The user's renderer setting was not changed (S-008). A real check under OBS's Metal
+    source. The user's renderer setting was not changed. A real check under OBS's Metal
     renderer is left for the user: switch Settings → Advanced → Renderer to Metal, restart, look
     for the warning, then switch back.
 
+**Spike results (2026-11-23)**
+
+All five questions are answered; none is left open. Verified on an Apple M1 with OBS 33.0.3
+(OpenGL renderer, GL 4.1 on Metal), libobs 31.1.1 from the template buildspec, core `v0.7.0`,
+Xcode 27.2 and CMake 4.4.5.
+
+| Question | Answer |
+|---|---|
+| Building the core | Not as a subdirectory. A separate `ExternalProject_Add` build plus install works, from one `cmake --preset` entry point (D4). Upstream ask #1 confirmed. |
+| Rendering into OBS | Yes. `sv_render_frame` into the texrender FBO inside `video_render`, once per tick, drawn with `GS_FLIP_V`. Other sources draw correctly. Adds about 4–7 ms of OBS render time per frame at 30 fps. |
+| GL state | Save and restore in OBS's own context (`gl_state_guard`, about 20 µs a call). No shared context. Upstream ask #2 confirmed. |
+| Loading GL functions | Isolated on macOS. The plugin exports only its 7 `obs_module_*` entry points. Linux needs a version script in the plugin; upstream ask #3 is optional. |
+| Renderer check | `gs_get_device_type()` in `obs_module_load`; anything but OpenGL logs a warning and registers no source. |
+
+- **Time against the 2-day box:** about 80 minutes of agent work (70 over the seven spike steps,
+  about 8 for the final load check, this summary and cleanup), inside a 7-hour wall-clock span.
+  About 5.4 hours of that span was a pause while the user fixed the obs-websocket login. Each
+  question took one pass, with no retry. Agent time is not a person-day measure, but the spike
+  finished well inside the box.
+- **Ready for the Phase 2 tasks:**
+  - Task 1, the CMake project, exists: `plugin/` builds libobs and the core and links an arm64,
+    ad-hoc-signed `.plugin` bundle (macOS only, no `OS_MACOS` guards yet).
+  - Task 2 has a working start: create/destroy under `obs_enter_graphics()`, the tick/render split,
+    the GL state guard, the renderer check and the export list.
+  - Stand-ins to replace: a synthetic tone instead of audio capture, a fixed 1920×1080 size, one
+    `preset_path` setting with no properties UI, a synchronous `sv_load_preset` inside
+    `video_render`, and a debug log line every 10 s at info level.
+- **Follow-ups the spike found** (fold into the tasks below):
+  - **Opaque draw.** Some presets (swarm) write alpha 0, and the sprite is drawn with blending off,
+    so screenshots, filters and nested scenes see the visualizer as transparent. Draw with an opaque
+    technique (for example `OBS_EFFECT_OPAQUE`) or force alpha to 1.
+  - **Clamp `dt` after hidden or inactive.** Off air, the source renders only when something draws
+    it, and the whole gap goes to the core as one step (5 s in a test). A screenshot of the
+    off-air source came back black. Clamp or reset the delta on `show`/`activate`.
+  - **1080p60 is unmeasured.** The user's OBS profile is 1080×1920 at 30 fps and was not changed.
+    The core always renders 1920×1080, so 4–7 ms per frame fits a 16.7 ms budget on paper. Measure
+    it with a dedicated profile before calling Phase 2 done.
+  - **Export-list catch.** `plugin/exported-symbols.txt` must name every `obs_module_*` entry
+    point the plugin defines. A new optional one that is not listed is hidden with no warning.
+  - **Linux version script** (Phase 4): `-Wl,--version-script` listing the same names, or
+    `-Wl,--exclude-libs,ALL`.
+  - **No in-app notice on renderer refusal**, only the log line. A dialog needs the frontend API
+    (`ENABLE_FRONTEND_API`); decide in Phase 2. The real Metal renderer was not exercised: that
+    check is left to the user.
+  - Also: presets are copied into the installed bundle by hand, so packaging must decide how they
+    ship under their local-only terms. The build is arm64 only; universal is task 7. Re-run the GL
+    state audit on every core bump.
+
 **Tasks after the spike**
 
 1. **CMake project** from the plugin template; links libobs and the core submodule.
@@ -408,16 +458,24 @@ copy is acceptable for the first release.
 | Risk | Impact | Mitigation |
 |---|---|---|
 | The core leaves GL state that OBS's GL device doesn't expect | Other sources draw wrong, or OBS crashes | Confirmed by the spike (program, VAO, framebuffers, viewport, blend, active unit, textures on units 0–7). Mitigated: `gl_state_guard` saves and restores exactly that state around every core call, at about 20 µs a call. Re-audit when the core version is bumped (upstream ask #2) |
-| Audio thread → render thread handoff | Crackle-free audio but a desynced or frozen visualizer, or a data race | Single-producer/single-consumer ring buffer; never call `sv_*` from the audio callback |
-| OBS on Metal (macOS) or a non-GL renderer | The plugin can't render | Check the renderer at load; refuse cleanly with a log message |
-| Duplicate glad symbols on Linux | Wrong GL entry points, crash at load | The plugin's link exports only its `obs_module_*` entry points: on macOS through `plugin/exported-symbols.txt` (done; it also stops cross-image weak-symbol coalescing), and on Linux, when Phase 4 gets there, through a version script or `--exclude-libs,ALL` |
-| Slow preset compiles stall OBS | Dropped frames on a live stream | `sv_prepare_preset` off-thread, commit on tick |
+| The core's CMake can't be embedded in the plugin build | The plugin can't build or link the core | Retired by the spike: `add_subdirectory` fails, but a separate `ExternalProject_Add` build plus install works from one `cmake --preset` entry point (D4). Needs `VCPKG_ROOT` and Ninja; the first configure takes about 3 minutes. Upstream ask #1 |
+| Xcode 27 and the macOS 26 SDK are newer than the template and libobs 31.1.1 | The build fails | Retired by the spike: libobs 31.1.1 builds cleanly. The core builds with about 105 deprecation warnings, so it is never built with `-Werror` |
+| Audio thread → render thread handoff | Crackle-free audio but a desynced or frozen visualizer, or a data race | Single-producer/single-consumer ring buffer; never call `sv_*` from the audio callback. Not exercised by the spike, which pushed a synthetic tone on the graphics thread |
+| OBS on Metal (macOS) or a non-GL renderer | The plugin can't render | Done in the spike: the renderer check at load refuses anything but OpenGL with a log warning and registers no source. Apple's GL-on-Metal driver passes. The real Metal renderer was not exercised, and there is no in-app notice yet |
+| Presets that write alpha 0 (swarm) | The visualizer shows transparent in screenshots, filters and nested scenes | Found by the spike. Draw with an opaque technique or force alpha to 1 (Phase 2) |
+| Duplicate glad symbols on Linux | Wrong GL entry points, crash at load | The plugin's link exports only its `obs_module_*` entry points: on macOS through `plugin/exported-symbols.txt` (done and loaded in OBS 33.0.3; it also stops cross-image weak-symbol coalescing), and on Linux, when Phase 4 gets there, through a version script or `--exclude-libs,ALL` |
+| Slow preset compiles stall OBS | Dropped frames on a live stream | `sv_prepare_preset` off-thread, commit on tick. The spike still loads with a synchronous `sv_load_preset` inside `video_render` |
 | GPL vs the MIT core and vendored code | Can't publish | D1 is settled (GPL-2.0-or-later); check the core's `NOTICE` for vendored licenses |
 | WebGL 2 / getUserMedia inside OBS's embedded Chromium | Phase 1 page is blank or silent | Test early on the oldest supported OBS; document the launch flag |
 
 ## Upstream asks (core repo)
 
-Open these on `lee5783/synesthesia-vibe` only if the Phase 2 spike confirms each is needed:
+Open these on `lee5783/synesthesia-vibe` only if the Phase 2 spike confirms each is needed.
+
+Status after the spike: #1 and #2 are confirmed, #3 is optional, and none blocks Phase 2. **None
+is filed.** The user owns the core repo and files them; each becomes `synesthesia-vibe#<n>` here
+once filed. Issue-ready text for all three is in
+`.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/SPK-008/spk-008-upstream-asks.md`.
 
 1. A CMake option to build only the library (no samples, desktop tools or tests) when the core is
    used as a subdirectory. The spike confirmed this is needed (see *Building the core* above).
```
