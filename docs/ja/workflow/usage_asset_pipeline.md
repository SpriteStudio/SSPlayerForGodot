# エディタ連携とアセットイテレーション

SpriteStudioPlayerForGodot は、単にアニメーションを再生するだけでなく、**「SpriteStudio と Godot をシームレスに行き来できる強力なアセットパイプライン」**を提供します。
これにより、アニメーションの修正からゲームへの反映までのイテレーションを爆速で回すことができます。

---

## 1. 初回のインポート (SS Import Dock を使ったコンバート)

まだ Godot プロジェクト内に `.ssab` が存在しない場合は、まず SpriteStudio のプロジェクトファイル (`.sspj`) をインポート（コンバート）します。

1. Godot エディタ右側の **「SS Import」ドック** を開きます。
2. 出力先（デフォルトは `res://ssab_generated`）を確認します。
3. エクスプローラーや Finder から、目的の **`.sspj` ファイルを「SS Import」ドックへドラッグ＆ドロップ** します。

これだけでコンバート処理が走り、指定した出力先に `.ssab` と `.ssqb` が生成されます。

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

この操作により、アタッチされている `.ssab` の元となる `.sspj` ファイルが OS の関連付けを通じて自動的に開かれ、すぐに SpriteStudio での編集作業に入ることができます。

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
> **1. SS Import Dock 経由でのインポートが必須です**
> この機能は、SS Import Dock でインポートした際に「元の `.sspj` ファイルがローカルPCのどこにあるか（絶対パス）」を記録することで実現しています。そのため、エクスプローラから手動で `.ssab` を `res://` にコピーしただけのリソースでは機能しません。
>
> **2. `git clone` した別PCでは機能しません**
> 上記の通りローカルの絶対パスに依存しているため、プロジェクトを Git 等でクローンして別のPCに持っていった場合、パスが変わってしまうためこれらのボタンは機能しなくなります。
>
> **3. SpriteStudio のインストールが必要です**
> `Open SSPJ` ボタンを押すと SpriteStudio が起動します。そのため、操作するPC自体に [OPTPiX SpriteStudio 7](https://www.webtech.co.jp/spritestudio/) がインストールされている必要があります。

これらの制限があるため、チーム開発環境や CI/CD パイプラインで `.ssab` を更新・共有したい場合は、CLI ツールを用いた一括コンバートが適しています。詳しくは [CLI コンバートと自動化](import.md) を参照してください。
