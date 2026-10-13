# 🎚️ Motion Blending

One `SpriteStudioPlayer2D` can play several animations at once: lay a limp over a walk, nod the head by a tracked value, or slide from idle into an attack instead of cutting. Animations are stacked on **tracks**, and the runtime folds them into one pose. Nothing changes in the `.ssab`; every animation comes from the one assigned to the player.

The methods are listed in the [class reference](../api/player.md#motion-blending). This page is how they fit together.

## The track stack

Track 0 is the animation `set_animation()` selects. Tracks above it are laid over it in order, each on top of the pose the tracks below produced:

```
result = setup pose (the bare pose the animation's .ssae defines)
track 0  ->  laid on   (the main animation: set_animation)
track 1  ->  laid on   (an overlay)
track 2  ->  laid on   (another one)
```

Two numbers adjust a stack, and **which one you reach for matters**:

| | What it does | Per |
|---|---|---|
| **alpha** (`set_track_alpha`) | How much of the track lays over what is below. **This is the strength control.** | track |
| **weight** (`set_source_weight`) | The proportions between several animations sharing **one** track | source |

A track only acts on what its animation keys. An overlay that keys only the head leaves the legs alone, with no mask to set up — so **an overlay reaches exactly as far as it was animated**.

Track 0 carries the **structure**: cells, part colour, shaders, flips, hide, draw priority and textures all come from track 0's main animation, as do the events by default. Overlays blend the continuous attributes (position, rotation, scale, alpha and the rest).

## Laying an overlay

```gdscript
@onready var player: SpriteStudioPlayer2D = $Player

func _ready() -> void:
    player.set_animation("idle")              # track 0
    player.set_track_animation(1, "look_around")
    player.set_track_alpha(1, 0.4)            # 40 %
    player.play()
```

Tracks are created as needed: setting track 3 creates tracks 1 and 2 empty. `alpha` is not clamped to 0–1; above 1 it extrapolates.

### Additive tracks

A `TRACK_BLEND_ADDITIVE` track adds its offset **from the setup pose** instead of pulling toward its own pose, so what is below keeps showing through. It suits an axis driven by a value — author the axis so that its middle frame is the setup pose, then scrub it:

```gdscript
player.set_track_animation(1, "lean")
player.set_track_blend(1, SpriteStudioPlayer2D.TRACK_BLEND_ADDITIVE)
player.set_track_speed(1, 0.0)                # driven by hand, not by the clock

func _process(_delta: float) -> void:
    player.set_track_frame(1, 30.0 + yaw * 30.0)
```

## Switching with a crossfade

`crossfade()` fades a track from what it plays to another animation. The incoming animation takes the track over **from the call on**; everything the track was playing fades out together, keeping its proportions, and is released when the fade completes. Durations are in **seconds**, as `advance()` takes them:

```gdscript
player.crossfade(0, "run", 0.3)
```

| Case | What happens |
|---|---|
| A track playing nothing | No fade: the animation starts at once |
| A track already fading | The fade in flight is taken over as it stands, so the pose does not jump |
| A track mixing several sources, or left part-way by `cancel_track_fade` | Everything on it fades out together |
| `duration` of 0 or less, or an unknown name | Refused: `false` |
| Stopping a fade part-way | `cancel_track_fade(track)` — alpha and weight stay where they are |

`get_track_fade_progress(track)` reads 0 to 1 during a fade and **-1** when the track is not fading.

`fade_track()` is the other fade: it changes how much a track counts, not what it plays. Fading track 0 to 0 settles the character onto the setup pose:

```gdscript
player.fade_track(1, 0.0, 0.2)                # fade the overlay out over 0.2 s
```

### What a crossfade on track 0 changes, and when

The incoming animation takes track 0 over **when the fade begins**, not when it ends:

- `get_current_animation()` names it straight after the call, and `animation_changed` is sent then. `animation_started` is not: the switch carries on track 0's play state. `set_animation()` with the name the fade left behind switches back; with the incoming name it changes nothing, and the fade carries on. The canvas (`get_canvas_size()` / `get_canvas_rect()`) follows the switch too.
- Its cells, hide and the other values that cannot be blended show from the first frame of the fade. `is_playing()`, `is_finished()` and the `animation_*` signals report it: the outgoing animation running out during the fade does not finish the character.
- It plays with the node's speed, frame rate, frame skip, direction and `loop_count`, as if `set_animation()` had switched to it. Its section is its own whole length.
- Its events fire from the start of the fade, and the outgoing animation's stop — see [Events](#events) below.

`set_track_animation(0, ...)` switches track 0 the same way, without the fade.

## Mixing animations on one track

To blend by proportion — a walk and a run by the character's speed — put both on one track and set their weights:

```gdscript
player.set_animation("walk")
var walk: int = player.get_track_main_source(0)
var run: int = player.add_track_source(0, "run", 0.0)

func _process(_delta: float) -> void:
    player.set_source_weight(walk, 1.0 - speed_ratio)
    player.set_source_weight(run, speed_ratio)
```

Each animation on a track is a **source**, named by an integer handle (`-1` for none; `0` is a valid handle). A handle stays valid until its source is released — a crossfade completing, `clear_track()`, or `set_animation()` to another animation. Weights are ratios: `(0.7, 0.3)` and `(7, 3)` mix the same.

## Starting in phase (`trackSync`)

An animation laid on a track starts from the head of its section. Laid over a walk half-way through its stride, a run puts its feet somewhere else. Set the track to `TRACK_SYNC_START` **before** laying, and what is laid next starts at the **phase** track 0's animation stands at — how far through its section it is, as a proportion — so animations of different lengths line up:

```gdscript
player.set_track_sync(0, SpriteStudioPlayer2D.TRACK_SYNC_START)
player.crossfade(0, "run", 0.3)                # the run starts in step with the walk
```

| | `TRACK_SYNC_FREE` (default) | `TRACK_SYNC_START` |
|---|---|---|
| Where a laid animation starts | The head of its section | Track 0's phase |
| Suited to | One-shot motions (a blink, an attack) | Cyclic motions laid together (walk and run) |

It applies to `set_track_animation`, `crossfade` (track 0 included, where the incoming animation starts at the outgoing one's phase) and `add_track_source`. It aligns once, when the animation is laid; both then run on the same clock, so they stay together only while their lengths and speeds match. A track that has nothing on it yet can take the setting.

## One clock for the whole character

Every track runs on the character's one clock, and the playback controls act on all of it:

| Call | Effect on the stack |
|---|---|
| `play()` | Heads **track 0** out to the start of its section and starts the clock. The other tracks keep their playheads |
| `pause()` / `resume()` | Hold / release every track and every fade |
| `stop()` | Stops the whole character where it stands. Only `play()` restarts it |

- **Track 0 finishing does not stop the clock.** Overlays and fades carry on after track 0 has played its last loop. `is_playing()`, `is_pausing()`, `is_finished()` and the `animation_*` signals describe track 0, and report its end as usual.
- The playback section is track 0's alone. The other playback settings are each track's — see the next section.

## A track's playback settings

The speed, the frame-rate override, frame skip and the playback direction belong to the **track**, not to the animation on it: they reach every animation on the track, both sides of a crossfade included, and every one laid on it later. Track 0's are the node's own `speed_scale`, `frame_rate`, `frame_skip_enabled` and `set_playback_direction()`.

```gdscript
player.set_track_speed(1, 0.5)
player.set_track_frame_rate(1, 24)
player.set_track_playback_direction(1,
        SpriteStudioPlayer2D.PLAYBACK_DIRECTION_FORWARD,
        SpriteStudioPlayer2D.PLAYBACK_STYLE_PING_PONG)
```

The **loop count** is the animation's instead: an animation laid on a track loops without end, so state the count after laying it. A track that has played its count out holds its last frame and stays mixed in until you fade or clear it:

```gdscript
player.set_track_animation(2, "hit_react")
player.set_track_loop_count(2, 1)              # once, then hold the last frame

func _process(_delta: float) -> void:
    if player.is_track_finished(2):
        player.clear_track(2)
```

Track 0's loop count is `loop_count` (`set_track_loop_count(0, ...)` writes the same), and the node puts it back on every animation track 0 switches to. `track_just_looped(track)` is true for the tick in which the track wrapped, as `just_looped()` is for track 0. Neither is a signal; only track 0's are (`animation_finished`, `animation_looped`).

## set_animation() folds the stack

`set_animation()` to a **different** animation puts the stack back to track 0 alone: every overlay, every source and every track setting (alpha, blend, sync) goes with it. Lay the overlays again after switching. Selecting the animation already playing changes nothing.

## Events

By default **only track 0's animation fires events** (`user_data`, `signal_emitted`, `audio`). Another source fires once its weight reaches its event threshold, which starts at `INF`:

```gdscript
var overlay: int = player.get_track_main_source(1)
player.set_source_event_threshold(overlay, 1.0)   # fires while its weight is 1 or more
```

Every event carries the handle of the source that fired it — `payload["source"]` for `user_data` and `audio`, `info["source"]` for `signal_emitted` — so events from two sources can be told apart.

From the moment a crossfade on track 0 begins, the incoming animation is the one that fires, and the outgoing side falls silent. To keep the outgoing side firing for part of the fade, give the track an outgoing event threshold: the outgoing side fires while its share of the track, `1 - progress`, is at least the threshold.

```gdscript
player.set_track_outgoing_event_threshold(0, 0.5)   # the outgoing side fires for the first half
```

The threshold is the track's setting and stays set. Its default, `INF`, silences the outgoing side as the fade begins.

## Limits

- **One `.ssab`.** Only the animations of the resource assigned to the player can be stacked.
- **Instance and Effect parts are outside motion blending.** They belong to the `.ssab`, not to one animation, so a crossfade on track 0 keeps them running instead of setting them up again. Which key drives such a part across the switch is not blended: where both animations key it on the same frame, it may carry on with the outgoing animation's setting.
- **Give every stacked animation the same cell on a part.** A cell's size and origin come from track 0, so a different cell on an overlay distorts the part.
- See also the runtime's own [animation mixing guide](https://github.com/cri-middleware/SpriteStudio-SDK/blob/HEAD/libs/ssruntime/docs/50_features/10_animation_mixing.md), which every official player follows.
