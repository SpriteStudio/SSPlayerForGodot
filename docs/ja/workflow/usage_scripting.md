# 📝 スクリプト制御とイベント駆動

Godot の GDScript を使って `SpriteStudioPlayer2D` を制御する方法を解説します。
Godot の設計思想（ノードとシグナル）に沿った直感的な API が提供されており、ゲームロジックへの組み込みが非常に容易です。

---

## 直感的な再生制御

インスペクタでの操作と同様に、スクリプトからもシンプルなメソッド呼び出しでアニメーションを制御できます。

```gdscript
extends Node2D

@onready var ss_player = $SpriteStudioPlayer2D

func _ready():
    # アニメーション名を指定
    ss_player.set_animation("attack")
    # 再生開始
    ss_player.play()

func _process(delta):
    # スペースキーで一時停止 / 再開
    if Input.is_action_just_pressed("ui_accept"):
        if ss_player.is_playing():
            ss_player.pause()
        else:
            ss_player.play()
```

---

## シグナルによるイベント駆動の実装

Godot ユーザーにとって最も強力な機能の一つが「シグナル (Signals)」を用いたイベント連携です。
`SpriteStudioPlayer2D` は再生状態の変更やユーザーデータに応じて便利なシグナルを発行します。

### 主なシグナル

* `animation_changed(anim_name)` : アニメーションが切り替わった時
* `animation_started(anim_name)` : アニメーションの再生が開始された時
* `animation_finished(anim_name)`: 指定したループ回数をすべて再生し終えた時（無限ループでは発火しません）
* `animation_looped(anim_name)`  : アニメーションが1周して先頭に戻った時
* `user_data(payload)`           : アニメーションに設定されたユーザーデータ（イベント）のフレームに到達した時

### 実装例: アニメーションの連続再生
「攻撃」アニメーションが終了したら、自動的に「待機」アニメーションへ遷移させる実装例です。

```gdscript
func _ready():
    # エディタのUI（ノードタブ）から接続することも可能ですが、コードで接続する場合は以下のように書きます。
    ss_player.animation_finished.connect(_on_animation_finished)

func _on_animation_finished(anim_name: String):
    if anim_name == "attack":
        # 攻撃が終わったら待機状態に戻る
        ss_player.set_animation("idle")
        ss_player.play()
```

> [!NOTE]
> ![シグナル接続画面の画像](../../assets/6-connect_signals_node_tab.png)

### 実装例: ユーザーデータを使ったイベント発火
SpriteStudio 上で設定したユーザーデータ（足音の再生、攻撃判定の発生など）を受け取り、ゲーム側で処理を行う実装例です。

```gdscript
func _ready():
    # ユーザーデータシグナルの接続
    ss_player.user_data.connect(_on_user_data)

func _on_user_data(payload):
    # payload は Dictionary。設定されたキーのみ含まれる（string / integer / point / rect）
    # 例: 文字列(string)を判定して処理
    if payload.get("string") == "play_footstep":
        $AudioStreamPlayer.play()
    elif payload.get("string") == "attack_hit":
        # 整数値(integer)を使ってダメージ量などを渡す例
        var damage = payload.get("integer", 0)
        spawn_hitbox(damage)
```

---

## 動的テクスチャ差し替え (アバターの着せ替え)

ゲーム内でキャラクターの装備を変えたい場合など、特定のパーツ（セルマップ）のテクスチャをプログラムから動的に差し替えることができます。

### 実装例: 衣装の着せ替え

```gdscript
func change_costume():
    # セルマップ名は SpriteStudio で定義された名前（get_cellmap_names() で取得、インスペクタの CellMap Overrides に表示）を指定する
    var new_costume_texture = preload("res://assets/sailor_uniform.png")
    ss_player.set_cellmap_texture("Clothes 1", new_costume_texture)
```

この機能により、アニメーションデータをパーツごとに何パターンも用意することなく、効率的なアバターシステムを構築できます。

> [!TIP]
> ![着せ替え前](../../assets/7-cellmap_override_before.png)
> ![着せ替え後](../../assets/7-cellmap_override_after.png)

---

## パーツトラッキング（指定パーツへの追従）

指定したパーツに、ユーザーが用意したノード（武器・エフェクト・当たり判定など）を毎フレーム追従させる機能です。プレーヤはノードを生成も破棄もせず、姿勢を書き込むだけの連動（Constraint）方式で、対象のライフサイクルはユーザーが所有します。

追従には専用ノード **`SpriteStudioPartAttachment2D`** を使います。`SpriteStudioPlayer2D` の子として置き、`part_name` に追従したいパーツ名を指定してください。このノードの下に武器やエフェクトをぶら下げれば、シーンツリーの継承でまとめて追従します。

> **プロパティは Godot 標準の `RemoteTransform2D` を踏襲**しています。これに「どのプレーヤの、どのパーツに追従するか」を指定する `follow_path` / `part_name` が加わった形です。

| プロパティ | 説明 |
|---|---|
| **Part Name** (`part_name`) | 追従対象のパーツ名（`.ssab` の PartData 名）。インスペクタではアセットのパーツ名からドロップダウンで選べます（プレーヤを解決できない場面のために手入力も可） |
| **Follow Path** (`follow_path`) | 追従元の `SpriteStudioPlayer2D`。空（既定）なら**最も近い祖先**のプレーヤを使います |
| **Remote Path** (`remote_path`) | 駆動する対象の `Node2D`。空（既定）なら自分自身を動かし、子はシーンツリーの継承で追従します。指定するとその外部ノードへ姿勢を書き込みます（プレーヤのサブツリーの外に置いた資産を追従させたい場合） |
| **Use Global Coordinates** (`use_global_coordinates`) | ON（既定）でグローバル座標として、OFF で対象のローカル座標として書き込みます |
| **Update Position / Update Rotation** (`update_position` / `update_rotation`) | 位置 / 回転を反映します（ともに既定 ON） |
| **Update Scale** (`update_scale`) | スケールを反映します（既定 OFF） |
| **On Part Hidden** (`on_part_hidden`) | パーツが hide のフレームでの挙動。`Follow Always`（追従を継続。既定）/ `Hide Target`（対象を非表示） |

### スクリプトからの参照

ノードを置かずに、プレーヤへ直接パーツの姿勢を問い合わせることもできます。

```gdscript
@onready var ss_player = $SpriteStudioPlayer2D
@onready var muzzle = $Muzzle

func _ready():
    print(ss_player.get_part_names())    # → ["root", "body", "hand_R", ...]

    # そのフレームのパーツ姿勢が確定するたびに通知される
    ss_player.frame_updated.connect(_on_frame_updated)

func _on_frame_updated(frame_no: float):
    # get_part_transform() はプレーヤローカル。グローバルにするならプレーヤの変換を掛ける
    muzzle.global_transform = ss_player.global_transform * ss_player.get_part_transform("hand_R")
```

| API | 説明 |
|---|---|
| `get_part_names()` | アセット（`.ssab`）に含まれる全パーツ名 |
| `get_part_index(part_name)` | パーツインデックス。アセットに無ければ `-1` |
| `get_part_transform(part_name)` | そのパーツの現在フレームの変換（`Transform2D`。プレーヤローカルで、`flip_h` / `flip_v` / `offset` を適用済み）。パーツが不明なら単位行列 |
| `is_part_hidden(part_name)` | そのパーツが現在フレームで hide かどうか。パーツが不明なら `false` |
| signal `frame_updated(frame_no: float)` | そのフレームのパーツ姿勢が確定した直後に発火する |

> **一瞬の姿勢だけが要る場合**: 弾の発射位置を取るなど、常時追従させるまでもない場合は、`SpriteStudioPartAttachment2D` を置かずに `get_part_transform()` を直接呼ぶ方が簡潔です。

### 追従のタイミングと精度

追従は、プレーヤが自身の更新を終えた直後に発行する `frame_updated` シグナルで駆動されます。パーツの変換が確定した後・描画の前なので、追従先は**同じフレーム内**で更新されます。どのプロセスで発火するかは、プレーヤの `animation_process_mode`（`Idle`（既定）/ `Physics`）に従います。

Godot の `Transform2D` は 2×3 のアフィン変換をそのまま保持できるため、`update_position` / `update_rotation` / `update_scale` が**すべて ON** のときは変換を丸ごと代入します。この場合は**せん断（Skew）や負のスケールも含めて厳密に一致**し、対象をプレーヤの子に置いても別階層に置いても差はありません。

1 つでも OFF にすると、`RemoteTransform2D` と同じく有効な成分だけを個別に書き込むため、せん断は保持されません。既定は `update_scale` のみ OFF なので、**既定では位置と回転だけが反映されます**。

> **`flip_h` / `flip_v` を使う場合は 3 つとも ON にしてください。** 反転したパーツの姿勢は**鏡像**で、鏡像は負のスケールとしてしか表せません。`update_scale` が OFF（既定）だと対象に鏡像が伝わらず、さらに **`flip_h`（左右反転）では向きに 180 度の差が残ります**（`flip_v` では残りません）。左右と上下で違うのは、`Transform2D` が鏡像を「回転＋負のスケール」に分解する際に符号を Y 軸側へ載せるためで、左右の鏡像はこの形にするのに余分な 180 度回転を必要とし、それが回転側に残ります。鏡像になるのは `flip_h` / `flip_v` を使ったときだけではなく、SpriteStudio 側でパーツやその親に負のスケールが付いている場合も同じです。
>
> なお **`get_part_transform()` の戻り値は反転時も正確です**。`Transform2D` は鏡像をそのまま保持でき、`get_rotation()` と `get_scale()` を**組で**使う限り厳密に一致します（反転時は `get_scale()` の Y が負になります）。片方だけを読むと上と同じ理由でズレます。

> **別階層の対象は 1 フレーム遅れることがあります。** 姿勢は駆動時点のプレーヤの `global_transform` を使って書き込むため、その後にプレーヤ自身を動かしても、対象が追随するのは次のフレームです。`SpriteStudioPartAttachment2D`（とその子）をプレーヤの子に置いた場合は、シーンツリーの継承で常に追随します。

> **`RigidBody2D` を追従対象にしないでください。** 毎フレーム transform を直接書き換えるとソルバがテレポートとして扱い、物理が破綻します。動く床のように他の剛体を押す必要がある場合は、Godot 標準の `AnimatableBody2D`（`sync_to_physics` を ON）を対象にし、プレーヤの `animation_process_mode` を `Physics` にして物理フレームで駆動してください。当たり判定を運ぶだけなら `Area2D` / `StaticBody2D` で十分です。

### 注意点

- **対象の `visible` はアタッチメントが操作します。** 次の 2 つの場合に自動で非表示になり、条件が解消すると自動で再表示されます。ユーザー側で設定した表示状態は上書きされることがあります。
    - パーツ名がアセットに存在しない（この場合は `On Part Hidden` の設定に関係なく常に非表示）
    - パーツが hide のフレームで、かつ `On Part Hidden` が `Hide Target`
- パーツ名は、そのプレーヤ自身が読み込んでいる `.ssab` のパーツから解決されます。**インスタンスパーツの内部（子アニメーション）のパーツは指定できません**（インスタンスパーツ自体は指定できます）。
- パーツ名の解決はアセット（`.ssab`）単位で、アニメーションには依存しません。`.ssab` を差し替えると自動で解決し直されるため、設定をやり直す必要はありません。
- 同名のパーツが複数ある場合は、最初に見つかったものが使われます。
- 追従するのは**空間的な変換だけ**です。描画順（Z 順）は追従しないため、追従させたノードが SpriteStudio のパーツの「間」に自動で挟まることはありません。前後関係が必要な場合は `z_index` などで別途調整してください。
- 対象にできるのは `Node2D` 系のノードです。`Control`（UI）はアンカーと矩形でレイアウトされるため対象にできません。
- 編集モードでも、プレビュー再生やフレームのスクラブでプレーヤが更新されれば、そのタイミングで追従します。

---

## パーツオーバーライド（パーツカラー / セル / 表示指定）

パーツ単位のランタイムオーバーライドは、「このパーツを**今**この色に / このセルに / 非表示に」とスクリプトから指示する機能です。オーバーライドはキーフレームやアニメーションのブレンドよりも優先されるので、アニメーションと取り合いになりません。

```gdscript
@onready var ss_player = $SpriteStudioPlayer2D

func _ready():
    # パーツを赤く着色（乗算）。通常（画像）パーツに適用されます。
    ss_player.set_part_color_override("body", Color.RED, 1)  # 1 = Mul

    # 4 頂点それぞれに色を指定してグラデーションにすることもできます。
    ss_player.set_part_color_override_corners(
        "body", Color.RED, Color.RED, Color.BLUE, Color.BLUE, 0)

    # 別のセルを描画させる（セルマップ名は ".ssce" を付けずに指定）。
    ss_player.set_part_cell_override("body", "Ringo", "effect3")

    # パーツを強制非表示にする（子孫にもカスケード）。
    ss_player.set_part_visibility_override("body", true, true)

    # 解除
    ss_player.clear_part_color_override("body")
    ss_player.clear_all_part_overrides()
```

| メソッド | 説明 |
|---|---|
| `get_part_index(part_name)` | パーツインデックス。アセットに無ければ `-1` |
| `set_part_color_override(part_name, color, blend_op = 0, priority = 1)` | パーツカラーオーバーライド（単色） |
| `set_part_color_override_corners(part_name, left_top, right_top, left_bottom, right_bottom, blend_op = 0, priority = 1)` | パーツカラーオーバーライド（4 頂点それぞれに色を指定＝グラデーション） |
| `set_part_cell_override(part_name, cellmap_name, cell_name, priority = 1)` | 別のセルで描画する |
| `set_part_visibility_override(part_name, force_hidden, cascade = false)` | 強制非表示（`force_hidden = false` でアニメーションに戻す） |
| `clear_part_color_override` / `clear_part_cell_override` / `clear_part_visibility_override` | 1 パーツの 1 オーバーライドを解除 |
| `clear_all_part_overrides()` | そのプレーヤの全オーバーライドを解除 |
| `*_by_index(part_index, ...)` | 上記各メソッドのパーツインデックス指定版（パーツ名の解決を省略） |

各メソッドは、パーツが不明な場合やランタイムが受け付けなかった場合に `false` を返します。なお単色と 4 頂点色は 1 パーツにつき同じオーバーライド枠を共有するので、後から呼んだ方が有効になり、`clear_part_color_override()` はどちらも解除します。

セルオーバーライドに指定できるセルマップ名 / セル名は、プレーヤから列挙できます。

```gdscript
print(ss_player.get_cellmap_names())        # → ["Ringo", ...]
print(ss_player.get_cell_names("Ringo"))    # → ["effect3", ...]
```

どちらも割り当て済みの `SSABResource` を読むので、未割り当てなら空の配列を返します。同じ 2 つのメソッドはリソース自身にもあり（`ss_player.get_ssab_resource().get_cellmap_names()`）、まだプレーヤに載せていない `.ssab` を列挙したい場合はそちらを使います。

> **テクスチャとセルの差し替えの使い分けについて**: 前節の `set_cellmap_texture()` は**セルマップ（テクスチャ）まるごと**の差し替えで、そのセルマップを使う全パーツにまとめて効きます。こちらは**パーツ 1 つ単位**で、描画するセルそのものを差し替える機能です。目的に応じて使い分けてください。

> **パーツインデックスの使い方について**: パーツインデックスは同一アセット（同じ `.ssab`）内では安定しているので、頻繁にオーバーライドするなら `get_part_index()` で一度パーツ名をパーツインデックスに解決して、`*_by_index()` にそのインデックスを使い回すことを推奨します。

### 合成モード（blend_op）

`set_part_color_override()` の `blend_op` は、キーフレームのパーツカラーと同じ 4 種です。

| 値 | 合成モード |
|---|---|
| `0` | Mix（既定） |
| `1` | Mul（乗算） |
| `2` | Add（加算） |
| `3` | Sub（減算） |

範囲外の値を渡した場合は設定に失敗し、`false` を返します。

### 優先モード（priority）

パーツカラーとセルのオーバーライドはアニメーションと競合するため、`priority` を取ります（表示指定にはありません。単なる強制非表示フラグで、アニメーションを設定し直すと必ずクリアされます）。

| 値 | 優先モード | 挙動 |
|---|---|---|
| `0` | OverwriteOnNextKeyframe | アニメーションデータが当該アトリビュートを更新するまで、オーバーライドが適用される |
| `1` | HoldUntilNextAnimation（既定） | 現在のアニメーション中は勝ち続け、新しいアニメーションを設定するとオーバーライドが解除される |
| `2` | Permanent | 同じアニメーションデータ（`.ssab`）である間、オーバーライドが適用される（アニメーション変更をまたいでも持続） |

### 注意点

- **カラー**は通常パーツ、**セル**は通常パーツとマスクパーツに適用されます。それ以外の種別のパーツでは黙って無視されます（呼び出し自体は `true` を返します）。
- 色はオーサリングされた Part Color と同じ 8bit sRGB 空間として解釈され、アルファはランタイム側で pre-multiply されます。自前で変換せず、オーサリングどおりの色を渡してください。
- セルオーバーライドは設定時に解決されるため、存在しないセルマップ名 / セル名はその場で失敗します（`false` を返します）。
- オーバーライドはランタイムが保持し、そのライフサイクルもランタイムが管理します。アニメーション変更後に再適用する必要はありません。意図に合った優先モードを選んでください。
- 別の `.ssab` リソースを割り当てると、パーツの同一性が失われるため全オーバーライドが解除されます。
- インスタンスパーツ**配下**のパーツには届きません（子アニメーションは別のプレーヤとして動作するためです）。インスタンスパーツ自体を強制非表示にした場合は、その配下もまとめて描画されなくなります。

> **オーバーライドの設定が描画に反映されないタイミングについて**: アニメーション停止 / 一時停止中や、フレームが進まない状況では描画が更新されないため、オーバーライドの設定・解除が画面に反映されません。その場で反映させたい場合は `set_frame(get_frame())` を呼んで再描画させてください。
