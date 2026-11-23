# SPK-007 evidence: glad loader isolation

- Binaries:
  - **before**: `plugin/build_macos_arm64/RelWithDebInfo/synesthesia-vibe-obs.plugin`, the
    SPK-006 build, copied aside before the change.
  - **after**: the same bundle rebuilt with `plugin/exported-symbols.txt`
    (`cmake --preset macos-arm64`, then `cmake --build --preset macos-arm64`).
- OBS reference: `/Applications/OBS.app` 33.0.3 (`libobs-opengl.dylib`, arm64). Only its files
  were read. OBS was not launched, quit or changed.
- The full dump, with paths scrubbed, is in `SPK-007-symbols.txt` (git-ignored, 9,355 lines).
  The excerpts below are trimmed.

## 1. What OBS's module loader looks up (libobs 31.1.1)

From `plugin/.deps/obs-studio-31.1.1/libobs/obs-module.c`, `load_module_exports`:

```
mod->load = os_dlsym(mod->module, "obs_module_load");                 -> required
mod->set_pointer = os_dlsym(mod->module, "obs_module_set_pointer");   -> required
mod->ver = os_dlsym(mod->module, "obs_module_ver");                   -> required
mod->unload / post_load / set_locale / free_locale / name / description / author / get_string -> optional
```

From `libobs/util/platform-nix.c`, `os_dlopen`:

```
#ifdef __APPLE__
	int dlopen_flags = RTLD_LAZY | RTLD_FIRST;
	... dlopen_flags = dlopen_flags | RTLD_LOCAL;      (RTLD_GLOBAL only for "Python")
#else
	void *res = dlopen(dylib_name.array, RTLD_LAZY);
```

The plugin defines 7 of the 11 names: `load`, `set_pointer`, `ver`, `unload`, `set_locale`,
`free_locale` and `get_string`. `OBS_DECLARE_MODULE` and `OBS_MODULE_USE_DEFAULT_LOCALE` provide
some of them.

## 2. OBS's own glad (libobs-opengl.dylib, OBS 33.0.3)

```
$ otool -hv libobs-opengl.dylib
MH_MAGIC_64    ARM64        ALL  0x00       DYLIB    25       2592   NOUNDEFS DYLDLINK TWOLEVEL NO_REEXPORTED_DYLIBS
$ nm -m libobs-opengl.dylib | grep -ci glad
103
$ nm -m libobs-opengl.dylib | grep -i glad | awk '{print $3,$4,$5,$6}' | sort | uniq -c
 103 non-external (was a private
$ nm -m libobs-opengl.dylib | grep -i 'gladload\|_glad_glUseProgram'
00000000000135b0 (__TEXT,__text) non-external (was a private external) _gladLoadGL
00000000000441b8 (__DATA,__common) non-external (was a private external) _glad_glUseProgram
$ nm -gU libobs-opengl.dylib | grep -ci glad
0
```

OBS's glad (glad1, `gladLoadGL`) is fully private: OBS builds with
`CMAKE_{C,CXX}_VISIBILITY_PRESET hidden`, per `cmake/common/compiler_common.cmake`. No other
`.dylib` in `OBS.app/Contents/Frameworks`, nor `libobs` or the `OBS` executable, exports a glad
symbol (a scan with `nm -gU | grep -ci glad` found 0 in each). Nothing in OBS can bind to the
plugin's glad, and the plugin's glad cannot bind to OBS's. Note that `_glad_glUseProgram` exists
in both images under the same name, so this isolation does matter.

## 3. Plugin before the change

```
$ otool -hv synesthesia-vibe-obs
MH_MAGIC_64    ARM64        ALL  0x00      BUNDLE    26       2808   NOUNDEFS DYLDLINK TWOLEVEL WEAK_DEFINES BINDS_TO_WEAK MH_HAS_TLV_DESCRIPTORS
$ nm -gU synesthesia-vibe-obs | wc -l
    4506
$ nm -m synesthesia-vibe-obs | grep -c ' external _glad_gl'          -> 2522
$ nm -m synesthesia-vibe-obs | grep -c 'external _GLAD_GL'           -> 631
$ nm -m synesthesia-vibe-obs | grep -i gladLoad
000000000003b0f0 (__TEXT,__text) external _gladLoadGL
000000000001e4d0 (__TEXT,__text) external _gladLoadGLUserPtr
$ nm -m synesthesia-vibe-obs | grep ' external _glad_gl' | head -3
0000000000160830 (__DATA,__common) external _glad_glAccumxOES
0000000000160838 (__DATA,__common) external _glad_glAcquireKeyedMutexWin32EXT
0000000000160840 (__DATA,__common) external _glad_glActiveProgramEXT
$ nm -gU synesthesia-vibe-obs | grep -c ' _sv_'                      -> 26
$ nm -m synesthesia-vibe-obs | grep -c 'weak external'               -> 484
00000000000e3ca8 (__TEXT,__text) weak external __Z18BdUniformIntReimplIiET_RNSt3__123mersenne_twister_engine...
00000000000de984 (__TEXT,__text) weak external __Z26EvaluatePerVertexEquationsP6CStateffffbffRfS1_S1_S1_S1_S1_S1_S1_S1_S1_
$ dyld_info -fixups synesthesia-vibe-obs | grep -c weak-def          -> 492
```

Exported symbols grouped by prefix (top of the list): 2,522 `glad_gl`, 631 `GLAD_GL`, 262
`std::__1` instantiations, 194 `render::`, 128 `prjm_eval`, 14 `projectm_eval`, 14 `stbi_write`,
7 `obs_module`, 7 `NSEEL_VM`, and so on. Everything except the 7 `obs_module_*` comes from the
prebuilt `libSynesthesiaVibeCore.a`. The template's hidden-visibility preset doesn't reach it.

## 4. Plugin after the change

```
$ otool -hv synesthesia-vibe-obs
MH_MAGIC_64    ARM64        ALL  0x00      BUNDLE    26       2808   NOUNDEFS DYLDLINK TWOLEVEL BINDS_TO_WEAK MH_HAS_TLV_DESCRIPTORS
$ nm -gU synesthesia-vibe-obs
00000000000037e4 T _obs_module_free_locale
0000000000003788 T _obs_module_get_string
000000000000380c T _obs_module_load
000000000000379c T _obs_module_set_locale
0000000000003730 T _obs_module_set_pointer
00000000000038b0 T _obs_module_unload
0000000000003748 T _obs_module_ver
$ dyld_info -exports synesthesia-vibe-obs
    -exports:
        offset      symbol
        0x000037E4  _obs_module_free_locale
        0x00003788  _obs_module_get_string
        0x0000380C  _obs_module_load
        0x0000379C  _obs_module_set_locale
        0x00003730  _obs_module_set_pointer
        0x000038B0  _obs_module_unload
        0x00003748  _obs_module_ver
$ nm -m synesthesia-vibe-obs | grep -c ' external _glad_gl\| external _GLAD_GL\| external _gladLoad'   -> 0
$ nm -m synesthesia-vibe-obs | grep -c 'was a private external) _glad_gl'                              -> 2522
$ nm -m synesthesia-vibe-obs | grep -E ' _glad_glUseProgram$| _gladLoadGLUserPtr$'
00000000001489f0 (__DATA,__common) non-external (was a private external) _glad_glUseProgram
000000000001832c (__TEXT,__text) non-external (was a private external) _gladLoadGLUserPtr
$ nm -m synesthesia-vibe-obs | grep -c 'weak external'               -> 0
$ dyld_info -fixups synesthesia-vibe-obs | grep weak-def
        __DATA_CONST    __got   0x00102D40   bind  <weak-def-coalesce>/__ZTISt12length_error
        __DATA_CONST    __got   0x00102D48   bind  <weak-def-coalesce>/__ZdaPv
        __DATA_CONST    __got   0x00102D50   bind  <weak-def-coalesce>/__ZdlPv
        __DATA_CONST    __got   0x00102D58   bind  <weak-def-coalesce>/__ZdlPvSt11align_val_t
        __DATA_CONST    __got   0x00102D60   bind  <weak-def-coalesce>/__Znam
        __DATA_CONST    __got   0x00102D68   bind  <weak-def-coalesce>/__Znwm
        __DATA_CONST    __got   0x00102D70   bind  <weak-def-coalesce>/__ZnwmRKSt9nothrow_t
        __DATA_CONST    __got   0x00102D78   bind  <weak-def-coalesce>/__ZnwmSt11align_val_t
$ ls -l (Contents/MacOS/synesthesia-vibe-obs)   before 2285840 bytes, after 1938208 bytes
```

The link line has `-exported_symbols_list <repo>/plugin/exported-symbols.txt` (Xcode
`EXPORTED_SYMBOLS_FILE`). When only the list file was edited, Xcode relinked, so changes to the
list are picked up.

`WEAK_DEFINES` is gone, and the 484 core weak definitions are no longer exported. This matters
because two-level namespaces do not stop dyld from coalescing weak definitions across images.
Before the change, a second plugin carrying different code under the same mangled name (for
example another build of the core) could have replaced these functions in both plugins. The 8
binds left are libc++'s replaceable `operator new`/`delete` and one typeinfo. Any C++ image has
those, and they are coalesced on purpose.

`-dead_strip` now drops the core API the plugin never calls. The 6 undefined imports that
disappeared are `std::to_string(int)`, `__assert_rtn`, `feof`, `frexpf`, `remove` and `ungetc`.

## 5. GL entry-point resolution is unaffected

```
$ diff <(otool -L before | tail -n +2) <(otool -L after | tail -n +2)  -> identical
	/System/Library/Frameworks/OpenGL.framework/Versions/A/OpenGL (compatibility version 1.0.0, current version 1.0.0)
	@rpath/libobs.framework/Versions/A/libobs (compatibility version 1.0.0, current version 31.0.0)
	... Foundation, AVFoundation, CoreAudio, AppKit, CoreVideo, IOSurface, QuartzCore, libc++, libSystem
$ nm -u (before and after, GL and dl imports, identical)
_dlopen _dlsym _glActiveTexture _glBindBuffer _glBindFramebuffer _glBindTexture _glBindVertexArray
_glBlendEquationSeparate _glBlendFuncSeparate _glDisable _glEnable _glGetIntegerv _glIsEnabled
_glUseProgram _glViewport
$ dyld_info -fixups after | grep -E 'bind  (OpenGL|libSystem)/_(glUseProgram|glGetIntegerv|dl)'
        __DATA_CONST    __got   0x00102660   bind  OpenGL/_glGetIntegerv
        __DATA_CONST    __got   0x00102670   bind  OpenGL/_glUseProgram
        __DATA_CONST    __got   0x00102B50   bind  libSystem/_dlopen
        __DATA_CONST    __got   0x00102B58   bind  libSystem/_dlsym
```

- The plugin's own `gl_state_guard` calls go straight to `OpenGL.framework`, bound two-level.
- The core reaches GL through its now-private `glad_gl*` pointers. `gladLoadGLUserPtr` fills
  them from `opengl_get_proc`, which runs `dlsym` on `dlopen(".../OpenGL.framework/OpenGL",
  RTLD_LAZY | RTLD_LOCAL)` in `plugin/src/visualizer-source.cpp`.
- That `dlsym` targets the framework handle, not the plugin, so hiding the plugin's symbols does
  not change it.
- `glad_` prefixed pointers and the framework's `gl*` functions have different names, so they
  can never alias.

## 6. dlopen probe with OBS's flags (no OBS process involved)

A scratch C program (not in the repo) opens the binary with `RTLD_LAZY | RTLD_FIRST |
RTLD_LOCAL`, `dlsym`s every name `load_module_exports` uses plus a sample of core symbols, and
calls `obs_module_ver()`. It ran with OBS.app's `Frameworks` as its rpath so that `@rpath/libobs`
resolves.

```
                           before      after
obs_module_load            found       found
obs_module_set_pointer     found       found
obs_module_ver             found       found
obs_module_unload          found       found
obs_module_post_load       not found   not found
obs_module_set_locale      found       found
obs_module_free_locale     found       found
obs_module_name            not found   not found
obs_module_description     not found   not found
obs_module_author          not found   not found
obs_module_get_string      found       found
gladLoadGLUserPtr          found       not found
glad_glUseProgram          found       not found
GLAD_GL_VERSION_3_3        found       not found
sv_create                  found       not found
sv_render_frame            found       not found
prjm_eval_compile_code     found       not found
NSEEL_VM_alloc             found       not found
stbi_load                  found       not found
obs_module_ver() = 0x1f010001 (31.1)   (both)
```

## 7. Export-list design notes

- First attempt: the wildcard `_obs_module_*`. It linked, but it also re-exported two libobs
  imports:
  ```
  I _obs_module_get_config_path (indirect for _obs_module_get_config_path)
  I _obs_module_load_locale (indirect for _obs_module_load_locale)
  [re-export] _obs_module_get_config_path (from libobs)
  ```
- Second attempt: all 11 loader names listed explicitly. The link fails:
  ```
  Undefined symbols for architecture arm64:
    "_obs_module_author", referenced from: <initial-undefines>
    "_obs_module_description", ... "_obs_module_name", ... "_obs_module_post_load", ...
  ld: symbol(s) not found for architecture arm64
  ```
- Final: the 7 names the plugin defines. If an optional entry point is added later, it must be
  added to the list too, or it is hidden without any warning.

## 8. Signing and architecture after the rebuild

```
$ codesign -dvv synesthesia-vibe-obs.plugin
Identifier=io.github.lee5783.synesthesia-vibe-obs
CodeDirectory v=20500 size=3975 flags=0x10002(adhoc,runtime) hashes=117+3 location=embedded
Signature=adhoc
$ codesign --verify --strict synesthesia-vibe-obs.plugin   -> ok
$ lipo -info .../Contents/MacOS/synesthesia-vibe-obs       -> arm64
```

## 9. Pending: OBS load and render check (run after BS-008 releases OBS)

Not run here: BS-008 was using OBS at the same time. The orchestrator should run it, from the
repo root:

```
osascript -e 'tell application "OBS" to quit'
P="$HOME/Library/Application Support/obs-studio/plugins/synesthesia-vibe-obs.plugin"
ditto plugin/build_macos_arm64/RelWithDebInfo/synesthesia-vibe-obs.plugin "$P"
mkdir -p "$P/Contents/Resources/presets" "$P/Contents/Resources/textures"
cp external/synesthesia-vibe/resources/mvp-presets/*.milk "$P/Contents/Resources/presets/"
cp external/synesthesia-vibe/resources/beatdrop/textures/* "$P/Contents/Resources/textures/"
codesign --force --sign - "$P" && codesign --verify "$P"
nm -gU "$P/Contents/MacOS/synesthesia-vibe-obs"
open -a OBS
```

Pass criteria, from the newest file in `~/Library/Application Support/obs-studio/logs/`:

- `[synesthesia-vibe-obs] renderer check passed (OpenGL, device type 1)`
- `plugin loaded successfully (version 0.1.0)`
- The module appears under Loaded Modules. A missing required export would instead log
  `Required module function '...' not found` (at `LOG_DEBUG`, so it only shows when OBS runs with
  `--verbose`), and the module would be missing from that list
- With the visualizer in the `synesthesia-spike` collection:
  `sv_init_gl and sv_create succeeded (1920x1080)`, then the periodic `frame N, ... gl_errors 0`
  line, with the preset animating
- On quit: `plugin unloaded`
