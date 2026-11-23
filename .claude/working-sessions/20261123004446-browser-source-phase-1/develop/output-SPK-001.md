# SPK-001

## Subtasks executed
- Confirmed the remote tag: `refs/tags/v0.7.0` → `887628f08dd35c1a3e984750323987f82a9ce3c6`.
- `git submodule add https://github.com/lee5783/synesthesia-vibe.git external/synesthesia-vibe`.
- `git -C external/synesthesia-vibe checkout v0.7.0`, then `git add external/synesthesia-vibe` to
  stage the pinned gitlink.
- Checked for nested submodules (none: no `.gitmodules` in the core) and a clean submodule tree.
- Recorded the answer in `PLAN.md` under the **Building the core** "Spike first" bullet.

## Files changed
- `.gitmodules` (new, staged by `git submodule add`)
- `external/synesthesia-vibe` (gitlink at `887628f`, staged)
- `PLAN.md` (one "Answer (2026-11-23), submodule:" sub-bullet; unstaged)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/input-dev.md` (SPK-001 section appended)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/SPK-001/SPK-001-submodule.log`

## What changed and why
The core is now a pinned submodule, which D4 and every later SPK task depend on. Nothing inside
the submodule was edited. `.gitmodules` holds the single entry with the public HTTPS URL and no
machine paths. Only `.gitmodules` and the gitlink are in the index; nothing was committed and the
other working-tree changes (`PLAN.md`, `LICENSE`, `browser-source/`, the session dir) stay
unstaged.

`PLAN.md` answer, under **Building the core**, limited to the pin. The subdirectory question
stays open for SPK-002.

## Verification
Key lines (full output in the evidence log):

```
$ git submodule status --recursive
 887628f08dd35c1a3e984750323987f82a9ce3c6 external/synesthesia-vibe (v0.5.0-19-g887628f)
$ git -C external/synesthesia-vibe describe --tags
v0.7.0
$ git -C external/synesthesia-vibe status
HEAD detached at v0.7.0
nothing to commit, working tree clean
$ cat .gitmodules
[submodule "external/synesthesia-vibe"]
	path = external/synesthesia-vibe
	url = https://github.com/lee5783/synesthesia-vibe.git
$ git status --short
A  .gitmodules
 M PLAN.md
A  external/synesthesia-vibe
?? .claude/working-sessions/20261123004446-browser-source-phase-1/
?? LICENSE
?? browser-source/
```

Acceptance: `git submodule status` shows the `v0.7.0` commit (`887628f`) — yes; `.gitmodules` has
one entry — yes; `describe --tags` prints `v0.7.0` — yes.

## Evidence
- `evidence/SPK-001/SPK-001-submodule.log`: remote tag lookup, `git submodule status --recursive`,
  `describe --tags`, `rev-parse HEAD`, tag object type, submodule `git status`, `.gitmodules`,
  nested-submodule check, `git diff --cached --stat` and `git status --short`. The `.log` is
  git-ignored by the repo's evidence rule (only `.md`/`.py` evidence is tracked); the key lines
  are quoted above.

## Known limitations / follow-ups
- `v0.7.0` (and `v0.4.0`, `v0.6.0`) are **lightweight** tags in the core, so plain
  `git describe`, which `git submodule status` uses, labels the commit `v0.5.0-19-g887628f`. The
  SHA is correct. Optional upstream nicety: annotated release tags. Not filed.
- `--recursive` is not needed (no nested submodules at `v0.7.0`); it stays harmless if docs use it.
- The submodule HEAD is detached at the tag, as expected for a pin. No `branch =` line in
  `.gitmodules`, so `git submodule update --remote` would move it to the default branch; nobody
  should run that.
- Doc gap for document-writer (SPK-010): setup step `git submodule update --init` in
  `docs/integrations.md`/`docs/structure.md`, and the lightweight-tag note.
- The `PLAN.md` answer carries no evidence path because the evidence log is git-ignored session
  output.

## Handoff for downstream review
- Acceptance criteria addressed: all three SPK-001 acceptance bullets.
- Areas to scrutinize: `.gitmodules` URL (HTTPS, public); the gitlink SHA matches the remote tag;
  the index holds only `.gitmodules` and the gitlink; the `PLAN.md` sub-bullet wording.
- How to run: `git submodule update --init` after cloning, then
  `git -C external/synesthesia-vibe describe --tags`.
