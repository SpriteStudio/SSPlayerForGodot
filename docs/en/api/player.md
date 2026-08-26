# 🧩 SpriteStudioPlayer2D

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
* `set_autoplay(autoplay: bool)` / `is_autoplay() -> bool`: Whether to start playing automatically when the scene starts. **Default `true`** — a node placed in a scene has no code to call `play()` for it, so it plays. Set it off for a node you drive yourself, or one that should hold its first frame as a pose.
* `set_offset(offset: Vector2)` / `get_offset() -> Vector2`: Shifts the drawing position without moving the Node2D's origin.
* `set_flip_h(flip: bool)` / `is_flipped_h() -> bool`: Flips the animation horizontally.
* `set_flip_v(flip: bool)` / `is_flipped_v() -> bool`: Flips the animation vertically.
* `set_animation_process_mode(mode: AnimationProcessMode)` / `get_animation_process_mode() -> AnimationProcessMode`: Sets whether to sync with `_physics_process` (`ANIMATION_PROCESS_PHYSICS` / `0`) or `_process` (`ANIMATION_PROCESS_IDLE` / `1`), or to stop ticking on its own (`ANIMATION_PROCESS_MANUAL` / `2`).
* `advance(delta: float)`: Steps playback by `delta` seconds and emits `frame_updated`, exactly as an automatic tick would. Meant for `ANIMATION_PROCESS_MANUAL` — under the other modes it advances the animation *on top of* the node's own tick.
* **In-editor preview**: Select the node and use the **SpriteStudio** bottom panel — play from start / play from current / stop, a frame scrubber, and loop and speed controls — to preview without running the game. Shortcuts mirror the AnimationPlayer editor (**D** play from current, **Shift+D** play from start, **S** stop). *(The former `editor_playing` inspector toggle has been replaced by this panel.)*
* `play(start_frame: float = -1.0)`: Starts playback. `-1.0` (the default) **rewinds to the start of the section** — its end when the direction is backward — rather than continuing from where the playhead is. Pass `get_frame()` to play on from the current position.
* `pause()`: Holds playback where it stands, keeping the current frame. **Idempotent** — pausing twice leaves it paused.
* `resume()`: Lifts the hold and carries on from the same frame. **Idempotent**, and a no-op on an animation that is stopped rather than held — `play()` is what starts a stopped animation, and it rewinds.
* `stop()`: Stops playback. The playhead **stays where it was**, so the node keeps drawing the frame it stopped on.
* `is_playing() -> bool`: `true` while playing, **including while paused** — a pause is a hold, not a stop. / `is_pausing() -> bool`: `true` only while held. Both are `false` on an animation that has never played, and after `stop()`.
* `is_finished() -> bool`: Whether every configured loop has played. A **state**, not a pulse — it latches on completion and holds until the next `play()` or animation change, so a caller that was not connected when `animation_finished` fired can still ask whether the run is over. Never `true` under an infinite `loop_count`, and **not raised by `stop()`** — a commanded stop is not a completion, which is what separates it from `not is_playing()`.
* `just_looped() -> bool`: Whether the last update crossed a loop boundary. A **pulse**, not a state — the runtime clears it at the top of every update, so it only reads `true` inside the tick that crossed (which is why it is not `is_looped`: reading it a tick late reads `false`). The `animation_looped` signal delivers the same edge if you would rather not poll.
* `get_animation_names() -> PackedStringArray`: Names of the animations in the assigned [SSABResource] — the same list the `animation` property is chosen from.
* `is_playing_forward() -> bool`: Which way the playhead is **actually** travelling. Not `get_playback_direction()`, which reports the configured heading: on a ping-pong return leg this reads `false` while that still reads `PLAYBACK_DIRECTION_FORWARD` (a speed of zero or below is a stop, not a reversal, and does not flip it either). **Gate audio on it** — SpriteStudio has no reverse audio, so a sound key crossed on a backward leg is not meant to sound, and this is the test the node itself applies when `play_audio` is on. `true` before anything has played: forward is the resting state.
* `set_frame(frame: float)` / `get_frame() -> float` / `get_total_frames() -> int`
* `get_start_frame() -> int` / `get_end_frame() -> int`: The first and last frame that actually plays — the current playback section. They return the same values as `get_animation_section_start()` / `get_animation_section_end()`, which is the whole animation until `set_animation_section()` narrows it.
* `set_speed_scale(speed_scale: float)` / `get_speed_scale() -> float`
* `set_frame_rate(fps: int)` / `get_frame_rate() -> int`
* `set_animation_section(start: int, end: int)`: Limits the playback to a specific frame range.
* `set_animation_section_start(start: int)` / `get_animation_section_start() -> int` / `set_animation_section_end(end: int)` / `get_animation_section_end() -> int`: Moves one endpoint of the section while keeping the other. These back the `animation_section_start` / `animation_section_end` inspector properties.
* `set_playback_direction(direction: PlaybackDirection, style: PlaybackStyle)`: Sets the playback direction and style. See the table below for values.
* `get_playback_direction() -> PlaybackDirection` / `get_playback_style() -> PlaybackStyle`: Reads back the two halves of the setter individually.
* `set_loop_count(count: int)` / `get_loop_count() -> int`: `n` plays `n` cycles then stops (`1` plays once). `-1` means infinite loop (`0` is an alias for infinite).
* `set_frame_skip_enabled(enabled: bool)` / `is_frame_skip_enabled() -> bool` (default: `true`)
* `set_sub_frame_enabled(enabled: bool)` / `is_sub_frame_enabled() -> bool` (default: `false`)
* `set_cellmap_texture(cellmap_name: String, texture: Texture2D)` / `get_cellmap_texture(cellmap_name: String) -> Texture2D`
* `get_cellmap_names() -> PackedStringArray` / `get_cell_names(cellmap_name: String) -> PackedStringArray`: Names read from the assigned `SSABResource` (empty when none is assigned) — the discovery half of `set_part_cell_override()`. Also available on [`SSABResource`](resource.md) itself for an `.ssab` that is not on a player.
* `set_play_audio(enabled: bool)` / `is_play_audio() -> bool` (default: `true`), `set_audio_volume(volume: float)` / `get_audio_volume() -> float`, `set_audio_backend(backend: SpriteStudioAudioBackend)` / `get_audio_backend() -> SpriteStudioAudioBackend`: Built-in audio playback. See [Audio](#audio) below.

### Arguments for `set_playback_direction`

| Argument | Constant | Value | Meaning |
| --- | --- | --- | --- |
| `direction` | `PLAYBACK_DIRECTION_FORWARD` | `0` | Forward |
| `direction` | `PLAYBACK_DIRECTION_BACKWARD` | `1` | Backward |
| `style` | `PLAYBACK_STYLE_NORMAL` | `0` | Normal / One-way |
| `style` | `PLAYBACK_STYLE_PING_PONG` | `1` | PingPong (Round-trip) |

## Part queries

* `get_part_names() -> PackedStringArray`: Every part name in the asset (`.ssab`). Parts do not depend on the animation, so the list is the same for every animation in that asset.
* `find_part_index(part_name: String) -> int`: Resolves a part name to its part index, or `-1` if it does not exist.
* `get_part_transform(part_name: String) -> Transform2D`: The part's `Transform2D` on the current frame, in the player node's local space (`flip_h` / `flip_v` / `offset` included). Returns the identity when the part is unknown.
* `is_part_hidden(part_name: String) -> bool`: Whether the part is hidden on the current frame.

See [Scripting and Event-Driven Control → Part Tracking](../workflow/usage_scripting.md) for `SpriteStudioPartAttachment2D`, the node that makes another node follow a specified part.

## Part overrides

Override a single part's color / cell / visibility so that it wins over the keyframes. Every method returns `true` on success, or `false` when the part is unknown or the runtime rejects the call. See [Scripting and Event-Driven Control → Part Overrides](../workflow/usage_scripting.md) for the details and caveats.

* `set_part_color_override(part_name: String, color: Color, blend_op: ColorBlendOperation = COLOR_BLEND_MIX, priority: OverridePriority = OVERRIDE_PRIORITY_HOLD_UNTIL_NEXT_ANIMATION) -> bool`
* `set_part_color_override_corners(part_name: String, left_top: Color, right_top: Color, left_bottom: Color, right_bottom: Color, blend_op: ColorBlendOperation = COLOR_BLEND_MIX, priority: OverridePriority = OVERRIDE_PRIORITY_HOLD_UNTIL_NEXT_ANIMATION) -> bool`: Four-corner (per-vertex) colour, for a gradient across the part. Shares one override slot with `set_part_color_override` — the last call wins, and `clear_part_color_override` clears either kind.
* `set_part_cell_override(part_name: String, cellmap_name: String, cell_name: String, priority: OverridePriority = OVERRIDE_PRIORITY_HOLD_UNTIL_NEXT_ANIMATION) -> bool`
* `set_part_visibility_override(part_name: String, force_hidden: bool, cascade: bool = false) -> bool`
* `clear_part_color_override(part_name: String) -> bool` / `clear_part_cell_override(part_name: String) -> bool` / `clear_part_visibility_override(part_name: String) -> bool`
* `clear_all_part_overrides() -> bool`
* Each method has an index-based variant `*_by_index(part_index: int, ...)` that skips the name lookup (get the index from `find_part_index()`).

### Values for `blend_op`

| Constant | Value | Blend operation |
| --- | --- | --- |
| `COLOR_BLEND_MIX` | `0` | Mix (default) |
| `COLOR_BLEND_MUL` | `1` | Mul (multiply) |
| `COLOR_BLEND_ADD` | `2` | Add |
| `COLOR_BLEND_SUB` | `3` | Sub (subtract) |

### Values for `priority`

| Constant | Value | Meaning |
| --- | --- | --- |
| `OVERRIDE_PRIORITY_OVERWRITE_ON_NEXT_KEYFRAME` | `0` | Applies until the animation updates that attribute |
| `OVERRIDE_PRIORITY_HOLD_UNTIL_NEXT_ANIMATION` | `1` | Applies for the current animation; cleared when a new animation is set up (default) |
| `OVERRIDE_PRIORITY_PERMANENT` | `2` | Applies for as long as the same `.ssab` is playing, surviving animation changes |

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
| `signal_emitted` | `command: String, value: Dictionary, info: Dictionary` | A "Signal" keyframe on the timeline is hit |
| `audio` | `payload: Dictionary` | An "Audio" keyframe on the timeline is hit |

### `user_data` payload fields

The User Data values configured in SpriteStudio are delivered as a `Dictionary`. The three origin keys are **always** present; of the four value keys, **only the ones that were set are present** — an unset field is omitted entirely rather than defaulted, because `0` is a value an author can mean.

| Key | Type | Meaning |
| --- | --- | --- |
| `part_index` | `int` | Index of the part the key sits on |
| `part_name` | `String` | Name of that part |
| `frame_no` | `int` | The frame the key sits on. Not necessarily the frame it was noticed on — one tick can step across several |
| `integer` | `int` | Integer value |
| `point` | `Vector2` | Point value |
| `rect` | `Rect2` | Rectangle value (`x`, `y`, `width`, `height`) |
| `string` | `String` | String value |

### `signal_emitted` value and info fields

The parameters configured on the timeline "Signal" keyframe are delivered as `value`, a `Dictionary` keyed by parameter ID, with each value as `bool` / `int` / `float` / `String`, etc. The `command` argument receives the signal name (`command_id`).

The event's origin arrives as a **separate** `info` dictionary rather than as more keys in `value`, precisely because `value`'s keys are author-defined and a fixed key could shadow one of them.

| `info` key | Type | Meaning |
| --- | --- | --- |
| `part_index` | `int` | Index of the part the key sits on |
| `part_name` | `String` | Name of that part |
| `frame_no` | `int` | The frame the key sits on |

### `audio` payload fields

The information configured on the timeline audio keyframe is delivered as a `Dictionary`. This signal is an **observation channel**: it fires in every playback direction and in the editor, independently of whether the built-in playback (`play_audio`) is on. Connect to it to react to a sound, or to replace playback entirely — see [Audio](#audio).

| Key | Type | Meaning |
| --- | --- | --- |
| `part_index` | `int` | Index of the part that fired |
| `part_name` | `String` | Name of that part |
| `frame_no` | `int` | The frame the key sits on |
| `sound_list_name_hash` | `int` | Hash of the sound list name |
| `sound_name_hash` | `int` | Hash of the sound name |
| `sound_name` | `String` | Sound name (present only when set) |
| `loop_num` | `int` | Play count (`1` plays once; SpriteStudio has no infinite audio loop) |

> [!NOTE]
> For the exact types and the latest set of accepted values, also refer to the implementation files `ss_player/ss_player_node_2d.h` and `ss_player/ss_internal_player.cpp`.

## Audio

Audio parts play through Godot out of the box — the node owns a pooled set of `AudioStreamPlayer` voices and starts one whenever the playhead crosses an audio key while playing **forward**. [Audio Playback](../workflow/audio.md) covers the semantics (fire-and-forget, no seek re-sync, overlap on re-fire); this is the API surface.

| Member | Type | Default | Description |
| --- | --- | --- | --- |
| `play_audio` | `bool` | `true` | Whether the built-in player makes sound. `set_play_audio(false)` also stops any in-flight built-in voices |
| `audio_volume` | `float` | `1.0` | Linear volume in `[0, 1]` for the built-in voices. Ignored while `audio_backend` is assigned |
| `audio_backend` | `SpriteStudioAudioBackend` | *(none)* | Replaces the built-in player entirely |

### `SpriteStudioAudioBackend`

A `Resource` subclass with a single overridable method. Assigning one to `audio_backend` **suppresses the built-in playback**, `audio_volume` included, so the backend owns voice lifecycle and play counts.

* `play_audio(payload: Dictionary, ssab: SSABResource, player: Node) -> void`: Called once per audio event, with the same `payload` the `audio` signal carries. The default implementation does nothing.

```gdscript
extends SpriteStudioAudioBackend

func play_audio(payload: Dictionary, ssab: SSABResource, player: Node) -> void:
    var info := ssab.get_sound_info(payload["sound_list_name_hash"], payload["sound_name_hash"])
    if not info.is_empty():
        MyMiddleware.play(info["path"], payload["loop_num"])
```

Resolving a sound from the payload is done on [`SSABResource`](resource.md#ssabresource), via `get_sound_stream()` or `get_sound_info()`.

## Driving from an AnimationPlayer

The `frame` property is animatable, so an `AnimationPlayer` can scrub a SpriteStudio animation in lockstep with its own timeline (and any other tracks on it — audio, calls, other nodes).

1. Assign the `Ssab` resource and pick an `Animation` on the `SpriteStudioPlayer2D` as usual.
2. In the `AnimationPlayer`, add a **Property Track** targeting the node's `frame` property.
3. Keyframe `frame` over time (e.g. `0` → the last frame across the desired duration). `frame` is a float, so values interpolate.
4. Play the `AnimationPlayer`.

> [!IMPORTANT]
> While the `AnimationPlayer` drives `frame`, do **not** let the node play itself — leave `Autoplay` off and don't call `play()`. Otherwise the node's own playback and the keyframed `frame` fight each other every frame.

No setup beyond this is required: keyframe values live in the `AnimationPlayer`'s animation (the node's `frame` is not stored in the scene), and the same track drives playback at runtime.
