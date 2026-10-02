# 🔍 コマンドラインで再生を検証する

エディタを開かずに、`.ssab` が読み込めて再生できることを確かめる方法です。CI や、あなたのプロジェクトで作業しているコーディングアシスタント（コマンドは実行できるが、エディタの画面は見られない）を想定しています。

確認は 2 種類あり、答えられる問いが違います。

| 確認 | 分かること | 必要なもの |
| --- | --- | --- |
| [数値](#確認-1-数値) | 拡張は読み込まれたか、アニメーションは存在するか、再生ヘッドは動くか | 不要（`--headless` で動く） |
| [ピクセル](#確認-2-ピクセル) | 実際に何かが描画されたか、どう見えるか | ディスプレイ（ウィンドウが一瞬開く） |

`--headless` はダミーのレンダラーを使うので、ピクセルは一切出ません。アニメーションの見た目に関わることは、2 つ目の確認が必要です。

以下のコマンドは、エディタのバイナリを `godot` と書いています。標準の 4.7 エディタビルドを使ってください。カスタムモジュールビルドにはクラスがすでに組み込まれているので、さらに拡張を読み込むと二重登録になって異常終了します。

---

## 初回の前に: プロジェクトを 1 度インポートする

Godot が一度も開いたことのないプロジェクトには `.godot/` フォルダがなく、`.ssab` の隣にある `.png` はインポートが済むまで使えません。コマンドラインから 1 度インポートしておきます。

```bash
[ -f .godot/extension_list.cfg ] || { mkdir -p .godot; echo "res://addons/spritestudio/spritestudio.gdextension" > .godot/extension_list.cfg; }
godot --headless --path . --import
```

1 行目が要です。これが無いと、Godot はインポートの途中で拡張を見つけ、終了時にクラッシュします（macOS の 4.7.2 で signal 11、終了ステータス 134）。もう一度 `--import` を走らせれば 0 で終わりますが、スクリプトや CI がクラッシュに依存すべきではありません。拡張を先に書いておけば、Godot は起動時にそれを読み込みます。このファイルは以降 Godot 自身が管理するので、必要なのは Godot が一度も開いていないプロジェクトだけです。

このインポートは、プロジェクトにもう 2 つのものを残します。エディタが起動すると SS Import Dock が自分で準備を済ませるためで、プロジェクトルートの `.ssplayer_sources.cfg`（ドックの設定です。バージョン管理に含めてください。[アセットのインポートとエディタ連携](usage_asset_pipeline.md) を参照）と、そこに書かれた出力フォルダ（既定は `ssab_generated/` で、何も変換していなければ空）です。どちらも、何かがおかしいという印ではありません。

---

## 確認 1: 数値

次を `smoke_check.gd` という名前でプロジェクト内のどこかに保存し、先頭の 2 つの定数を書き換えます。

```gdscript
extends SceneTree

const SSAB_PATH := "res://ssab_generated/Sample/Sample.ssab"
const ANIMATION := "anime_1"

var failures := 0


func check(condition: bool, what: String) -> void:
    print(("ok    " if condition else "FAIL  ") + what)
    if not condition:
        failures += 1


func _initialize() -> void:
    # このファイルには拡張の型名を書かない。拡張が読み込まれていないプロジェクトでも
    # パーサで死なずにこの確認まで届く（パース失敗は終了ステータス 0 で終わる）。
    if not ClassDB.class_exists("SpriteStudioPlayer2D"):
        print("FAIL  SpriteStudioPlayer2D is not registered -- the extension did not load")
        quit(2)
        return

    var ssab = load(SSAB_PATH)
    check(ssab != null, "loaded " + SSAB_PATH)
    if ssab == null:
        quit(2)
        return

    var player = ClassDB.instantiate("SpriteStudioPlayer2D")
    player.set_animation_process_mode(2)  # ANIMATION_PROCESS_MANUAL: advance() でしか進まない
    root.add_child(player)
    player.set_ssab_resource(ssab)

    # get_current_animation() は、存在しない名前でも渡された名前をそのまま返す。
    # そのため名前はアセット自身の一覧と照合する。
    check(ANIMATION in player.get_animation_names(), "animation '%s' exists" % ANIMATION)
    player.set_animation(ANIMATION)
    check(player.get_total_frames() > 0, "total frames > 0 (%d)" % player.get_total_frames())

    player.set_loop_count(1)
    player.play()
    var before: float = player.get_frame_no()
    player.advance(0.1)
    check(player.get_frame_no() > before, "advance(0.1) moved the playhead (%s -> %s)" % [before, player.get_frame_no()])

    player.free()
    if failures == 0:
        print("SMOKE PASS")
    quit(1 if failures > 0 else 0)
```

実行し、最後の行で合否を決めます。

```bash
out=$(godot --headless --path . --quit-after 600 --script res://smoke_check.gd 2>&1); echo "$out"
echo "$out" | grep -q '^SMOKE PASS$' && ! echo "$out" | grep -qE '^(SCRIPT ERROR|ERROR|WARNING):'
```

この最後の行の終了ステータスが判定です。`0` なら合格です。

### 失敗したとき、どう見えるか

| 出力 | 意味 |
| --- | --- |
| `SMOKE PASS` があり、`ERROR` 行が無い | 拡張が読み込まれ、アニメーションが存在し、再生ヘッドが動いた。 |
| `FAIL  SpriteStudioPlayer2D is not registered` | 拡張が読み込まれていない。[トラブルシューティング](../troubleshooting.md#ノードが見つからない) を参照。Godot が一度も開いていないプロジェクトなら、[先にインポート](#初回の前に-プロジェクトを-1-度インポートする) する。 |
| `FAIL  animation 'x' exists` と `ERROR: [SS] … has no animation "x"` | その名前はパックに無い。[トラブルシューティング](../troubleshooting.md#アニメーション名が存在しない) を参照。 |
| `SCRIPT ERROR: Parse Error: Could not find type "SpriteStudioPlayer2D"` | 自作のスクリプトが拡張のクラスを型として書いていて、拡張が読み込まれていない。 |

### 誤解しやすい 3 点

* 終了ステータスは合否ではありません。Godot はスクリプトのパースに失敗しても `0` で終了します。拡張が読み込まれていないプロジェクトで `SpriteStudioPlayer2D` や `SSABResource` を型として書いたスクリプトが、まさにこれになります。上のスクリプトが拡張に `ClassDB` 経由でしか触れないのも、合格を終了ステータスではなく `SMOKE PASS` の行で判定するのも、そのためです。この行が無ければ失敗です。
* `get_current_animation()` は、そのアニメーションが存在する証拠になりません。`set_animation("nope")` の後でも `"nope"` を返し、`get_total_frames()` は直前のアニメーションの値のまま、ノードは何も描画しません。ほかの手がかりは `ERROR: [SS] …` の行だけです。名前は `get_animation_names()` と照合し、出力に `ERROR` の行があれば失敗として扱ってください。上のコマンドはそうしています。
* 手で進めます。`set_animation_process_mode(2)` は `ANIMATION_PROCESS_MANUAL` で、自分で呼ぶ `advance(delta)` 以外では何も動かないため、結果はフレームにかかった時間に左右されません。数値で書いているのは、`SpriteStudioPlayer2D.ANIMATION_PROCESS_MANUAL` と書くとクラス名を名指しすることになるからです。`--quit-after 600` はフレーム数で、`quit()` に届かないスクリプトへの保険にすぎません。

---

## 確認 2: ピクセル

こちらは実際のレンダラーが必要なので、`--headless` を**付けずに**実行し、ウィンドウが一瞬開きます。`snapshot.gd` という名前で保存し、定数を書き換えます。

```gdscript
extends SceneTree

const SSAB_PATH := "res://ssab_generated/Sample/Sample.ssab"
const ANIMATION := "anime_1"
const FRAME := 8.0
const OUT := "res://snapshot.png"


func _initialize() -> void:
    var player = ClassDB.instantiate("SpriteStudioPlayer2D")
    player.set_animation_process_mode(2)  # ANIMATION_PROCESS_MANUAL
    root.add_child(player)
    player.set_ssab_resource(load(SSAB_PATH))
    player.set_animation(ANIMATION)
    player.position = Vector2(576, 560)
    # 特定のフレームを表示するには frame_no に代入する。
    player.frame_no = FRAME

    # これが返るまで、そのフレームは描画されていない。その前にビューポートを読むと
    # クリア色だけの画像になる。
    await RenderingServer.frame_post_draw
    var image := root.get_texture().get_image()
    image.save_png(ProjectSettings.globalize_path(OUT))

    # クリア色以外のピクセルがあれば、何かが描画されている。
    var background := image.get_pixel(0, 0)
    var drawn := 0
    for y in range(0, image.get_height(), 2):
        for x in range(0, image.get_width(), 2):
            if image.get_pixel(x, y) != background:
                drawn += 1
    print("snapshot %dx%d, %d sampled pixels differ from the background" % [image.get_width(), image.get_height(), drawn])
    if drawn > 0:
        print("SNAPSHOT PASS")
    quit(0 if drawn > 0 else 1)
```

```bash
out=$(godot --path . --quit-after 600 --script res://snapshot.gd 2>&1); echo "$out"
echo "$out" | grep -q '^SNAPSHOT PASS$' && ! echo "$out" | grep -qE '^(SCRIPT ERROR|ERROR|WARNING):'
```

画像はプロジェクトフォルダの `snapshot.png` で、サイズは `1152 x 648` です（スクリプトをこの方法で実行したときのビューポートの大きさです）。開けばそのフレームが見えます。ピクセル数は「何かが描画された」ことだけを見る粗い確認で、アニメーション名を間違えると `0` になります。基準画像との比較はしないので、見た目が正しいかどうかは PNG を読む人が判断します。

---

## 関連ページ

* [トラブルシューティング](../troubleshooting.md) — アニメーションが空・出ないときの原因。
* [SpriteStudioPlayer2D](../api/player.md) — 上で使ったメソッドの一覧。
