# AnimationPlayer との連携 (シネマティクスとステートマシン)

Godot 標準の `AnimationPlayer` と組み合わせることで、SSPlayer のアニメーション再生を Godot の強力なタイムライン機能やステートマシン機能と連携させることができます。

これにより、**攻撃判定（Hitbox）の同期、効果音（SE）の再生タイミング、カメラの揺らし、カットシーンの作成**などが、ノーコードで視覚的に行えるようになります。

---

## 1. アニメーションライブラリの生成

SSPlayer を `AnimationPlayer` で制御するためには、まず対象の `.ssab` に含まれるすべてのアニメーション（walk, attack など）を Godot が読み込める `AnimationLibrary`（`_anims.res`）に変換する必要があります。

1. Godot エディタ上で、ファイルシステムから `.ssab`（または `.ssqb`）ファイルを選択します。
2. インスペクタの下部にある **「Gen AnimLib (AnimationPlayer用ライブラリを生成)」** ボタンをクリックします。
3. 成功すると、同じフォルダに `[元のファイル名]_anims.res` というファイルが生成されます。

> [!NOTE]
> 生成された `_anims.res` の中には、元の SpriteStudio アニメーションと同じ名前の「Value Track（値トラック）」が自動生成されています。このトラックは対象ノードの `animation`（アニメーション名）と `frame`（現在のフレーム）を制御します。

---

## 2. AnimationPlayer のセットアップ

生成されたライブラリを実際にシーンへ適用する手順です。

1. **ノードの準備**
   シーン上に `SpriteStudioPlayer2D` ノードを配置し、対象の `.ssab` を `SSAB Resource` にセットしておきます。
2. **AnimationPlayer の追加**
   シーン内に `AnimationPlayer` ノードを追加します。
3. **ターゲットの指定（重要！）**
   追加した `AnimationPlayer` ノードを選択し、インスペクタ内の **`Root Node`** を、手順1の **`SpriteStudioPlayer2D` ノード** に設定します。
   *※自動生成されたアニメーションは「対象ノード自身の `animation` と `frame` を制御する」ようになっているため、Root Node を対象ノードに向ける必要があります。*
4. **ライブラリの読み込み**
   エディタ下部の「アニメーション (Animation)」パネルを開き、「アニメーション」メニュー ＞ **「アニメーションを管理... (Manage Animations)」** をクリックします。
   出てきたウィンドウのフォルダアイコン（ライブラリをロード）を押し、生成された `_anims.res` を読み込みます。

これで AnimationPlayer のドロップダウンに、SpriteStudio で作成したアニメーション一覧が表示されるようになります！

---

## 3. 応用例

AnimationPlayer と連携できるようになると、以下のような高度な制御が可能になります。

### A. イベントや効果音との同期（Call Method Track / Audio Track）
AnimationPlayer のタイムライン上で「トラックを追加」し、`Call Method Track` や `Audio Playback Track` を追加します。
「剣を振り下ろしたフレーム」に合わせて効果音を鳴らしたり、GDScript の `enable_hitbox()` 関数を呼び出したりするタイミングを、目で見ながら完璧に合わせることができます。

### B. ステートマシン（AnimationTree）の利用
シーンに `AnimationTree` ノードを追加し、`Anim Player` に上記の AnimationPlayer を割り当てます。
`Tree Root` に `AnimationNodeStateMachine` を設定することで、Idle（待機）→ Walk（歩き）→ Attack（攻撃）といった状態遷移を、コードではなくノード同士を矢印で繋ぐUI（ステートマシン）で構築できるようになります。

### C. カットシーン・シネマティクス
カメラ (`Camera2D`) の位置を動かすトラックや、背景の色を変えるトラックなどを追加することで、キャラクターの動きと完全に連動したイベントシーン（カットシーン）をひとつのタイムライン上で作成できます。
