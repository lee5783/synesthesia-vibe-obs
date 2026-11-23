# Input — document-writer (P4-012, Gate 3)

## Task (verbatim from the orchestrator)

> Sync the docs with the Phase 4 work that was shipped and reviewed. The Gate 2 review approved it
> with no blockers, and the review fixes are in commit f57bf4e.

## Skills (using)

- None beyond the role file. The orchestrator listed "Skills to load beyond your own instructions:
  none". This project has no separate diff-only sync skill; `codebase-mapper` and
  `project-documentation` are excluded by the packet (C-021 closed the mapper task, and this is a
  catch-up of existing pages, not a regeneration).

## Rules (read)

- Root `CLAUDE.md`: doc routing table, *Where documentation goes*, naming rules, public-repo
  hygiene.
- CodeGraph used first for source questions (loader files, `obs_module_load`).

## Inputs read

- `inputs/requirements.md`
- `plan/output-plan-tasks.md` (P4-012 row), `plan/output-plan-concerns.md`,
  `plan/output-plan-gate-1-decisions.md`
- `develop/output-P4-001.md` … `output-P4-007.md`, `develop/output-REVIEW-FIX.md`
- `review/output-review-findings.md` ("Documentation hand-off to document-writer (P4-012)" is the
  checklist)
- Shipped files: `.github/workflows/build-plugin.yaml`, `ci.yaml`, `release.yaml`,
  `plugin/CMakeLists.txt`, `plugin/exported-symbols.map`, `plugin/exported-symbols.txt`,
  `plugin/src/opengl-loader.hpp`, `opengl-loader-macos.cpp`, `opengl-loader-linux.cpp`,
  `plugin/README.md` (read only), `plugin/CMakePresets.json` (read only), `.gitignore`

## Approved write scope (Gate 1 C-022 plus accepted defaults)

- `docs/integrations.md`, `docs/stack.md`, `docs/structure.md`, `docs/architecture.md`,
  `docs/conventions.md`, `docs/testing.md`, `docs/concerns.md`; `docs/decisions.md` and
  `docs/README.md` only if needed.
- `.claude/common-issues/plugin-build.md` (+ new entries for the PPA 503 and GNU ld archive-order
  traps), `.claude/how-x-works/release-pipeline.md` + `.claude/HOW-X-WORKS.md`,
  `.claude/DICTIONARY.md` only for new jargon.
- `PLAN.md`: status line, Phase 4 item states, the "Duplicate glad symbols on Linux" risk row.
  Nothing else.
- `CLAUDE.md` *Setup*: one Linux build line; the evidence quick fact only if now wrong.
- Never: root `README.md`, any source or workflow file, `plugin/README.md`.

## Output

- Doc edits per the scope above.
- `output-doc-sync-report.md` in this folder.
