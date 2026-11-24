# Decisions

Locked decisions from [`../PLAN.md`](../PLAN.md) → *Decisions*. One section per locked decision.
Decisions still marked *Proposed* or open in `PLAN.md` (D2, D3, D5, D6) are not repeated here.

## D1 — License: GPL-2.0-or-later

**State:** Locked, 2026-11-23 (user decision).

**Decision:** this repository is licensed **GPL-2.0-or-later**: the GNU General Public License,
version 2, or (at your option) any later version.

**Rationale:** the native plugin links libobs, which is GPL-2.0-or-later, so the plugin must be
under a GPL-compatible licence. Using one licence for the whole repo keeps the Browser Source page
and the plugin consistent.

**Consequences:**
- `LICENSE` at the repo root holds the canonical GPL-2.0 text from gnu.org. That text alone grants
  "version 2" only; the **"or any later version" grant** is stated here and in the `PLAN.md` D1
  row. Source files carry no per-file SPDX line (the standard header has none). The root `README.md`
  carries a licence line (added 2026-11-24 on request). An SPDX line in the header is an optional
  follow-up on request only.
- The SynesthesiaVibe core stays **MIT** under its own `LICENSE` and `NOTICE`: the submodule
  `external/synesthesia-vibe/`, and the release files that `browser-source/fetch-dist.sh` copies
  into `browser-source/dist/` (`dist/LICENSE`, `dist/NOTICE`). Its vendored components keep their
  own notices.
- The presets and textures keep their upstream terms. They come from the BeatDrop distribution
  (textures BSD 3-Clause; many ShaderToy-converted presets CC-BY-NC-SA 3.0, no commercial use,
  attribution required), and the core approves them for local serving only. This repo fetches them
  into git-ignored folders for local use and **does not redistribute them**.

## D4 — How the plugin gets the core: pinned submodule, separate ExternalProject build

**State:** Locked, 2026-11-23 (confirmed by the user after the Phase 2 spike).

**Decision:** the core is the git submodule `external/synesthesia-vibe/`, pinned to release tag
`v0.7.0` (commit `887628f`). `plugin/CMakeLists.txt` configures, builds and installs it as a
separate `ExternalProject_Add` build (inner generator Ninja, vcpkg toolchain from `$VCPKG_ROOT`,
triplet `arm64-osx`) and links the installed `libSynesthesiaVibeCore.a` through the imported static
target `SynesthesiaVibe::Core`. The macOS XCFramework fallback is not used.

**Rationale:**
- One path for every OS: Windows and Linux have no prebuilt desktop library, so building from
  source is required there anyway.
- The spike showed the core's CMake is not subdirectory-safe: under `add_subdirectory` vcpkg reads
  the wrong manifest, the core `FORCE`-clears the parent's `CMAKE_*_FLAGS`, and its
  `${CMAKE_SOURCE_DIR}` paths resolve against the parent. A separate configure + install avoids all
  three and keeps the submodule tree clean.

**Consequences:**
- One entry point: `cd plugin && cmake --preset macos-arm64 && cmake --build --preset macos-arm64`
  (`docs/integrations.md`). Prerequisites: `VCPKG_ROOT` and `ninja`.
- The core has no CMake package config, so the plugin declares the imported target, its include
  dir and the Apple framework list itself.
- Bumping the core = checking out a new release tag in the submodule and committing the gitlink.
  `BUILD_ALWAYS TRUE` rebuilds the library; re-run the GL state audit after every bump
  (`docs/concerns.md`).
- Upstream ask #1 (`PLAN.md` → *Upstream asks*) would allow `add_subdirectory` later; it is drafted,
  not filed.
