[**English**](./README.md) | [**日本語**](./README.ja.md)

# SpriteStudioPlayer for Godot

**Professional 2D animations for your Godot games. A plugin that balances intuitive usability with extreme performance.**

> **Note:** This `develop` branch is a work-in-progress version. The stable version can be obtained from the [main branch](https://github.com/cri-middleware/SSPlayerForGodot/tree/main) or from [Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases). The APIs and workflows in this branch may change without notice, and no warranty or support is provided (we cannot respond to feature requests or bug reports).

A high-performance extension plugin (GDExtension / Custom Module) for playing animations (`.ssab`) created with **[SpriteStudio](https://www.webtech.co.jp/spritestudio/)** on [Godot Engine](https://godotengine.org/). By combining Godot's powerful features with the expressive capabilities of a dedicated animation tool, it fully supports the development of rich 2D games.

## ✨ Why use SpriteStudio with Godot?

- **Unmatched Versatility: From Characters to UI and Effects**
  Unlike character-specific tools, you can author everything from character animations using mesh deformation to UI transitions and rich particle effects, all within a single dedicated editor. It maximizes the expressive power of your raster images.
- **Build "Entire Scenes" including Backgrounds and Effects**
  Beyond animating individual characters, you can construct entire "cutscenes" or "full screen" presentations—combining characters, backgrounds, effects, and UI—directly in the editor, and play them back as a single animation in Godot.
- **Cross-Engine Visual Consistency**
  Pose, interpolation, deformation and draw order are computed by an independent core runtime shared with every official player, so the structural "visual deviations" that come from an engine's own specifications do not occur — those match across engines. Blend modes and part colours are carried as intent and reproduced as faithfully as the host's rendering layer allows, so they are best-effort rather than identical; masking, text and audio are Godot's own. Sub-frame interpolation ensures smooth rendering even at high refresh rates.
- **Natural Integration as a Godot "Node" and Conflict Avoidance**
  `SpriteStudioPlayer2D` seamlessly integrates into your Godot scenes as a standard node, allowing easy control from GDScript without bloating the Node tree. At the same time, the animation data itself is separated from the scene, preventing Git conflicts during team development.
- **Extreme Performance via Zero-copy Loading and SIMD**
  By converting your data into optimized binaries (`.ssab` / FlatBuffers) for use, parsing load is reduced to zero at runtime, allowing instant playback from memory. By fully utilizing SIMD in internal calculations, it achieves maximum animation playback performance with minimal CPU and memory overhead, ensuring smooth operation even in mobile environments or games rendering massive numbers of characters.

## 📚 Documentation

Comprehensive documentation is available in the `docs/` folder — the data-flow diagram, key features and supported versions are all there:

- [**Documentation site (hosted)**](https://cri-middleware.github.io/SSPlayerForGodot/) — 🚧 live after the first release
- [**Documentation (English)**](./docs/en/index.md)
- [**ドキュメント (日本語)**](./docs/ja/index.md)
- [**SpriteStudio Docs (portal)**](https://cri-middleware.github.io/SpriteStudio-Docs/) — the SDK and every official player in one place — 🚧 live after the first release

### Quick Links (English)
- [Installation](./docs/en/setup/install.md)
- [Basic Usage](./docs/en/workflow/usage_basic.md)
- [Limitations & Scope](./docs/en/limitations.md)
- [Troubleshooting](./docs/en/troubleshooting.md)
- [Migration from v1.x](./docs/en/migration_from_v1.md)

## 🚀 Quick Start with GDExtension

**Using it in your game?** Read on — install the add-on, drag a `.sspj` onto the editor, place a node.
Everything after that (scripting, signals, per-part overrides) is the same path further in, in the
[documentation](./docs/en/index.md).
**Working on the Player, or the Rust runtime under it?** [CONTRIBUTING.md](./CONTRIBUTING.md) and the [Build Guide](./docs/en/setup/build.md).

We provide two Quick Starts: one for quickly checking the operation using a sample project, and another for setting up your own project. Projects have to be authored in **SpriteStudio 7.5 or later**.

### 1. Check Operation with Sample

> 🚧 **This generation is not released yet.** [Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases) currently carries only the **1.x** plugin, which cannot open the samples on this branch — they use `SSABResource` and `SpriteStudioPlayer2D`, both of which arrived with 7.x. Until the first 7.x release, build the extension from this checkout with the [Build Guide](./docs/en/setup/build.md), then continue from step 3.

1. **Get Godot Engine**: Download a 4.7-series editor from the [official site](https://godotengine.org/download/).
2. **Get the repository**: Clone it with `--recurse-submodules`. The sample's source project lives in the `ss_player/SpriteStudio-SDK` submodule, and without it the sample has nothing to convert.
3. **Download GDExtension**: Get the latest package from [Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases) and extract it.
4. **Prepare Sample**: Copy the extracted `addons` folder into the [examples/Ringo](./examples/Ringo) folder of this repository.
5. **Check**: Open the [examples/Ringo](./examples/Ringo) project in Godot Engine. The add-on reads `.ssplayer_sources.cfg` and converts `Ringo.sspj` on first open — no `.ssab` is committed — then open `Ringo.tscn` to see the animation working.

### 2. Introduce to Your Project

1. **Install**: Copy the `addons` folder into your Godot project root.
2. **Import**: Drag & drop your `.sspj` onto the Godot editor to convert it to `.ssab`.
3. **Play**: Add a `SpriteStudioPlayer2D` node and assign the `.ssab` to its `SSAB Resource` property.

For more details, see the [Installation Guide](./docs/en/setup/install.md).

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
