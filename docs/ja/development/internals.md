# 🧩 プレイヤーの構成

このページは、SpriteStudio Player for Godot を開発する人と、自分のプロジェクトのために改造する人に向けたものです。サイトのほかのページより読み手に求めるものが多く、ファイル名を挙げて説明し、ビルドの通るチェックアウトがある前提で書いています。ツールチェーン・ビルド・エクスポート・デバッガは[ビルドガイド](../setup/build.md)に、制約とテストスイートは [CONTRIBUTING](https://github.com/cri-middleware/SSPlayerForGodot/blob/HEAD/CONTRIBUTING.md) にあります。このページは、それらをまたぐ地図です。

SpriteStudio の Player に共通すること（再生・モーションブレンド・サウンドキー・パーツトラッキングの振る舞い）は、SDK の [Player Porting Guide](https://github.com/cri-middleware/SpriteStudio-SDK/blob/HEAD/libs/ssruntime/docs/README.md) に一度だけ書いてあります。

## 1. 地図

プレイヤーはすべて `ss_player/` の下の C++ です。同じ 1 つのソースから 2 通りにビルドします。`godot-cpp` の上の **GDExtension**（`scripts/build-extension.sh`、`SConstruct`）と、エンジンに組み込む **カスタムモジュール**（`scripts/build.sh`、`ss_player/SCsub`）です。`#ifdef SPRITESTUDIO_GODOT_EXTENSION` のブロックが持つのは、2 つで異なる include と型の変換だけです。両方のクラス登録は `register_types.cpp` が行います。

### 再生

| ファイル | 役割 |
|---|---|
| `ss_player_node_2d.*` | ノードの `SpriteStudioPlayer2D`。プロパティ・メソッド・シグナルと、`_bind_methods` で登録するインスペクタのプロパティ |
| `ss_internal_player.*` | ノードを持たない再生コアの `SsInternalPlayer`。C API で `libssruntime` を動かし、各フレームの描画バッチをたどり、`RenderingServer` で 1 つのルートのキャンバスアイテムに描く。Instance パーツごとに子のプレイヤーを 1 つ持つ。マスク・エフェクト・パーツごとのメッシュもここにある。 |
| `ss_shader_setup.h`、`shaders/*.glsl` | キャンバスアイテムのシェーダー。マテリアルを作るときに GLSL の部品から組み立てる |
| `ss_audio_controller.*`、`ss_audio_backend.*` | 組み込みのサウンド再生と、それを引き継ぐ `SpriteStudioAudioBackend` リソース |
| `ss_part_attachment_2d.*` | パーツ 1 つに追従する、利用者のノード `SpriteStudioPartAttachment2D` |
| `ssab_resource.*`、`ssqb_resource.*` | `SSABResource` と `SSQBResource`、そのローダーとセーバー。`.ssab` の解析結果は、それに結び付いたすべてのプレイヤーで共有する。 |
| `runtime/` | `libssruntime` と `libssconverter` とその C ヘッダー。`download-sdk` か `build-runtime` が置く |
| `format/`、`flatbuffers/` | `.ssab` の読み取り。`generate-fbs-code` が生成し、ヘッダーだけで使う |

### エディタ

すべて `TOOLS_ENABLED` の中にあるので、エクスポートしたゲームには入りません。

| ファイル | 役割 |
|---|---|
| `ss_editor_plugin.*` | `SSEditorPlugin`。以下のすべてをエディタに足す |
| `ss_importer.*` | `SSImporter`。`libssconverter` で `.sspj` を変換し、出力フォルダごとの変換元を `res://.ssplayer_sources.cfg` に記録する |
| `ss_import_dock.*` | **SS Importer** ドック。ドロップ・最近使ったファイル・Reconvert |
| `ss_filesystem_menu.*` | ファイルシステムドックの右クリックメニュー |
| `ss_resource_inspector.*` | `SSABResource` のインスペクタ |
| `ss_playback_panel.*` | プレイヤーを選んだときに出る再生パネル |
| `ss_canvas_drop_overlay.*` | `.ssab` を 2D ビューポートにドロップするとプレイヤーができる |
| `ss_progress_dialog.*`、`ss_clickable_label.*` | ドックが使う小さなウィジェット |
| `ss_translation.*` | プラグインの日本語の文字列 |

### 周辺

| パス | 内容 |
|---|---|
| `misc/spritestudio.gdextension` | 拡張のマニフェスト。どの `addons/spritestudio/` にもコピーされる |
| `test_gdextension/` | ヘッドレスのスイート（`run_tests.gd`、`suites/`）。サンプルではない |
| `examples/` | サンプル。`dev_gdextension` と `dev_module` は開発用の作業プロジェクト |
| `godot/`、`godot-cpp/` | エンジンとバインディング。サブモジュールではなく、repo の横に clone する |

## 2. 変更したプレイヤーを使う

`scripts/build-extension.sh` はホスト向けの GDExtension をビルドし、`misc/spritestudio.gdextension`・アイコン・ライセンスと一緒に `examples/dev_gdextension/addons/spritestudio/` とほかのすべてのサンプルに入れます。そのフォルダを自分のプロジェクトの `addons/spritestudio/` にコピーし、プロジェクトを開き直してください。入っているのはホストのプラットフォームのバイナリだけです。ほかのプラットフォームはビルドガイドの[リリースビルド](../setup/build.md#リリースビルド)にあるプラットフォームごとのスクリプトで作ります。リリースの zip（`scripts/build-release.sh`）は、CI が作った全プラットフォームの成果物から組み立てるものです。

カスタムモジュールでは、エンジンそのものがプレイヤーです。`scripts/build.sh` がプレイヤーを組み込んだエディタをビルドし、リリース用のスクリプトがエクスポートテンプレートをビルドし、`scripts/install-template.sh` がそれをインストールします。

## 3. Godot で確かめる

`scripts/run-tests.sh` は GDExtension 版をヘッドレスで回します（規則は CONTRIBUTING にあります）。それで見えないものが 2 つあります。画素（`--headless` はダミーのラスタライザを入れるため）と、ツリーに入ったときに結線するもの（`run_tests.gd` はルートがツリーに入る前に全ケースを回すため）です。`SpriteStudioPartAttachment2D` と音がそれに当たります。CI は全プラットフォームの GDExtension とカスタムモジュールをコンパイルするだけで、テストは回しません。スイートは自分で回してください。エクスポート先ごとの確かめ方とデバッガの付け方は、ビルドガイドの[エクスポート機能のテスト・デバッグ方法](../setup/build.md#エクスポート機能のテストデバッグ方法)にあります。

### Godot の表記の引き方

ガイドでは、エディタのメニューとドックを Godot の表示どおりに書きます。英語の文字列はエンジンのソース（`godot/editor/`）に、日本語は `godot/editor/translations/editor/ja.po` にあります。プラグイン自身の文字列は `ss_translation.cpp` にありますが、エディタには表示されません。エディタはメインの翻訳ドメインを無効にし、自分の UI を `godot.editor` ドメインで翻訳するため、日本語のエディタでもドックとインスペクタは英語のままです。日本語のページでボタンの名前を英語で書いているのはこのためです。

### スクリプトから画素とツリーを確かめる

`--script` で走らせる `SceneTree` のスクリプトは、実際に描画し、ツリーにも入ります。

```bash
rsync -a --exclude 'ssab_generated/*' examples/dev_gdextension/ <scratch>/proj/
rsync -a examples/dev_gdextension/ssab_generated/<pack> <scratch>/proj/ssab_generated/
cd <scratch>/proj && <godot> --path . --script res://probe.gd
```

プロジェクトを `.godot/` ごとコピーすると、再インポートせずに `load()` が通ります。

- ノードを足すのは `await process_frame` の後です。`_initialize` の直後はまだツリーができていません。
- `await RenderingServer.frame_post_draw` の後で `get_root().get_texture().get_image().save_png(…)` するか、`Image.get_pixel` で画素を読みます。
- プレイヤーの `animation_process_mode` を `ANIMATION_PROCESS_MANUAL` にして `frame_no` を代入します。代入は必ず描き直しますが、同じフレームでの `advance(0.0)` は描き直さないことがあります。
- テクスチャを読めないエフェクトは描かれません。SDK のテストプロジェクトと比べるときは、作業用のプロジェクトに `.ssab` を元の画像と同じ階層に置き、`<godot> --headless --path <proj> --import` で一度取り込みます。
- 1 つを除くすべてのパーツに `set_part_visibility_override` を掛けると、そのパーツが何を描いているかが分かります。
- レンダラーの比較は `--rendering-method forward_plus` で行います。`dev_gdextension` の既定は Compatibility、`test_gdextension` 由来のプロジェクトは Forward+ です。
- ライトマスクは `PointLight2D` で確かめます。`DirectionalLight2D` は `range_item_cull_mask` に関係なく `light_mask = 2` の `Sprite2D` を照らすので、プレイヤーの横に対照の `Sprite2D` を置いてください。

### スクリプトからエディタを確かめる

`addons/spritestudio`（`.uid` ファイルは除く）と `.ssab`、`project.godot` で有効にした（`[editor_plugins] enabled=`）`@tool` の EditorPlugin を置いた作業用のプロジェクトで、プラグインのエディタ UI を動かせます。先に一度 `--headless --editor --quit` を実行して、`extension_list.cfg` を書かせてください。

- **再生パネル:** `EditorInterface.open_scene_from_path`、`edit_node` のあと、`get_base_control().find_child("SSPlayback", true, false)` で掴みます。元に戻す・やり直すは `get_undo_redo().get_history_undo_redo(get_object_history_id(node))` を通します。
- **インポートドック:** `find_child("SSPJ")` のあと、`_on_recent_file_pressed(<sspj>)` を呼びます。
- **ファイルのドロップ:** `_on_window_files_dropped` はマウスがドックの上にあるかを見ますが、ヘッドレスには OS のカーソルがありません。`Input.parse_input_event` でドックの中心に `InputEventMouseMotion` を置き、`Input.flush_buffered_events()` のあと `get_tree().root.files_dropped.emit([<dir>])` を呼びます。ダイアログは表示中の `AcceptDialog` で、ボタンは `pressed.emit()` で押します。
- **ファイルシステムのメニュー:** `EditorInterface.select_file(<path>)` のあと、ドックの最初の `Tree` で右ボタンの `item_mouse_selected` を emit すると、ドックの `PopupMenu` が埋まります。項目は `id_pressed.emit(<id>)` で押します。プラグインの項目は、2000 番台が `_get_menu_options`、3000 番台が `_popup_menu` から来るので、どちらの経路を通ったかが ID で分かります。
- **自分の設定を変えずに別の言語のエディタを見る:** `Godot.app` をコピーし（APFS なら `cp -Rc`）、コピーの横に空の `._sc_` を置きます。これで自己完結型になり、設定はその隣の `editor_data/editor_settings-4.7.tres` に入るので、そこで `interface/editor/localization/editor_language` を設定します。`--headless` を付けずに起動し、スクリーンショットはプローブから撮ります。`TabContainer.get_tab_title()` は翻訳前の文字列を返します。

### クラッシュしたとき

公式のエディタには lldb で接続できません（hardened runtime）。シンボル付きのエンジンをビルドし（`godot/` で `scons platform=macos target=editor dev_build=yes scu_build=yes arch=arm64`。モジュール版の横に `bin/godot.macos.editor.dev.arm64` ができます）、`--disable-crash-handler` を付けて、`lldb --batch -o run -k "bt all"` で実行します。拡張は `build-extension.sh debug_symbols=yes` でビルドしてください。シンボルが無いと、`atos` でも `libSSGodot` のフレームに名前が付きません。

プレイヤーのせいではないクラッシュが 2 つあります。1 つは、ビルド直後や新しい clone でのスイートの最初の import pass が、`Parameter "singleton" is null` の後に終了処理で落ちるものです。バックトレースに拡張のフレームは無く、2 回目は通ります。もう 1 つは `godot/bin/` のエディタで、これはモジュール版なので、GDExtension を読み込むとすべてのクラスが二重に登録されます（"already registered"）。GDExtension には `scripts/fetch-godot.sh` が置く `godot-bin/` を使ってください。

### SDK の変更を試す

`build-runtime` はサブモジュールからビルドするので、サブモジュールを SDK のブランチに向けて `scripts/build-runtime.sh` を実行します。手元のブランチなら push は要りません（`ss_player/SpriteStudio-SDK` の中で `git remote add local-sdk <SDK のチェックアウト>` を実行し、ブランチを fetch して checkout します）。そのあと、GDExtension なら `build-extension.sh`、モジュールなら `build.sh` でビルドし直してください。しないとモジュールは古い `libssruntime.a` のままです。終わったらサブモジュールを元に戻してください。

## 4. この作りになっている理由

ここに挙げる理由はどれも複数のファイルにまたがります。詳細は、それぞれが指すコードにあります。

**再生コアはノードを持ちません。** `SsInternalPlayer` はルートのキャンバスアイテムを持って `RenderingServer` で描き、`SpriteStudioPlayer2D` がそれを包みます。100 パーツのアニメもシーンツリーでは 1 つのノードで、コアはツリーに依存しません。要るのは親のキャンバスアイテムと、イベントの送り先だけです。`SpriteStudioPartAttachment2D` があるのは、パーツへの追従が、利用者が自分のノードを必要とする唯一の場面だからです。

**マスクはステンシルではなくカバレッジのビットマップです。** Godot のキャンバスにはプラグインが使えるステンシルバッファが無いため、毎フレーム、マスクの書き手をオフスクリーンのビットマップに 1 つ 1 ビットで描き、マスクされる各パーツのシェーダーがそれを読み戻します（`ss_internal_player.h` の `MaskWriter` と `_build_mask_writers`）。1 枚のビットマップがインスタンスツリー全体を受け持ち、その持ち主は最上位のプレイヤーです。ビットは RGB に詰め、1 タイルに 24、最大 4 タイルなので、1 フレームに書き手は 96 までです。それを超えた書き手は警告を 1 回出して捨て、それ以上が要るツリーは受け入れるのではなく組み直してもらいます。ビットマップはプロセス全体で 1 つのプールから借りるので（`ss_internal_player.cpp` の `SsMaskCoveragePool`）、ビデオメモリは、これまでにマスクしたプレイヤーの数ではなく、同時にマスクしているプレイヤーの最大数で決まります。

**音はプレイヤーが鳴らし、そのボイスはプールします。** ランタイムが伝えるのは、サウンドキーを通過したことだけです。このプレイヤーはそれを自分で鳴らします。Unity・Web・Ren'Py のプレイヤーもそれぞれのエンジンで同じことをし、Flutter・React Native・wgpu のプレイヤーは通知をアプリに渡します。音は 1 つごとに新しい `AudioStreamPlayer` で、必要になったときにプレイヤーの内部の子として作ります。シーンツリーには出ず、保存もされません。指定の回数を鳴らし終えたらプールに戻すので、忙しいアニメでもノードを作っては捨てることになりません（`ss_audio_controller.*`）。アニメを一時停止しても、音は最後まで鳴ります。それがすべての Player が守る規則である理由は、SDK のポーティングガイドにあります。`SpriteStudioAudioBackend` を割り当てると、組み込みの再生は完全に止まります。

**名前では使えるのに、どこにも保存されないプロパティがあります。** `frame_rate`・`playback_direction`・`playback_style` は `_set` / `_get` で扱い、プロパティの一覧からは外しています（`ss_player_node_2d.cpp`）。スクリプトからは設定できますが、インスペクタにも保存したシーンにも載りません。スクリプトで設定した値は、シーンを読み直すと消えます。

**出力フォルダごとの変換元は、プロジェクトに記録します。** `res://.ssplayer_sources.cfg` が出力フォルダごとにその `.sspj` を記録していて（`ss_importer.*`）、Reconvert・ファイルシステムのメニュー・`deploy-examples` が出どころを尋ねずにパックを作り直せるのはこのためです。ドックの最近使ったファイルは別の置き場で、コミットしない `.godot/` の下にあるエディタのプロジェクトメタデータなので、マシンごとのものです。

**モジュール版を守るのは、コンパイルが通ることだけです。** スイートが動かすのは GDExtension で、モジュール版を試すにはエンジンをビルドする必要があります。2 つのビルドが再生のロジックを 1 つ共有し、`#ifdef` のブロックが include と変換しか持たないのはこのためです（`run-tests.sh` のヘッダーにも同じことがあります）。
