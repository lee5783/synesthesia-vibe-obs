# Documentation sync report

## Summary

This pass covers LIC-002, BS-010 and SPK-010 together. It documents three things as shipped and
reviewed: the Phase 1 Browser Source page (`browser-source/`), D1 (GPL-2.0-or-later, LIC-001), and
the Phase 2 macOS spike (`plugin/`, the `external/synesthesia-vibe` submodule, D4).

- **Target:** `docs/`. It now has the seven codebase-mapper pages plus `decisions.md`, and
  `docs/README.md` lists exactly those eight. Gotchas went to `.claude/common-issues/`. Two flow
  write-ups went to `.claude/how-x-works/`.
- **Writes outside `docs/` and `.claude/`.** The user approved these in this run's pre-docs
  approvals (C-013), recorded in `plan/output-plan-gate-1-decisions.md`: *"the `CLAUDE.md`
  routing table plus the Setup and 'nothing builds yet' wording; the `PLAN.md` line-3 status;
  adding `.vscode/` to `.gitignore`."* The orchestrator's packet also listed these `PLAN.md` items:
  R2-N1, R2-N2 and the Pages (D5) mentions.
- **Not touched:** the root `README.md` and all application source.
- **Gate 0:** CodeGraph is configured and reachable. The index covers `plugin/src/*.cpp`,
  `browser-source/obs-page.js` and the core submodule, and it was used for the plugin and page
  symbols.

## Files updated

- `docs/stack.md` (new): languages (C++17 / C17, plain ES modules, Bash, CMake), runtimes (OBS CEF,
  libobs 31.1.1 checked in OBS 33.0.3, OpenGL renderer only), core `v0.7.0` web dist and C API,
  template pin, toolchain (CMake ≥ 3.28, Xcode, Ninja, vcpkg), env vars, platform requirements.
- `docs/integrations.md` (new):
  - core release and tarball URLs, the checksum behaviour, the version argument, the
    `FETCH_DIST_BASE_URL` / `FETCH_DIST_ARCHIVE_URL` test hooks and the required tools;
  - the submodule setup (`git submodule update --init --recursive`, never `--remote`);
  - obs-browser, libobs and obs-websocket (verification only, no credentials in the repo);
  - the template's first-configure downloads;
  - the `dist/` layout and manifest;
  - logs: the `obs-page:` prefix, OBS copying console errors, `--remote-debugging-port`
    `/json/list` exposure, and the plugin's expected log lines;
  - no hosting (D5) and no CI;
  - full local build, `ditto` install, preset copy, ad-hoc re-sign, `nm -gU` check, uninstall.
- `docs/architecture.md` (new):
  - the two-adapter pattern and its layers;
  - Browser Source startup flow, module load with the renderer check, and the plugin frame flow
    (tick/render split, render once per tick, texrender, sRGB, `GS_FLIP_V`);
  - the `gl_state_guard` contract (with the SPK-006 `GL_TEXTURE7` caveat), the core GL loader,
    the export list and the matcher;
  - entry points, error handling, logging, validation, threading and the licensing boundary.
- `docs/structure.md` (new):
  - the directory layout;
  - verbatim template files versus authored files, with the template SHA;
  - where new code goes, including the export-list rule;
  - the special directories: `dist/`, `.dist-*`, `.deps/`, `build_macos_arm64/` with the
    `synesthesia-vibe-core/{build,install}` layout, `.CMakeBuildNumber`, and the session evidence
    git-ignore rule.
- `docs/conventions.md` (new):
  - per-language naming and formatting (JS 4-space and double quotes, C++ tabs and snake_case,
    Bash `set -euo pipefail`, CMake 2-space) and include order;
  - the `stage()` error pattern, null-fallback ternaries and `die`;
  - logging prefixes and levels;
  - the no-comments rule with the standard header in each comment syntax, vendored files exempt;
  - the export-list trap and the `with_core_gl_state` rule.
- `docs/testing.md` (new): there is no suite. It describes how verification actually runs:
  - headless Chrome over CDP (flags, the fake audio WAV with the `AudioServiceSandbox` flag,
    `Browser.setPermission`, the PCM/levels probe);
  - OBS through obs-websocket only (`CreateInput`, `SetSceneItemEnabled`, `GetSourceScreenshot`,
    `GetInputKindList`, `GetStats`), plus the page console over CDP and OBS log lines;
  - plugin build checks (`lipo`, `codesign`, `nm -gU`);
  - the user checklist;
  - the SPK-006 audit, with the `GL_TEXTURE7` caveat;
  - where evidence lives and what is committed. No tests are invented.
- `docs/concerns.md` (new). It includes every item in review round 2's "remaining concerns for
  the docs pass":
  - S3 (macOS/arm64/Xcode unguarded);
  - S1 (the code-side race, with the TSan / screenshot-loop repro and atomic exchange from R2-N3);
  - `BUILD_ALWAYS` visibility plus R2-N4 (relink after a bump unverified);
  - the SPK-006 evidence note;
  - N2 (scissor and colour mask), N5 (build-number seed vs CI), N6–N10 (fetch-dist and matcher
    rough edges), N12 (the unverified "Shutdown source" claim, under test gaps).

  Also from the developer handoffs: alpha-0 presets, the `dt` gap, the spike stand-ins, the
  double preset load, hand-copied presets, the en-US-only locale, renderer-check gaps (no in-app
  notice, Metal value unobserved), 1080p60 unmeasured, supply-chain notes, local serving and
  debug-port exposure, the libobs 31.1.1 vs D6 note, and test-coverage gaps.

  Left out as not useful for a concerns page: N11 (header before the doctype, accepted) and N13
  (evidence file case, a harness-template matter).
- `docs/decisions.md` (new):
  - **D1** (locked 2026-11-23): GPL-2.0-or-later, the rationale, and the "or any later version"
    grant stated here and in the `PLAN.md` D1 row, per C-021 (no SPDX lines, no README edit).
    The MIT core and its notices keep their own licence; presets keep their BeatDrop /
    CC-BY-NC-SA terms and are not redistributed.
  - **D4** (locked 2026-11-23, confirmed by the user): the submodule pinned at `v0.7.0` /
    `887628f`, a separate `ExternalProject_Add` build, the imported static target and no
    XCFramework, with the rationale and consequences.
  - The licence identifier matches the `PLAN.md` D1 row word for word.
- `docs/README.md`: replaced "No pages exist yet". It now lists all eight pages, adds a
  `decisions.md` row, and points to `browser-source/README.md` for user setup.
- `.claude/common-issues/plugin-build.md` (new): the template's empty build number on first
  configure (the guard in `plugin/CMakeLists.txt`, and its CI trade-off); the lightweight
  `v0.7.0` tag that shows as `v0.5.0-19-g887628f`.
- `.claude/common-issues/plugin-runtime.md` (new): the OBS GL state cache desync and
  `gl_state_guard`, with the SPK-006 caveat.
- `.claude/common-issues/browser-source.md` (new): macOS mic permission denied, which the page
  reports as `NotFoundError`; the first-ever Browser Source in a profile staying blank until
  restart.
- `.claude/common-issues/verification-tooling.md` (new): the Chrome fake audio file is silent
  without `--disable-features=AudioServiceSandbox`; the `osascript` quit returns -128 but OBS
  quits.
- `.claude/common-issues/_example-area.md`: deleted, as its index row says to once real entries
  exist.
- `.claude/how-x-works/plugin-render-frame.md` (new): create, then `video_tick` →
  `video_render` → `render_core_frame` → guard → `sv_render_frame` → flipped sprite draw, then
  teardown, with line citations.
- `.claude/how-x-works/browser-source-startup.md` (new): URL settings → dist import → manifest
  → playlist → cycling → audio, with line citations.
- `.claude/how-x-works/_example-flow.md`: deleted, as its index says to once real entries exist.
- `CLAUDE.md` (user-approved):
  - added a `docs/decisions.md` row to the routing table;
  - replaced the "docs pages do not exist yet" sentence;
  - replaced "Nothing builds yet" in *Setup* with the real commands: the submodule; `fetch-dist.sh`
    plus `python3 -m http.server 8080 --bind 127.0.0.1 --directory browser-source`; the plugin
    build with its Xcode, CMake, `ninja` and `VCPKG_ROOT` prerequisites;
  - replaced the stale CodeGraph "index is empty" sentence.

  Nothing else changed.
- `PLAN.md` (user-approved):
  - line 3 status: Phase 1 built and awaiting the loopback checklist; spike done; D1 and D4
    locked; D5 open and skipped;
  - R2-N1: task 2 now says `video_tick` accumulates and `video_render` passes the delta;
  - R2-N2: "removed in the review fix pass";
  - D5 row state: open and skipped for now;
  - the layout comment and Phase 1 task 5 now mark the Pages workflow as deferred.
- `.gitignore` (user-approved): added `.vscode/`. `git check-ignore` confirms that
  `.vscode/settings.json` is now ignored.

## Index updates

- `docs/README.md`: eight pages listed (`stack`, `integrations`, `architecture`, `structure`,
  `conventions`, `testing`, `concerns`, `decisions`). This matches `ls docs/`.
- `CLAUDE.md` routing table: the seven existing rows plus `docs/decisions.md`.
- `.claude/COMMON-ISSUES.md`: four area rows. The example row is removed.
- `.claude/HOW-X-WORKS.md`: two flow rows. The example row is removed.
- `.claude/DICTIONARY.md`, `.claude/MEMORY.md`: unchanged (no entry was clearly needed).

## Verified

- Every page fact was checked against the current source: `obs-page.js`, `fetch-dist.sh`,
  `index.html`, `plugin/src/*`, `plugin/CMakeLists.txt`, `plugin/CMakePresets.json` (parsed),
  `plugin/buildspec.json`, `exported-symbols.txt`, `.gitmodules` and
  `git submodule status` / `git -C external/synesthesia-vibe log`. Facts were also checked
  against the handoffs and both review rounds.
- Every `.claude/common-issues/` and `.claude/how-x-works/` link cited from `docs/`,
  `.claude/` or `CLAUDE.md` resolves to a file.
- Hygiene grep over every file written in this pass found no absolute paths, user name, local
  folder names, websocket URL or port, or credential values. The only hits are generic prose
  saying credentials stay out of the repo.

## Follow-ups / gaps

- **Root `README.md` is not touched**, by instruction. codebase-mapper's README link rule (link
  `docs/README.md` from the root README) and C-021's optional licence line both wait for a user
  request.
- **`PLAN.md` items left alone (outside the approved edits):**
  - the public-plan tone nits at the *Spike results* time line ("while the user fixed the
    obs-websocket login") and the 1080p60 follow-up ("The user's OBS profile is 1080×1920…");
  - Phase 4 item 4 ("Run `codebase-mapper` to generate `docs/`…"), which is now partly done.
- **Hygiene items for the orchestrator** (outside my write list): review round 2 asks for
  `develop/evidence/REVIEW-FIX/review-fix-report.md:58,60` and `develop/output-REVIEW-FIX.md:109`
  to be neutralised before commit.
- **Still unverified, documented as such:**
  - Phase 1 loopback and real mic in OBS, and Windows/Linux (user checklist);
  - the flag-missing error name;
  - audio after hide/show;
  - 1080p60;
  - the real Metal renderer;
  - relink after a core bump;
  - the S1 race repro.
- `.claude/how-x-works/` could also get a "how OBS loads a module" page (an SPK-007 suggestion).
  Skipped to keep this pass small; `docs/architecture.md` covers module load and the export list.
- Evidence file names that break kebab-case (N13) are a harness-template fix, not a docs fix.
