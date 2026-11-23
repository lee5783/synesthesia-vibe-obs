# Documentation sync report — OBS retest (2026-11-23)

## Summary

The OBS checks that had been deferred all ran on 2026-11-23 and passed: the P2-011 Done-when run
(which included the TC-M03 macOS smoke load), the P2-012 1080p60 run and the P2-010b dialog check.
The `docs/` pages and the `.claude/` lazy caches no longer call these checks deferred or planned.
They now record the measured numbers from the four evidence files.

These items are still listed as open: notarization, TC-M04 / P2-013 (pkg install into OBS),
P4-008 (Linux in OBS) and the Phase 1 loopback checklist. Projectors, multiview and filters are
also listed as open with low priority. The P2-011 report does not show them being exercised, and
the old *Deferred checks* list had included them.

Target: `docs/` and `.claude/` lazy caches only. `PLAN.md`, `CLAUDE.md`, the root `README.md`,
`plugin/README.md`, evidence files and source were not touched.

## Files updated

- `docs/README.md`: the intro now says the Phase 2 OBS checks passed on 2026-11-23, with no notarization and the pkg install unverified. The `testing.md` / `concerns.md` Covers cells are updated.
- `docs/testing.md`: the "Planned, not yet run" block is now *Done-when run, 2026-11-23*, with the P2-011 / P2-012 / P2-010b results and numbers, plus a *Still open* line. The audio-levels patcher now points at `P2-011-add-audio-debug.py`, with a note that the P2-004 script's `load_pending_preset` anchor is gone. The GL-audit paragraph says the integrated check passed.
- `docs/concerns.md`:
  - The intro and the *Fixed in Phase 2* lead-in now say the checks passed.
  - The dialog-rewrite triggers note that P2-010b is confirmed.
  - The commit-time concern keeps the 17.8 ms fact and adds the 1080p60 result (20.7 ms slowest commit, 1 skipped frame in 17 446). The improvement path is lowered to low priority.
  - The render-cost section replaces "1080p60 is unmeasured" with 3.75–8.60 ms (median 4.69).
  - The C-029 note is updated.
  - The `gl_state_guard` fragile entry notes the integrated pass.
  - New low-priority fragile note: the unattributed renderSkippedFrames +13 across the profile switch back plus six create/remove cycles.
  - *Deferred Checks* is replaced by *Open Checks*: P2-013/TC-M04, notarization, P4-008, the Phase 1 loopback checklist, and projectors/multiview/filters.
  - The *Test Coverage Gaps* plugin list drops the deferred items and the TC-M03 line.
- `docs/integrations.md`: the cross-reference now points at *Open checks* instead of *Deferred checks*.
- `.claude/common-issues/plugin-runtime.md`: the integrated GL check on the final build passed in P2-011.
- `.claude/common-issues/verification-tooling.md`: a folder-only source is now confirmed safe (P2-010b). Removing a source while its dialog is dirty for any other reason still wedges OBS. "How to verify" now points at the completed check.
- `.claude/how-x-works/plugin-preset-loading.md`: the commit-cost line adds the 1080p60 result (20.7 ms, 1 skipped frame in 17 446).
- `.claude/working-sessions/20261123145422-phase-4-release-linux/docs/input-docs-obs-retest.md`: appended *Skills (using)* and *Rules (read)*. No authoring skill fits a targeted sync, so none was invoked.

## Index updates

- `docs/README.md` (Covers cells for `testing.md` and `concerns.md`). No new pages or entries, so
  `.claude/COMMON-ISSUES.md` and the other indexes are unchanged.

## Proposed PLAN.md wording (for the user to approve; not applied)

**Status paragraph (lines ~3–12)** — replace the Phase 2 sentences
("**Phase 2 (macOS) built and reviewed** … dialog fix).") with:

> **Phase 2 (macOS) built, reviewed and checked in OBS**: tasks 1–6 are done, and its *Done when*
> checks passed on 2026-11-23 (the 1080p60 run, the integrated Done-when run and the OBS check of
> the "(first preset)" dialog fix). Task 7 (packaging) is partly done, with an ad-hoc signed
> `.plugin`, an unsigned `.pkg` and a zip. Notarization waits on Developer ID credentials and the
> pkg install into OBS is not verified yet, so Phase 2 is not closed.

(The rest of the paragraph, from "**Phase 4 (release and Linux) built and reviewed**" on, is
unchanged.)

**Done-when state block (lines ~452–459)** — replace with:

> **Done-when state (2026-11-23):** met on the final integrated build (production build of
> `4c260b5`, OBS 33.0.3, Apple Silicon). Evidence is under the Phase 2 session's
> `develop/evidence/P2-011/`, `P2-012/` and `P2-010b/`.
>
> - **Passed:** the 1080p60 measurement in a dedicated profile (P2-012). `activeFps` was 60 in all
>   30 samples and `averageFrameRenderTime` was 3.75–8.60 ms (median 4.69 ms). 1 frame was
>   skipped in 17 446, and the slowest of 24 commits took 20.7 ms. Render scale reduction was not
>   needed.
> - **Passed:** the Done-when run on the integrated build (P2-011). It covered the GL-state check,
>   both audio paths, live settings, removal and quit (`Number of memory leaks: 0` on every quit)
>   and a 300 s race stress test, with 0 GL error lines.
> - **Passed:** the OBS check of the "(first preset)" fix (P2-010b). Opening the properties dialog
>   on a folder-only source no longer saves `preset`.
> - **Still open for Phase 2:** notarization (Developer ID credentials) and the pkg install into
>   OBS (TC-M04). Projectors, multiview and filters were not exercised.

## Follow-ups / gaps

- `.claude/common-issues/plugin-build.md` (~96) still says "deferred with the pkg install check".
  That is still true, because TC-M04 is open, so it was left as is.
- `PLAN.md` line ~442 ("The pkg install into OBS is not verified yet.") is still accurate.
- The `skipped frames` observation has no owner. Attributing it would need a profile switch and
  the create/remove cycles measured separately.

## Follow-up (orchestrator, same day)

The user approved the proposed `PLAN.md` text, which is now applied. A follow-up session C then ran
the projector, multiview and filter checks, and all passed (`P2-011-done-when.md`, *Session C*).
The orchestrator removed that item from *Open Checks* in `docs/concerns.md`, from `docs/testing.md`
and from `PLAN.md`.
