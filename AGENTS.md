# SpriteStudioPlayer for Godot

## Minimal operating rules

* Keep changes scoped to the requested task.
* Do not commit unless the user explicitly asks.
* Before editing, read files in full, especially if the read tool truncates them.
* Follow existing code style in touched files (naming, type usage, control flow, and error handling patterns).
* Do not mention or include 'godot' and 'godot-cpp' in responses or code suggestions (they are external dependencies).
* **Refer to the SpriteStudio7-SDK porting documentation** when implementing or modifying player logic:
  - Location: `ss_player/SpriteStudio7-SDK/libs/ssruntime/docs/README.ja.md`

## Project Overview

**SSPlayerForGodot** is a Godot Engine integration for [SpriteStudio](https://github.com/SpriteStudio).
The plugin can be consumed either as a **GDExtension** or as a **custom module** built into a Godot Engine binary.

`.sspj` projects are converted to `.ssab` (animation binary) / `.ssqb` (sequence binary) at build/import time, and played back at runtime through `libssruntime` from SpriteStudio7-SDK.

**Key Technologies:**
- **C++:** Plugin logic and Godot bindings.
- **Rust:** Core runtime (`libssruntime`) and converter (`libssconverter`) provided by SpriteStudio7-SDK; consumed via FFI.
- **FlatBuffers:** Serialization and data exchange.
- **SCons:** Build system.

## Documentation

User-facing documentation lives at the repository root. Treat these as the source of truth and update them alongside relevant code changes:

- `README.md` / `README.ja.md` — Project overview, quick start (Releases-based), samples.
- `BUILD.md` / `BUILD.ja.md` — Build instructions: source acquisition, environment notes, libssruntime placement, GDExtension build, custom-module build, release scripts, and an **SS7-SDK developer appendix** for users who build `libssruntime` from source.
- `USAGE.md` / `USAGE.ja.md` — Editor setup, `.sspj` import (via SS Import Dock or `ssconverter-cli`), node creation, inspector properties, and class API.

## Building and Running

### Default flow (Godot binaries)

This is the path for users who consume `libssruntime` as a prebuilt artifact.

1. **Get source:**
   ```bash
   git clone --recursive https://github.com/SpriteStudio/SSPlayerForGodot.git
   git clone https://github.com/godotengine/godot.git -b 4.6        # custom-module path
   git clone https://github.com/godotengine/godot-cpp.git -b 4.5    # GDExtension path
   ```
2. **Place `libssruntime`:** run `./scripts/download-sdk.sh` (POSIX) / `.\scripts\download-sdk.ps1` (Windows). The script fetches the version pinned in `ss_player/SDK_VERSION.txt` from [SpriteStudio7-SDK Releases](https://github.com/SpriteStudio/SpriteStudio7-SDK/releases), replaces `ss_player/runtime/`, and (on Windows) generates the target-suffixed `.lib` copies needed by the custom-module build. Skips when the same version is already extracted. iOS / Android / Web bundles do not include `libssconverter`.
3. **Choose one of:**
   - GDExtension: `./scripts/build-extension.sh` (POSIX) / `.\scripts\build-extension.ps1` (Windows)
   - Custom module: `./scripts/build.sh` (POSIX) / `.\scripts\build.ps1` (Windows)

Per-platform release scripts (`scripts/release-*.sh` / `scripts/release-gdextension-*.sh`) loop the build for `editor` / `template_debug` / `template_release` targets. They do **not** fetch or build `libssruntime`.

### SS7-SDK developer flow (build `libssruntime` from source)

Required only when developing/customizing SS7-SDK alongside the Godot side. Rust toolchain setup follows [SpriteStudio7-SDK README](https://github.com/SpriteStudio/SpriteStudio7-SDK?tab=readme-ov-file#for-sdk-developers).

- Build runtime: `./scripts/build-runtime.sh` / `.\scripts\build-runtime.ps1`
- Regenerate FlatBuffers headers (when `.fbs` files change): `./scripts/generate-runtime-code.sh` / `.\scripts\generate-runtime-code.ps1`

## Directory Overview

- `ss_player/`: Main C++ source, Godot node bindings, and editor import dock.
  - `ss_player/SDK_VERSION.txt`: Pinned SS7-SDK release tag consumed by `scripts/download-sdk.{sh,ps1}`. Edit this file to upgrade the bundled `libssruntime`.
  - `ss_player/runtime/`: Drop-in location for `libssruntime` / `libssconverter` (headers under `include/`, libs under `libs/<platform>/[<arch>/]`). Populated by `download-sdk.{sh,ps1}` or by manual extraction.
  - `ss_player/SpriteStudio7-SDK/`: Submodule. Used only for the SS7-SDK developer flow.
  - `ss_player/format/`: FlatBuffers-generated headers (regenerate via `generate-runtime-code.{sh,ps1}`).
- `scripts/`: Per-platform build and release scripts.
- `examples/`: Sample Godot projects. **Note:** existing samples were authored for the v1.x `.sspj` direct-load workflow and need migration to the current `.ssab` / `.ssqb` workflow before they will run.
- `misc/`: Platform-specific configuration files (`.gdextension`, plists, ccache patches).
