# Scripting and Event-Driven Control

This page explains how to control `SpriteStudioPlayer2D` using Godot's GDScript.
The intuitive API aligns with Godot's design philosophy (nodes and signals), making it very easy to integrate into your game logic.

---

## Intuitive Playback Control

Just like operating from the Inspector, you can control animations using simple method calls from your scripts.

```gdscript
extends Node2D

@onready var ss_player = $SpriteStudioPlayer2D

func _ready():
    # Specify the animation name
    ss_player.set_animation("attack")
    # Start playback
    ss_player.play()

func _process(delta):
    # Pause/Resume with the Space key
    if Input.is_action_just_pressed("ui_accept"):
        if ss_player.is_playing():
            ss_player.pause()
        else:
            ss_player.play()
```

---

## Implementing Event-Driven Logic with Signals

One of the most powerful features for Godot users is event linkage using "Signals".
`SpriteStudioPlayer2D` emits useful signals when its playback state changes or when user data is triggered.

### Key Signals

* `animation_changed(anim_name)`: Emitted when the animation is changed.
* `animation_started(anim_name)`: Emitted when animation playback starts.
* `animation_finished(anim_name)`: Emitted once every configured loop has been played (never under an infinite loop).
* `animation_looped(anim_name)`: Emitted when the animation loops and returns to the beginning.
* `user_data(payload)`: Emitted when reaching a frame containing user data (events) configured in the animation.

### Example: Sequential Animation Playback
Here is an example where an "idle" animation automatically plays after an "attack" animation finishes.

```gdscript
func _ready():
    # You can also connect via the editor UI (Node tab), but to do it via code:
    ss_player.animation_finished.connect(_on_animation_finished)

func _on_animation_finished(anim_name: String):
    if anim_name == "attack":
        # Return to idle state after the attack
        ss_player.set_animation("idle")
        ss_player.play()
```

> [!NOTE]
> ![Signal connection screen](../../assets/6-connect_signals_node_tab.png)

### Example: Triggering Events using User Data
This is an example of receiving user data configured in SpriteStudio (such as playing footsteps or generating attack hitboxes) and processing it in the game.

```gdscript
func _ready():
    # Connect the user data signal
    ss_player.user_data.connect(_on_user_data)

func _on_user_data(payload):
    # payload is a Dictionary; only the keys that were set are present (string / integer / point / rect)
    # Example: Check the string set as user data and process accordingly
    if payload.get("string") == "play_footstep":
        $AudioStreamPlayer.play()
    elif payload.get("string") == "attack_hit":
        # Example of passing the damage amount using an integer value
        var damage = payload.get("integer", 0)
        spawn_hitbox(damage)
```

---

## Dynamic Texture Replacement (Avatar Customization)

When you want to change character equipment in-game, you can dynamically replace the texture of specific parts (cell maps) from your code.

### Example: Changing Outfits

```gdscript
func change_costume():
    # Use the cell map name defined in SpriteStudio (retrievable via get_cellmap_names(); shown under CellMap Overrides in the Inspector)
    var new_costume_texture = preload("res://assets/sailor_uniform.png")
    ss_player.set_cellmap_texture("Clothes 1", new_costume_texture)
```

This feature allows you to build an efficient avatar system without needing to prepare multiple animation variations for each part.

> [!TIP]
> ![Before outfit change](../../assets/7-cellmap_override_before.png)
> ![After outfit change](../../assets/7-cellmap_override_after.png)

---

## Part Overrides (Color / Cell / Visibility)

Per-part runtime overrides let a script say "make this part this color / this cell / hidden **now**". An override wins over both the keyframe and any animation blending, so it does not have to fight the animation.

```gdscript
@onready var ss_player = $SpriteStudioPlayer2D

func _ready():
    # Tint a part red (multiply). Applies to normal (image) parts.
    ss_player.set_part_color_override("body", Color.RED, 1)  # 1 = Mul

    # Make a part draw a different cell (cell map name is written without ".ssce").
    ss_player.set_part_cell_override("body", "Ringo", "effect3")

    # Force-hide a part, cascading to its descendants.
    ss_player.set_part_visibility_override("body", true, true)

    # Revert
    ss_player.clear_part_color_override("body")
    ss_player.clear_all_part_overrides()
```

| Method | Description |
|---|---|
| `get_part_index(part_name)` | Part index, or `-1` if the part is not in the asset |
| `set_part_color_override(part_name, color, blend_op = 0, priority = 1)` | Color override (single color) |
| `set_part_cell_override(part_name, cellmap_name, cell_name, priority = 1)` | Draw a different cell |
| `set_part_visibility_override(part_name, force_hidden, cascade = false)` | Force-hide (`force_hidden = false` reverts to the animation) |
| `clear_part_color_override` / `clear_part_cell_override` / `clear_part_visibility_override` | Clear one override on one part |
| `clear_all_part_overrides()` | Clear every override on the player |
| `*_by_index(part_index, ...)` | Part-index variant of each method above (skips the name lookup) |

Every method returns `false` when the part is unknown or the runtime rejects the call.

The cell map / cell names you can pass to a cell override are enumerated from the resource:

```gdscript
var ssab := ss_player.get_ssab_resource()
print(ssab.get_cellmap_names())        # -> ["Ringo", ...]
print(ssab.get_cell_names("Ringo"))    # -> ["effect3", ...]
```

> **On choosing between a texture swap and a cell override**: `set_cellmap_texture()` in the previous section replaces a **whole cell map (texture)** at once, affecting every part that uses it. This feature instead replaces the cell that a **single part** draws. Pick whichever matches your intent.

> **On using part indices**: Part indices are stable within one asset (the same `.ssab`), so if you set overrides frequently, resolve the name once with `get_part_index()` and reuse that index with the `*_by_index()` variants.

### Blend operation (`blend_op`)

The `blend_op` of `set_part_color_override()` offers the same four operations as the keyframed Part Color.

| Value | Blend operation |
|---|---|
| `0` | Mix (default) |
| `1` | Mul (multiply) |
| `2` | Add |
| `3` | Sub (subtract) |

An out-of-range value fails the call and returns `false`.

### Priority mode (`priority`)

Color and cell overrides conflict with the animation, so they take a `priority` (visibility does not — it is a plain force-hide flag, and any new animation clears it):

| Value | Priority mode | Behavior |
|---|---|---|
| `0` | OverwriteOnNextKeyframe | The override applies until the animation data updates that attribute |
| `1` | HoldUntilNextAnimation (default) | The override wins for the current animation and is cleared when a new animation is set up |
| `2` | Permanent | The override applies for as long as the same animation data (`.ssab`) is playing, surviving animation changes |

### Notes

- **Color** applies to normal parts, **cell** to normal and mask parts; other part types silently ignore the override (the call still returns `true`).
- Colors are interpreted in the same 8-bit sRGB space as the authored Part Color, and alpha is pre-multiplied by the runtime — pass the color as authored, without converting it yourself.
- A cell override is resolved when you set it, so an unknown cell map / cell name fails immediately (returns `false`).
- Overrides live on the runtime, which owns their lifecycle. Do not re-apply them after an animation change; choose the priority mode that expresses what you want instead.
- Assigning a different `.ssab` resource clears every override, because part identity is lost.
- Overrides do not reach parts **inside** an instance part (the child animation runs as a separate player). Force-hiding the instance part itself does stop its contents from being drawn.

> **On when an override is not reflected in the drawing**: While playback is stopped or paused — or on any frame that does not advance — the drawing is not rebuilt, so setting or clearing an override will not appear on screen. Call `set_frame(get_frame())` to force a redraw when you need it reflected immediately.
