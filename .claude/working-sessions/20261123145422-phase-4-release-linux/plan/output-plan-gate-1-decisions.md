# Gate 1 decisions (user, 2026-11-23)

The orchestrator asked these in-thread. These are the user's answers.

| Concern | Answer |
|---|---|
| C-001 pushing to verify | **Branch, draft PR and test tag.** Allowed: push `feature/phase-4-release-linux`, including fix-forward pushes. The orchestrator opens a **draft PR stacked on `feature/plugin-phase-2-macos`** so CI runs. Also allowed: **one throwaway prerelease tag** (e.g. `v0.1.0-ci.1`) to test the release job. Delete its draft release and the tag afterwards. Commits on this branch are allowed for these pushes. |
| C-006 Linux shipping | **CI artifact only.** Build and check Linux in CI. The `.deb` is a workflow artifact and is **not attached to Releases** until the user's Ubuntu X11/Wayland test (P4-008) passes. The README calls Linux experimental and untested in OBS. Stop rule C-013 applies: if the core needs a source change, stop and draft an upstream ask. |
| C-016 release gating | **(b) Prereleases only.** Every release is a draft. `-beta.N`/`-rc.N` tags make prereleases. A plain `vX.Y.Z` is refused until a repo variable is set. The user's Publish click is the final gate. Signing is ad-hoc only, with no secrets. |
| C-008 `.deb` maintainer | **GitHub no-reply:** `lee5783 <lee5783@users.noreply.github.com>`, set as `CPACK_PACKAGE_CONTACT` in `plugin/CMakeLists.txt`. |
| C-024 OBS for the macOS regression check | **Not now; deferred.** The macOS evidence for the loader split is the build plus an export/import diff. The TC-M03 OBS smoke load is deferred along with P2-011. Do not drive OBS in this run. |
| C-020 / C-021 items 3 and 4 | **Drop the forum task (P4-009) and close the mapper task (P4-010)** as already satisfied. The docs gate covers the Phase 4 changes. |
| Everything else (C-002–C-005, C-007, C-009–C-015, C-017–C-019, C-022, C-023, C-025–C-029) | **Accept all recommended defaults** in `output-plan-concerns.md`. |

The defaults accepted in the last row:

- **CI:**
  - project-owned workflows (B);
  - triggers: PRs, pushes to `master`, `v*` tags and manual dispatch;
  - pinned runner image and Xcode;
  - vcpkg cloned at a pinned commit, with `actions/cache`.
- **Versioning:** the first tag is `v0.1.0-beta.1`, and the suffix lives only in the tag.
- **Pinning:** actions are pinned by SHA with no trailing comments, and the SHA-to-version map lives in `docs/integrations.md`.
- **Sources:** per-OS GL loader files.
- **Linux scope:**
  - the `.deb` only;
  - Ubuntu 24.04 x86_64 only;
  - Flatpak unsupported;
  - the PPA's libobs version is the Linux minimum OBS.
- **macOS:** arm64 only.
- **GPL source:** the release notes link to the core tag.
- **Docs:**
  - `PLAN.md`: the status line and the Phase 4 item states;
  - `CLAUDE.md` *Setup*: a Linux build line only if CI passes;
  - the core's `-O0` finding goes in `docs/concerns.md`, with an upstream ask drafted and no workaround.
