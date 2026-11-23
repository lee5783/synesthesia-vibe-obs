# P4-007

## Subtasks executed
- `plugin/README.md`: a new **Download** section after *What you need*.
- `plugin/README.md`: a new **Linux (experimental, untested in OBS)** section before *For developers*.
- Matching edits: the *Known limits* bullet, the table of contents (renumbered), the release-files
  bullet in *What you need*, and the *For developers* wording.
- Cross-check of every name, path, command and link against the workflows, the sources and CI
  run 45144284376.

## Files changed
- `plugin/README.md`
- `.claude/working-sessions/20261123145422-phase-4-release-linux/develop/evidence/P4-007-readme.md`

## What changed and why
- **Download:**
  - links the GitHub Releases page;
  - a table of the three assets (`synesthesia-vibe-obs-<version>-macos-arm64.pkg`, `.zip`,
    `SHA256SUMS.txt`);
  - pre-release only for now (`-beta.<n>`/`-rc.<n>`; the file names carry only the plugin version);
  - not notarized, linking to the existing Gatekeeper steps in *Step 1*;
  - no Linux file on releases;
  - how to check downloads with `shasum -a 256 --ignore-missing -c SHA256SUMS.txt`.
    `--ignore-missing` lets a user who downloaded only one of the two files check it; tested with
    the stock macOS `shasum`.
- **Linux, labelled "experimental, untested in OBS"** (C-006):
  - built and checked automatically, never run in OBS;
  - Ubuntu 24.04 x86_64 only;
  - OBS from `ppa:obsproject/obs-studio` at or above the build's libobs version: 32.2.0 at the time
    of writing, recorded per build in the run summary under *Linux libobs*, and not enforced by the
    package because `Depends: obs-studio` is unversioned (C-009);
  - Flatpak OBS and Ubuntu's own older `obs-studio` are not supported (C-010);
  - how to get the `.deb` from a CI run's artifacts, since it is not on releases;
  - `apt` install and uninstall, the two installed paths, and `dpkg -L`;
  - where the Linux log lives (`~/.config/obs-studio/logs/`) and the new
    `windowing platform: X11 (EGL)` / `Wayland (EGL)` line from P4-003;
  - building from source: the apt packages (the CI list plus `cmake` and `git`), vcpkg and
    `VCPKG_ROOT`, the `ubuntu-x86_64` presets with `-DCMAKE_INSTALL_PREFIX=/usr`,
    `--target package`, and the `plugin/release/` output.
- The macOS content is unchanged apart from the cross-links. Tone and layout follow the page:
  bold run-in headings, short bullets, `sh` blocks, `<version>` placeholders.

## Verification
- Every asset name, artifact name, path, package name and log line matches `release.yaml`,
  `build-plugin.yaml`, `opengl-loader-linux.cpp`, `plugin-main.cpp` and the run 5 CI log
  (`CPack: - package: …/plugin/release/synesthesia-vibe-obs-0.1.0-x86_64-linux-gnu.deb`,
  `dpkg-deb -c`, `dpkg-deb -I`, `gh api … artifacts`).
- The README's apt list compared against the CI array by script: README-only `cmake`, `git`;
  CI-only none.
- 37 anchor links resolve; 3 relative links exist; the external links return HTTP 200.
- No machine paths, emails or secrets in the added lines.

## Evidence
- `evidence/P4-007-readme.md`

## Known limitations / follow-ups
- *Build it yourself* uses the `ubuntu-x86_64` preset. CI runs `ubuntu-ci-x86_64`, which inherits
  from it, so the non-CI preset has not been run end to end.
- The Linux OBS floor (32.2.0) goes stale when the PPA moves. The README points to the per-build
  run summary. The next CI build after a PPA bump raises the floor.
- When P4-008 passes and Linux goes on releases, these must change: the *Linux* heading and status
  paragraph, *Getting the package*, the Download table and its "No Linux file" bullet, and
  `release.yaml`.
- Doc gaps (P4-012): `docs/integrations.md` *Releases* can link to this README's *Download*
  section; the root `README.md` is untouched.

## Handoff for downstream review
- Acceptance criteria addressed (TC-D01): asset names and paths match CI and the release workflow;
  the Linux status wording matches C-006; the macOS sections are unchanged apart from the
  Download cross-links.
- Areas to scrutinize: the artifact retention claim (90 days, which matches the artifact's
  `expires_at`); the floor wording; whether the Linux build-from-source block belongs on a user
  page (the coordinator asked for it here, while macOS build steps stay in `docs/integrations.md`).

## Note on the vcpkg cache in CI run 5
"Save vcpkg binary cache" being skipped on Linux was **expected**. The job restored the entry run 2
saved (`…-887628f…-45141112632-1`, 27 archives). `vcpkg install` then restored all 27 packages and
built none, so *Check vcpkg binary cache* printed `27 (restored 27)` and set `save=false`. That is
the iteration 1 rule doing its job: a warm run with nothing new adds no cache entry. The macOS job
did the same (`6 (restored 6)`).
