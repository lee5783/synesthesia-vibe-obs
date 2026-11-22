# .claude/agents — discovery copies of the harness roles

Most AI coding tools that support project-scoped subagents look for one file per agent under
`.claude/agents/`. The **canonical, source-of-truth copy** of every role's instructions lives under
[`../harness/`](../harness/) — see [`../harness/HARNESS.md`](../harness/HARNESS.md) for what each
role does and the delegation contract they all follow.

This directory should hold **one file per role you're using**, each a copy of its counterpart
under `harness/`:

```
.claude/agents/
├── planner.md                       (copy of ../harness/planner.md)
├── developer.md                     (copy of ../harness/developer.md)
├── code-reviewer.md                 (copy of ../harness/code-reviewer.md)
├── document-writer.md               (copy of ../harness/document-writer.md)
├── dependency-impact-analyzer.md    (copy of ../harness/dependency-impact-analyzer.md)
├── ui-ux-analyzer.md                (copy of ../harness/ui-ux-analyzer.md — optional role)
└── ticket-tracker-assistant.md      (copy of ../harness/ticket-tracker-assistant.md — optional role)
```

## Why a copy instead of just putting the files here directly

Splitting the *source of truth* (`harness/`) from the *discovery location* (`agents/`) keeps one
clear place to edit a role even as your tool's discovery convention changes or you add a second
discovery mechanism (e.g. also exposing a role as a slash command). Edit `harness/<role>.md`, then
update the matching copy here.

## Keeping copies in sync

Two options, in order of setup cost:

1. **Symlink** (if your OS/tool/repo-hosting setup tolerates symlinks in the discovery path):
   `ln -sfn ../harness/<role>.md agents/<role>.md` from inside `.claude/`. One edit, always in
   sync, zero drift risk. This is the simplest option if nothing in your toolchain treats
   symlinks specially (some git hosting or CI checkouts flatten or mishandle them — check first).
2. **Plain copy, kept in sync by hand or by a small script.** Simplest to reason about, but you
   must remember to re-copy after editing `harness/<role>.md`. A one-line script works:

   ```bash
   for f in .claude/harness/*.md; do
     name="$(basename "$f")"
     [ "$name" = "HARNESS.md" ] || [ "$name" = "VIBE-FLOW.md" ] || cp "$f" ".claude/agents/$name"
   done
   ```

Pick whichever fits your project; this template defaults to describing the copy approach since it
has no failure modes to explain, but symlinks are the better choice once you trust your toolchain
with them.
