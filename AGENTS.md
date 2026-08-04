# SpriteStudioPlayer for Godot — AI Agent Guide

## Operating rules

*   Keep changes scoped to the requested task.
*   Do not commit unless the user explicitly asks.
*   Before editing, read enough surrounding context to understand callers and invariants; if the read tool truncated the file, read the remaining ranges before making non-trivial changes.
*   Follow existing code style in touched files (naming, type usage, control flow, error handling).
*   **Dependency Constraint:** Never mention or include `godot` or `godot-cpp` source code in responses or suggestions; they are external dependencies.
*   **SDK Alignment:** When modifying core playback logic, refer to `ss_player/SpriteStudio-SDK/AGENTS.md` for Rust runtime constraints.

## Architecture in one paragraph

A Godot Engine integration for SpriteStudio 7, providing a C++ `SpriteStudioPlayer2D` node that can be built as a **GDExtension** or a **custom module**. It uses `libssruntime` (Rust) from SpriteStudio-SDK via FFI to play `.ssab` (FlatBuffers) binaries. Projects are converted from `.sspj` (XML) to `.ssab` at import-time using `libssconverter`. The C++ side handles Godot node lifecycle, resource management, and batch rendering.

## Key components

| Path | Role | Docs |
|---|---|---|
| `ss_player/` | C++ source: Node bindings, editor import dock, and FFI wrappers. | — |
| `ss_player/runtime/` | Binary artifacts (`libssruntime`, `libssconverter`) and FFI headers. | — |
| `ss_player/SpriteStudio-SDK/` | Submodule for core Rust runtime/converter. | [SDK AGENTS](./ss_player/SpriteStudio-SDK/AGENTS.md) |
| `ss_player/format/` | FlatBuffers-generated headers for C++. | — |
| `scripts/` | SCons build wrappers and release packaging. | [docs/en/setup/build.md](./docs/en/setup/build.md) |

## Hard constraints

*   **FFI Safety:** C++ interacts with Rust via a C-API. Ensure `SsState` and other Rust-allocated handles are properly released via their respective `*_release` functions to avoid leaks.
*   **Performance:** Avoid per-frame allocations in the playback hot path. Use the `DrawBatch` plans emitted by the runtime directly for rendering.
*   **SDK Versioning:** `scripts/SDK_VERSION.txt` pins the required SDK release. Binaries in `ss_player/runtime/` must match this version.
*   **Build System:** `SConstruct` and `SCsub` files must be updated if new C++ source files are added.

## Documentation site

Folder-based i18n (`mkdocs-static-i18n`, `docs_structure: folder`): every page exists as both `docs/en/<path>` and `docs/ja/<path>`, and `nav` is defined once in `mkdocs.yml` with Japanese labels from `nav_translations`. Three things do **not** follow that "everything twice" rule — check before duplicating:

*   **`overrides/main.html`** (`theme.custom_dir`) emits the Open Graph / Twitter Card tags Material omits. **One template serves both locales**: the i18n plugin substitutes `languages.ja.site_description` before rendering, so Japanese cards need no second copy. Localize by adding a per-locale config override, never by branching in the template. Keep it in sync with the copies in SpriteStudio-Docs and the sibling player repos.
*   **Footer social icons are defined twice** — `extra.social` and the `ja` block's `extra` override, which *replaces* the whole `social` list rather than merging into it. Add or reorder an icon in both, or Japanese readers lose it. Keep `twitter:site` in `overrides/main.html` in sync with the X link.
*   **There is deliberately no `edit_uri`**, so no "edit this page" button renders. It is a branch name nothing validates, and it had drifted wrong in three of the five player repos. Do not reintroduce it.

`mkdocs.yml` sets `strict: true`, so a plain `build` (and `serve`) fails on a broken link or nav mismatch. Nothing builds the docs on a pull request — `pages.yml` runs on `release: published` and dispatch — so the local build is the only gate: `.venv/bin/mkdocs build --strict`.

## Verification

| Task | Command |
|---|---|
| Build GDExtension | `./scripts/build-extension.sh` (POSIX) / `.\scripts\build-extension.ps1` (Win) |
| Build Custom Module | `./scripts/build.sh` (POSIX) / `.\scripts\build.ps1` (Win) |
| Setup (Source SDK) | `./scripts/build-runtime.sh` (POSIX) / `.\scripts\build-runtime.ps1` (Win) |
| Setup (Prebuilt SDK) | `./scripts/download-sdk.sh` (POSIX) / `.\scripts\download-sdk.ps1` (Win) |
| Deploy Example Assets | `./scripts/deploy-examples.sh` (POSIX) / `.\scripts\deploy-examples.ps1` (Win) |
| Format C++ Code | `clang-format -i ss_player/*.{cpp,h}` (if available) |

*Note: Setup (Source SDK) is recommended for developers using the submodule. Setup (Prebuilt SDK) is intended for CI or release-only environments.*

## Workspace conventions

*   SCons is the primary build tool.
*   C++ code follows Godot's style (CamelCase classes, snake_case methods/vars).
*   Documentation is maintained in both English (`.md`) and Japanese (`.ja.md`).
*   Submodule `ss_player/SpriteStudio-SDK` should be kept in sync with the project requirements.
