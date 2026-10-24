# 🧩 SpriteStudioPlayer2D

`Node2D` を継承する再生用ノードです。リソースとアニメーションを指定して再生を行います。

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
* `set_animation(name: String)` / `get_current_animation() -> String`
* `set_autoplay(autoplay: bool)` / `is_autoplay() -> bool`: シーン開始時に自動再生するかどうか。**既定は `true`** です。シーンに配置したノードのために `play()` を呼ぶコードは存在しないため、そのまま再生されます。自分で再生を制御したい場合や、先頭フレームをポーズとして保持したい場合はオフにしてください。
* `set_offset(offset: Vector2)` / `get_offset() -> Vector2`: Node2D の原点を動かさずに描画位置だけをずらします。
* `set_flip_h(flip: bool)` / `is_flipped_h() -> bool`: 水平反転。
* `set_flip_v(flip: bool)` / `is_flipped_v() -> bool`: 垂直反転。
* `set_animation_process_mode(mode: AnimationProcessMode)` / `get_animation_process_mode() -> AnimationProcessMode`: `ANIMATION_PROCESS_PHYSICS`（`0`）で Physics (`_physics_process`) 同期、`ANIMATION_PROCESS_IDLE`（`1`）で Idle (`_process`) 同期、`ANIMATION_PROCESS_MANUAL`（`2`）でノード自身による更新を停止します。
* `advance(delta: float)`: 再生を `delta` 秒ぶん進め、自動更新と同じように `frame_updated` を発行します。`ANIMATION_PROCESS_MANUAL` 向けの API です。他のモードで呼ぶと、ノード自身の更新に *加えて* アニメーションが進みます。
* **エディタ内プレビュー**: ノードを選択すると表示される **SpriteStudio** ボトムパネル（先頭から再生 / 現在位置から再生 / 停止、フレームスクラバ、ループと速度）で、ゲームを実行せずにプレビューできます。ショートカットは AnimationPlayer エディタと同じです（**D** 現在位置から再生 / **Shift+D** 先頭から再生 / **S** 停止）。
* `play(start_frame: float = -1.0)`: 再生を開始します。既定値の `-1.0` は、現在の再生ヘッド位置から続きを再生するのではなく、**区間の先頭に巻き戻します**（逆再生方向なら区間の末尾）。現在位置から再生したい場合は `get_frame_no()` を渡してください。
* `pause()`: 現在のフレームを保持したまま、その場で再生を止めます。**冪等**です（2 回呼んでも一時停止のままで、再開はしません）。
* `resume()`: 保持を解除し、同じフレームから再開します。**冪等**で、一時停止ではなく停止している場合は何もしません（停止からの開始は `play()` で、こちらは巻き戻ります）。
* `stop()`: 再生を停止します。再生ヘッドは **その場に留まる** ため、停止したフレームを表示し続けます。
* `is_playing() -> bool`: 再生中に `true`。**一時停止中も `true` です**（一時停止は「保持」であって「停止」ではありません）。 / `is_pausing() -> bool`: 保持中のみ `true`。一度も再生していないアニメーションと `stop()` 後は、どちらも `false` です。
* `is_finished() -> bool`: 設定したループを全て再生し終えたかどうか。パルスではなく**状態**で、完了時に latch され次の `play()` またはアニメ切り替えまで保持されます。`animation_finished` が飛んだ時点で接続していなかった側でも「終わったか」を問えます。`loop_count` が無限のときは決して `true` にならず、**`stop()` でも立ちません** —— 命令された停止は完了ではないからで、そこが `not is_playing()` との違いです。
* `just_looped() -> bool`: 直近の update でループ境界を跨いだかどうか。**パルス**であって状態ではありません — ランタイムが毎 update の冒頭でクリアするので、跨いだ tick の内側でのみ `true` です（だから `is_looped` ではありません。1 tick 遅れて読むと `false` になります）。ポーリングしたくなければ `animation_looped` シグナルが同じエッジを配ります。
* `get_animation_names() -> PackedStringArray`: 割り当てた [SSABResource] のアニメーション名一覧。`current_animation` プロパティが選ぶのと同じリストです。
* `is_playing_forward() -> bool`: 再生ヘッドが**実際に**進んでいる向きです。設定した向きを返す `get_playback_direction()` とは別物で、ピンポン再生の戻りの脚では `get_playback_direction()` が `PLAYBACK_DIRECTION_FORWARD` のままでもこちらは `false` になります（速度 0 以下は逆再生ではなく停止なので、これも反転しません）。**音声のゲートに使ってください** — SpriteStudio に逆再生音声は無いため、後ろ向きの脚で跨いだサウンドキーは鳴らすべきものではありません。`play_audio` が有効なときにプレイヤー自身が掛けている判定と同じものです。何も再生していないときは `true`（前進が既定の状態）。
* `set_frame_no(frame: float)` / `get_frame_no() -> float` / `get_total_frames() -> int`: `get_total_frames()` は現在の再生区間のフレーム数（`get_end_frame() - get_start_frame() + 1`）です。`set_animation_section()` で区間を狭めると、アニメーション全体ではなく区間の長さを返します。
* `get_start_frame() -> int` / `get_end_frame() -> int`: 実際に再生される先頭 / 末尾フレーム、すなわち現在の再生区間です。`get_animation_section_start()` / `get_animation_section_end()` と同じ値を返します（`set_animation_section()` で狭めるまではアニメーション全体）。
* `set_speed_scale(speed_scale: float)` / `get_speed_scale() -> float`
* `set_frame_rate(fps: int)` / `get_frame_rate() -> int`
* `set_animation_section(start: int, end: int) -> bool`: 再生するフレーム区間を `start` から `end` までに限定します（両端を含みます）。`start == end` ならその 1 フレームを再生し、最後のフレームでも同じです。アニメーションの範囲外の値（`-1` など）を渡すと、その側はアニメーションの端になります。両端が範囲内で `end` が `start` より前なら `false` を返し、区間と再生ヘッドはそのままにします。出力パネルには、その組を一度だけ出します。
* `set_animation_section_start(start: int)` / `get_animation_section_start() -> int` / `set_animation_section_end(end: int)` / `get_animation_section_end() -> int`: 片方の端点だけを移動します（もう一方は維持）。インスペクターの `animation_section_start` / `animation_section_end` プロパティの実体です。どちらも `set_animation_section()` を通るので、今の終わりより後ろへ始まりを、今の始まりより前へ終わりを動かすと受け付けません。先にもう一方の端点を動かすか、`set_animation_section()` で両方を指定してください。
* `set_playback_direction(direction: PlaybackDirection, style: PlaybackStyle)`: 再生方向と再生スタイルを指定します。値の意味は下表を参照してください。
* `get_playback_direction() -> PlaybackDirection` / `get_playback_style() -> PlaybackStyle`: セッターで指定した 2 つの値をそれぞれ読み出します。
* `frame_rate` / `playback_direction` / `playback_style` は、プロパティとしても読み書きできます（`player.playback_direction = ...`）。実行時専用で、インスペクターには出ず、シーンにも保存されません。
* `set_loop_count(count: int)` / `get_loop_count() -> int`: `n` で `n` 回再生して停止（`1` なら1回のみ）。`-1` で無限ループ（`0` も無限ループの別名）。
* `set_frame_skip_enabled(enabled: bool)` / `is_frame_skip_enabled() -> bool` (デフォルト: `true`)
* `set_sub_frame_enabled(enabled: bool)` / `is_sub_frame_enabled() -> bool` (デフォルト: `false`)
* `set_cellmap_texture(cellmap_name: String, texture: Texture2D)` / `get_cellmap_texture(cellmap_name: String) -> Texture2D`: インスペクターの **CellMap Overrides** グループに並ぶ `cellmaps/<セルマップ名>` プロパティと同じものです。
* `get_cellmap_names() -> PackedStringArray` / `get_cell_names(cellmap_name: String) -> PackedStringArray`: 割り当て済みの `SSABResource` から読んだ名前一覧（未割り当てなら空）。`set_part_cell_override()` に渡す名前を調べる用途です。まだプレーヤに載せていない `.ssab` を調べたい場合は [`SSABResource`](resource.md) 自身にも同じメソッドがあります。
* `set_play_audio(enabled: bool)` / `is_play_audio() -> bool` (デフォルト: `true`)、`set_audio_volume(volume: float)` / `get_audio_volume() -> float`、`set_audio_bus(bus: StringName)` / `get_audio_bus() -> StringName` (デフォルト: `&"Master"`)、`set_audio_backend(backend: SpriteStudioAudioBackend)` / `get_audio_backend() -> SpriteStudioAudioBackend`: 内蔵のサウンド再生。詳細は後述の [サウンド](#サウンド-audio) を参照してください。

### `set_playback_direction` の引数

| 引数 | 定数 | 値 | 意味 |
| --- | --- | --- | --- |
| `direction` | `PLAYBACK_DIRECTION_FORWARD` | `0` | 順再生 (Forward) |
| `direction` | `PLAYBACK_DIRECTION_BACKWARD` | `1` | 逆再生 (Backward) |
| `style` | `PLAYBACK_STYLE_NORMAL` | `0` | 通常 / 片道 (Normal) |
| `style` | `PLAYBACK_STYLE_PING_PONG` | `1` | 往復再生 (PingPong) |

## パーツの参照

* `get_part_names() -> PackedStringArray`: アセット（`.ssab`）に含まれる全パーツ名。パーツはアニメーションに依存しないため、同じアセット内のどのアニメーションでも同じ一覧になります。
* `find_part_index(part_name: String) -> int`: パーツ名をパーツインデックスへ解決します。存在しない場合は `-1`。
* `get_part_transform(part_name: String) -> Transform2D`: 現在のフレームでのパーツの `Transform2D`（プレイヤーノードのローカル空間。`flip_h` / `flip_v` / `offset` を含みます）。パーツが不明な場合は単位行列を返します。
* `is_part_hidden(part_name: String) -> bool`: 現在のフレームでそのパーツが非表示かどうか。
* `is_part_skinned_mesh(part_name: String) -> bool`: そのパーツがスキンメッシュかどうか（パーツが不明な場合は `false`）。スキンメッシュパーツは追従先に向かないため、`SpriteStudioPartAttachment2D` は対象から除外します。

指定したパーツにノードを追従させる `SpriteStudioPartAttachment2D` については [スクリプト制御とイベント駆動 → パーツトラッキング](../workflow/usage_scripting.md) を参照してください。

## オーサリング時のキャンバス

* `get_canvas_size() -> Vector2`: 現在のアニメーションがオーサリングされたキャンバスのサイズ（プレイヤーノードの単位）。何も設定されていない場合は `Vector2.ZERO`。
* `get_canvas_rect() -> Rect2`: そのキャンバスをアニメーションの **pivot** でノードの原点に対して配置した矩形（プレイヤーノードのローカル空間。`get_part_transform` と同じく `flip_h` / `flip_v` / `offset` を含みます）。

pivot はキャンバス内のどこに原点があるかを表します。地面に立った状態でオーサリングされたキャラクターは pivot がキャンバスの下辺にあるため、`get_canvas_rect()` は原点より上だけを占める矩形を返します。

キャンバスはアセット単位ではなく **アニメーション単位** でオーサリングされます。`Ringo` サンプルでは大半が `800 x 600` なのに対し `dead` は `1300 x 600` です。キャッシュする場合は `animation_changed` のたびに読み直してください。

これは作者が絵を構成した枠であって、実際の描画を測った外接矩形ではありません（パーツはキャンバスの外にも描けます）。読み出しのコストはゼロですが、より小さい矩形を得るには読み込み時にアニメーションの全フレームを進めて測る必要があります。

この矩形で画面外のプレイヤーを止める方法は [パフォーマンスチューニング → 画面外のプレイヤーを止める](../workflow/tips.md#画面外のプレイヤーを止める) を参照してください。

## パーツオーバーライド

パーツ単位で、カラー / セル / 表示指定をキーフレームより優先して上書きします。各メソッドは成功時に `true`、パーツが不明な場合やランタイムが受け付けなかった場合に `false` を返します。詳細と注意点は [スクリプト制御とイベント → パーツオーバーライド](../workflow/usage_scripting.md) を参照してください。

* `set_part_color_override(part_name: String, color: Color, blend_op: ColorBlendOperation = COLOR_BLEND_MIX, priority: OverridePriority = OVERRIDE_PRIORITY_HOLD_UNTIL_NEXT_ANIMATION) -> bool`
* `set_part_color_override_corners(part_name: String, corners: PackedColorArray, blend_op: ColorBlendOperation = COLOR_BLEND_MIX, priority: OverridePriority = OVERRIDE_PRIORITY_HOLD_UNTIL_NEXT_ANIMATION) -> bool`: 4 頂点それぞれに色を指定して、パーツ内をグラデーションにします。`corners` には左上・右上・左下・右下の順にちょうど 4 色を入れます。`set_part_color_override` と同じオーバーライド枠を共有するので、後から呼んだ方が有効になり、`clear_part_color_override` はどちらも解除します。
* `set_part_cell_override(part_name: String, cellmap_name: String, cell_name: String, priority: OverridePriority = OVERRIDE_PRIORITY_HOLD_UNTIL_NEXT_ANIMATION) -> bool`
* `set_part_visibility_override(part_name: String, force_hidden: bool, cascade: bool = false) -> bool`
* `clear_part_color_override(part_name: String) -> bool` / `clear_part_cell_override(part_name: String) -> bool` / `clear_part_visibility_override(part_name: String) -> bool`
* `clear_all_part_overrides() -> bool`
* 各メソッドには、パーツ名の解決を省略できるインデックス指定版 `*_by_index(part_index: int, ...)` があります（インデックスは `find_part_index()` で取得）。

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
| `OVERRIDE_PRIORITY_OVERWRITE_ON_NEXT_KEYFRAME` | `0` | アニメーションが当該アトリビュートを更新するまで適用 |
| `OVERRIDE_PRIORITY_HOLD_UNTIL_NEXT_ANIMATION` | `1` | 現在のアニメーション中は適用され、アニメーション変更で解除（既定） |
| `OVERRIDE_PRIORITY_PERMANENT` | `2` | 同じ `.ssab` を再生している間は適用（アニメーション変更をまたいで持続） |

> [!NOTE]
> 表示指定（`set_part_visibility_override`）に `priority` はありません。常にキーフレームに勝ち、アニメーションを設定し直すと必ずクリアされます。

## モーションブレンド { #motion-blending }

割り当てた `.ssab` の複数のアニメーションを、トラックに積んで同時に再生します。track 0 は `set_animation()` で選んだアニメーションで、その上のトラックが重なります。組み合わせ方は [モーションブレンド](../workflow/motion_blending.md) を参照してください。

* `set_track_animation(track: int, anim_name: String) -> bool`: `track` に `anim_name` を載せ、それまで再生していたものと置き換えます。トラックは必要に応じて作られます。track 0 ではクロスフェードと同じく現在のアニメーションを切り替え、その上のトラックはそのまま残します。
* `get_track_count() -> int`
* `clear_track(track: int) -> bool`: トラックを空にします。track 0 は空にできないので、alpha を 0 までフェードしてください。
* `set_track_alpha(track: int, alpha: float) -> bool` / `get_track_alpha(track: int) -> float`: そのトラックを下の結果にどれだけ重ねるか。0〜1 に制限しません。
* `set_track_blend(track: int, blend: TrackBlend) -> bool` / `get_track_blend(track: int) -> TrackBlend`
* `set_track_sync(track: int, sync: TrackSync) -> bool` / `get_track_sync(track: int) -> TrackSync`: そのトラックに **次に** 載せるアニメーションをどこから始めるか。載せる前に設定してください。
* `set_track_frame(track: int, frame_no: float) -> bool` / `get_track_frame(track: int) -> float`
* `set_track_speed(track: int, speed: float) -> bool` / `get_track_speed(track: int) -> float`: トラックの速度。これと下の 3 つの設定はトラックのもので、そのトラックに載るすべてのアニメーション（クロスフェードの両側を含む）に効きます。track 0 の速度は `speed_scale` で、同じ値を書き換えます。
* `set_track_frame_rate(track: int, fps: int) -> bool` / `get_track_frame_rate(track: int) -> int`: track 0 では `frame_rate` です。
* `set_track_frame_skip_enabled(track: int, enabled: bool) -> bool` / `is_track_frame_skip_enabled(track: int) -> bool`: track 0 では `frame_skip_enabled` です。
* `set_track_playback_direction(track: int, direction: PlaybackDirection, style: PlaybackStyle) -> bool` / `get_track_playback_direction(track: int) -> PlaybackDirection` / `get_track_playback_style(track: int) -> PlaybackStyle`: track 0 では `set_playback_direction()` です。
* `set_track_loop_count(track: int, count: int) -> bool` / `get_track_loop_count(track: int) -> int`: トラックに載っているアニメーションのループ回数。アニメーションは無限ループ（`-1`）で載り、何も載っていないトラックは `0` を返します。track 0 では `loop_count` で、ノードは track 0 が切り替わるたびにこの回数をかけ直します。
* `is_track_finished(track: int) -> bool` / `track_just_looped(track: int) -> bool`: track 0 では `is_finished()` / `just_looped()` です。終了したトラックは最後のフレームで止まり、混ざり続けます。どちらもシグナルは送られません。
* `set_track_outgoing_event_threshold(track: int, threshold: float) -> bool` / `get_track_outgoing_event_threshold(track: int) -> float`: トラックのクロスフェードで出ていく側は、その割合 `1 - progress` がこの値以上のあいだイベントを発火します。初期値は `INF` で、フェードの開始と同時に発火しなくなります。
* `crossfade(track: int, anim_name: String, duration: float) -> bool`: トラックが再生しているものから `anim_name` へ `duration` **秒** かけてフェードします。`anim_name` は呼んだ時点でトラックを引き継ぎます。track 0 ではその時点から現在のアニメーションになり、`animation_changed` が送られます。空のトラックではすぐに再生を始めます。
* `fade_track(track: int, target_alpha: float, duration: float) -> bool`: トラックの alpha を `duration` 秒かけて `target_alpha` にします。
* `get_track_fade_progress(track: int) -> float`: フェード中は 0〜1、フェードしていないときは **-1**。
* `cancel_track_fade(track: int) -> bool`: フェードを止めます。alpha と weight はその時点の値のままです。
* `get_track_main_source(track: int) -> int`: そのトラックのアニメーションを持つソースのハンドル。無いときは **-1**。
* `add_track_source(track: int, anim_name: String, weight: float) -> int`: トラックに別のアニメーションを足し、そのハンドルを返します（失敗したときは **-1**）。
* `set_source_weight(source: int, weight: float) -> bool` / `get_source_weight(source: int) -> float`: トラックの中でのソースの比率。絶対値ではなく比率です。
* `set_source_frame(source: int, frame_no: float) -> bool` / `get_source_frame(source: int) -> float`
* `set_source_event_threshold(source: int, threshold: float) -> bool` / `get_source_event_threshold(source: int) -> float`: ソースがイベントを発火する weight の下限。初期値は `INF` なので、既定では track 0 の主ソースだけが発火します。

設定のメソッドは、アニメーションが設定されていないとき、アニメーション名が `.ssab` に無いとき、ランタイムが受け付けなかったときに `false` を返します。存在しないトラックやソースの読み出しは `0` を返します（先に `get_track_count()` で確かめてください）。ただし `0` が正しい値になりうる 2 つは別で、`get_track_fade_progress()` は `-1` を返し、ソースのハンドルは `-1` です。

別のアニメーションへの `set_animation()` は、トラックの設定も含めてスタックを track 0 だけに戻します。`play()` は track 0 だけを先頭へ戻し、`pause()`・`resume()`・`stop()` はすべてのトラックとフェードに効きます。track 0 が終わっても、重ねたトラックは止まりません。`is_playing()`・`is_pausing()`・`is_finished()` と `animation_*` シグナルは track 0 の状態を表します。

### `TrackBlend` の値

| 定数 | 値 | 意味 |
| --- | --- | --- |
| `TRACK_BLEND_MIX` | `0` | alpha に応じてトラックの姿勢へ近づける（既定） |
| `TRACK_BLEND_ADDITIVE` | `1` | セットアップポーズからの差分を足す |

### `TrackSync` の値

| 定数 | 値 | 意味 |
| --- | --- | --- |
| `TRACK_SYNC_FREE` | `0` | 載せたアニメーションは再生区間の先頭から始まる（既定） |
| `TRACK_SYNC_START` | `1` | track 0 のアニメーションと同じ位相から始まる |

## シグナル

| シグナル | 引数 | 発行タイミング |
| --- | --- | --- |
| `animation_started` | `anim_name: String` | 再生が開始された時 |
| `animation_changed` | `anim_name: String` | アニメーションが切り替わった時（`set_animation()`、または track 0 への `crossfade()` / `set_track_animation()`） |
| `animation_finished` | `anim_name: String` | 指定したループ回数をすべて再生し終えた時。無限ループでは発火しない |
| `animation_looped` | `anim_name: String` | 1周して先頭に戻った時。最終周では発火せず `animation_finished` になる |
| `frame_updated` | `frame_no: float` | そのフレームのパーツ姿勢が確定した直後（プレーヤの更新直後、描画の前）。発火するプロセスは `animation_process_mode` に従う |
| `user_data` | `payload: Dictionary` | タイムライン上の「ユーザーデータ」キーに到達した時 |
| `signal_emitted` | `command: String, value: Dictionary, info: Dictionary` | タイムライン上の「シグナル」キーに到達した時 |
| `audio` | `payload: Dictionary` | タイムライン上の「オーディオ」キーに到達した時 |

### `user_data` の `payload` フィールド

SpriteStudio 上でユーザーデータに設定した値が `Dictionary` として渡されます。発生元を示す 4 つのキーは **常に** 含まれます。値の 4 キーは **設定された項目のみ** 含まれます（未設定の項目はキーごと存在しません。`0` も作者が意図し得る値であるため、既定値で埋めることはしません）。

| キー | 型 | 内容 |
| --- | --- | --- |
| `part_index` | `int` | キーが置かれているパーツのインデックス |
| `part_name` | `String` | そのパート名 |
| `frame_no` | `int` | キーが置かれているフレーム。1 ティックで複数フレームをまたぐことがあるため、検出されたフレームとは限りません |
| `source` | `int` | 発火したソースのハンドル（[モーションブレンド](#motion-blending) を参照） |
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
| `source` | `int` | 発火したソースのハンドル |

### `audio` の `payload` フィールド

タイムライン上のオーディオキーに設定された情報が `Dictionary` として渡されます。このシグナルは **観測用のチャンネル**で、再生方向を問わず、エディタ上でも、内蔵再生 (`play_audio`) のオン / オフに関わらず発火します。サウンドに反応したい場合や、再生そのものを置き換えたい場合に接続してください（後述の [サウンド](#サウンド-audio) を参照）。

| キー | 型 | 内容 |
| --- | --- | --- |
| `part_index` | `int` | 発火したパーツのインデックス |
| `part_name` | `String` | そのパート名 |
| `frame_no` | `int` | キーが置かれているフレーム |
| `source` | `int` | 発火したソースのハンドル |
| `sound_list_name_hash` | `int` | サウンドリスト名のハッシュ |
| `sound_name_hash` | `int` | サウンド名のハッシュ |
| `sound_name` | `String` | サウンド名（設定時のみ） |
| `loop_num` | `int` | 再生回数（`1` で 1 回。SpriteStudio に無限ループのサウンドはありません） |

> [!NOTE]
> 各シグナルの引数の型は、エディタ内蔵のクラスリファレンス（**ヘルプ → ヘルプを検索** で `SpriteStudioPlayer2D`）でも確認できます。

## サウンド (Audio)

サウンドパートは何もしなくても Godot 上で鳴ります。ノードは `AudioStreamPlayer` のボイスをプール管理し、**順再生中に**再生ヘッドがオーディオキーを通過するたびに 1 本開始します。挙動の詳細（撃ちっぱなし、シーク時に再同期しない、再発火で重なる）は [サウンド再生](../workflow/audio.md) にまとめてあります。ここでは API だけを示します。

| メンバー | 型 | 既定値 | 説明 |
| --- | --- | --- | --- |
| `play_audio` | `bool` | `true` | 内蔵プレイヤーが音を鳴らすかどうか。`set_play_audio(false)` は再生中の内蔵ボイスも停止します |
| `audio_volume` | `float` | `1.0` | 内蔵ボイスのリニア音量 (`[0, 1]`)。`audio_backend` を割り当てている間は無視されます |
| `audio_bus` | `StringName` | `&"Master"` | 内蔵ボイスを流すバス。`AudioStreamPlayer.bus` と同じく、バスレイアウトに無い名前は `Master` で鳴ります。`audio_backend` を割り当てている間は無視されます |
| `audio_backend` | `SpriteStudioAudioBackend` | *(なし)* | 内蔵プレイヤーを完全に置き換えます |

### `SpriteStudioAudioBackend`

オーバーライド可能なメソッドを 1 つだけ持つ `Resource` のサブクラスです。`audio_backend` に割り当てると **内蔵再生は抑制されます**（`audio_volume` と `audio_bus` も含む）。ボイスのライフサイクルと再生回数の管理はバックエンド側の責務になります。

* `play_audio(payload: Dictionary, ssab: SSABResource, player: Node) -> void`: 内蔵再生が鳴らすはずだったオーディオイベントごとに 1 回呼ばれます。内蔵再生と同じく、`play_audio` がオフのときと、順方向に再生していないとき（`is_playing_forward()` が `false`）は呼ばれません。どちらの場合も `audio` シグナルは送られます。`payload` は `audio` シグナルと同じ内容です。既定の実装は何もしません。

```gdscript
extends SpriteStudioAudioBackend

func play_audio(payload: Dictionary, ssab: SSABResource, player: Node) -> void:
    var info := ssab.get_sound_info(payload["sound_list_name_hash"], payload["sound_name_hash"])
    if not info.is_empty():
        MyMiddleware.play(info["path"], payload["loop_num"])
```

`payload` からサウンドを解決するには [`SSABResource`](resource.md#ssabresource) の `get_sound_stream()` / `get_sound_info()` を使います。

## AnimationPlayer から駆動する

`frame_no` プロパティはアニメート可能なので、`AnimationPlayer` のタイムライン（音・メソッド呼び出し・他ノードなど他トラック）と同期させて SpriteStudio アニメをスクラブできます。

1. `SpriteStudioPlayer2D` に通常どおり `Ssab`（SSAB リソース）を割り当て、`Current Animation` を選択。
2. `AnimationPlayer` で、ノードの `frame_no` プロパティを対象に **プロパティトラック** を追加。
3. `frame_no` を時間に沿ってキーフレーム（例：尺に合わせて `0` → 最終フレーム）。`frame_no` は float なので補間されます。
4. `AnimationPlayer` を再生。

> [!IMPORTANT]
> `AnimationPlayer` が `frame_no` を駆動している間は、ノードを**自走させない**でください（`Autoplay` をオフにし、`play()` も呼ばない）。さもないとノード自身の再生とキーフレームの `frame_no` が毎フレーム競合します。

これ以上の準備は不要です。キー値は `AnimationPlayer` のアニメーション側に保存され（ノードの `frame_no` はシーンに保存されません）、同じトラックがランタイムでも再生を駆動します。
