# Migration from v1.x

When migrating from v1.x to this version, please note the following major changes:

## 1. Asset Format and Import Pipeline
- **v1.x**: SpriteStudio files like `.sspj`, `.ssae`, and `.ssce` were placed directly in the project and used as-is from Godot's FileSystem dock.
- **Current Version**: Utilizes a customized optimized binary format (`.ssab`). Please remove the `.sspj` and other source files that were placed in your Godot project during v1 (move them outside the project). Then, use the **SS Import Dock** to convert them into `.ssab` files.
  - For detailed conversion steps, please refer to [Editor Integration and Asset Iteration](workflow/usage_asset_pipeline.md).

## 2. Node and GDScript API Changes
- **Node Change**: The node to use has changed. Please use the new `SpriteStudioPlayer2D` node instead of the old `GdNodeSsPlayer`.
- **Resource Assignment**:
  - Previously, you had to assign multiple files via `res_player.res_project = load("...sspj")` and `set_anime_pack("...ssae")`. In the current version, a `.ssab` file is generated per `.ssae` (Anime Pack).
  - Therefore, you simply need to set the `.ssab` containing the animation you want to play using `set_ssab_resource(load("...ssab"))`.
- **Loop Playback**: The previous `set_loop(bool)` method has been replaced with `set_loop_count(int)` (`-1` for infinite loop, `0` for play once / no loop).
- **Method Renames and Removals**:
  - `set_player_resource()` → `set_ssab_resource()`
  - `get_fps()` → `get_frame_rate()`
  - `set_anime_pack()` → Removed (Anime packs are now bundled within each `.ssab` file)
  - `set_play()` / `get_play()` → Removed (Use `play()`, `stop()`, and `is_playing()` instead)
  - `pause(bool)` → `pause()` (No longer takes arguments)
  - `set_texture_interpolate()` → Removed (Use Godot 4's native `Texture Filter` property on the CanvasItem instead)
- **Signal Names**: Signal names have been updated to follow Godot's standard conventions (e.g., removing the `on_` prefix).
  - `on_animation_finished` → `animation_finished`
  - `on_animation_changed` → `animation_changed`
  - `on_user_data` → `user_data`
  - For full details, see the [API Reference](api/player.md).
