# .claude/DICTIONARY.md — Term/Concept Definitions

Entry point for "what is X" / jargon-definition lookups in this repo (as distinct from
`docs/**`, which explains *how* code works, not what a term means).

**Check here first** when asked to define or look up a term — before any code-intelligence tool,
before `docs/`, before reading source. If the term isn't here, research it (fall through
to `CLAUDE.md`'s source-of-truth order), then **write what you found as a new entry** here so the
next lookup is instant instead of re-derived from scratch. This lazy-cache-and-backfill pattern is
the point: research cost is paid once, by whoever hits the gap first, and every session after that
gets an instant, cited answer instead of repeating the dig.

| Term | Read |
|---|---|
| `<example-term>` (delete once you have real entries) | [`dictionary/_example-term.md`](dictionary/_example-term.md) |

## Adding/updating an entry

- One file per term, kebab-case filename matching the term (`tenant.md`, `feature-flag.md`).
- Ground every claim in a `file:line` citation or a cross-link to `docs/**` — this is not
  a place for paraphrased-from-memory definitions.
- Add a row to the table above.
- If a later finding changes or extends a term already defined here, update that file in place
  (with a fresh **Analysis date**) rather than creating a duplicate.
