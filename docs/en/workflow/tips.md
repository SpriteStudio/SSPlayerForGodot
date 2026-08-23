# ⚡ Performance Tuning and Advanced Settings

This section introduces settings and tips to extract maximum performance from SpriteStudioPlayerForGodot and perform advanced playback control.

## Performance and Quality Settings

### Skip Frames Enabled
This setting is useful in environments with high rendering loads, such as mobile devices or scenes displaying a large number of characters.
When the `Skip Frames Enabled` property of `SpriteStudioPlayer2D` is activated, if rendering processing is delayed, intermediate drawing is skipped to maintain the animation's playback speed (time progression within the game).

### Sub Frame Enabled
This is extremely effective when rendering on high-refresh-rate monitors (e.g., 144Hz) or when performing slow-motion effects within Godot.
Normally, animations are played back frame-by-frame according to the set FPS (e.g., 30FPS or 60FPS). However, enabling `Sub Frame Enabled` automatically interpolates between keyframes based on the current time, resulting in incredibly smooth and fluid animation.

### Driving Playback Yourself (Manual)

Setting **Animation Process Mode** to `Manual` stops the node advancing itself; nothing moves until a script calls `advance(delta)`.

```gdscript
@onready var ss_player: SpriteStudioPlayer2D = $SpriteStudioPlayer2D

func _ready() -> void:
    ss_player.set_animation_process_mode(SpriteStudioPlayer2D.ANIMATION_PROCESS_MANUAL)

func _process(delta: float) -> void:
    # A slow-motion group of your own, independent of Engine.time_scale
    # and of the node's own speed_scale.
    ss_player.advance(delta * _bullet_time)
```

`advance()` does exactly what an automatic tick does — including emitting `frame_updated`, so `SpriteStudioPartAttachment2D` children and anything else driven by that signal stay in step. Use it for custom pause groups, deterministic stepping (replays, tests, frame captures), or a time scale the engine's own must not touch.

> [!NOTE]
> **The editor preview does not animate under `Manual`.** The SpriteStudio bottom panel drives transport, not time — with the node not ticking itself, its play button starts a playback that nothing advances. Leave the mode on `Idle` while authoring and switch to `Manual` from a script.

> [!NOTE]
> `Manual` stops the *animation*; it does not freeze sounds already playing. Audio is fire-and-forget and keeps ticking, exactly as it does while the animation is paused. See [Audio Playback](audio.md).

> [!TIP]
> Prefer `speed_scale` for a simple slow-motion effect and the `frame` property for seeking. `Manual` is for when the *source of time itself* has to be yours.

---

## Part Add-On Shaders (Per-Part Effects)

The **add-on shaders** you can assign to a part in SpriteStudio are reproduced natively — no setup, no material to assign. The shader id is carried in the `.ssab`, and the plugin compiles a Godot shader for it on first use and caches it per `(shader, blend mode)` pair.

Thirteen effects are ported from the SS6 reference implementation:

| Id | Effect | Parameters |
| --- | --- | --- |
| `ss-sepia` | Sepia / grayscale tone | strength (signed) |
| `ss-hsb` | Hue / saturation / brightness shift | hue, saturation, brightness |
| `ss-step` | Brightness-driven posterise | threshold (signed), stages, mono↔tint mix |
| `ss-outline` | 4-neighbour alpha-edge outline | threshold (signed) |
| `ss-bmask` | Brightness-threshold mask (discards above or below) | threshold (signed) |
| `ss-blur` | 9-tap box blur | focus shift |
| `ss-move` | Directional smear / motion blur | distance, direction, power |
| `ss-wave` | Sine-warp horizontal displacement | width, height, phase |
| `ss-noise` | Per-texel pseudo-random noise overlay | power, color, phase |
| `ss-pix` | UV-quantise pixelation | block size |
| `ss-scatter` | Per-cell directional UV scatter | power, ratio, phase |
| `ss-circle` | Cell rect → polar unwrap | — |
| `ss-spot` | Radial spotlight gradient | — |

A part whose shader id is not in this list falls back to the default shader rather than failing to draw, so an unrecognized add-on shows the part untouched.

> [!IMPORTANT]
> **An add-on part costs a draw call.** Ordinary parts are merged into batches; a part carrying an add-on shader needs its own uniforms, so it is drawn on its own canvas item with its own material (both pooled and reused across frames, so this costs draw calls rather than allocations). Effects are therefore cheap to use in a few places and expensive to use everywhere — the same trade-off as any per-object material.

> [!NOTE]
> `ss-circle` needs the part's source rectangle to unwrap. Part types that do not bind one — Shape parts, for instance — draw nothing under it.

---

## Choosing Between SSAB and SSQB

There are two types of animation binaries imported into Godot:

- **SSAB (Animation Binary)**
  - This is standard animation data. Generally, you will use this format.
- **SSQB (Sequence Binary)**
  - This is a "sequence" of multiple animations (e.g., `Walk` -> `Run` -> `Jump`) linked together on the timeline within SpriteStudio.
  - Use this when you want to reproduce the exact continuous playback designed in SpriteStudio directly in Godot, without writing complex state transition logic in your GDScript.
  - **Note (current status):** Loading into `SSQBResource` is supported, but **sequence playback is not available yet** (planned — subsumed into the runtime state machine in roadmap Phase 3, or a Player-side prototype). For now, chain animations yourself in GDScript via the `animation_finished` signal.
