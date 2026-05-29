# SpriteStudioPlayer2D

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
    ssnode.set_loop_count(-1)  # -1 で無限ループ
    ssnode.set_speed_scale(1.0)
    ssnode.play()
```

## 主なメソッド

* `set_ssab_resource(res: SSABResource)` / `get_ssab_resource() -> SSABResource`
* `set_animation(name: String)` / `get_animation() -> String`
* `set_autoplay(autoplay: bool)` / `is_autoplay() -> bool`: シーン開始時に自動再生するかどうか。
* **エディタ内プレビュー**: ノードを選択すると表示される **SpriteStudio** ボトムパネル（再生 / 一時停止 / 停止 / フレームスクラバ）でゲームを実行せずにプレビューできます。*(旧 `editor_playing` インスペクタトグルはこのパネルに置き換えられました。)*
* `play(start_frame: float = -1.0)`: 再生を開始します。`-1.0` を指定した場合は、現在のフレームまたは区間の先頭から再生します。
* `pause()`: 再生を一時停止します。
* `stop()`: 再生を停止します。
* `is_playing() -> bool` / `is_pausing() -> bool`
* `set_frame(frame: float)` / `get_frame() -> float` / `get_total_frames() -> int`
* `set_speed_scale(speed_scale: float)` / `get_speed_scale() -> float`
* `set_frame_rate(fps: int)` / `get_frame_rate() -> int`
* `set_animation_section(start: int, end: int)`: 再生するフレーム区間を限定します。
* `set_playback_direction(direction: int, style: int)`: 再生方向と再生スタイルを指定します。値の意味は下表を参照してください。
* `set_loop_count(count: int)` / `get_loop_count() -> int`: `-1` で無限ループ、`0` で1回だけ再生、`n` で `n` 回繰り返し。
* `set_frame_skip_enabled(enabled: bool)` / `is_frame_skip_enabled() -> bool` (デフォルト: `true`)
* `set_sub_frame_enabled(enabled: bool)` / `is_sub_frame_enabled() -> bool` (デフォルト: `false`)
* `set_cellmap_texture(cellmap_name: String, texture: Texture2D)` / `get_cellmap_texture(cellmap_name: String) -> Texture2D`

### `set_playback_direction` の引数

| 引数 | 値 | 意味 |
| --- | --- | --- |
| `direction` | `0` | 順再生 (Forward) |
| `direction` | `1` | 逆再生 (Backward) |
| `style` | `0` | 通常 / 片道 (Normal) |
| `style` | `1` | 往復再生 (PingPong) |

## シグナル

| シグナル | 引数 | 発行タイミング |
| --- | --- | --- |
| `animation_started` | `anim_name: String` | 再生が開始された時 |
| `animation_changed` | `anim_name: String` | アニメーションが切り替わった時 |
| `animation_finished` | `anim_name: String` | 再生が終了した時（非ループ時のみ） |
| `animation_looped` | `anim_name: String` | ループして先頭に戻った時 |
| `user_data` | `payload: Dictionary` | タイムライン上の「ユーザーデータ」キーに到達した時 |
| `signal_emitted` | `command: String, value: Dictionary` | タイムライン上の「シグナル」キーに到達した時 |

### `user_data` の `payload` フィールド

SpriteStudio 上でユーザーデータに設定した値が `Dictionary` として渡されます。**設定された項目のみ** キーが含まれます（未設定の項目はキーごと存在しません）。

| キー | 型 | 内容 |
| --- | --- | --- |
| `integer` | `int` | 整数値 |
| `point` | `Vector2` | 座標値 |
| `rect` | `Rect2` | 矩形値（`x`, `y`, `width`, `height`） |
| `string` | `String` | 文字列値 |

### `signal_emitted` の `value` フィールド

タイムライン上の「シグナル」に設定したパラメータが `Dictionary` として渡されます。パラメータ ID をキーに、各値（`bool` / `int` / `float` / `String` 等）が格納されます。`command` 引数にはシグナル名（`command_id`）が入ります。

> [!NOTE]
> 引数の正確な型・最新の取り得る値は実装 `ss_player/ss_player_node_2d.h` / `ss_player/ss_internal_player.cpp` を併せて参照してください。
