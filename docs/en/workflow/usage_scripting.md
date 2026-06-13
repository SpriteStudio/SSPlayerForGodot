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
* `animation_finished(anim_name)`: Emitted when animation playback finishes (for non-looping animations).
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
> ![Signal connection screen](../../assets/signal_connection.png)
> *(※ Image showing signals connected in the "Node" tab of the Godot editor will be placed here)*

### Example: Triggering Events using User Data
This is an example of receiving user data configured in SpriteStudio (such as playing footsteps or generating attack hitboxes) and processing it in the game.

```gdscript
func _ready():
    # Connect the user data signal
    ss_player.user_data.connect(_on_user_data)

func _on_user_data(payload):
    # Example: Check the string set as user data and process accordingly
    if payload.string_value == "play_footstep":
        $AudioStreamPlayer.play()
    elif payload.string_value == "attack_hit":
        # Example of passing the damage amount using an integer value (Int)
        var damage = payload.int_value
        spawn_hitbox(damage)
```

---

## Dynamic Texture Replacement (Avatar Customization)

When you want to change character equipment in-game, you can dynamically replace the texture of specific parts (cell maps) from your code.

### Example: Changing Weapons

```gdscript
func change_weapon():
    # Apply a new texture to the cell map named "weapon_map"
    var new_sword_texture = preload("res://assets/iron_sword.png")
    ss_player.set_cellmap_texture("weapon_map", new_sword_texture)
```

This feature allows you to build an efficient avatar system without needing to prepare multiple animation variations for each part.

> [!TIP]
> ![Before/After comparison of equipment change](../../assets/change_equipment.png)
> *(※ Before/After image comparison of the character changing weapons will be placed here)*
