# P4-002 evidence: workflows lint clean; macOS package and inspection steps run locally

CI itself has not run yet (nothing pushed; the orchestrator pushes). This file shows the parts that
can be checked on this Mac.

## actionlint

```
$ actionlint -version
1.7.12
$ shellcheck --version | sed -n 2p
version: 0.11.0
$ actionlint -verbose -shellcheck="$(command -v shellcheck)" .github/workflows/*.yaml
verbose: Found 0 parse errors in 1 ms for .github/workflows/build-plugin.yaml
verbose: Found 0 parse errors in 1 ms for .github/workflows/release.yaml
verbose: Found 0 parse errors in 1 ms for .github/workflows/ci.yaml
verbose: Rule "pyflakes" was disabled: exec: "pyflakes": executable file not found in $PATH
verbose: Found total 0 errors in 2 ms for .github/workflows/ci.yaml
verbose: Found total 0 errors in 213 ms for .github/workflows/release.yaml
verbose: Found total 0 errors in 221 ms for .github/workflows/build-plugin.yaml
verbose: Found 0 errors in 3 files
exit=0
```

shellcheck ran over every `run:` block (pyflakes is only for `python` shells, none here).

## The workflow's own *Package* and *Inspect package* scripts, run locally

The scripts were pulled out of `build-plugin.yaml` verbatim with `P4-002-workflow-step.py` (in this
folder) and run with `/bin/bash -eo pipefail` (bash 3.2, the bash on macOS runners), after
`cmake --build --preset macos-arm64 --config Release` (`** BUILD SUCCEEDED **`, 0 plugin
warnings). Environment: `CONFIG=Release`, `ASSET_BASE=synesthesia-vibe-obs-0.1.0`,
`GITHUB_RUN_ID` set to the local `CFBundleVersion` (3), since the local counter is not a run id.

```
$ python3 P4-002-workflow-step.py .github/workflows/build-plugin.yaml macos "Package" > package.sh
$ (cd plugin && bash -eo pipefail package.sh)
productbuild: Wrote product to build_macos_arm64/package-staging/synesthesia-vibe-obs.pkg
-rw-r--r--@ 1 …  786389 Nov 23 15:27 synesthesia-vibe-obs-0.1.0-macos-arm64.pkg
-rw-r--r--@ 1 …  496426 Nov 23 15:27 synesthesia-vibe-obs-0.1.0-macos-arm64.zip
exit=0
```

*Inspect package* (exit 0), trimmed:

```
::group::Distribution
    <options rootVolumeOnly="true" hostArchitectures="arm64" customize="never" allow-external-scripts="no"/>
::endgroup::
::group::Architecture
Non-fat file: synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs is architecture: arm64
::endgroup::
::group::Exports
_obs_module_free_locale
_obs_module_get_string
_obs_module_load
_obs_module_set_locale
_obs_module_set_pointer
_obs_module_unload
_obs_module_ver
::endgroup::
::group::Bundle version
CFBundleVersion 3
::endgroup::
::group::Code signature
Executable=<repo>/plugin/build_macos_arm64/package-staging/synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs
Identifier=io.github.lee5783.synesthesia-vibe-obs
Format=bundle with Mach-O thin (arm64)
CodeDirectory v=20500 size=4103 flags=0x10002(adhoc,runtime) hashes=121+3 location=embedded
Signature=adhoc
Info.plist entries=21
TeamIdentifier=not set
Runtime Version=27.2.0
Sealed Resources version=2 rules=13 files=1
Internal requirements count=0 size=12
synesthesia-vibe-obs.plugin: valid on disk
synesthesia-vibe-obs.plugin: satisfies its Designated Requirement
::endgroup::
::group::Payload
… (pkgutil --payload-files: 47 lines, the .plugin and the Release .plugin.dSYM under
   ./Library/Application Support/obs-studio/plugins/, plus local `._*` provenance sidecars)
… (unzip -Z1 of the zip:)
synesthesia-vibe-obs.plugin/
synesthesia-vibe-obs.plugin/Contents/
synesthesia-vibe-obs.plugin/Contents/_CodeSignature/
synesthesia-vibe-obs.plugin/Contents/_CodeSignature/CodeResources
synesthesia-vibe-obs.plugin/Contents/MacOS/
synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs
synesthesia-vibe-obs.plugin/Contents/Resources/
synesthesia-vibe-obs.plugin/Contents/Resources/locale/
synesthesia-vibe-obs.plugin/Contents/Resources/locale/en-US.ini
synesthesia-vibe-obs.plugin/Contents/Info.plist
::endgroup::
```

- Distribution inside the pkg: `hostArchitectures="arm64"`.
- `lipo -info`: arm64 only.
- `nm -gU`: exactly the 7 names of `plugin/exported-symbols.txt` (the step diffs and counts).
- `CFBundleVersion` equals the given run id.
- `codesign -dvv`: `Signature=adhoc`, `flags=0x10002(adhoc,runtime)`; `--verify --deep --strict`: valid.
- pkg payload: no `.milk`, presets or textures. It lists `._*` AppleDouble entries on this machine
  (`com.apple.provenance`, `.claude/common-issues/plugin-build.md`); the check does not fail on
  them in the pkg. A clean runner should not produce them.
- zip: 10 entries, all under `synesthesia-vibe-obs.plugin/`, no `._*`.

### Negative check: the step fails loudly

```
$ GITHUB_RUN_ID=4242 bash -eo pipefail inspect.sh
::group::Bundle version
::error::CFBundleVersion 3 is not the run id 4242
exit=1
```

## Still unverified (needs the first CI run)

- The real `macos-26` runner with Xcode 26.6 (`-Werror` drift, C-004).
- Cold and warm wall times; cache hits for `plugin/.deps` and the vcpkg binary archives (TC-C02).
- Artifact names and sizes as uploaded.

## CI result

Since this file was written the macOS job has run on GitHub. It was green on every run, 1 to 5
and the release run, with all inspection checks passing. Run 5 showed
`CFBundleVersion 45144284376` (= the run id, TC-C07). Wall time: cold 5m39s (run 1, libobs built
into `.deps` and vcpkg cold), warm 1m59s (run 5: `.deps` and 6/6 vcpkg archives restored). Xcode
26.6 on `macos-26` built the plugin cleanly under `-Werror`, with no source change needed (C-004).
