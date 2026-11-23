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
