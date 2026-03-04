# SpriteStudioPlayer for Godot

# はじめに

本プラグインは [OPTPiX SpriteStudio](https://www.webtech.co.jp/spritestudio/index.html) で作成したアニメーションを [Godot Engine](https://godotengine.org/) 上で再生するためのプラグインです。  

実行時パフォーマンスを優先するため C++ モジュールの形態になっています。
SpriteStudioPlayer for Godot を利用する場合、SpriteStudioPlayer の[カスタムモジュール](https://docs.godotengine.org/en/stable/contributing/development/core_and_modules/custom_modules_in_cpp.html) を組み込んだ Godot Engine の Editor を手元でビルドするか、 SpriteStudioPlayer の [GDExtension](https://docs.godotengine.org/en/stable/tutorials/scripting/gdextension/what_is_gdextension.html) をプロジェクトへ入れる必要があります。

## 対応する [OPTPiX SpriteStudio](https://www.webtech.co.jp/spritestudio/index.html) のバージョン

Ver.6 と Ver.7 に対応しています。  
ただし、Ver.7.1 で追加された新機能(テキスト、サウンド等)には未対応です。  

## 対応する [Godot Engine](https://github.com/godotengine/godot) のバージョン

- [4.4 ブランチ](https://github.com/godotengine/godot/tree/4.4)で Windows / macOS でビルド、および実行を確認しています。
- [3.x ブランチ](https://github.com/godotengine/godot/tree/3.x)で Windows / macOS でビルド、および実行を確認しています。

# ビルド

[BUILD.md](BUILD.md) を参照してください。

# 使い方

[USAGE.md](USAGE.md) を参照してください。

# サンプル

[examples フォルダ](./examples/)にサンプルプロジェクトがあります。

**サンプルプロジェクトは 3.x 向けに作っているため、4.4 で利用する際は "Convert Full Project" を実行してから利用してください。**

## [feature_test](./examples/feature_test)

基本機能のテストプロジェクトです。  
以下のシーンがあります。  

- [v6_feature.tscn](./examples/feature_test/v6_feature.tscn)
  - SpriteStudio v6.0~v7.0 までの各機能の再生状態を確認できます。
  - 確認したい機能ノードの可視性を有効にしてください。
  - Signal、UserData はそれぞれシグナル、ユーザーデータのキーに到達したタイミングでGodotのシグナルを発行するようになっています。
  - シーンを実行するとコンソールに受信したパラメータが出力されます。
  - ssdata サブフォルダにインポート元のプロジェクトファイル(v6_all.sspj)があります。
- [sspj_load.tscn](./examples/feature_test/sspj_load.tscn)
  - GDスクリプトからSSプロジェクトをロードしアニメーションを指定して再生するサンプルです。
  - アニメーション終了時に別のSSプロジェクトに読み替えるようになっています。
- [texture_change.tscn](./examples/feature_test/texture_change.tscn)
  - 再生中にセルマップリソースのテクスチャを変更するサンプルです。
  - インスペクタにある Change のチェックをOn/Offすると切り替わります。

## [mesh_bone](./examples/mesh_bone)

メッシュ、ボーン、エフェクトなどを利用したキャラクターアニメのサンプルです。

## [particle_effect](./examples/particle_effect)

エフェクト機能を利用したサンプルです。  
シーンに表示されるアニメーションは40種類のうちの一部になります。  
インスペクタの Anime Pack からその他のアニメーションも確認できます。

## [feature_test_gdextension](./examples/feature_test_gdextension)

gdextension の動作確認用プロジェクトです。

確認できるサンプルは [feature_test](./examples/feature_test) と同じものになります。

# お問い合わせ

ご質問、ご要望、不具合のご報告は [Issues](../../issues) に投稿してください。  
非公開でのお問い合わせを希望される場合は、[ヘルプセンター](https://www.webtech.co.jp/help/ja/spritestudio7/inquiries/ssplayer_tool/) よりお寄せください。  
再現データなどの送付が必要な場合も、上記ヘルプセンター経由でファイルを送信してください。  

=========================================================  

株式会社ＣＲＩ・ミドルウェア  
​https://www.cri-mw.co.jp/  
Copyright © CRI Middleware Co., Ltd.  

=========================================================  

* SpriteStudio, Web Technologyは、株式会社ウェブテクノロジの登録商標です。
* その他の商品名は各社の登録商標または商標です。  
