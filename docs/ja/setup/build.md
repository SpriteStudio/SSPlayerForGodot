# ビルド / 開発

GDExtension またはカスタムモジュール組み込み Godot Engine を自分でビルドしたい場合、および SpriteStudio-SDK と並行してプラグインを開発したい場合は、以下の手順に従ってください。

## 概要

本リポジトリで Godot 用バイナリを得るまでの流れは以下のとおりです。

1. **`libssruntime` の用意** — SpriteStudio-SDK のリリース成果物を取得し、`ss_player/runtime/` 配下に配置します。
2. 利用形態に応じて **2-A. GDExtension のビルド** または **2-B. カスタムモジュール組み込み Godot Engine のビルド** を実行し、上記ランタイムをリンクして Godot 用バイナリを生成します。

## ソース取得

本リポジトリを取得し、ビルド対象に応じて Godot Engine / godot-cpp を取得します。

```bash
git clone https://github.com/SpriteStudio/SSPlayerForGodot.git
cd SSPlayerForGodot
git clone https://github.com/godotengine/godot.git -b 4.6
git clone https://github.com/godotengine/godot-cpp.git -b master
```

`godot` ディレクトリはカスタムモジュール組み込み Godot Engine をビルドする場合に必要です。
`godot-cpp` ディレクトリは GDExtension をビルドする場合に必要です。

> [!NOTE]
> SDK のリリース成果物を使う一般的なビルドでは、サブモジュール `ss_player/SpriteStudio-SDK/` の取得は不要です（`--recursive` を付けずにクローンして構いません）。SpriteStudio-SDK 自体を手元で開発・ビルドする場合のみ、後述の [SpriteStudio-SDK 開発者向け](#spritestudio-sdk-開発者向け) に従って `git submodule update --init --recursive` でサブモジュールを初期化してください。

## ビルド環境のセットアップ

各プラットフォーム向けのビルドツール (コンパイラ・Python・SCons など) の準備は、Godot 公式のコンパイル手順を参照してください。

- [Windows](https://docs.godotengine.org/en/stable/engine_details/development/compiling/compiling_for_windows.html)
- [macOS](https://docs.godotengine.org/ja/stable/engine_details/development/compiling/compiling_for_macos.html)
- Linux: T.B.D.

### 注意点

#### macOS で Universal Binary をビルドする場合

Homebrew で配布されている `molten-vk` はホストアーキ向けのバイナリのみ提供されるため、`arch=universal` 指定で Universal Binary をビルドする際はリンクに失敗します。代わりに [Vulkan SDK for MoltenVK](https://vulkan.lunarg.com/sdk/home) (Universal 対応版) をインストールしてください。

## 1. libssruntime の用意

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

`libssruntime` を SpriteStudio-SDK ソースから自前でビルドしたい場合は [SpriteStudio-SDK 開発者向け](#spritestudio-sdk-開発者向け) を参照してください。

## 2-A. GDExtension のビルド

`godot-cpp` を `master` ブランチで clone 済みであることが前提です。

**macOS / Linux**

```sh
./scripts/build-extension.sh
```

**Windows (PowerShell)**

```powershell
$env:PYTHONUTF8=1
.\scripts\build-extension.ps1
```

成果物は `bin/<platform>/` に配置され、`misc/spritestudio.gdextension` を含む GDExtension 一式が各サンプルプロジェクト（`examples/*/addons/spritestudio/`）にインストールされます。

## 2-B. カスタムモジュール組み込み Godot Engine のビルド

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

## リリースビルド

各プラットフォーム向けに `editor` / `template_debug` / `template_release` を一括でビルドするスクリプトを `scripts/` 配下に用意しています。
内部では前述の `build.sh` / `build-extension.sh` を `target` を変えて連続実行する構成です。
これらのスクリプトは `libssruntime` を取得・ビルドはしないため、事前に [1. libssruntime の用意](#1-libssruntime-の用意) を済ませておく必要があります。

### GDExtension

| プラットフォーム | スクリプト                                   | 補足                                         |
| ---------------- | -------------------------------------------- | -------------------------------------------- |
| Windows          | `.\scripts\release-gdextension-windows.ps1`  | `arch` はホスト                              |
| macOS            | `./scripts/release-gdextension-macos.sh`     | `arch=universal` 固定                        |
| Linux            | `./scripts/release-gdextension-linux.sh`     | `arch` はホスト                              |
| iOS              | `./scripts/release-gdextension-ios.sh`       | `template_debug` / `template_release` のみ   |
| Android          | `./scripts/release-gdextension-android.sh`   | `arm32` / `arm64` / `x86_64` の3アーキ       |
| Web              | `./scripts/release-gdextension-web.sh`       | `wasm32` (`threads=yes` / `threads=no`)      |

### カスタムモジュール組み込み Godot Engine

| プラットフォーム | スクリプト                            | 補足                                          |
| ---------------- | ------------------------------------- | --------------------------------------------- |
| Windows          | `.\scripts\release-windows.ps1`       | `arch` はホスト                               |
| macOS            | `./scripts/release-macos.sh`          | `arch=universal` 固定                         |
| iOS              | `./scripts/release-ios.sh`            | `arch=arm64` (実機) と `arch=universal` (sim) |
| Android          | `./scripts/release-android.sh`        | `arm32` / `arm64` / `x86_64` の3アーキ        |

> Linux 向けのカスタムモジュール用一括ビルドスクリプトは未整備です。`./scripts/build.sh platform=linux target=...` を `editor` / `template_debug` / `template_release` で個別に呼び出してください。

## カスタムモジュールのデバッグ方法

カスタムモジュールとして組み込んだプラグインのC++コード（`ss_player` 以下）をデバッグしたい場合は、以下の手順で行います。

1. **デバッグ用バイナリの確認**
   `target=editor` あるいは `target=template_debug` を指定してビルドしたGodotバイナリには、デフォルトでデバッグシンボルが含まれています。
   * **macOS:** `godot/Godot.app/Contents/MacOS/Godot`
   * **Windows:** `godot/bin/godot.windows.editor.x86_64.exe` 等
   * **Linux:** `godot/bin/godot.linuxbsd.editor.x86_64` 等

2. **デバッガのアタッチと起動引数**
   VSCode、Visual Studio、XcodeなどのIDEやコマンドラインのデバッガ（LLDB/GDB）から、上記のバイナリを起動プログラムとして指定します。
   引数として対象プロジェクトへのパス（例: `--path examples/dev_module`）を渡すことで、エディタ画面を挟まずに直接プロジェクトを開いてデバッグを開始できます。

**VSCode (`launch.json`) の設定例 (macOS / LLDB の場合):**

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Godot Custom Module",
            "type": "lldb",
            "request": "launch",
            "program": "${workspaceFolder}/godot/Godot.app/Contents/MacOS/Godot",
            "args": [
                "--path",
                "${workspaceFolder}/examples/dev_module"
            ],
            "cwd": "${workspaceFolder}"
        }
    ]
}
```

## エクスポート機能のテスト・デバッグ方法

開発したカスタムモジュールやGDExtensionが、ビルド後のアプリで正常に動作するか（エクスポートが成功するか）を確認・デバッグするには、CLIを用いたヘッドレスエクスポートが便利です。

以下の流れで、手元でテンプレートをビルド・インストールし、サンプルプロジェクトをエクスポートします。（以下はmacOSの例です）

1. **ランタイムとテンプレートのビルド**
   事前に `libssruntime` を用意し、対象プラットフォームのリリーススクリプトを実行します。
   ```bash
   # (必要に応じて) ランタイムをリリースビルドで用意
   ./scripts/build-runtime.sh build=release platform=macos

   # エクスポート用テンプレートのビルド
   ./scripts/release-macos.sh
   ```

2. **テンプレートのインストール**
   ビルドしたテンプレートを、Godotが認識するローカルの所定ディレクトリへインストールします。
   **macOS / Linux:**
   ```bash
   ./scripts/install-template.sh macos
   ```
   **Windows (PowerShell):**
   ```powershell
   .\scripts\install-template.ps1 windows
   ```

3. **CLIからのエクスポート実行**
   ビルドしたGodotエディタ（headlessモード）を使い、コマンドラインから直接エクスポート処理を呼び出します。
   ```bash
   # 例: dev_module プロジェクトを macOS 向けにエクスポート (.appとして直接出力)
   ./godot/Godot.app/Contents/MacOS/Godot --path ./examples/dev_module/ --headless --export-debug "macOS" output.app
   ```
   > **Note:** エクスポートを実行するには、対象プロジェクト内の `export_presets.cfg` に指定したプラットフォーム名（上記の場合は `"macOS"`）のプリセットが存在し、必要な識別子（バンドルIDなど）が正しく設定されている必要があります。

### Webプラットフォームのエクスポートと動作確認

Web向けのエクスポートでは複数のファイル（`.html`, `.wasm`, `.pck`等）が出力されるため、専用のディレクトリを作成してエクスポートします。また、ブラウザのセキュリティ制限を回避するためローカルサーバーでの確認が必要です。

```bash
# 1. 出力用ディレクトリを作成し、Web向けにエクスポート
mkdir -p build_web
./godot/Godot.app/Contents/MacOS/Godot --path ./examples/dev_module/ --headless --export-debug "Web" ../../build_web/index.html

# 2. ローカルHTTPサーバーを起動
cd build_web
python3 -m http.server 8000
```
サーバー起動後、ブラウザで `http://localhost:8000` にアクセスすると動作確認ができます。
（本プラグインは Web においては `nothread` での動作となるため、特殊なCORSヘッダーなしの単純なHTTPサーバーで起動可能です）

## SpriteStudio-SDK 開発者向け

> 以降のセクションは **SpriteStudio-SDK 自体を手元で開発・カスタマイズしながら Godot 側もビルドしたい場合のみ** 必要です。SpriteStudio-SDK のリリース成果物を使う一般的な Godot ビルダーは読み飛ばして構いません。

### 追加で必要なもの

`libssruntime` を自前でビルドするための環境 (Rust ツールチェーン等) のセットアップ手順は [SpriteStudio-SDK の README](https://github.com/cri-middleware/SpriteStudio-SDK?tab=readme-ov-file#for-sdk-developers) を参照してください。

FlatBuffers のヘッダを再生成する場合は別途 `flatc` (FlatBuffers コンパイラ) も必要です。

### libssruntime を自前でビルドする

SpriteStudio-SDK サブモジュール (`ss_player/SpriteStudio-SDK/`) が初期化済みであることが前提です。未初期化の場合は以下で取得してください。

```bash
git submodule update --init --recursive
```

その後、以下を実行すると Rust ランタイム/コンバータがビルドされ、`ss_player/runtime/` 配下に成果物が自動配置されます。

**macOS / Linux**

```sh
./scripts/build-runtime.sh [platform=<platform>] [arch=<arch>] [build=<build>] [ios_simulator=<yes|no>]
```

**Windows (PowerShell)**

```powershell
.\scripts\build-runtime.ps1 [platform=<platform>] [arch=<arch>] [build=<build>] [ios_simulator=<yes|no>]
```

| オプション | 値 | デフォルト |
| --- | --- | --- |
| `platform` | `windows`, `macos`, `linux`, `android`, `ios`, `web` | ホストOS |
| `arch` | `x86_64`, `arm64`, `universal`, `wasm32` など | ホストアーキ |
| `build` | `debug`, `release` | `debug` |
| `ios_simulator` | `yes`, `no` | `no` |

### FlatBuffers ヘッダの再生成

SpriteStudio-SDK の `.fbs` を変更した場合は、以下で `ss_player/format/` 配下のヘッダを再生成します (`flatc` が必要)。

**macOS / Linux**

```sh
./scripts/generate-runtime-code.sh
```

**Windows (PowerShell)**

```powershell
.\scripts\generate-runtime-code.ps1
```

### SpriteStudio-SDK 内部ドキュメント

サブモジュール初期化済みであれば、ランタイムの内部仕様や移植時の注意点は以下を参照できます。

- `ss_player/SpriteStudio-SDK/libs/ssruntime/docs/README.ja.md`
