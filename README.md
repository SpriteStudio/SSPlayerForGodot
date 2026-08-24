[**日本語**](./README.ja.md) | [**English**](./README.md)

# SpriteStudioPlayer for Godot

**Professional 2D animations for your Godot games. A plugin that balances intuitive usability with extreme performance.**

> **Note:** This `develop` branch is a work-in-progress version. The stable version can be obtained from the [main branch](https://github.com/cri-middleware/SSPlayerForGodot/tree/main) or from [Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases). The APIs and workflows in this branch may change without notice, and no warranty or support is provided (we cannot respond to feature requests or bug reports).

A high-performance extension plugin (GDExtension / Custom Module) for playing animations (`.ssab`) created with **[OPTPiX SpriteStudio 7](https://www.webtech.co.jp/spritestudio/)** on [Godot Engine](https://godotengine.org/). By combining Godot's powerful features with the expressive capabilities of a dedicated animation tool, it fully supports the development of rich 2D games.

## ✨ Why use SpriteStudio with Godot?

- **Unmatched Versatility: From Characters to UI and Effects**
  Unlike character-specific tools, you can author everything from character animations using mesh deformation to UI transitions and rich particle effects, all within a single dedicated editor. It maximizes the expressive power of your raster images.
- **Build "Entire Scenes" including Backgrounds and Effects**
  Beyond animating individual characters, you can construct entire "cutscenes" or "full screen" presentations—combining characters, backgrounds, effects, and UI—directly in the editor, and play them back as a single animation in Godot.
- **Cross-Engine Visual Consistency**
  Since complex state calculations are handled by an independent core runtime, structural "visual deviations" caused by Godot's unique specifications do not occur. It is guaranteed that the appearance in the editor and the playback results in other engines perfectly match. Sub-frame interpolation ensures smooth rendering even at high refresh rates.
- **Natural Integration as a Godot "Node" and Conflict Avoidance**
  `SpriteStudioPlayer2D` seamlessly integrates into your Godot scenes as a standard node, allowing easy control from GDScript without bloating the Node tree. At the same time, the animation data itself is separated from the scene, preventing Git conflicts during team development.
- **Extreme Performance via Zero-copy Loading and SIMD**
  By converting your data into optimized binaries (`.ssab` / FlatBuffers) for use, parsing load is reduced to zero at runtime, allowing instant playback from memory. By fully utilizing SIMD in internal calculations, it achieves maximum animation playback performance with minimal CPU and memory overhead, ensuring smooth operation even in mobile environments or games rendering massive numbers of characters.

## 📚 Documentation

Comprehensive documentation is available in the `docs/` folder:

- [**Documentation site (hosted)**](https://cri-middleware.github.io/SSPlayerForGodot/) — 🚧 live after the first release
- [**Documentation (English)**](./docs/en/index.md)
- [**ドキュメント (日本語)**](./docs/ja/index.md)
- [**SpriteStudio Docs (portal)**](https://cri-middleware.github.io/SpriteStudio-Docs/) — the SDK and every official player in one place — 🚧 live after the first release

### Quick Links (English)
- [Installation](./docs/en/setup/install.md)
- [Basic Usage](./docs/en/workflow/usage_basic.md)
- [Asset Import and Editor Integration](./docs/en/workflow/usage_asset_pipeline.md)
- [Scripting and Events](./docs/en/workflow/usage_scripting.md)
- [Audio Playback](./docs/en/workflow/audio.md)
- [CLI Conversion and Automation](./docs/en/workflow/import.md)
- [Performance Tuning and Advanced Settings](./docs/en/workflow/tips.md)
- [Build Guide](./docs/en/setup/build.md)
- [Troubleshooting](./docs/en/troubleshooting.md)
- [Migration from v1.x](./docs/en/migration_from_v1.md)

## 🚀 Quick Start with GDExtension

We provide two Quick Starts: one for quickly checking the operation using a sample project, and another for setting up your own project.

### 1. Check Operation with Sample

1. **Get Godot Engine**: Download a 4.7-series editor from the [official site](https://godotengine.org/download/).
2. **Download GDExtension**: Get the latest package from [Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases) and extract it.
3. **Prepare Sample**: Copy the extracted `addons` folder into the [examples/Ringo](./examples/Ringo) folder of this repository.
4. **Check**: Open the [examples/Ringo](./examples/Ringo) project in Godot Engine and open `Ringo.tscn` to immediately see the animation working.

### 2. Introduce to Your Project

1. **Install**: Copy the `addons` folder into your Godot project root.
2. **Import**: Drag & drop your `.sspj` onto the Godot editor to convert it to `.ssab`.
3. **Play**: Add a `SpriteStudioPlayer2D` node and assign the `.ssab` to its `SSAB Resource` property.

For more details, see the [Installation Guide](./docs/en/setup/install.md).

## 💡 Overview

This plugin features a **powerful asset pipeline that allows you to seamlessly transition between SpriteStudio and Godot**, enabling you to update assets instantly.

For the data-flow diagram, key features, supported versions, and more, see the **[Documentation (English)](./docs/en/index.md)**.

## 🎬 Samples

Sample projects based on SDK test projects are available under the [examples folder](./examples/).

- [Ringo](./examples/Ringo) — Basic quickstart test for Ringo
- [Scripting](./examples/Scripting) — GDScript example for controlling animations and signals
- [Override_Ringo](./examples/Override_Ringo) — Attribute/material override example
- [overall](./examples/overall) — Comprehensive functional test (Custom Module)
- [overall_gdextension](./examples/overall_gdextension) — Comprehensive functional test (GDExtension)

## 🔗 Related Repositories

- [SpriteStudio Docs](https://cri-middleware.github.io/SpriteStudio-Docs/) — the documentation portal for the SDK and every official player
- [SpriteStudio-SDK](https://github.com/cri-middleware/SpriteStudio-SDK) — The SDK itself, providing `libssruntime` / `libssconverter`
- [SSConverterGUI](https://github.com/cri-middleware/SSConverterGUI) — a standalone desktop GUI for converting `.sspj` without a player

## 📄 License

See [LICENSE.md](./LICENSE.md).

For third-party library licenses (such as FlatBuffers and SpriteStudio-SDK dependencies), see [THIRD_PARTY_NOTICES.md](./THIRD_PARTY_NOTICES.md).
