# 🗃️ リソース管理クラス

Godot の `Resource` を継承しているため、複数の `SpriteStudioPlayer2D` から同じリソースを参照する場合は **Local To Scene** フラグを `True` に設定すると個別に状態を持たせられます。

## SSABResource

`.ssab` (アニメバイナリ) に対応するリソースクラスです。

アニメーションが参照するテクスチャとサウンドは、**リソースを読み込んだディレクトリからの相対パス**で解決されます。ファイルを移動する際はコンバーターの出力ディレクトリの構成を崩さないでください。

主なメソッド:

* `load_from_file(path: String) -> Error` / `save_to_file(path: String) -> Error`
* `is_valid() -> bool`: 保持しているバイナリが検証を通るかどうか。バッファは読み込みの間は不変であるため、結果はキャッシュされます。
* `get_animation_count() -> int` / `get_animation_names() -> PackedStringArray`
* `get_cellmap_names() -> PackedStringArray` / `get_cell_names(cellmap_name: String) -> PackedStringArray`: プレーヤ側と同じ 2 つの参照を、まだプレーヤに載せていない `.ssab` に対して行えます。存在しないセルマップを指定した場合、`get_cell_names()` は空配列を返します。
* `get_sound_stream(sound_list_name_hash: int, sound_name_hash: int) -> AudioStream`: オーディオイベントに対応するストリーム。参照先のファイルが無い / 非対応の場合は `null`。2 つのハッシュは `audio` シグナルの payload から得られます。結果はリソースごとにキャッシュされます（見つからなかった結果も含むため、欠損ファイルをイベントのたびに解決し直すことはありません）。
* `get_sound_info(sound_list_name_hash: int, sound_name_hash: int) -> Dictionary`: 同じ参照を、何も読み込まずに行います。`alias`（SpriteStudio 上で設定した名前）/ `file_path`（このリソースからの相対パス）/ `path`（解決済みのプロジェクトパス）/ `file_path_hash` / `time_total` を返します。該当が無い場合は空の `Dictionary` です。

2 つのサウンド参照の使い方は [サウンド再生](../workflow/audio.md) を参照してください。

## SSQBResource

`.ssqb` (シーケンスバイナリ) に対応するリソースクラスです。

主なメソッド:

* `load_from_file(path: String) -> Error`
* `save_to_file(path: String) -> Error`
