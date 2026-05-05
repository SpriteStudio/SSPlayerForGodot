[**日本語**](./USAGE.ja.md) | [**English**](./USAGE.md)

> **Note:** 本書で扱う API・ワークフローは現在開発中のため、予告なく変更される可能性があります。
> v1.x からの移行手順は別途マイグレーションドキュメントを用意予定です。

# エディタの選択

下記いずれかの方法で利用してください。

## A. SSPlayerForGodot の GDExtension を利用する

1. [公式サイト](https://godotengine.org/download/) より対応するバージョンの Godot Engine をダウンロードします。
2. [SSPlayerForGodot の Releases](https://github.com/SpriteStudio/SSPlayerForGodot/releases) から該当プラットフォーム向けの GDExtension 一式をダウンロードします (自前でビルドする場合は [BUILD.ja.md](BUILD.ja.md) を参照)。
3. ダウンロードした ZIP を解凍し、中にある `addons` フォルダをそのまま Godot プロジェクトのルートディレクトリにコピー（上書きマージ）します。
   * 正しく配置されると、`res://addons/spritestudio/spritestudio.gdextension` が存在する状態になります。

## B. SSPlayerForGodot のカスタムモジュールを組み込んだ Godot Engine を利用する

[BUILD.ja.md](BUILD.ja.md) を参照してカスタムモジュール組み込みの Godot Engine をビルドしてください。

# SpriteStudio データのインポート

SpriteStudio のプロジェクト (`.sspj`) は **`.ssab` (アニメバイナリ)** および **`.ssqb` (シーケンスバイナリ)** へ変換し、Godot プロジェクト配下に配置することで `SpriteStudioPlayer2D` から利用できます。
変換方法は以下の2通りです。どちらの方法で生成したファイルも同じく `SSABResource` / `SSQBResource` として読み込めます。

## 方法 A: Godot エディタの SS Import Dock を使う

エディタを起動するとプロジェクトドック側に SpriteStudio 用のインポートコントロールが追加されます。
最初に変換成果物の出力先を指定します。

* デフォルトの出力先: `res://ssab_generated`
* 設定キー (プロジェクト設定): `spritestudio/output_directory`

出力先はインポートコントロール上の `Browse` ボタンまたは入力欄から変更できます。

以下のいずれかの操作で変換が行われます。

* `.sspj` ファイルをエディタウィンドウへドラッグ＆ドロップする
* インポートコントロールに表示される履歴から再実行する

変換中は進捗ダイアログが表示され、完了後は出力先ディレクトリ配下に `.ssab` (アニメパック単位) と `.ssqb` (シーケンス単位) が生成されます。

## 方法 B: SpriteStudio7-SDK の `ssconverter-cli` を直接使う

[SpriteStudio7-SDK の Releases](https://github.com/SpriteStudio/SpriteStudio7-SDK/releases) から該当プラットフォーム向けの `ssconverter-cli` をダウンロードし、コマンドラインから `.sspj` を変換します。

**Windows:**
```powershell
.\ssconverter-cli.exe path\to\your.sspj
```

**macOS / Linux:**
```bash
./ssconverter-cli path/to/your.sspj
```

変換結果は `.sspj` と同じディレクトリの `<sspj 名>_ssab/` 配下に `.ssab` / `.ssqb` として生成されます。
このディレクトリ名を変更して Godot プロジェクト配下にコピーすると、`SSABResource` / `SSQBResource` として読み込めるようになります。

CI / ビルドパイプラインに変換処理を組み込みたい場合や、Godot エディタを起動せずに変換のみを行いたい場合に便利です。`ssconverter-cli` の詳細なオプションは [`SpriteStudio7-SDK/cli/README.ja.md`](https://github.com/SpriteStudio/SpriteStudio7-SDK/blob/main/cli/README.ja.md) を参照してください。

# SpriteStudio ノードの作成

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

実際の挙動・引数のとり得る値は `ss_player/ss_player_node_2d.h` を参照してください。

# クラス

GDScript からコントロールできる主なクラスです。
全メソッド・プロパティ・シグナルは Godot のスクリプトドキュメントから確認してください。

## リソース管理クラス

Godot の `Resource` を継承しているため、複数の `SpriteStudioPlayer2D` から同じリソースを参照する場合は **Local To Scene** フラグを `True` に設定すると個別に状態を持たせられます。

### [SSABResource](./ss_player/ssab_resource.h)

`.ssab` (アニメバイナリ) に対応するリソースクラスです。

主なメソッド:

* `load_from_file(path: String) -> Error`
* `save_to_file(path: String) -> Error`
* `is_valid() -> bool`
* `get_animation_count() -> int`
* `get_animation_names() -> PackedStringArray`

### [SSQBResource](./ss_player/ssqb_resource.h)

`.ssqb` (シーケンスバイナリ) に対応するリソースクラスです。

主なメソッド:

* `load_from_file(path: String) -> Error`
* `save_to_file(path: String) -> Error`

## 再生クラス: [SpriteStudioPlayer2D](./ss_player/ss_player_node_2d.h)

`Node2D` を継承する再生用ノードです。
リソースとアニメーションを指定して再生を行います。

```gdscript
@onready var ssnode: SpriteStudioPlayer2D = $target

func _ready() -> void:
    # .ssab を読み込んでリソースを指定
    var ssab: SSABResource = ResourceLoader.load("res://ssab_generated/Sample.ssab")
    ssnode.set_ssab_resource(ssab)

    # アニメーション名を指定
    ssnode.set_animation("anime_1")

    # 再生
    ssnode.set_loop(0)        # 0 で無限ループ
    ssnode.set_speed(1.0)
    ssnode.play()
```

主なメソッド:

* `set_ssab_resource(res: SSABResource)` / `get_ssab_resource() -> SSABResource`
* `set_animation(name: String)` / `get_animation() -> String`
* `play(start_frame: float = -1.0)` / `pause()` / `stop()`
* `is_playing() -> bool` / `is_pausing() -> bool`
* `set_frame(frame: float)` / `get_frame() -> float` / `get_total_frames() -> int`
* `set_speed(speed: float)` / `get_speed() -> float`
* `set_frame_rate(fps: int)` / `get_frame_rate() -> int`
* `set_animation_section(start: int, end: int)`
* `set_playback_direction(direction: int, style: int)`
* `set_loop(count: int)` / `get_loop() -> int`
* `set_skip_frames(enabled: bool)` / `is_skip_frames() -> bool`
* `set_sub_frame_enabled(enabled: bool)` / `is_sub_frame_enabled() -> bool`
