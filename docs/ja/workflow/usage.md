# 基本的な使い方

## SpriteStudio ノードの作成

1. シーンに **`SpriteStudioPlayer2D`** ノードを追加します (Node2D 系)。
2. インスペクタの **SSAB Resource** プロパティに、インポート済みの `.ssab` ファイルを指定します (`Load` から選択)。
3. **Animation** プロパティに再生したいアニメーション名を指定します。`.ssab` に含まれるアニメーション名がドロップダウンで選択できます。
4. インスペクタの再生関連プロパティ (Frame, Speed, Loop, Playing 等) を調整するとプレビューできます。

## 主なインスペクタプロパティ

| プロパティ                | 型     | 説明                                                                |
| -------------------------- | ------ | ------------------------------------------------------------------- |
| `SSAB Resource`           | Resource | 再生対象の `SSABResource` (`.ssab` ファイル)                     |
| `Animation`                | String | 選択中のアニメーション名                                            |
| `Frame`                    | float  | 現在のフレーム位置                                                  |
| `Speed`                    | float  | 再生速度倍率 (既定: 1.0)                                            |
| `Frame Rate`               | int    | FPS                                                                 |
| `Loop`                     | int    | ループ回数 (`0` で無限ループ)                                       |
| `Playing`                  | bool   | 再生フラグ                                                          |
| `Animation Section Start` | int    | 部分再生の開始フレーム                                              |
| `Animation Section End`   | int    | 部分再生の終了フレーム                                              |
| `Playback Direction`       | int    | 再生方向                                                            |
| `Playback Style`           | int    | 再生スタイル (片道/往復 等)                                         |
| `Skip Frames`              | bool   | 描画間隔がフレーム間隔を超えた際にフレームを飛ばすか                |
| `Sub Frame Enabled`        | bool   | サブフレーム補間を有効化するか                                      |
