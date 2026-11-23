# LIC-001

## Subtasks executed
- Fetched the canonical GPL-2.0 text from gnu.org into the root `LICENSE`, verbatim, no header.
- Sanity-checked it: the title lines, "END OF TERMS AND CONDITIONS", LF line endings, `wc -l`, sha256.
- Locked D1 in the `PLAN.md` Decisions table and removed the "No `LICENSE` file until then…" sentence.
- Made a minimal update to the Risks row "GPL vs the MIT core and vendored code" so it no longer says "Settle D1 before Phase 2".

## Files changed
- `LICENSE` (new)
- `PLAN.md` (D1 row State cell; Risks row Mitigation cell)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/input-dev.md` (LIC-001 section appended)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/output-LIC-001.md` (this file)
- `.claude/working-sessions/20261123004446-browser-source-phase-1/develop/evidence/LIC-001/LIC-001-license.log`

## What changed and why
The user locked D1 at Gate 1: GPL-2.0-or-later, with a LICENSE file in this run. `LICENSE` is the
gnu.org `gpl-2.0.txt` exactly as `curl -fsSL … -o LICENSE` wrote it. That is the current gnu.org
revision, with the `<https://fsf.org/>` address line: 338 lines, 17984 bytes, sha256
`edaef632cbb643e4e7a221717a6c441a4c1a7c918e6e4d56debc3d8739b233f6`. The D1 State cell now
says Locked (2026-11-23), using the wording from the task section. "License" keeps PLAN.md's
American spelling. The "or any later version" grant is recorded in the D1 row, per the C-021
recommended default. There are no SPDX lines and README.md is unchanged. The status line already
read "Last updated 2026-11-23", so it was left alone.

## Verification
- `curl -fsSL https://www.gnu.org/licenses/old-licenses/gpl-2.0.txt -o LICENSE` exited 0.
- `head`: "GNU GENERAL PUBLIC LICENSE / Version 2, June 1991". "END OF TERMS AND CONDITIONS" is on
  line 280. `file` reports ASCII text, and the CR count is 0 (LF only).
- `wc -lc LICENSE`: `338 17984`.
- A second, independent fetch from gnu.org to compare against **failed**. Right after the first
  fetch, www.gnu.org stopped accepting connections: six attempts over IPv4 and IPv6 all gave
  `curl: (28) Failed to connect … Timeout was reached`. As a substitute cross-check, `LICENSE` has
  the same sha256 as, and passes `cmp` against, the GPLv2 text shipped by two GNU-distributed
  packages on this machine: Homebrew `autoconf/2.73/COPYING` and `xz/5.8.4/COPYING.GPLv2`.
- `git diff --stat`: only `PLAN.md` (2 insertions, 2 deletions). `LICENSE` is the only other new
  file this task adds at the root (shows as `??`).

## Evidence
- `evidence/LIC-001/LIC-001-license.log`: the fetch command, the failed re-fetch, the cross-check
  sha256 and cmp, `wc`, `file`, CR count, head and tail, `git diff --stat`, and the full
  `git diff PLAN.md`.

## Known limitations / follow-ups
- The comparison against a second gnu.org download could not be run because gnu.org became
  unreachable. Byte identity rests on the single curl write plus the match with the two GNU
  package copies. Once gnu.org responds, re-check with
  `curl -fsSL https://www.gnu.org/licenses/old-licenses/gpl-2.0.txt | shasum -a 256`, which should
  print `edaef632…33f6`.
- The `PLAN.md` D1 row points to `docs/decisions.md`, which does not exist until LIC-002
  (document-writer) creates it.
- The acceptance criterion says "nothing else in PLAN.md changed except the date line". The Risks
  row edit goes beyond that, but the delegation asked for it explicitly. The date line needed no
  change.
- C-021 is still marked *pending — second Gate 1* in `output-plan-concerns.md`. This task applies
  its recommended default, as the delegation directed.
- Doc gap for the document-writer (LIC-002): `docs/decisions.md` with D1 and the "or later" grant.
  Optional and on request only: a license line in the root README, or an SPDX line in the standard
  header (C-021).

## Handoff for downstream review
- Acceptance criteria addressed: LICENSE is the verbatim gnu.org text (with the caveat above),
  D1 reads Locked, and the git diff touches only `LICENSE` and `PLAN.md` among repo files.
- Areas to scrutinize: the wording of the D1 State cell and the Risks row.
- How to run relevant tests: `shasum -a 256 LICENSE` and `git diff PLAN.md`.
