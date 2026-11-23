# P4-001 evidence: CI build number and `plugin/release/` ignore

All runs on this Mac (Apple Silicon, Xcode 27.2, CMake 4.4.5), from `plugin/`. Paths shortened to
`<repo>`.

## (a) Fresh first configure passes without a build-number file

The old tree was moved aside (not deleted) so libobs in `plugin/.deps/` stayed intact.

```
$ mv build_macos_arm64 build_macos_arm64-pre-p4 && rm -f cmake/.CMakeBuildNumber
$ cmake --preset macos-arm64
...
-- Configuring done (34.9s)
-- Generating done (0.0s)
-- Build files have been written to: <repo>/plugin/build_macos_arm64
exit=0
$ cat cmake/.CMakeBuildNumber
1
$ grep -m1 CURRENT_PROJECT_VERSION build_macos_arm64/synesthesia-vibe-obs.xcodeproj/project.pbxproj
                CURRENT_PROJECT_VERSION = 1;
```

The local seed (`0`, then the template's `+1`) still applies when `CI` is unset.

## (b) With `CI=true GITHUB_RUN_ID=4242` the template's run-id branch sets the number

```
$ rm -f cmake/.CMakeBuildNumber
$ CI=true GITHUB_RUN_ID=4242 cmake -S . -B build_ci_scratch -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=12.0
-- Configuring done (33.3s)
exit=0
$ cat cmake/.CMakeBuildNumber
4242
$ grep -m1 CURRENT_PROJECT_VERSION build_ci_scratch/synesthesia-vibe-obs.xcodeproj/project.pbxproj
                CURRENT_PROJECT_VERSION = 4242;
$ CI=true GITHUB_RUN_ID=4242 cmake --build build_ci_scratch --config RelWithDebInfo
** BUILD SUCCEEDED **
$ /usr/libexec/PlistBuddy -c 'Print :CFBundleVersion' build_ci_scratch/RelWithDebInfo/synesthesia-vibe-obs.plugin/Contents/Info.plist
4242
$ /usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' .../Info.plist
0.1.0
```

Afterwards: `rm -rf build_ci_scratch`, the `4242` counter file was removed, the local counter was
restored and `cmake --preset macos-arm64` reconfigured normally (exit 0).

The macOS CI job now asserts the same thing on every run: its *Inspect package* step fails unless
`CFBundleVersion` equals `GITHUB_RUN_ID` (TC-C07).

## (c) `plugin/release/` is ignored

```
$ mkdir -p plugin/release && touch plugin/release/probe.deb
$ git status --short --ignored plugin/release
!! plugin/release/
$ git check-ignore -v plugin/release/probe.deb
.gitignore:14:plugin/release/	plugin/release/probe.deb
$ rm -rf plugin/release
```

## Template untouched

`git diff feature/plugin-phase-2-macos --stat -- plugin/cmake` prints nothing.
