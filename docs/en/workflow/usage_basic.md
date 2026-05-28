# Basic Usage

This page explains the basic workflow for placing and playing SpriteStudio animations in your Godot scene.

> [!NOTE]
> This page assumes that **an `.ssab` file has already been imported into your Godot project**. If you do not have an `.ssab` yet, first see [Asset Import and Editor Integration](usage_asset_pipeline.md) (editor-based import) or [CLI Conversion and Automation](import.md) (CLI-based import).

## Fastest Setup: Auto Node Creation via Drag & Drop

Taking advantage of Godot's powerful editor, you can set up an animation with the shortest possible workflow.

1. Find the imported **`.ssab` file** in the **FileSystem dock**.
2. **Drag and drop** the file into the **2D workspace (Scene view)**.

This action automatically creates a `SpriteStudioPlayer2D` node and assigns the resource to it.

> [!TIP]
> <video autoplay loop muted playsinline width="100%">
>   <source src="../../assets/setup_drag_and_drop.webm" type="video/webm">
> </video>
> *(※ Short video showing node creation by dragging and dropping an .ssab file will be placed here)*

---

## Manual Node Addition and Attachment

If you want to place the node in a specific location within an existing node hierarchy, you can add it manually and assign the resource.

1. Add a `SpriteStudioPlayer2D` node to your scene tree using the "+" button in the Scene dock.
2. Select the added node and open the Inspector.
3. Drag and drop the `.ssab` file from the FileSystem dock into the **`SSAB Resource`** property box in the Inspector to attach it.

---

## Inspector Settings and Preview

Once the node is selected, you can adjust various settings from Godot's Inspector.

1. **Select an `Animation`**
   Open the dropdown for the `Animation` property in the Inspector. You will see a list of animations contained in the `.ssab`. Select the name of the animation you want to play.

2. **In-Editor Preview**
   Checking the **`Playing`** property in the Inspector allows the animation to **play directly in the editor without running the game**.
   Changes to parameters like `Frame`, `Speed`, and `Loop` are reflected in the preview in real-time, enabling quick adjustments.

> [!TIP]
> <video autoplay loop muted playsinline width="100%">
>   <source src="../../assets/inspector_preview.webm" type="video/webm">
> </video>
> *(※ Video showing the animation playing in the editor after checking the Playing property will be placed here)*

---

## Main Inspector Properties

| Property                   | Type     | Description                                                         |
| -------------------------- | -------- | ------------------------------------------------------------------- |
| `SSAB Resource`            | Resource | The target `SSABResource` (`.ssab` file) to play                    |
| `Animation`                | String   | The name of the currently selected animation                        |
| `Frame`                    | float    | The current frame position                                          |
| `Speed`                    | float    | Playback speed multiplier (Default: 1.0)                            |
| `Frame Rate`               | int      | FPS                                                                 |
| `Loop`                     | int      | Number of loops (`-1` for infinite loop)                            |
| `Playing`                  | bool     | Playback flag                                                       |
| `Animation Section Start`  | int      | Start frame for partial playback                                    |
| `Animation Section End`    | int      | End frame for partial playback                                      |
| `Playback Direction`       | int      | Playback direction                                                  |
| `Playback Style`           | int      | Playback style (e.g., One-way, Ping-pong)                           |
| `Skip Frames`              | bool     | Whether to skip frames when the draw interval exceeds the frame interval |
| `Sub Frame Enabled`        | bool     | Whether to enable sub-frame interpolation                           |
