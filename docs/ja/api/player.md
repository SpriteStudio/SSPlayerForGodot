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
    ssnode.set_loop(0)        # 0 で無限ループ
    ssnode.set_speed(1.0)
    ssnode.play()
```

## 主なメソッド

* `set_ssab_resource(res: SSABResource)` / `get_ssab_resource() -> SSABResource`
* `set_animation(name: String)` / `get_animation() -> String`
* `play(start_frame: float = -1.0)`: 再生を開始します。`-1.0` を指定した場合は、現在のフレームまたは区間の先頭から再生します。
* `pause()`: 再生を一時停止します。
* `stop()`: 再生を停止します。
* `is_playing() -> bool` / `is_pausing() -> bool`
* `set_frame(frame: float)` / `get_frame() -> float` / `get_total_frames() -> int`
* `set_speed(speed: float)` / `get_speed() -> float`
* `set_frame_rate(fps: int)` / `get_frame_rate() -> int`
* `set_animation_section(start: int, end: int)`: 再生するフレーム区間を限定します。
* `set_playback_direction(direction: int, style: int)`:
  * `direction`: `0` = 順再生 (Forward)、`1` = 逆再生 (Backward)。
  * `style`: `0` = 通常/片道 (Normal)、`1` = 往復再生 (PingPong)。
* `set_loop(count: int)` / `get_loop() -> int`: `0` で無限ループになります。
* `set_skip_frames(enabled: bool)` / `is_skip_frames() -> bool`
* `set_sub_frame_enabled(enabled: bool)` / `is_sub_frame_enabled() -> bool`
* `set_cellmap_texture(cellmap_name: String, texture: Texture2D)` / `get_cellmap_texture(cellmap_name: String) -> Texture2D`

## シグナル

* `animation_started(anim_name: String)`: 再生が開始された時に発行。
* `animation_changed(anim_name: String)`: アニメーションが切り替わった時に発行。
* `animation_finished(anim_name: String)`: 再生が終了した時に発行（非ループ時）。
* `animation_looped(anim_name: String)`: ループした時に発行。
* `user_data(payload: Dictionary)`: 「ユーザーデータ」キーに到達した時に発行。
* `signal(command: String, value: Dictionary)`: 「シグナル」キーに到達した時に発行。

実際の挙動・引数のとり得る値は `ss_player/ss_player_node_2d.h` を参照してください。
