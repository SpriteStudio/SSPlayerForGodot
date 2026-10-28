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
* `set_animation(name: String)` / `get_current_animation() -> String`
* `set_autoplay(autoplay: bool)` / `is_autoplay() -> bool`: Whether to start playing automatically when the scene starts. **Default `true`** — a node placed in a scene has no code to call `play()` for it, so it plays. Set it off for a node you drive yourself, or one that should hold its first frame as a pose.
* `set_offset(offset: Vector2)` / `get_offset() -> Vector2`: Shifts the drawing position without moving the Node2D's origin.
* `set_flip_h(flip: bool)` / `is_flipped_h() -> bool`: Flips the animation horizontally.
* `set_flip_v(flip: bool)` / `is_flipped_v() -> bool`: Flips the animation vertically.
* `set_animation_process_mode(mode: AnimationProcessMode)` / `get_animation_process_mode() -> AnimationProcessMode`: Sets whether to sync with `_physics_process` (`ANIMATION_PROCESS_PHYSICS` / `0`) or `_process` (`ANIMATION_PROCESS_IDLE` / `1`), or to stop ticking on its own (`ANIMATION_PROCESS_MANUAL` / `2`).
* `advance(delta: float)`: Steps playback by `delta` seconds and emits `frame_updated`, exactly as an automatic tick would. Meant for `ANIMATION_PROCESS_MANUAL` — under the other modes it advances the animation *on top of* the node's own tick.
* **In-editor preview**: Select the node and use the **SpriteStudio** bottom panel — play from start / play from current / stop, a frame scrubber, and loop and speed controls — to preview without running the game. Shortcuts mirror the AnimationPlayer editor (**D** play from current, **Shift+D** play from start, **S** stop).
* `play(start_frame: float = -1.0)`: Starts playback. `-1.0` (the default) **rewinds to the start of the section** — its end when the direction is backward — rather than continuing from where the playhead is. Pass `get_frame_no()` to play on from the current position.
* `pause()`: Holds playback where it stands, keeping the current frame. **Idempotent** — pausing twice leaves it paused.
* `resume()`: Lifts the hold and carries on from the same frame. **Idempotent**, and a no-op on an animation that is stopped rather than held — `play()` is what starts a stopped animation, and it rewinds.
* `stop()`: Stops playback. The playhead **stays where it was**, so the node keeps drawing the frame it stopped on.
* `is_playing() -> bool`: `true` while playing, **including while paused** — a pause is a hold, not a stop. / `is_pausing() -> bool`: `true` only while held. Both are `false` on an animation that has never played, and after `stop()`.
* `is_finished() -> bool`: Whether every configured loop has played. A **state**, not a pulse — it latches on completion and holds until the next `play()` or animation change, so a caller that was not connected when `animation_finished` fired can still ask whether the run is over. Never `true` under an infinite `loop_count`, and **not raised by `stop()`** — a commanded stop is not a completion, which is what separates it from `not is_playing()`.
* `just_looped() -> bool`: Whether the last update crossed a loop boundary. A **pulse**, not a state — the runtime clears it at the top of every update, so it only reads `true` inside the tick that crossed (which is why it is not `is_looped`: reading it a tick late reads `false`). The `animation_looped` signal delivers the same edge if you would rather not poll.
* `get_animation_names() -> PackedStringArray`: Names of the animations in the assigned [SSABResource] — the same list the `current_animation` property is chosen from.
* `is_playing_forward() -> bool`: Which way the playhead is **actually** travelling. Not `get_playback_direction()`, which reports the configured heading: on a ping-pong return leg this reads `false` while that still reads `PLAYBACK_DIRECTION_FORWARD` (a speed of zero or below is a stop, not a reversal, and does not flip it either). **Gate audio on it** — SpriteStudio has no reverse audio, so a sound key crossed on a backward leg is not meant to sound, and this is the test the node itself applies when `play_audio` is on. `true` before anything has played: forward is the resting state.
* `set_frame_no(frame: float)` / `get_frame_no() -> float` / `get_total_frames() -> int`: `get_total_frames()` counts the frames of the current playback section (`get_end_frame() - get_start_frame() + 1`). Once `set_animation_section()` narrows the section, it returns the section's length, not the whole animation's.
* `get_start_frame() -> int` / `get_end_frame() -> int`: The first and last frame that actually plays — the current playback section. They return the same values as `get_animation_section_start()` / `get_animation_section_end()`, which is the whole animation until `set_animation_section()` narrows it.
* `set_speed_scale(speed_scale: float)` / `get_speed_scale() -> float`
* `set_frame_rate(fps: int)` / `get_frame_rate() -> int`
* `set_animation_section(start: int, end: int) -> bool`: Limits playback to the frames from `start` to `end`, both included, so `start == end` plays that one frame, the last frame included. An end outside the animation (`-1`, say) is the animation's edge on that side. When both ends are inside the animation and `end` is before `start`, it returns `false` and leaves the section and the playhead as they were; the Output panel names the pair, once for each pair.
* `set_animation_section_by_label(start_label: String, end_label: String, start_offset: int = 0, end_offset: int = 0) -> bool`: `set_animation_section()` with the ends named by labels of the current animation: from `start_label`'s frame plus `start_offset` to `end_label`'s frame plus `end_offset`, the rule an Instance part's start and end labels follow. `_start` and `_end` are the animation's own first and last frame, as in SpriteStudio, unless it has a real label of that name. It does not play. When the animation has no such label, or the frames are a section `set_animation_section()` refuses, it returns `false` and leaves the section and the playhead as they were; the Output panel says which, once for each call.
* `get_label_table(animation: String = "") -> Array`: The labels of `animation` (the current one when empty) in frame order, as `{ "name": String, "frame": int }` dictionaries. Empty for an animation the asset does not have. It is read out of the resource on every call, so keep the result rather than asking every frame.
* `set_animation_section_start(start: int)` / `get_animation_section_start() -> int` / `set_animation_section_end(end: int)` / `get_animation_section_end() -> int`: Moves one endpoint of the section while keeping the other. These back the `animation_section_start` / `animation_section_end` inspector properties. Each goes through `set_animation_section()`, so a start past the current end, or an end before the current start, is refused: move the other endpoint first, or set both with `set_animation_section()`.
* `set_playback_direction(direction: PlaybackDirection, style: PlaybackStyle)`: Sets the playback direction and style. See the table below for values.
* `get_playback_direction() -> PlaybackDirection` / `get_playback_style() -> PlaybackStyle`: Reads back the two halves of the setter individually.
* `frame_rate`, `playback_direction` and `playback_style` can also be read and written as properties (`player.playback_direction = ...`). They are runtime-only: not shown in the inspector and not saved with the scene.
* `set_loop_count(count: int)` / `get_loop_count() -> int`: `n` plays `n` cycles then stops (`1` plays once). `-1` means infinite loop (`0` is an alias for infinite).
* `set_frame_skip_enabled(enabled: bool)` / `is_frame_skip_enabled() -> bool` (default: `true`)
* `set_sub_frame_enabled(enabled: bool)` / `is_sub_frame_enabled() -> bool` (default: `false`)
* `set_cellmap_texture(cellmap_name: String, texture: Texture2D)` / `get_cellmap_texture(cellmap_name: String) -> Texture2D`: The same as the `cellmaps/<cell map name>` properties listed under the inspector's **CellMap Overrides** group.
* `get_cellmap_names() -> PackedStringArray` / `get_cell_names(cellmap_name: String) -> PackedStringArray`: Names read from the assigned `SSABResource` (empty when none is assigned) — the discovery half of `set_part_cell_override()`. Also available on [`SSABResource`](resource.md) itself for an `.ssab` that is not on a player.
* `set_play_audio(enabled: bool)` / `is_play_audio() -> bool` (default: `true`), `set_audio_volume(volume: float)` / `get_audio_volume() -> float`, `set_audio_bus(bus: StringName)` / `get_audio_bus() -> StringName` (default: `&"Master"`), `set_audio_backend(backend: SpriteStudioAudioBackend)` / `get_audio_backend() -> SpriteStudioAudioBackend`: Built-in audio playback. See [Audio](#audio) below.

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
* `is_part_skinned_mesh(part_name: String) -> bool`: Whether the part is a skinned mesh (`false` when the part is unknown). Skinned-mesh parts are not suitable follow targets, so `SpriteStudioPartAttachment2D` excludes them.
* `get_part_quad(part_name: String) -> PackedVector2Array`: The outline the part is drawn with on the current frame: four corners — top-left, top-right, bottom-right, bottom-left — in the player node's local space (`flip_h` / `flip_v` / `offset` included). The corners go round the part, so the array works with `draw_polyline()` and `Geometry2D.is_point_in_polygon()` as it is. A part with a cell is its drawn corners, skew and vertex deformation included; a Mesh part is the box around its vertices; a Shape is its size around its pivot. Empty for a part that is unknown or hidden, and for a Null, bone, Instance or Effect part, which has no size.
* `hit_test_part(point: Vector2, parts: PackedStringArray = []) -> int`: The index of the front-most part whose `get_part_quad()` outline holds `point` (the player node's local space — use `to_local()` for a point on the screen), or `-1`. With `parts`, only those parts are tested, and an unknown name is skipped. It reads the current frame and computes nothing else, so it is cheap enough for every pointer move.

See [Scripting and Event-Driven Control → Part Tracking](../workflow/usage_scripting.md) for `SpriteStudioPartAttachment2D`, the node that makes another node follow a specified part, and [UI Buttons](../workflow/ui_buttons.md) for `SpriteStudioButton`, which takes clicks on the parts you name.

## The authored canvas

* `get_canvas_size() -> Vector2`: The canvas the current animation was authored in, in the player node's units. `Vector2.ZERO` when nothing is set up.
* `get_canvas_rect() -> Rect2`: The same canvas placed against the node's origin by the animation's **pivot**, in the player node's local space (`flip_h` / `flip_v` / `offset` included, exactly as `get_part_transform`).

The pivot is where the origin sits inside the canvas. A character authored standing on the ground has its pivot on the canvas's bottom edge, so `get_canvas_rect()` reports a box entirely above the origin.

The canvas is authored **per animation**, not per asset — in the `Ringo` sample, `dead` is `1300 x 600` where most of the others are `800 x 600` — so if you keep the value, re-read it whenever `animation_changed` fires.

It is the box the artist composed in, not a measured bounding box: parts are free to draw outside it. Reading it costs nothing, while measuring a tighter one would mean stepping every frame of the animation at load.

To use this box to skip players that are off screen, see [Performance Tuning → Skipping Players That Are Off Screen](../workflow/tips.md#skipping-players-that-are-off-screen).

## Part overrides

Override a single part's color / cell / visibility so that it wins over the keyframes. Every method returns `true` on success, or `false` when the part is unknown or the runtime rejects the call; the Output panel then names the call and what it was given, once for each. See [Scripting and Event-Driven Control → Part Overrides](../workflow/usage_scripting.md) for the details and caveats.

* `set_part_color_override(part_name: String, color: Color, blend_op: ColorBlendOperation = COLOR_BLEND_MIX, priority: OverridePriority = OVERRIDE_PRIORITY_HOLD_UNTIL_NEXT_ANIMATION) -> bool`
* `set_part_color_override_corners(part_name: String, corners: PackedColorArray, blend_op: ColorBlendOperation = COLOR_BLEND_MIX, priority: OverridePriority = OVERRIDE_PRIORITY_HOLD_UNTIL_NEXT_ANIMATION) -> bool`: Four-corner (per-vertex) colour, for a gradient across the part. `corners` holds exactly four colours in the order left-top, right-top, left-bottom, right-bottom. Shares one override slot with `set_part_color_override` — the last call wins, and `clear_part_color_override` clears either kind.
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

## Motion blending { #motion-blending }

Play several animations of the assigned `.ssab` at once, stacked on tracks: track 0 is the animation `set_animation()` selects, and the tracks above it are laid over it. [Motion Blending](../workflow/motion_blending.md) explains how the pieces fit together.

* `set_track_animation(track: int, anim_name: String) -> bool`: Lays `anim_name` on `track`, replacing what it played. Tracks are created as needed. On track 0 it switches the current animation, as a crossfade does, and leaves the tracks above it in place.
* `get_track_count() -> int`
* `clear_track(track: int) -> bool`: Empties a track. Track 0 cannot be emptied — fade its alpha to 0 instead.
* `set_track_alpha(track: int, alpha: float) -> bool` / `get_track_alpha(track: int) -> float`: How much of the track lays over what is below. Not clamped to 0–1.
* `set_track_blend(track: int, blend: TrackBlend) -> bool` / `get_track_blend(track: int) -> TrackBlend`
* `set_track_sync(track: int, sync: TrackSync) -> bool` / `get_track_sync(track: int) -> TrackSync`: Where the animations laid on the track **next** start. Set it before laying.
* `set_track_frame(track: int, frame_no: float) -> bool` / `get_track_frame(track: int) -> float`
* `set_track_speed(track: int, speed: float) -> bool` / `get_track_speed(track: int) -> float`: A track's speed. This and the three settings below belong to the track: they reach every animation on it, both sides of a crossfade included. Track 0's is `speed_scale`, which writes the same value.
* `set_track_frame_rate(track: int, fps: int) -> bool` / `get_track_frame_rate(track: int) -> int`: Track 0's is `frame_rate`.
* `set_track_frame_skip_enabled(track: int, enabled: bool) -> bool` / `is_track_frame_skip_enabled(track: int) -> bool`: Track 0's is `frame_skip_enabled`.
* `set_track_playback_direction(track: int, direction: PlaybackDirection, style: PlaybackStyle) -> bool` / `get_track_playback_direction(track: int) -> PlaybackDirection` / `get_track_playback_style(track: int) -> PlaybackStyle`: Track 0's is `set_playback_direction()`.
* `set_track_loop_count(track: int, count: int) -> bool` / `get_track_loop_count(track: int) -> int`: The loop count of the animation on the track. An animation is laid looping forever (`-1`); a track with none reads `0`. Track 0's is `loop_count`, which the node puts back on every animation track 0 switches to.
* `is_track_finished(track: int) -> bool` / `track_just_looped(track: int) -> bool`: Track 0's are `is_finished()` / `just_looped()`. A finished track holds its last frame and stays mixed in. No signal is sent for either.
* `set_track_outgoing_event_threshold(track: int, threshold: float) -> bool` / `get_track_outgoing_event_threshold(track: int) -> float`: The outgoing side of a crossfade on the track fires its events while its share, `1 - progress`, is at least this. Starts at `INF`, which silences it as the fade begins.
* `crossfade(track: int, anim_name: String, duration: float) -> bool`: Fades the track from what it plays to `anim_name` over `duration` **seconds**. `anim_name` takes the track over from the call on: on track 0 it is the current animation from then, and `animation_changed` is sent. On an empty track it starts at once.
* `fade_track(track: int, target_alpha: float, duration: float) -> bool`: Drives the track's alpha to `target_alpha` over `duration` seconds.
* `get_track_fade_progress(track: int) -> float`: 0 to 1 during a fade, **-1** when the track is not fading.
* `cancel_track_fade(track: int) -> bool`: Stops a fade, leaving alpha and weight where they are.
* `get_track_main_source(track: int) -> int`: The handle of the source carrying the track's animation, **-1** for none.
* `add_track_source(track: int, anim_name: String, weight: float) -> int`: Lays another animation on the track beside what it carries, and returns its handle (**-1** on failure).
* `set_source_weight(source: int, weight: float) -> bool` / `get_source_weight(source: int) -> float`: A source's share within its track — a ratio, not an absolute value.
* `set_source_frame(source: int, frame_no: float) -> bool` / `get_source_frame(source: int) -> float`
* `set_source_event_threshold(source: int, threshold: float) -> bool` / `get_source_event_threshold(source: int) -> float`: The weight at or above which the source fires its events. Starts at `INF`, so only track 0's main source fires by default.

Each setter returns `false` when no animation is set up, the animation name is not in the `.ssab`, or the runtime refuses the call. `set_track_animation()`, `crossfade()`, `add_track_source()`, `set_track_blend()` and `set_track_sync()` also name the call in the Output panel, once for each call and what it was given. A read of a track or source that does not exist returns `0` (check `get_track_count()` first), except the two whose `0` is a real answer: `get_track_fade_progress()` returns `-1`, and the source handles are `-1`.

`set_animation()` to a different animation folds the stack back to track 0 alone, every track setting included. `play()` heads out track 0 only; `pause()`, `resume()` and `stop()` act on every track and fade; and track 0 finishing does not stop the overlays. `is_playing()`, `is_pausing()`, `is_finished()` and the `animation_*` signals describe track 0.

### Values for `TrackBlend`

| Constant | Value | Meaning |
| --- | --- | --- |
| `TRACK_BLEND_MIX` | `0` | Pull toward the track's pose by its alpha (default) |
| `TRACK_BLEND_ADDITIVE` | `1` | Add the track's offset from the setup pose |

### Values for `TrackSync`

| Constant | Value | Meaning |
| --- | --- | --- |
| `TRACK_SYNC_FREE` | `0` | A laid animation starts at the head of its section (default) |
| `TRACK_SYNC_START` | `1` | It starts at the phase track 0's animation stands at |

## Signals

| Signal | Arguments | Emitted When |
| --- | --- | --- |
| `animation_started` | `anim_name: String` | Playback starts |
| `animation_changed` | `anim_name: String` | The animation name is changed — by `set_animation()`, or by `crossfade()` / `set_track_animation()` on track 0 |
| `animation_finished` | `anim_name: String` | Every configured loop has been played. Never emitted under an infinite loop |
| `animation_looped` | `anim_name: String` | The animation looped back to the start. Not emitted on the final cycle, which emits `animation_finished` instead |
| `frame_updated` | `frame_no: float` | The frame's part poses have just been finalized (right after the player's update, before the render phase). Which process it fires in follows `animation_process_mode` |
| `user_data` | `payload: Dictionary` | A "User Data" keyframe on the timeline is hit |
| `signal_emitted` | `command: String, value: Dictionary, info: Dictionary` | A "Signal" keyframe on the timeline is hit |
| `audio` | `payload: Dictionary` | An "Audio" keyframe on the timeline is hit |

### `user_data` payload fields

The User Data values configured in SpriteStudio are delivered as a `Dictionary`. The four origin keys are **always** present; of the four value keys, **only the ones that were set are present** — an unset field is omitted entirely rather than defaulted, because `0` is a value an author can mean.

| Key | Type | Meaning |
| --- | --- | --- |
| `part_index` | `int` | Index of the part the key sits on |
| `part_name` | `String` | Name of that part |
| `frame_no` | `int` | The frame the key sits on. Not necessarily the frame it was noticed on — one tick can step across several |
| `source` | `int` | Handle of the source that fired it — see [Motion blending](#motion-blending) |
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
| `source` | `int` | Handle of the source that fired it |

### `audio` payload fields

The information configured on the timeline audio keyframe is delivered as a `Dictionary`. This signal is an **observation channel**: it fires in every playback direction and in the editor, independently of whether the built-in playback (`play_audio`) is on. Connect to it to react to a sound, or to replace playback entirely — see [Audio](#audio).

| Key | Type | Meaning |
| --- | --- | --- |
| `part_index` | `int` | Index of the part that fired |
| `part_name` | `String` | Name of that part |
| `frame_no` | `int` | The frame the key sits on |
| `source` | `int` | Handle of the source that fired it |
| `sound_list_name_hash` | `int` | Hash of the sound list name |
| `sound_name_hash` | `int` | Hash of the sound name |
| `sound_name` | `String` | Sound name (present only when set) |
| `loop_num` | `int` | Play count (`1` plays once; SpriteStudio has no infinite audio loop) |

> [!NOTE]
> Each signal's argument types are also in the editor's built-in class reference: **Help → Search Help**, then `SpriteStudioPlayer2D`.

## Audio

Audio parts play through Godot out of the box — the node owns a pooled set of `AudioStreamPlayer` voices and starts one whenever the playhead crosses an audio key while playing **forward**. [Audio Playback](../workflow/audio.md) covers the semantics (fire-and-forget, no seek re-sync, overlap on re-fire); this is the API surface.

| Member | Type | Default | Description |
| --- | --- | --- | --- |
| `play_audio` | `bool` | `true` | Whether the built-in player makes sound. `set_play_audio(false)` also stops any in-flight built-in voices |
| `audio_volume` | `float` | `1.0` | Linear volume in `[0, 1]` for the built-in voices. Ignored while `audio_backend` is assigned |
| `audio_bus` | `StringName` | `&"Master"` | Bus the built-in voices play on, as `AudioStreamPlayer.bus`: a name the bus layout does not have plays on `Master`. Ignored while `audio_backend` is assigned |
| `audio_backend` | `SpriteStudioAudioBackend` | *(none)* | Replaces the built-in player entirely |

### `SpriteStudioAudioBackend`

A `Resource` subclass with a single overridable method. Assigning one to `audio_backend` **suppresses the built-in playback**, `audio_volume` and `audio_bus` included, so the backend owns voice lifecycle and play counts.

* `play_audio(payload: Dictionary, ssab: SSABResource, player: Node) -> void`: Called once for each audio event the built-in playback would have sounded, with the same `payload` the `audio` signal carries. Like the built-in playback, it is not called while `play_audio` is off or while playback is not moving forward (`is_playing_forward()` is `false`); the `audio` signal is still sent in both cases. The default implementation does nothing.

```gdscript
extends SpriteStudioAudioBackend

func play_audio(payload: Dictionary, ssab: SSABResource, player: Node) -> void:
    var info := ssab.get_sound_info(payload["sound_list_name_hash"], payload["sound_name_hash"])
    if not info.is_empty():
        MyMiddleware.play(info["path"], payload["loop_num"])
```

Resolving a sound from the payload is done on [`SSABResource`](resource.md#ssabresource), via `get_sound_stream()` or `get_sound_info()`.

## Driving from an AnimationPlayer

The `frame_no` property is animatable, so an `AnimationPlayer` can scrub a SpriteStudio animation in lockstep with its own timeline (and any other tracks on it — audio, calls, other nodes).

1. Assign the `Ssab` resource and pick a `Current Animation` on the `SpriteStudioPlayer2D` as usual.
2. In the `AnimationPlayer`, add a **Property Track** targeting the node's `frame_no` property.
3. Keyframe `frame_no` over time (e.g. `0` → the last frame across the desired duration). `frame_no` is a float, so values interpolate.
4. Play the `AnimationPlayer`.

> [!IMPORTANT]
> While the `AnimationPlayer` drives `frame_no`, do **not** let the node play itself — leave `Autoplay` off and don't call `play()`. Otherwise the node's own playback and the keyframed `frame_no` fight each other every frame.

No setup beyond this is required: keyframe values live in the `AnimationPlayer`'s animation (the node's `frame_no` is not stored in the scene), and the same track drives playback at runtime.
