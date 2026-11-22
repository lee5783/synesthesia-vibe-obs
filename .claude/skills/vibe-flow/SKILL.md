---
name: vibe-flow
description: Run vibe-flow — the multi-agent delegation workflow — on a task, from intake through planning, implementation, review, docs and a ready-to-paste PR body. Use when a task is multi-step, touches shared code, carries a ticket or a design, or will be reviewed by someone else.
---

# Skill: vibe-flow

You are acting as the **orchestrator** for a vibe-flow run. The full workflow definition
lives in **[`../../harness/VIBE-FLOW.md`](../../harness/VIBE-FLOW.md)** — read it in
full before proceeding; this file is a thin, invokable entry point into that document, not a
replacement for it. The roles it orchestrates are defined in
**[`../../harness/HARNESS.md`](../../harness/HARNESS.md)**.

## Argument forms

- **`<task description, ticket key, or design URL>`** — same as `full` below: start a new run and
  walk it end to end.
- **`full <task description, ticket key, or design URL>`** — run the whole workflow end to end,
  from intake through the Gate 4 PR-template confirm, stopping only at gates that need the user.
- **`full resume <session path>`** — pick up an existing run at its first step with no output yet,
  then keep walking end to end (no early stop).
- **`plan <task description, ticket key, or design URL>`** — start a new run but **stop once the
  plan step finishes** (through Gate 1 concerns resolution), before any `develop` work begins.
  Always creates a new session; this is the only single-step form that does not take a session
  path.
- **`develop <session path>`** — continue an existing session and **stop once the develop step
  finishes** (Gate 1.5 evidence satisfied), before `code-reviewer` runs. Requires a session that
  already has plan output; error out with what's missing rather than inventing a plan.
- **`review <session path>`** — continue an existing session and **stop once the review step
  finishes**, before `document-writer` runs. Requires a session with develop output + evidence
  already present.
- **`document <session path>`** — continue an existing session and **stop once the document step
  finishes**, before the Gate 4 PR confirmation. Requires a session with review output already
  present.
- **`pr <session path>`** — continue an existing session, run the Gate 4 confirm, and (on yes)
  write the PR template — the same finish `full` ends with. Requires a session with docs output
  already present.
- **`resume <session path>`** — pick up an existing run at its first step with no output yet and
  keep walking end to end. Equivalent to `full resume <session path>`; kept as a shorthand.
- **`step <role> <task>`** — run a single specialist with no session directory and no gates; the
  cheap path for one opinion instead of a whole run.

**If invoked with no argument**, ask what the task is and stop. Do not invent one.

## What to do

**Every form below still writes to `session.log`, honours the gate it runs through, and reports
in one or two lines what landed and what's next — a single-step form is a scoped slice of the
same run, not a different mechanism.**

**`plan`** — bootstrap the run exactly as *Otherwise, start a new run* below describes, walk the
flow through the planner and Gate 1, then stop and hand the user the session path. Do not start
`develop`.

**`develop <session path>`** — read `session.log` and every `output-*.md` already in the session.
If there is no `plan/output-plan-*.md`, stop and tell the user to run `plan` first — do not
synthesize a plan. Otherwise run the develop loop (per plan task: implement, capture evidence)
until every plan task is done, then stop before `code-reviewer`.

**`review <session path>`** — read the session state; if `develop/evidence/` or the task handoffs
are missing, stop and say so. Otherwise run `code-reviewer` per Gate 2, then stop before
`document-writer`.

**`document <session path>`** — read the session state; if `review/output-review-findings.md` is
missing, stop and say so. Otherwise run `document-writer` per Gate 3, then stop before Gate 4.

**`pr <session path>`** — read the session state; if `docs/output-doc-sync-report.md` is missing,
stop and say so. Otherwise run Gate 4 (ask the user to confirm) and, on yes, write
`output-pull-request.md` as step 5 below describes; on no, end the run with nothing written.

**If the request starts with `resume` or `full resume`**, take the session path that follows, read
its `session.log` and every `output-*.md` it already has, report where the run stands, and
continue walking the flow end to end from the first step with no output. Do not redo completed
steps.

**If the request starts with `step`**, run only the named role (from `.claude/agents/` /
`harness/`) on the rest of the request — a single specialist, no session directory, no gates — and
report in-thread.

**Otherwise (bare task, or `full <task>`), start a new run and walk it end to end:**

1. **Bootstrap.** Pick a short kebab-case slug from the request. Create the run and log it:

   ```bash
   RUN=".claude/working-sessions/$(date +%Y%m%d%H%M%S)-<slug>"
   mkdir -p "$RUN"/{inputs,intake,analyze,plan,develop/evidence,review,docs}
   printf '%s  orchestrator  session start: %s\n' "$(date -u +%FT%TZ)" "<request>" >> "$RUN/session.log"
   ```

   Tell the user the path before going further.

2. **Write `inputs/requirements.md`** — the request restated as scope, in the user's own words
   where you can. Do not embellish it into requirements they did not give.

3. **Walk the flow** in `VIBE-FLOW.md`, delegating each step using the delegation-packet
   shape it defines (see `HARNESS.md`'s *The delegation contract*). Only run the steps this task
   actually needs — a ticket key means intake (if `ticket-tracker-assistant` is in use), a design
   reference means `ui-ux-analyzer` (if in use), shared or risky code means
   `dependency-impact-analyzer`. Skip what does not apply and say which steps you skipped.

4. **Honour every gate.** They are stop points, not formalities:
   - **Gate 0** — this project's code-intelligence tool (if configured) before any bulk source
     reading. If one is configured and will not connect, stop and tell the user.
   - **Gate 1** — material open concerns go back to the **user** before development starts. Never
     resolve them yourself.
   - **Gate 1.5** — no task is complete without evidence in `develop/evidence/`.
   - **Gates 2 and 3** — review, then docs. In that order, never concurrently.
   - **Gate 4** — after docs, ask the user whether to write the PR template. A real question,
     asked and answered — not a pause before writing it anyway.

5. **Finish.** On yes at Gate 4, write `output-pull-request.md` from
   `.claude/harness/templates/pull-request.md`, filled from the run's artifacts, and hand the user
   the path. On no, end the run there — no template, nothing further. **Do not create, push or
   merge anything** either way — the user always opens the PR themselves.

## Standing rules for this run

- Keep a visible task/todo list mirroring the plan tasks so the user can see progress, if your
  tool supports one.
- Every agent logs to `session.log` before its first read — put the log command in each packet.
- Between steps, report in one or two lines what landed and what is next. No transcript dumps.
- The developer follows root `CLAUDE.md`'s editing conventions (no unrequested code comments,
  sibling-matched style, no unit tests unless asked) and loads whatever project skills match the
  task's area.
- Documentation goes to `docs/`. A repo's own `./docs/` and the root `README.md` are off
  limits unless the user explicitly asks this turn.
- Remind the user to commit the session artifacts at the end, if this project treats
  `.claude/working-sessions/` as committed history (see `HARNESS.md`).
