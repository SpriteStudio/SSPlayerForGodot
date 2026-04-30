# SpriteStudioPlayer for Godot

## Minimal operating rules

* Keep changes scoped to the requested task.
* Do not commit unless the user explicitly asks.
* Before editing, read files in full, especially if the read tool truncates them.
* Follow existing code style in touched files (naming, type usage, control flow, and error handling patterns).
* Do not mention or include 'godot' and 'godot-cpp' in responses or code suggestions (they are external dependencies).
* **Refer to the SpriteStudio7-SDK porting documentation** when implementing or modifying player logic:
  - Location: `gd_spritestudio/SpriteStudio7-SDK/libs/ssruntime/docs/README.ja.md`

## Project Overview
**SSPlayerForGodot** is a Godot Engine integration for [SpriteStudio](https://github.com/SpriteStudio), providing both GDExtension and custom engine module support.

**Key Technologies:**
- **C++:** Core plugin logic and Godot bindings.
- **Rust:** Core runtime (`libssruntime`) exposed via FFI.
- **FlatBuffers:** Serialization and data exchange.
- **SCons:** Build system.

## Building and Running

### 0. Prerequisites (Initial Setup)
Ensure submodules are initialized and external Godot repositories are cloned into the project root:
```bash
git submodule update --init --recursive
git clone https://github.com/godotengine/godot.git -b 4.6
git clone https://github.com/godotengine/godot-cpp.git -b 4.5
```

### 1. Build SS Runtime (Rust)
- **POSIX:** `./scripts/build-runtime.sh`
- **Windows:** `.\scripts\build-runtime.ps1`

### 2. Build GDExtension
- **POSIX:** `./scripts/build-extension.sh`
- **Windows:** `.\scripts\build-extension.ps1`

### 3. Build Custom Godot Engine
- **POSIX:** `./scripts/build.sh`
- **Windows:** `.\scripts\build.ps1`

## Directory Overview
- `gd_spritestudio/`: Main C++ source, Godot node bindings, and import plugins.
- `scripts/`: Build and release scripts for various platforms.
- `examples/`: Sample Godot projects.
- `misc/`: Platform-specific configuration files.
