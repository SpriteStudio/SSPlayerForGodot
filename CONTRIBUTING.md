# Contributing to SSPlayerForGodot

Thank you for your interest in contributing to SSPlayerForGodot! We welcome all kinds of contributions, including bug reports, feature requests, documentation improvements, and code contributions.

## Table of Contents
- [Code of Conduct](#code-of-conduct)
- [How Can I Contribute?](#how-can-i-contribute)
- [Development Environment](#development-environment)
- [Coding Standards](#coding-standards)

## Code of Conduct
Please refer to [CODE_OF_CONDUCT.md](./CODE_OF_CONDUCT.md).

## How Can I Contribute?

### Technical Constraints & Guiding Principles
To maintain high performance and native integration with Godot Engine, please adhere to the following principles:

- **Godot Idioms First:** Use Godot's built-in systems (`Node2D`, `CanvasItem` API, Signals) instead of recreating them. Let Godot handle the tree and transforms.
- **Performance in the Hot Path:** Avoid per-frame heap allocations during playback. Use the `DrawBatch` plans emitted by the runtime directly with Godot's `RenderingServer`.
- **SDK Separation:** The core playback logic resides in the `SpriteStudio-SDK` submodule (Rust). Changes to core logic should be directed to the SDK repository, while Godot-specific integrations belong in `ss_player/`.

### Reporting Bugs
If you find a bug, please use the provided Issue Templates. Include:
- A clear, descriptive title.
- Steps to reproduce.
- Your Godot version and OS.
- Expected vs. actual behavior.

*Note: If you need to send proprietary `.sspj` projects to reproduce the bug, please do not attach them to public issues. See [SUPPORT.md](./SUPPORT.md) for how to contact us privately.*

### Pull Requests
1. Fork the repository and create your branch from `develop`.
2. Ensure your code follows the existing style and Godot conventions.
3. Test your changes locally by running the sample projects in the `examples/` directory.
4. Submit a Pull Request targeting the `develop` branch with a clear description of the changes.

## Development Environment

### Prerequisites
- Godot Engine 4.x
- A C++ compiler (GCC, Clang, or MSVC)
- Python 3 and SCons
- A Rust toolchain (required to build the `libssruntime` runtime)
- zsh (the build scripts use a `#!/usr/bin/env zsh` shebang)
- `godot-cpp`, cloned into the repository root: `git clone https://github.com/godotengine/godot-cpp.git -b master` (it is not a submodule)

For the complete build guide, see [docs/en/setup/build.md](./docs/en/setup/build.md).

### Build the Plugin
To compile the Godot Extension, you must first build the Rust runtime from the SDK, and then compile the C++ extension using the provided build scripts:

**macOS / Linux:**
```bash
# 1. Build the Rust runtime (Requires Rust installed)
./scripts/build-runtime.sh

# 2. Build the GDExtension
./scripts/build-extension.sh
```

**Windows:**
```powershell
# 1. Build the Rust runtime (Requires Rust installed)
.\scripts\build-runtime.ps1

# 2. Build the GDExtension
.\scripts\build-extension.ps1
```

Once built, open the `examples/Ringo` project in the Godot Editor to verify your changes.

### Test
```bash
./scripts/fetch-godot.sh    # once: the editor build named in scripts/GODOT_VERSION.txt, into godot-bin/
./scripts/deploy-examples.sh
./scripts/run-tests.sh      # the GDExtension build, driven from GDScript
```
`run-tests.sh` needs a Godot binary — `godot=<path>`, else `$GODOT`, `godot-bin/`, then `PATH` — and refuses to start without the addon and the `.ssab` both installed, rather than skipping its way to a green run. **A green run is silent**: the suite ends with an `ENGINE` line counting the warnings and errors Godot itself printed, and anything it lists is a defect or a newly tolerated message. The verdict is the RESULT line, the marker after it, and that `ENGINE` line — not the exit code. Windows uses the `.ps1` variant of each script.

### Documentation
Pages live as both `docs/en/<path>` and `docs/ja/<path>`; there is no fallback locale, so a page missing from one side is a nav entry pointing at nothing. Each locale also ships its own `docs/<locale>/assets/`, and asset paths are source-relative (`../assets/…`) **including inside raw `<video>` / `<img>` HTML**, which Zensical rewrites the same way it rewrites Markdown links. If you edit a page, build both locales before you finish — see [RELEASING.md](./RELEASING.md). Nothing builds the docs on a pull request, so that local build is the only gate.

## Coding Standards

### C++ Guidelines
- Follow the [Godot Engine C++ style guide](https://docs.godotengine.org/en/stable/contributing/development/compiling/cpp_style_guide.html).
- Use `clang-format` if available (`clang-format -i ss_player/*.{cpp,h}`).
- Ensure safe handling of the Rust FFI objects: release every handle with its destroy function (`ss_runtime_destroy`, `ss_resource_destroy`, `ss_converter_destroy`) to prevent memory leaks.

---
By contributing to this project, you agree that your contributions will be licensed under the project's [LICENSE.md](./LICENSE.md).

<br>
<br>

---

# SSPlayerForGodot への貢献について

SSPlayerForGodot にご関心をお寄せいただき、ありがとうございます！ バグ報告、機能提案、ドキュメントの改善、コードの提供など、あらゆる形での貢献を歓迎します。

## 目次
- [行動規範](#行動規範)
- [貢献する方法](#貢献する方法)
- [開発環境のセットアップ](#開発環境のセットアップ)
- [コーディング規約](#コーディング規約)

## 行動規範
[CODE_OF_CONDUCT.md](./CODE_OF_CONDUCT.md) を参照してください。

## 貢献する方法

### 技術的制約と設計原則
Godot Engineとのネイティブな統合と高いパフォーマンスを維持するため、以下の原則に従ってください。

- **Godotのパラダイムを優先:** `Node2D` や `CanvasItem` API、シグナルなど、Godotの標準的な機能を積極的に活用してください。独自の実装を行うよりもGodotの設計に委ねることを優先します。
- **再生パスのパフォーマンス:** 再生中の毎フレームごとの動的なメモリ確保（ヒープアロケーション）は避けてください。Rustランタイムから提供される描画バッチ情報を直接 `RenderingServer` に渡すように実装されています。
- **SDKの分離:** コアのアニメーション計算ロジックは `SpriteStudio-SDK` サブモジュール(Rust)に存在します。アルゴリズムやコアロジックの修正はSDK側のリポジトリへ、Godot特有の実装は `ss_player/` へ行ってください。

### バグの報告
バグを発見した場合は、Issue テンプレートを使用して以下の情報を含めてください：
- 簡潔で分かりやすいタイトル
- 再現手順
- 利用している Godot のバージョンとOS
- 期待される動作と実際の動作

*注: バグ再現のために非公開の `.sspj` プロジェクトデータ等を提供いただける場合は、公開 Issue には添付せず、[SUPPORT.md](./SUPPORT.md) に記載のヘルプセンターをご利用ください。*

### プルリクエスト
1. リポジトリをフォークし、`develop` ブランチから作業用ブランチを作成してください。
2. 既存のGodot C++規約に従ってコードを記述してください。
3. `examples/` ディレクトリ内のサンプルプロジェクトを実行し、動作確認を行ってください。
4. `develop` ブランチに向けて、変更内容を明確に記載したプルリクエストを送信してください。

## 開発環境のセットアップ

### 前提条件
- Godot Engine 4.x
- C++ コンパイラ (GCC, Clang, MSVC のいずれか)
- Python 3 および SCons
- Rust ツールチェイン (`libssruntime` ランタイムのビルドに必要です)
- zsh (ビルドスクリプトは `#!/usr/bin/env zsh` を使用しています)
- `godot-cpp`（submodule ではありません。リポジトリのルートに `git clone https://github.com/godotengine/godot-cpp.git -b master` で取得してください）

完全なビルド手順については、[docs/ja/setup/build.md](./docs/ja/setup/build.md) を参照してください。

### ビルド方法
提供されているスクリプトを使用して、まずSDKからRustランタイムをビルドし、次にGDExtensionをコンパイルします。

**macOS / Linux:**
```bash
# 1. Rustランタイムのビルド (Rust環境が必要です)
./scripts/build-runtime.sh

# 2. GDExtensionのビルド
./scripts/build-extension.sh
```

**Windows:**
```powershell
# 1. Rustランタイムのビルド (Rust環境が必要です)
.\scripts\build-runtime.ps1

# 2. GDExtensionのビルド
.\scripts\build-extension.ps1
```

ビルド完了後、Godotエディタで `examples/Ringo` プロジェクトなどを開き、変更内容をテストしてください。

### テスト
```bash
./scripts/fetch-godot.sh    # 一度だけ: scripts/GODOT_VERSION.txt のエディタビルドを godot-bin/ へ
./scripts/deploy-examples.sh
./scripts/run-tests.sh      # GDExtension ビルドを GDScript から駆動
```
`run-tests.sh` は Godot バイナリを必要とします（`godot=<path>`、無ければ `$GODOT`、`godot-bin/`、最後に `PATH`）。addon と `.ssab` が揃っていなければ、スキップして緑にするのではなく起動を拒否します。**成功した実行は何も出力しません。** スイートの最後に Godot 自身が出した警告とエラーを数える `ENGINE` 行が出て、そこに並ぶものは不具合か新たに容認したメッセージのどちらかです。判定は終了コードではなく、RESULT 行とその後のマーカー、そしてこの `ENGINE` 行で行います。Windows は各スクリプトの `.ps1` 版を使ってください。

### ドキュメント
ページは `docs/en/<path>` と `docs/ja/<path>` の両方に存在します。フォールバックロケールは無いため、片方に欠けたページは行き先の無い nav エントリになります。アセットもロケールごとに `docs/<locale>/assets/` を持ち、パスはソース相対（`../assets/…`）です。**生の `<video>` / `<img>` HTML の中でも同じ**で、Zensical は Markdown リンクと同じ規則で書き換えます。ページを編集したら、仕上げる前に両ロケールをビルドしてください（手順は [RELEASING.md](./RELEASING.md)）。プルリクエストでは docs はビルドされないため、手元のビルドが唯一のゲートです。

## コーディング規約

### C++ のガイドライン
- [Godot EngineのC++スタイルガイド](https://docs.godotengine.org/ja/stable/contributing/development/compiling/cpp_style_guide.html)に従ってください。
- 可能であれば `clang-format` を使用してフォーマットを統一してください（`clang-format -i ss_player/*.{cpp,h}`）。
- Rust FFI オブジェクトの安全な取り扱いに注意してください。メモリリークを防ぐため、ハンドルは必ず対応する destroy 関数（`ss_runtime_destroy`、`ss_resource_destroy`、`ss_converter_destroy`）で解放してください。

---
本プロジェクトへの貢献は、プロジェクトの [LICENSE.md](./LICENSE.md) に同意したものとみなされます。
