# 🗃️ リソース管理クラス

Godot の `Resource` を継承しています。1 つのリソースは複数の `SpriteStudioPlayer2D` からそのまま共有できます。再生状態は各ノードが持ち、同じリソースを使うプレーヤはその解析結果を 1 つ共有します。**Local To Scene** はノードごとにバイト列と解析結果を複製してしまうため、オフのままにしてください。また、リソースの複製（Local To Scene、`duplicate()`、インスペクタでのユニーク化）は読み込み元のディレクトリを持たないため、テクスチャとサウンドを解決できません。

## SSABResource

`.ssab` (アニメバイナリ) に対応するリソースクラスです。

アニメーションが参照するテクスチャとサウンドは、**リソースを読み込んだディレクトリからの相対パス**で解決されます。ファイルを移動する際はコンバーターの出力ディレクトリの構成を崩さないでください。

主なメソッド:

* `load_from_file(path: String) -> Error` / `save_to_file(path: String) -> Error`
* `data: PackedByteArray`: 保持しているバイナリ。インスペクタには表示されない保存用プロパティです。代入すると `load_from_file()` と同じようにバイナリを置き換えますが、テクスチャとサウンドは引き続きリソースを読み込んだディレクトリから解決します。`.ssab` として使えないバイト列は保持されず、エラーが出力されます。リソースをその場で読み直す処理はこのプロパティを通るため、ディスク上で変更されたファイルをエディタが取り込めます。`CACHE_MODE_REPLACE` を指定した `ResourceLoader.load()` も同様です。
* `is_valid() -> bool`: 保持しているバイナリが検証を通るかどうか。バッファは読み込みの間は不変であるため、結果はキャッシュされます。
* `get_animation_count() -> int` / `get_animation_names() -> PackedStringArray`
* `get_cellmap_names() -> PackedStringArray` / `get_cell_names(cellmap_name: String) -> PackedStringArray`: プレーヤ側と同じ 2 つの参照を、まだプレーヤに載せていない `.ssab` に対して行えます。存在しないセルマップを指定した場合、`get_cell_names()` は空配列を返します。
* `get_canvas_size(anim_name: String) -> Vector2` / `get_canvas_rect(anim_name: String) -> Rect2`: 指定したアニメーションがオーサリングされたキャンバスと、それを pivot で原点に対して配置した矩形。プレーヤ側は再生中のアニメーションについて同じ 2 つを返しますが、こちらはアセット内の任意のアニメーションを名前で参照できます（キャンバスはアニメーション単位なので、比較するにはこちらが必要です）。存在しないアニメーションを指定した場合は `Vector2.ZERO` と空の `Rect2` を返します。
* `get_sound_stream(sound_list_name_hash: int, sound_name_hash: int) -> AudioStream`: オーディオイベントに対応するストリーム。参照先のファイルが無い / 非対応の場合は `null`。2 つのハッシュは `audio` シグナルの payload から得られます。結果はリソースごとにキャッシュされます（見つからなかった結果も含むため、欠損ファイルをイベントのたびに解決し直すことはありません）。
* `get_sound_info(sound_list_name_hash: int, sound_name_hash: int) -> Dictionary`: 同じ参照を、何も読み込まずに行います。`alias`（SpriteStudio 上で設定した名前）/ `file_path`（このリソースからの相対パス）/ `path`（解決済みのプロジェクトパス）/ `file_path_hash` / `time_total` を返します。該当が無い場合は空の `Dictionary` です。

2 つのサウンド参照の使い方は [サウンド再生](../workflow/audio.md) を参照してください。

## SSQBResource

`.ssqb` (シーケンスバイナリ) に対応するリソースクラスです。

主なメソッド:

* `load_from_file(path: String) -> Error`
* `save_to_file(path: String) -> Error`
* `data: PackedByteArray`: 保持しているバイナリ。`SSABResource.data` と同じ保存用プロパティです。`.ssqb` として使えないバイト列は保持されず、エラーが出力されます。
