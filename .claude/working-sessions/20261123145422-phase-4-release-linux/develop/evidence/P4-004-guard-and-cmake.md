# P4-004 evidence: Linux CMake port, macOS regression side

The Linux branch of `plugin/CMakeLists.txt` cannot run on this Mac (no Docker, no Linux host). Its
first real configure, compile and link is the CI `Linux x86_64` job (P4-005). This file covers the
macOS side.

## (a) Fresh configure and build on macOS

Same run as P4-003: `rm -rf build_macos_arm64 && cmake --preset macos-arm64 && cmake --build
--preset macos-arm64` exits 0 (`** BUILD SUCCEEDED **`), and the Release config builds too.

## (b) Exports

`nm -gU` lists the same 7 `obs_module_*` names as before (`P4-003-macos-export-import-diff.md`);
`otool -L` and every undefined symbol are unchanged.

## (c) Guard still refuses everything but the two allowed builds

`cmake_host_system_information(... QUERY OS_PLATFORM)` supplies the host processor before
`project()`, because `CMAKE_HOST_SYSTEM_PROCESSOR` is still empty there (checked with a scratch
project: `name=[Darwin] proc=[]` before `project()`; `OS_PLATFORM` gives `arm64`).

```
$ cat cmake/.CMakeBuildNumber
3
$ cmake --preset macos
CMake Error at CMakeLists.txt:24 (message):
  SynesthesiaVibe for OBS builds only on macOS, with the Xcode generator, for
  arm64 alone, or on Linux, with the Ninja generator, for x86_64.  Detected
  host 'Darwin' (arm64), generator 'Xcode', CMAKE_OSX_ARCHITECTURES
  'arm64;x86_64'.  On macOS configure with 'cmake --preset macos-arm64' and
  build with 'cmake --build --preset macos-arm64'.  On Linux configure with
  'cmake --preset ubuntu-x86_64' and build with 'cmake --build --preset
  ubuntu-x86_64'.
exit=1
$ cmake -S . -B build_ninja_scratch -G Ninja
CMake Error at CMakeLists.txt:24 (message):
  SynesthesiaVibe for OBS builds only on macOS, with the Xcode generator, for
  arm64 alone, or on Linux, with the Ninja generator, for x86_64.  Detected
  host 'Darwin' (arm64), generator 'Ninja', CMAKE_OSX_ARCHITECTURES ''.  On
  macOS configure with 'cmake --preset macos-arm64' and build with 'cmake
  --build --preset macos-arm64'.  On Linux configure with 'cmake --preset
  ubuntu-x86_64' and build with 'cmake --build --preset ubuntu-x86_64'.
$ cat cmake/.CMakeBuildNumber
3
$ rm -rf build_macos build_ninja_scratch
```

The counter is unchanged: the guard still runs before the seed and the template bootstrap.

## (d) Distribution override still applies

```
$ grep hostArchitectures build_macos_arm64/distribution
        hostArchitectures="arm64"
```

## Template and core untouched

```
$ git diff feature/plugin-phase-2-macos --stat -- plugin/cmake
(empty)
$ git diff feature/plugin-phase-2-macos -- external/synesthesia-vibe
(empty)
$ git submodule status
 887628f08dd35c1a3e984750323987f82a9ce3c6 external/synesthesia-vibe (v0.5.0-19-g887628f)
```
