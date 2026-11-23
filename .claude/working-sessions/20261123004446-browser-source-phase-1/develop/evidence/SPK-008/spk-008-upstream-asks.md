# SPK-008: upstream asks for `lee5783/synesthesia-vibe` (drafts, not filed)

Status: **drafts only. Nothing has been filed.** The user owns the core repo and files these when
they choose (S-011). Once one is filed, replace "to be filed" in `PLAN.md` *Upstream asks* with
`synesthesia-vibe#<n>`.

All measurements are against core `v0.7.0` (commit `887628f`), CMake 4.4.5, Xcode 27.2 and the
macOS 26 SDK on Apple Silicon, with the host plugin built against libobs 31.1.1 and run in OBS
33.0.3.

| # | Title | Confirmed by | Blocking? |
|---|---|---|---|
| 1 | Make the library-only build usable via `add_subdirectory` | SPK-002 | No. The plugin builds the core separately with `ExternalProject_Add`. |
| 2 | Document the GL state the `sv_*` calls leave changed | SPK-005, SPK-006 | No. The plugin saves and restores that state itself. |
| 3 | Hidden symbol visibility for the static library (optional) | SPK-007 | No. The plugin hides the core at its own link step. |

---

## Ask #1

**Title:** Make the library-only build usable via `add_subdirectory`

**Body:**

> Hosts that embed the core (for example an OBS plugin) would like to consume it with
> `add_subdirectory(external/synesthesia-vibe)` instead of a separate configure, build and install.
> At `v0.7.0`, with `SYNESTHESIA_VIBE_GL_DEV=ON`, `SYNESTHESIA_VIBE_BACKEND=gl`,
> `SYNESTHESIA_VIBE_BUILD_DESKTOP_TOOLS=OFF` and the vcpkg toolchain, this fails in three ways:
>
> 1. vcpkg manifest mode reads `vcpkg.json` from the parent project's root, so nothing is
>    installed and `find_package(projectM-Eval)` fails. Passing `VCPKG_MANIFEST_DIR` works around
>    it, but the manifest then also builds `glfw3` and `gtest`, which only the desktop tools use.
> 2. The core `FORCE`-sets `CMAKE_CXX_FLAGS`, `CMAKE_C_FLAGS` and their `_RELEASE` variants to
>    `""` in the cache, which wipes the parent's flags.
> 3. `${CMAKE_SOURCE_DIR}` paths resolve against the parent, so generation fails with
>    "Cannot find source file" for `vendor/glad/src/gl.c`, `src/audio/audiobuf.cpp` and four
>    others, and six targets end up with no sources.
>
> Suggested changes:
>
> - Use `CMAKE_CURRENT_SOURCE_DIR` or `PROJECT_SOURCE_DIR` instead of `CMAKE_SOURCE_DIR`.
> - Don't `FORCE` the global `CMAKE_*_FLAGS` cache entries; set per-target options instead.
> - Guard top-level-only settings with `PROJECT_IS_TOP_LEVEL`.
> - Move `glfw3` and `gtest` into a vcpkg manifest feature tied to
>   `SYNESTHESIA_VIBE_BUILD_DESKTOP_TOOLS`, so a library-only build doesn't fetch them.
> - Optionally, install an exported CMake package config for `SynesthesiaVibeCore`, so consumers of
>   the installed prefix can `find_package` it instead of declaring an imported target and the
>   framework list by hand.
>
> The separate build works today and is what we use meanwhile:
> `cmake -S external/synesthesia-vibe -B <build> -G Ninja` with the vcpkg toolchain, the
> `arm64-osx` triplet and the overlay triplets, then
> `cmake --build <build> --target SynesthesiaVibeCore` and `cmake --install <build>`. It produces
> a merged `libSynesthesiaVibeCore.a` and the header. So this ask is about convenience and
> single-configure builds, not a blocker.

---

## Ask #2

**Title:** Document the GL state the `sv_*` calls leave changed (or offer an opt-in restore)

**Body:**

> Hosts that render the core inside their own GL context (OBS, game engines) need to know which
> GL state each call leaves changed. OBS's GL device caches several bindings and skips the GL call
> when its cache matches, so any state the core leaves behind breaks the next source OBS draws.
> The header says nothing about GL state today, and the core does no save/restore.
>
> We snapshotted 250 GL items before and after every call at `v0.7.0`, across four presets
> (Rovastar - Fractopia, glowsticks v2 02 with 3D noise textures, Jester's Calling 2, swarm):
>
> - `sv_init_gl` + `sv_create` leave changed: the program, the VAO, `GL_ARRAY_BUFFER`, the draw
>   framebuffer (set to 0), the viewport (set to the context size) and the clear colour.
> - `sv_render_frame` leaves changed: the program, the VAO, `GL_ARRAY_BUFFER`, the read framebuffer,
>   blend enable and blend factors (preset-dependent), `GL_ACTIVE_TEXTURE` (always `GL_TEXTURE0`)
>   and the unit-0 2D texture. It also zeroes the 2D/3D texture binding and the sampler on every
>   unit a preset used, rather than restoring the host's bindings.
> - `sv_load_preset` and `sv_destroy` leave nothing changed.
> - Samplers and pixel store are reset by the core to GL defaults. Scissor, depth, cull, stencil
>   and colour mask show no net change.
>
> Without a restore, OBS sets uniforms on the core's program (`glUniformMatrix4fv failed …
> GL_INVALID_OPERATION`, two sets of errors per frame) and the next source can draw with blending
> off or the wrong texture. Our host now saves and restores exactly the items above around every
> call, at about 20 µs a call.
>
> Request, either of:
>
> - list this state in `synesthesia-vibe.h` as part of the API contract, or
> - add an opt-in `sv_config` flag that makes the core save and restore that state itself.
>
> Please also keep (and document) the current behaviour that the core sets every piece of state
> it relies on, on every call. Hosts change GL state between calls; four presets rendered
> correctly with OBS changing state in between.

---

## Ask #3 (optional, hygiene)

**Title:** Build the static library with hidden visibility and an `SV_API` export macro

**Body:**

> When `libSynesthesiaVibeCore.a` is linked into a shared object (an OBS plugin, for example),
> every non-static symbol in it becomes an export of that shared object unless the host adds an
> export list. At `v0.7.0` the library exports 2,522 `glad_gl*` symbols, 631 `GLAD_GL_*`,
> `gladLoadGL`/`gladLoadGLUserPtr`, projectM-Eval, ns-eel2, stb and C++ weak definitions. The
> host's `-fvisibility=hidden` does not reach a prebuilt archive.
>
> On macOS the weak definitions were the real problem: dyld coalesces them across every loaded
> image, so two plugins linking different core versions could share one copy of a core function.
> On Linux, ELF default-visibility symbols can also be preempted from the global scope.
>
> We fixed this on the host side (an exported-symbols list on macOS, a version script or
> `-Wl,--exclude-libs,ALL` on Linux), so this is not blocking. It would still help every host:
>
> - build `SynesthesiaVibeCore` with `C_VISIBILITY_PRESET hidden`,
>   `CXX_VISIBILITY_PRESET hidden` and `VISIBILITY_INLINES_HIDDEN ON`;
> - mark the public `sv_*` functions with an `SV_API` macro that sets default visibility.
>
> Hosts would then export only the `sv_*` API without an export list. The header has no
> visibility macro today.

---

## Not drafted

Low-priority items seen during the run, listed so they are not lost. None is needed:

- Annotated release tags. `v0.7.0` is a lightweight tag, so `git describe` (and
  `git submodule status`) label it `v0.5.0-19-g887628f`.
- Web dist: keep the DOMException name in `AudioInput` errors (Phase 1, C-006).
- Ship presets plus a checksum as a release asset and clarify their hosting terms (Phase 1,
  C-001/C-010).
