[**English**](./README.md) | [**日本語**](./README.ja.md)

# SpriteStudioPlayer for Godot

**Godot のゲームに、プロフェッショナルな2Dアニメーションを。直感的な使いやすさと、極限のパフォーマンスを両立するプラグイン。**

> **注意:** 本 `develop` ブランチは現在開発中のバージョンです。安定版は [main ブランチ](https://github.com/cri-middleware/SSPlayerForGodot/tree/main) または [Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases) から取得してください。本ブランチで扱う API・ワークフローは予告なく変更される可能性があり、いかなる保証もサポートも提供しません（リクエストやバグ報告への返信もできません）。

**[SpriteStudio](https://www.webtech.co.jp/spritestudio/)** のアニメーション (`.ssab`) を [Godot Engine](https://godotengine.org/) 上で再生するためのハイパフォーマンスな拡張プラグイン（GDExtension / カスタムモジュール）です。Godot の強力な機能と、専用アニメーションツールの表現力を組み合わせることで、リッチな2Dゲーム開発をサポートします。

## ✨ SpriteStudio を Godot で使うメリット

- **キャラクターからUI、エフェクトまで完結する「圧倒的な汎用性」**
  キャラクター特化型のツールとは異なり、メッシュ変形を用いたキャラクターアニメーションから、UIトランジション、リッチなパーティクルエフェクトまで、すべてを一つの専用エディタで完結できます。お使いのラスター画像の表現力を極限まで引き出します。
- **背景やエフェクトを含めた「シーン全体」の構築**キャラクター単体だけでなく、背景、エフェクト、UI を組み合わせた「カットシーン」や「画面全体」の演出をエディタ上で丸ごと構築し、Godot 上で1つのアニメーションとしてそのまま再生できます。
- **エンジンをまたいだ「視覚的整合性」**
  ポーズ・補間・デフォーム・描画順は、全公式プレイヤーで共通の独立したコアランタイムが計算します。そのためエンジン独自仕様に由来する構造的な「見た目のブレ」は発生せず、ここはエンジン間で一致します。ブレンドモードとパーツカラーは「意図」として渡され、ホスト側の描画レイヤーが再現できる範囲で忠実に再現します（完全一致ではなくベストエフォート）。マスク・テキスト・音声は Godot 自身が担当します。サブフレーム補間により、高リフレッシュレートでも滑らかに描画されます。
- **Godotの「ノード」としての自然な統合と、コンフリクト回避**
  `SpriteStudioPlayer2D` は標準的なノードとして Godot のシーンにシームレスに統合されるため、ノードツリーを肥大化させず GDScript から簡単に制御できます。同時に、アニメーションデータ自体はシーンから分離されるため、チーム開発時の Git コンフリクトを未然に防ぎます。
- **ゼロコピー・ロードと SIMD 活用による極限パフォーマンス**最適化されたバイナリ（`.ssab` / FlatBuffers）へコンバートして使用することで、実行時のパース負荷をゼロに抑え、オンメモリで即座に再生できます。内部計算でSIMDをフル活用しており、最小のCPU・メモリ負荷で最大のアニメーション再生パフォーマンスを叩き出します。大量のキャラクターを描画するゲームや、モバイル環境でも余裕のある動作を実現します。

## 📚 ドキュメント

詳細な使い方は `docs/` フォルダ内のドキュメントにあります。データフロー図・主な機能・対応バージョンもそちらです。

- [**ドキュメントサイト (ホスト版)**](https://cri-middleware.github.io/SSPlayerForGodot/ja/) — 🚧 初回リリース後に公開
- [**ドキュメント (日本語)**](./docs/ja/index.md)
- [**Documentation (English)**](./docs/en/index.md)
- [**SpriteStudio Docs（ポータル）**](https://cri-middleware.github.io/SpriteStudio-Docs/ja/) — SDK と全公式 Player の入口 — 🚧 初回リリース後に公開

### クイックリンク (日本語)
- [インストール](./docs/ja/setup/install.md)
- [基本的な使い方](./docs/ja/workflow/usage_basic.md)
- [制限事項と対応範囲](./docs/ja/limitations.md)
- [トラブルシューティング](./docs/ja/troubleshooting.md)
- [v1.x からのマイグレーション](./docs/ja/migration_from_v1.md)

## 🚀 GDExtension を用いたクイックスタート

**ゲームで使う場合**はこのまま下へ。アドオンを入れる → `.sspj` をエディタに D&D → ノードを置く。その先（スクリプト制御、シグナル、パーツ単位の上書き）も同じ道の続きで、[ドキュメント](./docs/ja/index.md)にあります。
**Player 自体や、その下の Rust ランタイムを変更する場合**は [CONTRIBUTING.md](./CONTRIBUTING.md) と[ビルドガイド](./docs/ja/setup/build.md)へ。

初めての方向けに、サンプルプロジェクトを使用した動作確認と、ご自身のプロジェクトへ導入する手順の2つを用意しています。対応するのは **SpriteStudio 7.5 以上**で作成されたプロジェクトです。

### 1. サンプルで動作確認する

> 🚧 **この世代はまだリリースされていません。** [Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases) にあるのは **1.x** 系のプラグインのみで、本ブランチのサンプルは開けません（`SSABResource` と `SpriteStudioPlayer2D` はいずれも 7.x で入ったクラスです）。7.x の初回リリースまでは、[ビルドガイド](./docs/ja/setup/build.md) に従ってこのチェックアウトから拡張をビルドし、手順 3 から続けてください。

1. **Godot Engine の準備**: [公式サイト](https://godotengine.org/download/) から 4.7 系のエディタをダウンロードします。
2. **リポジトリの取得**: `--recurse-submodules` 付きでクローンします。サンプルの元プロジェクトは `ss_player/SpriteStudio-SDK` submodule にあり、これが無いとサンプルは変換対象を持ちません。
3. **GDExtension の取得**: [Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases) から最新パッケージをダウンロードし、展開します。
4. **サンプルの準備**: 取得した `addons` フォルダを、本リポジトリの [examples/Ringo](./examples/Ringo) フォルダ内にコピーします。
5. **確認**: Godot Engine で [examples/Ringo](./examples/Ringo) プロジェクトを開きます。アドオンが `.ssplayer_sources.cfg` を読んで初回起動時に `Ringo.sspj` を変換するため（`.ssab` は commit していません）、そのあと `Ringo.tscn` を開けばアニメーションの動作を確認できます。

### 2. 自身のプロジェクトへ導入する

1. **配置**: 取得した `addons` フォルダを、ご自身の Godot プロジェクトのルートにコピーします。
2. **インポート**: `.sspj` を Godot エディタにドラッグ＆ドロップして `.ssab` へ変換します。
3. **再生**: `SpriteStudioPlayer2D` ノードを追加し、`Ssab` プロパティに生成された `.ssab` を指定します。

詳細は [インストールガイド](./docs/ja/setup/install.md) を参照してください。

## 🎬 サンプル

[examples フォルダ](./examples/) に SDK のテストプロジェクトに基づいたサンプルプロジェクトがあります。

- [Ringo](./examples/Ringo) — Ringo用の基本クイックスタートテスト
- [Scripting](./examples/Scripting) — GDScriptを用いたアニメーション制御やシグナル受信のサンプル
- [Override_Ringo](./examples/Override_Ringo) — アトリビュート・マテリアルのオーバーライドサンプル
- [overall](./examples/overall) — 総合的な機能テスト（カスタムモジュール版）
- [overall_gdextension](./examples/overall_gdextension) — 総合的な機能テスト（GDExtension版）

## 🔗 関連リポジトリ

- [SpriteStudio Docs](https://cri-middleware.github.io/SpriteStudio-Docs/ja/) — SDK と公式 Player のドキュメントポータル
- [SpriteStudio-SDK](https://github.com/cri-middleware/SpriteStudio-SDK) — `libssruntime` / `libssconverter` を提供する SDK 本体
- [SSConverterGUI](https://github.com/cri-middleware/SSConverterGUI) — Player なしで `.sspj` を変換できるデスクトップ GUI

## 📄 ライセンス

[LICENSE.md](./LICENSE.md)（正文）を参照してください。参考訳は [LICENSE.ja.md](./LICENSE.ja.md) にあります。

サードパーティライブラリ（FlatBuffers, SpriteStudio-SDK の依存クレートなど）のライセンスについては、[THIRD_PARTY_NOTICES.md](./THIRD_PARTY_NOTICES.md) を参照してください。
