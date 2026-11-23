## Summary

This PR adds the Phase 1 Browser Source page, locks D1 (GPL-2.0-or-later) and D4, and runs the
Phase 2 macOS plugin spike from `PLAN.md`. The page loads the SynesthesiaVibe web build as an OBS
Browser Source and is driven by URL settings. The spike shows the core can render inside OBS
33.0.3 through a native source, but only if the plugin saves and restores OBS's GL state around
every call into the core. Every spike question in `PLAN.md` now has an answer.

**Ticket:** none
**Design:** none

## Changes

**Browser Source (Phase 1)**
- `browser-source/fetch-dist.sh`: downloads the pinned core web build (`v0.7.0`) and checks its
  SHA-256. It then extracts the presets, textures, `LICENSE` and `NOTICE` from the tag's source
  tarball and writes `dist/resources/manifest.json`. The old `dist/` is replaced in one swap, and
  any failure leaves it untouched. `dist/` is git-ignored, and the presets are for local serving
  only.
- `browser-source/index.html`, `browser-source/obs-page.js`: a full-window black canvas with no UI.
  - URL settings: `preset` (name match), `cycle`, `blend`, `fps`, `audio`.
  - `cycle` keeps the engine's meaning: presets change every `cycle + blend` seconds.
  - A preset that fails to load is skipped.
  - Audio input is chosen by device label, with a fallback to the default input and an
    `audio=none` option.
  - Audio starts on the first click or keypress where autoplay is blocked.
  - If audio fails for any reason, the visuals keep running.
- `browser-source/README.md`: local serving with `--bind 127.0.0.1`, OBS setup, launch flags,
  parameter table, audio routing, troubleshooting, credits and licences.
- GitHub Pages (BS-006) was dropped at Gate 1, so D5 stays open. The core says its presets are
  not approved for public hosting.

**Licence (D1)**
- `LICENSE`: the GPL-2.0 text, downloaded from gnu.org.
- `PLAN.md`: D1 is locked as GPL-2.0-or-later. `docs/decisions.md` records it.

**Phase 2 spike (macOS, arm64)**
- `.gitmodules`, `external/synesthesia-vibe`: the core as a submodule, pinned to `v0.7.0`
  (`887628f`).
- `plugin/`: a minimal libobs source plugin, "SynesthesiaVibe Visualizer".
  - `plugin/cmake/**` and `src/plugin-support.*` are copied verbatim from obs-plugintemplate
    `3e7d7ac3`.
  - `CMakeLists.txt` builds the core separately with `ExternalProject_Add` (`BUILD_ALWAYS`) and
    links it as an imported static target. It also works around the template's empty build
    number on first configure.
  - `CMakePresets.json` adds a `macos-arm64` preset.
  - `exported-symbols.txt` exports only the 7 `obs_module_*` functions.
  - `src/plugin-main.cpp` registers the source only on the OpenGL renderer and logs a warning
    otherwise.
  - `src/visualizer-source.cpp`:
    - core setup and teardown inside the graphics context;
    - `sv_render_frame` into the texrender framebuffer, drawn flipped vertically with `GS_FLIP_V`;
    - `gl_state_guard` around every core call;
    - a synthetic tone as a stand-in for audio.
- `PLAN.md`: a *Spike results* section answers each question. D4 is locked (confirmed by the
  user). It also covers Phase 2 follow-ups, Risks, and drafts of upstream asks #1 to #3 (not
  filed). The status line is updated, and the task 3 and 4 threading plan is corrected.

**Docs and agent knowledge**
- `docs/`: new pages `stack`, `integrations`, `architecture`, `structure`, `conventions`,
  `testing`, `concerns` and `decisions`, and `docs/README.md` now lists them.
- `.claude/common-issues/`: four areas (`plugin-build`, `plugin-runtime`, `browser-source`,
  `verification-tooling`).
- `.claude/how-x-works/`: `plugin-render-frame`, `browser-source-startup`.
- `CLAUDE.md`: updated routing table and real *Setup* commands (user-approved).
- `.gitignore`: adds `plugin/.deps/`, `plugin/build_*/`, `plugin/cmake/.CMakeBuildNumber`,
  `plugin/CMakeUserPresets.json` and `.vscode/`.
- `.claude/working-sessions/20261123004446-browser-source-phase-1/`: the vibe-flow run (plan,
  decisions, task reports, evidence reports, review, docs report).

## Verification

- [x] Plugin builds clean: `cd plugin && cmake --preset macos-arm64 && cmake --build --preset macos-arm64`
  gives `** BUILD SUCCEEDED **` with 0 warnings in `plugin/src` (see `docs/integrations.md`).
  Needs `VCPKG_ROOT` and `ninja`. arm64 only.
- [x] `browser-source/fetch-dist.sh`: shellcheck clean. Tested on macOS for the happy path, both
  version forms, a tampered zip, a bad tarball, a missing version, an unreachable host and
  SIGTERM. Not run on Linux.
- [ ] Existing tests over the touched area still pass: none exist. The repo has no test suite
  yet (`docs/testing.md`).
- Tests added: none, not requested.

**Evidence**: from `develop/evidence/`. Captures are git-ignored; attach them to the PR. Each
folder's `.md` report quotes the key results.

| What it shows | Capture |
|---|---|
| Page renders a preset full-frame at 1920×1080 in headless Chrome | `BS-003/BS-003-render-1920x1080.png` |
| `cycle` switching between Fractopia and swarm | `BS-004/BS-004-cycle-a.png`, `BS-004/BS-004-cycle-b.png` |
| Audio-reactive vs silent (Chrome fake device) | `BS-005/BS-005-default-t9.5s.png`, `BS-005/BS-005-none-t5s.png` |
| Audio starts after a click when autoplay is blocked | `BS-005/BS-005-suspended-before-click.png`, `…-after-click.png` |
| Browser Source rendering inside OBS 33.0.3 | `BS-008/BS-008-obs-a.png`, `BS-008/BS-008-obs-b.png` |
| Plugin source listed in OBS | `SPK-003/SPK-003-add-source.png` |
| Before/after the shader-program fix; image upside down before/after `GS_FLIP_V` | `SPK-005/SPK-005-before-fix-visualizer-alone.png`, `…-orientation-after-flip.png` |
| Visualizer beside image, colour and text sources | `SPK-005/SPK-005-obs-scene-a.png`, `SPK-008/SPK-008-scene.png` |
| Two instances, hidden source, preset variety under the full guard | `SPK-006/SPK-006-two-instances.png`, `…-visualizer-hidden.png` |
| Final build after the review fixes, through scene switches and show/hide | `REVIEW-RECHECK/REVIEW-RECHECK-scene-t0.png`, `…-scene-after-cycles.png` |

Key log lines (OBS 33.0.3, final build, from `REVIEW-RECHECK/review-recheck-report.md`):

```
[synesthesia-vibe-obs] renderer check passed (OpenGL, device type 1)
[synesthesia-vibe-obs] plugin loaded successfully (version 0.1.0)
[synesthesia-vibe-obs] sv_init_gl and sv_create succeeded (1920x1080)
... 0 GL error lines over ~142 s, 4 scene switches, 2 hide/show cycles ...
[synesthesia-vibe-obs] plugin unloaded
Number of memory leaks: 0
```

`nm -gU` on the plugin binary lists exactly `obs_module_{free_locale,get_string,load,set_locale,set_pointer,unload,ver}`.

**Manual QA**: from `plan/output-plan-test-cases.md`

| Test case | Precondition | Expected | Status |
|---|---|---|---|
| TC-030 OBS macOS render | OBS with `--enable-media-stream`, 1920×1080 Browser Source on `http://localhost:8080/` | Visualizer fills the source | Agent-verified (BS-008) |
| **TC-031 OBS macOS loopback (Done when)** | BlackHole 2ch + Multi-Output Device, `?audio=BlackHole&cycle=15` | Visuals react to the music | **Pending: you.** First turn on microphone access for OBS (System Settings → Privacy & Security → Microphone); it is currently off |
| **TC-032 OBS Windows loopback (Done when)** | Windows 10/11, flag on the OBS shortcut, VB-Cable, `?audio=CABLE%20Output&cycle=15` | Same as TC-031 | **Pending: you** |
| TC-033 flag missing | OBS without `--enable-media-stream` | Visuals render; audio error in console | Partly checked: the error name was hidden by the denied mic permission |
| TC-034 source hidden/shown | Toggle visibility or switch scenes | Pauses, resumes; audio continues after resume | Pause/resume verified; audio after resume untested |
| TC-045 renderer check, non-OpenGL | You switch OBS to Metal and restart | "needs the OpenGL renderer" warning; source missing | Optional, yours. Checked only with a faked renderer value |

The full user checklist is in `develop/evidence/BS-008/bs-008-user-checklist.md`. Phase 1 is
marked done only after you report back on it.

## Review

- [x] Reviewed by the `code-reviewer` pass. Round 1: Request changes (2 blockers, 4 should-fix).
  Round 2: **Approve**.
- [x] Must-fix findings addressed:
  - B1: a private path removed from a session file.
  - B2: the 10 s debug log line removed.
  - S4: `PLAN.md` no longer links into the session folder.
  - S1: the `PLAN.md` threading wording fixed.
  - S2: `BUILD_ALWAYS` added.
  - Null guards added.
  - The active texture unit is now restored inside the guard.
  - An OBS run after the fixes passed.
  - Remaining items are listed under Risks and follow-ups.
- **Areas to scrutinize:**
  - `gl_state_guard` in `plugin/src/visualizer-source.cpp`: what it saves and what it
    deliberately leaves out (see `PLAN.md` *GL state*).
  - The renderer check returns `true` when it refuses. The reviewer confirmed this is intended.
  - The hand-maintained `plugin/exported-symbols.txt`.
  - `fetch-dist.sh`'s tarball extraction (exact paths, regular files only).
  - The deferred audio start in `obs-page.js`.

## Documentation

- [x] `docs/**` updated: the first eight pages plus `docs/README.md`, and `docs/decisions.md`
  (D1, D4).
- [x] New gotchas and flows recorded in `COMMON-ISSUES` (four areas) and `HOW-X-WORKS` (two
  flows). No `DICTIONARY` or `MEMORY` entries.
- Root `README.md`: not touched, not requested. A link to `docs/README.md` and a licence line are
  left for a later request.

## Risks and follow-ups

- **Phase 1 Done-when is open:** loopback audio in OBS on macOS (BlackHole) and Windows
  (VB-Cable) waits on your checklist. Microphone access for OBS is off on this Mac.
- **Thread race in the spike code:** `video_tick` calls `sv_push_pcm_float` and writes the
  pending `dt` without the graphics lock. That races `video_render` when it runs on another
  thread, for example obs-websocket `GetSourceScreenshot`. Phase 2 task 3 moves both into
  `video_render`. A TSan repro is in `docs/concerns.md`.
- **macOS arm64 / Xcode only, unguarded:** the template's universal, Windows and Ubuntu presets
  fail in confusing ways. A non-Xcode generator silently drops the export list.
- **Export list is hand-maintained:** a new optional `obs_module_*` entry point must be added to
  `plugin/exported-symbols.txt`, or OBS never calls it.
- **Alpha-0 presets** (e.g. swarm) show up transparent on the canvas. Phase 2 should draw the
  visualizer as opaque.
- **Not measured:** 1080p60 (the OBS profile on this Mac is 1080×1920 at 30 fps; cost there is
  about 4–7 ms per frame). The real Metal renderer was not run either.
- **Not verified:** whether the plugin relinks after a core bump, and the error name when only
  the media-stream flag is missing.
- **`dt` after hide or scene switch:** all the time that passed is handed to the core in one
  frame on return. Phase 2 should clamp it.
- **Presets:** they are copied into the installed bundle by hand. Packaging is a Phase 2
  decision.
- **Core `v0.7.0`:** presets are not approved for public hosting, so Pages (D5) is deferred. The
  tarball has no published checksum (accepted, C-010).
- **Upstream asks:** #1 (subdirectory-safe CMake) and #2 (document GL state) are confirmed; #3
  is optional. None is filed; drafts are in `PLAN.md`.
- **LICENSE double check:** gnu.org timed out on the second download, so `LICENSE` was matched
  against two GNU package copies instead. To re-check:
  `curl -fsSL https://www.gnu.org/licenses/old-licenses/gpl-2.0.txt | shasum -a 256` should
  print `edaef632…33f6`.
- **Your cleanup after this run:**
  - turn obs-websocket authentication back on with a new password (the old one was shared in
    chat);
  - delete the `synesthesia-spike` scene collection.

## Checklist

- [x] No secrets, tokens or credential files in the diff. Grepped for the websocket password,
  credential and scratchpad paths, absolute `/Users/` paths, the username and local folder names.
  `.vscode/settings.json` (which holds a home path) is now git-ignored.
- [x] No AI-session-only files staged unintentionally. Session artifacts under
  `.claude/working-sessions/` are meant to be committed per `HARNESS.md`. Bulk evidence
  (PNG, logs) is git-ignored.
- [x] New files carry the standard header: `.sh`, `.js`, `.html`, `.cpp`, `.hpp`, CMake, `.ini`
  and `exported-symbols.txt`. Excluded: README, LICENSE, JSON and verbatim template files.
- [x] No unrequested code comments added. The reviewer confirmed this.
- [x] Style matches the sibling components. The reviewer confirmed this.
- [ ] Shared components reused or extended: N/A, there are no shared or design-system components
  in this repo.

---
🤖 Built with an AI vibe-flow run — session `20261123004446-browser-source-phase-1`

🤖 Generated with [Claude Code](https://claude.com/claude-code)
