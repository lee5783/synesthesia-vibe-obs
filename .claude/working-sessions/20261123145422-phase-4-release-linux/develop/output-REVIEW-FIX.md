# REVIEW-FIX

Fixes for the Gate 2 review (`review/output-review-findings.md`, verdict Approve): m1, m2, m3 and
S1. m4–m7 skipped as directed. m4 was the only candidate one-liner, but it touches the Linux link
setup that has just gone green, so I left it for a later pass.

## Subtasks executed
- **m1:** release notes now say `shasum -a 256 --ignore-missing -c SHA256SUMS.txt`, the same as
  `plugin/README.md`.
- **m2:** "CI result" sections added to:
  - `evidence/P4-006-release-logic.md`: release run 45146667649, the assets and their sizes, no
    `.deb`, checksums OK, no pkg signature, cleanup;
  - `evidence/P4-005-linux-check-dry-run.md`: run 5 log excerpts;
  - `output-P4-005.md`: the run 5 summary plus the TC-C01/C02 timings table;
  - `evidence/P4-002-local-package-inspection.md`: macOS CI result and timings.
- **m3:** `plugin/README.md` *Build it yourself*:
  - vcpkg is checked out at `5f96cd15fd745122cf27e0524606d6c1efc5fd07`, as in CI;
  - the guard sentence is corrected to "configures only on x86_64 with the Ninja generator, which
    the `ubuntu-x86_64` presets use";
  - the block stays in the README.
- **S1:** *Set up environment* (both jobs of `build-plugin.yaml`) checks the buildspec `name`
  against `^[a-z0-9-]+$` and `version` against `^[0-9]+\.[0-9]+\.[0-9]+$` before anything is written
  to `GITHUB_ENV`/`GITHUB_OUTPUT`. On a mismatch it fails with an `::error::` that names the field
  and the value.

## Files changed
- `.github/workflows/release.yaml` (m1: one line in the notes heredoc)
- `.github/workflows/build-plugin.yaml` (S1: validation in both *Set up environment* steps)
- `plugin/README.md` (m3)
- `.claude/working-sessions/20261123145422-phase-4-release-linux/develop/evidence/P4-006-release-logic.md` (m2)
- `.claude/working-sessions/20261123145422-phase-4-release-linux/develop/evidence/P4-005-linux-check-dry-run.md` (m2)
- `.claude/working-sessions/20261123145422-phase-4-release-linux/develop/evidence/P4-002-local-package-inspection.md` (m2)
- `.claude/working-sessions/20261123145422-phase-4-release-linux/develop/output-P4-005.md` (m2)
- `.claude/working-sessions/20261123145422-phase-4-release-linux/develop/output-REVIEW-FIX.md`
- `.claude/working-sessions/20261123145422-phase-4-release-linux/session.log`

## Verification
- `actionlint -shellcheck=$(command -v shellcheck) .github/workflows/*.yaml`: exit 0.
- S1, with the Linux *Set up environment* script extracted verbatim and run under bash 3.2:
  - the real `plugin/buildspec.json` passes, `ASSET_BASE=synesthesia-vibe-obs-0.1.0`;
  - a name containing a newline (`"x\nEVIL=1"`) fails with exit 1 and nothing written to
    `GITHUB_ENV`. Bash `=~` anchors to the whole string, so a newline can't slip through;
  - version `0.1.0-beta.1` fails with exit 1;
  - name `Synesthesia Vibe` fails with exit 1.
  - The macOS step has the identical block.
- m1: `grep ignore-missing .github/workflows/release.yaml` finds the notes line. The rendered notes
  differ from the evidence copy only in that line.
- m3: the vcpkg commit is the one CI fetches (`VCPKG_COMMIT` in `build-plugin.yaml`) and equals
  the core's registry baseline. The guard wording matches `plugin/CMakeLists.txt`, which accepts
  Linux + `Ninja` + host `x86_64`.
- No code comments added, no machine paths, no secrets.

## Evidence
- The verification above, plus the appended *CI result* sections (they are themselves the m2
  deliverable).

## Known limitations / follow-ups
- m3's suggestion to move the build block into `docs/integrations.md` and link to it is left to
  P4-012, as the coordinator asked to keep it in the README.
- I1, the tag-scoped caches left by the deleted test tag, is the orchestrator's call
  (`gh cache delete`). Nothing to change in the workflows.
- m4–m8 not done. m4 (one variable per archive path in `plugin/CMakeLists.txt`) is a safe small
  refactor for a later pass.
- Doc hand-offs for P4-012 are unchanged from the review list.

## Handoff for downstream review
- Acceptance criteria addressed: m1, m2, m3 and S1 as asked.
- Areas to scrutinize: the S1 regex lines in YAML (`\.` inside a bash `[[ =~ ]]` in a block
  scalar). actionlint and shellcheck accept them, and the local run behaves as expected.
- CI to watch after the push: workflow **CI**, both jobs. S1 runs first in each job, so a typo would
  fail fast.
