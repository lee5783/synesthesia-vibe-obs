## Summary

Phase 4 of `PLAN.md` adds:
- **CI.** GitHub Actions builds, packages and checks the plugin on every PR for macOS arm64 and Linux x86_64.
- **A release workflow.** A `v*` tag produces a **draft prerelease** with the macOS `.pkg`, the `.zip` and `SHA256SUMS.txt`.
- **An experimental Linux port.** CI builds it, runs link checks on it and packages it as a `.deb`. Nobody has tested it in OBS yet.

This PR is stacked on #2 (Phase 2, macOS). Retarget it to `master` once #2 merges.

The Phase 4 items stand as follows:
- **Item 1, CI plus release:** done. Releases are prerelease-only while Phase 2 is open, and Windows stays blocked by Phase 3.
- **Item 2, Linux:** built and checked in CI. The `.deb` stays a workflow artifact until a real Ubuntu X11/Wayland test passes.
- **Item 3, forum page:** dropped for now.
- **Item 4, `codebase-mapper`:** closed, because the `docs/` corpus already exists.

**Ticket:** none
**Design:** none

## Changes

**CI and release**
- **`.github/workflows/build-plugin.yaml` (new, reusable).** It has two jobs:
  - **macOS arm64:** `macos-26`, Xcode 26.6.
  - **Linux x86_64:** `ubuntu-24.04`, with OBS from `ppa:obsproject/obs-studio`.

  Shared behaviour:
  - vcpkg is pinned at `5f96cd15` and uses binary caches.
  - Every inspection check fails the job:
    - macOS: arm64-only pkg and binary, exactly 7 `obs_module_*` exports, ad-hoc signature, no presets in the payload, `CFBundleVersion` equal to the run id.
    - Linux: 7 exports and no other symbols, no TEXTREL, no unresolved `ldd`, a `dlopen` probe with `RTLD_NOW`, and the `.deb` Maintainer, Depends and contents.
  - The buildspec `name` and `version` are validated before use.
  - apt fetches retry, and the job refuses to build against Ubuntu's own OBS.
- **`.github/workflows/ci.yaml` (new).** Runs on PRs, pushes to `master` and manual dispatch, with `contents: read`.
- **`.github/workflows/release.yaml` (new).** Runs on `v*` tags:
  - It checks the tag against the buildspec version and the core tag against the submodule.
  - `-beta.N` and `-rc.N` tags become prereleases. A plain `vX.Y.Z` is refused unless the repo variable `RELEASE_FINAL_ALLOWED` is `true`.
  - It writes `SHA256SUMS.txt` and runs `gh release create --draft --verify-tag`.
  - The release notes link this tag and core `v0.7.0` for the GPL source.
  - Only this job has `contents: write`. The Linux `.deb` is not attached.
  - Every action is pinned to a full SHA.

**Plugin build**
- **`plugin/CMakeLists.txt`.** Five changes:
  - **Guard:** it now allows exactly macOS + Xcode + arm64, or Linux + Ninja + x86_64.
  - **Apple settings:** they sit under `OS_MACOS`.
  - **Linux core build:** `x64-linux`, PIC, and the projectM-Eval and ns-eel2 archives linked through `$<LINK_GROUP:RESCAN,…>`. GNU ld reads archives in one pass, and the core and ns-eel2 need each other.
  - **Linux link flags:** `--version-script`, `--exclude-libs,ALL`, `--no-undefined`, and `CPACK_PACKAGE_CONTACT` set to the GitHub no-reply address.
  - **Build number:** on CI it comes from `GITHUB_RUN_ID`.
- **`plugin/exported-symbols.map` (new).** The Linux export list. It holds the same 7 names as `exported-symbols.txt`.
- **`.gitignore`.** Ignores `plugin/release/`, the CPack output.

**Plugin sources**
- **`plugin/src/opengl-loader.hpp`, `opengl-loader-macos.cpp`, `opengl-loader-linux.cpp` (new).** These hold the per-OS GL include and the function loader:
  - macOS: the loader moved here unchanged.
  - Linux: `eglGetProcAddress`, falling back to GLVND `libOpenGL.so.0` and then `libGL.so.1`. It logs a `windowing platform:` line.
- **`plugin/src/visualizer-source.cpp`, `plugin-main.cpp`.** They use the loader header. The only other change is the platform log call.

**User docs**
- **`plugin/README.md`.** It gains two sections:
  - **Download:** the release assets, and checking them with `shasum -a 256 --ignore-missing -c`.
  - **Linux (experimental, untested in OBS):**
    - Ubuntu 24.04 x86_64 only, OBS 32.2.0 or newer from the PPA, Flatpak not supported;
    - the `.deb` comes from CI artifacts;
    - install, uninstall and logs;
    - how to build it yourself.

## Verification

- [x] **Builds clean, macOS, locally.** `cmake --preset macos-arm64 && cmake --build --preset macos-arm64`. `nm -gU` still lists exactly the 7 exports, and the `otool -L` imports and undefined symbols match the pre-Phase-4 build.
- [x] **Builds clean, CI.** On draft PR #3 both jobs are green: run 45144284376 at ec49eeb, and run 45150243042 at f57bf4e, the review fixes. Runs 1–4 failed and were fixed forward:
  - missing autotools for vcpkg on Linux;
  - the GNU ld archive order;
  - two Launchpad PPA 503 outages, now retried, with the job refusing the distro OBS.
- [x] **Release workflow run once.** Run 45146667649 used a throwaway tag `v0.1.0-beta.0`:
  - all four jobs were green;
  - the draft prerelease held the pkg (768102 B), the zip (487636 B) and `SHA256SUMS.txt`, with no `.deb`;
  - `shasum -a 256 -c` passed on the downloaded files.

  Afterwards I deleted the draft release and the tag.
- [x] **`actionlint`.** 1.7.12 with shellcheck reports 0 errors on all three workflows.
- [x] **No unit tests, and none were requested.**

**Evidence** is in `develop/evidence/`. It is text only, because CI logs and job summaries hold the raw output.

| What it shows | Capture |
|---|---|
| The build number from `GITHUB_RUN_ID`, and the first configure without a seed | `P4-001-build-number.md` |
| The macOS package inspection, locally and in CI, plus timings | `P4-002-local-package-inspection.md` |
| macOS exports and imports unchanged after the loader split | `P4-003-macos-export-import-diff.md` |
| The guard's allowed and refused combinations, and the CMake OS split | `P4-004-guard-and-cmake.md` |
| Linux checks from the green run 5: exports, `NEEDED`, `dlopen`, `.deb` fields | `P4-005-linux-check-dry-run.md` |
| The tag-validation matrix and the result of the release run | `P4-006-release-logic.md` |
| README names, paths and links cross-checked against the CI output | `P4-007-readme.md` |

Excerpts from CI run 5, Linux:

```
obs-studio package: obs-studio 32.2.0-0obsproject1~noble
libobs API version: 32.2.0
Exports: obs_module_free_locale obs_module_get_string obs_module_load obs_module_set_locale
         obs_module_set_pointer obs_module_unload obs_module_ver
NEEDED: libobs.so.30 libOpenGL.so.0 libEGL.so.1 libstdc++.so.6 libm.so.6 libgcc_s.so.1 libc.so.6
dlopen probe: RTLD_NOW | RTLD_LOCAL load succeeded and obs_module_load resolved
```

**CI timings:**

| Job | Cold | Warm |
|---|---|---|
| macOS | 5m39s | 1m59s |
| Linux | 11m38s | about 14.5 min (mostly PPA retries) |

**Manual QA** comes from `plan/output-plan-test-cases.md`.

| Case | Precondition | Expected | State |
|---|---|---|---|
| TC-C01–C07 | CI on a PR | Green builds, cache restores, all checks fail loudly, no secrets used, build number equal to the run id | Done (runs 1–5 and 45150243042) |
| TC-R01, TC-R05 | A prerelease tag | A draft prerelease with the pkg, zip and checksums; `shasum` passes | Done (run 45146667649) |
| TC-R02–R04 | Wrong or plain tags | Rejected before any build | Dry run of the validation logic (`P4-006`) |
| TC-M01, TC-M02 | This Mac | Fresh configure passes; the guard refuses wrong generators | Done |
| TC-M03 | OBS usable with obs-websocket | The plugin still loads in OBS after the loader split | **Deferred** (C-024, with P2-011) |
| TC-L01–L08 | An Ubuntu 24.04 machine, X11 and Wayland | Install, render, audio, settings, teardown, uninstall; Flatpak refused | **User's task P4-008, not run.** Linux stays off releases until it passes |

## Review

- [x] **Reviewed by the `code-reviewer` pass.** Decision: **Approve**, nothing blocking (`review/output-review-findings.md`). It covered:
  - the SHA pins, checked against the official tags;
  - least-privilege permissions;
  - no `${{ }}` inside `run:`;
  - fork PRs can't write;
  - PR caches can't reach a release run (shown by the release run logging `Cache not found`);
  - `LINK_GROUP:RESCAN`;
  - the export map and the loader;
  - no changes to `plugin/cmake` or the core.
- [x] **Findings fixed in f57bf4e:**
  - m1, the checksum command in the release notes;
  - m2, CI results added to the evidence files;
  - m3, the README vcpkg pin and guard wording;
  - S1, buildspec validation before `GITHUB_ENV`.

  m4–m7 are optional and recorded as small debt in `docs/concerns.md`.
- **Areas to scrutinize:**
  - the Linux link group and export map in `plugin/CMakeLists.txt`;
  - the release tag gating in `release.yaml`;
  - the apt retry and PPA-origin checks in the Linux job.

## Documentation

- [x] **`docs/**` updated:**
  - `integrations.md`: CI/CD, pinned actions, caches, releases, the Linux build and the PPA;
  - `stack.md`, `structure.md`, `architecture.md` (the per-OS loader and the Linux core link), `conventions.md`, `testing.md`;
  - `concerns.md`: the Linux and CI caveats, m4–m7, the universal binary, and the core `-O0` finding with upstream ask #7 drafted, not filed;
  - the `README.md` index.
- [x] **Gotchas and flows recorded:**
  - `.claude/common-issues/plugin-build.md`: the GNU ld archive order, the PPA 503 and distro-OBS fallback, and the guard message;
  - new `.claude/how-x-works/release-pipeline.md`;
  - line citations in the other how-x-works pages updated for the loader split.
- [x] **Root files:**
  - `PLAN.md`: status line, Phase 4 item states, the Linux glad risk now mitigated, a pointer to ask #7.
  - `CLAUDE.md` *Setup*: one Linux build line.
  - Root `README.md`: not touched.

## Risks and follow-ups

- **Linux is untested in OBS.** P4-008 is your Ubuntu 24.04 X11 and Wayland test. When it passes, attach the `.deb` to releases (a one-line switch in `release.yaml`).
- **Linux OBS minimum.** It is whatever the PPA ships on the build day, 32.2.0 now. The `.deb`'s `Depends: obs-studio` has no version, so apt won't enforce it. Flatpak OBS is not supported.
- **The Launchpad PPA has intermittent 503s.** Retries cover short outages. A long outage fails the Linux job clearly; re-run it.
- **Releases are prerelease-only and not notarized** while Phase 2's Done-when checks (P2-010b, P2-011, P2-012) are open and there is no Developer ID. The macOS loader-split smoke test in OBS (TC-M03) waits with P2-011.
- **Most of the core builds at `-O0`** (C-029): 65 of 75 compile units have no `-O` flag. This may matter for the 1080p60 check P2-012. Upstream ask #7 is drafted in `docs/concerns.md`, not filed.
- **Not in this PR:** Windows (Phase 3, blocked) and a universal macOS binary (C-025).
- **Cache cleanup.** The throwaway tag left about 455 MiB of tag-scoped Actions caches. They expire after 7 days unused.

## Checklist

- [x] No secrets, tokens or credential files in the diff. There are no `secrets.*` references. The only email is the agreed `lee5783@users.noreply.github.com`.
- [x] Session artifacts under `.claude/working-sessions/20261123145422-phase-4-release-linux/` are committed by design; bulk evidence is git-ignored.
- [x] New files carry the standard header: `//` in C++, `#` in YAML, `/* */` in the `.map`.
- [x] No unrequested code comments added. Action pins have no trailing `# vX` comments; the SHA-to-version map is in `docs/integrations.md`.
- [x] Style matches the sibling files.
- [x] `plugin/cmake/**` (template) and `external/synesthesia-vibe` (core) unchanged.

---
🤖 Built with an AI vibe-flow run — session `20261123145422-phase-4-release-linux`

🤖 Generated with [Claude Code](https://claude.com/claude-code)
