# Delegation harness — agents and the delegation contract

A set of **specialist subagents**, for running multi-step work as orchestrated delegation instead
of one long improvised session. The orchestrator (the main AI session) hands each specialist a
**delegation packet**; the specialist writes its own input and output artifacts so the next step
reads a file rather than inheriting a transcript it was never part of.

**This file is the *who*.** For the *when* — how to start a run, the order the roles go in, the
gates between them and how a run ends — read
**[VIBE-FLOW.md](VIBE-FLOW.md)**, the workflow these roles serve.

## Why split roles instead of one agent doing everything

A single agent that plans, codes, reviews its own work and writes docs in one continuous session
tends to rationalize its own choices at every step — the same "mind" that wrote the code is the
one judging whether it's correct. Splitting into roles with **separate context** (each one reads a
fresh delegation packet, not the planner's or developer's internal reasoning) forces every
hand-off through a written artifact. That has two effects worth the overhead: a reviewer that
didn't watch the code get written catches things a continuous session glosses over, and the
written artifacts become a durable record of *why* the code looks the way it does — useful to a
human or another agent long after the session ends.

## Layout

```
.claude/harness/
├── HARNESS.md              ← this file: the roles and the delegation contract
├── VIBE-FLOW.md      ← vibe-flow: how to invoke it, the flow, gates and routing
├── templates/
│   └── pull-request.md     ← filled at the end of a run; the user opens the PR manually
├── planner.md               ┐
├── developer.md              │
├── code-reviewer.md          ├ one self-contained file per role: frontmatter + the
├── document-writer.md        │ non-negotiable execution order + the role's method,
├── dependency-impact-analyzer.md │ output structure and checklists, all in one place
├── ui-ux-analyzer.md         │ (optional role — see its header)
└── ticket-tracker-assistant.md ┘ (optional role — see its header)
```

`.claude/agents/*.md` are copies of the files here (or symlinks, if your tool supports them and
you'd rather have one source of truth) — that is how your AI tool discovers them as invocable
subagents. Edit the files here as the source of truth.

## The roles

| Agent | Does | Writes |
|---|---|---|
| `planner` | Decomposes work into an ordered, dependency-aware task table with specialist routing, high-level test scenarios and open concerns. | `input-plan.md`, `output-plan-summary.md`, `output-plan-tasks.md`, `output-plan-test-cases.md`, `output-plan-concerns.md` |
| `developer` | Implements one plan task: minimal incremental change, **no code comments** (per the default in root `CLAUDE.md`), sibling-matched style, a build/test check that nothing broke, and **captured evidence that it works**. Loads any project skills matching the task's area. No unit tests unless asked. | `input-dev.md`, `output-<PLAN_TASK_ID>.md`, `evidence/` |
| `code-reviewer` | PR-style review against the plan — correctness, architecture, tests, docs, security. Does not fix. | `input-code-review.md`, `output-review-findings.md` |
| `dependency-impact-analyzer` | Callers, callees, contract semantics, parallel paths, blast radius and test gaps for a proposed change. Read-only. | `input-dependency-impact.md`, `output-dependency-impact-analysis.md` |
| `document-writer` | Syncs `docs/**` with shipped behavior. Read-only on source; writes outside `docs/` only on explicit user request. | `input-docs.md`, `output-doc-sync-report.md` |
| `ui-ux-analyzer` *(optional — only if your project has significant UI/UX surface)* | Fetch-first UI/UX analysis (design tool or image source) → visual specs, accessibility, platform-convention fit, open questions. Read-only. | `input-ui-ux.md`, `output-ui-ux-analysis.md` |
| `ticket-tracker-assistant` | Fetches GitHub issues through `gh`, persists them to orchestrator-given paths, surfaces design links, posts comments on request. | `input-ticket-tracker-assistant.md`, `output-ticket-tracker-assistant.md`, `<ISSUE-KEY>.json` / `.md` |
| *(orchestrator)* | Fills the PR body from the run's artifacts using `templates/pull-request.md`. **No agent creates, pushes or merges a PR.** | `output-pull-request.md` |

**A reasonable minimum** if you're trimming this down: `planner`, `developer`, `code-reviewer` are
the core loop and cover most projects on their own. Add `dependency-impact-analyzer` once the
codebase is big enough that "what breaks if I change this" stops being answerable by eye.
`document-writer` earns its keep once `docs/` exists to keep in sync. `ui-ux-analyzer` and
`ticket-tracker-assistant` are genuinely situational — see their own files for when to keep them.

**Deliberately not in this harness by default:** a `test-runner` role and a `git`/PR-hosting
integration role. Build verification belongs to `developer` (see *Verifying your change* in
`developer.md`); pushing/opening a PR goes through your normal `git`/hosting CLI in the main
session, never through an agent. Add either back as its own role if your project's scale justifies
a dedicated one — the *Adding a role* section below covers how.

## The delegation contract

Every agent follows the same order, and **none of them self-start**:

1. **Log** — append one line to the session log, before reading anything else for the task.
2. **Read own instructions** — this agent's own file under `.claude/harness/`, in full.
3. **Read delegated paths** — every additional skill and every rule/doc path the packet listed.
4. **Write the input artifact** — `input-*.md`, from the packet's template if one was given,
   **otherwise** the shape shown in this agent's own file. After step 3, fill **Skills (using)**
   and **Rules (read)** there — that file is the canonical record of what was loaded, not the
   chat.
5. **Execute** — per the role's own instructions.
6. **Write the output artifact** — `output-*.md`, from the packet's template if one was given,
   **otherwise** the shape shown in this agent's own file.

A delegation packet should carry: the task text verbatim, the session root and step directory,
read paths, write paths, which extra skills and rules to load, and the log contract. Keep it this
shape:

```
You are the <role> for this vibe-flow run.

Task (verbatim): <the user's request, or the plan task row>

Session root:   .claude/working-sessions/<run>/
Step directory: <root>/plan|develop|review|docs|analyze|intake
Read:  <artifact paths from earlier steps that this step needs>
Write: <the input-/output- files this role owns>
Skills to load beyond your own instructions: <extra project skills, or "none">
Rules to read: root CLAUDE.md, <any docs pages>
Log: append one line to <root>/session.log before your first read —
     printf '%s  <role>  %s\n' "$(date -u +%FT%TZ)" "<message>" >> <root>/session.log
```

The agent handles the rest — its own execution order, artifacts and output structure are baked
into its own file under `harness/`.

## Stuck / long-running task capture (every role)

Any role can hit a wall: a task drags far past what its scope suggested, or the same
failure/diagnosis loop repeats without new information. When that happens, **capture it before
escalating or handing back to the orchestrator** — losing that context is what makes the next
session (yours or someone else's) re-pay the same cost.

**Triggers** (any one is enough):
- You've retried the same fix/diagnosis approach ≥2 times with no new information.
- A task is clearly taking far longer than its scope implied, with no forward progress to show for
  it.
- You're about to report back "stuck" / "blocked" / "couldn't get this working" in your output
  artifact.

**Action, before you write your output artifact:**
1. **`.claude/common-issues/<area>.md`** (+ a row in the `.claude/COMMON-ISSUES.md` index if the
   area file is new) — symptom, what you tried, root cause if known, and current status (stuck /
   worked around / deferred). Follow the shape already in that file's *Adding a new entry* section.
2. **`.claude/memory/<name>.md`** (+ a line in the `.claude/MEMORY.md` index) — only if what you
   learned is a durable, reusable fact beyond "this one bug": an environment quirk, a build/tooling
   workaround, a decision made under pressure. Not every stuck task needs this one; most just need
   #1.

This is a **narrow, explicit exception** to "only `document-writer` writes `.claude/COMMON-ISSUES.md`
et al." (see `document-writer.md`'s *Allowed writes*): the point is capturing the finding in the
moment, by whoever hit the wall, not deferring it to a doc pass that may never be scheduled. Keep
the edit scoped to the one entry (and its index row) — do not use this exception to touch `docs/**`
or anything outside these two files' own trees. Note what you wrote (or that nothing warranted it)
under your output artifact's follow-ups/limitations section, so the next reader can find it without
re-discovering it.

## Session artifacts

**Filename rule:** under a step directory, every markdown file starts with `input-` or `output-`.
No bare `input.md`, no unprefixed `plan.md`. Names otherwise follow root `CLAUDE.md`:
kebab-case, letters/digits/hyphens only, `yyyyMMdd` or `yyyyMMddHHmmss` for dates.

Default session root when the orchestrator names none:

```
.claude/working-sessions/<yyyyMMddHHmmss>-<slug>/
├── session.log             append-only; every agent logs before it reads
├── inputs/                 requirements.md, resources/
├── intake/                 ticket-tracker-assistant artifacts (if that role is in use)
├── analyze/                ui-ux and dependency-impact artifacts
├── plan/                   planner artifacts
├── develop/                developer artifacts
│   └── evidence/           screenshots, clips, log excerpts or test output per task
├── review/                 code-reviewer artifacts
├── docs/                   document-writer artifacts
└── output-pull-request.md  filled from templates/pull-request.md
```

**Why artifacts on disk, not just chat.** A long orchestrated run can span many separate agent
invocations, sometimes across sessions (see `VIBE-FLOW.md`'s `resume` form). Nothing in chat
history is guaranteed to still be in context by the time step 5 needs something step 1 decided —
but a file is. Treat the session directory as the actual state of the run; the chat transcript is
disposable.

**This project commits session artifacts, except bulk evidence** — `.gitignore` excludes
everything under `.claude/working-sessions/*/develop/evidence/` except its `*.md` reports and `*.py`
scripts. A run's inputs, decisions, handoffs, review findings and evidence reports are part of git
history, so a run can be continued from another machine and serve as the durable record of why the
code looks the way it does. Bulk evidence (captured frames, JSON dumps, build/console logs) is large
and machine-specific, so it stays local to the machine that produced it; a handoff that cites it
should quote the result it relies on, not only the path. Flip this rule only on the user's say-so; see `VIBE-FLOW.md`.

## Adding a role

1. Write `harness/<name>.md`: frontmatter (`name`, a `description` stating **when** to use it —
   that sentence is all the orchestrator sees when choosing), then the non-negotiable execution
   order, the role's method/checklist, and its output artifact structure, all in one file — follow
   the shape of any existing role file here.
2. Copy it into `.claude/agents/<name>.md` (or symlink it, per your tool's discovery convention).
3. Add the role to the routing table in [VIBE-FLOW.md](VIBE-FLOW.md) so the workflow
   knows when to reach for it.
4. Add a row to **The roles** table above.

## Adapting this to your project

Everything in this file and its sibling role files is written with bracketed placeholders where
something is genuinely project-specific (a build command, a code-intelligence tool, a UI
framework). Fill those in once during adoption. The mechanism — packet in, artifacts out, no
self-starting, filename contract — is meant to be used as-is; the roles themselves are meant to be
trimmed, renamed or extended to match what this project actually needs.
