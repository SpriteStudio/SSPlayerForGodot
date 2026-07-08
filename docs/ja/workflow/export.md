# プロジェクトのエクスポート

エディタ上でシーンが正しく再生できたら、あとは通常の Godot プロジェクトと同じ手順でエクスポートします（**プロジェクト → エクスポート…**）。SpriteStudioPlayer はすべての Godot ターゲットのエクスポート済みビルドで動作します。プラグイン固有の注意点はわずかです。

## 共通

- **GDExtension 形態** — ネイティブライブラリはプロジェクトの `addons/spritestudio/bin/` に含まれ、Godot が自動的にエクスポートへ同梱します。デスクトップ向けは追加設定不要です。
- **カスタムモジュール形態** — プラグインはエンジンに組み込まれているため、エクスポート時にプラグイン固有の設定は不要です。
- **モバイル / universal 向け** — *ETC2/ASTC テクスチャフォーマットが無効* という趣旨のメッセージでエクスポートが止まる場合は、**プロジェクト → プロジェクト設定 → レンダリング → テクスチャ → VRAM圧縮 → Import ETC2 ASTC** を有効にして再エクスポートしてください。

## Web

**GDExtension** 形態で Web ビルドを行う場合、プラグイン固有の要件が2つあります。

1. **Web エクスポートプリセットで「Extensions Support」を有効にする。**
   エクスポートダイアログで **Web** プリセット → **オプション** → **Extensions Support** を ON にします。GDExtension ライブラリは、エンジンテンプレートが動的リンクに対応している場合のみ Web で読み込まれます。このオプションが、そのテンプレートを使うよう Godot に指示します。

2. **dlink 対応の Web エクスポートテンプレートがインストールされている必要がある。**
   素の Godot Web テンプレートは GDExtension に対応していません。`web_nothreads_dlink_debug.zip` / `web_nothreads_dlink_release.zip` テンプレートを Godot のエクスポートテンプレートフォルダに導入しておく必要があります。これらのビルドと導入は上級者向けの手順で、[ビルドガイド → Web での GDExtension](../setup/build.md#web-gdextensionextensions-support-dlink) にまとめてあります。導入済みであれば、プリセット側は **Extensions Support** を ON にするだけで、Godot が適切なテンプレートを自動選択します。

> [!WARNING]
> Web ビルドが起動時に *「GDExtension libraries are not supported by this engine version…」* で失敗する場合、上記2つの要件のいずれかが欠けています（Extensions Support が OFF、または dlink テンプレートが未インストール）。

Web に関する補足:

- 本プラグインは Web では **シングルスレッド（`nothread`）** で動作するため、プリセットの **Thread Support** は OFF のままにしてください。エクスポートの動作確認は単純な HTTP サーバーで十分で、特殊なクロスオリジン（COOP/COEP）ヘッダーは不要です。
- 正常な GDExtension Web エクスポートでは `index.wasm` と並んで `index.side.wasm` が出力されます。これがあれば dlink テンプレートが使われた証拠です。
- ランタイムは **WebAssembly SIMD** を利用するため、対象ブラウザが SIMD に対応している必要があります。

> [!NOTE]
> **カスタムモジュール** 形態では上記の手順はいずれも不要です（*Extensions Support* も dlink テンプレートも不要）。プラグインがエンジンにコンパイル済みのためです。ただし Web では同じくシングルスレッドで動作し、他の custom-module ターゲットと同様、素の Godot テンプレートではなく **自分でビルドしたエンジン入り Web テンプレート**（プラグインを含む）を使います。そのビルド／インストール手順は [ビルドガイド](../setup/build.md) を参照してください。
