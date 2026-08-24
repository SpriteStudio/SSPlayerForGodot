# 🔊 サウンド再生

SpriteStudio で設定したサウンドパートは、**特別な準備なしに** Godot 上で鳴ります。`.ssab` を割り当てて再生すれば、タイムライン上のサウンドがそのまま再生されます。このページでは、その挙動を止める / 調整する / 自前のサウンド基盤に差し替える、3 つの方法を説明します。

> [!NOTE]
> サウンドは、このプレイヤーが共通ランタイムより一歩踏み込んでいる唯一の箇所です。`libssruntime` はサウンドキーを通過したことを *通知* するだけで、兄弟プレイヤーはその通知をアプリケーション側に渡します。`SpriteStudioPlayer2D` はそれに加えて再生機能そのものを内蔵しています。Godot には音を鳴らすために必要なものが既に揃っているためです。

---

## デフォルトの挙動

コンバーターは、プロジェクトが参照するサウンドファイルを生成された `.ssab` の隣にコピーします。そのため再生時点では、それらは通常の Godot リソースになっています。再生ヘッドがサウンドキーを通過すると、プレイヤーはバインドされた `SSABResource` 経由で該当ファイルを解決し、`AudioStreamPlayer` を開始します。

ボイス（`AudioStreamPlayer`）は必要になった時点で `SpriteStudioPlayer2D` の **内部子ノード** として生成されます（シーンツリーには現れず、シーンにも保存されません）。プール管理されており、再生が終わったボイスは破棄せず再利用するため、サウンドの多いアニメーションでもノードの生成・破棄を繰り返しません。`audio_volume` はリニア値で、各ボイスに設定される際にデシベルへ変換されます。

制御するプロパティは `SpriteStudioPlayer2D` に 3 つあります。

| プロパティ | 型 | 既定値 | 説明 |
| --- | --- | --- | --- |
| `Play Audio` (`play_audio`) | bool | `true` | 内蔵プレイヤーが音を鳴らすかどうか。オフにすると `audio` シグナルを使って完全に自前で処理できます |
| `Audio Volume` (`audio_volume`) | float | `1.0` | 内蔵ボイスに適用されるリニア音量 (`[0, 1]`)。バックエンドを割り当てている場合は無視されます |
| `Audio Backend` (`audio_backend`) | `SpriteStudioAudioBackend` | *(なし)* | 内蔵プレイヤーを置き換えます。[サウンドを別の基盤へ流す](#routing-audio-elsewhere)を参照 |

```gdscript
@onready var ss_player: SpriteStudioPlayer2D = $SpriteStudioPlayer2D

func _ready() -> void:
    ss_player.set_audio_volume(0.4)      # 音量を下げる
    # ss_player.set_play_audio(false)    # あるいは内蔵プレイヤーを完全に黙らせる
```

> [!TIP]
> 内蔵再生は**エディタプレビューでも動作します**。SpriteStudio ボトムパネルでプレビューやフレームスクラブをする際、ゲームを起動しなくても音を確認できます。

---

## 再生の性質

サウンドは **撃ちっぱなし (fire-and-forget)** です。フレームを通過した瞬間に再生が始まり、そこから先は通常の `AudioStreamPlayer` ボイスとして最後まで鳴ります。タイミングにシビアなサウンド設計を組む前に、以下を把握しておいてください。

- **順再生時のみ鳴ります。** 実効的な再生方向が逆向きのとき（逆再生、ping-pong の復路）は何も鳴りません。これは Godot 固有ではなく[共通の制限](../limitations.md)です。負の `speed_scale` はこれに該当しません。逆再生になるのではなく停止し、停止した再生ヘッドはサウンドキーを通過しないため、やはり鳴りません。
- **シークは飛ばした分を再生しません。** 再生ヘッドをジャンプさせた場合、発火するのは到達フレームのイベントだけです。既に鳴っているサウンドが新しい位置へ再同期されることもありません。
- **アニメーションの一時停止はサウンドを止めません。** `pause()` / `stop()` が止めるのは *アニメーション* です。既に鳴っているボイスは最後まで再生されます。`play_audio` をオフにした場合とノードがツリーから外れた場合は停止します。
- **再発火は重なります（切り替わりません）。** 前のインスタンスがまだ鳴っている最中に同じサウンドが再度発火すると（典型的にはループ境界）、2 本目のボイスが始まります。前のボイスが打ち切られることはありません。
- **`loop_num` はフラグではなく再生回数です。** SpriteStudio に無限ループのサウンドはありません。`1` なら 1 回、`n` なら連続して `n` 回再生します。

> [!NOTE]
> ゲームと一緒に一時停止する、ダッキングする、クロスフェードするといったサウンドが必要な場合は、`audio` シグナルか[バックエンド](#routing-audio-elsewhere)を使って自前で駆動してください。内蔵プレイヤーは、単純なケースを正確に実装することに意図的に絞っています。

---

## サウンドイベントを観測する

`audio` シグナルは **観測用のチャンネル**で、上記のすべてとは独立に発火します。再生方向を問わず、エディタ上でも、`play_audio` のオン / オフに関わらず発火します。再生を奪わずにサウンドへ反応したい場合（口パク、画面シェイクなど）や、`play_audio` を `false` にして再生を丸ごと置き換える場合に使います。

```gdscript
func _ready() -> void:
    ss_player.set_play_audio(false)          # 自前で鳴らす
    ss_player.audio.connect(_on_audio)

func _on_audio(payload: Dictionary) -> void:
    var ssab: SSABResource = ss_player.get_ssab_resource()
    var stream := ssab.get_sound_stream(payload["sound_list_name_hash"], payload["sound_name_hash"])
    if stream == null:
        return
    var voice := AudioStreamPlayer.new()
    add_child(voice)
    voice.stream = stream
    voice.finished.connect(voice.queue_free)
    voice.play()
```

| ペイロードのキー | 型 | 意味 |
| --- | --- | --- |
| `part_index` | `int` | 発火したサウンドパートのインデックス |
| `part_name` | `String` | そのパート名 |
| `frame_no` | `int` | キーが置かれているフレーム。1 ティックで複数フレームをまたぐことがあるため、検出されたフレームとは限りません |
| `sound_list_name_hash` | `int` | サウンドリスト名のハッシュ |
| `sound_name_hash` | `int` | サウンド名のハッシュ |
| `sound_name` | `String` | 設定されたサウンド名（設定時のみ存在） |
| `loop_num` | `int` | 再生回数 |

2 つのハッシュは `.ssab` 内でのサウンドのアドレスです。リソース側で解決します。

* [`get_sound_stream(sound_list_name_hash, sound_name_hash) -> AudioStream`](../api/resource.md#ssabresource) — 読み込まれたストリーム。ファイルが存在しない / 非対応の場合は `null`。結果はリソースごとにキャッシュされます（見つからなかった結果も含む）。
* [`get_sound_info(sound_list_name_hash, sound_name_hash) -> Dictionary`](../api/resource.md#ssabresource) — 何も読み込まずにメタデータだけを返します: `alias` / `file_path` / `path` / `file_path_hash` / `time_total`。

---

## サウンドを別の基盤へ流す (Routing audio elsewhere)

オーディオミドルウェアや独自のバス構成、内蔵プレイヤーにはないプーリング方式へサウンドを流したい場合は、**`SpriteStudioAudioBackend`** を継承したリソースを作り、ノードの `Audio Backend` プロパティに割り当てます。

```gdscript
# res://audio/my_backend.gd
extends SpriteStudioAudioBackend

func play_audio(payload: Dictionary, ssab: SSABResource, player: Node) -> void:
    var info := ssab.get_sound_info(payload["sound_list_name_hash"], payload["sound_name_hash"])
    if info.is_empty():
        return
    # info["path"] は解決済みの res:// パス、info["alias"] は SpriteStudio 上で設定した名前です。
    MyMiddleware.play(info["path"], payload["loop_num"])
```

インスペクタから割り当てます（`Audio Backend` にスクリプトをドラッグするか、`.tres` として保存したものを指定）。1 つのバックエンドリソースを複数のプレイヤーで共有できます。イベントを発火したノードは `player` 引数として渡されます。

> [!IMPORTANT]
> **バックエンドを割り当てると、内蔵再生は常に抑制されます**（`audio_volume` も含む）。ボイスのライフサイクルと再生回数の管理は完全にバックエンド側の責務になり、部分的な委譲はできません。あるイベントに対してバックエンドが何もせずに戻れば、そのイベントは無音になります。

`play_audio` は引き続き呼び出しのゲートとして働き、順再生のみという制約もそのまま適用されます（逆再生中はバックエンドも呼ばれません）。

---

## 関連ページ

- [スクリプト制御とイベント連携](usage_scripting.md) — 他のタイムラインイベント (`user_data` / `signal_emitted`) とスクリプト API 全般。
- [SpriteStudioPlayer2D API](../api/player.md#audio) — このページのプロパティに対応するメソッドリファレンス。
- [制限事項](../limitations.md) — サウンドが共通ランタイムから引き継ぐ制約。
