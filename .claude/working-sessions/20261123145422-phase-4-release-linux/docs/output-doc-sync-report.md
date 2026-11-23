# Documentation sync report

## Summary

Phase 4 shipped four things:
- project-owned GitHub Actions CI (macOS arm64 and Linux x86_64 on every PR);
- a tag-driven release pipeline that makes only draft releases, and only prereleases;
- an experimental Linux build: per-OS GL loader, version-script exports, `LINK_GROUP:RESCAN` core
  link, CPack `.deb` as a CI artifact only;
- the CI build-number fix.

The docs now describe all four, and the review's *For P4-012* list is covered item by item.

- **Target:** `docs/` is the default.
- **Outside `docs/`:** the writes below were all approved by the orchestrator's packet (Gate 1
  decision C-022 and the accepted defaults):
  - `PLAN.md`: status line, Phase 4 item states, one risk row, and the asks pointer sentence;
  - `CLAUDE.md` *Setup*: one Linux line;
  - `.claude/` caches.
- **Not touched:** the root `README.md`, `plugin/README.md`, and all source and workflow files.

## Files updated

### `docs/integrations.md`

- **New source:** the OBS project PPA, with the 32.2.0 floor and the refusal of the distro OBS.
- **Build downloads:** split into macOS and Linux; the Linux vcpkg plan; the pinned vcpkg commit
  `5f96cd15…` and why it matches the core baseline.
- **Linux paths and logs:** the `.deb` install paths; the Linux-only `windowing platform: …` log
  line.
- **Caching:** build vs runtime.
- **Auth:** `github.token` only, no `secrets.*`.
- **CI/CD, which used to say "None":**
  - a workflow table;
  - the macOS and Linux jobs;
  - *Set up environment* validation;
  - the build number on CI;
  - the security posture;
  - timings.
- **New *Pinned actions* SHA-to-version map** (checkout v7.0.1, cache v6.1.0, upload-artifact
  v7.0.2, download-artifact v8.0.2, vcpkg `5f96cd15…`) and the bump procedure.
- **New *Caches*:** the `.deps` and vcpkg keys, restore prefixes, the save-only-if-grown rule, ref
  scoping, and the leftover tag caches.
- **Rewrote *Releases*:**
  - jobs and tag rules, including `RELEASE_FINAL_ALLOWED`;
  - the core version check;
  - `SHA256SUMS.txt`, always a draft, no `.deb`, and how to attach it later;
  - notes content, including the core tag link;
  - re-run and cleanup.
- **Guard section:** the stale message replaced with the current two-combination text, plus the
  host-vs-target caveat.
- **New *Linux build and package* section:**
  - prerequisites, with the apt groups explained;
  - commands and outputs;
  - what the CMake Linux branch does;
  - package `Depends`/`NEEDED`;
  - *The PPA and its 503s*.
- **Packaging:** `CFBundleVersion` on CI; release asset names; *Inspection* no longer says "no
  automated one".
- **Signing:** C-018 future CI notarization secrets as **placeholder names only**.
- **Environment:** `CI`; the repository variable `RELEASE_FINAL_ALLOWED`; the secrets line.

### `docs/stack.md`

- Linux in the overview.
- New languages: YAML, the version script, the new C++ files.
- Linux runtime (OBS 32.2.0+, EGL).
- libobs from the PPA on Linux.
- Toolchain: Xcode 26.6 in CI, GCC 13, runner images `macos-26` / `ubuntu-24.04`, SHA pins, vcpkg
  commit, `gh`.
- Linux packaging tools; core archives on Linux; GLVND libraries.
- Presets passing the guard; both export files; `.github/workflows`.
- Linux dev and production requirements.

### `docs/structure.md`

- Tree: `exported-symbols.map`, `build_x86_64/`, `plugin/release/`, `.github/workflows/`.
- Authored-file list, with the `.map` header form and the three loader files.
- New sections: *`plugin/src/` per-OS files* and *`.github/workflows/`*.
- Configuration and core-logic entries.
- Both export files for a new entry point; where platform code goes.
- Build-change note on `if(OS_MACOS) … else()`; new *CI changes*.
- Special directories: `plugin/build_x86_64/`, `plugin/release/`, and the seed skipped on CI.

### `docs/architecture.md`

- Plugin adapter contents and dependencies now include the loader files and the Linux targets.
- The module-load step for `opengl_log_platform`.
- *Core GL loader* rewritten per OS: header `#if`, macOS dlsym, Linux EGL → `libOpenGL.so.0` →
  `libGL.so.1`, the GLVND stub note, and the call path.
- *Export lists* now cover both files and the Linux link options.
- New *Core link on Linux*: why `LINK_GROUP:RESCAN` is needed (the core ↔ ns-eel2 cycle, the
  `NSEEL_HOSTSTUB_*` stubs, GNU ld's single pass vs ld64), why not reorder or list twice, and PIC.
- Linux plugin scan path.
- All `visualizer-source.cpp` line citations shifted for the loader split (−3 between old lines
  69 and 218, −9 from old line 224 on). `plugin-main.cpp:18-47` → `19-49`. Spot-checked against
  the current source.

### `docs/conventions.md`

- Naming: `.yaml`; the per-OS file suffixes.
- Style: YAML; CMake `if(OS_MACOS)` blocks.
- **Import example fixed:** `visualizer-source.cpp` no longer lists `<OpenGL/gl3.h>`/`<dlfcn.h>`;
  platform GL headers come only through `opengl-loader.hpp`.
- Header forms: YAML `#`; `.map` uses one `/* */` block laid out like `distribution.in`.
- SHA-only `uses:` with no trailing comment (C-019); heredoc `##` lines are not comments.
- *Binary exports*: the two files kept in sync (leading `_` vs none, same order, CI diff).
- New *Platform code* rule.
- New *Workflows* section: `::error::` plus exit 1, values through `env:`, permissions, bounded
  retries, actionlint.

### `docs/testing.md`

- Run Commands now point at CI.
- New *CI checks (automated evidence)* section:
  - the macOS inspection list;
  - the Linux origin, exports, TEXTREL, ldd, dlopen and `.deb` checks;
  - the `-Werror` builds;
  - the release validation;
  - the local step-extraction helper and actionlint;
  - what CI does not cover.
- Build checks note that Linux-only code is first compiled in CI.
- Integration tests list CI.
- The user checklist adds the Linux X11/Wayland run (P4-008).

### `docs/concerns.md`

- Intro updated.
- New *Fixed in Phase 4*: seed debt resolved, CI added, Linux export control.
- macOS-only tech debt rewritten as macOS arm64 + Linux x86_64, with the **universal binary
  follow-up (C-025)** and the Windows revisit.
- New *Small Linux and build follow-ups*: review **m4–m7**.
- Seed debt entry reduced to "resolved", plus the cache residual.
- New *Most of the core builds without optimisation (C-029)*, re-confirmed: 75 compile units,
  10 with `-O`; the core `CMakeLists.txt` `FORCE`-clears the flags.
- Fragile area: both export files, and the CI count of 7.
- New Dependencies-at-Risk entries:
  - *The core on Linux*;
  - *Linux OBS floor and the PPA*: 32.2.0, unversioned `Depends`, PPA moves, 503s, versioned
    `Depends` as a possible fix;
  - *GitHub runner images and pins*: the Xcode path, no Dependabot, `CORE_VERSION`.
- New Security entry: *CI and release pipeline*.
- Notarization now mentions the CI placeholders.
- New *Linux in OBS (P4-008)*: Flatpak unsupported, other distros, a GLX revisit.
- Universal-binary line corrected.
- New upstream ask draft **#7**, optimisation flags per target, **not filed**.
- Test gaps: Linux in OBS, the macOS loader split OBS smoke (TC-M03), the plain `ubuntu-x86_64`
  preset.

### `.claude/` and root files

- **`.claude/common-issues/plugin-build.md`:**
  - analysis-date scope;
  - the *Known trade-off* line replaced by the CI seed-skip note;
  - the guard section retitled, with the current message and the Linux refusals;
  - new entries: **Linux GNU ld archive-order link error** and **PPA 503 / distro-OBS fallback**.
- **`.claude/how-x-works/release-pipeline.md` (new):** tag → validate → build → draft, with
  `file:line` citations into `release.yaml` / `build-plugin.yaml`, the manual steps before and
  after, and how to change the pipeline.
- **`.claude/how-x-works/plugin-render-frame.md`:** the `opengl_get_proc` bullet now points at
  the per-OS loader files; line citations shifted.
- **`.claude/how-x-works/plugin-audio-path.md`, `plugin-preset-loading.md`:** line citations
  shifted only. The `audio-ring.cpp` citations were left alone.
- **`PLAN.md`:**
  - status line (Phase 4 built and reviewed, prerelease-only, Windows waits, Linux
    experimental);
  - Phase 4 item states: 1 done, prerelease-only, Windows blocked; 2 built and checked in CI,
    experimental, pending P4-008; 3 dropped for this run; 4 closed, already satisfied;
  - the risk row *Duplicate glad symbols on Linux*, now mitigated;
  - the *Upstream asks* pointer sentence now names #7 as drafted in `docs/concerns.md`.
- **`CLAUDE.md` *Setup*:** one Linux build line (experimental, built in CI, untested in OBS).

## Index updates

- `.claude/HOW-X-WORKS.md`: a row for `how-x-works/release-pipeline.md`.
- `.claude/COMMON-ISSUES.md`: the `plugin-build.md` row now lists CI, the seed skip, the GNU ld
  archive-order error and the PPA 503 trap. Same area file, so no new row.
- `docs/README.md`: N/A (no pages added or removed).
- `.claude/DICTIONARY.md`: N/A (no new jargon needed an entry).

## Follow-ups / gaps

- **`docs/README.md` is left alone** per the packet ("only if pages are added or removed"). Its
  intro ("Phase 2 macOS …") and the *Covers* text for `integrations.md` / `testing.md` /
  `concerns.md` no longer mention CI, releases or Linux. Suggest a one-line refresh on approval.
- **`CLAUDE.md` *Quick facts*:** "No test suite yet. Plugin evidence is a manual OBS check with
  screenshots" is left alone. It is still true for runtime behaviour, and CI covers only build and
  package checks, so it is not wrong.
- **`docs/decisions.md`:** no change. No `PLAN.md` decision was locked in Phase 4; Gate 1 settled
  concerns, not D-numbered decisions.
- **Review m3, moving the README's Linux *Build it yourself* block:** `docs/integrations.md` now
  has the full developer steps. The README keeps its own block because `plugin/README.md` is
  off-limits, so the apt list now lives in three places (the workflow, the README and
  integrations). `docs/structure.md` notes the README block.
- **Not verified here:** the plain `ubuntu-x86_64` preset (CI runs `ubuntu-ci-x86_64`), and
  anything Linux inside OBS (P4-008). The docs say so.
- **Upstream ask #7 is drafted only.** The user files it.
- **Tag-scoped caches** from the deleted test tag (review I1) are documented as expiring, with the
  delete command. Whether to delete them now is the orchestrator's call.
