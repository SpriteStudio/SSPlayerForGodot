[**English**](./README.md) | [**日本語**](./README.ja.md)

# SpriteStudio Player for Godot

**Godot のゲームに、プロフェッショナルな2Dアニメーションを。直感的な使いやすさと、極限のパフォーマンスを両立するプラグイン。**

> **注意:** 本 `develop` ブランチは現在開発中のバージョンです。安定版は [main ブランチ](https://github.com/cri-middleware/SSPlayerForGodot/tree/main) または [Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases) から取得してください。本ブランチで扱う API・ワークフローは予告なく変更される可能性があり、いかなる保証もサポートも提供しません（リクエストやバグ報告への返信もできません）。

**[SpriteStudio](https://www.webtech.co.jp/spritestudio/)** のアニメーション (`.ssab`) を [Godot Engine](https://godotengine.org/) 上で再生するためのハイパフォーマンスな拡張プラグイン（GDExtension / カスタムモジュール）です。Godot の強力な機能と、専用アニメーションツールの表現力を組み合わせることで、リッチな2Dゲーム開発をサポートします。

## ✨ SpriteStudio を Godot で使うメリット

- **キャラクターが“生きる”。** 呼吸・まばたき、なびく髪や衣装、斬撃・魔法のエフェクトまで——動きは専用エディタ（SpriteStudio）でボーン・メッシュ変形・パーティクルを使ってビジュアルに作り込み、Godot はそれを再生するだけ。`AnimationPlayer` で組み直す必要はありません。
- **キャラクターだけでなく、エフェクトや UI、シーン全体も。** キャラクター・背景・エフェクト・UI を組み合わせたカットシーンや画面全体の演出をエディタで作り、Godot では 1 つのアニメーションとして再生できます。
- **いつものノードとして使える。** `SpriteStudioPlayer2D` は他のノードと同じようにシーンに置き、GDScript から操作できます。アニメーションはシーンの外の専用ファイルに入るので、チーム開発でシーンの Git コンフリクトが起きにくくなります。
- **SpriteStudio と Godot をシームレスに往復。** `.sspj` を SS Import Dock にドロップすれば、エディタ内で変換されます。直したくなったらインスペクタの **Open SSPJ** で SpriteStudio を開いて保存し、**Reconvert** を押すだけ。書き出しの手作業は要りません。
- **パース不要の読み込みと SIMD で、軽く滑らかに。** `.ssab` は FlatBuffers のバイナリをそのまま読むので、読み込み時のパースがありません。ポーズ計算は SIMD で最適化した Rust コアが担い、モバイルや大量のキャラクターを出すゲームでも滑らかに動きます。**サブフレーム補間**をオンにすれば、高リフレッシュレートのディスプレイにも追従します。

## 📚 ドキュメント

詳細な使い方は `docs/` フォルダ内のドキュメントにあります。データフロー図・主な機能・対応バージョンもそちらです。

- [**ドキュメントサイト (ホスト版)**](https://cri-middleware.github.io/SSPlayerForGodot/ja/)
- [**ドキュメント (日本語)**](./docs/ja/index.md)
- [**Documentation (English)**](./docs/en/index.md)
- [**AI アシスタント向け**](https://cri-middleware.github.io/SSPlayerForGodot/ja/llms.txt) — このドキュメントを Markdown で提供します。`llms.txt` はページの目次、`llms-full.txt` は全ページを 1 ファイルにまとめたものです
- [**SpriteStudio Docs（ポータル）**](https://cri-middleware.github.io/SpriteStudio-Docs/ja/) — SDK と全公式 Player の入口

### クイックリンク (日本語)
- [インストール](./docs/ja/setup/install.md)
- [基本的な使い方](./docs/ja/workflow/usage_basic.md)
- [制約と適用範囲](./docs/ja/limitations.md)
- [トラブルシューティング](./docs/ja/troubleshooting.md)
- [v1.x からのマイグレーション](./docs/ja/migration_from_v1.md)

## 🚀 GDExtension を用いたクイックスタート

**ゲームで使う場合**はこのまま下へ。アドオンを入れる → `.sspj` をエディタに D&D → ノードを置く。その先（スクリプト制御、シグナル、パーツ単位の上書き）も同じ道の続きで、[ドキュメント](./docs/ja/index.md)にあります。
**Player 自体や、その下の Rust ランタイムを変更する場合**は [CONTRIBUTING.md](./CONTRIBUTING.md) と[ビルドガイド](./docs/ja/setup/build.md)へ。

[公式サイト](https://godotengine.org/download/) から 4.7 系の Godot エディタを用意してください。対応するのは **SpriteStudio 7.5 以上**で作成されたプロジェクトです。

1. **配置**: [Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases) で、タグが `v7` で始まる最新の Release を開き、**Assets** から `ssplayer-godot-extension-4.7.zip` をダウンロードします（タグが `v1` で始まる Release は 1.x 系のプラグインで、`SSABResource` も `SpriteStudioPlayer2D` もありません）。展開して、`addons` フォルダをご自身の Godot プロジェクトのルートにコピーします。
2. **インポート**: `.sspj` をファイルマネージャーから **SS Import** ドックへドラッグ＆ドロップして `.ssab` へ変換します（[アセットのインポートとエディタ連携](./docs/ja/workflow/usage_asset_pipeline.md) を参照）。
3. **再生**: `SpriteStudioPlayer2D` ノードを追加し、`Ssab` プロパティに生成された `.ssab` を指定します。

SpriteStudio のデータが手元に無くても始められます。[公式のサンプルデータ](https://www.webtech.co.jp/help/ja/spritestudio7/download/sample/)がそのまま使えます。キャラクターなら [Ringo](https://www.webtech.co.jp/help/ja/spritestudio7/download/sample/#ringo)、エフェクトを含むものなら [パーティクル](https://www.webtech.co.jp/help/ja/spritestudio7/download/sample/#Perticle_sample) が手頃です。

詳細は [インストールガイド](./docs/ja/setup/install.md) を参照してください。

## 🔗 関連リポジトリ

- [SpriteStudio Docs](https://cri-middleware.github.io/SpriteStudio-Docs/ja/) — SDK と公式 Player のドキュメントポータル
- [SpriteStudio-SDK](https://github.com/cri-middleware/SpriteStudio-SDK) — `libssruntime` / `libssconverter` を提供する SDK 本体
- [SSConverterGUI](https://github.com/cri-middleware/SSConverterGUI) — Player なしで `.sspj` を変換できるデスクトップ GUI

## 📄 ライセンス

[LICENSE.md](./LICENSE.md)（正文）を参照してください。参考訳は [LICENSE.ja.md](./LICENSE.ja.md) にあります。

サードパーティライブラリ（FlatBuffers, SpriteStudio-SDK の依存クレートなど）のライセンスについては、[THIRD_PARTY_NOTICES.md](./THIRD_PARTY_NOTICES.md) を参照してください。
