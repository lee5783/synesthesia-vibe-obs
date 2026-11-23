# Input: document-writer (Gate 3, P2-016)

## Delegation envelope

- **Task (verbatim):** sync the docs with the shipped, reviewed Phase 2 plugin work (P2-001 …
  P2-010b, P2-013, P2-014). P2-011, P2-012 and the P2-010b OBS check were deferred by the user —
  the docs must say so wherever they'd otherwise claim verification.
- **Session root:** `.claude/working-sessions/20261123091415-plugin-phase-2-macos/`
- **Step directory:** `<root>/docs/`
- **Target:** `docs/` (default). Plus `.claude/how-x-works/`, `.claude/common-issues/` and their
  indexes. `PLAN.md` only for the two user-approved edits (Phase 2 status and task states;
  upstream ask #5 drafted, not filed). Root `README.md`, `plugin/README.md` and all source code
  are off-limits.

## Skills (using)

- `document-writer` role (`.claude/harness/document-writer.md`, this agent's instructions).
- `codebase-mapper` (`.claude/skills/codebase-mapper/SKILL.md`): template shape for the seven
  `docs/` pages, current-state-only, prescriptive, file paths in backticks. Used as a sync
  reference, not a fresh mapping run. Its "add a `README.md` link" rule is not applied: the root
  `README.md` is off-limits this turn.
- `project-documentation` (`.claude/skills/project-documentation/SKILL.md`): shape only. No
  `docs/features/**` tree is created (none exists; not requested).

## Rules (read)

- Root `CLAUDE.md`: kebab-case names, one page per question, public repo (no absolute paths,
  usernames, local folder names, identities), current state only.
- `<root>/plan/output-plan-tasks.md` (P2-016 row and detail, P2-011/P2-012 detail).
- `<root>/plan/output-plan-gate-1-decisions.md` (all sections, including the mid-develop and
  post-review decisions and the `PLAN.md` scope).
- `<root>/plan/output-plan-concerns.md` (C-006, C-011, C-012, C-024).
- `<root>/review/output-review-findings.md` (S1/S2, N1–N9, deferred-checks ranking).
- Developer handoffs `<root>/develop/output-P2-001.md` … `output-P2-014.md`,
  `output-P2-010b.md`, `output-REVIEW-FIX.md`.
- `plugin/README.md` (read only), `PLAN.md`, `docs/*.md`, `.claude/COMMON-ISSUES.md` and
  `common-issues/*`, `.claude/HOW-X-WORKS.md` and `how-x-works/*`.
- Source for accuracy (read only): `plugin/src/visualizer-source.cpp`, `plugin/src/audio-ring.*`,
  `plugin/CMakeLists.txt`, `plugin/data/locale/en-US.ini`, `plugin/packaging/macos/`.

## Code intelligence (Gate 0)

CodeGraph is configured and reachable (`codegraph_explore` answered for the render-path symbols
of `plugin/src/visualizer-source.cpp`). The remaining ranges of that file were read directly for
line citations.

## Exceptions

- None to the skill list. The codebase-mapper root-README rule is skipped (see above).
