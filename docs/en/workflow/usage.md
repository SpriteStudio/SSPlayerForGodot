# Basic Usage

## Creating the SpriteStudio node

1. Add a **`SpriteStudioPlayer2D`** node to your scene (Node2D family).
2. Set the inspector's **SSAB Resource** property to an imported `.ssab` file (use `Load`).
3. Set the **Animation** property to the animation name to play. The animation names contained in the `.ssab` are listed in a dropdown.
4. Adjust playback-related properties (Frame, Speed, Loop, Playing, etc.) in the inspector for previewing.

## Main inspector properties

| Property                  | Type     | Description                                                          |
| ------------------------- | -------- | -------------------------------------------------------------------- |
| `SSAB Resource`           | Resource | The `SSABResource` (`.ssab`) to play                               |
| `Animation`               | String   | Currently selected animation name                                    |
| `Frame`                   | float    | Current frame position                                               |
| `Speed`                   | float    | Playback speed multiplier (default: 1.0)                             |
| `Frame Rate`              | int      | FPS                                                                  |
| `Loop`                    | int      | Loop count (`0` for infinite loop)                                   |
| `Playing`                 | bool     | Playback flag                                                        |
| `Animation Section Start` | int      | Partial-playback start frame                                         |
| `Animation Section End`   | int      | Partial-playback end frame                                           |
| `Playback Direction`      | int      | Playback direction                                                   |
| `Playback Style`          | int      | Playback style (one-way / round-trip etc.)                           |
| `Skip Frames`             | bool     | Whether to skip frames when the draw interval exceeds the frame interval |
| `Sub Frame Enabled`       | bool     | Whether to enable sub-frame interpolation                            |
