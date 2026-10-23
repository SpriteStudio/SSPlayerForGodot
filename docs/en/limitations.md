# ⚠️ Limitations & Scope

Constraints and platform-specific caveats to be aware of when shipping with the current SpriteStudio Player for Godot. For what *is* supported, see the workflow guides.

## Not Yet Implemented

> [!WARNING]
> - **Text parts** — not drawn. The runtime reserves the part's slot in the draw plan but supplies no glyphs (glyph layout and rasterization are the player's job), and the plugin has no Godot text pass yet.
> - **9-slice parts** — not drawn. The runtime builds the whole grid into its dedicated `Nines` buffers, but the plugin does not read them yet, so the part renders as nothing rather than as a stretched cell.

Text and 9-slice parts still occupy their place in the draw order, so an animation that uses them plays with those parts missing rather than failing. If you ship one, replace it with an ordinary part at authoring time.

## Blend Modes

> [!WARNING]
> Of SpriteStudio's twelve part blend (mixing) modes, four are honored — **Mix**, **Multiply**, **Add**, and **Subtract** — and **Mulalpha** and **Mul2** draw as **Multiply**. The other six — **Screen**, **Exclusion**, **Invert**, **Div2**, **Screen2**, **Overlay2** — fall back to **Mix**.

The plugin draws straight to Godot's `RenderingServer` canvas item with no intermediate render target, so a part is limited to the blend modes that map onto a `CanvasItemMaterial`. A part set to an unsupported mode still draws — it just composites as ordinary alpha blending instead of the authored mode. If your artwork relies on one of the other modes, check it in Godot before shipping.

## Masks

> [!WARNING]
> - **At most 96 masks at once in one animation**, counting both mask parts and parts that write a clipping mask (`writeMask`), and counting the animations it plays through instance parts. An instance's masks count again every time the instance is placed; a hidden mask, or one whose mask value is 0, does not count. Past 96, the rest are ignored and a warning is printed once.
> - **Effect parts are not clipped by masks.**

Godot's 2D canvas has no stencil buffer, so the plugin draws every mask of an animation — its instances included — into one offscreen bitmap, one bit per mask. Up to 24 fit at full resolution; past that the bitmap is split into two or four tiles of the same size, so an animation with many masks gets coarser mask edges rather than taking more memory.

## Differences from Other 2D Nodes

The animation draws on canvas items of its own beneath the node, not on the node itself. Two things that work on a `Sprite2D` therefore work differently here.

> [!NOTE]
> - **Selecting in the 2D editor.** Clicking the artwork does not select the player. The 2D editor picks a node by the rectangle the node reports, and a node that reports none by its origin alone. Click near the origin, or select the node in the Scene dock; a box selection catches it when the box contains the origin. There are no resize handles either. Godot gives GDExtension nodes no way to report that rectangle, and the custom-module build behaves the same.
> - **The node's Material.** A material assigned to the player has no effect — every part draws with the plugin's own shader — and the node shows a configuration warning while one is set. To run a shader over the whole animation, give the material to a `CanvasGroup` parent instead; see [A Shader Over the Whole Animation](workflow/tips.md#a-shader-over-the-whole-animation).

## Platforms & Export

The plugin ships in two build variants — **GDExtension** and **custom module** — and both can be exported to every Godot target. See [Exporting Your Project](workflow/export.md) for the export flow of each.

Per-platform caveats to keep in mind:

- **macOS (GDExtension)** — one universal build covers Apple silicon and Intel.
- **iOS** — exporting requires macOS + Xcode.
- **Android** — the runtime ships for `arm64-v8a`, `armeabi-v7a` and `x86_64`.
- **Web** — see the dedicated section below.

## Web (WASM)

> [!WARNING]
> - **Single-threaded (`nothread`) only.** On the Web the plugin runs without threads. The upside is that no cross-origin isolation (COOP/COEP) headers are required, so a plain HTTP server is enough to run an export. Leave **Thread Support** off in the export preset.
> - **WebAssembly SIMD is required.** The runtime is built with WebAssembly SIMD instructions and will **not run on browsers without SIMD support**. Treat SIMD support as a browser requirement for your distribution.
> - **GDExtension on the Web needs a dlink-enabled engine template.** The official Godot Web export templates do not support GDExtension libraries. You must supply an engine template built with `dlink_enabled=yes` and turn on **"Extensions Support"** in the export preset — otherwise the exported page fails with *"GDExtension libraries are not supported by this engine version."* See [Exporting Your Project → Web](workflow/export.md#web) for the steps. (The **custom module** variant embeds the code into the engine itself and does not need this.)

## Motion Blending

> [!WARNING]
> - **Instance and Effect parts are outside motion blending.** They belong to the `.ssab`, not to one animation, so a crossfade on track 0 keeps them running instead of setting them up again. Which key drives such a part across the switch is not blended: where both animations key it on the same frame, it may carry on with the outgoing animation's setting.

## Playback Feature Constraints

These come from the shared `libssruntime` and therefore apply regardless of build variant. They
apply to every official player too, so the full explanations — and the rest of the shared
constraints (embedded mode, unvalidated `.ssab` input, and so on) — live in the portal:
**[Shared Limitations](https://cri-middleware.github.io/SpriteStudio-Docs/sdk/limitations/)**.

> [!WARNING]
> - **`independent=true` parts (Instance / Effect) do not reverse or seek correctly.** Only forward playback (including forward frame-skips) is correct.
> - **Seeking does not fire the events it skips over.** Only the destination frame's `UserData` / `Signal` / `Audio` events fire.
> - **No reverse audio.** Sounds are skipped whenever the effective direction is backward (a reverse direction, or a ping-pong return leg). A negative `speed_scale` is not one of those: speed is a multiplier, and a negative one stops the playhead rather than turning it around — as does `0`.
