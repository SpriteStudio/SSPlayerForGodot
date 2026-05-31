# Performance Tuning and Advanced Settings

This section introduces settings and tips to extract maximum performance from SpriteStudioPlayerForGodot and perform advanced playback control.

## Performance and Quality Settings

### Skip Frames Enabled
This setting is useful in environments with high rendering loads, such as mobile devices or scenes displaying a large number of characters.
When the `Skip Frames Enabled` property of `SpriteStudioPlayer2D` is activated, if rendering processing is delayed, intermediate drawing is skipped to maintain the animation's playback speed (time progression within the game).

### Sub Frame Enabled
This is extremely effective when rendering on high-refresh-rate monitors (e.g., 144Hz) or when performing slow-motion effects within Godot.
Normally, animations are played back frame-by-frame according to the set FPS (e.g., 30FPS or 60FPS). However, enabling `Sub Frame Enabled` automatically interpolates between keyframes based on the current time, resulting in incredibly smooth and fluid animation.

---

## Choosing Between SSAB and SSQB

There are two types of animation binaries imported into Godot:

- **SSAB (Animation Binary)**
  - This is standard animation data. Generally, you will use this format.
- **SSQB (Sequence Binary)**
  - This is a "sequence" of multiple animations (e.g., `Walk` -> `Run` -> `Jump`) linked together on the timeline within SpriteStudio.
  - Use this when you want to reproduce the exact continuous playback designed in SpriteStudio directly in Godot, without writing complex state transition logic in your GDScript.
  - **Note (current status):** Loading into `SSQBResource` is supported, but **sequence playback is not available yet** (planned — subsumed into the runtime state machine in roadmap Phase 3, or a Player-side prototype). For now, chain animations yourself in GDScript via the `animation_finished` signal.
