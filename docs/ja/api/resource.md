# リソース管理クラス

Godot の `Resource` を継承しているため、複数の `SpriteStudioPlayer2D` から同じリソースを参照する場合は **Local To Scene** フラグを `True` に設定すると個別に状態を持たせられます。

## SSABResource

`.ssab` (アニメバイナリ) に対応するリソースクラスです。

主なメソッド:

* `load_from_file(path: String) -> Error`
* `save_to_file(path: String) -> Error`
* `is_valid() -> bool`
* `get_animation_count() -> int`
* `get_animation_names() -> PackedStringArray`

## SSQBResource

`.ssqb` (シーケンスバイナリ) に対応するリソースクラスです。

主なメソッド:

* `load_from_file(path: String) -> Error`
* `save_to_file(path: String) -> Error`
