# REVIEW-FIX

## Subtasks executed

- **B1.** Removed the absolute scratchpad path from `output-SPK-006.md:176`. Then re-ran the
  hygiene grep over all 85 to-be-committed files (untracked, tracked changes and staged, without
  `.vscode/`). No real path is left.
- **B2.** Removed the plugin's 10 s `LOG_INFO` frame/fps/gl_errors debug callback and its
  interval constant. Updated the `PLAN.md` stand-ins list to match.
- **S4.** Dropped the `PLAN.md` pointer into `.claude/working-sessions/`. The upstream-ask drafts
  are inline below it.
- **S1 (`PLAN.md` wording only):**
  - Phase 2 task 3 now drains the ring and consumes `dt` in `video_render`, under the graphics
    context. `video_tick` writes only atomics.
  - Task 4's `sv_commit_preset` moved the same way, and so did the "Slow preset compiles" risk
    mitigation.
  - Added a spike follow-up on the `sv_context` race between tick and render, with the TSan
    screenshot-loop repro, and extended the audio-handoff risk row. The code is unchanged until
    Phase 2, as the reviewer agreed.
- **S2.** Added `BUILD_ALWAYS TRUE` to the core's `ExternalProject_Add`.
- **N1.** `gl_state_guard` now restores the saved active texture unit right after its save
  loop, before the core runs. Corrected the `PLAN.md` GL-state table row ("unit 7 to unit 0" →
  "always leaves `GL_TEXTURE0` active"). The upstream ask #2 text already said
  "always `GL_TEXTURE0`", so it is unchanged.
- **N3.** Null-guarded `obs_get_module_data_path`.
- **N4.** Null-guarded the device name in the "renderer check passed" log line.

## Files changed

- `plugin/src/visualizer-source.cpp`
- `plugin/src/plugin-main.cpp`
- `plugin/CMakeLists.txt`
- `PLAN.md`
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/output-SPK-006.md`
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/input-dev.md` (appended)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/output-REVIEW-FIX.md`
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/REVIEW-FIX/review-fix-report.md`
- `.claude/working-sessions/20261123004446-browser-source-phase-1/session.log` (appended)

## What changed and why

- **B1, S4.** Public-repo hygiene. A session artifact named a machine path with the user name in
  it. `PLAN.md` linked to a session folder that is not a stable home.
- **B2.** S-003 says spike instrumentation goes before review. The line wrote about 360 log
  lines an hour per source.
- **S1.** The PLAN's Phase 2 audio design repeated the spike's race:
  - `video_tick` runs without the graphics lock and touches the `sv_context`.
  - At the same time, `video_render` can run on another thread that holds the lock (for
    example obs-websocket `GetSourceScreenshot`).
  - The spike code still has the race; it is now recorded as a Phase 2 task-3 fix.
- **S2.** Without `BUILD_ALWAYS`, a submodule bump kept the old stamps and linked the old
  `libSynesthesiaVibeCore.a`. With it, an unchanged core costs only a Ninja no-op plus an install
  up-to-date check.
- **N1.**
  - The save loop left unit 7 active while the core ran.
  - The SPK-006 diff logs show `ACTIVE_TEXTURE 33991 -> 33984` (unit 7 to unit 0). The 7 came
    from the guard itself, not from OBS.
  - The core now starts on the unit OBS had active.
- **N3, N4.** Cheap protection against undefined behaviour from a `NULL` `const char *`.

## Verification

- `cd plugin && cmake --preset macos-arm64 && cmake --build --preset macos-arm64`:
  - exit 0, `** BUILD SUCCEEDED **`;
  - both plugin sources recompiled;
  - 0 `warning:` lines in the whole log;
  - the inner core build printed `ninja: no work to do.` and the install step printed
    `Up-to-date`.
- `nm -gU` on the bundle binary shows exactly the 7 `_obs_module_*` exports: `free_locale`,
  `get_string`, `load`, `set_locale`, `set_pointer`, `unload`, `ver`.
- `strings` on the binary:
  - `^frame %u` has 0 hits;
  - the one remaining `gl_errors` string is the core's own "GL pipeline destroyed" message;
  - the "renderer check passed" line is still there.
- Hygiene grep over the 85 files for `/private/tmp`, `/Users/`, `claude-501`,
  `scratchpad/obs-ws` and `credentials.env` finds pattern-list mentions only:
  - `output-SPK-008.md:164`;
  - `review/input-code-review.md:59`;
  - `review/output-review-findings.md:34`;
  - `review/output-review-findings.md:255`, a `/Users/<name>/…` placeholder.

## Evidence

- `evidence/REVIEW-FIX/review-fix-report.md` holds:
  - the build summary and the `nm -gU` output;
  - the `strings` checks;
  - the hygiene-grep file set, patterns and hits;
  - the code changes listed by hand (the plugin files are untracked, so there is no git diff);
  - the runtime reasoning for item 7.
- **Runtime re-check: verified by the build only.** OBS was not touched, as instructed: no
  relaunch, no install, no websocket.
- Item 7 is not expected to change what renders. The core calls `glActiveTexture` before each
  sampling bind and always ends on `GL_TEXTURE0`. Only core binds made before its first
  `glActiveTexture` in a call (texture uploads) now land on OBS's active unit instead of unit 7.
  The guard restores units 0–7 either way. An OBS re-run is optional, not required. If the
  orchestrator wants one, a single Fractopia + text-source scene check like SPK-006 would cover
  it.

## Known limitations / follow-ups

- **Left for the docs pass, as instructed:**
  - S3 (macOS/arm64/Xcode assumptions are unguarded).
  - N2 (scissor and colour mask not guarded).
  - N5–N13.
- **The code side of S1 is still open.** The spike's `video_tick` still calls
  `sv_push_pcm_float` (synthetic tone) and writes `pending_seconds` / `frame_pending` without the
  graphics lock. Phase 2 task 3 fixes this, and `PLAN.md` records it with the TSan repro.
- **`review/output-review-findings.md` names the local folder.** Lines 34, 53, 252, 255 and 259
  mention the local folder name (in prose, a placeholder and a `~/…` fragment). None is an absolute
  path or holds the user name, and the file was outside this task's write list. The orchestrator
  decides whether to soften it before a commit.
- **Stale evidence.** `evidence/SPK-006/SPK-006-state-diff.md` (lines 39, 64) still records
  `GL_TEXTURE7 → GL_TEXTURE0` as measured. That was true for the instrumented run, but the 7 is a
  guard artifact. The doc pass can add a note if it cites that table. The file was outside this
  task's write list.
- **Historical `gl_errors 0` results.** `PLAN.md` lines 166, 252 and 302 still quote
  `gl_errors 0` from the core. They record spike results, not the shipped log line, so they stay.
- **Doc gap for document-writer.** `docs/concerns.md` should note two things: the S1 race until
  Phase 2 lands, and that `BUILD_ALWAYS` makes the core step run on every build (cheap, but
  visible in Xcode).

## Handoff for downstream review

- **Acceptance criteria addressed:**
  - B1, B2, S4, and S1 (the PLAN part) from the reviewer's "must fix" list;
  - S2, N1, N3 and N4 as cheap extras;
  - build green, no warnings, 7 exports, hygiene grep clean.
- **Areas to scrutinize:**
  - The N1 constructor ordering in `gl_state_guard`: save the active unit, loop over units 0–7,
    then restore the saved unit.
  - The `PLAN.md` wording changes in task 3, task 4, the new race follow-up bullet, the Risks
    rows, and the GL-state table row.
- **How to run:** `cd plugin && cmake --preset macos-arm64 && cmake --build --preset macos-arm64`
  (needs `VCPKG_ROOT` and Ninja).
