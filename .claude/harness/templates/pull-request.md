<!--
Filled by the orchestrator at the end of a vibe-flow run and written to
`.claude/working-sessions/<run>/output-pull-request.md`. The user opens the PR and pastes it.
Every box must reflect something that actually happened — leave a box unchecked with a
one-line reason rather than checking it optimistically. Delete sections that do not apply.
-->

## Summary

<1–3 sentences: what changed and why. From `output-plan-summary.md`.>

**Ticket:** <ISSUE-KEY or "none">
**Design:** <design tool link, or "none">

## Changes

- `path/to/file` — <what changed>

<Grouped by area if the diff is large. From the `output-TASK-*.md` handoffs.>

## Verification

<!-- Most projects default to "no unit tests unless asked" per root CLAUDE.md — don't add a
     "tests added" line unless tests were actually requested and written. -->

- [ ] Builds clean: `cmake --build --preset <preset>` (see `docs/integrations.md`), or N/A for a Browser-Source-only change
- [ ] Existing tests over the touched area still pass, or: <none exist for this area>
- Tests added: <none — not requested | `path/to/file`, wired into CTest>

**Evidence** — from `develop/evidence/`

<!-- Attach the captures to the PR. -->

| What it shows | Capture |
|---|---|
| <state / flow> | `<PLAN_TASK_ID>-<what>.png` |

<For non-UI work, a trimmed log/test-output excerpt showing the new path executing.>

**Manual QA** — from `output-plan-test-cases.md`

| Screen / component | Precondition | Expected |
|---|---|---|
| <TC-001> | … | … |

## Review

- [ ] Reviewed by the `code-reviewer` pass — decision: <Approve / Request changes>
- [ ] Must-fix findings addressed, or listed under Follow-ups below
- **Areas to scrutinize:** <from the developer handoffs>

## Documentation

- [ ] `docs/**` updated, or: <N/A — no behavior change>
- [ ] New gotcha / term / flow recorded in `COMMON-ISSUES` / `DICTIONARY` / `HOW-X-WORKS` / `MEMORY`, or: <N/A>
- Root `README.md` / `./docs/`: <N/A — not requested> <!-- only if the user explicitly asked -->

## Risks and follow-ups

- <Known limitations from the handoffs, unresolved concerns, tech debt accepted>

## Checklist

- [ ] No secrets, tokens or credential files in the diff
- [ ] No AI-session-only files staged unintentionally (session artifacts under `.claude/working-sessions/` are usually meant to be committed — check your project's convention)
- [ ] New files carry the standard header, if this project uses one
- [ ] No unrequested code comments added — naming and structure carry the meaning
- [ ] Style matches the sibling components it sits next to
- [ ] Shared/design-system components reused or extended rather than duplicated, where applicable

---
🤖 Built with an AI vibe-flow run — session `<yyyyMMddHHmmss>-<slug>`
