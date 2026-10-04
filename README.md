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

We provide two Quick Starts: one for quickly checking the operation using a sample project, and another for setting up your own project. Projects have to be authored in **SpriteStudio 7.5 or later**.

### 1. Check Operation with Sample

> 🚧 **This generation is not released yet.** [Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases) currently carries only the **1.x** plugin, which cannot open the samples on this branch — they use `SSABResource` and `SpriteStudioPlayer2D`, both of which arrived with 7.x. Until the first 7.x release, build the extension from this checkout with the [Build Guide](./docs/en/setup/build.md), then continue from step 3.

1. **Get Godot Engine**: Download a 4.7-series editor from the [official site](https://godotengine.org/download/).
2. **Get the repository**: Clone it with `--recurse-submodules`. The sample's source project lives in the `ss_player/SpriteStudio-SDK` submodule, and without it the sample has nothing to convert.
3. **Download GDExtension**: Get the latest package from [Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases) and extract it.
4. **Prepare Sample**: Copy the extracted `addons` folder into the [examples/Ringo](./examples/Ringo) folder of this repository.
5. **Convert and check**: `ssab_generated/` is not committed, so the sample has no `.ssab` yet, and opening the project does not create one. Open the [examples/Ringo](./examples/Ringo) project in Godot Engine and drag `ss_player/SpriteStudio-SDK/tests/Ringo/Ringo.sspj` from your file manager onto the **SS Import** dock (see [Asset Import and Editor Integration](./docs/en/workflow/usage_asset_pipeline.md)); the sample's `.ssplayer_sources.cfg` already names `res://ssab_generated` as the output folder. Without the editor, `./scripts/deploy-examples.sh` (`deploy-examples.ps1` on Windows) converts every sample; it builds `ssconverter-cli`, so it needs a Rust toolchain. Then open `Ringo.tscn` to see the animation working.

### 2. Introduce to Your Project

1. **Install**: Copy the `addons` folder into your Godot project root.
2. **Import**: Drag & drop your `.sspj` onto the Godot editor to convert it to `.ssab`.
3. **Play**: Add a `SpriteStudioPlayer2D` node and assign the `.ssab` to its `Ssab` property.

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
