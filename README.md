[**日本語**](./README.ja.md) | [**English**](./README.md)

# SpriteStudioPlayer for Godot

This `develop` branch is a work-in-progress version.
No warranty or support is provided for this branch, and we cannot respond to feature requests or bug reports.
Interfaces may change without notice. A migration guide for v1.x users will be provided as a separate document.

A plugin for playing back animations created with [OPTPiX SpriteStudio](https://www.webtech.co.jp/spritestudio/) inside [Godot Engine](https://godotengine.org/).
Animation playback uses `libssruntime` provided by [SpriteStudio7-SDK](https://github.com/SpriteStudio/SpriteStudio7-SDK).

## Overview

The diagram below shows how data flows through the plugin from authored assets to in-game playback.

> **Diagram legend**
> - **Shapes**: `[ ]` data/files, `( )` libraries/components, `[[ ]]` tools/applications
> - **Arrows**: `-->` data flow, `-.->` dependency/reference
> - **Borders**: dashed borders indicate generated files.

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

There are two ways to convert `.sspj` to `.ssab` / `.ssqb`. Files produced by either method are loaded as `SSABResource` / `SSQBResource` in Godot. See [USAGE.md](./USAGE.md) for details.

- Nodes
    - `SpriteStudioPlayer2D`: A `Node2D`-based node for playing back SS animations.
- Resources
    - `SSABResource`: Resource representing a converted animation binary (`.ssab`).
    - `SSQBResource`: Resource representing a converted sequence binary (`.ssqb`).
- Editor extension
    - `SS Import Dock`: An import control that converts `.sspj` to `.ssab` / `.ssqb` via `libssconverter`.

## Supported Versions

- **Godot Engine**: [4.6 branch](https://github.com/godotengine/godot/tree/4.6)
- **godot-cpp**: [4.5 branch](https://github.com/godotengine/godot-cpp/tree/4.5)

Build and execution have been verified on Windows / macOS.

## Quick Start

The shortest path: download a prebuilt GDExtension and use it without any local build.

### 1. Install Godot Engine

Download a 4.6-series editor from the [official site](https://godotengine.org/download/).

### 2. Get the SSPlayerForGodot GDExtension

Download the GDExtension package for your target platform from the [SSPlayerForGodot Releases page](https://github.com/SpriteStudio/SSPlayerForGodot/releases).

### 3. Drop it into your project

Copy the downloaded GDExtension files (the `addons` folder inside the ZIP) directly into your Godot project's root directory.
When placed correctly, `res://addons/spritestudio/spritestudio.gdextension` should exist.
After restarting the Godot editor, the `SpriteStudioPlayer2D` node and the SS Import Dock become available.

### 4. Convert and play SpriteStudio data

For converting `.sspj` and playing it back via `SpriteStudioPlayer2D`, see [USAGE.md](./USAGE.md#importing-spritestudio-data).

## Samples

Sample projects based on SDK test projects are available under the [examples folder](./examples/).

- [allAttributeV7](./examples/allAttributeV7) — Functional test for all attributes
- [allPartsV7](./examples/allPartsV7) — Functional test for all part types
- [overall](./examples/overall) — Comprehensive functional test
- [overall_gdextension](./examples/overall_gdextension) — Comprehensive test for GDExtension
- [ParticleEffect](./examples/ParticleEffect) — Test for effect features
- [dev_module](./examples/dev_module) — Development project for Module version
- [dev_gdextension](./examples/dev_gdextension) — Development project for GDExtension version

## Building / Developing

If you want to build the GDExtension or a custom-module Godot Engine yourself, or to develop the plugin in parallel with SS7-SDK, see [BUILD.md](./BUILD.md).

## Related Repositories

- [SpriteStudio7-SDK](https://github.com/SpriteStudio/SpriteStudio7-SDK) — The SDK itself, providing `libssruntime` / `libssconverter`

## License

See [LICENSE.txt](./LICENSE.txt).
