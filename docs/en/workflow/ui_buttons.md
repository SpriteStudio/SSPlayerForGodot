# 🔘 UI Buttons

This page is for making a menu or HUD button out of a SpriteStudio animation: one that reacts as the pointer moves over it and presses it, greys out when disabled, and takes clicks only on its artwork. `SpriteStudioButton` is a Godot button (`BaseButton`) drawn by the `SpriteStudioPlayer2D` placed under it, so `pressed`, `toggle_mode`, `disabled`, shortcuts, focus and button groups all work as they do on `Button`.

## Setting one up

1. Add a **`SpriteStudioButton`** node where the button goes — under a `Control`, a container or a `CanvasLayer`.
2. Add a **`SpriteStudioPlayer2D`** as its child, and set its `ssab` and `current_animation` in the Inspector as usual. An `.ssab` dropped on the 2D view lands under the scene root; drag the new player onto the button in the Scene dock. Until the button has a player, it shows a warning in the Scene dock.
3. Give the button its size — drag its handles, set `custom_minimum_size`, or let a container lay it out. The player's canvas is fitted into it (see [Fitting the artwork](#fitting-the-artwork)); the player's own `position` and `scale` are the button's to set.
4. Connect the button's `pressed` signal, as with any button.

The states play only while the game runs. In the editor the player shows what its own properties say, and the SpriteStudio bottom panel previews it as before.

## States and the labels they play

The button plays a different section of the player's animation for each state it is in:

| State | When |
| --- | --- |
| `normal` | none of the below |
| `hover` | the pointer is over the button |
| `pressed` | the button is held down |
| `disabled` | `disabled` is on |
| `focused` | the button has keyboard or gamepad focus, shown as `Button` shows it |

When several hold at once, the first of `disabled`, `pressed`, `hover`, `focused` wins.

In SpriteStudio, put **labels named after the states** — `normal`, `hover`, `pressed`, `disabled`, `focused`, lowercase — on the animation the player has selected. Each state plays from its label up to the frame before the next label of any name (or to the end of the animation), and loops. A state change starts the new section from its first frame at once.

An animation does not need every label. A state with none falls back:

| State | Falls back to |
| --- | --- |
| `hover`, `focused`, `disabled` | `normal` |
| `pressed` | `hover`, then `normal` |
| `normal` | the whole animation |

So an animation with only a `normal` and a `pressed` label already makes a working button.

### Toggle buttons

With `toggle_mode` on, a toggle that is on is **selected**, and every state looks for a `selected_` label first — `selected_normal`, `selected_hover` and so on — before the plain one, so the on position stays visible while the pointer is over it. Pressing goes to `selected_pressed`, then straight to `pressed`. A toggle with no `selected_` labels looks the same on and off.

## Playing other animations: `state_clips`

`state_clips` replaces the label a state is read from. Each entry names an `animation`, a `label` or both:

```gdscript
@onready var button: SpriteStudioButton = $StartButton

func _ready() -> void:
    button.state_clips = {
        "normal": {"animation": "wait1"},    # the whole of wait1
        "hover": {"animation": "walk"},
        "pressed": {"label": "push"},        # the "push" label of the selected animation
        "disabled": {"animation": "dead"},
    }
```

An entry also serves the states that fall back to it, and the keys take the `selected_` prefix too. One that names an animation or a label the `.ssab` does not have is reported once in the Output panel, and the state plays as if it had none.

The states are read from the animation the player has selected. Selecting another one on the player from a script makes that the animation they are read from. Leave the rest of the player's playback — section, loop count, play and stop — to the button.

## Fitting the artwork

| Property | Default | Meaning |
| --- | --- | --- |
| `stretch_mode` | `STRETCH_FIT` | `STRETCH_FIT` fits the whole canvas inside the button; `STRETCH_COVER` fills the button with it; `STRETCH_KEEP` keeps the authored size with the player's origin at the button's center |
| `alignment_horizontal` / `alignment_vertical` | `ALIGNMENT_CENTER` | Where the canvas sits when its shape differs from the button's: `ALIGNMENT_BEGIN` (left / top), `ALIGNMENT_CENTER`, `ALIGNMENT_END` (right / bottom) |

The canvas is the one the selected animation was authored in (see [The authored canvas](../api/player.md#the-authored-canvas)), so the fit does not change when a `state_clips` entry plays an animation with another canvas. Shift the artwork within it with the player's `offset`. Turn on the button's `clip_contents` to cut off what overhangs.

## Clicks on the artwork only

By default the whole rectangle takes the pointer. To take it only on the art:

| Property | Default | Meaning |
| --- | --- | --- |
| `hit_parts` | `[]` | The parts that take the pointer, by name; the Inspector offers the player's part names |
| `hit_on_artwork` | `false` | With `hit_parts` empty, take the pointer on any part the player draws |

A point counts when it is on the outline `get_part_quad()` reports for one of those parts on the frame shown. Elsewhere the pointer passes to what is behind the button. Keyboard, gamepad and screen-reader activation are not narrowed. With `clip_contents` off, art that overhangs the button takes the pointer too.

## Reduced motion

Turn on `reduced_motion` for players who ask for less movement. Each state then holds the first frame of its section instead of playing it — stopped, so it fires no user data or sounds. If the animation has a label named `reduced_motion`, `normal` plays that section instead, so a calm loop can stand in for the usual one; `pressed`, `disabled` and `selected_` sections stay held, keeping their look.

The button does not read the system setting itself. Godot reports it on some platforms:

```gdscript
func _ready() -> void:
    $StartButton.reduced_motion = DisplayServer.accessibility_should_reduce_animation() == 1
```

## Screen readers

The animation is decorative. Give the button the name a screen reader should announce with its `accessibility_name` property.

The full list of members is in the editor's class reference: **Help > Search Help...**, then `SpriteStudioButton`.
