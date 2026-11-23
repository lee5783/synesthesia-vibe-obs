# P4-006

## Subtasks executed
- `.github/workflows/release.yaml` (`Release`), triggered on `push: tags: [v*]`, with
  `permissions: contents: read` at the top and `concurrency` per ref without cancelling.
- Job `validate` (*Validate tag*): checks the tag before anything builds.
- Job `build`: `needs: validate`, calls `build-plugin.yaml` with `config: Release`.
- Job `release` (*Create draft release*): the only job with `contents: write`. It downloads the
  macOS artifact, writes and self-checks `SHA256SUMS.txt`, writes the notes, then runs
  `gh release create … --draft [--prerelease]`.

## Files changed
- `.github/workflows/release.yaml` (new)
- `.claude/working-sessions/20261123145422-phase-4-release-linux/develop/evidence/P4-006-release-logic.md`

## What changed and why
- **Tag rule (C-017, C-016 b):** `plugin/buildspec.json` must be `X.Y.Z`. The tag must match
  `^v(X.Y.Z)(-(beta|rc).N)?$` with `X.Y.Z` equal to the buildspec version. A suffix means
  `prerelease=true`. A plain `vX.Y.Z` is refused unless the repository variable
  `RELEASE_FINAL_ALLOWED` is `true`. Each failure names the problem: wrong shape, both versions on
  a mismatch, or the variable for a final tag.
- **Core version:** `CORE_REPOSITORY=lee5783/synesthesia-vibe` and `CORE_VERSION=v0.7.0` are workflow
  env values. *Validate core version* compares `git ls-remote` of that tag with the submodule
  gitlink and fails on a mismatch, so the notes can't name the wrong core tag after a bump.
- **Release:** always `--draft`, `--verify-tag`, title `SynesthesiaVibe Visualizer <tag without v>`,
  `--prerelease` for suffixed tags. Assets: `synesthesia-vibe-obs-<X.Y.Z>-macos-arm64.pkg`, `.zip`
  and `SHA256SUMS.txt` (`sha256sum` format, `sha256sum -c` run before upload). The Linux `.deb` is
  neither downloaded nor attached (C-006); it stays a workflow artifact of `build`. Publishing the
  draft is a human click.
- **Notes**, generated in the job: downloads list, ad-hoc signed and unnotarized with a link to
  `plugin/README.md` at the tag, the Linux status, GPL-2.0-or-later plus the MIT core, links to this
  tag and to core `v0.7.0`, and a note that GitHub's source archives omit the submodule (C-026).
- `gh` is authenticated with `github.token`. No `secrets.*` and no third-party release action.

## Verification
- `actionlint` with shellcheck: 0 errors.
- *Validate tag*, extracted verbatim and run locally against `v0.1.0`, `v0.1.0-beta.1`,
  `v0.1.0-rc.2`, `v0.2.0`, `0.1.0`, `v0.1`, `v0.1.0-foo` and `v0.1.0-beta.0`, each with the variable
  unset and `true`. Only the suffixed matching tags pass, plus `v0.1.0` with the variable `true`.
  `v0.2.0` reports `names version 0.2.0, but plugin/buildspec.json says 0.1.0`.
- *Validate core version*: passes for `v0.7.0` (`887628f`) and fails for `v0.6.0`.
- *Write checksums* on the locally built assets: both `OK`.
- *Write release notes*: rendered for `v0.1.0-beta.1` (quoted in the evidence).

## Evidence
- `evidence/P4-006-release-logic.md`: every validation case with its output, the checksum run, and
  the rendered notes.
- No tag pushed and no draft created (the orchestrator owns git and GitHub writes).

## Known limitations / follow-ups
- **Test tag:** the Gate 1 example `v0.1.0-ci.1` is **refused** by the C-017 rule (only
  `-beta.N`/`-rc.N`). Use `v0.1.0-beta.0` as the throwaway (the first real tag is `v0.1.0-beta.1`).
  Afterwards delete the draft and the tag: `gh release delete v0.1.0-beta.0 --yes` and
  `git push --delete origin v0.1.0-beta.0`.
- Re-running the release job for a tag that already has a release fails at `gh release create`.
  Delete the draft first.
- To attach Linux after P4-008 passes: add a `download-artifact` step for
  `<name>-<ver>-x86_64-linux-gnu`, add the `.deb` to the `sha256sum` and `gh release create` lists,
  and change the Linux paragraph of the notes.
- Bumping the core means updating `CORE_VERSION` in `release.yaml`, or the release fails fast.
- Doc gaps (P4-012): `docs/integrations.md` *Releases* (tag rule, the `RELEASE_FINAL_ALLOWED`
  variable, draft-only, assets, checksum verification, notes, how to delete a test draft).

## Handoff for downstream review
- Acceptance criteria addressed: validate before build; Release config build; a single write-scoped
  job; `gh release create --draft` (`--prerelease` for suffixes); plain final tags refused without
  the variable; `SHA256SUMS.txt`; notes with the install link, core `v0.7.0`, Linux status and
  source links; no `.deb` on the release.
- Areas to scrutinize: unquoted `NOTES` heredoc (backticks escaped); `release-version` keeps the
  suffix and is used only in the title; asset names use the buildspec version without the suffix
  (C-017).
- How to run: push a tag such as `v0.1.0-beta.0`; watch workflow **Release**: **Validate tag**,
  **Build / macOS arm64**, **Build / Linux x86_64**, **Create draft release**.
