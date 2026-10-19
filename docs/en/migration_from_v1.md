# 🔄 Migration from v1.x

When migrating from v1.x to the current version, please note the following major changes:

## 1. Asset Format and Import Pipeline
- **v1.x**: SpriteStudio files like `.sspj`, `.ssae`, and `.ssce` were placed directly in the project and used as-is from Godot's FileSystem dock.
- **Current version**: Utilizes a customized optimized binary format (`.ssab`). Please remove the `.sspj` and other source files that were placed in your Godot project during v1 (move them outside the project). Then, use the **SS Importer** dock to convert them into `.ssab` files.
  - For detailed conversion steps, please refer to [Asset Import and Editor Integration](workflow/usage_asset_pipeline.md).

## 2. Node and GDScript API Changes
- **Node Change**: The node to use has changed. Please use the new `SpriteStudioPlayer2D` node instead of the old `GdNodeSsPlayer`.
- **Resource Assignment**:
  - Previously, you had to assign multiple files via `res_player.res_project = load("...sspj")` and `set_anime_pack("...ssae")`. In the current version, a `.ssab` file is generated per `.ssae` (Anime Pack).
  - Therefore, you simply need to set the `.ssab` containing the animation you want to play using `set_ssab_resource(load("...ssab"))`.
- **Loop Playback**: The previous `set_loop(bool)` method has been replaced with `set_loop_count(int)` (`-1` for infinite loop, `1` for play once / no loop).
- **Method Renames and Removals**:
  - `set_player_resource()` / `get_player_resource()` → `set_ssab_resource()` / `get_ssab_resource()`
  - `get_animation()` → `get_current_animation()`
  - `set_frame()` / `get_frame()` → `set_frame_no()` / `get_frame_no()`
  - `get_loop()` → `get_loop_count()`
  - `get_fps()` → `get_frame_rate()`
  - `set_anime_pack()` / `get_anime_pack()` → Removed (Anime packs are now bundled within each `.ssab` file)
  - `set_play()` / `get_play()` → Removed (Use `play()`, `stop()`, and `is_playing()` instead)
  - `pause(bool)` → `pause()` (No longer takes arguments)
  - `set_texture_interpolate()` / `get_texture_interpolate()` → Removed (Use Godot 4's native `Texture Filter` property on the CanvasItem instead)
- **Signals**: `animation_finished` and `animation_changed` keep their names. The others have changed:
  - `user_data(flag, int_value, rect_value, point_value, string_value)` → `user_data(payload: Dictionary)`
  - `signal(command, value)` → `signal_emitted(command, value, info)` (`signal` is a GDScript keyword)
  - `frame_changed(frame)` → `frame_updated(frame_no: float)`
  - `anime_pack_changed` → Removed (Anime packs are now bundled within each `.ssab` file)
  - For full details, see the [API Reference](api/player.md).
