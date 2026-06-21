# Building / Developing

If you want to build the GDExtension or a custom-module Godot Engine yourself, or to develop the plugin in parallel with SpriteStudio-SDK, follow these steps.

## Overview

The flow for producing Godot binaries from this repository is as follows:

1. **Prepare `libssruntime`** — Obtain the SpriteStudio-SDK release artifacts and place them under `ss_player/runtime/`.
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
> For typical builds that consume SDK release artifacts, the `ss_player/SpriteStudio-SDK/` submodule does **not** need to be initialized (a non-recursive clone is fine). Only when developing/building SpriteStudio-SDK itself, follow [For SpriteStudio-SDK Developers](#for-spritestudio-sdk-developers) below and initialize the submodule with `git submodule update --init --recursive`.

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

To build `libssruntime` from the SpriteStudio-SDK source yourself, see [For SpriteStudio-SDK Developers](#for-spritestudio-sdk-developers).

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

## Debugging the Custom Module

If you want to debug the C++ code (`ss_player/`) of the plugin embedded as a custom module, follow these steps:

1. **Verify the Debug Binary**
   Godot binaries built with `target=editor` or `target=template_debug` contain debug symbols by default.
   * **macOS:** `godot/Godot.app/Contents/MacOS/Godot`
   * **Windows:** `godot/bin/godot.windows.editor.x86_64.exe` etc.
   * **Linux:** `godot/bin/godot.linuxbsd.editor.x86_64` etc.

2. **Attach Debugger and Launch Arguments**
   Specify the above binary as the launch program in your debugger (e.g., VSCode, Visual Studio, Xcode, LLDB/GDB).
   By passing the path to the target project as an argument (e.g., `--path examples/dev_module`), you can open the project directly without the project manager screen and start debugging.

**VSCode (`launch.json`) Example (macOS / LLDB):**

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Godot Custom Module",
            "type": "lldb",
            "request": "launch",
            "program": "${workspaceFolder}/godot/Godot.app/Contents/MacOS/Godot",
            "args": [
                "--path",
                "${workspaceFolder}/examples/dev_module"
            ],
            "cwd": "${workspaceFolder}"
        }
    ]
}
```

## Export Testing and Debugging

To verify and debug whether the custom module or GDExtension works correctly in the exported application, using the headless export from the CLI is convenient.

The following is the flow for building/installing templates and exporting a sample project. (The example below uses macOS)

1. **Build the runtime and templates**
   Prepare `libssruntime` beforehand, then run the release script for the target platform.
   ```bash
   # Prepare the runtime in release build (if necessary)
   ./scripts/build-runtime.sh build=release platform=macos

   # Build the export templates
   ./scripts/release-macos.sh
   ```

2. **Install templates**
   Install the built templates into the local directory recognized by Godot.
   **macOS / Linux:**
   ```bash
   ./scripts/install-template.sh macos
   ```
   **Windows (PowerShell):**
   ```powershell
   .\scripts\install-template.ps1 windows
   ```

3. **Run export from CLI**
   Use the built Godot editor (in headless mode) to invoke the export process directly from the command line.
   **macOS / Linux:**
   ```bash
   # Example: Export the dev_module project for macOS (outputs directly as .app)
   ./godot/Godot.app/Contents/MacOS/Godot --path ./examples/dev_module/ --headless --export-debug "macOS" output.app
   ```
   **Windows (PowerShell):**
   ```powershell
   .\godot\bin\godot.windows.editor.x86_64.exe --path .\examples\dev_module\ --headless --export-debug "Windows Desktop" output.exe
   ```
   > **Note:** To run the export, the target project's `export_presets.cfg` must contain the preset for the specified platform name (e.g., `"macOS"`), and required identifiers (such as Bundle ID) must be properly configured.

### Web Platform Export and Testing

Exporting for the Web produces multiple files (e.g., `.html`, `.wasm`, `.pck`), so create a dedicated directory for the export. Additionally, a local server is required to bypass browser security restrictions.

```bash
# 1. Create an output directory and export for Web
mkdir -p build_web
./godot/Godot.app/Contents/MacOS/Godot --path ./examples/dev_module/ --headless --export-debug "Web" ../../build_web/index.html

# 2. Start a local HTTP server
cd build_web
python3 -m http.server 8000
```
After starting the server, access `http://localhost:8000` in your browser to verify it works.
(Since this plugin operates with `nothread` on the Web, it can be launched with a simple HTTP server without requiring special CORS headers.)

### GDExtension Export and Testing

For GDExtensions (e.g., `dev_gdextension`), **rebuilding the engine itself or installing custom templates is unnecessary**. You can export as-is using the standard Godot editor and official export templates distributed by Godot.

1. **Release build of the GDExtension plugin**
   Run the build script for the target platform beforehand to output the libraries (e.g., `.so`, `.xcframework`, `.dll`) into the project's `addons/` directory.
   **macOS / Linux:**
   ```bash
   ./scripts/release-gdextension-macos.sh
   ```
   **Windows (PowerShell):**
   ```powershell
   .\scripts\release-gdextension-windows.ps1
   ```

2. **Run export from CLI**
   Export the project using the official Godot binary (or your own Godot command).
   **macOS / Linux:**
   ```bash
   # * Here, "godot" refers to the official Godot editor executable in your path.
   godot --path ./examples/dev_gdextension/ --headless --export-debug "macOS" output.app
   ```
   **Windows (PowerShell):**
   ```powershell
   # * "godot.exe" refers to the official Godot executable.
   godot.exe --path .\examples\dev_gdextension\ --headless --export-debug "Windows Desktop" output.exe
   ```
   > During export, Godot will automatically bundle the plugin files (`.so`, `.framework`, `.dll`, etc.) into the exported artifacts.

## Debugging the GDExtension

You can debug the C++ code for GDExtensions using almost the same steps as a custom module. The only difference is that the launch program is the "official Godot editor".

1. **Verify the GDExtension debug build**
   The normal build scripts (like `build-extension.sh`) use `target=template_debug` by default, so the output libraries (`.dll`, `.dylib`) include debug symbols.

2. **Attach Debugger and Launch Arguments**
   Specify the **official Godot binary** as the launch program in your debugger (e.g., VSCode, Visual Studio), and pass the project path as an argument (`--path examples/dev_gdextension`). The breakpoint will be hit once Godot dynamically loads the plugin at startup.

**VSCode (`launch.json`) Examples:**

**macOS / Linux (LLDB):**
```json
{
    "name": "Debug GDExtension (macOS)",
    "type": "lldb",
    "request": "launch",
    "program": "/Applications/Godot.app/Contents/MacOS/Godot", // Path to official binary
    "args": [ "--path", "${workspaceFolder}/examples/dev_gdextension" ],
    "cwd": "${workspaceFolder}"
}
```

**Windows (Visual Studio Debugger):**
```json
{
    "name": "Debug GDExtension (Windows)",
    "type": "cppvsdbg",
    "request": "launch",
    "program": "C:\\path\\to\\Godot_v4.x-stable_win64.exe", // Path to official binary
    "args": [ "--path", "${workspaceFolder}\\examples\\dev_gdextension" ],
    "cwd": "${workspaceFolder}"
}
```

## For SpriteStudio-SDK Developers

> The sections below are required **only when developing/customizing SpriteStudio-SDK itself in parallel with the Godot side**. General Godot builders using the SpriteStudio-SDK release artifacts can skip them.

### Additional Requirements

For setting up the environment to build `libssruntime` yourself (Rust toolchain, etc.), refer to the [SpriteStudio-SDK README](https://github.com/cri-middleware/SpriteStudio-SDK?tab=readme-ov-file#for-sdk-developers).

If you also need to regenerate FlatBuffers headers, install `flatc` (the FlatBuffers compiler).

### Building libssruntime from source

Requires the SpriteStudio-SDK submodule (`ss_player/SpriteStudio-SDK/`) to be initialized. If it is not initialized yet, run:

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

When `.fbs` files in SpriteStudio-SDK have changed, regenerate the headers under `ss_player/format/` (requires `flatc`).

**macOS / Linux**

```sh
./scripts/generate-runtime-code.sh
```

**Windows (PowerShell)**

```powershell
.\scripts\generate-runtime-code.ps1
```

### SpriteStudio-SDK internal documentation

Once the submodule is initialized, internal runtime specifications and porting notes are available at:

- `ss_player/SpriteStudio-SDK/libs/ssruntime/docs/README.ja.md`
