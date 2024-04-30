# SpriteStudioPlayer for Godot

# はじめに

本プラグインは OPTPiX SpriteStudio で作成したアニメーションを Godot エンジン上で再生するためのプラグインです。  
実行時パフォーマンスを優先するためC++モジュールの形態になっています。  
このため後述のビルド環境のセットアップが必要になります。  

### 対応する OPTPiX SpriteStudio のバージョン

Ver.6 と Ver.7 に対応しています。  
ただし、Ver.7.1 で追加された新機能(テキスト、サウンド等)には未対応です。  

### 対応する [Godot Engine](https://github.com/godotengine/godot) のバージョン

#### 2024/04/30時点の対応状況
- 3.x ブランチでWindows/Macでビルド、および実行を確認しています。
- 4.2 ブランチに現在対応中です。

# ビルド環境の構築

以降でビルド環境の構築手順について説明していきます。  

## ビルドツールのセットアップ

### Windows

Godot公式のコンパイル手順  
https://docs.godotengine.org/en/stable/contributing/development/compiling/compiling_for_windows.html
※4.x 系のページのみ存在 (2024/02/22時点)

必要なツール
* VisualStudio 2017 or 2019(推奨)
  * or MSYS2 + MinGW + gcc + make
* Python 3.6 以降
* scons

VisualStudio 2019 でのビルド・デバッグを確認しています。

sconsは下記でインストールできます。(上記リンクにも記載あり)

```
python -m pip install scons
```

### macOS

Godot公式のコンパイル手順  
https://docs.godotengine.org/ja/4.x/contributing/development/compiling/compiling_for_macos.html

必要なツール
* Xcode
* Python3.6 以降
* scons

scons はHomebrewでインストールができます。

https://brew.sh/

```
brew install scons
```

## Godot と SpriteStudio SDK の取得 (Windows/Mac 共通)

本リポジトリをクローンし、リポジトリルートから以下のコマンドを実行します。  

```bat
git clone https://github.com/godotengine/godot.git -b 3.x
git submodule update --init --recursive
popd
```

ルートにある setup.bat で上記を行っています。

## ビルド

### Windowsでのビルド  

```bat
pushd godot
scons platform=windows vsproj=yes compiledb=yes custom_modules="../gd_spritestudio"
popd
```
上記は makesln.bat に該当します。

### Macでのビルド  

`./godot` フォルダに移動し、ビルドするアーキテクチャに応じて下記を実行します。

#### Intel

```sh
scons platform=osx arch=x86_64 compiledb=yes custom_modules="../gd_spritestudio"
```

#### Apple Silicon

```sh
scons platform=osx arch=arm64 compiledb=yes custom_modules="../gd_spritestudio"
```

#### Universal

上記を二つをビルドし

```sh
lipo -create bin/godot.macos.tools.x86_64 bin/godot.macos.tools.arm64 -output bin/godot.macos.tools.universal
```

でアプリの形態にします。  
Godot 内にアプリのテンプレートが用意されているのでそちらを利用します。

`godot`フォルダ直下で下記を実行します。

```sh
rm -rf ./Godot.app
cp -r misc/dist/osx_tools.app ./Godot.app
mkdir -p Godot.app/Contents/MacOS
cp bin/godot.osx.tools.x86_64 Godot.app/Contents/MacOS/Godot
chmod +x Godot.app/Contents/MacOS/Godot 
codesign --force --timestamp --options=runtime --entitlements misc/dist/osx/editor.entitlements -s - Godot.app
```

`cp bin/godot.osx.tools.x86_64` の末尾の `x86_64` は、Apple Silicon の場合は、`arm64` に、
Universalの場合は`universal` に書き換えてください。

macbuild.sh には上述のビルドからアプリの形態にしてデフォルトの署名を施すまでの処理が記述されています。  
第一引数にアーキテクチャ(x86_64, arm64 )を指定できます。 
省略するとx86_64になります。  

`Godot.app` を開いて起動を確認します。

ビルド環境の構築については以上です。

## 使い方

### SpriteStudioデータのインポート

SpriteStudioデータのインポート手順について説明します。  
現在の SpriteStudio for Godot プラグインでは sspj ファイルを直接指定する形態になっています。  
ご利用のプロジェクト下のフォルダに sspj、ssae、ssce と画像ファイルなどの一式を配置します。  

### SpriteStudioノードの作成と sspj ファイルの指定

1. 「Node を新規作成」から「GdNodeSsPlayer」を選択し、「作成」ボタンを押します。
2. インスペクターの「Res Player」から「新規 GdResourceSsPlayer」を選択します。
3. GdResourceSsPlayer の「Res Project」から「読み込み」を選択します。
4. 「ファイルを開く」ダイアログから「*.sspj」ファイルを選択して開きます。

### アニメーションの指定

1. 「Animation Settings」を展開「Anime Pack」から再生させる「*.ssae」ファイルを選択します。
2. 続いて「Animation」から再生させるアニメーションを選択します。
3. Frame をドラッグしたり、Play フラグをオンにすることでプレビューできます。

#### インスペクターの各プロパティの意味

![image](./doc_images/ssnode_inspector.png)

```
GdNodeSsPlayer - SsPlayer を扱うノード
  └ Res Player - SsPlayer が使用するリソース
    └ Res Project - sspj ファイル
      └ CellMap Settings - セルマップの設定
        └ <ssce ファイル> - セルマップ
  └ Animation Settings - アニメーションの設定
    └ Anime Pack - アニメパック
    └ Animation - アニメーション
    └ Frame - 現在のフレーム
    └ Loop - ループ再生フラグ
    └ Play - 再生フラグ
```

## クラス

GDScript からコントロールできるクラスの役割と主要なメソッドについて説明します。  
各クラスが持っている全てのメソッド、プロパティ、シグナルについては Godot の Script 画面で確認してみてください。  

### リソース管理クラス

SpriteStudio の各種 .ss** ファイルに相当するリソースを管理するクラスがあります。

#### GdResourceSsProject クラス

１つの sspj ファイルに相当するリソースを取り扱うクラスです。
sspj に登録された ssae, ssce, ssee 各々のリソースの取得と設定を行います。

#### GdResourceSsCellMap クラス

１つの ssce ファイルに相当するリソースを取り扱うクラスです。
現在テクスチャの取得と設定を行うメンバーのみ対応しています。

#### GdResourceSsAnimePack クラス

１つの ssae ファイルに相当するリソースを取り扱うクラスです。
内包しているアニメーションの数、とアニメーション名の取得を行えます。

#### GdResourceSsPlayer クラス

現在 GdResourceSsProject リソースを設定、取得するアクセサのみです。

### 再生のためのクラス

GdNodeSsPlayer に前述のリソースを指定することで再生を行います。  
以下はファイルの読み込みから再生開始までのシンプルなサンプルコードです。  

```python

onready var ssnode = $target # GdNodeSsPlayerノードを指します。

func _ready():
  # sspj ファイルを読み込みます。
  ssnode.res_player.res_project = ResourceLoader.load("Sample.sspj")

  # ssaeファイルとアニメーションを指定します。
  ssnode.set_anime_pack("Sample.ssae")
  ssnode.set_animation("anime_1")
    
  # アニメーション終了時コールバックの設定
  ssnode.connect("animation_finished", self, "_on_animation_finished")

  # ループを指定して再生を開始します。
  ssnode.set_loop(true)
  ssnode.play()

# アニメ終了時のコールバック関数です
func _on_animation_finished(name):
	print("SIGNAL _on_animation_finished from " + name)
```

他にもカレントフレームの指定、一時停止、開始、終了フレーム、FPSの取得など一般的なメソッドがあります。  
一覧は Godot の Script 画面で確認してみてください。  

#### シグナル

GdNodeSsPlayer が発行するシグナルについて説明します。

#### on_animation_changed(name: String)

アニメーション変更時に発行されます。  
name：変更したアニメーション名

#### on_animation_finished(name: String)

再生中のアニメーションが終了フレームに到達した時に発行されます。  
ループ再生オンでも毎周発行されます。  
name：変更したアニメーション名

#### on_animepack_changed(name: String)

アニメパック変更時に発行されます。  
name：変更したアニメパック名  

#### on_frame_changed(frame: int)

フレーム位置変更時に発行されます。  
frame：変更したフレーム位置  

#### on_signal(command: String, value: Dictionary)

SpriteStudioのシグナルアトリビュートのキーフレーム到達時に発行されます。  
command：コマンド名  
value：パラメータ名と値のコレクション  

#### on_user_data(flag: int, intValue: int, rectValue: Rect2, pointValue: Vector2, stringValue: String)

ユーザーデータのキーフレーム到達時に発行されます。

- flag：後続の引数が有効かどうかの論理値
   - 1：整数が有効
   - 2：範囲が有効
   - 4：位置が有効
   - 8：文字列が有効
- intValue：整数
- rectValue：範囲
- pointValue：位置
- stringValue：文字列

## 制限事項

### 動作しないもの

- マスク機能
- 描画モード：ミックス以外もミックス相当になります。
- SpriteStudio Ver.7.1 で追加された新機能(テキスト・サウンド・9スライス・シェイプ)

### 表示が異なるもの

- パーツカラー
  - ミックスの頂点単位の時にX状(三角形の辺部分)の輝度が高くなっています。
  - 乗算でテクスチャカラーの割合がエディタより大きくなります。

### その他の制限

- インスタンスの独立動作がOnのパーツの再生について
  - シーン上で Frame プロパティが 0 以外の状態で Play プロパティを On にして再生開始した場合、独立動作がOnに設定されたインスタンスパーツを再生させた場合、インスタンスパーツの再生フレームがずれることがあります。
  - この場合、一旦 Frame プロパティの値を 0 以外に変更後、0 に戻してから再生開始することで一致させられます。
- シェーダー
  - SpriteStudio公式の一部のみ対応しています。
  - カスタムシェーダーは独自に追加・対応する必要があります。

## サンプル

examples フォルダにサンプルプロジェクトがあります。  

### feature_test

基本機能のテストプロジェクトです。  
以下のシーンがあります。  

- v6_feature.tscn
  - SpriteStudio v6.0~v7.0 までの各機能の再生状態を確認できます。
  - 確認したい機能ノードの可視性を有効にしてください。
  - Signal、UserData はそれぞれシグナル、ユーザーデータのキーに到達したタイミングでGodotのシグナルを発行するようになっています。
  - シーンを実行するとコンソールに受信したパラメータが出力されます。
  - ssdata サブフォルダにインポート元のプロジェクトファイル(v6_all.sspj)があります。
- sspj_load.tscn
  - GDスクリプトからSSプロジェクトをロードしアニメーションを指定して再生するサンプルです。
  - アニメーション終了時に別のSSプロジェクトに読み替えるようになっています。
- texture_change.tscn
  - 再生中にセルマップリソースのテクスチャを変更するサンプルです。
  - インスペクタにある Change のチェックをOn/Offすると切り替わります。

### mesh_bone

メッシュ、ボーン、エフェクトなどを利用したキャラクターアニメのサンプルです。

### particle_effect

エフェクト機能を利用したサンプルです。  
シーンに表示されるアニメーションは40種類のうちの一部になります。  
インスペクタの Anime Pack からその他のアニメーションも確認できます。

# お問い合わせ

ご質問、ご要望、不具合のご報告は [Issues](https://github.com/SpriteStudio/SSPlayerForGodot/issues) に投稿してください。  
非公開でのお問い合わせを希望される場合は、[ヘルプセンター](https://www.webtech.co.jp/help/ja/spritestudio7/inquiries/ssplayer_tool/) よりお寄せください。  
再現データなどの送付が必要な場合も、上記ヘルプセンター経由でファイルを送信してください。  

=========================================================  

株式会社ＣＲＩ・ミドルウェア  
​https://www.cri-mw.co.jp/  
Copyright © CRI Middleware Co., Ltd.  

=========================================================  

* SpriteStudio, Web Technologyは、株式会社ＣＲＩ・ミドルウェアの登録商標です。  
* その他の商品名は各社の登録商標または商標です。  
