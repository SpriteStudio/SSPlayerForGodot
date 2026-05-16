# SpriteStudioPlayer for Godot

This `develop` branch is a work-in-progress version.  
The stable version can be obtained from the [main branch](https://github.com/SpriteStudio/SSPlayerForGodot/tree/main) or from [Releases](https://github.com/SpriteStudio/SSPlayerForGodot/releases).  
No warranty or support is provided for this branch, and we cannot respond to feature requests or bug reports.  
Interfaces may change without notice. A migration guide for v1.x users will be provided as a separate document.  

A plugin for playing back animations created with [OPTPiX SpriteStudio](https://www.webtech.co.jp/spritestudio/) inside [Godot Engine](https://godotengine.org/).
Animation playback uses `libssruntime` provided by [SpriteStudio7-SDK](https://github.com/SpriteStudio/SpriteStudio7-SDK).

## Key Features

This plugin is designed to bring the full expressive power of SpriteStudio 7 to Godot Engine seamlessly.

*   **Full Feature Support:** Fully supports SpriteStudio 7 features including bone hierarchies, mesh & deformations, and high-performance particle effects.
*   **Seamless Integration:** The built-in "SS Import Dock" allows you to convert and import assets with a simple drag-and-drop right inside the Godot Editor.
*   **Dynamic Customization (CellMap Overrides):** Easily swap textures at runtime to implement character equipment changes or color variations.
*   **Signals & Events:** Receive "User Data" and "Signals" from your animation timeline directly as Godot Signals, allowing frame-perfect triggers for audio or logic.
*   **Smooth Slow-Motion:** Built-in sub-frame interpolation ensures buttery-smooth playback even on high-refresh-rate displays or during slow-motion effects.
*   **High Performance & Low Memory:** Backed by `libssruntime`'s SIMD optimizations and zero-parse overhead binary formats (`.ssab`), it renders massive amounts of characters efficiently, even on mobile targets.

## Overview

The diagram below shows how data flows through the plugin from authored assets to in-game playback.

```mermaid
graph LR
    subgraph Assets ["Project assets"]
        SS[" .sspj / .ssae / .ssce "]
        IMG[" Image files / .png "]
    end

    subgraph Convert ["Convert (choose one)"]
        DOCK[[" SS Import Dock<br>(built into Godot Editor) "]]
        CLI[[" ssconverter-cli<br>(SS7-SDK Releases) "]]
        SS --> DOCK
        SS --> CLI
        DOCK --> BIN[" .ssab / .ssqb "]
        CLI --> BIN
    end

    subgraph Runtime ["Godot runtime (playback)"]
        BIN --> RES(" SSABResource ")
        RES --> NODE(" SpriteStudioPlayer2D ")
        NODE -.-> RT(" libssruntime ")
        IMG --> NODE
        NODE --> RENDER[[" Godot rendering "]]
    end

    classDef generated stroke-dasharray: 5 5;
    class BIN generated;
```

## Supported Versions

- **Godot Engine**: [4.6 branch](https://github.com/godotengine/godot/tree/4.6)
- **godot-cpp**: [4.5 branch](https://github.com/godotengine/godot-cpp/tree/4.5)

Build and execution have been verified on Windows / macOS.

## Samples

Sample projects based on SDK test projects are available under the `examples/` folder in the repository.

- `allAttributeV7` — Functional test for all attributes
- `allPartsV7` — Functional test for all part types
- `overall` — Comprehensive functional test
- `overall_gdextension` — Comprehensive test for GDExtension
- `ParticleEffect` — Test for effect features
- `dev_module` — Development project for Module version
- `dev_gdextension` — Development project for GDExtension version

## Related Repositories

- [SpriteStudio7-SDK](https://github.com/SpriteStudio/SpriteStudio7-SDK) — The SDK itself, providing `libssruntime` / `libssconverter`

## License

See [LICENSE.txt](../../LICENSE.txt).
