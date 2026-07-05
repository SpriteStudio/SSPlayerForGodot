# Limitations & Scope

Constraints and platform-specific caveats to be aware of when shipping with the current SpriteStudio Player for Godot. For the list of what *is* supported, see the [Key Features](index.md#key-features) and workflow guides.

> [!NOTE]
> This is a living document. Items are added as they are confirmed; wording and coverage may change as the plugin evolves.

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

These come from the shared `libssruntime` and therefore apply regardless of build variant.

> [!WARNING]
> - **`independent=true` parts (Instance / Effect) do not reverse or seek correctly.** An `independent=true` child runs on its own real-time clock, detached from the parent timeline. Parent **reverse playback** and **seeking** break it; only **forward** playback (including forward frame-skips) is correct. Prefer authoring without `independent=true` for animations that may be played in reverse or seeked.
> - **Seeking does not fire the events it skips over.** Jumping the playhead directly (e.g. setting the frame) does **not** emit the `UserData` / `Signal` / `Audio` events on the frames passed over — only the destination frame's events fire. If you need every intermediate event, advance the animation step by step instead of jumping.
> - **No reverse audio.** Sounds fire only while the animation advances forward in time. When the effective direction is backward (reverse direction, ping-pong return, or negative speed), audio events are skipped — audio cannot be played in reverse.
> - **Animation blending is same-`.ssab` only.** Cross-`.ssab` (cross-resource) blending is not supported, and every blended animation must assign the **same Cell to the same part**; mismatched Cells cause size / pivot mismatches that break the pose.
