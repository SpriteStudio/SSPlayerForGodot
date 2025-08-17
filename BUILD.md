# ソース取得

本リポジトリをクローンして、 `SSPlayerForGodot` ディレクトリ内に入ってください。

```bash
git clone --recursive https://github.com/SpriteStudio/SSPlayerForGodot.git
cd SSPlayerForGodot
```

SSPlayerForGodot ディレクトリから以下のコマンドを実行し、 Godot を取得します。

```bash
git clone https://github.com/godotengine/godot.git
```

gdextension をビルドする場合は godot-cpp を取得します。

```bash
git clone https://github.com/godotengine/godot-cpp.git
```

# ブランチ選択
## Godot
SSPlayerForGodot ディレクトリの `godot` ディレクトリ内でビルドする Godot Engine のブランチを選択してください。

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
gdextension をビルドする場合は `godot-cpp` ディレクトリで対照する Godot Engine バージョンと同じブランチ名を選択してください。


```bash
pushd godot-cpp
git checkout 4.4
popd 
```

# ビルド環境のセットアップ

以降でビルド環境の構築手順について説明していきます。  

## Windows

[Godot公式のコンパイル手順](https://docs.godotengine.org/en/stable/contributing/development/compiling/compiling_for_windows.html)

必要なツール
* ビルドツール (いずれかを選択)
    * VisualStudio 2019(推奨) or 2022
    * MSYS2 + MinGW + gcc + make
* Python 3.6 以降
* scons 3.0 以降

VisualStudio 2019 でのビルド・デバッグを確認しています。

scons は下記でインストールできます。(上記リンクにも記載あり)

```bat
python -m pip install scons
```

## macOS

[Godot公式のコンパイル手順](https://docs.godotengine.org/ja/4.x/contributing/development/compiling/compiling_for_macos.html)

必要なツール
* Xcode
* Python 3.6 以降
* scons 3.0 以降
* Vulkan SDK for MoltenVK (4 対応用)
* emscripten (optional)
* Android SDK / Android NDK (optional)

Xcode 以外は [Homebrew](https://brew.sh/) でインストールができます。

```sh
brew install python3 scons 
```

```sh
brew install molten-vk
```

ホストアーキテクチャとは異なるアーキテクチャの Godot Engine をビルドする場合や Universal Binary 対応 Godot Engine をビルドする場合は `molten-vk` の代わりに Universal Binary 対応している [Vulkan SDK for MoltenVK](https://vulkan.lunarg.com/sdk/home) をインストールしてください。

(Optional) target を Web のビルドをする際は emscripten をインストールしてください。

```sh
brew install emscripten
```

(Optional) target を Android のビルドをする際は Android NDK をインストールしてください。


## Linux
T.B.D

# ビルド
## Windows 
### 4

[build.ps1](./scripts/build.ps1) でビルド可能です。


成果物は `godot\bin` に格納されます。

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

[build-v3.ps1](./scripts/build-v3.ps1) でビルド可能です。
成果物は `godot\bin` に格納されます。

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

## macOS ビルド

### 4

[build.sh](./scripts/build.sh) でビルド可能です。
成果物は `godot/bin` に格納されます。

```sh
./scripts/build.sh
```

引数を指定しない場合はホストマシンのアーキテクチャと同じアーキテクチャ向けにビルドします。
アーキテクチャを明示的に指定する場合は `arch=` に引数を追加してください。

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

`godot/Godot.app` を開いて起動を確認します。

### 3.x

[build-v3.sh](./scripts/build-v3.sh) でビルド可能です。
成果物は `godot/bin` に格納されます。

```sh
./scripts/build-v3.sh
```

引数を指定しない場合はホストマシンのアーキテクチャと同じアーキテクチャ向けにビルドします。
アーキテクチャを明示的に指定する場合は `arch=` に引数を追加してください。

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

`godot/Godot.app` を開いて起動を確認します。


# GDExtension
## Windows 
[build-extension.ps1](./scripts/build-extension.ps1) でビルド可能です。
成果物は `bin` ディレクトリに格納されます。


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

## macOS, linux, iOS, Android, Web ビルド

macOS か Linux で実行してください。

[build-extension.sh](./scripts/build-extension.sh) でビルド可能です。
成果物は `bin` ディレクトリに格納されます。

```sh
./scripts/build-extension.sh
```


# リリースビルド
## 4

各プラットフォームの godot の `editor`, `template_debug`, `template_release` をまとめてビルドスクリプトは下記の通りです。

windows
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
各プラットフォームの gdextension の `editor`, `template_debug`, `template_release` をまとめてビルドスクリプトは下記の通りです。

windows
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