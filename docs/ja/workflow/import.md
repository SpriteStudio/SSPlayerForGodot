# SpriteStudio データのインポート

SpriteStudio のプロジェクト (`.sspj`) は **`.ssab` (アニメバイナリ)** および **`.ssqb` (シーケンスバイナリ)** へ変換し、Godot プロジェクト配下に配置することで `SpriteStudioPlayer2D` から利用できます。
変換方法は以下の2通りです。どちらの方法で生成したファイルも同じく `SSABResource` / `SSQBResource` として読み込めます。

## 方法 A: Godot エディタの SS Import Dock を使う

エディタを起動するとプロジェクトドック側に SpriteStudio 用のインポートコントロールが追加されます。
最初に変換成果物の出力先を指定します。

* デフォルトの出力先: `res://ssab_generated`
* 設定キー (プロジェクト設定): `spritestudio/output_directory`

出力先はインポートコントロール上の `Browse` ボタンまたは入力欄から変更できます。

以下のいずれかの操作で変換が行われます。

* `.sspj` ファイルをエディタウィンドウへドラッグ＆ドロップする
* インポートコントロールに表示される履歴から再実行する

変換中は進捗ダイアログが表示され、完了後は出力先ディレクトリ配下に `.ssab` (アニメパック単位) と `.ssqb` (シーケンス単位) が生成されます。

## 方法 B: SpriteStudio7-SDK の `ssconverter-cli` を直接使う

[SpriteStudio7-SDK の Releases](https://github.com/SpriteStudio/SpriteStudio7-SDK/releases) から該当プラットフォーム向けの `ssconverter-cli` をダウンロードし、コマンドラインから `.sspj` を変換します。

**Windows:**
```powershell
.\ssconverter-cli.exe path\to\your.sspj
```

**macOS / Linux:**
```bash
./ssconverter-cli path/to/your.sspj
```

変換結果は `.sspj` と同じディレクトリの `<sspj 名>_ssab/` 配下に `.ssab` / `.ssqb` として生成されます。
このディレクトリ名を変更して Godot プロジェクト配下にコピーすると、`SSABResource` / `SSQBResource` として読み込めるようになります。

CI / ビルドパイプラインに変換処理を組み込みたい場合や、Godot エディタを起動せずに変換のみを行いたい場合に便利です。`ssconverter-cli` の詳細なオプションは [`SpriteStudio7-SDK/cli/README.ja.md`](https://github.com/SpriteStudio/SpriteStudio7-SDK/blob/main/cli/README.ja.md) を参照してください。
