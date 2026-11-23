# Review findings — P4-011 (Gate 2)

Scope: `git diff feature/plugin-phase-2-macos...feature/phase-4-release-linux` (draft PR #3, HEAD
`dca171a`), reviewed against `plan/output-plan-*.md`, `plan/output-plan-gate-1-decisions.md`, the
developer handoffs P4-001 to P4-007 and their evidence, and the CI history.

## Decision

**Approve.** There are no blocking findings. A few small follow-ups are listed under *Minor
issues*. The orchestrator can fold them into the P4-012 doc pass or a last commit before the PR
leaves draft. None of them needs another review round.

## Tooling

- There is no linter or formatter in this repo (`CLAUDE.md`). `actionlint` with shellcheck was run
  by the developer (0 errors, evidence P4-002). I did not re-run it.
- CI, checked read-only with `gh`:
  - Run 5 (`45144284376`, `ec49eeb`): both jobs green.
  - HEAD run `45147255843` (`dca171a`, the README commit): finished green on both jobs during this
    review.
  - Release test run `45146667649` (tag `v0.1.0-beta.0`): all four jobs green.
    - The release job logged `…macos-arm64.pkg: OK` and `…zip: OK`, then created a draft at an
      `untagged-…` URL.
    - `gh release list` is now empty, and `git ls-remote --tags origin` lists no tags. The cleanup
      held.
- Every `uses:` SHA was checked against the official tag through `gh api`, and all match:
  - `actions/checkout` v7.0.1
  - `actions/cache` v6.1.0
  - `actions/upload-artifact` v7.0.2
  - `actions/download-artifact` v8.0.2
- CodeGraph was used for the GL loader blast radius:
  - `opengl_get_proc` → `sv_init_gl` → `render_gl::LoadGlFunctions` → `gladLoadGLUserPtr`.
  - `opengl_log_platform` has one caller, `obs_module_load`.
- `git diff feature/plugin-phase-2-macos -- plugin/cmake external` is **empty** (0 bytes).

## Code quality / common issues

**Conventions** (confirmed):
- No added comments. The only comment blocks are the standard headers. The `## …` lines in
  `release.yaml` are Markdown inside the notes heredoc, not YAML comments.
- **Headers on YAML and `.map` (confirming the developer's choice):**
  - The `#` header in the three workflow files matches the CMake, `.txt` and `.ini` siblings.
  - The `/* … */` block in `plugin/exported-symbols.map` follows the indented layout of the XML
    header in `plugin/packaging/macos/distribution.in`. It is a comment form GNU ld's version-script
    parser accepts, and the CI link proves it.
  - Both choices are correct. `docs/conventions.md` should record them (P4-012).
- New file names are kebab-case.
- `void f(void)` declarations match `plugin-main.cpp` and `visualizer-source.hpp`.
- Include order matches each file's existing pattern.

**`plugin/CMakeLists.txt`:**
- The guard (`plugin/CMakeLists.txt:12-34`) allows exactly two combinations:
  - Darwin + `Xcode` + `CMAKE_OSX_ARCHITECTURES` `arm64`;
  - Linux + `Ninja` (exact match, so Ninja Multi-Config is refused) + host `x86_64`, from
    `cmake_host_system_information(OS_PLATFORM)`, because `CMAKE_HOST_SYSTEM_PROCESSOR` is still
    empty before `project()`.
  - Evidence P4-004 (c) shows `macos` and `-G Ninja` refused, with the counter unchanged.
- The Apple-only settings sit under `if(OS_MACOS)`:
  - triplet, overlay triplets and `CMAKE_OSX_*`;
  - the framework list;
  - `GL_SILENCE_DEPRECATION`;
  - `XCODE_ATTRIBUTE_EXPORTED_SYMBOLS_FILE`;
  - the macOS loader source;
  - the `distribution.in` `configure_file`.
- Linux core PIC: `-DCMAKE_POSITION_INDEPENDENT_CODE=ON` goes to the ExternalProject. CI proves it
  took effect:
  - the `.so` links on x86_64;
  - *Check plugin binary* has a `TEXTREL` gate and passed.
- `LINK_GROUP:RESCAN`:
  - The fix is correct for the core ↔ ns-eel2 cycle (`NSEEL_HOSTSTUB_*` is defined in the core and
    used by ns-eel2).
  - The CMake manual confirms the developer's claim: "any occurrence of the non-group link item
    will be replaced by the groups it belongs to". So the shared
    `target_link_libraries(… SynesthesiaVibe::Core)` is safe.
  - The link is green with `-Wl,--no-undefined`.
- Export map vs list: `exported-symbols.map` and `exported-symbols.txt` list the same 7 names
  (scripted diff: match).
- The CI exports check:
  - `nm -D --defined-only` shows exactly the 7 `obs_module_*` names;
  - no `glad`, `sv_`, projectM or `_Z` names;
  - checked on the built `.so` and again on the packaged one.
  - Together with `--exclude-libs,ALL` and `local: *;`, this retires the PLAN.md duplicate-glad
    risk.
- `CPACK_PACKAGE_CONTACT` is set on Linux before `include(defaults)`. The CI `.deb` shows
  `Maintainer: lee5783 <lee5783@users.noreply.github.com>`, the agreed address (C-008).
- CI build number:
  - `NOT "$ENV{CI}"` uses the same truthiness test as the template.
  - Run 5 shows `CFBundleVersion 45144284376`, which equals the run id (TC-C07).
  - The local first-configure seed still works (evidence P4-001 a).

**Per-OS GL loader:**
- macOS behaviour is unchanged:
  - `opengl_get_proc` was moved byte for byte;
  - `opengl_log_platform` is empty, so the macOS log is unchanged;
  - `otool -L`, the GL imports and all 319 undefined symbols are identical before and after
    (evidence P4-003);
  - CI builds macOS cleanly under `-Werror` with Xcode 26.6.
- Linux uses `eglGetProcAddress`, then falls back to `dlopen`/`dlsym` on `libOpenGL.so.0` and then
  `libGL.so.1`.
  - The handles are created once, in a thread-safe function-local static, and never closed. That
    is lifetime-safe: they outlive every lookup, as on macOS.
  - The core loads through glad 2 (`gladLoadGLUserPtr`), which picks functions from the version and
    extension strings. So GLVND's habit of returning stubs from `eglGetProcAddress` does no harm.
- Platform log: `obs_get_nix_platform()` is valid at module load. The line is logged after a
  passing renderer check, which is the right place.
- Header design: `plugin/src/opengl-loader.hpp` holds the only `#if` (C-012). This is acceptable;
  see Minor m5.

**Hygiene:**
- No machine paths, personal emails or secrets were found in the added lines or the session
  artifacts. The only email is the agreed no-reply address.
- Evidence paths are written as `<repo>`.
- The README uses `~/Downloads` and `~/vcpkg`, which are generic.

## Architecture

- **Separation of concerns:**
  - The reusable `build-plugin.yaml` is called by `ci.yaml` (RelWithDebInfo) and `release.yaml`
    (Release).
  - Validation runs before the build (`needs: validate`).
  - The single write-scoped job only downloads artifacts, writes checksums and creates the draft.
  - The OS split stays inside `plugin/CMakeLists.txt`, and the template is untouched.
- **Robustness:** the Linux job refuses to build against Ubuntu's own OBS, at two points:
  - before install, the candidate version must contain `obsproject`;
  - after install, the installed version must contain `obsproject`, and the header is found
    through dpkg.

  Without this, a PPA 503 would quietly move the OBS floor (iteration 3). That was a good catch.
- **Risks (accepted by decision, not defects):**
  - Linux has not been run in OBS (P4-008).
  - The Linux floor is the PPA version on the build day (32.2.0, C-009).
  - TC-M03 is deferred (C-024).
- **Maintainability:**
  - Four separate `if(OS_MACOS) … else()` blocks follow the file's top-to-bottom flow.
  - `else()` means Linux only because the guard forbids everything else. Phase 3 (Windows) must
    revisit both the guard and these branches; the guard makes that impossible to miss.

## Security

The workflows were reviewed for TC-C05.

| Check | Result |
|---|---|
| Every `uses:` pinned to a 40-char SHA | Yes; 4 actions, all verified against the official tags. Local reusable calls use `./`. |
| Least privilege | Top-level `contents: read` in all three files; only `release.yaml` job `release` has `contents: write`. |
| `secrets.*` | None. `gh` uses `github.token`. Signing is ad-hoc. |
| Script injection | No `${{ }}` inside any `run:` (scripted check). `github.ref_name`, `inputs.config` and `vars.RELEASE_FINAL_ALLOWED` reach the shell through `env:`. `TAG` is regex-checked before the notes heredoc expands it. No `head_ref` or PR titles are used. |
| Fork PRs | Trigger is `pull_request` (not `_target`), so the token is read-only and there are no secrets. Every checkout has `persist-credentials: false`. |
| Cache poisoning (PR → release) | Not possible. PR caches are scoped to `refs/pull/N/merge`, while a tag run reads only its own ref and the default branch. **Confirmed empirically:** the release run logged `Cache not found` for `obs-deps-…` and both vcpkg keys, although PR #3 holds entries with those exact keys. |
| Release gating (C-016/C-017/C-006) | Always `--draft` plus `--verify-tag`; `--prerelease` for `-beta.N`/`-rc.N`. A plain `vX.Y.Z` is refused unless `vars.RELEASE_FINAL_ALLOWED == 'true'`. The tag must match the buildspec `X.Y.Z`, and the core tag must match the submodule. No `.deb` is downloaded or attached. Evidence P4-006 covers 16 tag/variable cases, and the live run confirms it. |

By severity:
- **Critical / High:** none.
- **Medium:** none.
- **Low:** S1. A fork PR controls `plugin/buildspec.json`, and `jq -r .name` goes into
  `GITHUB_ENV` unvalidated (`build-plugin.yaml:53-59`, `:257-263`).
  - A name with a newline could inject env vars into that PR's own run.
  - Under `pull_request` (read-only token, no secrets, PR-scoped cache) this gains nothing.
  - It would become exploitable only if someone later switched to `pull_request_target`.
  - Optional hardening: validate `name`/`version` against `^[a-z0-9-]+$` / `^[0-9]+\.[0-9]+\.[0-9]+$`
    in *Set up environment*.
- **Info:** I1. The deleted test tag left about 455 MiB of tag-scoped caches. They are
  `obs-deps-…` saved at 09:26:35 and the two `…-45146667649-1` vcpkg entries, shown by
  `gh cache list`. They expire after 7 days unused; `gh cache delete <id>` removes them now.

## Major issues (must-fix)

None.

## Minor issues (nice-to-have)

- **m1, checksum command mismatch.** Judged **Low; worth aligning, not blocking.**
  - The release notes (`release.yaml`, *Write release notes*) say `shasum -a 256 -c SHA256SUMS.txt`.
  - `plugin/README.md` *Download* says `shasum -a 256 --ignore-missing -c SHA256SUMS.txt`.
  - Both are correct when both files are downloaded. But the notes version prints `FAILED open or
    read` and exits 1 for the file a user did not download. Most users take only the pkg or only
    the zip, so they may conclude the download is corrupt.
  - Fix: use the README form in the notes (one line). TC-R05's wording can stay as the
    both-files check.
- **m2, stale evidence files.** The proof exists, but the session files lag behind it.
  - `develop/evidence/P4-006-release-logic.md` still ends with "Unverified until a test tag runs".
    Run `45146667649` has since verified it: four green jobs, the draft created, pkg + zip +
    `SHA256SUMS.txt` with no `.deb`, the checksums `OK`, then the draft and tag deleted.
  - `output-P4-005.md` / `evidence/P4-005-linux-check-dry-run.md` stop at CI iteration 4 and have
    no "run 5 green" section. The check excerpts are in the CI log, `session.log` and
    `P4-007-readme.md`: the 7 exports, `NEEDED libobs.so.30 / libOpenGL.so.0 / libEGL.so.1`, the
    `dlopen` probe line, `dpkg-deb -c` and `Depends`.
  - The plan also asked for cold and warm timings (TC-C01/C02), which were never recorded. From
    `gh`:
    - macOS: cold 5m39s (run 1), warm 1m59s (run 5);
    - Linux: about 14.5 min warm (run 5, mostly PPA retries), 11m38s on the release run with cold
      vcpkg.
  - Append a short "CI result" section to each, so the record does not depend on Actions log
    retention.
- **m3, README Linux build-from-source block** (`plugin/README.md`, *Build it yourself*).
  - `docs/structure.md` says developer build steps belong in `docs/integrations.md`, not the user
    page. The developer flagged this, and the coordinator asked for it here, so it is a judgement
    call. Consider moving it to `docs/integrations.md` in P4-012 and linking to it.
  - The block also clones vcpkg at HEAD while CI pins `5f96cd15`. That is harmless, because the
    core's `vcpkg-configuration.json` pins the baseline, but `git -C ~/vcpkg checkout 5f96cd15…`
    would match CI exactly.
  - "the build only runs with the `ubuntu-x86_64` presets" overstates the guard, which accepts any
    Ninja configure on Linux x86_64.
- **m4, duplicated archive paths** (`plugin/CMakeLists.txt`). The `libprojectM_ns-eel2.a` and
  `libprojectM_eval.a` paths are spelled twice:
  - in `SYNESTHESIA_VIBE_CORE_DEPENDENCY_LIBRARIES` (`BUILD_BYPRODUCTS`);
  - again in the `IMPORTED_LOCATION` of `SynesthesiaVibe::NsEel2` / `::ProjectMEval`.

  One variable per archive would keep them in step.
- **m5, platform log in the GL header** (`plugin/src/opengl-loader.hpp`). `plugin-main.cpp` now
  includes the platform GL headers only to call `opengl_log_platform`. The windowing-platform log
  is not a GL-loading concern. It works and keeps the single `#if`; only revisit it if the file
  grows.
- **m6, dead `libGL.so.1` fallback** (`plugin/src/opengl-loader-linux.cpp:21`). The `.so` has
  `NEEDED libOpenGL.so.0`, so `dlopen("libOpenGL.so.0")` always succeeds once the plugin has
  loaded, and the `libGL.so.1` fallback can't be reached in practice. It is harmless and matches
  C-011 as decided. Note it if C-011 is revisited after P4-008.
- **m7, Linux guard checks only the host.** It checks the host processor, not the target, so an
  x86_64 host cross-compiling with a toolchain file would pass. This is an edge case and not in
  scope.
- **m8, PR #2 log line in this diff.** The diff carries one line appended to the **Phase 2**
  session log (`20261123091415-plugin-phase-2-macos/session.log`: "committed a7b62ed … opened PR
  #2"). It is harmless bookkeeping that logically belongs to PR #2. Leave it, or move it when
  rebasing onto merged #2.

Documentation hand-off to document-writer (P4-012), not developer defects:
- `docs/integrations.md`:
  - CI/CD still says "None", and the guard message it quotes is stale;
  - add the release flow, the `RELEASE_FINAL_ALLOWED` variable, the SHA-to-version map (C-019) and
    Linux build/package;
  - add the `windowing platform:` log line.
- `.claude/common-issues/plugin-build.md`:
  - it quotes the old guard message;
  - its "Known trade-off" line about the CI seed is now resolved.
- `docs/structure.md`: `.github/workflows/`, `exported-symbols.map`, the loader files,
  `plugin/release/`, `build_x86_64/`.
- `docs/conventions.md`:
  - the import example still lists `<OpenGL/gl3.h>` and `<dlfcn.h>` under `visualizer-source.cpp`;
  - *Binary exports* should name both export files;
  - record the YAML `#` and `.map` `/* */` header forms.
- `docs/concerns.md`:
  - mark the seed debt resolved;
  - add the Linux floor 32.2.0 / PPA dependency, Flatpak, and the C-029 `-O0` finding.
- The LINK_GROUP rationale goes in architecture or integrations.

## Tests

N/A. Tests are not expected (`CLAUDE.md`). The inline CI checks are acceptance gates, not a test
suite (C-027). Every one is wired in, fails the job with an `::error::`, and ran green.

## Plan alignment notes

| Task | Status | Notes |
|---|---|---|
| P4-001 | Met | Seed skipped on CI; local first configure passes; `plugin/release/` ignored; TC-C07 asserted in CI. |
| P4-002 | Met | Reusable + entry workflows; pinned runner and Xcode; vcpkg pinned and cached; failing inspection. Timings unrecorded (m2). |
| P4-003 | Met | Per-OS loader; macOS import/export diff identical; Linux compiled under GCC 13 `-Werror`. TC-M03 deferred (C-024). |
| P4-004 | Met | Guard, `OS_MACOS` split, PIC, archives + RESCAN group, version script + `--exclude-libs,ALL` (+ `--no-undefined`, a justified extra), contact. |
| P4-005 | Met in CI | Exports, TEXTREL, ldd, `dlopen` `RTLD_NOW`, `.deb` contents and Maintainer/Depends all green. Evidence file stale (m2). C-013 stop rule not triggered; no core edit. |
| P4-006 | Met | C-016 (b) and C-017 as decided; `-ci.1` correctly refused, `-beta.0` used. Live draft verified, then cleaned up. |
| P4-007 | Met | Asset names, paths, package name and log lines cross-checked against CI. Linux wording matches C-006. m1 and m3 are small follow-ups. |
| P4-008 | Not a defect | User-owned. |
| P4-009, P4-010 | Not a defect | Dropped/closed per Gate 1. |

No scope creep beyond m8. Neither the core submodule nor `plugin/cmake/**` was touched.
