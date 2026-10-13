# 🎚️ モーションブレンド

1 つの `SpriteStudioPlayer2D` で、複数のアニメーションを同時に再生できます。歩きに足を引きずる動きを重ねる、トラッキングの値で首を振らせる、待機から攻撃へ切り替えるときにカットではなくなめらかに移る、といったことができます。アニメーションは **トラック** に積み、ランタイムがそれを 1 つの姿勢にまとめます。`.ssab` は変える必要がなく、どのアニメーションもプレーヤに割り当てた 1 つの `.ssab` から取ります。

メソッドの一覧は [クラスリファレンス](../api/player.md#motion-blending) にあります。このページでは、それらの組み合わせ方を説明します。

## トラックの積み重ね

track 0 は、`set_animation()` で選んだアニメーションです。その上のトラックは番号順に、下のトラックが作った姿勢の上へ重なります。

```
結果 = セットアップポーズ（アニメーションの .ssae が定義する素の姿勢）
track 0  →  重ねる   （主のアニメーション: set_animation）
track 1  →  重ねる   （上に重ねるアニメーション）
track 2  →  重ねる   （さらにもう 1 つ）
```

調整の値は 2 つあり、**どちらを使うかが大事** です。

| | 役割 | 単位 |
|---|---|---|
| **alpha**（`set_track_alpha`） | そのトラックを、下の結果にどれだけ重ねるか。**強さの調整はこちら** | トラックごと |
| **weight**（`set_source_weight`） | **同じトラック** に載せた複数のアニメーションの比率 | ソースごと |

トラックは、自分のアニメーションがキーを打ったアトリビュートにだけ効きます。頭だけにキーを打ったアニメーションを重ねても、マスクを設定しなくても脚には影響しません。つまり **重ねたアニメーションが効く範囲は、作画した範囲そのもの** です。

track 0 は **構造** を受け持ちます。セル・パーツカラー・シェーダ・反転・非表示・描画優先度・テクスチャはすべて track 0 の主アニメーションから来ます。既定のイベントの発火も track 0 からです。重ねたトラックが混ぜるのは、連続的なアトリビュート（位置・回転・スケール・不透明度など）です。

## 重ねる

```gdscript
@onready var player: SpriteStudioPlayer2D = $Player

func _ready() -> void:
    player.set_animation("idle")              # track 0
    player.set_track_animation(1, "look_around")
    player.set_track_alpha(1, 0.4)            # 40% 重ねる
    player.play()
```

トラックは必要に応じて作られます（track 3 を設定すると、track 1・2 も空のまま作られます）。`alpha` は 0〜1 に制限しません。1 を超えると外挿になります。

### 加算のトラック

`TRACK_BLEND_ADDITIVE` のトラックは、自分の姿勢へ近づける代わりに **セットアップポーズからの差分** を足すため、下のアニメーションが残り続けます。値で動かす軸に向いています。軸のアニメーションは中央のフレームがセットアップポーズになるように作り、フレームを直接動かします。

```gdscript
player.set_track_animation(1, "lean")
player.set_track_blend(1, SpriteStudioPlayer2D.TRACK_BLEND_ADDITIVE)
player.set_track_speed(1, 0.0)                # 時計ではなく手で動かす

func _process(_delta: float) -> void:
    player.set_track_frame(1, 30.0 + yaw * 30.0)
```

## クロスフェードで切り替える

`crossfade()` は、トラックがいま再生しているアニメーションから別のアニメーションへフェードします。入ってくるアニメーションは **呼んだ時点で** トラックを引き継ぎます。トラックがそれまで再生していたものは比率を保ったまままとめてフェードアウトし、フェードが終わると解放されます。時間は `advance()` と同じく **秒** で指定します。

```gdscript
player.crossfade(0, "run", 0.3)
```

| 状況 | 挙動 |
|---|---|
| 何も再生していないトラック | フェードせず、すぐに再生を始める |
| フェード中のトラック | 進行中のフェードをその時点の状態のまま引き取る。ポーズは飛ばない |
| 複数のソースを混ぜているトラック、`cancel_track_fade` で途中で止めたトラック | 載っているものがまとめてフェードアウトする |
| `duration` が 0 以下、または無いアニメーション名 | 受け付けず `false` を返す |
| フェードを途中で止めたい | `cancel_track_fade(track)`。alpha と weight はその時点の値で止まる |

`get_track_fade_progress(track)` はフェード中に 0〜1、フェードしていないときに **-1** を返します。

`fade_track()` はもう 1 つのフェードで、トラックが再生する内容ではなく、トラックがどれだけ効くかを変えます。track 0 を 0 までフェードすると、キャラクターはセットアップポーズに落ち着きます。

```gdscript
player.fade_track(1, 0.0, 0.2)                # 0.2 秒かけて重ねたトラックを消す
```

### track 0 のクロスフェードで、何がいつ替わるか

入ってくるアニメーションが track 0 を引き継ぐのは、フェードが終わったときではなく **始まったとき** です。

- `get_current_animation()` は呼んだ直後から入ってくるアニメーションの名前を返し、その時点で `animation_changed` が送られます。`animation_started` は送られません。切り替えは track 0 の再生状態をそのまま引き継ぎます。フェードが残していった側の名前で `set_animation()` を呼ぶとそちらへ戻り、入ってくる側の名前なら何も変わらずフェードが続きます。キャンバス（`get_canvas_size()` / `get_canvas_rect()`）も切り替えに追従します。
- セル・非表示など補間できない値は、フェードの最初のフレームから入ってくる側のものになります。`is_playing()`・`is_finished()` と `animation_*` シグナルも入ってくる側の状態を報告します。出ていく側がフェード中に終わっても、キャラクターは終了になりません。
- 入ってくる側は、`set_animation()` で切り替えたときと同じく、ノードの速度・フレームレート・フレームスキップ・再生方向・`loop_count` で再生されます。再生区間はそのアニメーションの全体です。
- イベントはフェードの開始から入ってくる側が発火し、出ていく側は止まります。下の [イベント](#イベント) を参照してください。

`set_track_animation(0, ...)` も、フェードせずに同じように track 0 を切り替えます。

## 1 つのトラックでアニメーションを混ぜる

比率で混ぜる場合（キャラクターの速さで歩きと走りを混ぜるなど）は、同じトラックに両方を載せて weight を設定します。

```gdscript
player.set_animation("walk")
var walk: int = player.get_track_main_source(0)
var run: int = player.add_track_source(0, "run", 0.0)

func _process(_delta: float) -> void:
    player.set_source_weight(walk, 1.0 - speed_ratio)
    player.set_source_weight(run, speed_ratio)
```

トラックに載せたアニメーションの 1 本 1 本が **ソース** で、整数のハンドルで指します（無いときは `-1`。`0` は正しいハンドルです）。ハンドルは、そのソースが解放されるまで有効です。解放されるのは、クロスフェードの完了、`clear_track()`、別のアニメーションへの `set_animation()` のときです。weight は比率なので、`(0.7, 0.3)` と `(7, 3)` は同じ結果になります。

## 位相をそろえて始める（`trackSync`）

トラックに載せたアニメーションは、再生区間の先頭から始まります。歩幅の途中にいる歩きの上に走りを載せると、足の運びが食い違います。載せる **前に** トラックを `TRACK_SYNC_START` にしておくと、次に載せたアニメーションは、track 0 のアニメーションがいる **位相**（再生区間のどこまで進んだかの割合）から始まります。尺の違うアニメーションも割合でそろいます。

```gdscript
player.set_track_sync(0, SpriteStudioPlayer2D.TRACK_SYNC_START)
player.crossfade(0, "run", 0.3)                # 走りは歩きと同じ位相から始まる
```

| | `TRACK_SYNC_FREE`（既定） | `TRACK_SYNC_START` |
|---|---|---|
| 載せたアニメーションの開始位置 | 再生区間の先頭 | track 0 の位相 |
| 向いている使い方 | 一回きりの動き（まばたき、攻撃など） | 周期的な動きどうしを重ねる（歩きと走りなど） |

`set_track_animation`、`crossfade`（track 0 でも効き、入ってくる側は出ていく側の位相から始まる）、`add_track_source` のすべてに効きます。そろえるのは載せたときの 1 回だけで、その後は両方が同じ時計で進むため、尺と速度が同じ間だけそろったままになります。まだ何も載っていないトラックにも設定できます。

## キャラクター全体で 1 つの時計

すべてのトラックはキャラクターの 1 つの時計で動き、再生の操作はその全体に効きます。

| 呼び出し | スタックへの効果 |
|---|---|
| `play()` | **track 0** を再生区間の先頭へ戻し、時計を動かす。他のトラックは再生位置をそのまま保つ |
| `pause()` / `resume()` | すべてのトラックとすべてのフェードを止める／再開する |
| `stop()` | キャラクター全体をその場で止める。再開できるのは `play()` だけ |

- **track 0 が終わっても時計は止まりません。** track 0 が最後のループを再生し終えても、重ねたトラックとフェードは進み続けます。`is_playing()`・`is_pausing()`・`is_finished()` と `animation_*` シグナルは track 0 の状態を表し、終了は通常どおり報告されます。
- 再生区間は track 0 だけのものです。それ以外の再生設定はトラックごとに持ちます。次の節を参照してください。

## トラックの再生設定

速度・フレームレートの上書き・フレームスキップ・再生方向は、トラックに載ったアニメーションではなく **トラック** のものです。そのトラックに載っているすべてのアニメーション（クロスフェードの両側を含む）と、あとから載せるすべてのアニメーションに効きます。track 0 の設定は、ノードの `speed_scale`・`frame_rate`・`frame_skip_enabled`・`set_playback_direction()` そのものです。

```gdscript
player.set_track_speed(1, 0.5)
player.set_track_frame_rate(1, 24)
player.set_track_playback_direction(1,
        SpriteStudioPlayer2D.PLAYBACK_DIRECTION_FORWARD,
        SpriteStudioPlayer2D.PLAYBACK_STYLE_PING_PONG)
```

一方 **ループ回数** はアニメーションのものです。トラックに載せたアニメーションは無限にループするので、回数は載せた後で設定してください。回数を再生し終えたトラックは最後のフレームで止まり、フェードするか空にするまで混ざり続けます。

```gdscript
player.set_track_animation(2, "hit_react")
player.set_track_loop_count(2, 1)              # 1 回再生して最後のフレームで止まる

func _process(_delta: float) -> void:
    if player.is_track_finished(2):
        player.clear_track(2)
```

track 0 のループ回数は `loop_count` です（`set_track_loop_count(0, ...)` も同じ値を書き換えます）。ノードは、track 0 が切り替わるたびにこの回数をかけ直します。`track_just_looped(track)` は、トラックが先頭へ戻った tick で `true` になります。track 0 の `just_looped()` と同じです。どちらもシグナルにはなりません。シグナルが出るのは track 0 の終了とループ（`animation_finished`・`animation_looped`）だけです。

## set_animation() でスタックは畳まれる

**別の** アニメーションへの `set_animation()` は、スタックを track 0 だけに戻します。重ねたトラック・ソース・トラックの設定（alpha・blend・sync）はすべてなくなるので、切り替えた後で重ね直してください。いま再生中のアニメーションをもう一度選んでも、何も変わりません。

## イベント

既定では **track 0 のアニメーションだけ** がイベント（`user_data`・`signal_emitted`・`audio`）を発火します。他のソースは、weight がそのソースのイベントのしきい値に届くと発火します。しきい値の初期値は `INF` です。

```gdscript
var overlay: int = player.get_track_main_source(1)
player.set_source_event_threshold(overlay, 1.0)   # weight が 1 以上なら発火する
```

各イベントには、発火したソースのハンドルが付きます。`user_data` と `audio` は `payload["source"]`、`signal_emitted` は `info["source"]` です。2 つのソースのイベントを区別できます。

track 0 のクロスフェードが始まった時点から、発火するのは入ってくるアニメーションで、出ていく側は止まります。フェードの途中まで出ていく側も発火させたい場合は、トラックに出ていく側のしきい値を設定します。出ていく側は、トラックに占める割合 `1 - progress` がしきい値以上のあいだ発火します。

```gdscript
player.set_track_outgoing_event_threshold(0, 0.5)   # フェードの前半だけ出ていく側も発火する
```

しきい値はトラックの設定なので、一度設定すれば残ります。既定値の `INF` では、フェードの開始と同時に出ていく側は発火しなくなります。

## 制約

- **`.ssab` は 1 つです。** 積めるのは、プレーヤに割り当てたリソースのアニメーションだけです。
- **インスタンスパーツとエフェクトパーツはモーションブレンドの対象外です。** これらは 1 つのアニメーションではなく `.ssab` 全体のものなので、track 0 のクロスフェードでは準備し直さず、動いているまま引き継ぎます。切り替えの前後でどちらのキーがそのパーツを動かすかは混ぜられません。両方のアニメーションが同じフレームにキーを持つ場合、出ていく側のキーの設定のまま動き続けることがあります。
- **積むアニメーションでは、同じパーツに同じセルを割り当ててください。** セルの大きさと原点は track 0 から取るため、重ねたトラックのセルが違うとパーツが崩れます。
- 公式プレーヤ共通の仕様は、ランタイムの [アニメーションミキシングのガイド](https://github.com/cri-middleware/SpriteStudio-SDK/blob/HEAD/libs/ssruntime/docs/50_features/10_animation_mixing.ja.md) も参照してください。
