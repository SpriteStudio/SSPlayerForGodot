# インストール

SpriteStudioPlayer for Godot を使い始めるための手順です。

## A. SSPlayerForGodot の GDExtension を利用する (推奨)

ビルド作業なしでプラグインを利用できる最短の手順です。

1. [公式サイト](https://godotengine.org/download/) より 4.6 系の Godot Engine をダウンロードします。
2. [SSPlayerForGodot の Releases](https://github.com/SpriteStudio/SSPlayerForGodot/releases) から該当プラットフォーム向けの GDExtension 一式をダウンロードします。
3. ダウンロードした ZIP を解凍し、中にある `addons` フォルダをそのまま Godot プロジェクトのルートディレクトリにコピーします。
   * 正しく配置されると、`res://addons/spritestudio/spritestudio.gdextension` が存在する状態になります。
4. Godot エディタを再起動すると `SpriteStudioPlayer2D` ノードや SS Import Dock が利用可能になります。

## B. SSPlayerForGodot のカスタムモジュールを組み込んだ Godot Engine を利用する

カスタムモジュールとしてビルドして利用する場合は、[ビルドガイド](./build.md) を参照してください。
