# .claude/HOW-X-WORKS.md — Flow/Process Write-ups

Entry point for "how does flow X work" questions (as distinct from `.claude/DICTIONARY.md`, which
is single-term definitions, and `docs/**`, which explains one file/module rather than a
cross-cutting flow).

**Check here first** when asked to explain a multi-step process end-to-end — before any
code-intelligence tool, before `docs/`, before reading source. If the flow isn't
documented yet, research it (fall through to `CLAUDE.md`'s source-of-truth order), then **save the
write-up** here so the next lookup is instant instead of re-traced through the codebase again.

| Flow | Read |
|---|---|
| Plugin render frame (`video_tick` atomics → `video_render` → transition config, commit, resize, audio push, `sv_render_frame` in the GL guard → opaque flipped sprite; teardown) | [`how-x-works/plugin-render-frame.md`](how-x-works/plugin-render-frame.md) |
| Plugin audio path (source capture or whole mix → resampler → SPSC ring → `sv_push_pcm_float`, silence push) | [`how-x-works/plugin-audio-path.md`](how-x-works/plugin-audio-path.md) |
| Plugin preset loading (setting / auto-cycle request / hotkey → worker `sv_prepare_preset` → ready slot → `sv_commit_preset` in render; cycle latch, pause) | [`how-x-works/plugin-preset-loading.md`](how-x-works/plugin-preset-loading.md) |
| Browser Source startup (URL settings → dist import → playlist → cycling → audio) | [`how-x-works/browser-source-startup.md`](how-x-works/browser-source-startup.md) |
| Release pipeline (`v*` tag → validate tag and core version → Release build on macOS and Linux → checksums, notes → draft (pre)release; Linux `.deb` not attached) | [`how-x-works/release-pipeline.md`](how-x-works/release-pipeline.md) |

## Adding/updating an entry

- One file per flow, kebab-case filename describing the flow end-to-end (`request-lifecycle.md`,
  `checkout-flow.md`).
- Ground every step in a `file:line` citation — this is not a place for paraphrased-from-memory
  flows.
- Add a row to the table above.
- If a later finding changes or extends a flow already written up here, update that file in place
  (with a fresh **Analysis date**) rather than creating a duplicate.

## Why this saves real work

A cross-cutting flow (auth, checkout, a background job pipeline) is expensive to trace the first
time — it touches many files and the connections between them aren't visible from any single one.
Once someone has traced it and written it down with citations, every later session reads a page
instead of repeating the multi-file trace. Treat a stale entry as a bug: if you read one and find
it no longer matches the code, fix it in place rather than silently working around the
discrepancy — the next reader deserves the corrected version, not a repeat of your confusion.
