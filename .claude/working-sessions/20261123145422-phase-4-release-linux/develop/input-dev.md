# Developer input — Phase 4 (release and Linux)

## Assignment

Plan tasks P4-001, P4-002, P4-003, P4-004, P4-005 and P4-006 from `plan/output-plan-tasks.md`,
adjusted by `plan/output-plan-gate-1-decisions.md` (Gate 1 overrides the plan where they differ).
Done in that order. One handoff per task: `output-P4-001.md` … `output-P4-006.md`.

## Hard rules from the delegation

- No commit, push, tag or PR. Finish with a "ready to push" handoff (file list, workflow and job
  names to watch). Read-only `gh api` allowed.
- Do not drive OBS (C-024). macOS evidence: clean local rebuild, `nm -gU` = the 7 `obs_module_*`,
  `otool -L` unchanged versus a "before" captured first. Keep `plugin/.deps` and avoid the
  release/Release case-insensitive collision.
- `plugin/cmake/**` and the core submodule never change. A needed core change stops Linux (C-013)
  and becomes a drafted upstream ask.
- No comments (YAML included), SHA-pinned `uses:` with no trailing version comments (map goes in
  the handoff), standard header on new files only where sibling files of that type carry one,
  kebab-case names.
- Public repo: no secrets, no `secrets.*`, no machine paths, no personal email. `.deb` contact
  `lee5783 <lee5783@users.noreply.github.com>`.
- Workflows validated with `actionlint`. Release: always `--draft`, `--prerelease` for
  `-beta.N`/`-rc.N`, plain `vX.Y.Z` refused unless `vars.RELEASE_FINAL_ALLOWED == 'true'`, no
  Linux `.deb` on the release (artifact only), `SHA256SUMS.txt` for macOS assets, notes link this
  tag and core `v0.7.0`.
- Linux cannot be compiled locally (no Docker): CI is the first real compile. Checks fail loudly.

## Skills (using)

- None loaded. The delegation names no extra skill. `shader-programming` is not needed: the loader
  split moves the GL include and the get-proc function into per-OS files and does not change any GL
  state, render target or shader. `cpp-coding-standards` / `cpp-testing` are manual-only and were not
  invoked.

## Rules (read)

- `.claude/harness/developer.md` (role file, via the agent definition)
- Root `CLAUDE.md`
- `docs/structure.md`, `docs/conventions.md`, `docs/integrations.md`, `docs/stack.md`
- `.claude/common-issues/plugin-build.md`, `.claude/common-issues/verification-tooling.md`
- `.claude/harness/HARNESS.md` (stuck-capture section)
- Session: `inputs/requirements.md`; `plan/output-plan-summary.md`, `output-plan-tasks.md`,
  `output-plan-test-cases.md`, `output-plan-concerns.md`, `output-plan-gate-1-decisions.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/output-P2-013.md` (macOS
  packaging and inspection checks reused in CI)
- Reference only: obs-plugintemplate `.github/` at `3e7d7ac3` (read through `gh api`)
