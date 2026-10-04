[**English**](./README.md) | [**日本語**](./README.ja.md)

# SpriteStudio Player for Godot

**Professional 2D animations for your Godot games. A plugin that balances intuitive usability with extreme performance.**

> **Note:** This `develop` branch is a work-in-progress version. The stable version can be obtained from the [main branch](https://github.com/cri-middleware/SSPlayerForGodot/tree/main) or from [Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases). The APIs and workflows in this branch may change without notice, and no warranty or support is provided (we cannot respond to feature requests or bug reports).

A high-performance extension plugin (GDExtension / Custom Module) for playing animations (`.ssab`) created with **[SpriteStudio](https://www.webtech.co.jp/spritestudio/)** on [Godot Engine](https://godotengine.org/). By combining Godot's powerful features with the expressive capabilities of a dedicated animation tool, it fully supports the development of rich 2D games.

## ✨ Why use SpriteStudio with Godot?

- **Bring your characters to life.** Breathing and blinking, swaying hair and clothes, flashy slash and magic effects — the motion is built visually in a dedicated editor (SpriteStudio) with bone rigs, mesh deformation and particles, and Godot just plays it. Nothing to rebuild by hand in `AnimationPlayer`.
- **Not just characters — effects, UI and whole scenes.** Combine characters, backgrounds, effects and UI into a cutscene or a full-screen sequence in the editor, and play it in Godot as a single animation.
- **Just another node.** `SpriteStudioPlayer2D` sits in your scene like any other node and is controlled from GDScript. The animation lives in its own file rather than inside the scene, so teammates don't collide over scene merges in Git.
- **A seamless SpriteStudio ⇄ Godot round trip.** Drop a `.sspj` onto the SS Import Dock and it is converted inside the editor. To tweak it, click **Open SSPJ** in the Inspector, save in SpriteStudio, then click **Reconvert** — no manual export step.
- **Zero-parse loading, SIMD playback.** `.ssab` is a FlatBuffers binary read in place, so loading involves no parsing, and poses are computed by a SIMD-optimised Rust core — light enough for mobile and for scenes with many characters. Turn on **sub-frame interpolation** and motion follows high-refresh-rate displays.

## 📚 Documentation

Comprehensive documentation is available in the `docs/` folder — the data-flow diagram, key features and supported versions are all there:

- [**Documentation site (hosted)**](https://cri-middleware.github.io/SSPlayerForGodot/) — 🚧 live after the first release
- [**Documentation (English)**](./docs/en/index.md)
- [**ドキュメント (日本語)**](./docs/ja/index.md)
- [**For AI assistants**](https://cri-middleware.github.io/SSPlayerForGodot/llms.txt) — these docs as Markdown: `llms.txt` indexes the pages and `llms-full.txt` holds all of them in one file — 🚧 live after the first release
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

You need a 4.7-series Godot editor from the [official site](https://godotengine.org/download/). Projects have to be authored in **SpriteStudio 7.5 or later**.

> 🚧 **This generation is not released yet.** [Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases) currently carries only the **1.x** plugin, which has neither `SSABResource` nor `SpriteStudioPlayer2D` — both arrived with 7.x. Until the first 7.x release, build the add-on from a checkout with the [Build Guide](./docs/en/setup/build.md).

1. **Install**: Download the latest package from [Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases), extract it, and copy its `addons` folder into your Godot project root.
2. **Import**: Drag your `.sspj` from your file manager onto the **SS Import** dock to convert it to `.ssab` (see [Asset Import and Editor Integration](./docs/en/workflow/usage_asset_pipeline.md)).
3. **Play**: Add a `SpriteStudioPlayer2D` node and assign the `.ssab` to its `Ssab` property.

No `.sspj` of your own yet? SpriteStudio's [official sample data](https://www.webtech.co.jp/help/ja/spritestudio7/download/sample/) works as-is — the page is in Japanese, the downloads are not. [Ringo](https://www.webtech.co.jp/help/ja/spritestudio7/download/sample/#ringo) is a good character to start with, and [Particle](https://www.webtech.co.jp/help/ja/spritestudio7/download/sample/#Perticle_sample) a good one for effects.

For more details, see the [Installation Guide](./docs/en/setup/install.md).

## 🔗 Related Repositories

- [SpriteStudio Docs](https://cri-middleware.github.io/SpriteStudio-Docs/) — the documentation portal for the SDK and every official player
- [SpriteStudio-SDK](https://github.com/cri-middleware/SpriteStudio-SDK) — The SDK itself, providing `libssruntime` / `libssconverter`
- [SSConverterGUI](https://github.com/cri-middleware/SSConverterGUI) — a standalone desktop GUI for converting `.sspj` without a player

## 📄 License

See [LICENSE.md](./LICENSE.md).

For third-party library licenses (such as FlatBuffers and SpriteStudio-SDK dependencies), see [THIRD_PARTY_NOTICES.md](./THIRD_PARTY_NOTICES.md).
