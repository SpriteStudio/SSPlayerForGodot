# アセットのインポートとエディタ連携

このページでは **`.sspj` を Godot プロジェクトへ取り込む（コンバートする）初回手順** と、その後 SpriteStudio と Godot を行き来しながら高速にイテレーションを回すためのワークフローを解説します。

SpriteStudioPlayerForGodot は単にアニメーションを再生するだけでなく、**SpriteStudio と Godot をシームレスに行き来できる強力なアセットパイプライン**を提供します。これにより、アニメーションの修正からゲームへの反映までのイテレーションを高速に回すことができます。

---

## 1. 初回のインポート (SS Import Dock を使ったコンバート)

まだ Godot プロジェクト内に `.ssab` が存在しない場合は、まず SpriteStudio のプロジェクトファイル (`.sspj`) をインポート（コンバート）します。

1. Godot エディタ右側の **「SS Import」ドック** を開きます。
2. 出力先（デフォルトは `res://ssab_generated`）を確認します。
3. エクスプローラーや Finder から、目的の **`.sspj` ファイルを「SS Import」ドックへドラッグ＆ドロップ** します。

これだけでコンバート処理が走り、指定した出力先に `.ssab` と `.ssqb` が生成されます。
生成された `.ssab` を 2D ビューポートへドラッグ＆ドロップして、あらかじめ `SpriteStudioPlayer2D` ノードを配置しておきましょう（詳細は [基本的な使い方](usage_basic.md) を参照）。

> [!TIP]
> <video autoplay loop muted playsinline width="100%">
>   <source src="../../assets/sspj_import.webm" type="video/webm">
> </video>
> *(※上記に .sspj を SS Import ドックへドラッグ＆ドロップしてコンバートする様子の動画が入ります)*

---

## 2. Godot から SpriteStudio を直接開く

Godot で作業中に「もう少しアニメーションを微調整したい」と思った場合、わざわざ SpriteStudio を別で立ち上げてプロジェクトを探す必要はありません。

1. Godot エディタ上で、アニメーションを再生している **`SpriteStudioPlayer2D` ノードを選択**します。
2. インスペクタの **`SSAB Resource` プロパティ** をクリックして展開します。
3. リソースプレビューの横に用意されている **「Open SSPJ」ボタン** をクリックします。

この操作により、SpriteStudio が起動して元となる `.sspj` ファイルが開き、すぐに編集作業に入ることができます。

> [!NOTE]
> ![インスペクタからSSPJを開く画像](../../assets/open_sspj_from_inspector.png)
> *(※ここにインスペクタ上の「Open SSPJ」ボタンがハイライトされた画像が入ります)*

---

## 3. 強力なアセットパイプライン (シームレスなエディタ間連携)

SpriteStudio 側でアニメーションを修正して保存したら、Godot エディタに戻ります。
以下の操作だけで、変更内容がゲーム（およびエディタ上のプレビュー）に一瞬で反映されます。

1. 先ほどと同様に、インスペクタの **`SSAB Resource`** を展開します。
2. 「Open SSPJ」ボタンの横にある **「Reconvert」ボタン** をクリックします。

> [!IMPORTANT]
> **「開く」→「保存」→「再コンバート」の最短ワークフロー**
> ノードを選択したままインスペクタから SpriteStudio を呼び出し、編集後すぐに Godot 上で再コンバートをかけることができます。**SpriteStudio と Godot をシームレスに行き来できるこの強力なアセットパイプライン**により、アニメーション調整におけるトライアンドエラーのコストが劇的に下がります。

> [!TIP]
> <video autoplay loop muted playsinline width="100%">
>   <source src="../../assets/fast_iteration_inspector.webm" type="video/webm">
> </video>
> *(※上記に「インスペクタから開く -> SpriteStudioで保存 -> インスペクタでReconvert -> プレビューが即座に変わる」という一連の高速ワークフロー動画が入ります)*

---

## 制限事項とチーム開発時の注意点

この強力なエディタ間連携機能（`Open SSPJ` および `Reconvert`）は非常に便利ですが、その仕組み上、いくつかの制限があります。特にチーム開発を行う場合は注意してください。

> [!WARNING]
> **SpriteStudio のインストールが必要です**
> `Open SSPJ` ボタンを押すと SpriteStudio が起動します。そのため、操作するPC自体に [OPTPiX SpriteStudio 7](https://www.webtech.co.jp/spritestudio/) がインストールされている必要があります。

> [!TIP]
> **チーム開発時のファイルパス共有とスマート再リンク**
> 
> SS Import Dock経由でインポートした際の連携情報（`.sspj` のファイルパス）は、Godot プロジェクト直下の `.ssplayer_sources.cfg` に **相対パス** で保存されます。
> 
> `.ssplayer_sources.cfg` を Git 等のバージョン管理システムで共有することで、チームの別メンバーがPCに `git clone` した場合でもパスの連携が維持され、そのまま `Open SSPJ` や `Reconvert` を利用できます。
> 
> 万が一、フォルダ構造の変更などにより `.sspj` へのリンクが切れてしまった場合でも簡単に修復できます。Godotのファイルシステムドックでリンク切れの `.ssab` を右クリックして「Reconvert（再変換）」を選ぶと、新しい `.sspj` の場所を尋ねるダイアログが表示されます。ここで1つのファイルを再リンクするだけで、同じフォルダ内にある他の関連ファイルも**全自動でスマート再リンク**されるため、修復の手間がかかりません。
