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
* `set_offset(offset: Vector2)` / `get_offset() -> Vector2`: Node2D の原点を動かさずに描画位置だけをずらします。
* `set_flip_h(flip: bool)` / `is_flipped_h() -> bool`: 水平反転。
* `set_flip_v(flip: bool)` / `is_flipped_v() -> bool`: 垂直反転。
* `set_animation_process_mode(mode: int)` / `get_animation_process_mode() -> int`: `0` で Physics (`_physics_process`) 同期、`1` で Idle (`_process`) 同期。
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
* `set_loop_count(count: int)` / `get_loop_count() -> int`: `n` で `n` 回再生して停止（`1` なら1回のみ）。`-1` で無限ループ（`0` も無限ループの別名）。
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

## パーツの参照

* `get_part_names() -> PackedStringArray`: アセット（`.ssab`）に含まれる全パーツ名。パーツはアニメーションに依存しないため、同じアセット内のどのアニメーションでも同じ一覧になります。
* `get_part_index(part_name: String) -> int`: パーツ名をパーツインデックスへ解決します。存在しない場合は `-1`。
* `get_part_transform(part_name: String) -> Transform2D`: 現在のフレームでのパーツの `Transform2D`（プレイヤーノードのローカル空間。`flip_h` / `flip_v` / `offset` を含みます）。パーツが不明な場合は単位行列を返します。
* `is_part_hidden(part_name: String) -> bool`: 現在のフレームでそのパーツが非表示かどうか。

指定したパーツにノードを追従させる `SpriteStudioPartAttachment2D` については [スクリプト制御とイベント駆動 → パーツトラッキング](../workflow/usage_scripting.md) を参照してください。

## パーツオーバーライド

パーツ単位で、カラー / セル / 表示指定をキーフレームより優先して上書きします。各メソッドは成功時に `true`、パーツが不明な場合やランタイムが受け付けなかった場合に `false` を返します。詳細と注意点は [スクリプト制御とイベント → パーツオーバーライド](../workflow/usage_scripting.md) を参照してください。

* `set_part_color_override(part_name: String, color: Color, blend_op: int = 0, priority: int = 1) -> bool`
* `set_part_cell_override(part_name: String, cellmap_name: String, cell_name: String, priority: int = 1) -> bool`
* `set_part_visibility_override(part_name: String, force_hidden: bool, cascade: bool = false) -> bool`
* `clear_part_color_override(part_name: String) -> bool` / `clear_part_cell_override(part_name: String) -> bool` / `clear_part_visibility_override(part_name: String) -> bool`
* `clear_all_part_overrides() -> bool`
* 各メソッドには、パーツ名の解決を省略できるインデックス指定版 `*_by_index(part_index: int, ...)` があります（インデックスは `get_part_index()` で取得）。

### `blend_op` の値

| 値 | 合成モード |
| --- | --- |
| `0` | Mix（既定） |
| `1` | Mul（乗算） |
| `2` | Add（加算） |
| `3` | Sub（減算） |

### `priority` の値

| 値 | 優先モード | 意味 |
| --- | --- | --- |
| `0` | OverwriteOnNextKeyframe | アニメーションが当該アトリビュートを更新するまで適用 |
| `1` | HoldUntilNextAnimation（既定） | 現在のアニメーション中は適用され、アニメーション変更で解除 |
| `2` | Permanent | 同じ `.ssab` を再生している間は適用（アニメーション変更をまたいで持続） |

> [!NOTE]
> 表示指定（`set_part_visibility_override`）に `priority` はありません。常にキーフレームに勝ち、アニメーションを設定し直すと必ずクリアされます。

## シグナル

| シグナル | 引数 | 発行タイミング |
| --- | --- | --- |
| `animation_started` | `anim_name: String` | 再生が開始された時 |
| `animation_changed` | `anim_name: String` | アニメーションが切り替わった時 |
| `animation_finished` | `anim_name: String` | 指定したループ回数をすべて再生し終えた時。無限ループでは発火しない |
| `animation_looped` | `anim_name: String` | 1周して先頭に戻った時。最終周では発火せず `animation_finished` になる |
| `frame_updated` | `frame_no: float` | そのフレームのパーツ姿勢が確定した直後（プレーヤの更新直後、描画の前）。発火するプロセスは `animation_process_mode` に従う |
| `user_data` | `payload: Dictionary` | タイムライン上の「ユーザーデータ」キーに到達した時 |
| `signal_emitted` | `command: String, value: Dictionary` | タイムライン上の「シグナル」キーに到達した時 |
| `audio` | `payload: Dictionary` | タイムライン上の「オーディオ」キーに到達した時 |

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

### `audio` の `payload` フィールド

タイムライン上のオーディオキーに設定された情報が `Dictionary` として渡されます。再生はプレーヤ側では行わないため、ゲーム側で `AudioStreamPlayer` 等に橋渡ししてください。

| キー | 型 | 内容 |
| --- | --- | --- |
| `part_index` | `int` | 発火したパーツのインデックス |
| `sound_list_name_hash` | `int` | サウンドリスト名のハッシュ |
| `sound_name_hash` | `int` | サウンド名のハッシュ |
| `sound_name` | `String` | サウンド名（設定時のみ） |
| `loop_num` | `int` | ループ回数 |

> [!NOTE]
> 引数の正確な型・最新の取り得る値は実装 `ss_player/ss_player_node_2d.h` / `ss_player/ss_internal_player.cpp` を併せて参照してください。

## AnimationPlayer から駆動する

`frame` プロパティはアニメート可能なので、`AnimationPlayer` のタイムライン（音・メソッド呼び出し・他ノードなど他トラック）と同期させて SpriteStudio アニメをスクラブできます。

1. `SpriteStudioPlayer2D` に通常どおり `Ssab`（SSAB リソース）を割り当て、`Animation` を選択。
2. `AnimationPlayer` で、ノードの `frame` プロパティを対象に **プロパティトラック** を追加。
3. `frame` を時間に沿ってキーフレーム（例：尺に合わせて `0` → 最終フレーム）。`frame` は float なので補間されます。
4. `AnimationPlayer` を再生。

> [!IMPORTANT]
> `AnimationPlayer` が `frame` を駆動している間は、ノードを**自走させない**でください（`Autoplay` をオフにし、`play()` も呼ばない）。さもないとノード自身の再生とキーフレームの `frame` が毎フレーム競合します。

これ以上の準備は不要です。キー値は `AnimationPlayer` のアニメーション側に保存され（ノードの `frame` はシーンに保存されません）、同じトラックがランタイムでも再生を駆動します。
