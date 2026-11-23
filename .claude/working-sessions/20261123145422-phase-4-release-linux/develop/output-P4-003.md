# P4-003

## Subtasks executed
- Moved the GL include and the get-proc loader out of `visualizer-source.cpp` into a thin shared
  header plus one loader file per OS (C-012).
- macOS loader: the existing `dlsym` on `OpenGL.framework`, byte-for-byte the same logic.
- Linux loader: `eglGetProcAddress`, then `dlsym` on `libOpenGL.so.0`, then `libGL.so.1` (C-011).
- Linux platform log line at load: `windowing platform: X11 (EGL)` / `Wayland (EGL)` / `unknown`,
  from `obs_get_nix_platform()`, right after the renderer check.
- macOS regression evidence: clean rebuild, export and import diff against a captured "before".

## Files changed
- `plugin/src/opengl-loader.hpp` (new): the only `#if` (`__APPLE__` → `<OpenGL/gl3.h>`, else
  `<GL/gl.h>` + `<GL/glext.h>`), declares `opengl_get_proc` and `opengl_log_platform`.
- `plugin/src/opengl-loader-macos.cpp` (new): `opengl_framework_path` and `opengl_get_proc` moved
  here unchanged; `opengl_log_platform` is empty (macOS log output unchanged).
- `plugin/src/opengl-loader-linux.cpp` (new): EGL-first loader with the GLVND fallbacks and the
  platform log.
- `plugin/src/visualizer-source.cpp`: includes `opengl-loader.hpp`; drops `<OpenGL/gl3.h>`,
  `<dlfcn.h>`, `opengl_framework_path` and `opengl_get_proc`. No other line changed.
- `plugin/src/plugin-main.cpp`: includes `opengl-loader.hpp`; calls `opengl_log_platform()` after a
  passing renderer check, before `visualizer_source_register()`.
- Source selection per OS is in `plugin/CMakeLists.txt` (counted under P4-004).

## What changed and why
`plugin/src/` had no platform split, and `gl_state_guard` calls `gl*` directly. The guard's calls
now get prototypes from `<GL/gl.h>`/`<GL/glext.h>` with `GL_GLEXT_PROTOTYPES` (set from CMake on
Linux, like `GL_SILENCE_DEPRECATION` on macOS) and resolve at link time against GLVND
`libOpenGL.so.0` (`OpenGL::OpenGL`). The core gets its pointers through `opengl_get_proc`.
`eglGetProcAddress` comes from `OpenGL::EGL`; `EGL_NO_X11` keeps `<EGL/eglplatform.h>` from pulling
in Xlib. The fallback libraries are opened once, on the first lookup that EGL can't answer, in a
thread-safe static initializer. No core call, guard content or threading changed.

## Verification
- Before: HEAD rebuilt (`** BUILD SUCCEEDED **`), `nm -gU`, `otool -L`, `nm -u` saved.
- After: `rm -rf build_macos_arm64 && cmake --preset macos-arm64 && cmake --build --preset
  macos-arm64` exit 0, `opengl-loader-macos.o` compiled, 0 warnings from `plugin/src` (105, all
  from the core, as before). Release config also builds.
- `nm -gU`: the same 7 `obs_module_*`; `otool -L`: identical; GL and `dlopen`/`dlsym` imports:
  identical; all 319 undefined symbols: identical.

## Evidence
- `evidence/P4-003-macos-export-import-diff.md`: before/after lists and diffs.
- OBS not driven (C-024): TC-M03 smoke load deferred with P2-011.

## Known limitations / follow-ups
- **The Linux loader has not been compiled anywhere yet.** The CI `Linux x86_64` job is its first
  compile (GCC 13, `-Werror`).
- `opengl_log_platform` on macOS is an empty function, so `plugin-main.cpp` needs no `#if`. If a
  macOS log line is wanted later, it goes there.
- If the user's Linux test (P4-008) shows a GLX context, revisit C-011 (OBS is believed to be
  EGL-only on Linux since around 30; libobs 31.1.1's enum has only `X11_EGL` and `WAYLAND`).
- Doc gaps (P4-012): `docs/structure.md` (the three new files, the per-OS pattern);
  `docs/conventions.md` *Import Organization* example lists `<OpenGL/gl3.h>`, `<dlfcn.h>` under
  `visualizer-source.cpp`, now in the loader files; `docs/architecture.md` if it names
  `opengl_get_proc`'s location; `docs/integrations.md` *Logs* (new Linux line
  `windowing platform: …`).

## Handoff for downstream review
- Acceptance criteria addressed: per-OS include and loader, macOS behaviour unchanged (proven by
  export/import diff), Linux EGL → `libOpenGL.so.0` → `libGL.so.1`, Linux platform log, no core or
  guard change.
- Areas to scrutinize: `reinterpret_cast<void *>(eglGetProcAddress(name))` (function-to-object
  pointer cast; fine on GCC without `-Wpedantic`); header order change in `visualizer-source.cpp`
  (GL now comes in through a project header before the OBS headers).
- How to run relevant tests: see the evidence file; Linux via CI.
