# Asset Import and Editor Integration

This page explains **how to bring your `.sspj` into a Godot project (the initial conversion)** and the workflows for iterating quickly between SpriteStudio and Godot afterwards.

SpriteStudioPlayerForGodot does not just play animations; it provides a **powerful asset pipeline for seamless transitions between SpriteStudio and Godot**, allowing you to iterate quickly from tweaking animations to seeing them reflected in-game.

---

## 1. Initial Import (Converting via SS Import Dock)

If there is no `.ssab` in your Godot project yet, you first need to import (convert) the SpriteStudio project file (`.sspj`).

1. Open the **"SS Import" dock** on the right side of the Godot editor.
2. Check the output directory (default is `res://ssab_generated`).
3. **Drag and drop the `.sspj` file** from your Explorer/Finder into the "SS Import" dock.

This triggers the conversion process, generating `.ssab` and `.ssqb` files in the specified output directory.
Drag and drop the generated `.ssab` file into the 2D viewport to place a `SpriteStudioPlayer2D` node in advance (see [Basic Usage](usage_basic.md) for details).

> [!TIP]
> <video autoplay loop muted playsinline width="100%">
>   <source src="../../assets/sspj_import.webm" type="video/webm">
> </video>
> *(※ Video showing the drag-and-drop import of an .sspj file into the SS Import dock will be placed here)*

---

## 2. Opening SpriteStudio Directly from Godot

When working in Godot and feeling the need to tweak an animation, you don't have to launch SpriteStudio separately and search for the project.

1. Select the **`SpriteStudioPlayer2D` node** playing the animation in the Godot editor.
2. Click to expand the **`SSAB Resource` property** in the Inspector.
3. Click the **"Open SSPJ" button** located next to the resource preview.

This action automatically launches SpriteStudio and opens the underlying `.sspj` file, allowing you to start editing immediately.

> [!NOTE]
> ![Opening SSPJ from the Inspector](../../assets/open_sspj_from_inspector.png)
> *(※ Image highlighting the "Open SSPJ" button in the Inspector will be placed here)*

---

## 3. Powerful Asset Pipeline (Seamless Editor Integration)

After modifying and saving the animation in SpriteStudio, return to the Godot editor.
With just the following steps, your changes will instantly reflect in the game (and in the editor preview).

1. As before, expand the **`SSAB Resource`** in the Inspector.
2. Click the **"Reconvert" button** next to the "Open SSPJ" button.

> [!IMPORTANT]
> **The shortest workflow: "Open" -> "Save" -> "Reconvert"**
> You can call SpriteStudio directly from the Inspector with the node selected, and immediately reconvert in Godot after editing. **This powerful asset pipeline that allows seamless transition between SpriteStudio and Godot** drastically lowers the cost of trial and error during animation adjustments.

> [!TIP]
> <video autoplay loop muted playsinline width="100%">
>   <source src="../../assets/fast_iteration_inspector.webm" type="video/webm">
> </video>
> *(※A video demonstrating the fast workflow: Open from Inspector -> Save in SpriteStudio -> Reconvert in Inspector -> Preview updates instantly)*

---

## Limitations and Team Development Notes

While this powerful cross-editor integration (`Open SSPJ` and `Reconvert`) is incredibly convenient, it has some limitations due to how it works. Please keep these in mind, especially when working in a team.

> [!WARNING]
> **SpriteStudio installation is required**
> Clicking the "Open SSPJ" button will launch SpriteStudio. Therefore, [OPTPiX SpriteStudio 7](https://www.webtech.co.jp/spritestudio/) must be installed on the PC you are using.

> [!TIP]
> **File Path Sharing and Smart Re-link for Team Development**
> 
> When you import using the SS Import Dock, the integration information (the file path to the `.sspj`) is saved as a **relative path** inside the `.ssplayer_sources.cfg` file located at the root of your Godot project.
> 
> By tracking `.ssplayer_sources.cfg` in a version control system like Git, team members can `git clone` the project and immediately use the `Open SSPJ` and `Reconvert` buttons, as the paths remain valid across different PCs.
> 
> Furthermore, if the `.sspj` file is moved and the link breaks, it is easy to fix. Right-click the broken `.ssab` in the FileSystem dock and select "Reconvert". You will be prompted to locate the new `.sspj` location. By re-linking just one file, all other related files in the same directory will be **automatically and smartly re-linked**, minimizing manual repair efforts.
