# P4-007 evidence: `plugin/README.md` cross-check

Every name, path and command the new *Download* and *Linux* sections use, checked against the
workflows, the sources and CI run 45144284376 (commit ec49eeb, both jobs green).

## Asset and release names (`release.yaml`)

| README | Source | Match |
|---|---|---|
| `synesthesia-vibe-obs-<version>-macos-arm64.pkg` / `.zip` | `asset_base="synesthesia-vibe-obs-${VERSION}"` + `-macos-arm64.pkg` / `.zip` in *Write checksums* and *Create draft release* | yes |
| `SHA256SUMS.txt` holds the checksums of the two files | `sha256sum <pkg> <zip> > SHA256SUMS.txt` | yes |
| releases are Pre-release, tags `-beta.<n>` / `-rc.<n>` | tag regex `(-(beta\|rc)\.[0-9]+)?`, plain `vX.Y.Z` refused without `RELEASE_FINAL_ALLOWED`, `--prerelease` for suffixed tags | yes |
| name *SynesthesiaVibe Visualizer 0.1.0-beta.1*; file names carry only the plugin version | `--title "SynesthesiaVibe Visualizer ${RELEASE_VERSION}"` (tag without `v`); asset names use the buildspec `X.Y.Z` | yes |
| no Linux file on releases | the `release` job downloads only `…-macos-arm64` | yes |

`shasum -a 256 --ignore-missing -c SHA256SUMS.txt` with the stock macOS `shasum` (macOS 26.7) on the
locally built assets: both files `OK`; with the zip removed, only the pkg is checked, exit 0.

## Linux package (`build-plugin.yaml` and the CI log)

- Artifact name: the README says `synesthesia-vibe-obs-<version>-x86_64-linux-gnu`.
  `gh api …/actions/runs/45144284376/artifacts` returns
  `synesthesia-vibe-obs-0.1.0-x86_64-linux-gnu 447203 expires 2027-02-21` (90 days after the run,
  as the README says) and `synesthesia-vibe-obs-0.1.0-macos-arm64 972453`.
- `.deb` name: CI log line
  `CPack: - package: …/plugin/release/synesthesia-vibe-obs-0.1.0-x86_64-linux-gnu.deb generated.`,
  plus `…-dbgsym.ddeb generated.` (the README mentions both, and the `plugin/release/` folder).
- Installed paths, from `dpkg-deb -c` in the CI log:
  ```
  -rw-r--r-- root/root   1470288 … ./usr/lib/x86_64-linux-gnu/obs-plugins/synesthesia-vibe-obs.so
  -rw-r--r-- root/root      1090 … ./usr/share/obs/obs-plugins/synesthesia-vibe-obs/locale/en-US.ini
  ```
  Same two paths in the README.
- Package name for `dpkg -L` / `apt remove`: `Package: synesthesia-vibe-obs` (`dpkg-deb -I`).
- `Depends: libc6 (>= 2.38), libegl1, libgcc-s1 (>= 3.0), libopengl0, libstdc++6 (>= 13.1), obs-studio`.
  `obs-studio` has no version, which is why the README says the package does not enforce the OBS
  minimum.
- OBS floor: CI log `obs-studio package: obs-studio 32.2.0-0obsproject1~noble`,
  `libobs API version: 32.2.0`. The README says 32.2.0 at the time of writing and points to the run
  summary heading *Linux libobs* (`echo "### Linux libobs"` in *Record libobs version*).
- Ubuntu's own `obs-studio` is older: CI run 3 fell back to `obs-studio 30.0.2+dfsg-3build1` from
  noble/universe.
- Log lines: `opengl-loader-linux.cpp` logs `"windowing platform: %s"` with `"X11 (EGL)"` or
  `"Wayland (EGL)"`; `plugin-main.cpp` logs `renderer check passed (%s, device type %d)` and
  `plugin loaded successfully (version %s)`.

## Build commands

- The apt list in *Build it yourself* equals the CI `packages=(…)` array plus `cmake` and `git`,
  which the runner image already has (scripted comparison: README-only `['cmake', 'git']`,
  CI-only `[]`).
- `cmake --preset ubuntu-x86_64 -DCMAKE_INSTALL_PREFIX=/usr` and `cmake --build --preset
  ubuntu-x86_64`: the presets exist in `plugin/CMakePresets.json` with `binaryDir`
  `${sourceDir}/build_x86_64`, the Ninja generator and the Linux host condition. CI uses
  `ubuntu-ci-x86_64`, which inherits from `ubuntu-x86_64` and adds `-Werror` and ccache.
- `cmake --build build_x86_64 --target package` is the CI *Package* step verbatim.
- The `VCPKG_ROOT` requirement matches the `FATAL_ERROR` in `plugin/CMakeLists.txt`.

## Links

- 37 in-page anchor links; every one resolves to a heading (GitHub slug rules, scripted check).
- Relative links: `../LICENSE`, `../browser-source/README.md`, `../docs/integrations.md`; all exist.
- External links, HTTP status: `https://github.com/lee5783/synesthesia-vibe-obs/releases` 200,
  `…/actions` 200, `https://github.com/microsoft/vcpkg.git` 200. The repo is public; the workflows
  are named `CI` and `Release` (`gh api …/actions/workflows`).

## Not verified

- The non-CI `ubuntu-x86_64` preset has not been run end to end; CI only runs `ubuntu-ci-x86_64`.
- The Linux install and the log lines in OBS: that is P4-008, the user's Ubuntu test.
- No machine paths, emails or secrets in the diff (`grep -E '/Users/|/home/|@'` on the added lines
  finds nothing).
