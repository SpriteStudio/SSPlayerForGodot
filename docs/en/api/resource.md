# 🗃️ Resource classes

These extend Godot's built-in `Resource`. When multiple `SpriteStudioPlayer2D` nodes share the same resource, set the **Local To Scene** flag to `True` to give each node its own state.

## SSABResource

Resource class corresponding to `.ssab` (animation binary).

Textures and sounds referenced by the animation resolve **relative to the directory the resource was loaded from**, so keep the converter's output directory intact when moving files around.

Main methods:

* `load_from_file(path: String) -> Error` / `save_to_file(path: String) -> Error`
* `is_valid() -> bool`: Whether the held binary passes verification. The result is cached, since the buffer is immutable between loads.
* `get_animation_count() -> int` / `get_animation_names() -> PackedStringArray`
* `get_cellmap_names() -> PackedStringArray` / `get_cell_names(cellmap_name: String) -> PackedStringArray`: The same two lookups the player offers, for an `.ssab` that is not on a player yet. `get_cell_names()` returns an empty array for an unknown cell map.
* `get_sound_stream(sound_list_name_hash: int, sound_name_hash: int) -> AudioStream`: The stream for an audio event, or `null` when the referenced file is missing or unsupported. Both hashes come from the `audio` signal's payload. Results are cached per resource — including the misses, so a missing file is not re-resolved on every event.
* `get_sound_info(sound_list_name_hash: int, sound_name_hash: int) -> Dictionary`: The same lookup without loading anything: `alias` (the name authored in SpriteStudio), `file_path` (relative to this resource), `path` (the resolved project path), `file_path_hash` and `time_total`. Empty when the event is unknown.

See [Audio Playback](../workflow/audio.md) for how the two sound lookups are used.

## SSQBResource

Resource class corresponding to `.ssqb` (sequence binary).

Main methods:

* `load_from_file(path: String) -> Error`
* `save_to_file(path: String) -> Error`
