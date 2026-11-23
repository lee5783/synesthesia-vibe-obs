# P2-001 evidence — build guards

Run on 2026-11-23, macOS (Apple Silicon), CMake 4.4.5, Xcode generator, from `plugin/`.
`<scratch>` is a throwaway directory outside the repo; `<repo>` is the repository root.

## Refused configures

Each command exits 1 at `plugin/CMakeLists.txt:15`, before the build-number seed and the template
bootstrap: no `.deps` download or libobs build starts, and `plugin/cmake/.CMakeBuildNumber` was
byte-identical before and after all six runs.

### Non-Xcode generator (Ninja, arm64)

```
$ cmake -S . -B <scratch>/ninja -G Ninja -DCMAKE_OSX_ARCHITECTURES=arm64
CMake Error at CMakeLists.txt:15 (message):
  SynesthesiaVibe for OBS builds only on macOS, with the Xcode generator, for
  arm64 alone.  Detected host 'Darwin', generator 'Ninja',
  CMAKE_OSX_ARCHITECTURES 'arm64'.  Configure with 'cmake --preset
  macos-arm64' and build with 'cmake --build --preset macos-arm64'.


-- Configuring incomplete, errors occurred!
exit 1
```

### Universal architectures (Xcode, `arm64;x86_64`)

```
$ cmake -S . -B <scratch>/universal -G Xcode "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64"
CMake Error at CMakeLists.txt:15 (message):
  SynesthesiaVibe for OBS builds only on macOS, with the Xcode generator, for
  arm64 alone.  Detected host 'Darwin', generator 'Xcode',
  CMAKE_OSX_ARCHITECTURES 'arm64;x86_64'.  Configure with 'cmake --preset
  macos-arm64' and build with 'cmake --build --preset macos-arm64'.


-- Configuring incomplete, errors occurred!
exit 1
```

### Intel only (Xcode, `x86_64`)

```
$ cmake -S . -B <scratch>/x86 -G Xcode -DCMAKE_OSX_ARCHITECTURES=x86_64
CMake Error at CMakeLists.txt:15 (message):
  SynesthesiaVibe for OBS builds only on macOS, with the Xcode generator, for
  arm64 alone.  Detected host 'Darwin', generator 'Xcode',
  CMAKE_OSX_ARCHITECTURES 'x86_64'.  Configure with 'cmake --preset
  macos-arm64' and build with 'cmake --build --preset macos-arm64'.


-- Configuring incomplete, errors occurred!
exit 1
```

### No architecture given (Xcode, `CMAKE_OSX_ARCHITECTURES` unset)

```
$ cmake -S . -B <scratch>/noarch -G Xcode
CMake Error at CMakeLists.txt:15 (message):
  SynesthesiaVibe for OBS builds only on macOS, with the Xcode generator, for
  arm64 alone.  Detected host 'Darwin', generator 'Xcode',
  CMAKE_OSX_ARCHITECTURES ''.  Configure with 'cmake --preset macos-arm64'
  and build with 'cmake --build --preset macos-arm64'.


-- Configuring incomplete, errors occurred!
exit 1
```

### Template preset `macos` (universal)

```
$ cmake --preset macos
CMake Error at CMakeLists.txt:15 (message):
  SynesthesiaVibe for OBS builds only on macOS, with the Xcode generator, for
  arm64 alone.  Detected host 'Darwin', generator 'Xcode',
  CMAKE_OSX_ARCHITECTURES 'arm64;x86_64'.  Configure with 'cmake --preset
  macos-arm64' and build with 'cmake --build --preset macos-arm64'.


-- Configuring incomplete, errors occurred!
exit 1
```

It fails at configure, not at link as before. `cmake --preset macos-ci` prints the same message
(exit 1). The stub `plugin/build_macos/` both presets left behind (only `CMakeCache.txt` and
`CMakeFiles/`) was deleted afterwards.

## From-scratch configure and build (`macos-arm64`)

The previous `plugin/build_macos_arm64/` was moved to `<scratch>` (not deleted) and
`plugin/cmake/.CMakeBuildNumber` was removed. `plugin/.deps/` was reused.

```
$ ls -d build_macos_arm64 cmake/.CMakeBuildNumber
ls: build_macos_arm64: No such file or directory
ls: cmake/.CMakeBuildNumber: No such file or directory

$ cmake --preset macos-arm64
...
-- Configure OBS sources (universal) - done
-- Build OBS sources (Release - universal)
-- Install OBS sources (universal) - done
-- Configuring done (65.9s)
-- Generating done (0.0s)
-- Build files have been written to: <repo>/plugin/build_macos_arm64
exit 0   (first try; build number seeded 0, now 1)

$ cmake --build --preset macos-arm64
...
** BUILD SUCCEEDED **
exit 0   (30 s wall clock, warm vcpkg binary cache)
```

Warnings: 105 `warning:` lines, all 105 from `external/synesthesia-vibe/` (the core's inner Ninja
build: `sprintf` deprecations and similar). **None from `plugin/src`.**

A second `cmake --preset macos-arm64` on the fresh tree also passes (`Configuring done (7.7s)`).

## Output binary

```
$ lipo -info build_macos_arm64/RelWithDebInfo/synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs
Non-fat file: ... is architecture: arm64

$ nm -gU build_macos_arm64/RelWithDebInfo/synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs
00000000000037e4 T _obs_module_free_locale
0000000000003788 T _obs_module_get_string
000000000000380c T _obs_module_load
000000000000379c T _obs_module_set_locale
0000000000003730 T _obs_module_set_pointer
00000000000038b0 T _obs_module_unload
0000000000003748 T _obs_module_ver
       7   (sorted names identical to the `_`-lines of plugin/exported-symbols.txt)

$ codesign -dv build_macos_arm64/RelWithDebInfo/synesthesia-vibe-obs.plugin
Identifier=io.github.lee5783.synesthesia-vibe-obs
Signature=adhoc
```

OBS was not touched.
