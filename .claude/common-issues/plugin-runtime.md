# Plugin runtime in OBS (GL state)

> Part of the known-issues corpus — see [`../COMMON-ISSUES.md`](../COMMON-ISSUES.md) for the
> index. **Analysis date:** 2026-11-23 (OBS 33.0.3 OpenGL renderer, libobs 31.1.1, core `v0.7.0`).

## OBS GL errors or broken draws after a core call = GL state cache desync

- **Symptom:** the OBS log fills with `glUniformMatrix4fv failed … GL_INVALID_OPERATION`,
  `glActiveTexture failed`, `device_load_texture (GL) failed`; the visualizer shows as a black quad
  over part of the canvas; or other sources lose alpha blending or sample the wrong texture after
  the visualizer draws. It can come and go with draw order, screenshots and show/hide.
- **Root cause:** `libobs-opengl` caches GL state (`cur_program`, `cur_fbo`, `cur_viewport`,
  `cur_blend_state`, `cur_textures[8]`) and skips binds it thinks are redundant. The core changes
  that state inside `sv_*` calls, so OBS's cache no longer matches the real GL state.
- **Fix / workaround:** every GL-touching `sv_*` call must run inside
  `with_core_gl_state([&] { … })` (`plugin/src/visualizer-source.cpp`). `gl_state_guard` saves and
  restores program, VAO, `GL_ARRAY_BUFFER`, draw/read FBO, viewport, blend state, active unit and
  2D/3D textures on units 0–7. If the symptom appears:
  1. check the new code path calls the core through `with_core_gl_state`;
  2. if the core was bumped, re-run the GL state audit (`PLAN.md`, *GL state*) and add whatever new
     state it changes to the guard. Scissor and colour mask are not guarded today
     (`docs/concerns.md`).
- **Evidence note:** the committed SPK-006 state-diff evidence records the active unit as
  `GL_TEXTURE7 → GL_TEXTURE0`. The 7 was an artifact of the guard at the time, not OBS. The core
  always leaves `GL_TEXTURE0` active; the starting unit is whatever the host had active.
- **How to verify this is still current:** run the visualizer next to an image, a colour and a text
  source, toggle them and switch scenes; the OBS log must stay free of GL error lines.

## `sv_resize` writes the same unguarded state as `sv_render_frame`

- **Symptom:** none so far. Listed so the next GL-state investigation does not miss it: after a
  size or render-scale change, scissor is off, the colour mask is all-true and the clear colour is
  the core's, outside what `gl_state_guard` restores.
- **Root cause:** `sv_resize` rebuilds the core's frame targets (`FrameTargets`, `Framebuffer`,
  `Texture`, `BlurChain`, `MotionInterpolator`) and clears them (`ClearFramebuffer`). The framebuffer
  bindings, the active unit's 2D texture and the viewport it touches are guarded. Scissor, colour
  mask and clear colour are not; `sv_render_frame` writes the same three every frame
  (`blend_state.cpp` in the core). It is harmless only because libobs-opengl sets them on each use
  without caching them. `sv_resize` runs inside the guard, before `gs_texrender_begin`
  (`resize_core`, `plugin/src/visualizer-source.cpp`).
- **Fix / workaround:** none needed today. If OBS ever starts caching scissor or colour mask, or a
  draw after a resize shows clipped or colour-masked output, add them to `gl_state_guard` (two
  `glGet`s and two restores). Re-check after every core bump.
- **How to verify this is still current:** resize a visualizer next to image, colour and text
  sources and compare their pixels before and after (P2-009 found them bit-identical), with 0 GL
  error lines. The integrated check on the final build passed in the P2-011 run (2026-11-23):
  other sources drew correctly next to the visualizer and a second instance, 0 GL error lines.
