[**日本語**](./README.ja.md) | [**English**](./README.md)

# SpriteStudioPlayer for Godot

> **注意:** 本 `develop` ブランチは現在開発中のバージョンです。安定版は [main ブランチ](https://github.com/SpriteStudio/SSPlayerForGodot/tree/main) または [Releases](https://github.com/SpriteStudio/SSPlayerForGodot/releases) から取得してください。本ブランチで扱う API・ワークフローは予告なく変更される可能性があり、いかなる保証もサポートも提供しません（リクエストやバグ報告への返信もできません）。

[OPTPiX SpriteStudio 7](https://www.webtech.co.jp/spritestudio/) で作成したアニメーションを [Godot Engine](https://godotengine.org/) 上で再生するためのハイパフォーマンスなプラグインです。
このプラグインを使用することで、ラスターベースの2Dアニメーションを Godot プロジェクトへ簡単に実装・再生することができます。

## ドキュメント

詳細な使い方は `docs/` フォルダ内のドキュメントを参照してください。

- [**ドキュメント (日本語)**](./docs/ja/index.md)
- [**Documentation (English)**](./docs/en/index.md)

### クイックリンク (日本語)
- [インストール](./docs/ja/setup/install.md)
- [基本的な使い方](./docs/ja/workflow/usage_basic.md)
- [アセットのインポートとエディタ連携](./docs/ja/workflow/usage_asset_pipeline.md)
- [スクリプト制御とイベント](./docs/ja/workflow/usage_scripting.md)
- [CLI コンバートと自動化](./docs/ja/workflow/import.md)
- [パフォーマンスチューニングと高度な設定](./docs/ja/workflow/tips.md)
- [ビルドガイド](./docs/ja/setup/build.md)
- [v1.x からのマイグレーション](./docs/ja/migration_from_v1.md)

## GDExtension を用いたクイックスタート

初めての方向けに、サンプルプロジェクトを使用した動作確認と、ご自身のプロジェクトへ導入する手順の2つを用意しています。

### 1. サンプルで動作確認する

1. **Godot Engine の準備**: [公式サイト](https://godotengine.org/download/) から 4.6 系のエディタをダウンロードします。
2. **GDExtension の取得**: [Releases](https://github.com/SpriteStudio/SSPlayerForGodot/releases) から最新パッケージをダウンロードし、展開します。
3. **サンプルの準備**: 取得した `addons` フォルダを、本リポジトリの [examples/Ringo](./examples/Ringo) フォルダ内にコピーします。
4. **確認**: Godot Engine で [examples/Ringo](./examples/Ringo) プロジェクトを開き、`Ringo.tscn` を開くことですぐにアニメーションの動作を確認できます。

### 2. 自身のプロジェクトへ導入する

1. **配置**: 取得した `addons` フォルダを、ご自身の Godot プロジェクトのルートにコピーします。
2. **インポート**: `.sspj` を Godot エディタにドラッグ＆ドロップして `.ssab` へ変換します。
3. **再生**: `SpriteStudioPlayer2D` ノードを追加し、`SSAB Resource` プロパティに生成された `.ssab` を指定します。

詳細は [インストールガイド](./docs/ja/setup/install.md) を参照してください。

## 概要 (Overview)

本プラグインは、**SpriteStudio と Godot エディタをシームレスに行き来できる強力なアセットパイプライン**を備え、一瞬でアセットを更新することが可能です。

データフロー図、主な機能、対応バージョンなどの詳細は **[ドキュメント (日本語)](./docs/ja/index.md)** を参照してください。

## サンプル

[examples フォルダ](./examples/) に SDK のテストプロジェクトに基づいたサンプルプロジェクトがあります。

- [Ringo](./examples/Ringo) — 「りんご」のテスト
- [allAttributeV7](./examples/allAttributeV7) — 全属性の機能テスト
- [allPartsV7](./examples/allPartsV7) — 全パーツ種の機能テスト
- [overall](./examples/overall) — 総合的な機能テスト
- [overall_gdextension](./examples/overall_gdextension) — GDExtension 版での総合テスト
- [ParticleEffect](./examples/ParticleEffect) — エフェクト機能のテスト

## 関連リポジトリ

- [SpriteStudio-SDK](https://github.com/cri-middleware/SpriteStudio-SDK) — `libssruntime` / `libssconverter` を提供する SDK 本体

## ライセンス

[LICENSE.md](./LICENSE.md) を参照してください。
