# SynesthesiaVibe Visualizer for OBS (native plugin)

A music visualizer source for OBS Studio on macOS. It adds **SynesthesiaVibe Visualizer** to
OBS's list of sources. The source runs the [SynesthesiaVibe](https://github.com/lee5783/synesthesia-vibe)
core (a MilkDrop-preset renderer) inside OBS's own renderer, plays `.milk` presets from a folder
you choose and reacts to an OBS audio source or to OBS's audio mix.

Unlike the [Browser Source page](../browser-source/README.md), the plugin hears OBS's own audio
directly. You need no loopback device, no local server and no command-line flag.

**Known limits**

- Apple Silicon Macs only. Intel Macs and Windows are not supported yet. A Linux build exists but
  is experimental and untested in OBS ([Linux](#linux-experimental-untested-in-obs)).
- OBS must use its **OpenGL** renderer. On any other renderer the source does not appear (see
  [Troubleshooting](#troubleshooting)).
- No presets come with the plugin. You choose a folder of `.milk` presets
  ([Presets](#presets-where-to-get-them-and-their-terms)).
- All visualizers in one OBS share their audio analysis
  ([Several visualizers](#hidden-sources-several-visualizers-and-performance)).
- Checked on OBS 33.0.3 on an Apple M1, with the OpenGL renderer.

## Contents

1. [What you need](#what-you-need)
2. [Download](#download)
3. [Step 1: install the plugin](#step-1-install-the-plugin)
4. [Step 2: check that OBS uses OpenGL](#step-2-check-that-obs-uses-opengl)
5. [Step 3: add the source](#step-3-add-the-source)
6. [Settings](#settings)
7. [Hotkeys](#hotkeys)
8. [Presets: where to get them and their terms](#presets-where-to-get-them-and-their-terms)
9. [Hidden sources, several visualizers and performance](#hidden-sources-several-visualizers-and-performance)
10. [Reading the OBS log](#reading-the-obs-log)
11. [Troubleshooting](#troubleshooting)
12. [Uninstall](#uninstall)
13. [Linux (experimental, untested in OBS)](#linux-experimental-untested-in-obs)
14. [For developers](#for-developers)
15. [Credits and licences](#credits-and-licences)

## What you need

- A Mac with Apple Silicon (M1 or later).
- OBS Studio 31 or later (checked on 33.0.3).
- A folder of MilkDrop `.milk` presets. See
  [Presets](#presets-where-to-get-them-and-their-terms).
- The release files from this repository's GitHub Releases page: a `.pkg` installer and a `.zip`
  holding the plugin bundle ([Download](#download)).

## Download

Releases are on the [GitHub Releases page](https://github.com/lee5783/synesthesia-vibe-obs/releases).
Each release has three files, where `<version>` is the plugin version (for example `0.1.0`):

| File | What it is |
|---|---|
| `synesthesia-vibe-obs-<version>-macos-arm64.pkg` | The macOS installer, Apple Silicon only ([Option A](#option-a-the-installer-pkg)). |
| `synesthesia-vibe-obs-<version>-macos-arm64.zip` | The same plugin bundle for a manual install ([Option B](#option-b-the-zip)). |
| `SHA256SUMS.txt` | The SHA-256 checksums of the two files above. |

- **Releases and pre-releases.** A release is named after the plugin version (for example
  *SynesthesiaVibe Visualizer 0.1.0*). Test builds are marked **Pre-release** on GitHub and their
  name ends in `-beta.<n>` or `-rc.<n>`. The file names carry only the plugin version.
- **Signed and notarized.** The installer and the plugin are signed with a Developer ID and
  notarized by Apple, so macOS opens them without a warning.
- **No Linux file.** Releases have no Linux package yet. See
  [Linux](#linux-experimental-untested-in-obs).

**Check the download.** Put `SHA256SUMS.txt` in the same folder as the files you downloaded, then
run, in Terminal:

```sh
cd ~/Downloads
shasum -a 256 --ignore-missing -c SHA256SUMS.txt
```

Every file you downloaded must show `OK`. `--ignore-missing` skips the file you did not download.
Any other result means the file is damaged or not the one on the release: delete it and download it
again.

## Step 1: install the plugin

The plugin is a bundle named `synesthesia-vibe-obs.plugin`. OBS loads it from your user plugins
folder:

```
~/Library/Application Support/obs-studio/plugins/
```

Quit OBS before you install, and install **one** way: the installer or the zip.

### Option A: the installer (`.pkg`)

The installer puts `synesthesia-vibe-obs.plugin` into the plugins folder above, for your user
account only. It needs no administrator password. Next to it, it also puts
`synesthesia-vibe-obs.plugin.dSYM`, the plugin's debug symbols (about 1.4 MB). OBS ignores that
file; [Uninstall](#uninstall) removes it too.

Double-click the `.pkg` and follow the installer to the end. If it asks where to install, choose
**Install for me only**.

From Terminal, this does the same (use the file name from the release):

```sh
installer -pkg ~/Downloads/<installer>.pkg -target CurrentUserHomeDirectory
```

### Option B: the zip

1. Unzip the release zip. You get `synesthesia-vibe-obs.plugin`.
2. Create the plugins folder if it does not exist yet, and move the bundle into it:

   ```sh
   mkdir -p ~/Library/Application\ Support/obs-studio/plugins
   mv ~/Downloads/synesthesia-vibe-obs.plugin ~/Library/Application\ Support/obs-studio/plugins/
   ```

   In Finder: Go → Go to Folder…, paste the folder path above, and drag the bundle in.
3. The bundle is notarized, so OBS should load it as downloaded. If OBS does not load it, remove
   macOS's download (quarantine) flag and restart OBS:

   ```sh
   xattr -dr com.apple.quarantine ~/Library/Application\ Support/obs-studio/plugins/synesthesia-vibe-obs.plugin
   ```

### Upgrading

Quit OBS, then install the new release the same way. With the zip, delete the old `synesthesia-vibe-obs.plugin` from the plugins folder first, then copy the
new one in. Your sources and their settings stay in your scene collection.

## Step 2: check that OBS uses OpenGL

The core renders with OpenGL only, so the plugin needs OBS's OpenGL renderer. On a Mac, OBS uses
OpenGL unless you switched it.

1. Settings → Advanced → Video → **Renderer**.
2. If it shows anything other than **OpenGL** (for example Metal), choose **OpenGL**, click OK and
   restart OBS.

To check it worked, open the OBS log (Help → Log Files → View Current Log) and look for:

```
[synesthesia-vibe-obs] renderer check passed (OpenGL, device type 1)
[synesthesia-vibe-obs] plugin loaded successfully (version 0.1.0)
```

The version number is the one of the release you installed.

## Step 3: add the source

1. In the **Sources** dock, click **+** and choose **SynesthesiaVibe Visualizer**.
2. Name it, and click OK. The properties open.
3. Under **Presets**, click **Browse** next to **Preset folder** and choose your folder of `.milk`
   presets. The first preset appears at once.
4. Under **Audio**, choose what the visuals react to: an audio source, or **Whole mix**.
5. If your canvas is not 1920×1080, set **Width** and **Height** under **Output** to match it (for
   example 1080×1920 for a portrait canvas).
6. Click OK.

Until you choose a folder, the source shows the core's built-in "no preset" image: a grey wave and
mesh on an opaque background.

## Settings

The properties are in four groups. Changes take effect at once.

### Presets

| Setting | Default | What it does |
|---|---|---|
| **Preset folder** | none | The folder the presets come from. Only `.milk` files directly inside it are used, not files in subfolders. Hidden files are skipped, and the extension may be any case. |
| **Preset** | **(first preset)** | The preset to show now. The list holds **(first preset)** and then every preset in the folder by name (the file name without `.milk`), sorted A–Z with case ignored. **(first preset)** always means the first file in that order, even after you add or remove files. |

- When the folder is not set, does not exist or holds no `.milk` file, the **Preset** list is
  replaced by the warning *Choose a folder of .milk presets.* Whatever preset was already showing
  keeps running.
- The folder is not watched. Files you add show up in the list the next time you open the
  properties.
- When you choose another folder and the selected preset is not in it, the selection goes back to
  **(first preset)**.
- The **Preset** list shows your last choice. It does not follow auto-cycle or the hotkeys, so the
  preset on screen can differ from the one in the list.
- A preset that fails to load is skipped: the one on screen stays, and the OBS log says why
  ([Troubleshooting](#troubleshooting)).

### Transitions

| Setting | Default | Range | What it does |
|---|---|---|---|
| **Change preset every (0 = off)** | `16 s` | `0`–`3600 s` | How long each preset plays on its own before the next one starts blending in. `0` turns auto-cycle off: the preset stays until you pick another or press a hotkey. |
| **Blend time** | `2.7 s` | `0`–`30 s`, in steps of `0.1` | How long the cross-fade into the next preset takes. `0` is a hard cut. Applies to every change: auto-cycle, the hotkeys and picks from the **Preset** list. Only two changes skip the blend: the first preset a new source shows, and the next change after a `preset failed to commit` error in the log. |
| **Random order** | off | on / off | Off: auto-cycle and **Next preset** go through the folder in list order and start again at the top. On: they pick a random preset, never the one on screen (when the folder has two or more). |

**How the two times add up.** A preset changes every **cycle + blend** seconds. Each preset plays
alone for the cycle time after its blend has finished, then the next one starts blending in. With
the defaults (16 s and 2.7 s) that is a change about every 18.7 s. For a change every 30 s with a
2 s blend, set 28 and 2.

When you change the times, the change already scheduled for the current preset keeps the old
timing; the next ones follow the new values. If you turn auto-cycle back on after it was off for
longer than the cycle time, the next preset may start at once.

### Audio

| Setting | Default | What it does |
|---|---|---|
| **Audio source** | **None** | What the visuals react to. **None**: no audio of their own (but see [Several visualizers](#hidden-sources-several-visualizers-and-performance)). **Whole mix**: one of OBS's audio tracks (see **Mix track**). Below those, every OBS source that has audio, by name: a microphone, a Media Source, desktop audio and so on. |
| **Mix track** | **Track 1** | Shown only with **Whole mix**. Which of OBS's six audio tracks (**Track 1**–**Track 6**) to listen to. |

- **A single source.** The visuals follow that source's sound. A muted source counts as silence,
  as it does on the stream. The source is remembered by its identity, not its name, so renaming it
  is fine. If you delete it and create a new one with the same name, the list shows
  **(missing)** and the visuals go silent: choose the new source.
- **Whole mix.** The visuals follow what OBS mixes into that track, which is what the stream or
  recording on that track hears. So the mix leaves out muted sources, sources set to
  **Monitor Only** (Edit → Advanced Audio Properties → Audio Monitoring), and sources whose
  checkbox for that track is off in Advanced Audio Properties.
- When OBS starts, a chosen source may load after the visualizer. The visualizer checks again every
  second and attaches as soon as the source exists.
- When the audio stops, the visuals settle back to silence within about half a second.

### Output

| Setting | Default | Range | What it does |
|---|---|---|---|
| **Width** | `1920 px` | `128`–`8192 px` | The source's width. |
| **Height** | `1080 px` | `128`–`8192 px` | The source's height. Set 1080×1920 for a portrait canvas. |
| **Render scale** | **100 % (full)** | **100 % (full)**, **75 %**, **50 %** | Renders the visualizer at a fraction of the size and stretches it to fill the source. The source keeps its size in the scene, so your layout does not move. |

- Use **Render scale** when the visualizer is large or OBS skips frames. At 8192×8192 on an M1,
  100 % reached only about 21 fps; 50 % kept 30 fps. At 1920×1080 the default 100 % is fine on an
  M1.
- Changing the size or the render scale restarts the visualizer's image, which takes about a second
  to build up again. The preset itself does not change.

## Hotkeys

Each visualizer source has four hotkeys. Bind them in Settings → Hotkeys, where they are listed
under the source's name:

| Hotkey | What it does |
|---|---|
| **Next preset** | The next preset in list order, or a random one when **Random order** is on. |
| **Previous preset** | Goes back to the presets that played before, most recent first, up to the last 50 changes. With no history left, it takes the previous file in list order. |
| **Random preset** | A random preset other than the one on screen. |
| **Pause/resume auto-cycle** | Stops automatic preset changes, or starts them again. |

- Next, previous and random blend with the **Blend time**.
- While auto-cycle is paused, the three preset hotkeys and the **Preset** list still work, and
  auto-cycle stays paused.
- If a change came due while auto-cycle was paused, resuming makes it at once. A change that was
  already being prepared when you pressed pause still happens once.
- Pause is per source and is not saved: every visualizer starts unpaused when OBS starts.
- Hotkeys also work while the source is hidden. The new preset appears when the source is shown.
- Several presses in quick succession can merge into one change while a preset is loading.

## Presets: where to get them and their terms

No presets or textures come with the plugin. Some of the presets this project uses may not be
used commercially, so the plugin leaves the choice of presets to you.

- **Where to get them.** The [SynesthesiaVibe core repository](https://github.com/lee5783/synesthesia-vibe)
  has a small tested set in `resources/mvp-presets/` (10 presets) and their textures in
  `resources/beatdrop/textures/`. They come from the
  [BeatDrop](https://github.com/OfficialIncubo/BeatDrop-Music-Visualizer) distribution, which has
  many more. Any MilkDrop 2 `.milk` preset may work; not every one compiles.
- **Textures go next to the presets.** Some presets use image textures by name. The core looks
  for them in the preset's own folder, so copy the texture files (`.jpg`, `.png`) into the same
  folder as the `.milk` files, not into a subfolder.
- **Random textures do not work yet.** A few presets ask for a *random* texture instead of a named
  one, for example *Stahlregen & Flexi + Geiss - Tiger No.5 (Random texture flow)*. With the
  plugin these get no texture, and look different from the original.
- **Licence terms.** Each preset's file name carries its authors' names. BeatDrop's preset notice
  says:

  > Majority of ShaderToy shader MilkDrop presets are licensed with CC-BY-NC-SA 3.0 […] You aren't
  > allowed to use these ShaderToy converted presets for commercial purposes. Use them with care and
  > you must credit shader authors of you have used them.

  If your stream earns money, check whether that counts as commercial use before you show these
  presets, and credit the preset authors. The BeatDrop textures are under BeatDrop's BSD 3-Clause
  licence.

## Hidden sources, several visualizers and performance

- **Hidden sources.** When the source is hidden (the eye icon in the Sources dock) or its scene is
  not showing, OBS does not draw it, and the visualizer stops rendering and uses no GPU. When it
  is shown again it carries on smoothly, with no jump.
- **Several visualizers share their audio.** The core keeps one audio buffer for the whole OBS
  process, so every visualizer reacts to the audio of all of them together. Two visualizers with
  different audio sources both react to both sources, and a visualizer set to **None** still moves
  while another one has audio. Use one audio choice for all your visualizers. This needs a change
  in the core and is tracked as an upstream request.
- **Several visualizers cost GPU each.** Each source renders on its own. To show the same
  visualizer in several scenes, add the **existing** source to each scene (click **+**, choose
  **SynesthesiaVibe Visualizer**, then **Add Existing**) instead of creating a new one.
- **Creating a visualizer** compiles its first preset, which can make OBS skip a frame or two. Do it
  before you go live.

## Reading the OBS log

The plugin writes its messages to the OBS log, with lines that start with
`[synesthesia-vibe-obs]`. Open it with Help → Log Files → View Current Log. It logs one line per
event: a preset loaded, an audio source attached, a hotkey pressed. Nothing is logged per frame.

Lines you see when everything works:

```
[synesthesia-vibe-obs] renderer check passed (OpenGL, device type 1)
[synesthesia-vibe-obs] plugin loaded successfully (version 0.1.0)
[synesthesia-vibe-obs] sv_init_gl and sv_create succeeded (1920x1080)
[synesthesia-vibe-obs] audio source attached: Mic/Aux (48000 Hz to 44100 Hz stereo)
[synesthesia-vibe-obs] auto-cycle: every 16 s, blend 2.7 s, sequential order
[synesthesia-vibe-obs] preset committed: <folder>/shifter - swarm.milk (blend 0.0 s, 9.4 ms)
```

The source name, folder, sizes and times are yours.

## Troubleshooting

**SynesthesiaVibe Visualizer is not in the + list**

- The log says
  `[synesthesia-vibe-obs] SynesthesiaVibe Visualizer needs the OpenGL renderer; current renderer is … The source is not registered.`
  → OBS is not using OpenGL. Switch the renderer ([Step 2](#step-2-check-that-obs-uses-opengl))
  and restart OBS.
- No `[synesthesia-vibe-obs]` line at all → OBS did not load the plugin.
  - Check that `synesthesia-vibe-obs.plugin` is directly in
    `~/Library/Application Support/obs-studio/plugins/`, not in a subfolder.
  - If you installed from the zip, remove the quarantine flag ([Option B](#option-b-the-zip),
    step 3) and restart OBS.
  - Check that the Mac has Apple Silicon and OBS is 31 or later.

**The source shows a grey wave and mesh, not a preset**
No preset has loaded yet. The log says which:

- `preset folder not set; choose a folder of .milk presets` → choose a **Preset folder**.
- `preset folder not found: …` → the folder was moved, renamed or deleted. Choose it again.
- `preset folder has no .milk presets: …` → the folder holds no `.milk` file directly inside it.
  Presets in subfolders are not used.

**A preset does not play, or auto-cycle skips one**
Look for `preset failed to prepare: <file> (<reason>)` or `preset failed to commit: …`. That preset
does not compile with this core. The one on screen stays. Remove the file from the folder if it
keeps coming up.

**A preset looks wrong or is missing its pictures**
Its textures are not next to it. Copy the texture files into the preset folder
([Presets](#presets-where-to-get-them-and-their-terms)). Presets that use random textures cannot get
them yet.

**The visuals run but do not react to sound**

- `audio source: none` → **Audio source** is **None**. Choose a source or **Whole mix**.
- The **Audio source** list shows **(missing)**, or the log says
  `audio source not found (…); checking again every second` or
  `audio source gone (…); audio is silent until it returns` → the chosen source was deleted. Choose
  another one.
- With a single source: check that it is not muted and that its meter in the Audio Mixer moves.
  A muted source counts as silence.
- With **Whole mix**: check the log for `audio mix connected: track <n> (44100 Hz stereo)` and that
  `<n>` is the track you want. In Edit → Advanced Audio Properties, check that your music source has
  that track ticked, is not muted, and is not set to **Monitor Only**.
- `audio mix could not be connected: track <n>` → OBS has no audio output running. Restart OBS, then
  choose **Whole mix** again.
- Microphone and macOS audio capture sources need OBS's microphone or screen-recording permission
  in System Settings → Privacy & Security. Without it, those sources are silent in OBS and so are
  the visuals.

**A visualizer set to None, or to a different source, reacts to other audio**
All visualizers share their audio analysis. See
[Several visualizers](#hidden-sources-several-visualizers-and-performance).

**OBS skips frames or the visuals stutter**
Lower **Render scale** to 75 % or 50 %, make the source smaller, or use fewer visualizers. Very
complex presets also cost more; try another one.

**The Preset list shows a different preset than the one on screen**
That is expected: the list shows your last choice and does not follow auto-cycle or the hotkeys.

## Uninstall

1. Quit OBS.
2. Delete the bundle and, if you used the installer, its debug symbols:

   ```sh
   rm -rf ~/Library/Application\ Support/obs-studio/plugins/synesthesia-vibe-obs.plugin
   rm -rf ~/Library/Application\ Support/obs-studio/plugins/synesthesia-vibe-obs.plugin.dSYM
   ```

   After a zip install there is no `.dSYM`, and the second line does nothing. You can also delete
   both in Finder from that folder (Go → Go to Folder…).
3. If you used the installer, you can also remove its receipt:

   ```sh
   pkgutil --forget "'io.github.lee5783.synesthesia-vibe-obs'"
   ```

   Type it exactly as shown, single quotes inside double quotes. The receipt's identifier
   includes the single quotes as part of its name; `pkgutil --pkgs` lists it as
   `'io.github.lee5783.synesthesia-vibe-obs'`. If you paste that name without the outer double
   quotes, the shell removes the single quotes and `pkgutil` finds no receipt.

4. Visualizer sources in your scene collections cannot render without the plugin. Remove them from
   your scenes before you uninstall, or afterwards if OBS still lists them.

Your preset folder is yours and is not touched.

## Linux (experimental, untested in OBS)

A Linux build exists, but **nobody has run it in OBS yet**. Every build is compiled, packaged and
checked automatically (the plugin loads as a library and exposes only OBS's entry points), but the
visualizer has not been tested inside OBS on X11 or Wayland. Use it only to try it out, and expect
problems.

**What you need**

- Ubuntu 24.04 on x86_64. Other distributions, other Ubuntu versions and Arm are not supported.
- OBS Studio from the OBS project's PPA, `ppa:obsproject/obs-studio`, at or above the libobs version
  the package was built against. At the time of writing that is **32.2.0**. Each build records its
  version in its run summary on GitHub, under *Linux libobs*. The package does not enforce this
  minimum itself, so check your OBS version (Help → About) before you install.
- Not supported: **Flatpak OBS** (from Flathub), which cannot see plugins installed into `/usr`, and
  Ubuntu's own `obs-studio` package, which is older than the version the plugin needs.

**Getting the package**

The Linux package is **not attached to releases**. It exists only as a build artifact of this
repository's CI:

1. Open the repository's [Actions](https://github.com/lee5783/synesthesia-vibe-obs/actions) tab,
   choose the **CI** workflow and a successful run.
2. Under **Artifacts**, download `synesthesia-vibe-obs-<version>-x86_64-linux-gnu`. GitHub asks you to
   sign in, and deletes artifacts after a while (90 days by default).
3. Unzip it. You get `synesthesia-vibe-obs-<version>-x86_64-linux-gnu.deb`.

**Install**

Quit OBS first. Install OBS from the PPA if you do not have it yet, then the package:

```sh
sudo add-apt-repository ppa:obsproject/obs-studio
sudo apt update
sudo apt install obs-studio
sudo apt install ./synesthesia-vibe-obs-<version>-x86_64-linux-gnu.deb
```

The package installs two files, for all users:

```
/usr/lib/x86_64-linux-gnu/obs-plugins/synesthesia-vibe-obs.so
/usr/share/obs/obs-plugins/synesthesia-vibe-obs/locale/en-US.ini
```

`dpkg -L synesthesia-vibe-obs` lists them. Then continue with
[Step 3: add the source](#step-3-add-the-source); the settings, hotkeys and presets work as on
macOS. OBS on Linux always uses OpenGL, so you can skip Step 2.

**The OBS log on Linux**

Help → Log Files → View Current Log, or the newest file in `~/.config/obs-studio/logs/`. Besides the
lines in [Reading the OBS log](#reading-the-obs-log), the Linux build logs which windowing system OBS
uses:

```
[synesthesia-vibe-obs] renderer check passed (OpenGL, device type 1)
[synesthesia-vibe-obs] windowing platform: X11 (EGL)
[synesthesia-vibe-obs] plugin loaded successfully (version 0.1.0)
```

On a Wayland session the second line reads `windowing platform: Wayland (EGL)`. If you try the
plugin, these lines are the most useful thing to include in a report.

**Uninstall**

Quit OBS, then:

```sh
sudo apt remove synesthesia-vibe-obs
```

This removes both files. Remove visualizer sources from your scenes as on macOS
([Uninstall](#uninstall), step 4).

**Build it yourself**

On Ubuntu 24.04 x86_64, with OBS from the PPA as above:

```sh
sudo apt install autoconf autoconf-archive automake bison build-essential cmake curl flex gettext \
  git gperf libegl-dev libgl-dev libgles2-mesa-dev libglu1-mesa-dev libltdl-dev libopengl-dev \
  libsimde-dev libtool libxcursor-dev libxinerama-dev libxkbcommon-dev libxml2-utils make \
  ninja-build obs-studio pkg-config python3 tar unzip xorg-dev xsltproc zip

git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
git -C ~/vcpkg checkout 5f96cd15fd745122cf27e0524606d6c1efc5fd07
~/vcpkg/bootstrap-vcpkg.sh -disableMetrics
export VCPKG_ROOT=~/vcpkg

git clone --recursive https://github.com/lee5783/synesthesia-vibe-obs.git
cd synesthesia-vibe-obs/plugin
cmake --preset ubuntu-x86_64 -DCMAKE_INSTALL_PREFIX=/usr
cmake --build --preset ubuntu-x86_64
cmake --build build_x86_64 --target package
```

The package is written to `plugin/release/synesthesia-vibe-obs-<version>-x86_64-linux-gnu.deb`,
next to a `-dbgsym.ddeb` holding its debug symbols. The first build compiles the core's
dependencies with vcpkg, which takes a while. Install the `.deb` as above. CMake 3.28 or later is
needed (Ubuntu 24.04's `cmake` is fine). The vcpkg commit is the one CI uses. On Linux the build
configures only on x86_64 with the Ninja generator, which the `ubuntu-x86_64` presets use.

## For developers

Building the plugin from source on macOS, the dev-only preset copy, packaging and signing are
described in [`docs/integrations.md`](../docs/integrations.md). Apart from the Linux section, this
page covers only installing and using a release.

## Credits and licences

- **This plugin** links OBS Studio's libobs and is licensed under the GNU General Public License,
  version 2 or (at your option) any later version (GPL-2.0-or-later). See [`../LICENSE`](../LICENSE).
- **SynesthesiaVibe core** ([lee5783/synesthesia-vibe](https://github.com/lee5783/synesthesia-vibe))
  is MIT-licensed. Its licence and third-party notices (BeatDrop, MilkDrop 2 and others) are in that
  repository's `LICENSE` and `NOTICE`.
- **Presets and textures** are not part of this plugin. See
  [Presets](#presets-where-to-get-them-and-their-terms) for where they come from and their terms.
