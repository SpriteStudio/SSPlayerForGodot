# Advanced Usage & Tips

## Handling Signals and User Data

SpriteStudio allows you to embed custom data in your animations. In Godot, you can listen for these via signals.

```gdscript
func _ready():
    # Connect to the user_data signal
    ss_player.user_data.connect(_on_ss_user_data)

    # Connect to the "signal" event
    # Note: 'signal' is a GDScript keyword, so we must use a string-based connection
    ss_player.connect("signal", _on_ss_signal)

func _on_ss_user_data(payload: Dictionary):
    # Example: Playing a sound effect based on user data
    if payload.has("se"):
        audio_player.stream = load(payload["se"])
        audio_player.play()

func _on_ss_signal(command: String, value: Dictionary):
    # 'command' contains the Command ID, 'value' contains the parameters
    print("Signal command received: ", command)
    print("Params: ", value)
```

## Dynamic Texture Overrides (CellMap Overrides)

You can swap out textures at runtime. This is useful for character customization or multi-colored variants of the same animation.

```gdscript
# Replace the texture for a specific cellmap
var new_skin = load("res://textures/hero_red.png")
ss_player.set_cellmap_texture("chara_skin", new_skin)

# To reset to the original texture, pass null
ss_player.set_cellmap_texture("chara_skin", null)
```

## Performance Considerations

### SSAB vs. SSQB
- Use **SSAB** for standard animations.
- Use **SSQB** (Sequences) when you want to manage multiple animations as a single timeline or state machine within SpriteStudio.

### Skip Frames and Sub-frames
- **Skip Frames**: Enable this for heavy animations to maintain playback speed even if rendering drops frames.
- **Sub Frame Enabled**: Enable this for smooth interpolation between keyframes, especially for slow-motion playback.
