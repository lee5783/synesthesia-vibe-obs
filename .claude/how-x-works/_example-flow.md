# Example flow — request lifecycle (demo entry)

> Part of the how-x-works corpus — see [`../HOW-X-WORKS.md`](../HOW-X-WORKS.md) for the index.
> This is a **demo entry** showing the format; replace it with a real, project-specific flow
> write-up and delete this file once you have at least one.

A how-x-works entry traces one cross-cutting flow end-to-end, the way you'd explain it to a new
engineer at a whiteboard — but with citations, so a reader can jump straight to the code instead
of taking your word for it.

## 1. `<entry point>`

`<How the flow begins — an incoming request, a user action, a scheduled trigger — with a concrete
file:line citation, e.g.>` `src/http/router.py:18` `routes the request to the matching handler
based on `<method + path>`.

## 2. `<middle steps>`

`<Each subsequent hop: what calls what, what transforms happen, what decisions get made and on
what data — one numbered section per meaningful stage, each grounded in file:line citations.>`

## 3. `<exit point / outcome>`

`<How the flow ends — a response is written, an event is published, a job completes — and what
the caller/user observes.>`

## Format notes

- One file per **flow**, kebab-case filename describing it end-to-end
  (`request-lifecycle.md`, `checkout-flow.md`), not one file per component the flow touches.
- Number the stages in execution order — a reader should be able to follow the file top-to-bottom
  the same way the code executes.
- Ground every step in a `file:line` citation.
- Note **edge cases and failure modes** inline where they diverge from the happy path, rather than
  only describing the success case.
- Update in place with a fresh **Analysis date** when a later finding changes or extends the flow,
  rather than creating a duplicate file.

**Analysis date:** 2026-01-01 *(placeholder — this is a demo entry, not a real finding)*
