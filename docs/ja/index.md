# SpriteStudioPlayer for Godot

本developブランチは現在開発中のバージョンです。  
安定版は [mainブランチ](https://github.com/SpriteStudio/SSPlayerForGodot/tree/main) または、[Releases](https://github.com/SpriteStudio/SSPlayerForGodot/releases)から取得してください。  
本developブランチに関してはいかなる保証もサポートも提供しません。リクエストやバグ報告への返信もできません。  
インターフェースは予告なく変更される可能性があります。v1.x からの移行手順は別途マイグレーションドキュメントを用意予定です。

[OPTPiX SpriteStudio](https://www.webtech.co.jp/spritestudio/) で作成したアニメーションを [Godot Engine](https://godotengine.org/) 上で再生するためのプラグインです。
再生処理は [SpriteStudio7-SDK](https://github.com/SpriteStudio/SpriteStudio7-SDK) が提供する `libssruntime` を介して行います。

## 主な機能 (Key Features)

本プラグインは、Godot Engine 上で SpriteStudio 7 の表現力をフルに引き出すために設計されています。

*   **完全な機能サポート:** ボーン階層、メッシュ＆デフォーム、パーティクルエフェクトなど、SpriteStudio 7 の全機能を標準でサポートします。
*   **シームレスな統合:** Godot のエディタ内に統合された「SS Import Dock」により、ドラッグ＆ドロップで簡単にデータを変換・インポートできます。
*   **動的な着せ替え (CellMap Overrides):** 実行時にテクスチャ（セルマップ）を差し替えることで、キャラクターのカラーバリエーションや装備変更を簡単に実装できます。
*   **シグナルとイベント:** タイムライン上に設定した「ユーザーデータ」や「シグナル」を Godot のシグナルとして受け取り、足音の再生やスクリプトのトリガーを正確なタイミングで行えます。
*   **滑らかなスローモーション:** サブフレーム補間（Sub-frame interpolation）をサポートし、高リフレッシュレートのモニターやスローモーション演出でもカクつかない滑らかな再生が可能です。
*   **超高速・省メモリ:** バックエンドの `libssruntime` による SIMD 最適化と、パース不要なバイナリ形式（`.ssab`）により、モバイルなどのリソースが限られた環境でも多数のキャラクターを高速に描画します。

## 概要 (Overview)

本プラグインを利用してアニメーションを再生する際の流れを以下に示します。

```mermaid
graph LR
    subgraph Assets ["プロジェクトアセット"]
        SS[" .sspj / .ssae / .ssce "]
        IMG[" 画像ファイル / .png "]
    end

    subgraph Convert ["変換 (いずれか一方)"]
        DOCK[[" SS Import Dock<br>(Godot Editor 内蔵) "]]
        CLI[[" ssconverter-cli<br>(SS7-SDK Releases) "]]
        SS --> DOCK
        SS --> CLI
        DOCK --> BIN[" .ssab / .ssqb "]
        CLI --> BIN
    end

    subgraph Runtime ["Godot ランタイム (再生時)"]
        BIN --> RES(" SSABResource ")
        RES --> NODE(" SpriteStudioPlayer2D ")
        NODE -.-> RT(" libssruntime ")
        IMG --> NODE
        NODE --> RENDER[[" Godot レンダリング "]]
    end

    classDef generated stroke-dasharray: 5 5;
    class BIN generated;
```

## 対応バージョン

- **Godot Engine**: [4.6 ブランチ](https://github.com/godotengine/godot/tree/4.6)
- **godot-cpp**: [master ブランチ](https://github.com/godotengine/godot-cpp/tree/master)

> [!NOTE]
> GDExtension は Godot 4.6 以降から正式サポートされます。

Windows / macOS でのビルドおよび実行を確認しています。

## サンプル

リポジトリの `examples/` フォルダに SDK のテストプロジェクトに基づいたサンプルプロジェクトがあります。

- `allAttributeV7` — 全属性の機能テスト
- `allPartsV7` — 全パーツ種の機能テスト
- `overall` — 総合的な機能テスト
- `overall_gdextension` — GDExtension 版での総合テスト
- `ParticleEffect` — エフェクト機能のテスト
- `Ringo` — Ringoのテスト
- `dev_module` — モジュール版開発用プロジェクト
- `dev_gdextension` — GDExtension 版開発用プロジェクト

## 関連リポジトリ

- [SpriteStudio7-SDK](https://github.com/SpriteStudio/SpriteStudio7-SDK) — `libssruntime` / `libssconverter` を提供する SDK 本体

## ライセンス

[LICENSE.txt](../../LICENSE.txt) を参照してください。
