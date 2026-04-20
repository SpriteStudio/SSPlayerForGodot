# SpriteStudioPlayer for Godot

## Minimal operating rules

* Keep changes scoped to the requested task.
* Do not commit unless the user explicitly asks.
* Before editing, read files in full, especially if the read tool truncates them.
* Follow existing code style in touched files (naming, type usage, control flow, and error handling patterns).

## Project Overview
**SSPlayerForGodot** is a Godot Engine integration for [SpriteStudio](https://github.com/SpriteStudio), a 2D animation tool. 
This project provides both a Godot GDExtension and the ability to build a custom Godot engine module to load and play SpriteStudio animations within Godot 4.

**Key Technologies:**
- **C++:** Core plugin logic.
- **Godot 4:** Target engine for the plugin.
- **FlatBuffers:** Used for serialization and data exchange (found in `gd_spritestudio/flatbuffers/`).
- **SCons:** Build system used for compiling the C++ code (via `SConstruct`).
- **Shell / PowerShell:** Cross-platform scripts used for building and releasing.

## Directory Overview
- `gd_spritestudio/`: The main C++ source code for the Godot extension. Contains Godot node bindings (e.g., `gd_ssplayer_node2d.cpp`), import plugins, and FlatBuffer parsers.
- `scripts/`: Build, generate, and release scripts for various platforms (Windows, macOS, Linux, iOS, Android, Web).
- `examples/`: Sample Godot projects demonstrating how to use the plugin (e.g., `feature_test`, `feature_test_gdextension`, `mesh_bone`, `particle_effect`, `new`, `new_gdextension`).
- `misc/`: Property list files and other miscellaneous configuration files for platform-specific builds.

## Building and Running

The project relies on SCons and requires the Godot development environment to be set up. Scripts are provided to simplify the build process.

### 1. Initial Setup
Ensure that the repository's native submodules (`flatbuffers`, `SpriteStudio7-SDK`) are initialized:
```bash
git submodule update --init --recursive
```
Additionally, the `godot` and `godot-cpp` repositories are not included in this repository and must be cloned manually as follows (refer to `README.md` for details):
```bash
# Inside the SSPlayerForGodot directory
git clone https://github.com/godotengine/godot.git -b 4.6
git clone https://github.com/godotengine/godot-cpp.git -b 4.5
```

### 2. Building a Custom Godot Engine
During development, you can use the `build.*` scripts. This compiles SpriteStudio directly into the Godot engine.
- **Linux / macOS:**
  ```bash
  ./scripts/build.sh
  ```
- **Windows:**
  ```powershell
  .\scripts\build.ps1
  ```

### 3. Building the GDExtension
If you prefer to build the project as a standalone GDExtension plugin:
- **Linux / macOS:**
  ```bash
  ./scripts/build-extension.sh
  ```
- **Windows:**
  ```powershell
  .\scripts\build-extension.ps1
  ```

### 4. Building the SS Runtime
You may also need to build the `ssruntime` binary for your platform:
- **Linux / macOS:**
  ```bash
  ./scripts/build-runtime.sh
  ```
- **Windows:**
  ```powershell
  .\scripts\build-runtime.ps1
  ```

## Development Conventions
- **C++ Standards:** The extension relies on standard Godot extension practices.
- **Platform Specifics:** SCons handles platform-specific flags. Noticeable configuration exists for iOS (`-miphoneos-version-min=12.0`), macOS (`-framework CoreFoundation`), and Windows.
- **External Dependencies:** Ensure `godot` and `godot-cpp` are correctly cloned into the root directory before running build scripts.
