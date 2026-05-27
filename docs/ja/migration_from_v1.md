# v1.x からのマイグレーション

v1.x から本バージョンへ移行する場合は、以下の大きな変更点にご注意ください。

## 1. アセットフォーマットとインポート仕様の変更
- **v1.x**: `.sspj` や `.ssae`, `.ssce` などの SpriteStudio のファイル群を直接プロジェクトに配置し、Godot のファイルシステム (FileSystem) ドックからそのまま読み込んで使用していました。
- **本バージョン**: 独自の最適化されたバイナリ形式 `.ssab` を利用します。v1 の時に Godot プロジェクト内に配置していた `.sspj` などのファイル群は、一度プロジェクト外に退避してください。その後、**SS Import Dock** を利用して `.ssab` にコンバートして利用します。
  - コンバートの詳細な手順については [エディタ連携とアセットイテレーション](workflow/usage_asset_pipeline.md) を参照してください。

## 2. 利用するノードと GDScript API の変更
- **ノードの変更**: 利用するノードが変わります。旧バージョンの `GdNodeSsPlayer` ではなく、新しい `SpriteStudioPlayer2D` ノードを利用してください。
- **リソースの割り当て**:
  - 以前は `res_player.res_project = load("...sspj")` や `set_anime_pack("...ssae")` のように複数ファイルを設定していましたが、本バージョンでは `.ssab` が `.ssae` (アニメパック) 単位で生成されます。
  - そのため、利用する（再生したい）アニメーションが含まれている `.ssab` を `set_ssab_resource(load("...ssab"))` のようにセットしてください。
- **ループ再生の設定**: 以前の `set_loop(bool)` は廃止され、ループ回数を指定する `set_loop_count(int)` に変更されました（`-1` で無限ループ、`0` で1回再生（ループなし））。
- **メソッドの変更と廃止**:
  - `set_player_resource()` → `set_ssab_resource()`
  - `get_fps()` → `get_frame_rate()`
  - `set_anime_pack()` → 廃止（アニメパックは `.ssab` ごとに含まれるため不要）
  - `set_play()` / `get_play()` → 廃止（代わりに `play()`, `stop()`, `is_playing()` 等を利用してください）
  - `pause(bool)` → `pause()`（引数なしに変更）
  - `set_texture_interpolate()` → 廃止（Godot 4 標準の `Texture Filter` (CanvasItem プロパティ) を利用してください）
- **シグナル名の変更**: イベント名の先頭の `on_` が外れるなど、Godot の標準的な命名規則に統一されました。
  - `on_animation_finished` → `animation_finished`
  - `on_animation_changed` → `animation_changed`
  - `on_user_data` → `user_data`
  - 詳細は [API リファレンス](api/player.md) をご確認ください。
