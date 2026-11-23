# SPK-003 evidence report

Minimal `plugin/` on the obs-plugintemplate layout, built arm64 and loaded in OBS 33.0.3 on this
Mac (macOS 26.7.2, Apple M1, Xcode 27.2, CMake 4.4.5, Ninja 1.13.2). The raw logs and screenshots
next to this file are git-ignored by the repo's evidence rule; the key lines are quoted here.

## Template pin

- `obsproject/obs-plugintemplate` at `3e7d7ac3b5342cd7d9b88890b9c70b472d1520fc` (2025-12-09,
  "Fix typo of Visual Studio in README"). Its `buildspec.json` pins obs-studio `31.1.1` and
  obs-deps / Qt6 `2025-07-11`. These were kept as shipped.
- Verbatim copies (byte-identical, `diff -r` clean): `cmake/**`, `src/plugin-support.c.in`,
  `src/plugin-support.h`.
- Edited copies: `buildspec.json` (identity values only, S-007), `CMakePresets.json` (one added
  `macos-arm64` configure preset and one build preset; the existing presets are unchanged).
- Left out: `.github/`, `build-aux/`, `.clang-format`, `.gersemirc`, `.gitignore`, `LICENSE`,
  `README.md`, `src/plugin-main.c`.

## libobs 31.1.1 with Xcode 27.2

The template's buildspec mechanism built libobs 31.1.1 (universal, Debug and Release) with no
failure, so the 33.0.3 fallback (S-001 / S-013) was not needed. First configure: 167.6 s, including
the downloads.

```
-- Configure OBS sources (universal) - done
-- Build OBS sources (Debug - universal) - done
-- Build OBS sources (Reelase - universal) - done
-- Install OBS sources (universal) - done
```

## Template first-run bug and the guard

The first configure failed after libobs was built:

```
CMake Error at cmake/macos/helpers.cmake:22 (set_target_properties):
  set_target_properties called with incorrect number of arguments.
  CMakeLists.txt:108 (set_target_properties_plugin)
```

Cause: on a non-CI machine with no `cmake/.CMakeBuildNumber`, `cmake/common/buildnumber.cmake`
leaves `PLUGIN_BUILD_NUMBER` empty and writes an empty file. The unquoted empty value then drops
an argument from `set_target_properties` in `helpers.cmake`. A second configure works because the
empty file reads as `0+1`. Instead of patching the verbatim template file, `plugin/CMakeLists.txt`
seeds the file with `0` when it doesn't exist and no build number was passed. After
`rm -rf plugin/build_macos_arm64 plugin/cmake/.CMakeBuildNumber`, a fresh configure passed on the
first try (41.8 s with `.deps` reused), the build succeeded, and `CFBundleVersion` was `1`.

## Core through ExternalProject

The Xcode build ran the ExternalProject steps `mkdir`, `download`, `update`, `patch`, `configure`,
`build` and `install` for `synesthesia-vibe-core` (inner generator Ninja, vcpkg toolchain from
`$ENV{VCPKG_ROOT}`, triplet `arm64-osx`), then linked the imported static target:

```
[79/79] Linking CXX static library src/synesthesia-vibe/libSynesthesiaVibeCore.a
-- Installing: <plugin>/build_macos_arm64/synesthesia-vibe-core/install/lib/libSynesthesiaVibeCore.a
-- Installing: <plugin>/build_macos_arm64/synesthesia-vibe-core/install/include/synesthesia-vibe/synesthesia-vibe.h
** BUILD SUCCEEDED **
```

- Full plugin build including the core: 26.9 s (warm vcpkg cache).
- 105 warnings, all inside `external/synesthesia-vibe`. None come from `plugin/src`.
- `git -C external/synesthesia-vibe status --short --ignored` was empty after both builds.

## Bundle

```
synesthesia-vibe-obs.plugin/Contents/Info.plist
synesthesia-vibe-obs.plugin/Contents/_CodeSignature/CodeResources
synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs
synesthesia-vibe-obs.plugin/Contents/Resources/locale/en-US.ini
Non-fat file: ... is architecture: arm64
Identifier=io.github.lee5783.synesthesia-vibe-obs
CodeDirectory v=20500 size=4167 flags=0x10002(adhoc,runtime) hashes=123+3 location=embedded
Signature=adhoc
TeamIdentifier=not set
```

`otool -L` lists Foundation, AVFoundation, CoreAudio, OpenGL, AppKit, CoreVideo, IOSurface,
QuartzCore, `@rpath/libobs.framework` (31.0.0 compat), libc++ and libSystem.

## Install and load

The install was a `ditto` of the bundle into `~/Library/Application Support/obs-studio/plugins/`
after quitting OBS with AppleScript. OBS exited cleanly, and the old log ends with
`Number of memory leaks: 0`. `codesign --verify` on the installed bundle reported "valid on disk"
and "satisfies its Designated Requirement". OBS was relaunched with `open -a OBS`.

```
01:42:53.563: OBS 33.0.3 (mac)
01:42:53.621: OpenGL loaded successfully, version 4.1 Metal - 90.5, shading language 4.10
01:42:54.667: Failed to load 'en-GB' text for module: 'synesthesia-vibe-obs'
01:42:54.667: [synesthesia-vibe-obs] core linked (sv_last_error(NULL) = "")
01:42:54.667: [synesthesia-vibe-obs] plugin loaded successfully (version 0.1.0)
01:42:54.667:   Loaded Modules:
01:42:54.667:     synesthesia-vibe-obs
01:43:48.279: Created scene collection 'synesthesia-spike' (clean, synesthesiaspike.json)
01:45:09.784: User added source 'SynesthesiaVibe Visualizer' (synesthesia_vibe_visualizer) to scene 'Scene'
01:45:38.093: Switched to scene collection 'Untitled' (Untitled.json)
```

The `en-GB` line is OBS trying the UI locale (en-GB) after loading the default `en-US` file. The
display name resolves from `en-US.ini`, as the screenshots show. It is not an error in the module.

## Driving OBS

The local obs-websocket credentials were rejected: close code `4009 Authentication failed`, twice
before the OBS restart and once after it, with a standard obs-websocket v5 client. The protocol
works (Hello arrives, the server is on the default port), but the stored password does not match
the server's. `GetInputKindList` / `CreateInput` were therefore not used. Instead, the OBS GUI was
driven through macOS accessibility, using System Events menu clicks and synthetic mouse clicks from
a helper in the session scratchpad (not in the repo):

1. Scene Collection → New → `synesthesia-spike`.
2. Sources "+" → the "Add Source" dialog lists "SynesthesiaVibe Visualizer" → select it.
   Screenshot: `SPK-003-add-source.png`.
3. "Add a new SynesthesiaVibe Visualizer" → the properties dialog shows "No properties available"
   → OK. Screenshots: `SPK-003-properties.png` and `SPK-003-source-in-scene.png` (the source is in
   the Sources list, with a 1920×1080 transparent bounding box).
4. Scene Collection → `Untitled`, the user's original collection, restored.

OBS was left running on the user's collection, with the plugin installed (S-012 removes it at
SPK-008). The `synesthesia-spike` collection is left for the user to delete.
