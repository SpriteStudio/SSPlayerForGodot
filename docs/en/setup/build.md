# Building / Developing

If you want to build the GDExtension or a custom-module Godot Engine yourself, or to develop the plugin in parallel with SpriteStudio-SDK, follow these steps.

## Overview

The flow for producing Godot binaries from this repository is as follows:

1. **Prepare `libssruntime`** — Obtain the SpriteStudio-SDK release artifacts and place them under `ss_player/runtime/`.
2. Depending on how you want to consume the plugin, run **2-A. Build the GDExtension** or **2-B. Build the Custom-Module Godot Engine** to link the runtime above and produce Godot binaries.

## Get the Source

Clone this repository, and clone Godot Engine / godot-cpp depending on your build target.

```bash
git clone https://github.com/cri-middleware/SSPlayerForGodot.git
cd SSPlayerForGodot
git clone https://github.com/godotengine/godot.git -b 4.7
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

#### Godot third-party build dependencies (AccessKit / ANGLE)

Godot 4.7 links two prebuilt SDKs that are not part of the engine source tree: **AccessKit** (screen reader support) and **ANGLE** (the OpenGL ES rendering driver). If they are missing, `scons` prints a warning and silently disables those drivers, producing binaries that lack features the official Godot builds ship with.

`build.sh` / `build.ps1` download them on the first desktop build (using the installers in the `godot/` checkout, so the versions always match the engine revision) into `godot/bin/build_deps/` — or `%LOCALAPPDATA%\Godot\build_deps` on Windows. Pass `deps=no` to skip the download and build without those drivers:

```sh
./scripts/build.sh deps=no
```

The mobile and web platforms do not use either dependency, so nothing is downloaded for them.

#### Building Universal Binaries on macOS

The `molten-vk` package distributed via Homebrew only provides binaries for the host architecture, so linking fails when building with `arch=universal`. Install the universal-capable [Vulkan SDK for MoltenVK](https://vulkan.lunarg.com/sdk/home) instead.

## 1. Prepare libssruntime

Fetches and extracts the SDK package version pinned in `scripts/SDK_VERSION.txt`.

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

Requires `godot` to be cloned at the `4.7` branch.
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

> [!IMPORTANT]
> Common export pitfalls (all platforms):
> - **Build the exact `target` you export with.** An `editor` build alone is not enough — `--export-debug` needs the `template_debug` library and `--export-release` needs `template_release`. If those were never built, the export still "succeeds" but ships an empty/missing extension library (on macOS this surfaces as a `CodeSign: Invalid binary format` error on the embedded framework).
> - **The extension architecture is bounded by the runtime slices you have.** The GDExtension links `libssruntime` from `ss_player/runtime/libs/<platform>/`; you can only build the architectures present there. For example, with an `arm64`-only macOS runtime you cannot produce a `universal` / `x86_64` extension — set the preset's `binary_format/architecture` to match (e.g. `arm64`). The engine template can still be `universal`; a single-arch extension just won't load on the missing arch.
> - **Enable ETC2/ASTC for `universal` / `arm64` / mobile exports.** Set `rendering/textures/vram_compression/import_etc2_astc=true` in the project (`project.godot` → `[rendering]`), otherwise the export aborts with *"Cannot export … with the ETC2/ASTC texture format disabled."*
> - **Export templates must match the editor version.** Setting `custom_template/debug` / `custom_template/release` bypasses the version check (this is how a template built from the bundled `godot/` source can be reused). Set **both** debug and release paths even for a debug-only export — Godot validates both and otherwise reports the release template as missing.

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

#### GDExtension on the Web (Extensions Support / `dlink`)

The official Godot Web export templates do **not** support GDExtension libraries. If you export the **GDExtension** build variant for the Web with a stock template, the page fails at startup with:

> GDExtension libraries are not supported by this engine version. Enable "Extensions Support" for your export preset and/or build your custom template with "dlink_enabled=yes".

To load a GDExtension on the Web, you need an engine template built with dynamic linking enabled (`dlink_enabled=yes`). Build one from the bundled `godot/` source, matching the `threads` mode of your extension (this plugin ships `nothread`, so use `threads=no`):

```bash
# Build dlink-enabled Web templates from source (nothreads)
cd godot
scons platform=web target=template_debug   dlink_enabled=yes threads=no
scons platform=web target=template_release dlink_enabled=yes threads=no
# → godot/bin/godot.web.template_debug.wasm32.nothreads.dlink.zip
# → godot/bin/godot.web.template_release.wasm32.nothreads.dlink.zip
```

Then install the built templates so Godot can resolve them by name:

```bash
./scripts/install-template.sh web
# installs → <export_templates>/<version>/web_nothreads_dlink_debug.zip
#                                          web_nothreads_dlink_release.zip
```

Once these are installed, exporting only requires turning on **Extensions Support** in the Web preset — Godot auto-selects the matching `..._dlink_...` template by name, so no `custom_template` is needed. The preset side (from a plugin user's perspective) is covered in [Exporting Your Project → Web](../workflow/export.md#web).

> If you would rather not install them into Godot's templates folder, you can instead point `custom_template/debug` / `custom_template/release` in the `Web` preset directly at the built `*.dlink.zip` files.

A dlink template splits the engine into a small main module plus a large `godot.side.wasm`; after export you will see a corresponding `index.side.wasm` alongside `index.wasm`, which confirms the dlink template was used.

> [!NOTE]
> `dlink` / *Extensions Support* is required only for the **GDExtension** build variant. The **custom module** variant compiles the plugin into the engine, so its Web template is the ordinary (non-`dlink`) `web_nothreads_{debug,release}.zip` produced by `release-web.sh` (via `build.sh`) and installed the same way — with no `dlink_enabled` and no *Extensions Support* in the preset.

### Android Platform Export and Testing

Android export templates are template APKs packaged by the engine's Gradle project. Unlike the desktop platforms, the template build therefore has three stages (runtime → per-ABI engine libraries → Gradle packaging) before `install-template.sh`.

**Prerequisites**
- Android SDK and NDK, with `cargo-ndk` and the Rust Android targets installed (`aarch64-linux-android`, `armv7-linux-androideabi`, `x86_64-linux-android`).
- Export the SDK/NDK paths so the build scripts can find them (adjust to your environment):
  ```bash
  export ANDROID_HOME="$HOME/Library/Android/sdk"
  export ANDROID_NDK_ROOT="$ANDROID_HOME/ndk/<ndk-version>"
  ```
- In the Godot editor settings, configure the Android SDK path and a debug keystore (`export/android/android_sdk_path`, `export/android/debug_keystore`, `export/android/debug_keystore_pass`).

1. **Build the runtime and templates**
   Build the Rust runtime once, then build the engine shared library for each ABI/target, and package them into the template APKs with Gradle.
   ```bash
   # Rust runtime (a single release build is reused by both template targets)
   ./scripts/build-runtime.sh platform=android build=release
   # Alternatively, fetch the prebuilt runtime for all platforms instead of building it:
   #   ./scripts/download-sdk.sh

   # Engine .so per ABI. arch: arm64 / arm32 / x86_64, target: template_release / template_debug
   ./scripts/build.sh platform=android arch=arm64  target=template_release
   ./scripts/build.sh platform=android arch=arm32  target=template_release
   ./scripts/build.sh platform=android arch=x86_64 target=template_release
   ./scripts/build.sh platform=android arch=arm64  target=template_debug   # add other ABIs as needed

   # Package into android_release.apk / android_debug.apk / android_source.zip
   (cd godot/platform/android/java && ./gradlew generateGodotTemplates)
   ```
   > The `arch` values follow Godot's names (`arm64`, `arm32`, `x86_64`, `x86_32`), while the runtime libraries are placed under the matching Android ABI directories (`arm64-v8a`, `armeabi-v7a`, `x86_64`, `x86`). You only need the ABI of the device/emulator you intend to run on.

2. **Install templates**
   ```bash
   ./scripts/install-template.sh android
   ```

3. **Run export from CLI**
   ```bash
   mkdir -p bin_export
   ./godot/Godot.app/Contents/MacOS/Godot --path ./examples/dev_module/ --headless --export-debug "Android" "$(pwd)/bin_export/dev_module_debug.apk"
   ```
   > **Note:** Android export requires ETC2/ASTC texture import to be enabled. Otherwise the export aborts with a configuration error whose message is empty. Set `rendering/textures/vram_compression/import_etc2_astc=true` in the project settings (already enabled in `examples/dev_module`).

4. **Install and run on a device / emulator**
   ```bash
   adb install -r bin_export/dev_module_debug.apk
   adb shell am start -n com.crimw.devmodule/com.godot.game.GodotAppLauncher
   adb logcat -s godot        # check the engine log / errors
   adb exec-out screencap -p > screen.png   # capture the rendered frame
   ```
   The `dev_module` sample plays the `Knight_arrow` animation on `SpriteStudioPlayer2D` with autoplay, so a successful run renders the character on screen.

### iOS Platform Export and Testing

iOS export requires **macOS with Xcode**. Godot generates an **Xcode project** (not a ready-to-run app) and then auto-runs `xcodebuild archive` for a device, which needs an Apple Developer account — so an **App Store Team ID must be set in the preset**, and the device-archive step fails without valid signing. For a quick, unsigned check, build the generated project for the **iOS Simulator** yourself.

> [!IMPORTANT]
> **Self-built iOS templates do not bundle MoltenVK.** Godot's iOS engine is built with Vulkan, so the app binary hard-links `@rpath/MoltenVK.framework/MoltenVK` (confirm with `otool -L`). This is a property of the *engine build*, not the project's renderer — a GL Compatibility project links it too. The **official** Godot iOS templates ship `MoltenVK.xcframework`; the template produced by `release-ios.sh` does **not**. When testing a self-built template you must supply it yourself, or the app crashes at launch with `Library not loaded: @rpath/MoltenVK.framework/MoltenVK`.

Simulator smoke-test of a self-built template (custom module shown; the GDExtension flow is identical with `dev_gdextension`):

```bash
# 1. Export the Xcode project (Godot's device-archive step will fail on signing — that's expected)
mkdir -p bin_export/ios
./godot/Godot.app/Contents/MacOS/Godot --path ./examples/dev_module/ --headless \
    --export-debug "iOS" "$(pwd)/bin_export/ios/dev_module.xcodeproj" || true

# 2. Place MoltenVK where the Xcode project expects it (e.g. from the Vulkan SDK)
MVK="$HOME/VulkanSDK/<ver>/iOS/lib/MoltenVK.xcframework"
cp -R "$MVK" bin_export/ios/MoltenVK.xcframework

# 3. Build for the Simulator, unsigned
cd bin_export/ios
xcodebuild -project dev_module.xcodeproj -scheme dev_module \
    -sdk iphonesimulator -configuration Debug -derivedDataPath ./DerivedData \
    CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO build
APP="DerivedData/Build/Products/Debug-iphonesimulator/dev_module.app"

# 4. Embed MoltenVK into the app and ad-hoc re-sign (the simulator rejects the SDK's own signature)
cp -R "$MVK/ios-arm64_x86_64-simulator/MoltenVK.framework" "$APP/Frameworks/"
codesign --force --sign - "$APP/Frameworks/MoltenVK.framework"
codesign --force --sign - "$APP"

# 5. Boot a simulator, install, launch, screenshot
DEV=$(xcrun simctl list devices available | grep -m1 -oE '\([0-9A-F-]{36}\)' | tr -d '()')
xcrun simctl boot "$DEV" && xcrun simctl install "$DEV" "$APP"
xcrun simctl launch "$DEV" com.crimw.devmodule
xcrun simctl io "$DEV" screenshot screen.png
```

> **Note:** The proper fix is to bundle `MoltenVK.xcframework` into the iOS template itself (`install-template.sh` / `misc/dist/apple_embedded_xcode`) so the export embeds and signs it automatically, matching the official templates. The manual steps above are only needed for **self-built** templates — end users on the official Godot editor and official export templates are unaffected.

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

   **Android note:** Android needs a few extra steps beyond the desktop flow above:
   - Build the Android plugin libraries with `./scripts/release-gdextension-android.sh` (requires the Android SDK/NDK, `cargo-ndk`, the Rust Android targets, and the Android runtime — build it with `./scripts/build-runtime.sh platform=android build=release` or fetch the prebuilt one with `./scripts/download-sdk.sh`). The `arch` values are Godot's names (`arm64`, `arm32`, `x86_64`), while the runtime is linked from the matching Android ABI directory (`arm64-v8a`, `armeabi-v7a`, `x86_64`).
   - Also build the **host** plugin (e.g. `./scripts/release-gdextension-macos.sh`). During export the `.ssab` resources are loaded on the host machine, so without a working host library the export drops them with a `Failed loading resource` error and the resulting APK ships without animation data.
   - Install the official **Android** export templates for the official Godot version you are using (via *Manage Export Templates* in the editor, or by placing the `.tpz` contents under `.../export_templates/<version>/`).
   - Enable `rendering/textures/vram_compression/import_etc2_astc=true` in the project settings (already set in `examples/dev_gdextension`); otherwise the export aborts with a configuration error whose message is empty.
   - Then export and run on a device / emulator:
     ```bash
     mkdir -p bin_export
     godot --path ./examples/dev_gdextension/ --headless --export-debug "Android" "$(pwd)/bin_export/dev_gdextension_debug.apk"
     adb install -r bin_export/dev_gdextension_debug.apk
     adb shell am start -n com.crimw.devgdext/com.godot.game.GodotAppLauncher
     adb logcat -s godot
     ```

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
