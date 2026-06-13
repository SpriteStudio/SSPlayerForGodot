# Building / Developing

If you want to build the GDExtension or a custom-module Godot Engine yourself, or to develop the plugin in parallel with SpriteStudio7-SDK, follow these steps.

## Overview

The flow for producing Godot binaries from this repository is as follows:

1. **Prepare `libssruntime`** — Obtain the SpriteStudio7-SDK release artifacts and place them under `ss_player/runtime/`.
2. Depending on how you want to consume the plugin, run **2-A. Build the GDExtension** or **2-B. Build the Custom-Module Godot Engine** to link the runtime above and produce Godot binaries.

## Get the Source

Clone this repository, and clone Godot Engine / godot-cpp depending on your build target.

```bash
git clone https://github.com/SpriteStudio/SSPlayerForGodot.git
cd SSPlayerForGodot
git clone https://github.com/godotengine/godot.git -b 4.6
git clone https://github.com/godotengine/godot-cpp.git -b master
```

The `godot` directory is required when building a custom-module Godot Engine.
The `godot-cpp` directory is required when building the GDExtension.

> [!NOTE]
> For typical builds that consume SDK release artifacts, the `ss_player/SpriteStudio7-SDK/` submodule does **not** need to be initialized (a non-recursive clone is fine). Only when developing/building SpriteStudio7-SDK itself, follow [For SpriteStudio7-SDK Developers](#for-ss7-sdk-developers) below and initialize the submodule with `git submodule update --init --recursive`.

## Build Environment Setup

For build tools (compiler, Python, SCons, etc.) on each platform, follow the official Godot compilation guides:

- [Windows](https://docs.godotengine.org/en/stable/engine_details/development/compiling/compiling_for_windows.html)
- [macOS](https://docs.godotengine.org/en/stable/engine_details/development/compiling/compiling_for_macos.html)
- Linux: T.B.D.

### Notes

#### Building Universal Binaries on macOS

The `molten-vk` package distributed via Homebrew only provides binaries for the host architecture, so linking fails when building with `arch=universal`. Install the universal-capable [Vulkan SDK for MoltenVK](https://vulkan.lunarg.com/sdk/home) instead.

## 1. Prepare libssruntime

Fetches and extracts the SDK package version pinned in `ss_player/SDK_VERSION.txt`.

**macOS / Linux**

```sh
./scripts/download-sdk.sh
```

**Windows (PowerShell)**

```powershell
.\scripts\download-sdk.ps1
```

> `libssconverter` (the `.sspj` → `.ssab` converter library) is bundled only for desktop platforms. The iOS / Android / Web `libssruntime` packages do not include it.

To build `libssruntime` from the SpriteStudio7-SDK source yourself, see [For SpriteStudio7-SDK Developers](#for-ss7-sdk-developers).

## 2-A. Build the GDExtension

Requires `godot-cpp` to be cloned at the `master` branch.

**macOS / Linux**

```sh
./scripts/build-extension.sh
```

**Windows (PowerShell)**

```powershell
$env:PYTHONUTF8=1
.\scripts\build-extension.ps1
```

Output is placed under `bin/<platform>/`, and the GDExtension package (including `misc/spritestudio.gdextension`) is installed into the sample projects (`examples/*/addons/spritestudio/`).

## 2-B. Build the Custom-Module Godot Engine

Requires `godot` to be cloned at the `4.6` branch.
`build.sh` / `build.ps1` invoke `scons` with `custom_modules=../ss_player`.

**macOS / Linux**

```sh
./scripts/build.sh
```

**Windows (PowerShell)**

```powershell
$env:PYTHONUTF8=1
.\scripts\build.ps1
```

Output is placed under `godot/bin/`. On macOS, `godot/Godot.app` is also created.

## Release Builds

Per-platform scripts under `scripts/` build `editor` / `template_debug` / `template_release` in one shot.
Internally they invoke `build.sh` / `build-extension.sh` repeatedly with different `target` values.
These scripts do **not** fetch or build `libssruntime`, so [1. Prepare libssruntime](#1-prepare-libssruntime) must be completed first.

### GDExtension

| Platform | Script                                       | Notes                                                |
| -------- | -------------------------------------------- | ---------------------------------------------------- |
| Windows  | `.\scripts\release-gdextension-windows.ps1`  | `arch` = host                                        |
| macOS    | `./scripts/release-gdextension-macos.sh`     | Fixed at `arch=universal`                            |
| Linux    | `./scripts/release-gdextension-linux.sh`     | `arch` = host                                        |
| iOS      | `./scripts/release-gdextension-ios.sh`       | Only `template_debug` / `template_release`           |
| Android  | `./scripts/release-gdextension-android.sh`   | Three architectures: `arm32` / `arm64` / `x86_64`    |
| Web      | `./scripts/release-gdextension-web.sh`       | `wasm32` (`threads=yes` / `threads=no`)              |

### Custom-module Godot Engine

| Platform | Script                                | Notes                                                  |
| -------- | ------------------------------------- | ------------------------------------------------------ |
| Windows  | `.\scripts\release-windows.ps1`       | `arch` = host                                          |
| macOS    | `./scripts/release-macos.sh`          | Fixed at `arch=universal`                              |
| iOS      | `./scripts/release-ios.sh`            | `arch=arm64` (device) and `arch=universal` (simulator) |
| Android  | `./scripts/release-android.sh`        | Three architectures: `arm32` / `arm64` / `x86_64`      |

> No batch release script is provided for the Linux custom module. Invoke `./scripts/build.sh platform=linux target=...` directly for each of `editor` / `template_debug` / `template_release`.

## For SpriteStudio7-SDK Developers

> The sections below are required **only when developing/customizing SpriteStudio7-SDK itself in parallel with the Godot side**. General Godot builders using the SpriteStudio7-SDK release artifacts can skip them.

### Additional Requirements

For setting up the environment to build `libssruntime` yourself (Rust toolchain, etc.), refer to the [SpriteStudio7-SDK README](https://github.com/SpriteStudio/SpriteStudio7-SDK?tab=readme-ov-file#for-sdk-developers).

If you also need to regenerate FlatBuffers headers, install `flatc` (the FlatBuffers compiler).

### Building libssruntime from source

Requires the SpriteStudio7-SDK submodule (`ss_player/SpriteStudio7-SDK/`) to be initialized. If it is not initialized yet, run:

```bash
git submodule update --init --recursive
```

Then run the script below to build the Rust runtime/converter; the artifacts are placed under `ss_player/runtime/` automatically.

**macOS / Linux**

```sh
./scripts/build-runtime.sh [platform=<platform>] [arch=<arch>] [build=<build>] [ios_simulator=<yes|no>]
```

**Windows (PowerShell)**

```powershell
.\scripts\build-runtime.ps1 [platform=<platform>] [arch=<arch>] [build=<build>] [ios_simulator=<yes|no>]
```

| Option | Values | Default |
| --- | --- | --- |
| `platform` | `windows`, `macos`, `linux`, `android`, `ios`, `web` | Host OS |
| `arch` | `x86_64`, `arm64`, `universal`, `wasm32`, etc. | Host Arch |
| `build` | `debug`, `release` | `debug` |
| `ios_simulator` | `yes`, `no` | `no` |

### Regenerating FlatBuffers headers

When `.fbs` files in SpriteStudio7-SDK have changed, regenerate the headers under `ss_player/format/` (requires `flatc`).

**macOS / Linux**

```sh
./scripts/generate-runtime-code.sh
```

**Windows (PowerShell)**

```powershell
.\scripts\generate-runtime-code.ps1
```

### SpriteStudio7-SDK internal documentation

Once the submodule is initialized, internal runtime specifications and porting notes are available at:

- `ss_player/SpriteStudio7-SDK/libs/ssruntime/docs/README.ja.md`
