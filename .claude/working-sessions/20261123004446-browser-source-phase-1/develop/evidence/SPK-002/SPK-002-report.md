# SPK-002 evidence — core as a CMake subdirectory, and the separate-build fallback

Run 2026-11-23 on this Mac: macOS 26.7.2 arm64, Xcode 27.2 (AppleClang 21.0.0), CMake 4.4.5,
Ninja 1.13.2, `VCPKG_ROOT=$HOME/vcpkg` at the core's baseline `5f96cd15fd`. Core submodule at
`v0.7.0` (`887628f`). All commands run from the repo root. Paths below are scrubbed to repo-relative
or `$HOME/…`. The raw `.log` files next to this report are git-ignored.

## Step 1 — `add_subdirectory()` from a throwaway parent: fails

Parent: `build/core-subdir-probe/CMakeLists.txt` (git-ignored, not committed). It sets
`SYNESTHESIA_VIBE_GL_DEV=ON`, `SYNESTHESIA_VIBE_BUILD_DESKTOP_TOOLS=OFF`,
`SYNESTHESIA_VIBE_BACKEND=gl` as cache values, records the parent's `CMAKE_CXX_FLAGS`, then calls
`add_subdirectory(<core> ${CMAKE_BINARY_DIR}/core)` and adds one parent target.

### Attempt 1 — preset-equivalent toolchain args, parent flag `-DPROBE_PARENT_FLAG`

```
cmake -S build/core-subdir-probe -B build/core-subdir-probe/out -G Ninja \
  -DPROBE_CORE_DIR="$PWD/external/synesthesia-vibe" \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_TARGET_TRIPLET=arm64-osx \
  -DVCPKG_OVERLAY_TRIPLETS="$PWD/external/synesthesia-vibe/triplets" \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS=-DPROBE_PARENT_FLAG -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

Exit 1. vcpkg looks for `vcpkg.json` at the **parent's** `CMAKE_SOURCE_DIR`, finds none, installs
nothing, and the core's `find_package` fails (`SPK-002-subdirectory-configure-1.log`):

```
CMake Error at $HOME/vcpkg/scripts/buildsystems/vcpkg.cmake:939 (_find_package):
  By not providing "FindprojectM-Eval.cmake" in CMAKE_MODULE_PATH this
  project has asked CMake to find a package configuration file provided by
  "projectM-Eval", but CMake did not find one.
Call Stack (most recent call first):
  external/synesthesia-vibe/CMakeLists.txt:78 (find_package)
-- Configuring incomplete, errors occurred!
```

### Attempt 2 (the one retry) — point vcpkg at the core's manifest

Same command plus
`-DVCPKG_MANIFEST_DIR="$PWD/external/synesthesia-vibe"` and
`-DVCPKG_INSTALLED_DIR="$PWD/build/core-subdir-probe/out/vcpkg_installed"`.

vcpkg now installs the manifest (cold, from source: 92.7 s configure). Note it builds **`glfw3` and
`gtest` too**, although desktop tools are OFF: they are plain manifest dependencies, not a feature.

```
The following packages will be built and installed:
    glfw3:arm64-osx@3.5.1
    glm:arm64-osx@1.0.3
    gtest:arm64-osx@1.18.0
    projectm-eval:arm64-osx@1.0.7
```

The configure step then completes, but the parent's flags are gone and generation fails on every
`${CMAKE_SOURCE_DIR}`-based source path (`SPK-002-subdirectory-configure-2.log`):

```
-- PROBE parent CMAKE_CXX_FLAGS before add_subdirectory: '-DPROBE_PARENT_FLAG'
-- PROBE parent CMAKE_CXX_FLAGS after add_subdirectory:  ''
-- PROBE cache CMAKE_CXX_FLAGS after add_subdirectory:   ''
-- Configuring done (92.7s)

CMake Error at $HOME/vcpkg/scripts/buildsystems/vcpkg.cmake:666 (_add_library):
  Cannot find source file:
    build/core-subdir-probe/vendor/glad/src/gl.c
Call Stack (most recent call first):
  external/synesthesia-vibe/CMakeLists.txt:621 (add_library)
...
CMake Generate step failed.  Build files cannot be regenerated correctly.
```

12 errors in all. Six "Cannot find source file", each resolved under the parent's source dir
instead of the core's:

```
build/core-subdir-probe/src/audio/audiobuf.cpp
build/core-subdir-probe/src/parser/state.cpp
build/core-subdir-probe/src/platform/path_utils.cpp
build/core-subdir-probe/src/render/core/time_model.cpp
build/core-subdir-probe/vendor/glad/src/gl.c
build/core-subdir-probe/vendor/hlslparser/src/CodeWriter.cpp
```

and six "No SOURCES given to target": `beatdrop-audio`, `beatdrop-parser`,
`beatdrop-platform-portable`, `beatdrop-render-core`, `synesthesia-vibe-hlslparser`,
`synesthesia-vibe-render-gl`.

Also seen, warning only (CMake 4.4.5 accepts both `cmake_minimum_required(VERSION 3.25)` and the
policy):

```
CMake Warning (deprecated) at external/synesthesia-vibe/CMakeLists.txt:50 (cmake_policy):
  The OLD behavior for policy CMP0091 will be removed from a future version
  of CMake.
```

Stopped here per the S-014 time-box (one retry used). Making it work would need either editing
the core or a parent that is effectively a copy of the core's root — neither is allowed.

## Step 2 — separate configure + build + install: works

```
cmake -S external/synesthesia-vibe -B build/core-macos-arm64 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
  -DVCPKG_TARGET_TRIPLET=arm64-osx \
  -DVCPKG_OVERLAY_TRIPLETS="$PWD/external/synesthesia-vibe/triplets" \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0 -DCMAKE_BUILD_TYPE=Release \
  -DSYNESTHESIA_VIBE_GL_DEV=ON -DSYNESTHESIA_VIBE_BACKEND=gl -DSYNESTHESIA_VIBE_BUILD_DESKTOP_TOOLS=OFF \
  -DCMAKE_INSTALL_PREFIX="$PWD/build/core-install"
cmake --build build/core-macos-arm64 --target SynesthesiaVibeCore
cmake --install build/core-macos-arm64
```

| Step | Exit | Wall time |
|---|---|---|
| configure (vcpkg binary cache warm: `Restored 6 package(s)`) | 0 | 8.8 s |
| configure, vcpkg cold (measured in step 1 attempt 2, same packages) | — | ~93 s |
| build `SynesthesiaVibeCore` (79 Ninja steps) | 0 | 9.9 s |
| install | 0 | 0.06 s |

vcpkg's default `VCPKG_INSTALLED_DIR` is `<binary dir>/vcpkg_installed`, so with `-B` outside the
submodule nothing lands in the core tree. Build warnings: 105, none fatal (87
`-Wdeprecated-declarations`, 10 `-Wwritable-strings`, 4 `-Wunused-value`, 2 `-Wunused-comparison`,
1 `-Wnontrivial-memcall`, 1 `-Wformat-extra-args`).

Install log (`SPK-002-core-build.log`, tail):

```
-- Install configuration: "Release"
-- Installing: build/core-install/lib/libSynesthesiaVibeCore.a
-- Installing: build/core-install/include/synesthesia-vibe/synesthesia-vibe.h
-- Installing: build/core-install/include/SynesthesiaVibeCore/SynesthesiaVibeCore.h
   … 11 more Objective-C headers + module.modulemap under include/SynesthesiaVibeCore/
```

Artifacts, relative to the prefix:

```
$ ls -l build/core-install/lib
-rw-r--r--  1 …  4860120 Nov 23 01:31 libSynesthesiaVibeCore.a
$ ls build/core-install/include
SynesthesiaVibeCore  synesthesia-vibe
$ lipo -info build/core-install/lib/libSynesthesiaVibeCore.a
Non-fat file: build/core-install/lib/libSynesthesiaVibeCore.a is architecture: arm64
$ ar -t build/core-install/lib/libSynesthesiaVibeCore.a | wc -l
      86
$ nm -g --defined-only … | grep -ciE "projectm_eval|nseel"
29
$ nm -g --defined-only … | grep -c _glad_gl
2522
```

All 27 `sv_*` C API functions are defined (`sv_create`, `sv_init_gl`, `sv_render_frame`,
`sv_push_pcm_float`, `sv_validate_preset_memory`, …). projectM-Eval/ns-eel2 are merged in by the
core's POST_BUILD `libtool -static`, so the one `.a` is self-contained. No CMake package config is
installed (no `install(EXPORT)`), so the consumer declares an imported target itself.

### Link smoke test (C, outside both trees)

`build/core-link-smoke/smoke.c` includes `<synesthesia-vibe/synesthesia-vibe.h>` and calls
`sv_validate_preset_memory` on a three-line preset.

```
clang -arch arm64 -mmacosx-version-min=12.0 -Ibuild/core-install/include -c smoke.c -o smoke.o
clang++ -arch arm64 -mmacosx-version-min=12.0 smoke.o build/core-install/lib/libSynesthesiaVibeCore.a \
  -framework Foundation -framework AVFoundation -framework CoreAudio -framework OpenGL \
  -framework AppKit -framework CoreVideo -framework IOSurface -framework QuartzCore -o smoke
$ ./smoke
sv_validate_preset_memory status=0 error=''
exit=0
$ lipo -info smoke
Non-fat file: smoke is architecture: arm64
$ otool -l smoke | grep -A4 LC_BUILD_VERSION
    minos 12.0
      sdk 27.2
```

The same object also links with no frameworks at all, because this one call pulls in no GL or
Objective-C code. The framework list above is the core's own `INTERFACE` link set from
`platform/darwin/CMakeLists.txt`; the plugin should pass it until SPK-003 shows which are needed.

## Submodule stays clean

```
$ git -C external/synesthesia-vibe status --short --ignored
$ git -C external/synesthesia-vibe status
HEAD detached at v0.7.0
nothing to commit, working tree clean
```

No `build/` or `vcpkg_installed/` inside the core, ignored or not.
