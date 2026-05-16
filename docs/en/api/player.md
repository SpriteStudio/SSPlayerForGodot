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
    ssnode.set_loop(0)        # 0 = infinite loop
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
* `set_playback_direction(direction: int, style: int)`: 
  * `direction`: `0` = Forward, `1` = Backward.
  * `style`: `0` = Normal (One-way), `1` = PingPong (Round-trip).
* `set_loop(count: int)` / `get_loop() -> int`: `0` means infinite loop.
* `set_skip_frames(enabled: bool)` / `is_skip_frames() -> bool`
* `set_sub_frame_enabled(enabled: bool)` / `is_sub_frame_enabled() -> bool`
* `set_cellmap_texture(cellmap_name: String, texture: Texture2D)` / `get_cellmap_texture(cellmap_name: String) -> Texture2D`

## Signals

* `animation_started(anim_name: String)`: Emitted when playback starts.
* `animation_changed(anim_name: String)`: Emitted when the animation name is changed.
* `animation_finished(anim_name: String)`: Emitted when playback reaches the end (non-looping).
* `animation_looped(anim_name: String)`: Emitted when the animation loops.
* `user_data(payload: Dictionary)`: Emitted when a "User Data" keyframe is hit.
* `signal(command: String, value: Dictionary)`: Emitted when a "Signal" keyframe is hit.

For exact behavior and accepted argument values, refer to `ss_player/ss_player_node_2d.h`.
