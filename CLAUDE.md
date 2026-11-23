# AI Agent Guide — root rules

> **This is the canonical rules + guide file for this repo.** `AGENTS.md`, `GEMINI.md`,
> `.github/copilot-instructions.md` and `.cursor/rules/project-rules.mdc` are thin stubs that
> point here — kept only because some tools look for those filenames on startup. Don't fork rules
> between files; everything substantive lives here.

This file and everything under `.claude/` started from a shared AI-agent template. The prose is
the actual rule set; edit it deliberately.

## Non-Negotiable Rules

These apply regardless of which AI tool is running (Claude Code, Cursor, Codex, Windsurf, Kiro,
Gemini CLI, Copilot, etc.) — kept provider-agnostic on purpose, even though the canonical copy
lives under this Claude-Code-conventional filename rather than `AGENTS.md`.

### Rule 1 — This repo is provider-agnostic. It must not be locked to one AI vendor.

- Any AI agent must be able to onboard here even if it only knows to look for `AGENTS.md` (or
  `GEMINI.md`, `.github/copilot-instructions.md`, `.cursor/rules/`) — those files are pointers to
  this one, not forks of these rules.
- Don't write scripts, hooks, or instructions that only work with one vendor's CLI/agent unless
  explicitly scoped to that tool's own directory (e.g. `.claude/`).
- Don't invent tool-only conventions in a differently-named file that a different agent couldn't
  follow — this file is the one source of truth.

### Rule 2 — Check the source of truth before acting. Don't default to reading large source files.

Re-deriving architecture and conventions from raw source every session is slow and inconsistent —
this repo keeps a small, curated knowledge corpus under `.claude/` and `docs/` specifically so
agents don't have to. Before executing any prompt that requires understanding the codebase, check
sources **in this order** and stop at the first one that answers the question:

0a. **Pure "what does term X mean?" / definition-lookup questions specifically** (not "how does X
    work"): check **[`.claude/DICTIONARY.md`](.claude/DICTIONARY.md) → `.claude/dictionary/<term>.md`**
    first, before any code-intelligence tool. If the term isn't there, fall through to steps 1-4
    below to research it, then **save what you found as a new `.claude/dictionary/<term>.md`
    entry** (or update the existing one if it's stale) so the next lookup is instant.
0b. **"How does flow X work end-to-end?" questions** (a multi-step process spanning several
    files — e.g. request handling, checkout, auth, a background job pipeline — not a
    single-file/module question): check **[`.claude/HOW-X-WORKS.md`](.claude/HOW-X-WORKS.md) →
    `.claude/how-x-works/<flow>.md`** first, before any code-intelligence tool. If the flow isn't
    documented, fall through to steps 1-4 to research it, then **save the write-up** as a new
    `.claude/how-x-works/<flow>.md` entry (or update the existing one) so the next lookup is
    instant.
1. **Code-intelligence tool, if this project has one configured** (see *Code intelligence*
   below). Query it first for structure, relationships, call sites and references — it is
   pre-computed and far cheaper than a grep-and-read sweep. **If it is configured but you cannot
   connect to it, stop and tell the user explicitly** — do not silently skip to a fallback.
2. **`docs/`** — read only the specific doc file that answers the current question (see the
   routing table below). Do not bulk-load the corpus.
3. **`.claude/COMMON-ISSUES.md` → `.claude/common-issues/<area>.md`** — before fixing anything (a
   bug, a build failure, unexpected behavior), check whether it's already a known/catalogued issue
   rather than diagnosing from scratch.
4. **Source code — fallback only.** Read source directly only if #1-#3 didn't answer the question.
   If you had to fall back to source for something the docs should have covered, update the
   relevant file under `docs/` (or trigger a doc-sync pass) in the same session so the corpus
   doesn't go stale.

Jumping straight to reading large source files/directories without checking the above first is a
rule violation, not a shortcut.

## Code intelligence

Some repos have a code-intelligence tool indexed over them — a code-graph database, a language
server, a symbol index, or similar — that answers "who calls this / what does this affect / where
is this defined" faster and more accurately than a grep-and-read sweep, because it's pre-computed
structure instead of something re-derived by reading files every time.

**CodeGraph is configured** (`.codegraph/` exists at the repo root). Reach it through the
`codegraph_explore` MCP tool when available, otherwise `codegraph explore "<symbol names or
question>"` from the shell. Reach for it before grep/find or reading files when locating or
understanding code; if it's configured but unreachable, say so explicitly rather than silently
falling back to a grep sweep. `.codegraph/` is local and git-ignored: run `codegraph init .` once
after cloning. The index covers the plugin C++ (`plugin/src/`), the Browser Source JS
(`browser-source/obs-page.js`) and the core submodule's sources.

## The project

**SynesthesiaVibe for OBS** brings the SynesthesiaVibe music visualizer (a MilkDrop-preset
renderer, public repo `lee5783/synesthesia-vibe`, MIT) into OBS Studio for streamers. It ships two
things:

- **Browser Source page** (`browser-source/`) — a static page built on the synesthesia-vibe web
  dist that OBS loads as a Browser Source. Works on every OS OBS runs on.
- **Native OBS plugin** (`plugin/`) — a C/C++ libobs source plugin, "SynesthesiaVibe Visualizer",
  that links the core through its public `sv_*` C API, renders into OBS's graphics context and
  reacts to an OBS audio source. macOS first, then Windows, Linux optional.

The core lives in the pinned submodule `external/synesthesia-vibe/` and is **never edited here**:
a needed core change is a request on the core repo, referenced as `synesthesia-vibe#<n>`.

**Read [`PLAN.md`](PLAN.md) first.** It holds the phases, the decisions still open and the risks.
This is a **public repo**: never commit secrets, tokens, machine-specific absolute paths, personal
email addresses, or references to private repositories.

## Read the relevant deep-dive doc before making changes

A doc corpus lives under `docs/` — start at **[`docs/README.md`](docs/README.md)**, the entry
point into the whole corpus. This is the knowledge base for this repo: don't re-derive
architecture/conventions from scratch every session, and don't load all of it into context at
once — **read only the file that answers your current question**, via `Read`/`Grep`, same as any
other file.

| Question | Read |
|---|---|
| Where does X live / where should new code go? | `docs/structure.md` |
| How does data/control flow, what are the layers/abstractions? | `docs/architecture.md` |
| What naming/style/error-handling convention should I follow? | `docs/conventions.md` |
| Where do tests actually run, what's the mocking style? | `docs/testing.md` |
| Third-party services, auth, analytics, CI/CD, env/build config | `docs/integrations.md` |
| Language/framework/dependency versions | `docs/stack.md` |
| Known tech debt, bugs, fragile areas, security notes | `docs/concerns.md` |
| Locked decisions (D1 licence, D4 core build) and their consequences | `docs/decisions.md` |
| Phases, open decisions, risks, upstream asks | `PLAN.md` |
| **Known issues (check before fixing)** | `.claude/COMMON-ISSUES.md` (index) → `.claude/common-issues/<area>.md` |
| **What does term/jargon X mean?** | `.claude/DICTIONARY.md` (index) → `.claude/dictionary/<term>.md` — check first, save new findings here |
| **How does flow X work end-to-end?** | `.claude/HOW-X-WORKS.md` (index) → `.claude/how-x-works/<flow>.md` — check first, save new write-ups here |
| **Repo-specific durable facts / workarounds** | `.claude/MEMORY.md` (index) → `.claude/memory/<name>.md` |

All eight `docs/` pages above exist and describe `browser-source/` and the `plugin/` spike. Keep
this table and `docs/README.md` listing exactly the pages that exist, and add a section to
`docs/decisions.md` whenever a `PLAN.md` decision is locked. A doc-routing
table beats one giant `ARCHITECTURE.md`: a single page either gets skipped (too long to read every
session) or gets stale in patches. One focused page per question keeps each page short enough to
actually re-read, and cheap enough to regenerate when it drifts. The `codebase-mapper` skill
generates the first seven pages.

## Setup

Run from the repository root. Details, install steps and logs: `docs/integrations.md`.

- `git submodule update --init --recursive` — fetches the pinned core (the plugin needs it; the
  Browser Source does not).
- Browser Source: `browser-source/fetch-dist.sh` (downloads the pinned web dist and presets into
  the git-ignored `browser-source/dist/`), then
  `python3 -m http.server 8080 --bind 127.0.0.1 --directory browser-source`. OBS setup:
  `browser-source/README.md`.
- Plugin (macOS, Apple Silicon only for now): needs Xcode, CMake ≥ 3.28, `ninja` on the `PATH` and
  `VCPKG_ROOT` pointing at a vcpkg checkout. Then
  `cd plugin && cmake --preset macos-arm64 && cmake --build --preset macos-arm64`. The first
  configure downloads and builds libobs (about 3 minutes).

## Quick facts worth knowing up front

- C++17 for the plugin (C where libobs callbacks want it), plain ES modules for the Browser
  Source page (no bundler, no npm). CMake; dependencies are the core submodule and libobs.
- No linter or formatter — the sibling file is the style guide.
- No test suite yet. Plugin evidence is a manual OBS check with screenshots
  (`.claude/harness/developer.md`, *Evidence*).
- One `sv_context` is single-threaded and needs the GL context current. OBS calls a source from
  the UI, graphics and audio threads: audio crosses to the graphics thread through a ring buffer.
- OBS renders with OpenGL on macOS and Linux and with Direct3D 11 on Windows; newer macOS builds
  can opt into Metal. The core is OpenGL / GLES / WebGL only.
- The plugin links libobs, so it is GPL-2.0-or-later; the core stays MIT (`PLAN.md`, D1).

## Editing source files

These are team preferences, and they override an AI tool's defaults. Tune them for your project,
then treat them as binding — a sibling file is the style guide here, not a linter.

- **Do not add code comments** *(default — edit to taste)*. Write code whose naming and structure
  carry the meaning instead. This covers inline commentary, explanatory block comments, and
  doc-comment blocks on new or edited code.
  - **Do not strip existing comments** from code you touch — leaving them is not the same as
    adding them. Update one only when your change makes it actively wrong.
  - Structural section markers (this project's equivalent of `// MARK: -`, if it uses one) are
    structure, not commentary — keep matching the file's existing use of them.
  - The **standard file header** required below (*Adding new source files*), if this project uses
    one, is also exempt — every new file still gets it.
  - The only exception is an **explicit request** to document something ("add doc comments to
    this service"). Absent that, no comments.
- **Follow the sibling components' style.** Before writing anything, open the structurally closest
  existing component and match it — naming, file layout, ordering, access levels, how state and
  dependencies are passed, error handling. If there's no linter/formatter here to catch drift, a
  sibling file is the only style guide there is. Do not introduce a pattern the neighbours do not
  already use.
- **Do not write unit tests unless asked** *(default — edit to taste)*. Implementation is the
  deliverable. Do not add test targets, test files or test cases on your own initiative, and do
  not treat missing tests as a defect in review. If a change is risky enough that you think it
  needs a test, say so in one line and let the user decide. When the user *does* ask, follow
  `docs/testing.md`.
  - Running the **existing** suite to check you did not break anything is still fine and
    encouraged when the change warrants it.

## Where documentation goes

- **Default target is `docs/`** — the AI doc corpus (routing table + generated deep-dive pages).
  Documentation work lands there unless told otherwise. `COMMON-ISSUES` / `DICTIONARY` /
  `HOW-X-WORKS` / `MEMORY` and their entry files stay under `.claude/` (see the routing table
  above) since they're agent-maintained lazy caches, not human-facing reference docs.
- The root `README.md` is only touched on request.

## Adding new source files

Whenever creating a new file anywhere in the codebase, never invent a new location or layout.
First find a sibling file/feature that is structurally closest to what's being added and mirror
it: same folder shape, same naming pattern, same relative position a sibling feature would put its
equivalent file. Check `docs/structure.md` for this project's documented layout if unsure.

**File header** *(delete this block if the project doesn't use one)*: every new source file gets
the standard header, with the author line filled in as `lee5783` — not the AI tool's name,
regardless of which AI agent generates the file:

```
//
//  <FileName>.<ext>
//  SynesthesiaVibeOBS
//
//  Created by lee5783 on <yyyy-MM-dd>.
//
```

Use the language's own comment syntax for the same block (`#` lines in CMake and shell).

## Naming AI-generated files and folders

Any file or folder your AI tool creates in this repo (docs, scratch notes, generated assets,
anything under `.claude/`) must follow these rules:

- **kebab-case** — lowercase words separated by single hyphens (`payment-service.md`,
  `screen-flow-notes/`). No camelCase, PascalCase, or snake_case.
- **No special characters** — letters, digits, and hyphens only. No spaces, underscores,
  parentheses, colons, ampersands, or unicode; avoid characters illegal on any of
  macOS/Linux/Windows (``\ / : * ? " < > |``) and no trailing dots/spaces (Windows rejects both).
- **Short but memorable** — prefer a clear short phrase over a cryptic abbreviation; a future
  session (or teammate) should be able to guess the contents from the name alone.
- **Dates/times in filenames** — full timestamp `yyyyMMddHHmmss`, date-only `yyyyMMdd` (e.g.
  `audit-20260807.md`, `run-log-20260807143022.md`).

When a doc tree mirrors a source directory tree (e.g. `docs/`), map path segments 1:1 with
the source, converting each segment to kebab-case: `src/PaymentService` → `docs/src/payment-service`,
`app/Modules/Billing` → `docs/app/modules/billing`.

## Skills

Skills live in `.claude/skills/<name>/SKILL.md` (plain markdown — any agent can read and follow
one even if it has no native skill mechanism).

- **`codebase-mapper`** — explores the codebase for one focus area (`tech`, `arch`, `quality`,
  `concerns`, `deeplink`) and writes the matching `docs/` pages from its templates.
- **`project-documentation`** — README shape, `docs/features/**/INDEX.md` hierarchy with
  cross-links, and (on request only) in-code doc comments.
- **`vibe-flow`** — entry point for the delegation workflow (see below).

- **`shader-programming`** — load first when touching GL state, render targets or shaders.
- **`cpp-coding-standards`**, **`cpp-testing`** — manual activation only: apply them only when the
  user invokes them that turn.

## Delegation harness (agents)

Specialist subagents — see **[`.claude/harness/HARNESS.md`](.claude/harness/HARNESS.md)** for the
full roster — live in **[`.claude/harness/`](.claude/harness/)** and are copied (or symlinked, if
your tool supports it and you prefer one source of truth) into `.claude/agents/` from there. Read
[`.claude/harness/HARNESS.md`](.claude/harness/HARNESS.md) before invoking, adding or editing one:
it defines the delegation packet, the `input-`/`output-` artifact contract and the session
directory layout. Edit the files under `.claude/harness/` as the source of truth; if
`.claude/agents/` holds copies rather than symlinks, keep them in sync by hand or with a small
sync script.

The workflow these agents serve is **vibe-flow** —
[`.claude/harness/VIBE-FLOW.md`](.claude/harness/VIBE-FLOW.md): how to invoke it, the gate order,
the `.claude/working-sessions/` working directory and the PR-template finish. Invoke it via the
`vibe-flow` skill (`.claude/skills/vibe-flow/SKILL.md`); a user who names the flow in plain prose
is asking for the same thing. Read `VIBE-FLOW.md` before starting one. Agents without native
subagent support run the same roles sequentially in one session, reading each role file and
writing the same artifacts.

## Memory

Two kinds, kept apart:

- **Repo memory** — `.claude/MEMORY.md` → `.claude/memory/<name>.md`, checked in. Durable facts
  about *this repo* any contributor or agent should see: environment quirks, build workarounds,
  decisions made under pressure.
- **Your AI tool's private memory** (if it has one) — durable facts about how *you* (the user)
  want repos worked on. Not repo architecture: that belongs in `docs/`.
