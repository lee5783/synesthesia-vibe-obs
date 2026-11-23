# Doc sync packet — OBS retest (2026-11-23)

The deferred OBS checks ran and **all passed**. Update the corpus so nothing still calls them
deferred or planned, and record the new measurements.

## Sources of truth (read these; do not re-derive)

- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-011/P2-011-done-when.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-012/P2-012-1080p60.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/P2-010b/P2-010b-obs-check.md`
- `.claude/working-sessions/20261123145422-phase-4-release-linux/develop/evidence/P4-003-obs-smoke.md`

## Places known to mention the deferred checks

- `docs/testing.md` ~207-222 ("Planned, not yet run" block: P2-011, P2-012, P2-010b).
- `docs/concerns.md` lines ~6, 15, 179, 281, 296, 311, the *Deferred checks* section (~448-470)
  and ~525-528 (TC-M03 deferred with P2-011).
- `docs/README.md` line 4 (intro: "three OBS checks deferred") and the `testing.md` /
  `concerns.md` Covers cells (lines 19-20).
- `.claude/common-issues/plugin-runtime.md` ~46 and `.claude/common-issues/verification-tooling.md`
  ~48-64 (the wedge entry: a folder-only source is now confirmed safe after P2-010b; removing a
  source while its dialog is open and dirty for other reasons still wedges OBS).
- Grep the corpus for `P2-011`, `P2-012`, `P2-010b`, `TC-M03`, `deferred`, `1080p60 is
  unmeasured` to catch any others.

## Facts to record (from the evidence)

- 1080p60 (P2-012): activeFps 60 in all 30 samples; averageFrameRenderTime 3.75-8.60 ms
  (median 4.69); 1 skipped frame in 17 446; 24 commits in the window, slowest 20.7 ms; render
  scale reduction not needed. This resolves/relaxes the "17.8 ms commit over the 60 fps budget"
  concern — keep the commit-time fact, update the risk.
- Unattributed observation: renderSkippedFrames +13 across the profile switch back plus six
  create/remove cycles (create holds the graphics lock for sv_init_gl/sv_create). Record it as a
  low-priority fragile-area note, not a bug.
- The audio-debug patch script from P2-004 no longer applies (anchor `load_pending_preset`
  renamed); the adapted copy is `P2-011-add-audio-debug.py`. Update any doc that tells readers to
  use the old script.
- Still NOT done, keep listed as open: notarization (Developer ID), TC-M04 (CI pkg install into
  OBS), P4-008 (Linux in OBS on X11/Wayland), Phase 1 loopback checklist.

## Rules

- Log to `.claude/working-sessions/20261123145422-phase-4-release-linux/session.log` before your
  first read: `printf '%s  document-writer  <msg>\n' "$(date -u +%FT%TZ)" >> <that file>`.
- Edit only `docs/` and the `.claude/` lazy caches (common-issues, how-x-works, dictionary).
  **Do not edit `PLAN.md`, `CLAUDE.md`, the root `README.md` or `plugin/README.md`.** Instead put
  proposed `PLAN.md` wording (status line ~7 and the Done-when block ~452-459) in your report for
  the user to approve.
- Do not edit evidence files or source code. No machine paths, secrets or personal emails.
- Keep each change minimal and factual; match the surrounding style.
- Write the report to
  `.claude/working-sessions/20261123145422-phase-4-release-linux/docs/output-doc-sync-obs-retest.md`:
  files changed (one line each), and the proposed PLAN.md text.

## Skills (using)

- None invoked. This is a targeted factual sync of existing pages; `codebase-mapper` (full page
  generation) and `project-documentation` (README / feature-doc hierarchy) do not fit and the
  packet names no skill. Exception recorded by the document-writer.

## Rules (read)

- `CLAUDE.md` (root), `.claude/agents` document-writer role, this packet, and the four evidence
  files listed above.
