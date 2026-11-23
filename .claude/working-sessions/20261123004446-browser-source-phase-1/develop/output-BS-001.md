# BS-001

## Subtasks executed

- Created `browser-source/fetch-dist.sh` (executable, `#!/usr/bin/env bash`, `set -euo pipefail`,
  standard `#` header).
- Optional version argument with C-004's default: one `PINNED_VERSION="0.7.0"` constant. `0.7.0`
  and `v0.7.0` both normalise to tag `v0.7.0` and asset `synesthesia-vibe-web-0.7.0.zip`. Anything
  outside `[0-9A-Za-z.-]` (or starting with `.`/`-`) is rejected before any network call.
- `curl -fsL` downloads `SHA256SUMS.txt` first (so a bogus version fails fast), then the zip, into
  a `mktemp -d` work dir removed by an `EXIT` trap (`HUP`/`INT`/`TERM` route through it).
- Verification: `awk` extracts the exact line for the asset (also accepts the `*name` binary-mode
  form), then `shasum -a 256 -c`, falling back to `sha256sum -c`.
- Install: unzip into `browser-source/.dist-stage.XXXXXX` (same filesystem as `dist/`), move the
  old `dist/` into `browser-source/.dist-old.XXXXXX/dist`, rename the stage to `dist/`, delete the
  old copy. If the swap fails, the trap moves the old `dist/` back.
- Prints `Installed synesthesia-vibe web dist <ver> (<n> files) into browser-source/dist`.
- Every failure prints one `fetch-dist: …` line on stderr and exits 1.

## Files changed

- `browser-source/fetch-dist.sh` (new)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/input-dev.md` (new)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/output-BS-001.md` (new)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/BS-001/bs-001-fetch-dist-report.md` (new)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/BS-001/BS-001-fetch-dist.log` (new, git-ignored)
- `browser-source/dist/**` (generated, git-ignored)

## What changed and why

PLAN Phase 1 item 1. The page tasks (BS-003 onwards) need the core web dist at
`browser-source/dist/`, and the plan requires it to be fetched and checksum-verified, never
committed. The script resolves paths from its own directory, so it works from any cwd. The staged
rename keeps a working `dist/` in place until a verified, fully unpacked replacement is ready.

One addition beyond the plan text: an optional `FETCH_DIST_BASE_URL` environment variable
(default `https://github.com/lee5783/synesthesia-vibe/releases/download`). The acceptance criteria
ask for a tampered-copy test "without changing the committed script"; pointing this variable at a
local `file://` mirror does that. It does not change default behaviour.

## Verification

- `bash -n browser-source/fetch-dist.sh` → exit 0.
- `shellcheck browser-source/fetch-dist.sh` → exit 0, no findings.
- Ran under `/bin/bash` 3.2.57 (macOS stock). The script uses no bash-4 features: no associative
  arrays, no `${var,,}`, no `mapfile`.
- No existing test suite covers this area.

## Evidence

- `evidence/BS-001/bs-001-fetch-dist-report.md` quotes every run:
  - fresh install from the repo root: 15 files, `VERSION` = `0.7.0`, exit 0
  - re-runs from inside `browser-source/` with no argument and with `v0.7.0`: exit 0, no leftovers
  - bogus version `9.9.9`: `not found (HTTP 404)…`, exit 1
  - invalid version `../../etc`: rejected, exit 1
  - tampered zip through `FETCH_DIST_BASE_URL=file://…`: `checksum mismatch…`, exit 1
  - sums file with no entry for the zip: exit 1
  - unreachable host: exit 1
  - extra argument: usage line, exit 1
  - `dist/` inode and content digest identical before and after all six failure runs
  - `git status` shows only `fetch-dist.sh`, and `git check-ignore` matches `.gitignore:10:browser-source/dist/`
  - `sha256sum` fallback (PATH with no `shasum`): success and mismatch paths both work
  - SIGTERM mid-run: temp dir cleaned, `dist/` unchanged
- `evidence/BS-001/BS-001-fetch-dist.log`: the raw log (git-ignored).

## Known limitations / follow-ups

- Not run on `ubuntu-latest` (no Docker or Linux runner here). The fallback path was exercised with
  macOS's `/sbin/sha256sum`, not GNU coreutils. GNU `sha256sum -c` takes the same
  `<hash>  <name>` line, so it should behave the same, but that is unverified.
- The checksum comes from the same release as the zip, so it catches corruption and partial
  downloads, not a compromised release. Pinning the expected hash in the script would close that
  gap but adds a second value to bump per version. Not done; the plan did not ask for it.
- The swap is two renames (old `dist/` aside, stage in), not one atomic syscall. A web server
  reading during that window could see `dist/` missing for a moment. That does not matter for
  local serving.
- A `SIGKILL` or power loss during install can leave a `browser-source/.dist-stage.*` or
  `.dist-old.*` directory, which would show as untracked in `git status`. Adding
  `browser-source/.dist-*/` to `.gitignore` would hide it. Not done, to keep `.gitignore` out of
  scope (the plan allows that edit only under C-002).
- The zip contents are unzipped but never executed. `unzip` strips `../` components. Symlinks in
  the archive would be created as-is. The checksum check limits this to the published release
  asset.
- The test hook `FETCH_DIST_BASE_URL` is undocumented outside this file. The document-writer should
  mention it, and the version argument, in `docs/integrations.md` and in the BS-007 README.
- No test was added (project default).

## Handoff for downstream review

- Acceptance criteria addressed: all five BS-001 criteria (both invocation forms produce 15 files
  with `VERSION` 0.7.0; corrupted or wrong checksum exits non-zero and keeps `dist/`; non-existent
  version gives a clear message; `dist/` untracked; bash 3.2 and BSD tools). The `ubuntu-latest`
  part is unverified (see above).
- Areas to scrutinise: the `cleanup` restore logic in the trap; the `awk` line match (it requires
  an exact filename field, so it ignores lines for other assets); the `curl -w '%{http_code}'`
  error mapping (`000`/empty → generic, `404` → "not found").
- How to run: `browser-source/fetch-dist.sh [version]`. To reproduce the mismatch case, mirror
  `v0.7.0/SHA256SUMS.txt` and a modified `v0.7.0/synesthesia-vibe-web-0.7.0.zip` into a local
  directory and run with `FETCH_DIST_BASE_URL=file://<that dir>`.
