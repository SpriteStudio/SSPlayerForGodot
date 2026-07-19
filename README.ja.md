[**日本語**](./README.ja.md) | [**English**](./README.md)

# SpriteStudioPlayer for Godot

**Godot のゲームに、プロフェッショナルな2Dアニメーションを。直感的な使いやすさと、極限のパフォーマンスを両立するプラグイン。**

> **注意:** 本 `develop` ブランチは現在開発中のバージョンです。安定版は [main ブランチ](https://github.com/cri-middleware/SSPlayerForGodot/tree/main) または [Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases) から取得してください。本ブランチで扱う API・ワークフローは予告なく変更される可能性があり、いかなる保証もサポートも提供しません（リクエストやバグ報告への返信もできません）。

**[OPTPiX SpriteStudio 7](https://www.webtech.co.jp/spritestudio/)** のアニメーション (`.ssab`) を [Godot Engine](https://godotengine.org/) 上で再生するためのハイパフォーマンスな拡張プラグイン（GDExtension / カスタムモジュール）です。Godot の強力な機能と、専用アニメーションツールの表現力を組み合わせることで、リッチな2Dゲーム開発をサポートします。

## ✨ SpriteStudio を Godot で使うメリット

- **キャラクターからUI、エフェクトまで完結する「圧倒的な汎用性」**
  キャラクター特化型のツールとは異なり、メッシュ変形を用いたキャラクターアニメーションから、UIトランジション、リッチなパーティクルエフェクトまで、すべてを一つの専用エディタで完結できます。お使いのラスター画像の表現力を極限まで引き出します。
- **背景やエフェクトを含めた「シーン全体」の構築**
  キャラクター単体だけでなく、背景、エフェクト、UI を組み合わせた「カットシーン」や「画面全体」の演出をエディタ上で丸ごと構築し、Godot 上で1つのアニメーションとしてそのまま再生できます。
- **他エンジンと完全に一致する「視覚的整合性」**
  複雑な状態計算は独立したコアランタイムが処理するため、Godotの独自仕様による「見た目のブレ」が構造的に発生しません。エディタ上の見た目や他エンジン上での再生結果と完全に一致することが保証されます。サブフレーム補間により、高リフレッシュレートでも滑らかに描画されます。
- **Godotの「ノード」としての自然な統合と、コンフリクト回避**
  `SpriteStudioPlayer2D` は標準的なノードとして Godot のシーンにシームレスに統合されるため、ノードツリーを肥大化させず GDScript から簡単に制御できます。同時に、アニメーションデータ自体はシーンから分離されるため、チーム開発時の Git コンフリクトを未然に防ぎます。
- **ゼロコピー・ロードと SIMD 活用による極限パフォーマンス**
  最適化されたバイナリ（`.ssab` / FlatBuffers）へコンバートして使用することで、実行時のパース負荷をゼロに抑え、オンメモリで即座に再生できます。内部計算でSIMDをフル活用しており、最小のCPU・メモリ負荷で最大のアニメーション再生パフォーマンスを叩き出します。大量のキャラクターを描画するゲームや、モバイル環境でも余裕のある動作を実現します。

## 📚 ドキュメント

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

## 🚀 GDExtension を用いたクイックスタート

初めての方向けに、サンプルプロジェクトを使用した動作確認と、ご自身のプロジェクトへ導入する手順の2つを用意しています。

### 1. サンプルで動作確認する

1. **Godot Engine の準備**: [公式サイト](https://godotengine.org/download/) から 4.6 系のエディタをダウンロードします。
2. **GDExtension の取得**: [Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases) から最新パッケージをダウンロードし、展開します。
3. **サンプルの準備**: 取得した `addons` フォルダを、本リポジトリの [examples/Ringo](./examples/Ringo) フォルダ内にコピーします。
4. **確認**: Godot Engine で [examples/Ringo](./examples/Ringo) プロジェクトを開き、`Ringo.tscn` を開くことですぐにアニメーションの動作を確認できます。

### 2. 自身のプロジェクトへ導入する

1. **配置**: 取得した `addons` フォルダを、ご自身の Godot プロジェクトのルートにコピーします。
2. **インポート**: `.sspj` を Godot エディタにドラッグ＆ドロップして `.ssab` へ変換します。
3. **再生**: `SpriteStudioPlayer2D` ノードを追加し、`SSAB Resource` プロパティに生成された `.ssab` を指定します。

詳細は [インストールガイド](./docs/ja/setup/install.md) を参照してください。

## 💡 概要 (Overview)

本プラグインは、**SpriteStudio と Godot エディタをシームレスに行き来できる強力なアセットパイプライン**を備え、一瞬でアセットを更新することが可能です。

データフロー図、主な機能、対応バージョンなどの詳細は **[ドキュメント (日本語)](./docs/ja/index.md)** を参照してください。

## 🎬 サンプル

[examples フォルダ](./examples/) に SDK のテストプロジェクトに基づいたサンプルプロジェクトがあります。

- [Ringo](./examples/Ringo) — Ringo用のテスト
- [Scripting](./examples/Scripting) — GDScriptを用いたアニメーション制御やシグナル受信のサンプル
- [allAttributeV7](./examples/allAttributeV7) — 全アトリビュートの機能テスト
- [allPartsV7](./examples/allPartsV7) — 全パーツ種の機能テスト
- [overall](./examples/overall) — 総合的な機能テスト
- [overall_gdextension](./examples/overall_gdextension) — GDExtension 版での総合テスト
- [ParticleEffect](./examples/ParticleEffect) — エフェクト機能のテスト

## 🔗 関連リポジトリ

- [SpriteStudio-SDK](https://github.com/cri-middleware/SpriteStudio-SDK) — `libssruntime` / `libssconverter` を提供する SDK 本体

## 📄 ライセンス

[LICENSE.md](./LICENSE.md) を参照してください。

サードパーティライブラリ（FlatBuffers, SpriteStudio-SDK の依存クレートなど）のライセンスについては、[THIRD_PARTY_NOTICES.md](./THIRD_PARTY_NOTICES.md) を参照してください。
