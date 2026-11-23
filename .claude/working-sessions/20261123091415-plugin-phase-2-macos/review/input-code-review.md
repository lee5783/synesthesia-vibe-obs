# Input: code review (P2-015, Gate 2)

## Task (verbatim)

Review all Phase 2 work in this run (P2-001 … P2-010b, P2-013, P2-014) against the plan,
decisions and evidence. P2-011 (Done-when run), P2-012 (1080p60) and the OBS check of P2-010b
were DEFERRED by the user because OBS is stuck at a startup prompt — note what that leaves
unverified; it is not a defect of the code, but say whether anything in the code looks risky
enough that those checks matter more.

## Session

- Root: `.claude/working-sessions/20261123091415-plugin-phase-2-macos/`
- Step directory: `review/`
- Branch: `feature/plugin-phase-2-macos`, uncommitted, compared with `master`.

## Skills (using)

- `shader-programming` (loaded as delegated; generic cross-engine shader material, applied to the
  GL guard, resize, commit, opaque draw and sRGB checks)
- Not loaded, per delegation: `cpp-coding-standards`, `cpp-testing` (manual-only)

## Rules (read)

- Root `CLAUDE.md` (no new code comments, sibling style, file header, no unit tests unless asked,
  one `sv_context` single-threaded with GL current, public repo hygiene)
- `docs/architecture.md`
- `docs/concerns.md`
- `.claude/common-issues/plugin-runtime.md`
- `.claude/harness` code-reviewer role (this agent's instructions)

All delegated rule paths exist; none missing.

## Artifacts to read

- `inputs/requirements.md`
- `plan/output-plan-summary.md`, `output-plan-tasks.md`, `output-plan-test-cases.md`,
  `output-plan-concerns.md`, `output-plan-gate-1-decisions.md` (with the mid-develop section)
- `develop/output-P2-001.md` … `output-P2-010b.md`, `output-P2-013.md`, `output-P2-014.md`
- `develop/evidence/**/*.md` reports (plus spot-checked screenshots)
- `session.log`
- Change set: `git diff master` (`plugin/CMakeLists.txt`, `plugin/src/visualizer-source.cpp`,
  `plugin/data/locale/en-US.ini`, `.claude/COMMON-ISSUES.md`,
  `.claude/common-issues/verification-tooling.md`, previous run's `session.log`) and untracked
  `plugin/src/audio-ring.{hpp,cpp}`, `plugin/packaging/macos/distribution.in`, `plugin/README.md`,
  the session directory.
- `plugin/cmake/**` must be unchanged against `master`.

## Focus (from the delegation)

Threading and lifetime (sv_* thread rule, tick atomics, audio callbacks, attach/detach ordering,
worker vs `sv_destroy`, prepared-handle ownership, latch bookkeeping, hotkeys), ring buffer
correctness, GL guard coverage, properties and the P2-010b fix, CMake guard and distribution
override, README accuracy, public-repo hygiene.

## Output

`review/output-review-findings.md`: blocker / should-fix / nit with `file:line`, a must-fix list
before docs, and which deferred OBS checks matter most. No fixes.
