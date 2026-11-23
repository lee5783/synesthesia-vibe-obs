# Release pipeline — from a pushed `v*` tag to a draft GitHub Release

> Part of the how-x-works corpus — see [`../HOW-X-WORKS.md`](../HOW-X-WORKS.md) for the index.
> **Analysis date:** 2026-11-23. Settings, pins, caches and the bump checklist:
> `docs/integrations.md`, *CI/CD & Deployment*. The checks each build job runs:
> `docs/testing.md`, *CI checks*.

How a tag push becomes a draft release with the macOS pkg, zip and `SHA256SUMS.txt`. Citations
are `.github/workflows/release.yaml` (`R:`) and `.github/workflows/build-plugin.yaml` (`B:`).

## 0. Before tagging (by hand)

- `plugin/buildspec.json` `version` holds the plain `X.Y.Z` of the release. Bump it in a commit
  first if needed; the prerelease suffix never goes there.
- If the core submodule moved, `CORE_VERSION` in `release.yaml` (`R:22-24`) must name the new
  core tag.
- Tag that commit `v<X.Y.Z>-beta.<N>` or `v<X.Y.Z>-rc.<N>` and push the tag. A plain `v<X.Y.Z>`
  works only once the repository variable `RELEASE_FINAL_ALLOWED` is `true`.

## 1. Trigger

- `on: push: tags: [v*]` (`R:10-13`). Top-level `permissions: contents: read` (`R:15-16`).
- `concurrency: release-<ref>` without cancelling (`R:18-20`), so two pushes of the same tag queue
  rather than race.

## 2. `validate` — refuse a bad tag before anything builds

Job `validate` (`R:27-83`), on `ubuntu-24.04`, checkout without persisted credentials.

1. *Validate tag* (`R:39-73`) gets `TAG` from `github.ref_name` and `RELEASE_FINAL_ALLOWED` from
   `vars` through `env:`.
   - The buildspec `version` must match `^[0-9]+\.[0-9]+\.[0-9]+$`.
   - The tag must match `^v(X.Y.Z)(-(beta|rc)\.N)?$`, and its `X.Y.Z` must equal the buildspec's.
   - A suffix sets `prerelease=true`. No suffix passes only when the variable is `true`, with
     `prerelease=false`. Otherwise the job fails and the error suggests a `-beta.<N>` or `-rc.<N>`
     tag.
   - Outputs: `version` (X.Y.Z, used in asset names), `release-version` (the tag without `v`,
     used in the title) and `prerelease`.
2. *Validate core version* (`R:75-83`): `git rev-parse HEAD:external/synesthesia-vibe` must equal
   the commit of `CORE_REPOSITORY` `CORE_VERSION` from `git ls-remote` (peeled tag or plain
   tag). On a mismatch the job fails and says to update `CORE_VERSION`.

## 3. `build` — the same build CI runs, in Release

Job `build` (`R:85-90`) has `needs: validate` and calls `build-plugin.yaml` with
`config: Release`.

- **macOS arm64** (`B:25-240`):
  - builds, packages and inspects;
  - uploads the artifact `synesthesia-vibe-obs-<X.Y.Z>-macos-arm64` with the pkg and zip
    (`B:233-240`).
- **Linux x86_64** (`B:242-569`):
  - builds, checks and packages;
  - uploads `…-x86_64-linux-gnu` with the `.deb` (`B:564-569`). Nothing downstream reads it
    (step 4).
- A tag run cannot read PR caches, only its own ref's and the default branch's. So a release
  usually builds vcpkg cold (about 11.5 min on Linux), and no PR can feed it a poisoned cache.
- Any failing check fails `build`, and the `release` job never runs.

## 4. `release` — checksums, notes, draft

Job `release` (`R:92-172`) needs `validate` and `build`. It is the only job with
`contents: write` (`R:98-99`).

1. *Download macOS packages* (`R:106-110`): only the macOS artifact, into `assets/`. The Linux
   artifact is deliberately not downloaded (C-006).
2. *Write checksums* (`R:112-124`):
   - fails if the pkg or zip is missing;
   - writes `SHA256SUMS.txt` with `sha256sum`;
   - verifies it with `sha256sum -c` before going on.
3. *Write release notes* (`R:126-156`): an unquoted heredoc, with backticks escaped and `TAG`
   already regex-checked. It covers:
   - the downloads list, with `shasum -a 256 --ignore-missing -c SHA256SUMS.txt`;
   - the ad-hoc / unnotarized warning, linking `plugin/README.md` at the tag;
   - the Linux status;
   - the licences;
   - links to this tag's tree and to the core tag's tree;
   - the submodule caveat for GitHub's source archives.
4. *Create draft release* (`R:158-172`): `gh release create "$TAG"` with the pkg, zip and
   `SHA256SUMS.txt`, plus `--draft --verify-tag --title "SynesthesiaVibe Visualizer
   <release-version>" --notes-file …`, and `--prerelease` when `prerelease` is `true`. `gh` uses
   `github.token`; no secret is involved.

## 5. After the run (by hand)

- Review the draft on the Releases page and click **Publish**. Until then nothing is public.
- To throw away a test run: `gh release delete <tag> --yes`, `git push --delete origin <tag>`,
  and optionally `gh cache delete <id>` for the tag-scoped caches.
- Re-running the workflow for a tag that already has a release fails in step 4; delete the draft
  first.

## Changing the pipeline

- Attaching Linux after P4-008: download the `…-x86_64-linux-gnu` artifact in step 4.1, add the
  `.deb` to steps 4.2 and 4.4, and rewrite the Linux paragraph in 4.3. Update
  `plugin/README.md` (*Download*, *Linux*) in the same change.
- Notarization would add signing inside the macOS build job, behind secrets that only tag runs
  read. The placeholder names are in `docs/integrations.md`, *Signing and notarization*.
