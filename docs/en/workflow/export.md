# Exporting Your Project

Once your scene plays correctly in the editor, you export it like any other Godot project (**Project → Export…**). SpriteStudioPlayer runs in exported builds on every Godot target. There are only a few plugin-specific points to know.

## General

- **GDExtension variant** — the native library lives in your project's `addons/spritestudio/bin/`, and Godot bundles it into the export automatically. Desktop targets need nothing extra.
- **Custom module variant** — the plugin is compiled into the engine, so there is nothing plugin-specific to configure at export time.
- **Mobile / universal targets** — if an export stops with a message about the *ETC2/ASTC texture format being disabled*, enable **Project → Project Settings → Rendering → Textures → VRAM Compression → Import ETC2 ASTC**, then re-export.

## Web

Web builds have two plugin-specific requirements when you use the **GDExtension** variant.

1. **Enable "Extensions Support" in the Web export preset.**
   In the Export dialog, select your **Web** preset → **Options** → turn on **Extensions Support**. GDExtension libraries only load on the Web when the engine template supports dynamic linking, and this option tells Godot to use that template.

2. **The dlink-enabled Web export templates must be installed.**
   The stock Godot Web templates do not support GDExtension. You need the `web_nothreads_dlink_debug.zip` / `web_nothreads_dlink_release.zip` templates installed in your Godot export-templates folder. Building and installing them is an advanced step covered in the [Build Guide → GDExtension on the Web](../setup/build.md#web-platform-export-and-testing). Once they are in place, enabling **Extensions Support** is all the preset needs — Godot finds the right template automatically.

> [!WARNING]
> If a Web build fails at startup with *"GDExtension libraries are not supported by this engine version…"*, one of the two requirements above is missing (Extensions Support is off, or the dlink templates are not installed).

Additional notes for the Web:

- The plugin runs **single-threaded (`nothread`)** on the Web, so leave **Thread Support** off in the preset. A plain HTTP server is enough to test the export — no special cross-origin (COOP/COEP) headers are required.
- A working GDExtension Web export produces an `index.side.wasm` next to `index.wasm`; its presence confirms the dlink template was used.
- The runtime uses **WebAssembly SIMD**, so the target browser must support it.

> [!NOTE]
> The **custom module** variant needs none of the steps above — no *Extensions Support* and no dlink template — because the plugin is compiled into the engine. It still runs single-threaded on the Web, and (like every custom-module target) it uses **your own engine-built Web template**, which already contains the plugin, rather than a stock Godot template. That build/install flow is described in the [Build Guide](../setup/build.md).
