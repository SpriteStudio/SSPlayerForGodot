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

Built with [Zensical](https://zensical.org), the successor to MkDocs + Material for MkDocs. Zensical has no multi-locale build, so **each locale is built from its own config**: `mkdocs.yml` builds English to `site/`, `mkdocs.ja.yml` builds Japanese to `site/ja/`, and both inherit the shared settings from `mkdocs.base.yml`. Every page still exists as both `docs/en/<path>` and `docs/ja/<path>`; there is no fallback locale any more, so a page missing from one side is a nav entry pointing at nothing.

**The three-config split has one rule: a YAML sequence must be defined in exactly one of the three files.** Zensical's `INHERIT` *concatenates* parent and child sequences where MkDocs replaces them — put `nav` in the base as well as a locale config and the Japanese site renders the English navigation followed by the Japanese one. Shared sequences (`theme.palette`, `theme.features`, `plugins`, `markdown_extensions`, `extra.alternate`, `extra_css`) live in `mkdocs.base.yml` and nowhere else; `nav` and `extra.social` live in the locale configs and never in the base. Mappings deep-merge, so scalars like `theme.language` are safe to override.

Things that do **not** follow the "everything twice" rule, or that changed with the migration — check before duplicating:

*   **`overrides/main.html`** (`theme.custom_dir`) emits the Open Graph / Twitter Card tags the theme omits, **and repairs the page title**: Zensical never sets `page.is_homepage`, which its own `base.html` branches on, so the landing page would title itself "SpriteStudioPlayer for Godot - SpriteStudioPlayer for Godot". The override derives the flag from `page.url` and overrides the `htmltitle` block. **One template serves both locales** — everything is read from whichever locale config is building. Localize by adding a per-locale config override, never by branching in the template. Keep it in sync with the copies in SpriteStudio-Docs and the sibling player repos.
*   **`overrides/partials/alternate.html`** rebuilds the header language selector so switching language keeps you on the same page. `mkdocs-static-i18n` used to rewrite those links per page; nothing does once the locales are separate builds, and the stock partial would send every reader to the other locale's home page. It is driven by `extra.alternate[].link` (a locale subpath, not a URL) plus `extra.site_root_prefix` in each locale config. Known gap: the `<link rel="alternate" hreflang>` tags in `<head>` still point at each locale root, because they sit inside the theme's `site_meta` block and overriding that would mean copying thirty lines that Zensical may change.
*   **Each locale ships its own `docs/<locale>/assets/`.** There is no shared `docs/assets/` — it would sit outside both `docs_dir`s and never be copied. The screenshots and videos are therefore duplicated (Git stores identical content once, so this costs working-tree space, not history).
*   **Asset paths are source-relative** (`../assets/…` from a page one level deep), including inside raw `<video>` / `<img>` HTML. Zensical resolves raw HTML `src` the same way it resolves Markdown links; MkDocs passed it through untouched. That difference is why these paths cannot satisfy both engines at once, and it is the one place where the MkDocs fallback build produces wrong output.
*   **`site_url` is per locale** and the ja one keeps its `ja/` suffix — `page.canonical_url`, and therefore `og:url`, is built from it.
*   **Footer social icons are defined twice** — in `mkdocs.yml` and `mkdocs.ja.yml` (translated labels, same icons and links). Add or reorder an icon in both, or the two footers diverge. Keep `twitter:site` in `overrides/main.html` in sync with the X link.
*   **There is deliberately no `edit_uri`**, so no "edit this page" button renders. It is a branch name nothing validates, and it had drifted wrong in three of the five player repos. Do not reintroduce it.

`mkdocs.base.yml` sets `strict: true`, so a plain `build` fails on a broken link. `serve` validates nothing — `zensical serve --strict` is accepted and currently does nothing. Nothing builds the docs on a pull request either (`pages.yml` runs on `release: published` and dispatch; `pr.yml` builds the extension, not the docs), so the local build is the only gate, and **both locales have to be built** — English first, because it clears `site/`, which contains `site/ja`:

```bash
.venv/bin/zensical build --strict
.venv/bin/zensical build -f mkdocs.ja.yml --strict
```

Building only English is the easy mistake: it leaves a stale `site/ja` behind, so a Japanese page you just broke still looks fine.

Callouts (`> [!NOTE]`) are parsed natively — `mkdocs-callouts` is gone. Two consequences: `[!IMPORTANT]` is not a built-in admonition type, so `docs/*/stylesheets/extra.css` styles it to keep the appearance it had; and a list must be preceded by a blank `>` line, or it renders as literal text rather than a list.

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
