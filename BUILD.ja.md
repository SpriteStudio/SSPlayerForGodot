[**日本語**](./BUILD.ja.md) | [**English**](./BUILD.md)

# 概要

本ブランチで Godot 用バイナリを得るまでの流れは以下のとおりです。

1. **`libssruntime` の用意** — SpriteStudio7-SDK のリリース成果物を取得し、`ss_player/runtime/` 配下に配置します。
2. 利用形態に応じて **2-A. GDExtension のビルド** または **2-B. カスタムモジュール組み込み Godot Engine のビルド** を実行し、上記ランタイムをリンクして Godot 用バイナリを生成します。

# ソース取得

本リポジトリをサブモジュールごと取得し、ビルド対象に応じて Godot Engine / godot-cpp を取得します。

```bash
git clone --recursive https://github.com/SpriteStudio/SSPlayerForGodot.git
cd SSPlayerForGodot
git clone https://github.com/godotengine/godot.git -b 4.6
git clone https://github.com/godotengine/godot-cpp.git -b 4.5
```

`godot` ディレクトリはカスタムモジュール組み込み Godot Engine をビルドする場合に必要です。
`godot-cpp` ディレクトリは GDExtension をビルドする場合に必要です。

# ビルド環境のセットアップ

各プラットフォーム向けのビルドツール (コンパイラ・Python・SCons など) の準備は、Godot 公式のコンパイル手順を参照してください。

- [Windows](https://docs.godotengine.org/en/stable/engine_details/development/compiling/compiling_for_windows.html)
- [macOS](https://docs.godotengine.org/ja/stable/engine_details/development/compiling/compiling_for_macos.html)
- Linux: T.B.D.

## 注意点

### macOS で Universal Binary をビルドする場合

Homebrew で配布されている `molten-vk` はホストアーキ向けのバイナリのみ提供されるため、`arch=universal` 指定で Universal Binary をビルドする際はリンクに失敗します。代わりに [Vulkan SDK for MoltenVK](https://vulkan.lunarg.com/sdk/home) (Universal 対応版) をインストールしてください。

# 1. libssruntime の用意

`ss_player/SDK_VERSION.txt` で指定されたバージョンの SDK パッケージを取得・展開します。

**macOS / Linux**

```sh
./scripts/download-sdk.sh
```

**Windows (PowerShell)**

```powershell
.\scripts\download-sdk.ps1
```

> `libssconverter` (`.sspj` → `.ssab` 変換ライブラリ) はデスクトップ向けにのみ同梱されます。iOS / Android / Web 向けの `libssruntime` パッケージには含まれません。

`libssruntime` を SS7-SDK ソースから自前でビルドしたい場合は [SS7-SDK 開発者向け](#ss7-sdk-開発者向け) を参照してください。

# 2-A. GDExtension のビルド

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

# 2-B. カスタムモジュール組み込み Godot Engine のビルド

`godot` を `4.6` ブランチで clone 済みであることが前提です。
`build.sh` / `build.ps1` は `custom_modules=../ss_player` を指定して `scons` を実行します。

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

# リリースビルド

各プラットフォーム向けに `editor` / `template_debug` / `template_release` を一括でビルドするスクリプトを `scripts/` 配下に用意しています。
内部では前述の `build.sh` / `build-extension.sh` を `target` を変えて連続実行する構成です。
これらのスクリプトは `libssruntime` を取得・ビルドはしないため、事前に [1. libssruntime の用意](#1-libssruntime-の用意) を済ませておく必要があります。

## GDExtension

| プラットフォーム | スクリプト                                   | 補足                                         |
| ---------------- | -------------------------------------------- | -------------------------------------------- |
| Windows          | `.\scripts\release-gdextension-windows.ps1`  | `arch` はホスト                              |
| macOS            | `./scripts/release-gdextension-macos.sh`     | `arch=universal` 固定                        |
| Linux            | `./scripts/release-gdextension-linux.sh`     | `arch` はホスト                              |
| iOS              | `./scripts/release-gdextension-ios.sh`       | `template_debug` / `template_release` のみ   |
| Android          | `./scripts/release-gdextension-android.sh`   | `arm32` / `arm64` / `x86_64` の3アーキ       |
| Web              | `./scripts/release-gdextension-web.sh`       | `wasm32` (`threads=yes` / `threads=no`)      |

## カスタムモジュール組み込み Godot Engine

| プラットフォーム | スクリプト                            | 補足                                          |
| ---------------- | ------------------------------------- | --------------------------------------------- |
| Windows          | `.\scripts\release-windows.ps1`       | `arch` はホスト                               |
| macOS            | `./scripts/release-macos.sh`          | `arch=universal` 固定                         |
| iOS              | `./scripts/release-ios.sh`            | `arch=arm64` (実機) と `arch=universal` (sim) |
| Android          | `./scripts/release-android.sh`        | `arm32` / `arm64` / `x86_64` の3アーキ        |

> Linux 向けのカスタムモジュール用一括ビルドスクリプトは未整備です。`./scripts/build.sh platform=linux target=...` を `editor` / `template_debug` / `template_release` で個別に呼び出してください。

# SS7-SDK 開発者向け

> 以降のセクションは **SS7-SDK 自体を手元で開発・カスタマイズしながら Godot 側もビルドしたい場合のみ** 必要です。SS7-SDK のリリース成果物を使う一般的な Godot ビルダーは読み飛ばして構いません。

## 追加で必要なもの

`libssruntime` を自前でビルドするための環境 (Rust ツールチェーン等) のセットアップ手順は [SpriteStudio7-SDK の README](https://github.com/SpriteStudio/SpriteStudio7-SDK?tab=readme-ov-file#for-sdk-developers) を参照してください。

FlatBuffers のヘッダを再生成する場合は別途 `flatc` (FlatBuffers コンパイラ) も必要です。

## libssruntime を自前でビルドする

[ソース取得](#ソース取得) の段階で SS7-SDK サブモジュール (`ss_player/SpriteStudio7-SDK/`) が初期化済みであることが前提です。
以下を実行すると Rust ランタイム/コンバータがビルドされ、`ss_player/runtime/` 配下に成果物が自動配置されます。

**macOS / Linux**

```sh
./scripts/build-runtime.sh
```

**Windows (PowerShell)**

```powershell
.\scripts\build-runtime.ps1
```

## FlatBuffers ヘッダの再生成

SS7-SDK の `.fbs` を変更した場合は、以下で `ss_player/format/` 配下のヘッダを再生成します (`flatc` が必要)。

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

- `ss_player/SpriteStudio7-SDK/libs/ssruntime/docs/README.ja.md`
