# REVIEW-FIX

## Subtasks executed
- S1: Option A now says the installer also puts `synesthesia-vibe-obs.plugin.dSYM` (debug symbols,
  about 1.4 MB, ignored by OBS) next to the plugin. Uninstall step 2 removes both bundles and notes
  that the dSYM line does nothing after a zip install.
- S2: Uninstall step 3 gives the exact command
  `pkgutil --forget "'io.github.lee5783.synesthesia-vibe-obs'"` and explains that the receipt id
  includes the single quotes, so pasting the `pkgutil --pkgs` line unquoted makes the shell strip
  them and `pkgutil` finds no receipt. The `pkgutil --pkgs | grep` lookup step was dropped from the
  code block (the id is fixed), but is still named in the explanation.
- N4: Blend time row: "Only two changes skip the blend: the first preset a new source shows, and
  the next change after a `preset failed to commit` error in the log." One sentence, replacing the
  old one.

## Files changed
- `plugin/README.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/input-dev.md` (REVIEW-FIX section appended)
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/output-REVIEW-FIX.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/develop/evidence/REVIEW-FIX/review-fix-report.md`
- `.claude/working-sessions/20261123091415-plugin-phase-2-macos/session.log` (log lines)

## What changed and why
README text only, so the uninstall instructions match the P2-013 pkg: it installs a dSYM next to
the plugin, and its receipt id is `'io.github.lee5783.synesthesia-vibe-obs'` with the quotes
(template bug in the vendored `create-package.cmake.in`). The template was not overridden; no code
or CMake changed.

## Verification
- Pkg identity: SHA-256 `1dbf3ab2…f187889`, matching `output-P2-013.md`.
- `pkgutil --expand` in the scratchpad: `PackageInfo identifier="'io.github.lee5783.synesthesia-vibe-obs'"`,
  `Distribution packageIdentifier="'io.github.lee5783.synesthesia-vibe-obs'"`.
- `pkgutil --payload-files`: `.plugin` and `.plugin.dSYM` in `Library/Application Support/obs-studio/plugins/`.
  The zip holds only the `.plugin`.
- The README's `pkgutil --forget` line, sourced in zsh and bash with `pkgutil` stubbed: argv is
  `['io.github.lee5783.synesthesia-vibe-obs']` (quotes kept). The unquoted paste gives
  `io.github.lee5783.synesthesia-vibe-obs` (quotes stripped).
- The README's two `rm -rf` lines in a fake HOME: they remove both bundles, leave an unrelated
  plugin, and exit 0 when the dSYM is absent.
- `grep` for `/Users/` or `/private/` in `plugin/README.md`: none.
- No build: README-only change.

## Evidence
- `evidence/REVIEW-FIX/review-fix-report.md` — pkg hash, payload listing, PackageInfo and
  Distribution excerpts, shell-quoting demo, fake-HOME uninstall dry run.

## Known limitations / follow-ups
- The real `pkgutil --pkgs` and `pkgutil --forget` against an installed receipt are unverified (not
  installed, as delegated). Part of the deferred end-to-end install check P2-014 already lists. Also
  unverified in that check: whether a `CurrentUserHomeDirectory` receipt is listed by plain
  `pkgutil --pkgs` or needs `--volume ~`. The README assumes the former, as it did before.
- If a project-owned `create-package.cmake` override later fixes the quoted id or drops the dSYM
  from the payload, Option A and Uninstall steps 2–3 must be updated back.
- Doc gap for document-writer: `docs/integrations.md` packaging notes should mention the dSYM in
  the payload and the quoted receipt id, if they don't already.

## Handoff for downstream review
- Acceptance criteria addressed: S1 (Option A mention + `rm -rf`), S2 (exact command +
  explanation), N4 (one-sentence wording fix).
- Areas to scrutinize: wording of Uninstall step 3; that "1.4 MB" matches the 1,380 KiB staged
  dSYM closely enough.
- How to run relevant checks: see the evidence report; all are read-only on the pkg.
