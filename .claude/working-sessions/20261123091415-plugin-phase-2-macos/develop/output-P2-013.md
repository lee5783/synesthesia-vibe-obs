# P2-013

## Subtasks executed

- Project-owned copy of the template's `distribution.in` with `hostArchitectures="arm64"`,
  configured from `plugin/CMakeLists.txt` over the template's generated `distribution` (C-016).
  The vendored template file is untouched.
- Release build (`--config Release`): arm64, ad-hoc signed, with a `.dSYM`.
- The template's install step into a staging prefix under the build dir: `pkgbuild` +
  `productbuild` produced an unsigned `.pkg` (C-002).
- `ditto` zip of the staged `.plugin`, without extended attributes.
- Inspection of all three artifacts (Distribution XML, PackageInfo, payload, bundle id and
  version, `lipo`, `codesign`, `nm -gU`, `spctl`). Nothing was installed and OBS was not touched.
- Developer and notarization commands recorded below for the doc pass (C-017). Notarization was
  not run.

## Files changed

- `plugin/packaging/macos/distribution.in` (new, project-owned; standard header as an XML comment
  after the XML declaration)
- `plugin/CMakeLists.txt` (one `configure_file` line after `set_target_properties_plugin`)
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/input-dev.md` (appended
  the P2-013 section)
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/output-P2-013.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-013/*`

## What changed and why

`cmake/macos/helpers.cmake` (vendored) configures `cmake/macos/resources/distribution.in` into
`${CMAKE_CURRENT_BINARY_DIR}/distribution`, and the generated `create-package.cmake` hands that
path to `productbuild --distribution` at install time. `plugin/CMakeLists.txt` now configures
`packaging/macos/distribution.in` to the same output path right after
`set_target_properties_plugin`, so the later write wins at configure time. The copy differs from
the template only in `hostArchitectures` (`arm64,x86_64` → `arm64`), so Installer refuses the
pkg on Intel Macs, where OBS can't load the arm64-only bundle. Diff:
`evidence/P2-013/P2-013-distribution-diff.txt`.

Location: `plugin/cmake/**` is template-only and `plugin/data/**` is globbed into the bundle by
`target_install_resources`, so neither can hold the file. `plugin/packaging/macos/` keeps the
template's per-OS segment (`cmake/macos/`). `docs/structure.md` needs the new folder (doc gap
below).

No presets or textures are in any artifact (C-003): the bundle holds `Contents/Info.plist`,
`Contents/MacOS/synesthesia-vibe-obs`, `Contents/_CodeSignature/CodeResources` and
`Contents/Resources/locale/en-US.ini`.

## Verification

From `plugin/`:

- `cmake --preset macos-arm64`: configure OK. `build_macos_arm64/distribution` has
  `hostArchitectures="arm64"`.
- `cmake --build --preset macos-arm64 --config Release`: `** BUILD SUCCEEDED **`, 0 warnings
  (`evidence/P2-013/P2-013-build-release.log`).
- `cmake --install build_macos_arm64 --config Release --prefix build_macos_arm64/package-staging`:
  exit 0, `productbuild: Wrote product …/synesthesia-vibe-obs.pkg`
  (`evidence/P2-013/P2-013-install-release.log`).
- `cmake --build --preset macos-arm64` (default RelWithDebInfo): `** BUILD SUCCEEDED **`, so the
  configure change doesn't break the dev build other tasks use.

### Artifacts (git-ignored, `plugin/build_macos_arm64/package-staging/`)

| Artifact | Size | SHA-256 |
|---|---|---|
| `synesthesia-vibe-obs.pkg` | 785,522 B | `1dbf3ab24811451344351a0d31c9ffb274510f1c6fd6c15c6a5a2da2df187889` |
| `synesthesia-vibe-obs-0.1.0-macos-arm64.zip` | 496,298 B | `10344f23c9ca252ef4aa2f78a960c4968c2437db3aa253a28795ab863f9a88d3` |
| `synesthesia-vibe-obs.plugin` (staged) | 1,956 KiB on disk (binary 1,990,528 B) | — |
| `synesthesia-vibe-obs.plugin.dsym` (staged) | 1,380 KiB on disk | — |

### Inspection results (full output: `evidence/P2-013/P2-013-package-inspection.md`)

- **Distribution XML** (`pkgutil --expand`): `hostArchitectures="arm64"`,
  `<domains enable_currentUserHome="true" enable_anywhere="false" enable_localSystem="false"/>`,
  min macOS `12.0`. `installer -dominfo`: `CurrentUserHomeDirectory` only.
- **Payload** (`pkgutil --payload-files`): `Library/Application Support/obs-studio/plugins/`
  with `synesthesia-vibe-obs.plugin` and `synesthesia-vibe-obs.plugin.dSYM` only. No `presets`,
  `textures` or `.milk` entries. The listing also shows `._*` AppleDouble entries; they carry the
  `com.apple.provenance` xattr (see limitations). `pkgutil --expand-full` extracts the payload the
  way Installer does: the sidecars merge back into xattrs, no `._` files land on disk, and the
  extracted `.plugin` passes `codesign --verify --deep --strict`.
- **PackageInfo:** bundle id `io.github.lee5783.synesthesia-vibe-obs`,
  `CFBundleShortVersionString 0.1.0`, `CFBundleVersion 4`, `relocatable="false"`, empty
  `<relocate/>`, install location in the home domain. The identifier and version carry literal
  single quotes (template bug, see limitations).
- **pkg signature:** `pkgutil --check-signature`: `Status: no signature` (expected, C-002).
- **Bundle:** `CFBundleIdentifier io.github.lee5783.synesthesia-vibe-obs`, version `0.1.0`
  (build `4`). `lipo -info`: `Non-fat file … architecture: arm64`.
- **codesign -dvv:** `Signature=adhoc`, `flags=0x10002(adhoc,runtime)`, `TeamIdentifier=not set`,
  `Format=bundle with Mach-O thin (arm64)`, no entitlements.
  `codesign --verify --deep --strict --verbose=2`: `valid on disk`, `satisfies its Designated
  Requirement` on the staged bundle, the extracted pkg payload and the unzipped zip.
- **Exports:** `nm -gU` lists exactly 7 symbols, identical to `plugin/exported-symbols.txt`.
- **Zip:** 10 entries, the bundle only, no `._` entries (`--norsrc --noextattr`).
- **Gatekeeper (expected while unnotarized):**
  - `spctl --assess -vv --type install synesthesia-vibe-obs.pkg`: `rejected`,
    `source=no usable signature` (exit 3).
  - `spctl --assess -vv --type execute synesthesia-vibe-obs.plugin`: `rejected` (exit 3).
  - `spctl --assess -vv --type open --context context:primary-signature …zip`: `rejected`,
    `source=no usable signature` (exit 3).

## Developer commands (for `docs/integrations.md`, C-017)

Run from `plugin/`. Always pass `--prefix`: `cmake/macos/defaults.cmake` caches
`CMAKE_INSTALL_PREFIX` as the real `~/Library/Application Support/obs-studio/plugins`. Don't name
the staging dir `release`: on a case-insensitive volume it's the same folder as Xcode's
`build_macos_arm64/Release/`.

```sh
cmake --preset macos-arm64
cmake --build --preset macos-arm64 --config Release
rm -rf build_macos_arm64/package-staging
cmake --install build_macos_arm64 --config Release --prefix build_macos_arm64/package-staging
cd build_macos_arm64/package-staging
ditto -c -k --norsrc --noextattr --keepParent synesthesia-vibe-obs.plugin synesthesia-vibe-obs-0.1.0-macos-arm64.zip
pkgutil --payload-files synesthesia-vibe-obs.pkg
codesign -dvv synesthesia-vibe-obs.plugin
codesign --verify --deep --strict --verbose=2 synesthesia-vibe-obs.plugin
nm -gU synesthesia-vibe-obs.plugin/Contents/MacOS/synesthesia-vibe-obs
spctl --assess -vv --type install synesthesia-vibe-obs.pkg
```

User install from the pkg (needs approval; not run here):
`installer -pkg synesthesia-vibe-obs.pkg -target CurrentUserHomeDirectory`.

### Notarization (documented only, run once Developer ID credentials exist — C-002)

Credentials stay outside the repo. The presets read `CODESIGN_IDENT` / `CODESIGN_TEAM` from the
environment; `xcode.cmake` then signs with that identity, hardened runtime and `--timestamp` for
Release. All `<…>` are placeholders.

```sh
xcrun notarytool store-credentials "<NOTARY_PROFILE>" --apple-id "<APPLE_ID>" --team-id "<TEAM_ID>"

export CODESIGN_IDENT="Developer ID Application: <NAME> (<TEAM_ID>)"
export CODESIGN_TEAM="<TEAM_ID>"
cmake --preset macos-arm64 --fresh
cmake --build --preset macos-arm64 --config Release
rm -rf build_macos_arm64/package-staging
cmake --install build_macos_arm64 --config Release --prefix build_macos_arm64/package-staging
cd build_macos_arm64/package-staging

productsign --timestamp --sign "Developer ID Installer: <NAME> (<TEAM_ID>)" \
  synesthesia-vibe-obs.pkg synesthesia-vibe-obs-signed.pkg
xcrun notarytool submit synesthesia-vibe-obs-signed.pkg --keychain-profile "<NOTARY_PROFILE>" --wait
xcrun stapler staple synesthesia-vibe-obs-signed.pkg
spctl --assess -vv --type install synesthesia-vibe-obs-signed.pkg

ditto -c -k --norsrc --noextattr --keepParent synesthesia-vibe-obs.plugin notarize-upload.zip
xcrun notarytool submit notarize-upload.zip --keychain-profile "<NOTARY_PROFILE>" --wait
xcrun stapler staple synesthesia-vibe-obs.plugin
ditto -c -k --norsrc --noextattr --keepParent synesthesia-vibe-obs.plugin synesthesia-vibe-obs-0.1.0-macos-arm64.zip
```

Not verified: whether `stapler` staples a `.plugin` bundle (it does for apps, pkgs and dmgs). If
it refuses, ship the zip unstapled; Gatekeeper then checks the ticket online.

## Evidence

All under `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-013/`:

- `P2-013-package-inspection.md`: every inspection command and its output (committed; paths
  sanitized).
- `P2-013-distribution-diff.txt`: vendored vs project-owned distribution file (header plus the
  one arch line).
- `P2-013-build-release.log`: the Release build (git-ignored).
- `P2-013-install-release.log`: the `cmake --install` run with `pkgbuild` / `productbuild`
  (git-ignored).
- No screenshot and no OBS log. The task's last acceptance item (install with
  `installer -target CurrentUserHomeDirectory`, see the source in OBS, `renderer check passed`)
  was excluded by the delegation: OBS is busy with another task and the home-domain install is
  not allowed. It's still open for the orchestrator to run with the user's approval.

## Known limitations / follow-ups

- **Template bug, pkg identifier and version carry literal quotes:** the vendored
  `create-package.cmake.in` passes `--identifier '@MACOS_BUNDLEID@'` and
  `--version '@CMAKE_PROJECT_VERSION@'` through `execute_process`, which doesn't use a shell. The
  receipt id is `'io.github.lee5783.synesthesia-vibe-obs'` and the version `'0.1.0'`, quotes
  included. Upgrades still match because every build is the same. It does affect
  `pkgutil --pkgs` / `pkgutil --forget` (the user must type the quotes). The same override
  pattern would fix it: configure a project-owned `create-package.cmake` to the same
  `${CMAKE_CURRENT_BINARY_DIR}` path. Not done because it's outside P2-013's scope; user decides.
- **The template ships the `.dSYM` into the user's plugins folder** (Release only). OBS loads
  only `*.plugin`, so it should be ignored, but users get about 1.4 MB of debug symbols. A future
  override could drop it from the payload and keep it as a separate artifact.
- **`._*` entries in the pkg payload listing:** this machine stamps `com.apple.provenance` on
  every file a build writes, and it can't be removed (`xattr -cr` and `ditto --noextattr` both
  leave it). `pkgbuild` stores it as AppleDouble sidecars. On extraction they merge back into
  xattrs, so nothing extra lands on disk and the signature verifies. A clean CI machine wouldn't
  have them. `COPYFILE_DISABLE=1` doesn't help.
- The Installer title and the bundle's `CFBundleDisplayName` are the project name
  (`synesthesia-vibe-obs`), not "SynesthesiaVibe Visualizer" (template behaviour).
  `CFBundleVersion` is the local build counter (`4` here), which goes up on every configure.
- The ad-hoc signature has the hardened-runtime flag (`runtime`), from `xcode.cmake` for non-Debug
  configs. OBS loads it because `OBS.app` has `disable-library-validation`, same as the
  RelWithDebInfo dev builds.
- Universal binary deferred to Phase 4 (C-016). Moving to universal means reverting the
  `hostArchitectures` change in the project-owned copy.
- **Doc gaps for document-writer (P2-016):**
  - `docs/integrations.md`: the packaging, zip, inspection and notarization commands above. The
    note "Install with `ditto`, not `cmake --install` … drops a `.pkg` into the plugins folder"
    should say why: the cached default prefix is the real plugins folder, so always pass
    `--prefix`.
  - `docs/structure.md`: add `plugin/packaging/macos/distribution.in` as an authored file and
    `build_macos_arm64/package-staging/` as generated.
  - `.claude/common-issues/plugin-build.md`: (1) a staging dir named `release` inside the build
    dir is Xcode's `Release/` on case-insensitive APFS, so `rm -rf` on it deletes the Release
    build (it happened here; rebuilt). (2) The quoted pkg identifier. (3) The `._` provenance
    sidecars in `--payload-files`.
- No test added (none requested). Packaging has no automated check; the inspection commands above
  are the regression check.

## Handoff for downstream review

- Acceptance criteria addressed:
  - `.pkg` and `.zip` produced.
  - The payload holds only the `.plugin` plus the Release `.dSYM` (the `._` listing entries are
    xattr sidecars, explained above).
  - `codesign -dvv` reports ad-hoc, and `codesign --verify --deep --strict` passes.
  - `spctl -a -vv -t install` recorded: rejected, as expected.
  - The pkg declares arm64 only and the home domain only (C-016).
  - No presets or textures (C-003).
  - Notarization documented, not run (C-002).
  - **Open:** the approved `installer` install plus OBS check (excluded by delegation).
- Areas to scrutinize:
  - The override relies on `configure_file` order: the template writes `distribution` inside
    `set_target_properties_plugin`, and our line rewrites it afterwards. If a future template
    moves its `configure_file` to generate time, or renames the output, the override silently
    stops working. Check with `grep hostArchitectures plugin/build_macos_arm64/distribution`
    after configure.
  - The XML-comment header sits between the XML declaration and the root element. `productbuild`
    accepts it and keeps it in the pkg's Distribution file.
- How to run: see *Developer commands*. Don't run `cmake --install` without `--prefix`.
