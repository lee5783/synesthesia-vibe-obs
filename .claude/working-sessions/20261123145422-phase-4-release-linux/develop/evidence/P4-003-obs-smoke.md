# P4-003 — macOS OBS smoke load (TC-M03)

Run on 2026-11-23, after the run's Gate 4, once the user had reopened OBS. It is part (d) of
P4-003's evidence, which was deferred under C-024.

**Result: pass.** The production plugin built from `4c260b5`, which includes the per-OS GL loader
split, loads and renders in OBS 33.0.3 on macOS.

| TC-M03 expectation | Observed |
|---|---|
| `renderer check passed (OpenGL, device type 1)` | `18:43:14.117: [synesthesia-vibe-obs] renderer check passed (OpenGL, device type 1)` |
| `sv_init_gl and sv_create succeeded` | `18:44:33.810: [synesthesia-vibe-obs] sv_init_gl and sv_create succeeded (1920x1080)` |
| A preset animates next to an image, colour and text source | `P4-003-obs-smoke-scene.png`. Presets prepared, committed and auto-cycled for the whole session. |
| 0 GL error lines | 0 in both OBS sessions |
| `Number of memory leaks: 0` on quit | Both quits logged `plugin unloaded`, then `Number of memory leaks: 0`. |

`opengl_get_proc` is now `opengl-loader-macos.cpp`. It resolved every function the core needed
for its whole run: the GL-state, stress, 1080p60, audio and teardown checks.

Full report, logs and scripts: the Phase 2 session's evidence, under
`.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/`:
- `P2-011/P2-011-done-when.md`
- `P2-012/P2-012-1080p60.md`
- `P2-010b/P2-010b-obs-check.md`

TC-M04 (installing the CI pkg) was not run: the plugin was dev-installed with `ditto`.
