# SPK-006: GL state the core changes vs what OBS tracks

Setup: OBS 33.0.3, OpenGL renderer (GL 4.1 on Metal, Apple M1). Core `v0.7.0` (pinned submodule).
libobs reference source: obs-studio 31.1.1 in `plugin/.deps/`. Spike scene collection
`synesthesia-spike`. The OBS profile was not changed (1080×1920 base canvas, 30 fps).

## Method

A temporary instrumented build snapshotted 250 GL items with `glGet*`/`glIsEnabled` around each
core call. The items were: bindings, framebuffers, viewport/scissor, blend, depth/stencil/cull,
color mask, sRGB, pixel store, clear colour, point size/line width, the active unit, and the
2D/3D/cube/rect/2D-array texture and sampler binding on each of units 0–31. The build logged
two diffs:

- **raw**: just before and just after the core call. This is what the core changes.
- **guarded**: outside the save/restore guard. This is what OBS sees afterwards.

The snapshot drained `glGetError` before and after itself, so it could not leave an error for
OBS's next `gl_success` check. No snapshot ever raised one.

Runs:

1. Instrumented, SPK-005 program-only guard: log `07-39-25`.
2. Instrumented, full `gl_state_guard`, 4 minutes: 14 scene switches, 4 visualizer hide/show
   cycles, text/image/colour toggles, a second visualizer instance created then removed, and
   screenshots. Log `07-41-49`. The guarded diff was checked on every core call: 7200+ checks.
3. Final build with the instrumentation removed, same 4-minute script: log `07-46-14`.
4. Instrumented, preset switches: glowsticks v2 02 (uses 3D noise textures), Jester's Calling 2,
   swarm, back to Fractopia. Log `07-51-07`. Also timed an empty guard.

Raw log lines: `SPK-006-obs-log-instrumented.txt` and `SPK-006-obs-log-final.txt`.

## Raw diff: what each core call changes (observed)

| Core call | Thread | Items changed (before → after, typical values) |
|---|---|---|
| `sv_init_gl` + `sv_create` | UI thread, inside `obs_enter_graphics()` | `CURRENT_PROGRAM` 284 → 288; `VERTEX_ARRAY_BINDING` 3 → 20; `ARRAY_BUFFER_BINDING` 0 → 31; `ELEMENT_ARRAY_BUFFER_BINDING` 0 → 32 (VAO state); **`DRAW_FRAMEBUFFER_BINDING` 3 → 0**; `DRAW_BUFFER0` follows the FBO; **`VIEWPORT` 0,0,2580,994 → 0,0,1920,1080**; `COLOR_CLEAR_VALUE` → 0,0,0,0 |
| `sv_load_preset` (4 presets) | graphics | nothing (0 of 250) |
| `sv_render_frame` (Fractopia, Jester, swarm, glowsticks) | graphics, inside the texrender | `CURRENT_PROGRAM`; `VERTEX_ARRAY_BINDING`; `ARRAY_BUFFER_BINDING`; `ELEMENT_ARRAY_BUFFER_BINDING` (VAO state); `READ_FRAMEBUFFER_BINDING` 0 → core FBO; `READ_BUFFER` follows; **`BLEND` 1 → 0**; **`BLEND_SRC_RGB`/`DST_RGB`/`SRC_ALPHA`/`DST_ALPHA`** (preset-dependent: 770/771/1/771 → 1/1/1/1, 774/1/774/1, …); **`ACTIVE_TEXTURE` `GL_TEXTURE7` → `GL_TEXTURE0`**; **`UNIT0.TEXTURE_2D` 0 → core texture** |
| `sv_destroy` | UI/graphics, inside `obs_enter_graphics()` | nothing (0 of 250) |

The census items that never showed a net change:

- Sampler bindings on all units. The core's `PresetShaderInputs::Restore` binds 0.
- 2D/3D bindings on units 1–31. `Restore` binds 0, which is only "unchanged" because OBS had 0
  bound there.
- Pixel store. The core sets `UNPACK`/`PACK_ALIGNMENT` to 1, then back to 4.
- Scissor, depth, cull and stencil. The core only disables them, and they were already off.
- Color mask. The core sets all channels true, and they already were.
- `GL_FRAMEBUFFER_SRGB`, point size and line width.

## Against what OBS caches (libobs 31.1.1)

| State | Core changes it | What OBS does (source) | Effect if left changed | Guarded |
|---|---|---|---|---|
| Program | yes, every call | `gs_device.cur_program`; `device_draw` skips `glUseProgram` when its cache matches (`gl-subsystem.c`) | **Seen in SPK-005:** `glUniformMatrix4fv … GL_INVALID_OPERATION`, a black quad | yes |
| VAO | yes | Not cached: `device_draw` binds `vb->vao` or `empty_vao` every draw. But `gl_create_buffer(GL_ELEMENT_ARRAY_BUFFER, …)` (index buffer creation) binds and then unbinds into **whatever VAO is current** (`gl-helpers.c`) | Reverse hazard: OBS would wipe the core VAO's element binding | yes (the element buffer comes back with the VAO) |
| `GL_ARRAY_BUFFER` | yes | Not cached: bound before each use and reset to 0 (`gl-vertexbuffer.c`) | harmless | yes (1 call, keeps the diff empty) |
| Draw framebuffer | yes, in `sv_create` (→ 0) | `gs_device.cur_fbo`; `set_current_fbo` skips `glBindFramebuffer` when its cache matches | **OBS would render into the wrong framebuffer** after a UI-thread `create` | yes |
| Read framebuffer | yes, `sv_render_frame` | Not cached: `gl_copy_fbo` and the stage surface save and restore it with `glGet` | harmless | yes (1 call) |
| Viewport | yes, in `sv_create` | `gs_device.cur_viewport` feeds `gs_viewport_push`/`pop` and `gs_get_viewport` | A push/pop pair would re-apply the wrong rect | yes |
| Blend enable / func | yes, every frame | `graphics->cur_blend_state` in `graphics.c`; `gs_reset_blend_state` returns early when its cache says it is already enabled with the default func | **Other sources draw without alpha blending.** In SPK-005 this was masked only because the plugin's own `gs_blend_state_pop` re-issued the blend state after the frame | yes |
| Blend equation | sets ADD (no net change seen) | `cur_blend_state.op` | same as above | yes (cheap, set together) |
| Active texture unit | yes (`TEXTURE7` → `TEXTURE0`) | Not cached, but OBS texture creation, `gs_texture_unmap` and `gl_copy_fbo` bind and then unbind on whatever unit is active (`gl-texture2d.c`, `gl-helpers.c`) | OBS would zero a different unit's binding than it does without the plugin | yes |
| 2D texture, units 0–7 | yes (unit 0; `Restore` zeroes the others) | `gs_device.cur_textures[GS_MAX_TEXTURES = 8]`; `device_load_texture` skips the bind when its cache matches | **OBS samples the core's texture, or texture 0** | yes |
| 3D texture, units 0–7 | yes, for `noisevol` samplers (`Restore` zeroes them) | same cache. OBS binds 3D textures for LUT filters | same | yes |
| Textures, units 8+ | yes | OBS never uses them | none | no |
| Sampler objects | the core binds them, then binds 0 itself | OBS never binds sampler objects and assumes 0 | none observed | no |
| Pixel store | the core sets 1, then back to 4 | OBS never calls `glPixelStorei` and assumes the defaults | none observed | no |
| Scissor / depth / cull / stencil | disables only | Scissor is set per call; cull is cached (`cur_cull_mode`) but OBS keeps it off in normal rendering | none observed | no |
| Color mask | sets all true | `gs_enable_color` sets it per use (only `linux-capture` changes it) | none observed | no |
| Clear colour | yes, in `sv_create` | OBS sets it before every `glClear` | none | no (this is the only remaining guarded diff: 1 of 7200+ checks) |
| `GL_FRAMEBUFFER_SRGB` | no | queried with `glIsEnabled`; the plugin turns it off around `sv_render_frame` (SPK-005) | — | handled by the existing render code |

## Guarded diff with the final guard

- Run 2: `sv_create` 0 of 250 at start-up and 1 of 250 (clear colour only) for the second
  instance. `sv_load_preset` 0. `sv_render_frame` 0, every frame. `sv_destroy` 0. Totals:
  `guarded checks 7200, with any change 1`.
- Run 4: every preset load and every logged frame of every preset reads `guarded: 0 of 250
  items changed`.
- Runs 2–4 and the final run: **0 GL error lines** in the OBS log; the core reports
  `gl_errors 0`; `Number of memory leaks: 0` on quit.

## Decision

**Save/restore in the same context** (option a). The guard is `gl_state_guard` in
`plugin/src/visualizer-source.cpp`. It wraps exactly one core call at a time (`sv_init_gl` +
`sv_create`, `sv_load_preset`, `sv_render_frame`, `sv_destroy`) and saves and restores:

- program
- VAO
- `GL_ARRAY_BUFFER`
- draw and read framebuffers
- viewport
- blend enable, `glBlendFuncSeparate` factors and `glBlendEquationSeparate`
- active texture unit
- the 2D and 3D binding on units 0–7

Cost: 19.6–20.3 µs for one save+restore, measured over 1200 calls. That is about 0.6 ms per second
at 30 fps, and 0.12% of a 16.7 ms frame. Render time was the same as SPK-005 within GetStats noise
(`SPK-006-stats.txt`).

Option (b), a separate shared CGL context, was not needed. It would cost:

- context creation in OBS's share group;
- cross-context fences or `glFlush`;
- no direct rendering into the texrender FBO, because FBOs are not shared between contexts;
- per-OS work.

Nothing in it fixes a failure that (a) leaves.
