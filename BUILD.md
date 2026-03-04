[**日本語**](./BUILD.ja.md) | [**English**](./BUILD.md)

# Source Retrieval

Clone this repository and place it inside a directory named `SSPlayerForGodot`.

```bash
git clone --recursive https://github.com/SpriteStudio/SSPlayerForGodot.git
cd SSPlayerForGodot
```

From the SSPlayerForGodot directory, run the following command to obtain Godot:

```bash
git clone https://github.com/godotengine/godot.git
```

If you will build the GDExtension, retrieve `godot-cpp` as well:

```bash
git clone https://github.com/godotengine/godot-cpp.git
```

# Selecting Branches

## Godot

In the `godot` directory under SSPlayerForGodot, select the branch of the Godot Engine you want to build.

### 4

```bash
pushd godot
git checkout 4.4
popd
```

### 3.x

```bash
pushd godot
git checkout 3.x
popd
```

## GDExtension

When building a GDExtension, select the same branch name in the `godot-cpp` directory that matches the Godot Engine version.

```bash
pushd godot-cpp
git checkout 4.4
popd
```

# Setting Up the Build Environment

The following describes the setup steps for each platform.

## Windows

[Godot official compilation instructions](https://docs.godotengine.org/en/stable/contributing/development/compiling/compiling_for_windows.html)

Required tools:

* Build tools (choose one):
  * Visual Studio 2019 (recommended) or Visual Studio 2022  
  * MSYS2 + MinGW + gcc + make  
* Python 3.6 or later  
* scons 3.0 or later  

Build/debug has been verified with Visual Studio 2019.

You can install scons using the following command (also listed in the official documentation):

```bat
python -m pip install scons
```

## macOS

[Godot official compilation instructions](https://docs.godotengine.org/ja/4.x/contributing/development/compiling/compiling_for_macos.html)

Required tools:

* Xcode  
* Python 3.6 or later  
* scons 3.0 or later  
* Vulkan SDK for MoltenVK (required for Godot 4)  
* emscripten (optional)  
* Android SDK / Android NDK (optional)  

Except for Xcode, you can install the necessary tools with [Homebrew](https://brew.sh/):

```sh
brew install python3 scons
```

```sh
brew install molten-vk
```

When building Godot Engine for an architecture different from the host, or for Universal Binary support, install the Universal Binary–compatible version of the [Vulkan SDK for MoltenVK](https://vulkan.lunarg.com/sdk/home) instead of `molten-vk` from Homebrew.

(Optional) When building for Web targets, install emscripten:

```sh
brew install emscripten
```

(Optional) When building for Android targets, install the Android NDK.

## Linux

T.B.D.

# Build

## Windows

### 4

You can build using [build.ps1](./scripts/build.ps1).  
Outputs are stored in `godot\bin`.

**PowerShell**

```powershell
$env:PYTHONUTF8=1
.\scripts\build.ps1
```

**Cmd**

```cmd
set PYTHONUTF8=1
PowerShell.exe -ExecutionPolicy Bypass -File .\scripts\build.ps1
```

### 3.x

You can build using [build-v3.ps1](./scripts/build-v3.ps1).  
Outputs are stored in `godot\bin`.

**PowerShell**

```powershell
$env:PYTHONUTF8=1
.\scripts\build-v3.ps1
```

**Cmd**

```cmd
set PYTHONUTF8=1
PowerShell.exe -ExecutionPolicy Bypass -File .\scripts\build-v3.ps1
```

## macOS Build

### 4

You can build using [build.sh](./scripts/build.sh).  
Outputs are stored in `godot/bin`.

```sh
./scripts/build.sh
```

If no architecture argument is provided, the build will target the same architecture as the host machine.  
To explicitly specify an architecture, add an argument to `arch=`.

**Universal Binary (supports both arm64 and x86_64)**

```sh
./scripts/build.sh arch=universal
```

**arm64 (Apple Silicon)**

```sh
./scripts/build.sh arch=arm64
```

**x86_64 (Intel)**

```sh
./scripts/build.sh arch=x86_64
```

Open `godot/Godot.app` to verify the build runs.

### 3.x

You can build using [build-v3.sh](./scripts/build-v3.sh).  
Outputs are stored in `godot/bin`.

```sh
./scripts/build-v3.sh
```

If no architecture argument is provided, the build will target the same architecture as the host machine.  
To explicitly specify an architecture, add an argument to `arch=`.

**Universal Binary (supports both arm64 and x86_64)**

```sh
./scripts/build-v3.sh arch=universal
```

**arm64 (Apple Silicon)**

```sh
./scripts/build-v3.sh arch=arm64
```

**x86_64 (Intel)**

```sh
./scripts/build-v3.sh arch=x86_64
```

Open `godot/Godot.app` to verify it launches properly.

# GDExtension

## Windows

You can build using [build-extension.ps1](./scripts/build-extension.ps1).  
Outputs are placed in the `bin` directory.

**PowerShell**

```powershell
$env:PYTHONUTF8=1
.\scripts\build-extension.ps1
```

**Cmd**

```cmd
set PYTHONUTF8=1
PowerShell.exe -ExecutionPolicy Bypass -File .\scripts\build-extension.ps1
```

## macOS, Linux, iOS, Android, Web Builds

Run on macOS or Linux.   
You can build using [build-extension.sh](./scripts/build-extension.sh).  
Outputs are stored in the `bin` directory.

```sh
./scripts/build-extension.sh
```

# Release Build

## 4

Below are the scripts that build all Godot platform binaries (`editor`, `template_debug`, `template_release`).

Windows

```
.\scripts\release-windows.ps1
```

macOS

```sh
./scripts/release-macos.sh
```

Linux

```sh
./scripts/release-linux.sh
```

iOS

```sh
./scripts/release-ios.sh
```

Android

```sh
./scripts/release-android.sh
```

Web

```sh
./scripts/release-web.sh
```

## GDExtension

Below are the scripts that build all GDExtension platform binaries (`editor`, `template_debug`, `template_release`).

Windows

```
.\scripts\release-gdextension-windows.ps1
```

macOS

```sh
./scripts/release-gdextension-macos.sh
```

Linux

```sh
./scripts/release-gdextension-linux.sh
```

iOS

```sh
./scripts/release-gdextension-ios.sh
```

Android

```sh
./scripts/release-gdextension-android.sh
```

Web

```sh
./scripts/release-gdextension-web.sh
```

## 3.x

...