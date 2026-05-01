[**日本語**](./BUILD.ja.md) | [**English**](./BUILD.md)

# Overview

Producing Godot binaries from this repository involves two stages:

1. **Prepare `libssruntime`** — Obtain the SpriteStudio7-SDK release artifacts and place them under `gd_spritestudio/runtime/`.
2. **Build the GDExtension** or **custom-module Godot Engine** — Link the runtime above to produce the Godot binaries.

# Get the Source

Clone this repository (with submodules) and clone Godot Engine / godot-cpp depending on your build target.

```bash
git clone --recursive https://github.com/SpriteStudio/SSPlayerForGodot.git
cd SSPlayerForGodot
git clone https://github.com/godotengine/godot.git -b 4.6
git clone https://github.com/godotengine/godot-cpp.git -b 4.5
```

If you forgot `--recursive`, run:

```bash
git submodule update --init --recursive
```

The `godot` directory is required when building a custom-module Godot Engine.
The `godot-cpp` directory is required when building the GDExtension.

# Build Environment Setup

## Windows

Follow the [official Godot compilation guide](https://docs.godotengine.org/en/stable/contributing/development/compiling/compiling_for_windows.html) to install the following.

* Build tools (choose one)
    * Visual Studio 2019 (recommended) or 2022
    * MSYS2 + MinGW + gcc + make
* Python 3.6 or later
* SCons 3.0 or later

SCons can be installed with:

```bat
python -m pip install scons
```

## macOS

Follow the [official Godot compilation guide](https://docs.godotengine.org/en/4.x/contributing/development/compiling/compiling_for_macos.html) to install the following.

* Xcode
* Python 3.6 or later
* SCons 3.0 or later
* Vulkan SDK for MoltenVK
* (Optional) emscripten — required for Web builds
* (Optional) Android SDK / Android NDK — required for Android builds

Everything except Xcode can be installed via [Homebrew](https://brew.sh/):

```sh
brew install python3 scons
brew install molten-vk
```

When building for an architecture other than the host (e.g. a Universal Binary), install [Vulkan SDK for MoltenVK](https://vulkan.lunarg.com/sdk/home) instead of `molten-vk`.

## Linux

T.B.D.

# 1. Prepare libssruntime

Download the SDK binaries for your target platform from the [SpriteStudio7-SDK Releases page](https://github.com/SpriteStudio/SpriteStudio7-SDK/releases) and extract them under `gd_spritestudio/runtime/` (path is relative to the repository root).

The expected layout is:

```
gd_spritestudio/runtime/
├── include/
│   ├── ssruntime.h
│   └── ssconverter.h
└── libs/
    ├── macos/        libssruntime.a, libssconverter.a            (universal binary)
    ├── ios/          libssruntime.a, libssconverter.a            (universal binary)
    ├── web/          libssruntime.a, libssconverter.a
    ├── windows/<arch>/    libssruntime.a, libssconverter.a       (e.g. x86_64)
    ├── linux/<arch>/      libssruntime.a, libssconverter.a       (e.g. x86_64)
    └── android/<arch>/    libssruntime.a, libssconverter.a       (e.g. arm64, x86_64)
```

To build `libssruntime` from the SS7-SDK source yourself, see [For SS7-SDK Developers](#for-ss7-sdk-developers).

# 2. Build the GDExtension

Requires `godot-cpp` to be cloned at the `4.5` branch.

**macOS / Linux**

```sh
./scripts/build-extension.sh
```

**Windows (PowerShell)**

```powershell
$env:PYTHONUTF8=1
.\scripts\build-extension.ps1
```

Output is placed under `bin/<platform>/`, and `misc/ssplayer_godot_extension.gdextension` is copied to `examples/feature_test_gdextension/bin`.

Main options (key=value form):

| Key        | Default                            | Description                                                              |
| ---------- | ---------------------------------- | ------------------------------------------------------------------------ |
| `arch`     | host                               | Output architecture (`universal` produces `arm64+x86_64` on Android)     |
| `platform` | host (`win`/`macos`/`linux`)       | Target platform                                                          |
| `target`   | `editor`                           | `editor` / `template_debug` / `template_release`                         |
| `cpus`     | auto-detected                      | `scons -j` parallelism                                                   |

# 3. Build the Custom-Module Godot Engine

Requires `godot` to be cloned at the `4.6` branch.
`build.sh` / `build.ps1` invoke `scons` with `custom_modules=../gd_spritestudio`.

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

Main options:

| Key        | Default   | Description                                                                            |
| ---------- | --------- | -------------------------------------------------------------------------------------- |
| `arch`     | host      | `arm64` / `x86_64` / `universal` (combined via `lipo` on macOS)                        |
| `platform` | host      | Target platform                                                                        |
| `target`   | `editor`  | `editor` / `template_debug` / `template_release`                                       |
| `cpus`     | auto-detected | `scons -j` parallelism                                                             |
| `ccache`   | `no`      | Set `yes` to enable `ccache` or `sccache` (applies a patch on macOS)                   |
| `version`  | `4.6`     | Fallback when the version cannot be detected from a git branch/tag                     |
| `strip`    | `no`      | Set `yes` to run `strip` on the output binaries                                        |

# Release Builds

Per-platform scripts under `scripts/` build `editor` / `template_debug` / `template_release` in one shot.
Internally they invoke `build.sh` / `build-extension.sh` repeatedly with different `target` values.
These scripts do **not** fetch or build `libssruntime`, so [1. Prepare libssruntime](#1-prepare-libssruntime) must be completed first.

## Custom-module Godot Engine

| Platform | Script                                | Notes                                                  |
| -------- | ------------------------------------- | ------------------------------------------------------ |
| Windows  | `.\scripts\release-windows.ps1`       | `arch` = host                                          |
| macOS    | `./scripts/release-macos.sh`          | Fixed at `arch=universal`                              |
| iOS      | `./scripts/release-ios.sh`            | `arch=arm64` (device) and `arch=universal` (simulator) |
| Android  | `./scripts/release-android.sh`        | Three architectures: `arm32` / `arm64` / `x86_64`      |

## GDExtension

| Platform | Script                                       | Notes                                                |
| -------- | -------------------------------------------- | ---------------------------------------------------- |
| Windows  | `.\scripts\release-gdextension-windows.ps1`  | `arch` = host                                        |
| macOS    | `./scripts/release-gdextension-macos.sh`     | Fixed at `arch=universal`                            |
| Linux    | `./scripts/release-gdextension-linux.sh`     | `arch` = host                                        |
| iOS      | `./scripts/release-gdextension-ios.sh`       | Only `template_debug` / `template_release`           |
| Android  | `./scripts/release-gdextension-android.sh`   | Three architectures: `arm32` / `arm64` / `x86_64`    |
| Web      | `./scripts/release-gdextension-web.sh`       | `wasm32` (`threads=yes` / `threads=no`)              |

# For SS7-SDK Developers

> The sections below are required **only when developing/customizing SS7-SDK itself in parallel with the Godot side**. General Godot builders using the SS7-SDK release artifacts can skip them.

## Additional Requirements

For setting up the environment to build `libssruntime` yourself (Rust toolchain, etc.), refer to the [SpriteStudio7-SDK README](https://github.com/SpriteStudio/SpriteStudio7-SDK?tab=readme-ov-file#for-sdk-developers).

If you also need to regenerate FlatBuffers headers, install `flatc` (the FlatBuffers compiler).

## Building libssruntime from source

Requires the SS7-SDK submodule (`gd_spritestudio/SpriteStudio7-SDK/`) to be initialized at the [Get the Source](#get-the-source) step.
Running the script below builds the Rust runtime/converter and places the artifacts under `gd_spritestudio/runtime/` automatically.

**macOS / Linux**

```sh
./scripts/build-runtime.sh
```

**Windows (PowerShell)**

```powershell
.\scripts\build-runtime.ps1
```

Main options:

| Key        | Default | Description                                                                          |
| ---------- | ------- | ------------------------------------------------------------------------------------ |
| `arch`     | host    | Output architecture (`arm64`, `x86_64`, `universal`, etc.)                           |
| `platform` | host    | `windows` / `macos` / `linux` / `ios` / `web`                                        |
| `build`    | `debug` | `debug` or `release` (`release` invokes the SDK-side `release-<platform>.sh`)        |

## Regenerating FlatBuffers headers

When `.fbs` files in SS7-SDK have changed, regenerate the headers under `gd_spritestudio/format/` (requires `flatc`).

**macOS / Linux**

```sh
./scripts/generate-runtime-code.sh
```

**Windows (PowerShell)**

```powershell
.\scripts\generate-runtime-code.ps1
```

## SS7-SDK internal documentation

Once the submodule is initialized, internal runtime specifications and porting notes are available at:

- `gd_spritestudio/SpriteStudio7-SDK/libs/ssruntime/docs/README.ja.md`
