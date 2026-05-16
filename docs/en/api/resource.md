# Resource classes

These extend Godot's built-in `Resource`. When multiple `SpriteStudioPlayer2D` nodes share the same resource, set the **Local To Scene** flag to `True` to give each node its own state.

## SSABResource

Resource class corresponding to `.ssab` (animation binary).

Main methods:

* `load_from_file(path: String) -> Error`
* `save_to_file(path: String) -> Error`
* `is_valid() -> bool`
* `get_animation_count() -> int`
* `get_animation_names() -> PackedStringArray`

## SSQBResource

Resource class corresponding to `.ssqb` (sequence binary).

Main methods:

* `load_from_file(path: String) -> Error`
* `save_to_file(path: String) -> Error`
