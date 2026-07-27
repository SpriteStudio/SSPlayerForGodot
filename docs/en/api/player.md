# SpriteStudioPlayer2D

A playback node extending `Node2D`.
Specify a resource and an animation, then start playback.

```gdscript
@onready var ssnode: SpriteStudioPlayer2D = $target

func _ready() -> void:
    # Load the .ssab and assign it as the resource
    var ssab: SSABResource = ResourceLoader.load("res://ssab_generated/Sample.ssab")
    ssnode.set_ssab_resource(ssab)

    # Specify the animation name
    ssnode.set_animation("anime_1")

    # Play
    ssnode.set_loop_count(-1)  # -1 = infinite loop
    ssnode.set_speed_scale(1.0)
    ssnode.play()
```

## Main methods

* `set_ssab_resource(res: SSABResource)` / `get_ssab_resource() -> SSABResource`
* `set_animation(name: String)` / `get_animation() -> String`
* `set_autoplay(autoplay: bool)` / `is_autoplay() -> bool`: Whether to start playing automatically when the scene starts.
* `set_offset(offset: Vector2)` / `get_offset() -> Vector2`: Shifts the drawing position without moving the Node2D's origin.
* `set_flip_h(flip: bool)` / `is_flipped_h() -> bool`: Flips the animation horizontally.
* `set_flip_v(flip: bool)` / `is_flipped_v() -> bool`: Flips the animation vertically.
* `set_animation_process_mode(mode: int)` / `get_animation_process_mode() -> int`: Sets whether to sync with `_physics_process` (`0` / Physics) or `_process` (`1` / Idle).
* **In-editor preview**: Select the node and use the **SpriteStudio** bottom panel (play / pause / stop / frame scrubber) to preview without running the game. *(The former `editor_playing` inspector toggle has been replaced by this panel.)*
* `play(start_frame: float = -1.0)`: Starts playback. If `start_frame` is `-1.0`, it plays from the current frame or the start of the section.
* `pause()`: Pauses playback while retaining the current frame.
* `stop()`: Stops playback and typically resets the state.
* `is_playing() -> bool` / `is_pausing() -> bool`
* `set_frame(frame: float)` / `get_frame() -> float` / `get_total_frames() -> int`
* `set_speed_scale(speed_scale: float)` / `get_speed_scale() -> float`
* `set_frame_rate(fps: int)` / `get_frame_rate() -> int`
* `set_animation_section(start: int, end: int)`: Limits the playback to a specific frame range.
* `set_playback_direction(direction: int, style: int)`: Sets the playback direction and style. See the table below for values.
* `set_loop_count(count: int)` / `get_loop_count() -> int`: `n` plays `n` cycles then stops (`1` plays once). `-1` means infinite loop (`0` is an alias for infinite).
* `set_frame_skip_enabled(enabled: bool)` / `is_frame_skip_enabled() -> bool` (default: `true`)
* `set_sub_frame_enabled(enabled: bool)` / `is_sub_frame_enabled() -> bool` (default: `false`)
* `set_cellmap_texture(cellmap_name: String, texture: Texture2D)` / `get_cellmap_texture(cellmap_name: String) -> Texture2D`

### Arguments for `set_playback_direction`

| Argument | Value | Meaning |
| --- | --- | --- |
| `direction` | `0` | Forward |
| `direction` | `1` | Backward |
| `style` | `0` | Normal / One-way |
| `style` | `1` | PingPong (Round-trip) |

## Part queries

* `get_part_names() -> PackedStringArray`: Every part name in the asset (`.ssab`). Parts do not depend on the animation, so the list is the same for every animation in that asset.
* `get_part_index(part_name: String) -> int`: Resolves a part name to its part index, or `-1` if it does not exist.
* `get_part_transform(part_name: String) -> Transform2D`: The part's `Transform2D` on the current frame, in the player node's local space (`flip_h` / `flip_v` / `offset` included). Returns the identity when the part is unknown.
* `is_part_hidden(part_name: String) -> bool`: Whether the part is hidden on the current frame.

See [Scripting and Event-Driven Control → Part Tracking](../workflow/usage_scripting.md) for `SpriteStudioPartAttachment2D`, the node that makes another node follow a specified part.

## Part overrides

Override a single part's color / cell / visibility so that it wins over the keyframes. Every method returns `true` on success, or `false` when the part is unknown or the runtime rejects the call. See [Scripting and Event-Driven Control → Part Overrides](../workflow/usage_scripting.md) for the details and caveats.

* `set_part_color_override(part_name: String, color: Color, blend_op: int = 0, priority: int = 1) -> bool`
* `set_part_cell_override(part_name: String, cellmap_name: String, cell_name: String, priority: int = 1) -> bool`
* `set_part_visibility_override(part_name: String, force_hidden: bool, cascade: bool = false) -> bool`
* `clear_part_color_override(part_name: String) -> bool` / `clear_part_cell_override(part_name: String) -> bool` / `clear_part_visibility_override(part_name: String) -> bool`
* `clear_all_part_overrides() -> bool`
* Each method has an index-based variant `*_by_index(part_index: int, ...)` that skips the name lookup (get the index from `get_part_index()`).

### Values for `blend_op`

| Value | Blend operation |
| --- | --- |
| `0` | Mix (default) |
| `1` | Mul (multiply) |
| `2` | Add |
| `3` | Sub (subtract) |

### Values for `priority`

| Value | Priority mode | Meaning |
| --- | --- | --- |
| `0` | OverwriteOnNextKeyframe | Applies until the animation updates that attribute |
| `1` | HoldUntilNextAnimation (default) | Applies for the current animation; cleared when a new animation is set up |
| `2` | Permanent | Applies for as long as the same `.ssab` is playing, surviving animation changes |

> [!NOTE]
> `set_part_visibility_override` has no `priority`. It always wins over the keyframes and is always cleared when a new animation is set up.

## Signals

| Signal | Arguments | Emitted When |
| --- | --- | --- |
| `animation_started` | `anim_name: String` | Playback starts |
| `animation_changed` | `anim_name: String` | The animation name is changed |
| `animation_finished` | `anim_name: String` | Every configured loop has been played. Never emitted under an infinite loop |
| `animation_looped` | `anim_name: String` | The animation looped back to the start. Not emitted on the final cycle, which emits `animation_finished` instead |
| `frame_updated` | `frame_no: float` | The frame's part poses have just been finalized (right after the player's update, before the render phase). Which process it fires in follows `animation_process_mode` |
| `user_data` | `payload: Dictionary` | A "User Data" keyframe on the timeline is hit |
| `signal_emitted` | `command: String, value: Dictionary` | A "Signal" keyframe on the timeline is hit |
| `audio` | `payload: Dictionary` | An "Audio" keyframe on the timeline is hit |

### `user_data` payload fields

The User Data values configured in SpriteStudio are delivered as a `Dictionary`. **Only the keys that were set are present** — unset fields are omitted entirely.

| Key | Type | Meaning |
| --- | --- | --- |
| `integer` | `int` | Integer value |
| `point` | `Vector2` | Point value |
| `rect` | `Rect2` | Rectangle value (`x`, `y`, `width`, `height`) |
| `string` | `String` | String value |

### `signal_emitted` value fields

The parameters configured on the timeline "Signal" keyframe are delivered as a `Dictionary` keyed by parameter ID, with each value as `bool` / `int` / `float` / `String`, etc. The `command` argument receives the signal name (`command_id`).

### `audio` payload fields

The information configured on the timeline audio keyframe is delivered as a `Dictionary`. The player does not play sound itself, so bridge it to an `AudioStreamPlayer` (or similar) on the game side.

| Key | Type | Meaning |
| --- | --- | --- |
| `part_index` | `int` | Index of the part that fired |
| `sound_list_name_hash` | `int` | Hash of the sound list name |
| `sound_name_hash` | `int` | Hash of the sound name |
| `sound_name` | `String` | Sound name (present only when set) |
| `loop_num` | `int` | Loop count |

> [!NOTE]
> For the exact types and the latest set of accepted values, also refer to the implementation files `ss_player/ss_player_node_2d.h` and `ss_player/ss_internal_player.cpp`.

## Driving from an AnimationPlayer

The `frame` property is animatable, so an `AnimationPlayer` can scrub a SpriteStudio animation in lockstep with its own timeline (and any other tracks on it — audio, calls, other nodes).

1. Assign the `Ssab` resource and pick an `Animation` on the `SpriteStudioPlayer2D` as usual.
2. In the `AnimationPlayer`, add a **Property Track** targeting the node's `frame` property.
3. Keyframe `frame` over time (e.g. `0` → the last frame across the desired duration). `frame` is a float, so values interpolate.
4. Play the `AnimationPlayer`.

> [!IMPORTANT]
> While the `AnimationPlayer` drives `frame`, do **not** let the node play itself — leave `Autoplay` off and don't call `play()`. Otherwise the node's own playback and the keyframed `frame` fight each other every frame.

No setup beyond this is required: keyframe values live in the `AnimationPlayer`'s animation (the node's `frame` is not stored in the scene), and the same track drives playback at runtime.
