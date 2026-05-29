# 基本的な使い方

このページでは、Godot のシーン上に SpriteStudio のアニメーションを配置して再生するまでの基本的な手順を解説します。

> [!NOTE]
> 本ページは **`.ssab` ファイルがすでに Godot プロジェクトにインポート済みであること** を前提としています。まだ `.ssab` が無い場合は、先に [アセットのインポートとエディタ連携](usage_asset_pipeline.md)（エディタからの取り込み）または [CLI コンバートと自動化](import.md)（CLI からの取り込み）を参照してください。

## 最速セットアップ: ドラッグ＆ドロップによるノード生成

Godot エディタの強力な機能を活かし、最短の手順でアニメーションをセットアップできます。

1. **ファイルシステムドック**から、インポート済みの **`.ssab` ファイル** を探します。
2. そのファイルを **2D ワークスペース (シーンビュー)** へドラッグ＆ドロップします。

これだけで自動的に `SpriteStudioPlayer2D` ノードがシーンに追加され、リソースの割り当ても完了します。

> [!TIP]
> <video autoplay loop muted playsinline width="100%">
>   <source src="../../assets/setup_drag_and_drop.webm" type="video/webm">
> </video>
> *(※上記に .ssab をドラッグ＆ドロップしてノードが自動生成される様子を示す動画が入ります)*

---

## 手動でのノード追加とアタッチ

既存のノード階層の特定の場所に配置したい場合は、手動でノードを追加してリソースを割り当てることも可能です。

1. シーンドックの「＋」ボタンなどから、`SpriteStudioPlayer2D` ノードをシーンツリーに追加します。
2. 追加したノードを選択し、インスペクタを開きます。
3. インスペクタの **`SSAB Resource`** プロパティへ、ファイルシステムドックから `.ssab` ファイルをドラッグ＆ドロップしてアタッチします。

---

## インスペクタでの設定とプレビュー

ノードを選択すると、Godot のインスペクタから各種設定を行えます。

1. **`Animation` の選択**
   インスペクタの `Animation` プロパティのドロップダウンを開くと、`.ssab` に含まれるアニメーションのリストが表示されます。再生したいアニメーション名を選択してください。

2. **エディタ上でのプレビュー**
   インスペクタにある **`Editor Playing`** プロパティにチェックを入れると、**ゲームを実行しなくてもエディタ上でアニメーションが再生されます**（この状態はシーンには保存されません）。
   `Frame` や `Speed`、`Loop` などのパラメータを変更するとリアルタイムにプレビューへ反映されるため、素早い調整が可能です。

> [!TIP]
> <video autoplay loop muted playsinline width="100%">
>   <source src="../../assets/inspector_preview.webm" type="video/webm">
> </video>
> *(※上記に Editor Playing にチェックを入れてエディタ上でプレビューが動く様子を示す動画が入ります)*

---

## 主なインスペクタプロパティ

| プロパティ                | 型     | 説明                                                                |
| -------------------------- | ------ | ------------------------------------------------------------------- |
| `SSAB Resource`           | Resource | 再生対象の `SSABResource` (`.ssab` ファイル)                     |
| `Animation`                | String | 選択中のアニメーション名                                            |
| `Autoplay`                 | bool   | ゲーム開始時に自動再生するかどうか                                  |
| `Editor Playing`           | bool   | エディタ上でのみ再生するプレビュー用フラグ                          |
| `Frame`                    | float  | 現在のフレーム位置                                                  |
| `Speed`                    | float  | 再生速度倍率 (既定: 1.0)                                            |
| `Frame Rate`               | int    | FPS                                                                 |
| `Loop Count`               | int    | ループ回数 (`-1` で無限ループ)                                      |
| `Animation Section Start` | int    | 部分再生の開始フレーム                                              |
| `Animation Section End`   | int    | 部分再生の終了フレーム                                              |
| `Playback Direction`       | int    | 再生方向                                                            |
| `Playback Style`           | int    | 再生スタイル (片道/往復 等)                                         |
| `Skip Frames`              | bool   | 描画間隔がフレーム間隔を超えた際にフレームを飛ばすか                |
| `Sub Frame Enabled`        | bool   | サブフレーム補間を有効化するか                                      |
