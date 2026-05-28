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
    ssnode.set_speed(1.0)
    ssnode.play()
```

## Main methods

* `set_ssab_resource(res: SSABResource)` / `get_ssab_resource() -> SSABResource`
* `set_animation(name: String)` / `get_animation() -> String`
* `play(start_frame: float = -1.0)`: Starts playback. If `start_frame` is `-1.0`, it plays from the current frame or the start of the section.
* `pause()`: Pauses playback while retaining the current frame.
* `stop()`: Stops playback and typically resets the state.
* `is_playing() -> bool` / `is_pausing() -> bool`
* `set_frame(frame: float)` / `get_frame() -> float` / `get_total_frames() -> int`
* `set_speed(speed: float)` / `get_speed() -> float`
* `set_frame_rate(fps: int)` / `get_frame_rate() -> int`
* `set_animation_section(start: int, end: int)`: Limits the playback to a specific frame range.
* `set_playback_direction(direction: int, style: int)`: Sets the playback direction and style. See the table below for values.
* `set_loop_count(count: int)` / `get_loop_count() -> int`: `-1` means infinite loop. `0` plays once with no repeat. `n` repeats `n` times.
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

## Signals

| Signal | Arguments | Emitted When |
| --- | --- | --- |
| `animation_started` | `anim_name: String` | Playback starts |
| `animation_changed` | `anim_name: String` | The animation name is changed |
| `animation_finished` | `anim_name: String` | Playback reaches the end (non-looping only) |
| `animation_looped` | `anim_name: String` | The animation loops back to the start |
| `user_data` | `payload: Dictionary` | A "User Data" keyframe on the timeline is hit |
| `signal` | `command: String, value: Dictionary` | A "Signal" keyframe on the timeline is hit |

### `user_data` payload fields

The User Data values configured in SpriteStudio are delivered as a `Dictionary`. **Only the keys that were set are present** — unset fields are omitted entirely.

| Key | Type | Meaning |
| --- | --- | --- |
| `integer` | `int` | Integer value |
| `point` | `Vector2` | Point value |
| `rect` | `Rect2` | Rectangle value (`x`, `y`, `width`, `height`) |
| `string` | `String` | String value |

### `signal` value fields

The parameters configured on the timeline "Signal" keyframe are delivered as a `Dictionary` keyed by parameter ID, with each value as `bool` / `int` / `float` / `String`, etc. The `command` argument receives the signal name (`command_id`).

> [!NOTE]
> For the exact types and the latest set of accepted values, also refer to the implementation files `ss_player/ss_player_node_2d.h` and `ss_player/ss_internal_player.cpp`.
