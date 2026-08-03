# ⚠️ Limitations & Scope

Constraints and platform-specific caveats to be aware of when shipping with the current SpriteStudio Player for Godot. For the list of what *is* supported, see the [Key Features](index.md#key-features) and workflow guides.

> [!NOTE]
> This is a living document. Items are added as they are confirmed; wording and coverage may change as the plugin evolves.

## Not Yet Implemented

> [!WARNING]
> - **Text parts** — not drawn. The runtime reserves the part's slot in the draw plan but supplies no glyphs (glyph layout and rasterization are the player's job), and the plugin has no Godot text pass yet.
> - **9-slice parts** — not drawn. The runtime builds the whole grid into its dedicated `Nines` buffers, but the plugin does not read them yet, so the part renders as nothing rather than as a stretched cell.

Both part types still occupy their place in the draw order, so an animation that uses them plays with those parts missing rather than failing. If you ship one, replace it with an ordinary part at authoring time.

## Platforms & Export

The plugin ships in two build variants — **GDExtension** and **custom module** — and both can be exported to every Godot target. Build and execution are actively verified on **Windows / macOS**; the other targets are supported but less frequently exercised. See [Build Guide](setup/build.md) for the export flow of each.

Per-platform caveats to keep in mind:

- **macOS (GDExtension)** — released as a `universal` build (`arch=universal`).
- **iOS** — requires macOS + Xcode, and only `template_debug` / `template_release` are produced (no editor target).
- **Android** — the runtime is shipped as a native library placed under the matching ABI directory (`arm64-v8a` / `armeabi-v7a` / `x86_64`); build only the ABI of the device/emulator you run on.
- **Web** — see the dedicated section below.

## Web (WASM)

> [!WARNING]
> - **Single-threaded (`nothread`) only.** On the Web the plugin runs without threads. The upside is that no cross-origin isolation (COOP/COEP) headers are required, so a plain HTTP server is enough to run an export. Build the extension with `threads=no`.
> - **WebAssembly SIMD is required.** The runtime is built with WebAssembly SIMD instructions and will **not run on browsers without SIMD support**. Treat SIMD support as a browser requirement for your distribution.
> - **GDExtension on the Web needs a dlink-enabled engine template.** The official Godot Web export templates do not support GDExtension libraries. You must supply an engine template built with `dlink_enabled=yes` and turn on **"Extensions Support"** in the export preset — otherwise the exported page fails with *"GDExtension libraries are not supported by this engine version."* See [Exporting Your Project → Web](workflow/export.md#web) for the steps. (The **custom module** variant embeds the code into the engine itself and does not need this.)

## Playback Feature Constraints

These come from the shared `libssruntime` and therefore apply regardless of build variant. They
apply to every official player too, so the full explanations — and the rest of the shared
constraints (embedded mode, unvalidated `.ssab` input, and so on) — live in the portal:
**[Shared Limitations](https://cri-middleware.github.io/SpriteStudio-Docs/sdk/limitations/)**.

> [!WARNING]
> - **`independent=true` parts (Instance / Effect) do not reverse or seek correctly.** Only forward playback (including forward frame-skips) is correct.
> - **Seeking does not fire the events it skips over.** Only the destination frame's `UserData` / `Signal` / `Audio` events fire.
> - **No reverse audio.** Sounds are skipped whenever the effective direction is backward (reverse direction, ping-pong return, or negative speed).
> - **Animation blending is same-`.ssab` only**, and every blended animation must assign the same Cell to the same part.
