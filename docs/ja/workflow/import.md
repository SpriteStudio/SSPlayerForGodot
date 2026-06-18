# CLI ツールによるコンバートと自動化

Godot エディタの「SS Import Dock」を使った直感的なインポート方法は [アセットのインポートとエディタ連携](usage_asset_pipeline.md) を参照してください。

このドキュメントでは、Godot エディタを起動せずにコマンドラインから変換を行う方法を解説します。
これは CI/CD (継続的インテグレーション) やビルドパイプラインに変換処理を組み込みたい場合、あるいは大量のアセットを一括変換したい場合に非常に便利です。

## SpriteStudio-SDK の `ssconverter-cli` を使う

SpriteStudio プロジェクト (`.sspj`) から Godot 用のアニメーションバイナリ (`.ssab` / `.ssqb`) への変換は、SDK に同梱されている `ssconverter-cli` という独立したコマンドラインツールを使用して行われます（エディタ内蔵のインポータも裏ではこれを呼び出しています）。

> [!NOTE]
> `ssconverter-cli` バイナリは **SpriteStudio-SDK のリリース成果物にのみ同梱** されています。SSPlayerForGodot の GDExtension パッケージ（`addons/spritestudio/`）には含まれないため、CLI を利用する場合は SDK 側から別途取得してください。

### 1. ツールの取得

[SpriteStudio-SDK の Releases](https://github.com/cri-middleware/SpriteStudio-SDK/releases) から該当プラットフォーム (Windows / macOS / Linux) 向けの `ssconverter-cli` バイナリをダウンロードします。

### 2. コマンドラインからの実行

ターミナル（またはコマンドプロンプト）を開き、`.sspj` ファイルへのパスを引数に渡して実行します。

**Windows:**
```powershell
.\ssconverter-cli.exe path\to\your.sspj
```

**macOS / Linux:**
```bash
./ssconverter-cli path/to/your.sspj
```

### 3. 生成物の配置

変換結果は、元の `.sspj` と同じディレクトリに `<sspj 名>_ssab/` というフォルダが作成され、その配下に `.ssab` (アニメパック単位) および `.ssqb` (シーケンス単位) として出力されます。

生成されたファイルを Godot プロジェクトの `res://` 配下の任意のディレクトリにコピーすることで、Godot 上で `SSABResource` / `SSQBResource` として読み込めるようになります。

---

## 高度なオプション

`ssconverter-cli` には、変換時の挙動を制御するためのオプションが用意されています。
詳細なオプション一覧や仕様については、SDK リポジトリの [cli/README.ja.md](https://github.com/cri-middleware/SpriteStudio-SDK/blob/main/cli/README.ja.md) を参照してください。
