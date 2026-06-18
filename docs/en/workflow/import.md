# CLI Conversion and Automation

For the intuitive import method using the Godot editor's "SS Import Dock", please refer to [Asset Import and Editor Integration](usage_asset_pipeline.md).

This document explains how to perform conversion from the command line without launching the Godot editor.
This is highly useful when you want to integrate the conversion process into your CI/CD (Continuous Integration/Continuous Deployment) pipeline, build systems, or when you need to batch convert a large number of assets.

## Using SpriteStudio-SDK's `ssconverter-cli`

The conversion from a SpriteStudio project (`.sspj`) to Godot animation binaries (`.ssab` / `.ssqb`) is handled by a standalone command-line tool called `ssconverter-cli`, which is included in the SDK (the built-in editor importer also calls this tool under the hood).

> [!NOTE]
> The `ssconverter-cli` binary is **only distributed with the SpriteStudio-SDK releases**. It is **not** bundled with the SSPlayerForGodot GDExtension package (`addons/spritestudio/`), so download it separately from the SDK side when you need the CLI.

### 1. Obtaining the Tool

Download the `ssconverter-cli` binary for your platform (Windows / macOS / Linux) from the [SpriteStudio-SDK Releases](https://github.com/cri-middleware/SpriteStudio-SDK/releases).

### 2. Running from the Command Line

Open your terminal or command prompt, and execute the tool by passing the path to your `.sspj` file as an argument.

**Windows:**
```powershell
.\ssconverter-cli.exe path\to\your.sspj
```

**macOS / Linux:**
```bash
./ssconverter-cli path/to/your.sspj
```

### 3. Placing the Artifacts

The conversion results will be output into a new folder named `<sspj_name>_ssab/` created in the same directory as the original `.sspj`. Inside, you will find the `.ssab` (animation packs) and `.ssqb` (sequences) files.

By copying these generated files into any directory under your Godot project's `res://`, they will be recognized and loadable as `SSABResource` / `SSQBResource` in Godot.

---

## Advanced Options

`ssconverter-cli` provides various options to control the conversion behavior.
For a detailed list of options and specifications, please refer to [cli/README.md](https://github.com/cri-middleware/SpriteStudio-SDK/blob/main/cli/README.md) in the SDK repository.
