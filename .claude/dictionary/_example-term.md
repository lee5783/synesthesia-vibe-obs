---
name: idempotency-key
description: An example dictionary entry, showing the format this file family uses. Replace with real terms from your own project — delete this file once you have at least one.
metadata:
  type: reference
---

**Analysis date:** 2026-01-01 *(placeholder — this is a demo entry, not a real finding)*

**Idempotency key** is a client-generated unique token attached to a request so that retrying the
same logical operation (e.g. after a timeout) doesn't cause it to execute twice. This entry exists
purely to demonstrate the expected shape of a dictionary entry: short, grounded, cross-linked —
not to document a real concept in your project.

## What a real entry should contain

- A definition grounded in **this repo's actual usage** of the term, not a generic dictionary
  definition — cite the file(s) where it's defined or enforced, e.g. `src/http/middleware.py:42`.
- Enough context that a reader unfamiliar with the codebase understands why the term exists here
  specifically (what problem it solves, what would break without it).
- Cross-links to related entries: other dictionary terms (`[related-term](related-term.md)`),
  relevant `docs/**` pages, or a `.claude/how-x-works/**` flow that uses this concept.

## Format notes

- One file per term, kebab-case filename matching the term.
- Frontmatter: `name`, a one-line `description`, and `metadata.type: reference` (kept consistent
  across entries so a future tool could parse this corpus programmatically if useful).
- Ground every claim in a `file:line` citation — this is not a place for paraphrased-from-memory
  definitions.
- Update in place with a fresh **Analysis date** when a later finding changes or extends the term,
  rather than creating a duplicate file.

## Related

- `../DICTIONARY.md` — the index this entry is registered in.
- `../../README.md` — the doc-corpus entry point this dictionary complements.
