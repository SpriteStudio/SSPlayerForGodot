# 🗃️ Resource classes

These extend Godot's built-in `Resource`. Several `SpriteStudioPlayer2D` nodes can share one resource as it is: each node keeps its own playback state, and the players of one resource share one parse of it. Leave **Local To Scene** off, since it copies the bytes and the parse for every node. Any copy of a resource — Local To Scene, `duplicate()`, or making it unique in the Inspector — also has no directory to resolve its textures and sounds from.

## SSABResource

Resource class corresponding to `.ssab` (animation binary).

Textures and sounds referenced by the animation resolve **relative to the directory the resource was loaded from**, so keep the converter's output directory intact when moving files around.

Main methods:

* `load_from_file(path: String) -> Error` / `save_to_file(path: String) -> Error`
* `data: PackedByteArray`: The held binary, as a storage property that the Inspector does not show. Setting it replaces the binary as `load_from_file()` does, but textures and sounds still resolve from the directory the resource was loaded from. Bytes that are not a usable `.ssab` are not kept, and an error is printed. Reloading the resource in place goes through it, so the editor picks up a file changed on disk, and so does `ResourceLoader.load()` with `CACHE_MODE_REPLACE`.
* `is_valid() -> bool`: Whether the held binary passes verification. The result is cached, since the buffer is immutable between loads.
* `get_animation_count() -> int` / `get_animation_names() -> PackedStringArray`
* `get_cellmap_names() -> PackedStringArray` / `get_cell_names(cellmap_name: String) -> PackedStringArray`: The same two lookups the player offers, for an `.ssab` that is not on a player yet. `get_cell_names()` returns an empty array for an unknown cell map.
* `get_canvas_size(anim_name: String) -> Vector2` / `get_canvas_rect(anim_name: String) -> Rect2`: The canvas one animation was authored in, and that canvas placed against the origin by the animation's pivot. The player offers both for the animation it is playing; these address any animation in the asset by name, which is what a caller comparing them needs — the canvas is authored per animation. `Vector2.ZERO` / an empty `Rect2` for an unknown animation.
* `get_sound_stream(sound_list_name_hash: int, sound_name_hash: int) -> AudioStream`: The stream for an audio event, or `null` when the referenced file is missing or unsupported. Both hashes come from the `audio` signal's payload. Results are cached per resource — including the misses, so a missing file is not re-resolved on every event.
* `get_sound_info(sound_list_name_hash: int, sound_name_hash: int) -> Dictionary`: The same lookup without loading anything: `alias` (the name authored in SpriteStudio), `file_path` (relative to this resource), `path` (the resolved project path), `file_path_hash` and `time_total`. Empty when the event is unknown.

See [Audio Playback](../workflow/audio.md) for how the two sound lookups are used.

## SSQBResource

Resource class corresponding to `.ssqb` (sequence binary).

Main methods:

* `load_from_file(path: String) -> Error`
* `save_to_file(path: String) -> Error`
* `data: PackedByteArray`: The held binary, a storage property like `SSABResource.data`. Bytes that are not a usable `.ssqb` are not kept, and an error is printed.
