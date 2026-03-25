[**日本語**](./README.ja.md) | [**English**](./README.md)

# SpriteStudioPlayer for Godot

## Introduction

This plugin enables playback of animations created with  
[OPTPiX SpriteStudio](https://www.webtech.co.jp/spritestudio/index.html)  
within the [Godot Engine](https://godotengine.org/).

To prioritize runtime performance, the plugin is implemented as a C++ module.

To use SpriteStudioPlayer for Godot, you must either:

- Build the Godot Engine editor with the SpriteStudioPlayer [custom module](https://docs.godotengine.org/en/stable/contributing/development/core_and_modules/custom_modules_in_cpp.html), **or**
- Add the SpriteStudioPlayer [GDExtension](https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/what_is_gdextension.html) to your project.

## Supported Versions of OPTPiX SpriteStudio

Supports SpriteStudio **Ver.6** and **Ver.7**.  
*Note: New features added in Ver. 7.1 (text, sound, etc.) are not supported.*

## Supported Versions of Godot Engine

- [4.5 branch](https://github.com/godotengine/godot/tree/4.5):  
  Verified build and execution on Windows / macOS  
- [3.x branch](https://github.com/godotengine/godot/tree/3.x):  
  Verified build and execution on Windows / macOS

## Build

See **[BUILD.md](BUILD.md)**.

## Usage

See **[USAGE.md](USAGE.md)**.

## Samples

Sample projects are available in the **[examples folder](./examples/)**.

**Note:**  
These samples were created for **Godot 3.x**.  
For use with **Godot 4.4**, please run **“Convert Full Project”** first.

### [feature_test](./examples/feature_test)

Basic functional test project with the following scenes:

- **v6_feature.tscn**  
  - Verify features supported from SpriteStudio v6.0 to v7.0  
  - Enable visibility of nodes you want to inspect  
  - `Signal` & `UserData` trigger Godot signals when reaching their keys  
  - Parameters appear in the console  
  - Original project (`v6_all.sspj`) is located under `ssdata/`

- **sspj_load.tscn**  
  - Loads SS projects via GDScript and plays selected animations  
  - Automatically switches to another SS project when playback finishes

- **texture_change.tscn**  
  - Demonstrates dynamic texture replacement  
  - Toggle “Change” in Inspector to switch textures

### [mesh_bone](./examples/mesh_bone)

Sample using mesh, bone, and effect features for character animation.

### [particle_effect](./examples/particle_effect)

Sample demonstrating effect animation features.  
Displays one of 40 possible animations.  
More animations can be tested through the Anime Pack.

### [feature_test_gdextension](./examples/feature_test_gdextension)

Test project for GDExtension operation.  
Provides the same samples as `feature_test`.

## Contact

For questions, feature requests, or bug reports, please post to **[Issues](../../issues)**.

For private inquiries, contact us via the  
[Help Center](https://www.webtech.co.jp/help/ja/spritestudio7/inquiries/ssplayer_tool/).

=========================================================  

CRI Middleware Co., Ltd.  
https://www.cri-mw.co.jp/  
Copyright © CRI Middleware Co., Ltd.  

=========================================================  

* SpriteStudio and Web Technology are registered trademarks of Web Technology Corp.
* Other product names are registered trademarks or trademarks of their respective companies.
