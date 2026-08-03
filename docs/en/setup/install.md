# 📥 Installation

Steps for getting started with SpriteStudioPlayer for Godot.

## A. Use the SSPlayerForGodot GDExtension (Recommended)

The shortest path to using the plugin without any build work.

1. Download a 4.7-series Godot Engine from the [official site](https://godotengine.org/download/).
2. Download the GDExtension bundle for your platform from the [SSPlayerForGodot Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases).
3. Extract the ZIP and copy the `addons` folder it contains into your Godot project root directory.
   * If placed correctly, `res://addons/spritestudio/spritestudio.gdextension` should exist.
4. Restart the Godot editor and the `SpriteStudioPlayer2D` node and the SS Import Dock will become available.

## B. Use a Godot Engine with the SSPlayerForGodot Custom Module

If you want to build and use the plugin as a custom module, refer to the [Build Guide](./build.md).

## Next Steps

Once installation is complete, continue to one of the following:

- If you do **not** have an `.ssab` file yet → [Asset Import and Editor Integration](../workflow/usage_asset_pipeline.md) to generate `.ssab` from your SpriteStudio project (`.sspj`).
- If you already have an `.ssab` file in your project → [Basic Usage](../workflow/usage_basic.md) to place and play it in a scene.
