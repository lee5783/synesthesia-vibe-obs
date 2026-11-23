# Implementation plan summary

## Goal
Add GitHub Actions CI to SynesthesiaVibe for OBS. On every PR it builds, packages and checks the
macOS arm64 plugin. It also builds the plugin for Linux x86_64. A `v*` tag produces a **draft**
GitHub Release with the macOS pkg and zip and `SHA256SUMS.txt`, and the Linux `.deb` once the user
allows it. The plugin gets a Linux port: per-OS GL loading, the CMake OS split, export control
through a version script, and a CPack `.deb`. In CI it is built, link-checked and packaged. A real
OBS run on Ubuntu X11 and Wayland can only be done by the user (P4-008), and Linux is not called
shippable until that passes. Phase 4 items 3 (forum page) and 4 (`codebase-mapper`) are proposed
as dropped for now and already done, respectively.

## Scope
**In scope:**
- `.github/workflows/` (reusable build, CI entry, release);
- CI hygiene in `plugin/CMakeLists.txt` (the build-number seed) and `.gitignore`;
- the Linux CMake branch and the build guard relaxed to exactly macOS arm64 Xcode or Linux x86_64
  Ninja;
- `plugin/exported-symbols.map`;
- per-OS GL include and loader files in `plugin/src/`;
- `plugin/README.md` *Download* and Linux sections;
- review and doc sync.

**Out of scope:**
- Windows (Phase 3, blocked);
- a universal macOS binary;
- notarization and Developer ID signing (no credentials);
- editing the core submodule or the vendored `plugin/cmake/**`;
- the root `README.md`;
- finishing the deferred Phase 2 checks (P2-010b, P2-011, P2-012);
- Flatpak, aarch64, distros other than Ubuntu 24.04;
- posting on the OBS forum;
- publishing a release (always a human click on a draft).

## Key decisions
These are proposed. The user decides at Gate 1 (`output-plan-concerns.md`).
- **Project-owned workflows (C-002, Option B).** The template's `.github/` at `3e7d7ac3` is the
  reference. It can't be used as-is: it assumes the repo root, a universal build, bare-semver tags
  and a Windows job.
- **Ad-hoc signing only in CI, no secrets (C-018).** Every action is pinned to a full SHA (C-019).
  Build jobs get least privilege; only the release job may write.
- **Releases are always drafts (C-016).** While Phase 2 is open, only `-beta.N`/`-rc.N` tags (which
  become prereleases) are allowed.
- **Versioning (C-017).** The version comes only from `plugin/buildspec.json` (`X.Y.Z`). The tag
  must be `v<version>[-beta.N|-rc.N]`. Asset names are
  `synesthesia-vibe-obs-<ver>-{macos-arm64.pkg,macos-arm64.zip,x86_64-linux-gnu.deb}`.
- **Linux (C-006, C-007).** Built and checked in CI. The `.deb` is a workflow artifact only, not a
  release asset, until the user's Ubuntu X11 and Wayland test passes.
- **Linux GL (C-011).** `eglGetProcAddress` with a `dlsym` fallback to GLVND. Exports are limited
  to the 7 `obs_module_*` names through `--version-script` plus `--exclude-libs,ALL`. This retires
  the PLAN.md "duplicate glad symbols" risk.

## Risks
- **The core is not a Linux release target upstream (C-013).** It may not compile with GCC or
  Clang. Its objects are built without `-fPIC`, and on Linux its projectM archives are not merged
  into the core library (C-015). Stop rule: if the core needs a source change, Linux stops and an
  upstream ask is drafted.
- **No local Linux and no local Actions runner.** Docker and `act` are missing. Every Linux and CI
  claim needs pushes to GitHub, which needs the user's permission (C-001).
- **The Linux minimum OBS is the PPA's libobs version on the build day, not OBS 31 (C-009).**
  Flatpak OBS can't load the `.deb` (C-010).
- **Xcode drift on CI runners can trip `-Werror` (C-004).** The first cold CI runs are slow
  (libobs, vcpkg). Caching mitigates it (C-005).
- **The branch is stacked on the unmerged PR #2 (C-023).**
- **Side finding (C-029):** about 65 of 75 core compile units build at `-O0`. This may bear on the
  deferred 1080p60 check. It is not fixed here.

## Impact
- **New:**
  - `.github/workflows/build-plugin.yaml`, `ci.yaml`, `release.yaml`;
  - `plugin/exported-symbols.map`;
  - per-OS GL loader sources in `plugin/src/` (names per C-012).
- **Changed:**
  - `plugin/CMakeLists.txt`: guard, OS branches, Linux core link, export flags, CPack contact,
    CI seed;
  - `plugin/src/visualizer-source.cpp`: the GL include and loader move out; behaviour unchanged;
  - `.gitignore`;
  - `plugin/README.md`.
- **Unchanged:** `plugin/cmake/**`, `external/synesthesia-vibe/`, `plugin/buildspec.json`
  (unless the version is bumped for the first tag), and the plugin's runtime behaviour on macOS.
- **Docs (P4-012):** integrations, stack, structure, concerns, testing, `docs/README.md`; and
  `PLAN.md` / `CLAUDE.md` only with approval (C-022).
- **UX:** users get downloads from GitHub Releases; Linux users get an experimental `.deb` later.

## References
- Task breakdown: [output-plan-tasks.md](output-plan-tasks.md)
- Test scenarios (high level): [output-plan-test-cases.md](output-plan-test-cases.md)
- Open questions and edge cases: [output-plan-concerns.md](output-plan-concerns.md)
- Analyzer inputs: none this run (no intake, ui-ux or impact step; see `session.log`). Requirements:
  `../inputs/requirements.md`.
