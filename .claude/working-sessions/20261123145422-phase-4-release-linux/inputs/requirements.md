# Requirements

## Request (verbatim)

> start phase 4 on top of this

"This" is the Phase 2 work: branch `feature/plugin-phase-2-macos`, commit `a7b62ed`, open as PR #2
and not merged. This run starts from that branch on `feature/phase-4-release-linux`. Earlier runs:
`20261123004446-browser-source-phase-1` (PR #1, merged) and `20261123091415-plugin-phase-2-macos`
(PR #2).

## Scope

`PLAN.md` → **Phase 4 — Release and Linux (medium)**:

1. CI builds the plugin for each OS on a `v*` tag and attaches the artifacts to a GitHub Release.
2. Linux (optional): OBS uses OpenGL/EGL there, so the macOS path should mostly carry over. Ship
   it only after a real test on Ubuntu with an X11 and a Wayland session.
3. Publish on the OBS forums resource page.
4. Run `codebase-mapper` to generate `docs/` and fill the routing table in `CLAUDE.md`.

## Known context (not new requirements)

- **Phase 3 (Windows) is blocked** until the core's Windows GL build is re-verified. "Each OS" in
  item 1 can't include a Windows plugin build yet.
- **Phase 2 is not done.** Its *Done when* checks were deferred: P2-011, P2-012 and P2-010b.
  Notarization is waiting on Developer ID credentials. Phase 1 is waiting on the user's loopback
  checklist.
- **Item 4 is mostly already done.** All eight `docs/` pages exist, and the `CLAUDE.md` routing
  table is filled.
- **Duplicate glad symbols on Linux.** `PLAN.md` *Risks* says the Linux link must export only
  `obs_module_*`, through a version script or `--exclude-libs,ALL`.
- The repo has **no `.github/workflows/`** yet. The obs-plugintemplate files under `plugin/cmake/`
  are verbatim and are never edited.

## Out of scope

Phase 3 (Windows), editing the core submodule, the root `README.md`, and finishing the deferred
Phase 2 checks.

## Standing constraints (from earlier runs)

- **OBS:**
  - Drive OBS only through obs-websocket; never use GUI or accessibility automation. If the
    websocket is unavailable, stop and ask the user.
  - Never edit OBS config files.
  - Never change the profile, canvas or fps without asking.
  - Test in a separate scene collection.
- **Repo contents:** no secrets, credential paths or machine paths in repo files. This is a public
  repo.
- **Git and outward-facing actions:**
  - Commit, push, open PRs, push tags or create releases only when the user asks.
  - Publishing anything outside the repo (for example a forum post) is the user's action.
- **`CLAUDE.md` conventions:**
  - no code comments;
  - the standard file header, with author lee5783;
  - no unit tests unless asked;
  - kebab-case names;
  - match the sibling files' style.
