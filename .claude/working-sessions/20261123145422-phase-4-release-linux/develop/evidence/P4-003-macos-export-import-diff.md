# P4-003 evidence: macOS regression check for the GL loader split

OBS was not driven (Gate 1, C-024). The macOS evidence is a clean build plus an export and import
diff against a "before" captured from the unchanged branch.

## Before (HEAD `a7b62ed`, sources unchanged)

`cmake --build --preset macos-arm64` (`** BUILD SUCCEEDED **`), then on
`build_macos_arm64/RelWithDebInfo/synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs`:
`nm -gU`, `otool -L`, `nm -u | grep -E '^_gl|dlopen|dlsym'` and the full `nm -u` were saved.

## After (P4-001, P4-003 and P4-004 applied), clean rebuild

```
$ rm -rf build_macos_arm64        # the fresh tree from P4-001; the pre-P4 tree had been moved aside
$ cmake --preset macos-arm64 && cmake --build --preset macos-arm64
...
** BUILD SUCCEEDED **
exit=0   (1:00.77 wall; 105 compiler warnings, all from external/synesthesia-vibe, 0 from plugin/src)
```

The new file was compiled:
`CompileC …/Objects-normal/arm64/opengl-loader-macos.o`. `opengl-loader-linux.cpp` is not part of
the macOS target.

### Exports

```
$ nm -gU synesthesia-vibe-obs | awk '{ print $3 }'
_obs_module_free_locale
_obs_module_get_string
_obs_module_load
_obs_module_set_locale
_obs_module_set_pointer
_obs_module_unload
_obs_module_ver
count=7
names identical to before; identical to plugin/exported-symbols.txt
```

`opengl_get_proc` and `opengl_log_platform` are hidden (default-hidden visibility plus the export
list), so they do not show up.

### Imports

```
$ diff before/otool-L.txt after/otool-L.txt
identical
$ diff before/nm-u-gl.txt after/nm-u-gl.txt
identical
$ diff before/nm-u-all.txt after/nm-u-all.txt      # all 319 undefined symbols
identical
```

`otool -L` (unchanged):

```
/System/Library/Frameworks/Foundation.framework/Versions/C/Foundation (compatibility version 300.0.0, current version 5027.1.8)
/System/Library/Frameworks/AVFoundation.framework/Versions/A/AVFoundation (compatibility version 1.0.0, current version 2.0.0)
/System/Library/Frameworks/CoreAudio.framework/Versions/A/CoreAudio (compatibility version 1.0.0, current version 1.0.0)
/System/Library/Frameworks/OpenGL.framework/Versions/A/OpenGL (compatibility version 1.0.0, current version 1.0.0)
/System/Library/Frameworks/AppKit.framework/Versions/C/AppKit (compatibility version 45.0.0, current version 2775.20.133)
/System/Library/Frameworks/CoreVideo.framework/Versions/A/CoreVideo (compatibility version 1.2.0, current version 776.2.0)
/System/Library/Frameworks/IOSurface.framework/Versions/A/IOSurface (compatibility version 1.0.0, current version 1.0.0)
/System/Library/Frameworks/QuartzCore.framework/Versions/A/QuartzCore (compatibility version 1.2.0, current version 1223.10.17)
@rpath/libobs.framework/Versions/A/libobs (compatibility version 1.0.0, current version 31.0.0)
/usr/lib/libc++.1.dylib (compatibility version 1.0.0, current version 2200.32.0)
/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1359.40.3)
```

GL and loader imports (unchanged):

```
_dlopen
_dlsym
_glActiveTexture
_glBindBuffer
_glBindFramebuffer
_glBindTexture
_glBindVertexArray
_glBlendEquationSeparate
_glBlendFuncSeparate
_glDisable
_glEnable
_glGetIntegerv
_glIsEnabled
_glUseProgram
_glViewport
```

### Release build

`cmake --build --preset macos-arm64 --config Release`: `** BUILD SUCCEEDED **`, 0 warnings from
`plugin/src`. Its package passed every inspection check (`P4-002-local-package-inspection.md`).

## Deferred

TC-M03 (OBS smoke load: `renderer check passed`, `sv_init_gl and sv_create succeeded`, 0 GL error
lines, `Number of memory leaks: 0`) is deferred with P2-011 (C-024). The Linux compile of
`opengl-loader-linux.cpp` is first proven by CI (P4-005).
