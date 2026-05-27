# 応用的な使い方と Tips

## シグナルとユーザーデータの利用

SpriteStudio ではアニメーションのタイムライン上にカスタムデータを埋め込むことができます。Godot ではこれらをシグナルとして受け取れます。

```gdscript
func _ready():
    # ユーザーデータシグナルに接続
    ss_player.user_data.connect(_on_ss_user_data)

    # 「シグナル」イベントに接続
    # 注意: 'signal' は GDScript の予約語であるため、ドット構文ではなく文字列で接続します
    ss_player.connect("signal", _on_ss_signal)

func _on_ss_user_data(payload: Dictionary):
    # 例: ユーザーデータに基づいて SE を再生する
    if payload.has("se"):
        audio_player.stream = load(payload["se"])
        audio_player.play()

func _on_ss_signal(command: String, value: Dictionary):
    # command にコマンドID文字列、value に各パラメータが Dictionary として格納されます
    print("Signal command received: ", command)
    print("Params: ", value)
```

## 動的なテクスチャ差し替え (CellMap Overrides)

実行時に特定のセルマップのテクスチャを差し替えることができます。キャラの着せ替えや色違いパターンの実装に役立ちます。

```gdscript
# 特定のセルマップのテクスチャを差し替える
var new_skin = load("res://textures/hero_red.png")
ss_player.set_cellmap_texture("chara_skin", new_skin)

# 元に戻す場合は null を渡します
ss_player.set_cellmap_texture("chara_skin", null)
```

## パフォーマンスと品質の設定

### SSAB と SSQB の使い分け
- **SSAB**: 通常のアニメーションに使用します。
- **SSQB**: 複数のアニメーションを連結した「シーケンス」を、SpriteStudio 側で制御した状態で再生したい場合に使用します。

### フレームスキップとサブフレーム
- **Skip Frames**: 負荷が高いアニメーションで、描画が遅れても再生速度を維持したい場合に有効にします。
- **Sub Frame Enabled**: キーフレーム間を補完して滑らかに再生します。スロー再生などを行う場合に特に効果的です。
