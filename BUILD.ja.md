[**日本語**](./BUILD.ja.md) | [**English**](./BUILD.md)

# 概要

本ブランチで Godot 用バイナリを得るまでの流れは以下の2段階です。

1. **`libssruntime` の用意** — SpriteStudio7-SDK のリリース成果物を取得し、`gd_spritestudio/runtime/` 配下に配置します。
2. **GDExtension** または **カスタムモジュール組み込み Godot Engine** のビルド — 上記ランタイムをリンクして Godot 用バイナリを生成します。

# ソース取得

本リポジトリをサブモジュールごと取得し、ビルド対象に応じて Godot Engine / godot-cpp を取得します。

```bash
git clone --recursive https://github.com/SpriteStudio/SSPlayerForGodot.git
cd SSPlayerForGodot
git clone https://github.com/godotengine/godot.git -b 4.6
git clone https://github.com/godotengine/godot-cpp.git -b 4.5
```

`--recursive` を付け忘れた場合は以下を実行してください。

```bash
git submodule update --init --recursive
```

`godot` ディレクトリはカスタムモジュール組み込み Godot Engine をビルドする場合に必要です。
`godot-cpp` ディレクトリは GDExtension をビルドする場合に必要です。

# ビルド環境のセットアップ

## Windows

[Godot 公式のコンパイル手順](https://docs.godotengine.org/en/stable/contributing/development/compiling/compiling_for_windows.html) に従って下記を用意します。

* ビルドツール (いずれかを選択)
    * Visual Studio 2019 (推奨) または 2022
    * MSYS2 + MinGW + gcc + make
* Python 3.6 以降
* SCons 3.0 以降

SCons は下記でインストールできます。

```bat
python -m pip install scons
```

## macOS

[Godot 公式のコンパイル手順](https://docs.godotengine.org/ja/4.x/contributing/development/compiling/compiling_for_macos.html) に従って下記を用意します。

* Xcode
* Python 3.6 以降
* SCons 3.0 以降
* Vulkan SDK for MoltenVK
* (Optional) emscripten — Web 向けビルド用
* (Optional) Android SDK / Android NDK — Android 向けビルド用

Xcode 以外は [Homebrew](https://brew.sh/) でインストールできます。

```sh
brew install python3 scons
brew install molten-vk
```

ホストアーキテクチャと異なる構成 (Universal Binary など) をビルドする場合は `molten-vk` の代わりに [Vulkan SDK for MoltenVK](https://vulkan.lunarg.com/sdk/home) をインストールしてください。

## Linux

T.B.D.

# 1. libssruntime の用意

[SpriteStudio7-SDK のリリースページ](https://github.com/SpriteStudio/SpriteStudio7-SDK/releases) から該当プラットフォーム向けの SDK バイナリ一式を取得し、リポジトリルートからの相対パスで `gd_spritestudio/runtime/` 配下に展開してください。

期待されるレイアウトは以下のとおりです。

```
gd_spritestudio/runtime/
├── include/
│   ├── ssruntime.h
│   └── ssconverter.h
└── libs/
    ├── macos/        libssruntime.a, libssconverter.a            (universal binary)
    ├── ios/          libssruntime.a, libssconverter.a            (universal binary)
    ├── web/          libssruntime.a, libssconverter.a
    ├── windows/<arch>/    libssruntime.a, libssconverter.a       (例: x86_64)
    ├── linux/<arch>/      libssruntime.a, libssconverter.a       (例: x86_64)
    └── android/<arch>/    libssruntime.a, libssconverter.a       (例: arm64, x86_64)
```

`libssruntime` を SS7-SDK ソースから自前でビルドしたい場合は [SS7-SDK 開発者向け](#ss7-sdk-開発者向け) を参照してください。

# 2. GDExtension のビルド

`godot-cpp` を `4.5` ブランチで clone 済みであることが前提です。

**macOS / Linux**

```sh
./scripts/build-extension.sh
```

**Windows (PowerShell)**

```powershell
$env:PYTHONUTF8=1
.\scripts\build-extension.ps1
```

成果物は `bin/<platform>/` に配置され、`misc/ssplayer_godot_extension.gdextension` が `examples/feature_test_gdextension/bin` にコピーされます。

主なオプション (key=value 形式)。

| キー       | 既定値                          | 説明                                                              |
| ---------- | ------------------------------- | ----------------------------------------------------------------- |
| `arch`     | ホスト                          | 出力アーキテクチャ (`universal` 指定で Android は `arm64+x86_64`) |
| `platform` | ホスト (`win`/`macos`/`linux`)  | ビルド対象プラットフォーム                                        |
| `target`   | `editor`                        | `editor` / `template_debug` / `template_release`                  |
| `cpus`     | 自動検出                        | `scons -j` の並列度                                               |

# 3. カスタムモジュール組み込み Godot Engine のビルド

`godot` を `4.6` ブランチで clone 済みであることが前提です。
`build.sh` / `build.ps1` は `custom_modules=../gd_spritestudio` を指定して `scons` を実行します。

**macOS / Linux**

```sh
./scripts/build.sh
```

**Windows (PowerShell)**

```powershell
$env:PYTHONUTF8=1
.\scripts\build.ps1
```

成果物は `godot/bin/` に出力されます。macOS では `godot/Godot.app` が併せて作成されます。

主なオプション。

| キー       | 既定値    | 説明                                                                       |
| ---------- | --------- | -------------------------------------------------------------------------- |
| `arch`     | ホスト    | `arm64` / `x86_64` / `universal` (macOS のみ lipo で結合)                  |
| `platform` | ホスト    | ビルド対象プラットフォーム                                                 |
| `target`   | `editor`  | `editor` / `template_debug` / `template_release`                           |
| `cpus`     | 自動検出  | `scons -j` の並列度                                                        |
| `ccache`   | `no`      | `yes` を指定すると `ccache` または `sccache` を有効化 (macOS パッチを適用) |
| `version`  | `4.6`     | git ブランチ/タグから検出できない場合のフォールバック                      |
| `strip`    | `no`      | `yes` で生成バイナリに `strip` を実行                                      |

# リリースビルド

各プラットフォーム向けに `editor` / `template_debug` / `template_release` を一括でビルドするスクリプトを `scripts/` 配下に用意しています。
内部では前述の `build.sh` / `build-extension.sh` を `target` を変えて連続実行する構成です。
これらのスクリプトは `libssruntime` を取得・ビルドはしないため、事前に [1. libssruntime の用意](#1-libssruntime-の用意) を済ませておく必要があります。

## カスタムモジュール組み込み Godot Engine

| プラットフォーム | スクリプト                            | 補足                                          |
| ---------------- | ------------------------------------- | --------------------------------------------- |
| Windows          | `.\scripts\release-windows.ps1`       | `arch` はホスト                               |
| macOS            | `./scripts/release-macos.sh`          | `arch=universal` 固定                         |
| iOS              | `./scripts/release-ios.sh`            | `arch=arm64` (実機) と `arch=universal` (sim) |
| Android          | `./scripts/release-android.sh`        | `arm32` / `arm64` / `x86_64` の3アーキ        |

## GDExtension

| プラットフォーム | スクリプト                                   | 補足                                         |
| ---------------- | -------------------------------------------- | -------------------------------------------- |
| Windows          | `.\scripts\release-gdextension-windows.ps1`  | `arch` はホスト                              |
| macOS            | `./scripts/release-gdextension-macos.sh`     | `arch=universal` 固定                        |
| Linux            | `./scripts/release-gdextension-linux.sh`     | `arch` はホスト                              |
| iOS              | `./scripts/release-gdextension-ios.sh`       | `template_debug` / `template_release` のみ   |
| Android          | `./scripts/release-gdextension-android.sh`   | `arm32` / `arm64` / `x86_64` の3アーキ       |
| Web              | `./scripts/release-gdextension-web.sh`       | `wasm32` (`threads=yes` / `threads=no`)      |

# SS7-SDK 開発者向け

> 以降のセクションは **SS7-SDK 自体を手元で開発・カスタマイズしながら Godot 側もビルドしたい場合のみ** 必要です。SS7-SDK のリリース成果物を使う一般的な Godot ビルダーは読み飛ばして構いません。

## 追加で必要なもの

`libssruntime` を自前でビルドするための環境 (Rust ツールチェーン等) のセットアップ手順は [SpriteStudio7-SDK の README](https://github.com/SpriteStudio/SpriteStudio7-SDK?tab=readme-ov-file#for-sdk-developers) を参照してください。

FlatBuffers のヘッダを再生成する場合は別途 `flatc` (FlatBuffers コンパイラ) も必要です。

## libssruntime を自前でビルドする

[ソース取得](#ソース取得) の段階で SS7-SDK サブモジュール (`gd_spritestudio/SpriteStudio7-SDK/`) が初期化済みであることが前提です。
以下を実行すると Rust ランタイム/コンバータがビルドされ、`gd_spritestudio/runtime/` 配下に成果物が自動配置されます。

**macOS / Linux**

```sh
./scripts/build-runtime.sh
```

**Windows (PowerShell)**

```powershell
.\scripts\build-runtime.ps1
```

主なオプション。

| キー       | 既定値  | 説明                                                                |
| ---------- | ------- | ------------------------------------------------------------------- |
| `arch`     | ホスト  | 出力アーキテクチャ (`arm64`, `x86_64`, `universal` など)            |
| `platform` | ホスト  | `windows` / `macos` / `linux` / `ios` / `web`                       |
| `build`    | `debug` | `debug` または `release` (`release` は SDK 側 `release-<platform>.sh` を呼ぶ) |

## FlatBuffers ヘッダの再生成

SS7-SDK の `.fbs` を変更した場合は、以下で `gd_spritestudio/format/` 配下のヘッダを再生成します (`flatc` が必要)。

**macOS / Linux**

```sh
./scripts/generate-runtime-code.sh
```

**Windows (PowerShell)**

```powershell
.\scripts\generate-runtime-code.ps1
```

## SS7-SDK 内部ドキュメント

サブモジュール初期化済みであれば、ランタイムの内部仕様や移植時の注意点は以下を参照できます。

- `gd_spritestudio/SpriteStudio7-SDK/libs/ssruntime/docs/README.ja.md`
