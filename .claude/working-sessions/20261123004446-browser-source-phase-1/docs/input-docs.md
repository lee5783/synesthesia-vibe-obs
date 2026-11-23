# Input — document-writer (Gate 3)

**Plan rows:** LIC-002, BS-010, SPK-010 (one pass).
**Session root:** `.claude/working-sessions/20261123004446-browser-source-phase-1/`
**Step directory:** `<root>/docs/`

## Task (verbatim)

> sync the docs with the shipped, reviewed behaviour of this run — Phase 1 Browser Source
> (browser-source/), LIC-001 (GPL-2.0-or-later), and the Phase 2 macOS spike (plugin/,
> external/synesthesia-vibe submodule).

## Write targets

- `docs/**`: `stack.md`, `integrations.md`, `architecture.md`, `structure.md`, `conventions.md`,
  `testing.md`, `concerns.md` (codebase-mapper templates), `decisions.md` (D1, D4), `README.md`
  (list exactly the pages that exist).
- `.claude/COMMON-ISSUES.md` + `.claude/common-issues/<area>.md` for the run's gotchas.
- Optional, small: `.claude/HOW-X-WORKS.md` + `.claude/how-x-works/<flow>.md`.
- **User-approved outside `docs/`** (pre-docs approvals, C-013, in
  `plan/output-plan-gate-1-decisions.md`):
  1. root `CLAUDE.md`: routing table / "docs pages do not exist yet" paragraph, *Setup* section,
     stale CodeGraph sentence. Minimal edits.
  2. `PLAN.md`: line 3 status, R2-N1, R2-N2, Pages (D5) still open/skipped.
  3. `.gitignore`: add `.vscode/`.
- Not allowed: root `README.md`, application source.
- Session artifacts: this file and `output-doc-sync-report.md`.

## Skills (using)

- `.claude/harness/document-writer.md` (own role file).
- `codebase-mapper` (`.claude/skills/codebase-mapper/SKILL.md` + all seven `templates/*.md`
  read before filling each page).
- `project-documentation` (`.claude/skills/project-documentation/SKILL.md`), for doc shape only.
  The skill's `features` / `readme` / `comments` outputs are not produced: no
  `docs/features/**`, no root README, no code comments.
- Exception recorded: codebase-mapper's *README link rule* step 3 (add a link to the root
  `README.md`) is **not** applied, because the packet forbids touching the root `README.md`.
  Its exploration exclusion for `.claude/` is relaxed only for the session artifacts the packet
  names as inputs.

## Rules (read)

- Root `CLAUDE.md` (kebab-case names, one page per question, public-repo hygiene: no absolute
  paths, user names, local folder names or secrets; SPK-006 `GL_TEXTURE7` caveat wherever that
  evidence is cited).
- `PLAN.md` (full).
- `<root>/plan/output-plan-tasks.md` (LIC-002, BS-010, SPK-010 rows and sections).
- `<root>/plan/output-plan-gate-1-decisions.md` (incl. mid-develop `cycle` decision, pre-docs
  approvals).
- `<root>/plan/output-plan-gate-1-spike-decisions.md` (incl. websocket, GUI-fallback and D4
  sections).
- `<root>/plan/output-plan-concerns.md` (C-021).
- `<root>/review/output-review-findings.md` (round 1 and round 2, incl. *Remaining concerns for
  the docs pass*).
- Every developer handoff `<root>/develop/output-*.md` (doc-gap sections in full; selected
  evidence reports for the gotchas).
- `browser-source/README.md`, `browser-source/index.html`, `browser-source/obs-page.js`,
  `browser-source/fetch-dist.sh`, `plugin/CMakeLists.txt`, `plugin/CMakePresets.json`,
  `plugin/buildspec.json`, `plugin/src/*`, `plugin/exported-symbols.txt`, `.gitmodules`,
  `.gitignore`.

## Gate 0 (CodeGraph)

CodeGraph is configured and reachable (MCP `codegraph_explore`). The index now holds the plugin
C++ (`plugin/src/visualizer-source.cpp`) and the core submodule sources.
