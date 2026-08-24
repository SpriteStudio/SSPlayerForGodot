# 🧩 SpriteStudioPlayer2D

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
* `set_animation_process_mode(mode: AnimationProcessMode)` / `get_animation_process_mode() -> AnimationProcessMode`: `ANIMATION_PROCESS_PHYSICS`（`0`）で Physics (`_physics_process`) 同期、`ANIMATION_PROCESS_IDLE`（`1`）で Idle (`_process`) 同期、`ANIMATION_PROCESS_MANUAL`（`2`）でノード自身による更新を停止します。
* `advance(delta: float)`: 再生を `delta` 秒ぶん進め、自動更新と同じように `frame_updated` を発行します。`ANIMATION_PROCESS_MANUAL` 向けの API です。他のモードで呼ぶと、ノード自身の更新に *加えて* アニメーションが進みます。
* **エディタ内プレビュー**: ノードを選択すると表示される **SpriteStudio** ボトムパネル（先頭から再生 / 現在位置から再生 / 停止、フレームスクラバ、ループと速度）で、ゲームを実行せずにプレビューできます。ショートカットは AnimationPlayer エディタと同じです（**D** 現在位置から再生 / **Shift+D** 先頭から再生 / **S** 停止）。*(旧 `editor_playing` インスペクタトグルはこのパネルに置き換えられました。)*
* `play(start_frame: float = -1.0)`: 再生を開始します。既定値の `-1.0` は、現在の再生ヘッド位置から続きを再生するのではなく、**区間の先頭に巻き戻します**（逆再生方向なら区間の末尾）。現在位置から再生したい場合は `get_frame()` を渡してください。
* `pause()`: 一時停止状態を **トグル** します（現在のフレームは保持）。もう一度呼ぶと再開します。`play()` は一時停止の再開ではなく、再生のやり直しです。
* `stop()`: 再生を停止します。再生ヘッドは **その場に留まる** ため、停止したフレームを表示し続けます。
* `is_playing() -> bool`: 再生中に `true`。**一時停止中も `true` です。** / `is_pausing() -> bool`: 一時停止中のみ `true`。両者を区別する必要がある場合は `is_pausing()` を見てください。
* `set_frame(frame: float)` / `get_frame() -> float` / `get_total_frames() -> int`
* `get_start_frame() -> int` / `get_end_frame() -> int`: 実際に再生される先頭 / 末尾フレーム、すなわち現在の再生区間です。`get_animation_section_start()` / `get_animation_section_end()` と同じ値を返します（`set_animation_section()` で狭めるまではアニメーション全体）。
* `set_speed_scale(speed_scale: float)` / `get_speed_scale() -> float`
* `set_frame_rate(fps: int)` / `get_frame_rate() -> int`
* `set_animation_section(start: int, end: int)`: 再生するフレーム区間を限定します。
* `set_animation_section_start(start: int)` / `get_animation_section_start() -> int` / `set_animation_section_end(end: int)` / `get_animation_section_end() -> int`: 片方の端点だけを移動します（もう一方は維持）。インスペクタの `animation_section_start` / `animation_section_end` プロパティの実体です。
* `set_playback_direction(direction: PlaybackDirection, style: PlaybackStyle)`: 再生方向と再生スタイルを指定します。値の意味は下表を参照してください。
* `get_playback_direction() -> PlaybackDirection` / `get_playback_style() -> PlaybackStyle`: セッターで指定した 2 つの値をそれぞれ読み出します。
* `set_loop_count(count: int)` / `get_loop_count() -> int`: `n` で `n` 回再生して停止（`1` なら1回のみ）。`-1` で無限ループ（`0` も無限ループの別名）。
* `set_frame_skip_enabled(enabled: bool)` / `is_frame_skip_enabled() -> bool` (デフォルト: `true`)
* `set_sub_frame_enabled(enabled: bool)` / `is_sub_frame_enabled() -> bool` (デフォルト: `false`)
* `set_cellmap_texture(cellmap_name: String, texture: Texture2D)` / `get_cellmap_texture(cellmap_name: String) -> Texture2D`
* `get_cellmap_names() -> PackedStringArray` / `get_cell_names(cellmap_name: String) -> PackedStringArray`: 割り当て済みの `SSABResource` から読んだ名前一覧（未割り当てなら空）。`set_part_cell_override()` に渡す名前を調べる用途です。まだプレーヤに載せていない `.ssab` を調べたい場合は [`SSABResource`](resource.md) 自身にも同じメソッドがあります。
* `set_play_audio(enabled: bool)` / `is_play_audio() -> bool` (デフォルト: `true`)、`set_audio_volume(volume: float)` / `get_audio_volume() -> float`、`set_audio_backend(backend: SpriteStudioAudioBackend)` / `get_audio_backend() -> SpriteStudioAudioBackend`: 内蔵のサウンド再生。詳細は後述の [サウンド](#audio) を参照してください。

### `set_playback_direction` の引数

| 引数 | 定数 | 値 | 意味 |
| --- | --- | --- | --- |
| `direction` | `PLAYBACK_DIRECTION_FORWARD` | `0` | 順再生 (Forward) |
| `direction` | `PLAYBACK_DIRECTION_BACKWARD` | `1` | 逆再生 (Backward) |
| `style` | `PLAYBACK_STYLE_NORMAL` | `0` | 通常 / 片道 (Normal) |
| `style` | `PLAYBACK_STYLE_PING_PONG` | `1` | 往復再生 (PingPong) |

## パーツの参照

* `get_part_names() -> PackedStringArray`: アセット（`.ssab`）に含まれる全パーツ名。パーツはアニメーションに依存しないため、同じアセット内のどのアニメーションでも同じ一覧になります。
* `get_part_index(part_name: String) -> int`: パーツ名をパーツインデックスへ解決します。存在しない場合は `-1`。
* `get_part_transform(part_name: String) -> Transform2D`: 現在のフレームでのパーツの `Transform2D`（プレイヤーノードのローカル空間。`flip_h` / `flip_v` / `offset` を含みます）。パーツが不明な場合は単位行列を返します。
* `is_part_hidden(part_name: String) -> bool`: 現在のフレームでそのパーツが非表示かどうか。

指定したパーツにノードを追従させる `SpriteStudioPartAttachment2D` については [スクリプト制御とイベント駆動 → パーツトラッキング](../workflow/usage_scripting.md) を参照してください。

## パーツオーバーライド

パーツ単位で、カラー / セル / 表示指定をキーフレームより優先して上書きします。各メソッドは成功時に `true`、パーツが不明な場合やランタイムが受け付けなかった場合に `false` を返します。詳細と注意点は [スクリプト制御とイベント → パーツオーバーライド](../workflow/usage_scripting.md) を参照してください。

* `set_part_color_override(part_name: String, color: Color, blend_op: ColorBlendOperation = COLOR_BLEND_MIX, priority: OverridePriority = OVERRIDE_PRIORITY_UNTIL_ANIMATION_CHANGE) -> bool`
* `set_part_color_override_corners(part_name: String, left_top: Color, right_top: Color, left_bottom: Color, right_bottom: Color, blend_op: ColorBlendOperation = COLOR_BLEND_MIX, priority: OverridePriority = OVERRIDE_PRIORITY_UNTIL_ANIMATION_CHANGE) -> bool`: 4 頂点それぞれに色を指定して、パーツ内をグラデーションにします。`set_part_color_override` と同じオーバーライド枠を共有するので、後から呼んだ方が有効になり、`clear_part_color_override` はどちらも解除します。
* `set_part_cell_override(part_name: String, cellmap_name: String, cell_name: String, priority: OverridePriority = OVERRIDE_PRIORITY_UNTIL_ANIMATION_CHANGE) -> bool`
* `set_part_visibility_override(part_name: String, force_hidden: bool, cascade: bool = false) -> bool`
* `clear_part_color_override(part_name: String) -> bool` / `clear_part_cell_override(part_name: String) -> bool` / `clear_part_visibility_override(part_name: String) -> bool`
* `clear_all_part_overrides() -> bool`
* 各メソッドには、パーツ名の解決を省略できるインデックス指定版 `*_by_index(part_index: int, ...)` があります（インデックスは `get_part_index()` で取得）。

### `blend_op` の値

| 定数 | 値 | 合成モード |
| --- | --- | --- |
| `COLOR_BLEND_MIX` | `0` | Mix（既定） |
| `COLOR_BLEND_MUL` | `1` | Mul（乗算） |
| `COLOR_BLEND_ADD` | `2` | Add（加算） |
| `COLOR_BLEND_SUB` | `3` | Sub（減算） |

### `priority` の値

| 定数 | 値 | 意味 |
| --- | --- | --- |
| `OVERRIDE_PRIORITY_NEXT_KEYFRAME` | `0` | アニメーションが当該アトリビュートを更新するまで適用 |
| `OVERRIDE_PRIORITY_UNTIL_ANIMATION_CHANGE` | `1` | 現在のアニメーション中は適用され、アニメーション変更で解除（既定） |
| `OVERRIDE_PRIORITY_PERMANENT` | `2` | 同じ `.ssab` を再生している間は適用（アニメーション変更をまたいで持続） |

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
| `signal_emitted` | `command: String, value: Dictionary, info: Dictionary` | タイムライン上の「シグナル」キーに到達した時 |
| `audio` | `payload: Dictionary` | タイムライン上の「オーディオ」キーに到達した時 |

### `user_data` の `payload` フィールド

SpriteStudio 上でユーザーデータに設定した値が `Dictionary` として渡されます。発生元を示す 3 つのキーは **常に** 含まれます。値の 4 キーは **設定された項目のみ** 含まれます（未設定の項目はキーごと存在しません。`0` も作者が意図し得る値であるため、既定値で埋めることはしません）。

| キー | 型 | 内容 |
| --- | --- | --- |
| `part_index` | `int` | キーが置かれているパーツのインデックス |
| `part_name` | `String` | そのパート名 |
| `frame_no` | `int` | キーが置かれているフレーム。1 ティックで複数フレームをまたぐことがあるため、検出されたフレームとは限りません |
| `integer` | `int` | 整数値 |
| `point` | `Vector2` | 座標値 |
| `rect` | `Rect2` | 矩形値（`x`, `y`, `width`, `height`） |
| `string` | `String` | 文字列値 |

### `signal_emitted` の `value` / `info` フィールド

タイムライン上の「シグナル」に設定したパラメータが `value` として渡されます。パラメータ ID をキーに、各値（`bool` / `int` / `float` / `String` 等）が格納されます。`command` 引数にはシグナル名（`command_id`）が入ります。

発生元は `value` に混ぜず、**別の `info` 辞書**として渡されます。`value` のキーは作者が自由に決めるものであり、固定キーを混ぜると衝突しうるためです。

| `info` のキー | 型 | 内容 |
| --- | --- | --- |
| `part_index` | `int` | キーが置かれているパーツのインデックス |
| `part_name` | `String` | そのパート名 |
| `frame_no` | `int` | キーが置かれているフレーム |

### `audio` の `payload` フィールド

タイムライン上のオーディオキーに設定された情報が `Dictionary` として渡されます。このシグナルは **観測用のチャンネル**で、再生方向を問わず、エディタ上でも、内蔵再生 (`play_audio`) のオン / オフに関わらず発火します。サウンドに反応したい場合や、再生そのものを置き換えたい場合に接続してください（後述の [サウンド](#audio) を参照）。

| キー | 型 | 内容 |
| --- | --- | --- |
| `part_index` | `int` | 発火したパーツのインデックス |
| `part_name` | `String` | そのパート名 |
| `frame_no` | `int` | キーが置かれているフレーム |
| `sound_list_name_hash` | `int` | サウンドリスト名のハッシュ |
| `sound_name_hash` | `int` | サウンド名のハッシュ |
| `sound_name` | `String` | サウンド名（設定時のみ） |
| `loop_num` | `int` | 再生回数（`1` で 1 回。SpriteStudio に無限ループのサウンドはありません） |

> [!NOTE]
> 引数の正確な型・最新の取り得る値は実装 `ss_player/ss_player_node_2d.h` / `ss_player/ss_internal_player.cpp` を併せて参照してください。

## サウンド (Audio)

サウンドパートは何もしなくても Godot 上で鳴ります。ノードは `AudioStreamPlayer` のボイスをプール管理し、**順再生中に**再生ヘッドがオーディオキーを通過するたびに 1 本開始します。挙動の詳細（撃ちっぱなし、シーク時に再同期しない、再発火で重なる）は [サウンド再生](../workflow/audio.md) にまとめてあります。ここでは API だけを示します。

| メンバー | 型 | 既定値 | 説明 |
| --- | --- | --- | --- |
| `play_audio` | `bool` | `true` | 内蔵プレイヤーが音を鳴らすかどうか。`set_play_audio(false)` は再生中の内蔵ボイスも停止します |
| `audio_volume` | `float` | `1.0` | 内蔵ボイスのリニア音量 (`[0, 1]`)。`audio_backend` を割り当てている間は無視されます |
| `audio_backend` | `SpriteStudioAudioBackend` | *(なし)* | 内蔵プレイヤーを完全に置き換えます |

### `SpriteStudioAudioBackend`

オーバーライド可能なメソッドを 1 つだけ持つ `Resource` のサブクラスです。`audio_backend` に割り当てると **内蔵再生は抑制されます**（`audio_volume` も含む）。ボイスのライフサイクルと再生回数の管理はバックエンド側の責務になります。

* `play_audio(payload: Dictionary, ssab: SSABResource, player: Node) -> void`: オーディオイベントごとに 1 回呼ばれます。`payload` は `audio` シグナルと同じ内容です。既定の実装は何もしません。

```gdscript
extends SpriteStudioAudioBackend

func play_audio(payload: Dictionary, ssab: SSABResource, player: Node) -> void:
    var info := ssab.get_sound_info(payload["sound_list_name_hash"], payload["sound_name_hash"])
    if not info.is_empty():
        MyMiddleware.play(info["path"], payload["loop_num"])
```

`payload` からサウンドを解決するには [`SSABResource`](resource.md#ssabresource) の `get_sound_stream()` / `get_sound_info()` を使います。

## AnimationPlayer から駆動する

`frame` プロパティはアニメート可能なので、`AnimationPlayer` のタイムライン（音・メソッド呼び出し・他ノードなど他トラック）と同期させて SpriteStudio アニメをスクラブできます。

1. `SpriteStudioPlayer2D` に通常どおり `Ssab`（SSAB リソース）を割り当て、`Animation` を選択。
2. `AnimationPlayer` で、ノードの `frame` プロパティを対象に **プロパティトラック** を追加。
3. `frame` を時間に沿ってキーフレーム（例：尺に合わせて `0` → 最終フレーム）。`frame` は float なので補間されます。
4. `AnimationPlayer` を再生。

> [!IMPORTANT]
> `AnimationPlayer` が `frame` を駆動している間は、ノードを**自走させない**でください（`Autoplay` をオフにし、`play()` も呼ばない）。さもないとノード自身の再生とキーフレームの `frame` が毎フレーム競合します。

これ以上の準備は不要です。キー値は `AnimationPlayer` のアニメーション側に保存され（ノードの `frame` はシーンに保存されません）、同じトラックがランタイムでも再生を駆動します。
