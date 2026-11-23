# SPK-008

## Subtasks executed
- **(a) Pending OBS load check of the SPK-007 build.** Ran the commands from `output-SPK-007.md`
  exactly:
  - quit OBS;
  - `ditto` the bundle into the plugins folder and copy the 10 presets and 23 textures;
  - `codesign --force --sign -`, then `codesign --verify`, which reported "signature ok";
  - `nm -gU` listed exactly the 7 `obs_module_*` symbols;
  - `open -a OBS`.

  Then, over the websocket only:
  - switched to `synesthesia-spike` and set the visualizer's `preset_path` to Fractopia;
  - took `GetSourceScreenshot` images 4 s apart and read them;
  - checked the log;
  - switched back to `Untitled`.

  The first pass left the collection's program scene on `BS-008`, so the visualizer was off air.
  The second pass put the program scene on `Scene`, where the visualizer lives, and set it back
  to `BS-008` before leaving the collection.
- **PLAN.md consolidation:**
  - added a *Spike results* block (answer table, time against the box, what is ready, follow-ups);
  - set D4 to locked, per the evidence;
  - added and updated Risks rows;
  - added a status to *Upstream asks*;
  - tidied the per-question answers.
- Drafted upstream asks #1, #2 and optional #3 as issue-ready text. None was filed.
- **(b) S-012 cleanup:**
  - quit OBS gracefully;
  - checked the path and bundle id, then removed only
    `~/Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin`;
  - relaunched with plain `open -a OBS`;
  - confirmed from the log and the websocket that OBS started without the plugin.

## Files changed
- `PLAN.md`
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/input-dev.md` (SPK-008 section appended)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/output-SPK-008.md` (this file)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/SPK-008/spk-008-upstream-asks.md`
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/SPK-008/SPK-008-plan-diff.md`
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/SPK-008/SPK-008-visualizer-t0.png`
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/SPK-008/SPK-008-visualizer-t4s.png`
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/SPK-008/SPK-008-scene.png`
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/SPK-008/SPK-008-visualizer-off-air.png`
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/SPK-008/SPK-008-obs-log-load-check.txt` (git-ignored)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/SPK-008/SPK-008-cleanup.txt` (git-ignored)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/session.log` (log lines)
- Outside the repo:
  - The installed plugin bundle was reinstalled for the check, then removed.
  - In the `synesthesia-spike` collection, the visualizer's `preset_path` was set, then reset to
    empty, and the program scene was restored to `BS-008`.
- Not touched:
  - `plugin/` source and `plugin/build_*`;
  - `external/synesthesia-vibe/`;
  - OBS config files, the OBS profile and the `synesthesia-spike` collection itself;
  - `docs/`.

## What changed and why

**OBS load check: pass.** OBS 33.0.3 log (`evidence/SPK-008/SPK-008-obs-log-load-check.txt`):
- `[synesthesia-vibe-obs] renderer check passed (OpenGL, device type 1)`
- `[synesthesia-vibe-obs] plugin loaded successfully (version 0.1.0)`, and the module is listed
  under Loaded Modules.
- `sv_init_gl and sv_create succeeded (1920x1080)` on each switch into `synesthesia-spike`.
- `frame 310, 30.0 fps, preset "Rovastar - Fractopia (Upspoken Mix)", gl_errors 0, last_error ""`
  and `frame 610 …, gl_errors 0`.
- 0 GL error lines (`glUniform|(GL) failed|GL_INVALID|glActiveTexture failed`).
- On quit: `plugin unloaded` and `Number of memory leaks: 0`.
- `GetInputKindList` included `synesthesia_vibe_visualizer`.
- `GetStats` on air: 6.96 ms, then 6.21 ms average render time; 30 fps; 2 skipped frames out of
  3250.

The export list from SPK-007 therefore loads and runs in OBS. The `PLAN.md` "still pending" note
is replaced with this result.

**Observation.** With the visualizer off air (program scene `BS-008`), both source screenshots came
back black. Nothing draws an off-air source, so it renders only when the screenshot asks for a
frame, and it gets the whole gap as one step. This is the "clamp `dt` after hidden/inactive"
follow-up. It is recorded in `PLAN.md` as an observation, not a diagnosed cause.

**PLAN.md**, see `evidence/SPK-008/SPK-008-plan-diff.md` (79 lines in, 21 out):
- **D4 → Locked (2026-11-23), confirmed by the spike.**
  - The submodule is pinned to `v0.7.0` (`887628f`).
  - The core is not `add_subdirectory`'d. It is a separate `ExternalProject_Add`
    configure/build/install (Ninja, vcpkg from `$VCPKG_ROOT`), linked as an imported static
    target, from one `cmake --preset` entry point.
  - The XCFramework fallback is not needed. Upstream ask #1 would allow `add_subdirectory`
    later.
  - The proposal column was left as it was.
- **Spike results (2026-11-23)**, a new block after the five questions:
  - A five-row answer table; no question is left open.
  - Time against the 2-day box.
  - What is ready for the Phase 2 tasks, including the stand-ins to replace.
  - The follow-ups: opaque draw for alpha-0 presets (swarm); clamp `dt` after hidden or inactive;
    1080p60 unmeasured (the user's profile is 1080×1920 at 30 fps); the export-list catch; the
    Linux version script; no in-app notice on renderer refusal, and the real Metal renderer not
    exercised.
  - Also: shipping presets under their local-only terms, arm64 only, and re-auditing GL state on
    each core bump.
- The "Spike first" heading now says "Done 2026-11-23" and points to the results.
- **Tidied answers.** No facts were dropped.
  - Session IDs (`S-005`, `S-008`, `SPK-005`, `SPK-006`) were replaced with plain wording, since
    `PLAN.md` is public and the IDs mean nothing outside the session.
  - "set up in the next spike step" now says ExternalProject works with the template's Xcode
    generator.
  - The *Loading GL functions* "OBS load and render check is still pending" now gives the pass
    result.
  - One long line was rewrapped.
- **Risks:**
  - New rows:
    - "the core's CMake can't be embedded": retired;
    - "Xcode 27 / macOS 26 SDK newer than the template": retired;
    - "presets that write alpha 0": found, Phase 2 fix.
  - Updated rows:
    - Metal / non-GL renderer: done in the spike, real Metal untested, no in-app notice;
    - audio handoff: not exercised by the spike;
    - slow preset compiles: the spike still loads synchronously;
    - Linux glad: macOS export list loaded in OBS.
  - The GL state row was already current.
- **Upstream asks:**
  - A status paragraph: #1 and #2 confirmed, #3 optional, none blocks Phase 2, **none filed**.
  - Each becomes `synesthesia-vibe#<n>` once the user files it.
  - A pointer to the issue-ready text file in this session.
  - The inline drafts are kept as they were.

**Time against the box (S-014)**, from `session.log` (UTC):

| Step | Start → end | Agent time |
|---|---|---|
| SPK-001 | 18:26:12 → 18:27:28 | 1 m 16 s |
| SPK-002 | 18:28:04 → 18:34:01 | 5 m 57 s |
| SPK-003 | 18:34:57 → 18:49:01 | 14 m 04 s |
| pause, websocket auth fixed by the user | 18:49 → 00:12 | 5 h 23 m (waiting) |
| SPK-004 | 00:12:16 → 00:19:54 | 7 m 38 s |
| SPK-005 | 00:20:40 → 00:34:15 | 13 m 35 s |
| SPK-006 | 00:35:13 → 00:55:58 | 20 m 45 s |
| SPK-007 | 00:57:05 → 01:03:59 | 6 m 54 s |
| SPK-008 | 01:13:56 → 01:22:07 | 8 m 11 s |

- Totals: about 70 minutes for SPK-001…007 and about 78 minutes with SPK-008, inside a wall-clock
  span of about 7 h. Each question took one pass, with no retry.
- Well inside the 2-day box. Agent time is not a person-day measure, and `PLAN.md` says so.

## Verification
- Install, from the repo root: `ditto` …, `codesign --force --sign -` then `codesign --verify`
  reported "signature ok". `nm -gU` showed the 7 entry points.
- Websocket scripts (session scratchpad, not the repo):
  - `SetCurrentSceneCollection synesthesia-spike` → ok.
  - `SetCurrentProgramScene Scene` → ok.
  - `SetInputSettings preset_path=<Fractopia>` → ok.
  - Two `GetSourceScreenshot` captures → ok, then `GetStats`.
  - Restore: `preset_path` "", program scene `BS-008`, collection `Untitled`. The final
    `GetSceneCollectionList` reports current = `Untitled`.
- Log pass criteria: all met (see above).
- Cleanup:
  - After the bundle was removed, the `plugins/` folder is empty.
  - The new OBS log (`2026-11-23 08-20-26.txt`) has 0 lines containing `synesthesia`, and the
    module is absent from Loaded Modules.
  - `obs-websocket` logged `Module loaded` and `Server started successfully`.
  - A websocket `GetVersion` worked (OBS 33.0.3, obs-websocket 5.7.6).
  - `GetInputKindList` has 16 kinds, without `synesthesia_vibe_visualizer`.
  - The current collection is `Untitled`.
- `PLAN.md`:
  - Grepped for `/Users`, `/private/tmp`, the user name, the port and "password": none.
  - No prose line over 100 columns except the unavoidable session-path line.
- No build was needed: no source changed. There is no existing test suite for this area.

## Evidence
- `evidence/SPK-008/SPK-008-visualizer-t0.png`: the visualizer source on air, Fractopia (green
  phase), 960 px.
- `evidence/SPK-008/SPK-008-visualizer-t4s.png`: the same source 4 s later, a clearly different
  frame (white/violet phase), so it is animating.
- `evidence/SPK-008/SPK-008-scene.png`: the whole `Scene` (portrait canvas, 720 px). The
  visualizer draws with the colour, image and text sources, all correct.
- `evidence/SPK-008/SPK-008-visualizer-off-air.png`: the off-air screenshot that came back black
  (the observation above).
- `evidence/SPK-008/SPK-008-obs-log-load-check.txt`: the trimmed OBS log of the check, home
  directory scrubbed, with the GL error count.
- `evidence/SPK-008/SPK-008-cleanup.txt`: the removal, the relaunch log (Loaded Modules without
  the plugin, websocket started) and the websocket check. The LAN address and port are elided.
- `evidence/SPK-008/SPK-008-plan-diff.md`: the `PLAN.md` diff for this task.
- `evidence/SPK-008/spk-008-upstream-asks.md`: drafts of asks #1, #2 and #3, with titles, bodies
  and measurements. Not filed.

## Known limitations / follow-ups
- **Locking D4 is a judgement call.** I wrote "Locked": the user picked ExternalProject at the
  spike gate (S-001/S-005), and SPK-002/SPK-003 proved it. If the user wants the word "Locked"
  for their own explicit decisions only, change it to "Accepted (spike)". SPK-010 adds a D4
  section to `docs/decisions.md` only if D4 is locked.
- `PLAN.md` line 3 still says "Status: **proposed**, no code yet." That line belongs to BS-010
  (C-013), so I left it.
- **The `synesthesia-spike` collection still exists, for the user to delete.** Its visualizer
  input has no plugin now, so opening that collection may show a missing-source warning.
  `Untitled` is current.
- The S-012 cleanup removed only the bundle. `~/Library/Application Support/obs-studio/plugins/`
  (created by the spike) is left, empty. `plugin/build_*` and `plugin/.deps/` are kept.
- Spike instrumentation that SPK-009 should settle:
  - the 10 s `frame …` debug line at `LOG_INFO`;
  - the synthetic tone;
  - the `SPK-005 Image` source in the spike collection, which points into the session
    scratchpad.
- The user should turn obs-websocket authentication back on, and rotate the password shared in
  chat (gate note).
- 1080p60, a real Metal renderer run and a scene collection loaded under refusal are still
  unverified. They are listed in `PLAN.md`.
- Doc gaps for document-writer (SPK-010):
  - D4 in `docs/decisions.md`.
  - The spike follow-ups in `docs/concerns.md`.
  - The local install, re-sign and cleanup steps in `docs/integrations.md`.
- No test risk worth flagging; nothing in the source changed.

## Handoff for downstream review
- Acceptance criteria addressed:
  - Each of the five "Spike first" bullets has its answer, plus a summary table.
  - D4 is updated.
  - Risks are updated.
  - *Upstream asks* is marked confirmed/optional, with draft text in the session dir. Nothing
    was filed.
  - Time spent against the 2-day box is recorded.
  - What is ready for the Phase 2 tasks is listed.
  - The `git diff PLAN.md` excerpt is in the evidence.
- Areas to scrutinize:
  - Whether the tidied answers in `PLAN.md` kept every fact. Compare with the diff.
  - The D4 wording.
  - Whether a repo path into `.claude/working-sessions/` from `PLAN.md` is acceptable if the
    session dir is not committed.
- How to re-run the load check: the commands in `output-SPK-007.md` *Known limitations*. The
  plugin must be reinstalled first, because SPK-008 removed it.
