# Code review input — P4-011 (Gate 2)

## Assignment

PR-style review of the full Phase 4 diff,
`git diff feature/plugin-phase-2-macos...feature/phase-4-release-linux` (draft PR #3, stacked on
PR #2), against the plan, the Gate 1 decisions (which override the plan) and the evidence. Do not
fix anything. Report the verdict, the blocking findings and the top non-blocking findings.

Focus (P4-011, TC-C05): workflow security (SHA pins, least privilege, no `secrets.*`, script
injection, fork PRs, cache poisoning); release gating per C-016/C-017 (draft, prerelease, final-tag
variable, tag ↔ buildspec, no `.deb`, the README vs notes `shasum` wording); `plugin/CMakeLists.txt`
(guard, `OS_MACOS` split, Linux PIC, `LINK_GROUP:RESCAN`, export map vs list, `--exclude-libs,ALL`,
`CPACK_PACKAGE_CONTACT`, CI build number); the per-OS GL loader; `plugin/cmake` and `external`
untouched; CLAUDE.md conventions (no comments, headers incl. YAML and `.map`, kebab-case, sibling
style); public-repo hygiene; evidence vs claims. Linux untested in OBS (P4-008) and the deferred
TC-M03 (C-024) are not defects; missing unit tests are not a defect.

## Skills (using)

- None beyond the role file. The delegation names none; `cpp-coding-standards` is manual-only and
  was not invoked by the user.

## Rules (read)

- `.claude/harness/code-reviewer.md` (role file, via the agent definition)
- Root `CLAUDE.md`
- `docs/conventions.md`, `docs/structure.md`, `docs/integrations.md`
- `.claude/common-issues/plugin-build.md`

## Artifacts to read

- `inputs/requirements.md`
- `plan/output-plan-summary.md`, `output-plan-tasks.md`, `output-plan-test-cases.md`,
  `output-plan-concerns.md`, `output-plan-gate-1-decisions.md`
- `develop/input-dev.md`, `develop/output-P4-001.md` … `output-P4-007.md` (incl. CI iterations)
- `develop/evidence/*.md` and `develop/evidence/P4-002-workflow-step.py`
- `session.log` (CI runs 1–5, release test run 45146667649)
- The diff itself; CodeGraph for source call sites; read-only `gh run view` where a claim needs it

## Outputs

- `review/output-review-findings.md`
