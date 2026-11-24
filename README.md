# SynesthesiaVibe for OBS

OBS Studio integration for [SynesthesiaVibe](https://github.com/lee5783/synesthesia-vibe), a
MilkDrop-preset music visualizer. It adds a visualizer to OBS that plays `.milk` presets and reacts
to your music, for streamers and recorders.

**Status:** [`v0.1.0`](https://github.com/lee5783/synesthesia-vibe-obs/releases/tag/v0.1.0) is out
for macOS on Apple Silicon. Windows support is planned; a Linux build exists but is experimental and
untested in OBS. See [`PLAN.md`](PLAN.md) for the phases, open decisions and risks.

## Two ways to use it

| | Native OBS plugin | Browser Source page |
|---|---|---|
| What it is | A **SynesthesiaVibe Visualizer** source in OBS's source list | A web page that OBS loads as a Browser Source |
| Audio | Any OBS audio source, or OBS's whole mix | One system input device, so it needs a loopback device to hear your music |
| Platforms | macOS, Apple Silicon (Linux experimental) | Every OS OBS runs on (checked on macOS) |
| Setup | Install a signed, notarized package | Download files and run a small local web server |
| Guide | [`plugin/README.md`](plugin/README.md) | [`browser-source/README.md`](browser-source/README.md) |

Use the plugin if you are on an Apple Silicon Mac. Otherwise, use the Browser Source page.

## Install the OBS plugin (macOS)

The short version. The full guide, with settings, hotkeys, troubleshooting and uninstall, is
[`plugin/README.md`](plugin/README.md).

**You need:** a Mac with Apple Silicon (M1 or later), OBS Studio 31 or later, and a folder of
MilkDrop `.milk` presets. No presets come with the plugin
([where to get them, and their licence terms](plugin/README.md#presets-where-to-get-them-and-their-terms)).

1. **Download** `synesthesia-vibe-obs-<version>-macos-arm64.pkg` and `SHA256SUMS.txt` from the
   [latest release](https://github.com/lee5783/synesthesia-vibe-obs/releases/latest). To check the
   download, run this in the folder that holds both files:

   ```sh
   shasum -a 256 --ignore-missing -c SHA256SUMS.txt
   ```

2. **Install.** Quit OBS, double-click the `.pkg` and follow the installer. If it asks where to
   install, choose **Install for me only**. The installer is signed and notarized by Apple and
   needs no administrator password. It puts the plugin in
   `~/Library/Application Support/obs-studio/plugins/`. A zip for a manual install is on the
   release page too ([Option B](plugin/README.md#option-b-the-zip)).

3. **Check the renderer.** The visualizer needs OBS's **OpenGL** renderer. Open OBS → Settings →
   Advanced → Video → **Renderer**. If it says anything else (for example Metal), choose
   **OpenGL** and restart OBS.

4. **Add the source.** In the **Sources** dock, click **+** → **SynesthesiaVibe Visualizer**.
   - **Presets:** set **Preset folder** to your folder of `.milk` presets.
   - **Audio:** pick the audio source to react to, or **Whole mix**.
   - **Output:** if your canvas is not 1920×1080, set **Width** and **Height** to match it.

If the source is missing from the **+** list, see
[Troubleshooting](plugin/README.md#troubleshooting).

## Documentation

**User guides**
- [`plugin/README.md`](plugin/README.md): install, settings, hotkeys, presets, performance,
  troubleshooting, uninstall and the experimental Linux build.
- [`browser-source/README.md`](browser-source/README.md): Browser Source setup, URL parameters,
  audio routing and troubleshooting.

**Project**
- [`PLAN.md`](PLAN.md): phases, open decisions, risks and upstream requests.
- [`docs/decisions.md`](docs/decisions.md): locked decisions (licence, core build).

**Developer reference**, indexed in [`docs/README.md`](docs/README.md):
- [`docs/stack.md`](docs/stack.md): languages, dependency versions and toolchain.
- [`docs/structure.md`](docs/structure.md): where code lives and where new code goes.
- [`docs/architecture.md`](docs/architecture.md): data and control flow, threading, the GL state
  contract.
- [`docs/integrations.md`](docs/integrations.md): building, the core submodule, packaging, signing
  and notarization, CI and the release flow.
- [`docs/conventions.md`](docs/conventions.md): naming, style and error handling.
- [`docs/testing.md`](docs/testing.md): how changes are verified.
- [`docs/concerns.md`](docs/concerns.md): known limitations, tech debt and open checks.

## Building from source

Clone with the core submodule:

```sh
git clone --recursive https://github.com/lee5783/synesthesia-vibe-obs.git
```

- **Plugin, macOS (Apple Silicon):** needs Xcode, CMake 3.28 or later, `ninja` and `VCPKG_ROOT`
  pointing at a vcpkg checkout. Then
  `cd plugin && cmake --preset macos-arm64 && cmake --build --preset macos-arm64`.
- **Browser Source:** `browser-source/fetch-dist.sh`, then
  `python3 -m http.server 8080 --bind 127.0.0.1 --directory browser-source`.

Details, the Linux build and the dev install: [`docs/integrations.md`](docs/integrations.md).

## License

This repository is licensed under the GNU General Public License, version 2 or (at your option)
any later version ([`LICENSE`](LICENSE)), because the plugin links OBS Studio's libobs. The
[SynesthesiaVibe core](https://github.com/lee5783/synesthesia-vibe) is MIT-licensed under its own
`LICENSE` and `NOTICE`. Presets are not included and carry their own terms.
