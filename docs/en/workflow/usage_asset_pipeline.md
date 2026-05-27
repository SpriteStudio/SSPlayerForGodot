# Editor Integration and Asset Iteration

SpriteStudioPlayerForGodot does not just play animations; it provides a **powerful asset pipeline for seamless transitions between SpriteStudio and Godot**.
This allows you to iterate at lightning speed, from tweaking animations to seeing them reflected in-game.

---

## 1. Initial Import (Converting via SS Import Dock)

If there is no `.ssab` in your Godot project yet, you first need to import (convert) the SpriteStudio project file (`.sspj`).

1. Open the **"SS Import" dock** on the right side of the Godot editor.
2. Check the output directory (default is `res://ssab_generated`).
3. **Drag and drop the `.sspj` file** from your Explorer/Finder into the "SS Import" dock.

This triggers the conversion process, generating `.ssab` and `.ssqb` files in the specified output directory.

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

This action automatically opens the underlying `.sspj` file in SpriteStudio via OS file association, allowing you to start editing immediately.

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
> **1. Must be imported via SS Import Dock**
> This feature works because the SS Import Dock records the "absolute local path to the original `.sspj` file" during import. Therefore, simply copying a `.ssab` file manually into `res://` using your file explorer will not enable this feature.
>
> **2. Does not work on a different PC after `git clone`**
> Because it relies on a local absolute path, if you clone the project to another PC via Git, the paths will no longer match, and these buttons will not function.
>
> **3. SpriteStudio installation is required**
> Clicking the "Open SSPJ" button will launch SpriteStudio. Therefore, [OPTPiX SpriteStudio 7](https://www.webtech.co.jp/spritestudio/) must be installed on the PC you are using.

Due to these limitations, if you need to update or share `.ssab` files within a team development environment or CI/CD pipeline, bulk conversion using the CLI tool is more appropriate. For details, please refer to [CLI Conversion and Automation](import.md).
