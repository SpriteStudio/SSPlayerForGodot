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

Track 0 carries the **structure**: cells, part colour, shaders, flips, hide, draw priority and textures all come from track 0, as do the events by default. Overlays blend the continuous attributes (position, rotation, scale, alpha and the rest).

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

`crossfade()` fades a track from what it plays to another animation, then hands the track to it and releases the outgoing one. Durations are in **seconds**, as `advance()` takes them:

```gdscript
player.crossfade(0, "run", 0.3)
```

| Case | What happens |
|---|---|
| A track playing nothing | No fade: the animation starts at once |
| A track already fading | The fade under way completes first, and the new one starts from there |
| `duration` of 0 or less, or an unknown name | Refused: `false` |
| Stopping a fade part-way | `cancel_track_fade(track)` — alpha and weight stay where they are |

`get_track_fade_progress(track)` reads 0 to 1 during a fade and **-1** when the track is not fading.

`fade_track()` is the other fade: it changes how much a track counts, not what it plays. Fading track 0 to 0 settles the character onto the setup pose:

```gdscript
player.fade_track(1, 0.0, 0.2)                # fade the overlay out over 0.2 s
```

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
- These settings act on **track 0 only**: `speed_scale`, the frame-rate override, frame skip, playback direction, loop count and the playback section. Each other track has its own speed (`set_track_speed`) and loops without end.
- An animation that enters track 0 by a crossfade starts with the runtime's defaults rather than the node's settings: it loops without end, whatever `loop_count` says. Set the loop count again after the crossfade completes if you need it.

## set_animation() folds the stack

`set_animation()` to a **different** animation puts the stack back to track 0 alone: every overlay, every source and every track setting (alpha, blend, sync) goes with it. Lay the overlays again after switching. Selecting the animation already playing changes nothing.

## Events

By default **only track 0's animation fires events** (`user_data`, `signal_emitted`, `audio`). Another source fires once its weight reaches its event threshold, which starts at `INF`:

```gdscript
var overlay: int = player.get_track_main_source(1)
player.set_source_event_threshold(overlay, 1.0)   # fires while its weight is 1 or more
```

Every event carries the handle of the source that fired it — `payload["source"]` for `user_data` and `audio`, `info["source"]` for `signal_emitted` — so events from two sources can be told apart. After a crossfade on track 0, the events are those of the incoming animation.

## Limits

- **One `.ssab`.** Only the animations of the resource assigned to the player can be stacked.
- **A crossfade on track 0 keeps what `set_animation()` built.** The Instance and Effect parts are set up for the animation `set_animation()` selected, and a crossfade does not set them up again; `get_current_animation()` still names that animation. Crossfade between animations of one `.ssae`, which share their parts.
- **Give every stacked animation the same cell on a part.** A cell's size and origin come from track 0, so a different cell on an overlay distorts the part.
- See also the runtime's own [animation mixing guide](https://github.com/cri-middleware/SpriteStudio-SDK/blob/HEAD/libs/ssruntime/docs/50_features/10_animation_mixing.md), which every official player follows.
