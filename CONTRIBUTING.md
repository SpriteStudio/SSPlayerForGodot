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
To maintain the high-performance and portable nature of this SDK, all code contributions must adhere to the following core principles:

- **Strict `no_std` Compliance:** The core runtime (`Godot Plugin`, `libssab`, `libssinterpolate`) must remain `#![no_std]`. We do not allow dependencies on `std` or any platform-specific libraries that break portability.
- **Zero-Allocation in Playback Hot Path:** Dynamic memory allocation (heap allocation) is strictly prohibited during the animation playback loop. All necessary memory must be pre-allocated or provided by the caller.
- **Performance over Convenience:** We prioritize runtime execution speed and memory efficiency over "easy-to-use" helper functions if they introduce overhead.
- **SIMD-Friendly Design:** Data structures (especially in `ssab`) must be designed with SIMD (f32x4) vectorization in mind. Prefer columnar layouts and flat arrays.
- **Stateless Brain (Separation of Concerns):** The runtime (Brain) calculates states but must not have knowledge of the rendering engine (Body). Keep the interface primitive and deterministic.
- **Explicit over Implicit:** Avoid "magic" or hidden behaviors. Code should be predictable, traceable, and explicit in its intent.

### Reporting Bugs
If you find a bug, please open an issue and include:
- A clear, descriptive title.
- Steps to reproduce the issue.
- Your environment (OS, Rust version, target platform).
- Expected vs. actual behavior.

*Note: If you need to send proprietary `.sspj` projects to reproduce the bug, please do not attach them to public issues. See [SUPPORT.md](./SUPPORT.md) for how to contact us privately.*

### Suggesting Enhancements
If you have an idea, please open an issue to discuss it before starting work.

### Pull Requests
1. Fork the repository and create your branch from `main`.
2. Ensure your code follows the existing style and conventions.
3. Add tests for any new features or bug fixes.
4. Ensure all tests pass (`cargo test`).
5. Update documentation if necessary (READMEs, Doc comments).
6. Submit a Pull Request with a clear description of the changes.

## Development Environment

### Prerequisites
- [Rustup](https://rustup.rs/) (Stable channel)
- [FlatBuffers compiler](https://google.github.io/flatbuffers/) (Required if modifying `.fbs` files)
- [cbindgen](https://github.com/mozilla/cbindgen) (For C-API header generation)

### Build and Test
```bash
# Build the entire workspace
cargo build --release

# Run all tests
cargo test

# Run the converter CLI for verification
cargo run -p ssconverter-cli -- <path_to_sspj>
```

## Coding Standards

### Rust Guidelines
- Use `rustfmt` to format your code: `cargo fmt`.
- Check for common mistakes with `clippy`: `cargo clippy`.
- Follow [Idiomatic Rust](https://github.com/mre/idiomatic-rust) practices.

### FFI and C-API
Since this SDK provides C-compatible interfaces (`ssruntime`, `ssconverter`), please pay attention to:
- **Binary Compatibility**: Avoid breaking changes to the C-API unless absolutely necessary.
- **Header Generation**: Ensure the C++ headers are regenerated and verified if you modify FFI functions.
- **Memory Management**: Clearly document who owns the memory across the FFI boundary.

---
By contributing to this project, you agree that your contributions will be licensed under the project's [LICENSE.md](./LICENSE.md).

<br>
<br>

---

# SSPlayerForGodot への貢献について

SSPlayerForGodot にご関心をお寄せいただき、ありがとうございます！ バグ報告、機能提案、ドキュメントの改善、コードの提供など、あらゆる形での貢献を歓迎します。

## 目次
- [行動規範](#行動規範-1)
- [貢献する方法](#貢献する方法-1)
- [開発環境のセットアップ](#開発環境のセットアップ-1)
- [コーディング規約](#コーディング規約-1)

## 行動規範
[CODE_OF_CONDUCT.md](./CODE_OF_CONDUCT.md) を参照してください。

## 貢献する方法

### 技術的制約と設計原則
本 SDK の高いパフォーマンスとポータビリティを維持するため、すべてのコード貢献は以下の核心的な原則に従う必要があります。

- **厳格な `no_std` 遵守:** コアランタイム (`Godot Plugin`, `libssab`, `libssinterpolate`) は `#![no_std]` を維持しなければなりません。ポータビリティを損なう `std` やプラットフォーム依存のライブラリへの依存は許可されません。
- **再生パスでのゼロ・アロケーション:** アニメーションの再生ループ（ホットパス）内での動的なメモリ割り当て（ヒープ確保）は厳禁です。必要なメモリはすべて事前確保するか、呼び出し側から提供される必要があります。
- **便利さよりもパフォーマンス:** オーバーヘッドを導入するような「使いやすいヘルパー関数」よりも、実行速度とメモリ効率を常に優先します。
- **SIMD フレンドリーな設計:** データ構造（特に `ssab`）は、SIMD (f32x4) によるベクトル化を念頭に設計される必要があります。カラム指向のレイアウトやフラットな配列を優先してください。
- **ステートレスな Brain（関心の分離）:** ランタイム（Brain）は状態の計算に徹し、レンダリングエンジン（Body）に関する知識を持ってはなりません。インターフェースはプリミティブかつ決定論的（Deterministic）に保ってください。
- **暗黙的より明示的:** 「マジック」や隠れた挙動を避けてください。コードは意図が明示的であり、予測可能でトレース可能である必要があります。

### バグの報告
バグを発見した場合は、Issue を作成し、以下の情報を含めてください：
- 簡潔で分かりやすいタイトル
- 再現手順
- 実行環境（OS、Rust バージョン、ターゲットプラットフォーム）
- 期待される動作と実際の動作

*注: バグ再現のために非公開の `.sspj` プロジェクトデータ等を提供いただける場合は、公開 Issue には添付せず、[SUPPORT.md](./SUPPORT.md) に記載のヘルプセンターをご利用ください。*

### 機能の提案
SDK を改善するアイデアがある場合は、開発を始める前に Issue を作成して内容を議論しましょう。

### プルリクエスト
1. リポジトリをフォークし、`main` ブランチから作業用ブランチを作成してください。
2. 既存のスタイルや規約に従ってコードを記述してください。
3. 新しい機能やバグ修正には、対応するテストを追加してください。
4. すべてのテストがパスすることを確認してください (`cargo test`)。
5. 必要に応じてドキュメント (README やコメント) を更新してください。
6. 変更内容を明確に記載したプルリクエストを送信してください。

## 開発環境のセットアップ

### 前提条件
- [Rustup](https://rustup.rs/) (Stable チャンネル)
- [FlatBuffers コンパイラ](https://google.github.io/flatbuffers/) (`.fbs` ファイルを変更する場合に必要)
- [cbindgen](https://github.com/mozilla/cbindgen) (C-API ヘッダー生成に必要)

### ビルドとテスト
```bash
# ワークスペース全体のビルド
cargo build --release

# すべてのテストを実行
cargo test

# コンバーター CLI を実行して動作確認
cargo run -p ssconverter-cli -- <path_to_sspj>
```

## コーディング規約

### Rust のガイドライン
- `rustfmt` を使用してフォーマットを統一してください: `cargo fmt`
- `clippy` を使用して一般的なミスをチェックしてください: `cargo clippy`
- [慣用的な Rust](https://github.com/mre/idiomatic-rust) (Idiomatic Rust) のプラクティスに従ってください。

### FFI と C-API
この SDK は C 互換インターフェースを提供しているため、以下の点に注意してください：
- **バイナリ互換性**: 避けられない場合を除き、C-API の破壊的変更は避けてください。
- **ヘッダー生成**: FFI 関数を変更した場合は、C++ ヘッダーが正しく更新・検証されていることを確認してください。
- **メモリ管理**: FFI 境界を越えてデータをやり取りする場合、メモリの所有権を明確にドキュメント化してください。

---
本プロジェクトへの貢献は、プロジェクトの [LICENSE.md](./LICENSE.md) に同意したものとみなされます。
