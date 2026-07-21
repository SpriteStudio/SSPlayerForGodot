# スクリプト制御とイベント駆動

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

## パーツオーバーライド（パーツカラー / セル / 表示指定）

パーツ単位のランタイムオーバーライドは、「このパーツを**今**この色に / このセルに / 非表示に」とスクリプトから指示する機能です。オーバーライドはキーフレームやアニメーションのブレンドよりも優先されるので、アニメーションと取り合いになりません。

```gdscript
@onready var ss_player = $SpriteStudioPlayer2D

func _ready():
    # パーツを赤く着色（乗算）。通常（画像）パーツに適用されます。
    ss_player.set_part_color_override("body", Color.RED, 1)  # 1 = Mul

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
| `set_part_cell_override(part_name, cellmap_name, cell_name, priority = 1)` | 別のセルで描画する |
| `set_part_visibility_override(part_name, force_hidden, cascade = false)` | 強制非表示（`force_hidden = false` でアニメーションに戻す） |
| `clear_part_color_override` / `clear_part_cell_override` / `clear_part_visibility_override` | 1 パーツの 1 オーバーライドを解除 |
| `clear_all_part_overrides()` | そのプレーヤの全オーバーライドを解除 |
| `*_by_index(part_index, ...)` | 上記各メソッドのパーツインデックス指定版（パーツ名の解決を省略） |

各メソッドは、パーツが不明な場合やランタイムが受け付けなかった場合に `false` を返します。

セルオーバーライドに指定できるセルマップ名 / セル名は、リソース側から列挙できます。

```gdscript
var ssab := ss_player.get_ssab_resource()
print(ssab.get_cellmap_names())        # → ["Ringo", ...]
print(ssab.get_cell_names("Ringo"))    # → ["effect3", ...]
```

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
