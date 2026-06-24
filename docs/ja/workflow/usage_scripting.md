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
* `animation_finished(anim_name)`: アニメーションの再生が終了した時（非ループ時）
* `animation_looped(anim_name)`  : アニメーションがループして先頭に戻った時
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
    # 例: ユーザーデータとして設定された文字列(String)を判定して処理
    if payload.string_value == "play_footstep":
        $AudioStreamPlayer.play()
    elif payload.string_value == "attack_hit":
        # 整数値(Int)を使ってダメージ量などを渡す例
        var damage = payload.int_value
        spawn_hitbox(damage)
```

---

## 動的テクスチャ差し替え (アバターの着せ替え)

ゲーム内でキャラクターの装備を変えたい場合など、特定のパーツ（セルマップ）のテクスチャをプログラムから動的に差し替えることができます。

### 実装例: 武器の持ち替え

```gdscript
func change_weapon():
    # "weapon_map" というセルマップに対し、新しいテクスチャを適用する
    var new_sword_texture = preload("res://assets/iron_sword.png")
    ss_player.set_cellmap_texture("weapon_map", new_sword_texture)
```

この機能により、アニメーションデータをパーツごとに何パターンも用意することなく、効率的なアバターシステムを構築できます。

> [!TIP]
> ![着せ替え前](../../assets/7-cellmap_override_before.png)
> ![着せ替え後](../../assets/7-cellmap_override_after.png)
