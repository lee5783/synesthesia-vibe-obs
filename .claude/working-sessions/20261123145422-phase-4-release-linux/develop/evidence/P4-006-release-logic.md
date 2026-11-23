# P4-006 evidence: release workflow logic run locally

No tag was pushed and no release was created (the orchestrator owns git and GitHub writes). The
`release.yaml` scripts were pulled out verbatim with `P4-002-workflow-step.py` and run with
`/bin/bash -eo pipefail` from the repository root, `plugin/buildspec.json` at `0.1.0`.

## actionlint

Clean, see `P4-002-local-package-inspection.md` (0 errors in 3 files, shellcheck on).

## Tag validation (job `validate`, step *Validate tag*)

Each tag with the repository variable `RELEASE_FINAL_ALLOWED` unset and set to `true`:

```
TAG=v0.1.0 VAR='' exit=1
  ::error::final tag 'v0.1.0' is refused until the repository variable RELEASE_FINAL_ALLOWED is 'true'; tag v0.1.0-beta.<N> or v0.1.0-rc.<N> instead
TAG=v0.1.0 VAR='true' exit=0
  tag v0.1.0: version 0.1.0, prerelease false
  outputs: version=0.1.0 release-version=0.1.0 prerelease=false 
TAG=v0.1.0-beta.1 VAR='' exit=0
  tag v0.1.0-beta.1: version 0.1.0, prerelease true
  outputs: version=0.1.0 release-version=0.1.0-beta.1 prerelease=true 
TAG=v0.1.0-beta.1 VAR='true' exit=0
  tag v0.1.0-beta.1: version 0.1.0, prerelease true
  outputs: version=0.1.0 release-version=0.1.0-beta.1 prerelease=true 
TAG=v0.1.0-rc.2 VAR='' exit=0
  tag v0.1.0-rc.2: version 0.1.0, prerelease true
  outputs: version=0.1.0 release-version=0.1.0-rc.2 prerelease=true 
TAG=v0.1.0-rc.2 VAR='true' exit=0
  tag v0.1.0-rc.2: version 0.1.0, prerelease true
  outputs: version=0.1.0 release-version=0.1.0-rc.2 prerelease=true 
TAG=v0.2.0 VAR='' exit=1
  ::error::tag 'v0.2.0' names version 0.2.0, but plugin/buildspec.json says 0.1.0
TAG=v0.2.0 VAR='true' exit=1
  ::error::tag 'v0.2.0' names version 0.2.0, but plugin/buildspec.json says 0.1.0
TAG=0.1.0 VAR='' exit=1
  ::error::tag '0.1.0' must be v<X.Y.Z>, v<X.Y.Z>-beta.<N> or v<X.Y.Z>-rc.<N>
TAG=0.1.0 VAR='true' exit=1
  ::error::tag '0.1.0' must be v<X.Y.Z>, v<X.Y.Z>-beta.<N> or v<X.Y.Z>-rc.<N>
TAG=v0.1 VAR='' exit=1
  ::error::tag 'v0.1' must be v<X.Y.Z>, v<X.Y.Z>-beta.<N> or v<X.Y.Z>-rc.<N>
TAG=v0.1 VAR='true' exit=1
  ::error::tag 'v0.1' must be v<X.Y.Z>, v<X.Y.Z>-beta.<N> or v<X.Y.Z>-rc.<N>
TAG=v0.1.0-foo VAR='' exit=1
  ::error::tag 'v0.1.0-foo' must be v<X.Y.Z>, v<X.Y.Z>-beta.<N> or v<X.Y.Z>-rc.<N>
TAG=v0.1.0-foo VAR='true' exit=1
  ::error::tag 'v0.1.0-foo' must be v<X.Y.Z>, v<X.Y.Z>-beta.<N> or v<X.Y.Z>-rc.<N>
TAG=v0.1.0-beta.0 VAR='' exit=0
  tag v0.1.0-beta.0: version 0.1.0, prerelease true
  outputs: version=0.1.0 release-version=0.1.0-beta.0 prerelease=true 
TAG=v0.1.0-beta.0 VAR='true' exit=0
  tag v0.1.0-beta.0: version 0.1.0, prerelease true
  outputs: version=0.1.0 release-version=0.1.0-beta.0 prerelease=true 
```

- `v0.1.0` (plain final) is refused while the variable is unset (C-016 b), and makes a draft
  non-prerelease when it is `true`.
- `-beta.N` / `-rc.N` give `prerelease=true`, which adds `--prerelease`.
- `v0.2.0` fails before any build job, naming both versions (TC-R02).
- `0.1.0` would not even trigger the workflow (trigger `v*`); fed in directly it fails the shape check.
- `v0.1` and `v0.1.0-foo` fail with the shape message (TC-R03).
- `build` has `needs: validate`, so a failed validation starts no build.

## Core version check (step *Validate core version*)

```
$ CORE_VERSION=v0.7.0 bash -eo pipefail core.sh
core lee5783/synesthesia-vibe v0.7.0 at 887628f08dd35c1a3e984750323987f82a9ce3c6
exit=0
$ CORE_VERSION=v0.6.0 bash -eo pipefail core.sh
::error::the core submodule is at 887628f08dd35c1a3e984750323987f82a9ce3c6, but lee5783/synesthesia-vibe v0.6.0 is b01bd88b6557c9f1f0eb4eac5fe2dee10931353b; update CORE_VERSION in release.yaml
exit=1
```

So the release notes cannot name a core tag that differs from the submodule.

## Checksums (step *Write checksums*)

Run on the locally built assets (`sha256sum` stood in by `shasum -a 256`, same output format):

```
synesthesia-vibe-obs-0.1.0-macos-arm64.pkg: OK
synesthesia-vibe-obs-0.1.0-macos-arm64.zip: OK
4f15bfed0d9cf3a0a4ee416703a0d3be9d00c3369992064deb9c804e823cf259  synesthesia-vibe-obs-0.1.0-macos-arm64.pkg
6eedb84af81db4604c740f4aa329e3a2cefbd915e1976260a4f9d29f7a37ac90  synesthesia-vibe-obs-0.1.0-macos-arm64.zip
exit=0
```

The file is in the format `shasum -a 256 -c SHA256SUMS.txt` reads (TC-R05).

## Release notes (step *Write release notes*), rendered for `v0.1.0-beta.1`

```markdown
SynesthesiaVibe Visualizer 0.1.0-beta.1, a MilkDrop-preset music visualizer source for OBS Studio.

## Downloads

- `synesthesia-vibe-obs-0.1.0-macos-arm64.pkg`: macOS installer, Apple Silicon only.
- `synesthesia-vibe-obs-0.1.0-macos-arm64.zip`: the same plugin bundle for a manual install.
- `SHA256SUMS.txt`: check the downloads with `shasum -a 256 -c SHA256SUMS.txt`.

## Before you install

The macOS plugin is ad-hoc signed and not notarized, so macOS Gatekeeper warns about it. Follow the install steps in [plugin/README.md](https://github.com/lee5783/synesthesia-vibe-obs/blob/v0.1.0-beta.1/plugin/README.md).

## Linux

Linux is built and checked in CI but has not been tested in OBS yet, so no Linux package is attached to this release.

## Source

The plugin is GPL-2.0-or-later and includes the MIT-licensed SynesthesiaVibe core v0.7.0.

- This release: https://github.com/lee5783/synesthesia-vibe-obs/tree/v0.1.0-beta.1
- SynesthesiaVibe core v0.7.0: https://github.com/lee5783/synesthesia-vibe/tree/v0.7.0

GitHub's "Source code" archives leave out the core submodule. Get it from the core link above, or clone this repository with `git clone --recursive`.
```

## Create step (not run)

```
gh release create "${TAG}" assets/<base>-macos-arm64.pkg assets/<base>-macos-arm64.zip assets/SHA256SUMS.txt \
  --repo "${GITHUB_REPOSITORY}" --draft --verify-tag --title "SynesthesiaVibe Visualizer ${RELEASE_VERSION}" \
  --notes-file "${RUNNER_TEMP}/release-notes.md" [--prerelease]
```

Always `--draft`; `--prerelease` only for suffixed tags; the Linux `.deb` is not downloaded or
attached (C-006) and stays a workflow artifact of the `build` job. Only the `release` job has
`contents: write`; it uses `github.token` and no `secrets.*`.

## Unverified until a test tag runs

The real run: draft creation, asset upload, and the draft's asset list. Use a throwaway tag the
validation accepts, e.g. `v0.1.0-beta.0` (the Gate 1 example `v0.1.0-ci.1` is refused by design,
C-017).
