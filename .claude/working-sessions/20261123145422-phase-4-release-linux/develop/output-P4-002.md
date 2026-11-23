# P4-002

## Subtasks executed
- Reusable `.github/workflows/build-plugin.yaml` (`workflow_call`, input `config`:
  `RelWithDebInfo` default or `Release`, validated in the first step) with the macOS job
  `macOS arm64`.
- Entry `.github/workflows/ci.yaml` (`CI`): `pull_request` (any base), `push` to `master`,
  `workflow_dispatch`; calls the reusable workflow with `RelWithDebInfo`. Tag pushes go only to
  `release.yaml` (P4-006).
- macOS job: pinned `macos-26` (arm64) runner, `xcode-select` to `/Applications/Xcode_26.6.app`,
  submodules checkout, vcpkg at a pinned commit, caches, configure, build, package, a failing
  inspection step, artifact upload.
- Local run of the workflow's own package and inspection scripts; actionlint.

## Files changed
- `.github/workflows/build-plugin.yaml` (new; the Linux job in it is P4-005)
- `.github/workflows/ci.yaml` (new)
- `.claude/working-sessions/20261123145422-phase-4-release-linux/develop/evidence/P4-002-workflow-step.py` (evidence helper)
- `.claude/working-sessions/20261123145422-phase-4-release-linux/develop/evidence/P4-002-local-package-inspection.md`

## What changed and why
Project-owned workflows (C-002 Option B), modelled on the template's `build-project.yaml` at
`3e7d7ac3` (job split, `check-event`-style config, pinned runner/Xcode, CPack path) but rooted at
`plugin/`, arm64 only, `v*` tags, no Windows or format jobs, no codesign secrets.

macOS job steps:
1. checkout `submodules: recursive`, `persist-credentials: false`;
2. set up: validate `config`, select Xcode, print tool versions, read `name`/`version` from
   `plugin/buildspec.json`, export `VCPKG_ROOT=$RUNNER_TEMP/vcpkg`,
   `VCPKG_DEFAULT_BINARY_CACHE=$RUNNER_TEMP/vcpkg-archives`, `ASSET_BASE=<name>-<version>`, and
   output the core gitlink SHA and the Xcode version;
3. restore `plugin/.deps` (key `obs-deps-macos-arm64-xcode<ver>-<hash of plugin/buildspec.json>`);
4. restore the vcpkg binary archives (key `vcpkg-macos-arm64-osx-<vcpkg commit>-<core gitlink>`,
   prefix restore key);
5. vcpkg: shallow fetch of `microsoft/vcpkg` at `5f96cd15…` (the core's registry baseline, so the
   tool and the baseline match), `bootstrap-vcpkg.sh -disableMetrics`;
6. `cmake --preset macos-arm64`; save `plugin/.deps` right after configure (libobs is built
   there) on a cache miss;
7. `cmake --build --preset macos-arm64 --config "$CONFIG"`; save the vcpkg archives if the restore
   missed and at least one archive exists (also after a failed build, so fix-forward runs stay warm);
8. package exactly as `docs/integrations.md` *Packaging*: `cmake --install … --prefix
   build_macos_arm64/package-staging`, `ditto -c -k --norsrc --noextattr --keepParent`, pkg renamed
   to `<name>-<version>-macos-arm64.pkg`, zip `<name>-<version>-macos-arm64.zip`;
9. inspect, each failing the job with an `::error::`: `hostArchitectures="arm64"` in the pkg's own
   Distribution (`pkgutil --expand`), `lipo -info` arm64 only, `nm -gU` diffed against
   `plugin/exported-symbols.txt` and counted (7), `CFBundleVersion == GITHUB_RUN_ID` (TC-C07),
   `codesign -dvv` shows `Signature=adhoc`, `codesign --verify --deep --strict`, pkg payload
   without `.milk`/presets/textures, zip only under the bundle and without `._*`;
10. upload artifact `<name>-<version>-macos-arm64` (pkg + zip, `if-no-files-found: error`).

Security posture: top-level `permissions: contents: read` in both files, `concurrency` with
`cancel-in-progress` in `ci.yaml`, every `uses:` pinned to a full SHA with no trailing comment, no
`secrets.*`, ad-hoc signing only (`CODESIGN_IDENT` unset).

## Verification
- `actionlint -shellcheck=$(command -v shellcheck) .github/workflows/*.yaml` (actionlint 1.7.12,
  shellcheck 0.11.0): 0 errors in 3 files.
- `cmake --build --preset macos-arm64 --config Release` then the *Package* and *Inspect package*
  scripts extracted verbatim from the YAML, run with `/bin/bash -eo pipefail`: both exit 0.
  Negative check with a wrong run id: exit 1 with `::error::CFBundleVersion 3 is not the run id 4242`.

## Evidence
- `evidence/P4-002-local-package-inspection.md`: actionlint output, package and inspection output.
- `evidence/P4-002-workflow-step.py`: extracts a step's `run:` script from a workflow (used for
  P4-002, P4-005 and P4-006 local runs).
- No CI run yet: the workflows are ready to push.

## Known limitations / follow-ups
- **Unverified until the first CI run:** Xcode 26.6 on `macos-26` vs the local Xcode 27.2
  (`-Werror` drift, C-004); cold/warm timings and cache hits (TC-C01, TC-C02); artifact sizes.
- Runner choice: `macos-26` is the GA arm64 image (Xcode 26.0.1–26.6, default 26.6). The Xcode 27
  image is a public preview, so it was not used. The runner-images policy replaces older patch
  versions, so the `Xcode_26.6.app` path can disappear in a future image; bump it then.
- The `.deps` cache holds the libobs build tree (about 1.2 GB locally); fine under the 10 GB
  repo cache limit.
- PR caches are scoped to the PR ref; a `master` push seeds caches for later PRs.
- SHA-to-version map for `docs/integrations.md` (C-019):
  - `actions/checkout` v7.0.1 → `3d3c42e5aac5ba805825da76410c181273ba90b1`
  - `actions/cache` (and `/restore`, `/save`) v6.1.0 → `55cc8345863c7cc4c66a329aec7e433d2d1c52a9`
  - `actions/upload-artifact` v7.0.2 → `cf430e030ddbb5b0abf93d22962f4752f3646cd9`
  - `actions/download-artifact` v8.0.2 → `9000827ccba6bdab643e8b6fd33ac0654aef8333` (used in P4-006)
  - vcpkg tool checkout: `microsoft/vcpkg` `5f96cd15fd745122cf27e0524606d6c1efc5fd07` (2026-09-20,
    equals the core's `vcpkg-configuration.json` baseline)
- Doc gaps (P4-012): `docs/integrations.md` *CI/CD* (workflows, triggers, runner and Xcode, caches
  and keys, inspection checks, SHA map, template reference commit); `docs/stack.md` (runner images);
  `docs/structure.md` (`.github/workflows/`); `docs/testing.md` (CI checks as the regression gate).
- The YAML files carry the standard header as `#` lines, following `plugin/CMakeLists.txt`; there
  are no comments otherwise.

## Handoff for downstream review
- Acceptance criteria addressed: reusable build + CI entry, pinned runner/Xcode, submodules,
  vcpkg pinned and cached, `.deps` cached on the buildspec hash, package per *Packaging*, failing
  inspection (arch, 7 exports, codesign, payload, Distribution), uploads, least privilege, SHA pins,
  no secrets.
- Areas to scrutinize: cache save conditions (`!cancelled()` plus archive count); the
  `CFBundleVersion == GITHUB_RUN_ID` assertion (relies on P4-001); bash 3.2 compatibility on macOS
  (`[[ … ]]` pattern, no bash 4 features).
- How to run: push the branch; watch workflow **CI**, job **Build / macOS arm64**.
