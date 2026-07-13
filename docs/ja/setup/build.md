# ビルド / 開発

GDExtension またはカスタムモジュール組み込み Godot Engine を自分でビルドしたい場合、および SpriteStudio-SDK と並行してプラグインを開発したい場合は、以下の手順に従ってください。

## 概要

本リポジトリで Godot 用バイナリを得るまでの流れは以下のとおりです。

1. **`libssruntime` の用意** — SpriteStudio-SDK のリリース成果物を取得し、`ss_player/runtime/` 配下に配置します。
2. 利用形態に応じて **2-A. GDExtension のビルド** または **2-B. カスタムモジュール組み込み Godot Engine のビルド** を実行し、上記ランタイムをリンクして Godot 用バイナリを生成します。

## ソース取得

本リポジトリを取得し、ビルド対象に応じて Godot Engine / godot-cpp を取得します。

```bash
git clone https://github.com/SpriteStudio/SSPlayerForGodot.git
cd SSPlayerForGodot
git clone https://github.com/godotengine/godot.git -b 4.6
git clone https://github.com/godotengine/godot-cpp.git -b master
```

`godot` ディレクトリはカスタムモジュール組み込み Godot Engine をビルドする場合に必要です。
`godot-cpp` ディレクトリは GDExtension をビルドする場合に必要です。

> [!NOTE]
> SDK のリリース成果物を使う一般的なビルドでは、サブモジュール `ss_player/SpriteStudio-SDK/` の取得は不要です（`--recursive` を付けずにクローンして構いません）。SpriteStudio-SDK 自体を手元で開発・ビルドする場合のみ、後述の [SpriteStudio-SDK 開発者向け](#spritestudio-sdk) に従って `git submodule update --init --recursive` でサブモジュールを初期化してください。

## ビルド環境のセットアップ

各プラットフォーム向けのビルドツール (コンパイラ・Python・SCons など) の準備は、Godot 公式のコンパイル手順を参照してください。

- [Windows](https://docs.godotengine.org/en/stable/engine_details/development/compiling/compiling_for_windows.html)
- [macOS](https://docs.godotengine.org/ja/stable/engine_details/development/compiling/compiling_for_macos.html)
- Linux: T.B.D.

### 注意点

#### macOS で Universal Binary をビルドする場合

Homebrew で配布されている `molten-vk` はホストアーキ向けのバイナリのみ提供されるため、`arch=universal` 指定で Universal Binary をビルドする際はリンクに失敗します。代わりに [Vulkan SDK for MoltenVK](https://vulkan.lunarg.com/sdk/home) (Universal 対応版) をインストールしてください。

## 1. libssruntime の用意

`scripts/SDK_VERSION.txt` で指定されたバージョンの SDK パッケージを取得・展開します。

**macOS / Linux**

```sh
./scripts/download-sdk.sh
```

**Windows (PowerShell)**

```powershell
.\scripts\download-sdk.ps1
```

> `libssconverter` (`.sspj` → `.ssab` 変換ライブラリ) はデスクトップ向けにのみ同梱されます。iOS / Android / Web 向けの `libssruntime` パッケージには含まれません。

`libssruntime` を SpriteStudio-SDK ソースから自前でビルドしたい場合は [SpriteStudio-SDK 開発者向け](#spritestudio-sdk) を参照してください。

## 2-A. GDExtension のビルド

`godot-cpp` を `master` ブランチで clone 済みであることが前提です。

**macOS / Linux**

```sh
./scripts/build-extension.sh
```

**Windows (PowerShell)**

```powershell
$env:PYTHONUTF8=1
.\scripts\build-extension.ps1
```

成果物は `bin/<platform>/` に配置され、`misc/spritestudio.gdextension` を含む GDExtension 一式が各サンプルプロジェクト（`examples/*/addons/spritestudio/`）にインストールされます。

## 2-B. カスタムモジュール組み込み Godot Engine のビルド

`godot` を `4.6` ブランチで clone 済みであることが前提です。
`build.sh` / `build.ps1` は `custom_modules=../ss_player` を指定して `scons` を実行します。

**macOS / Linux**

```sh
./scripts/build.sh
```

**Windows (PowerShell)**

```powershell
$env:PYTHONUTF8=1
.\scripts\build.ps1
```

成果物は `godot/bin/` に出力されます。macOS では `godot/Godot.app` が併せて作成されます。

## リリースビルド

各プラットフォーム向けに `editor` / `template_debug` / `template_release` を一括でビルドするスクリプトを `scripts/` 配下に用意しています。
内部では前述の `build.sh` / `build-extension.sh` を `target` を変えて連続実行する構成です。
これらのスクリプトは `libssruntime` を取得・ビルドはしないため、事前に [1. libssruntime の用意](#1-libssruntime) を済ませておく必要があります。

### GDExtension

| プラットフォーム | スクリプト                                   | 補足                                         |
| ---------------- | -------------------------------------------- | -------------------------------------------- |
| Windows          | `.\scripts\release-gdextension-windows.ps1`  | `arch` はホスト                              |
| macOS            | `./scripts/release-gdextension-macos.sh`     | `arch=universal` 固定                        |
| Linux            | `./scripts/release-gdextension-linux.sh`     | `arch` はホスト                              |
| iOS              | `./scripts/release-gdextension-ios.sh`       | `template_debug` / `template_release` のみ   |
| Android          | `./scripts/release-gdextension-android.sh`   | `arm32` / `arm64` / `x86_64` の3アーキ       |
| Web              | `./scripts/release-gdextension-web.sh`       | `wasm32` (`threads=yes` / `threads=no`)      |

### カスタムモジュール組み込み Godot Engine

| プラットフォーム | スクリプト                            | 補足                                          |
| ---------------- | ------------------------------------- | --------------------------------------------- |
| Windows          | `.\scripts\release-windows.ps1`       | `arch` はホスト                               |
| macOS            | `./scripts/release-macos.sh`          | `arch=universal` 固定                         |
| iOS              | `./scripts/release-ios.sh`            | `arch=arm64` (実機) と `arch=universal` (sim) |
| Android          | `./scripts/release-android.sh`        | `arm32` / `arm64` / `x86_64` の3アーキ        |

> Linux 向けのカスタムモジュール用一括ビルドスクリプトは未整備です。`./scripts/build.sh platform=linux target=...` を `editor` / `template_debug` / `template_release` で個別に呼び出してください。

## カスタムモジュールのデバッグ方法

カスタムモジュールとして組み込んだプラグインのC++コード（`ss_player` 以下）をデバッグしたい場合は、以下の手順で行います。

1. **デバッグ用バイナリの確認**
   `target=editor` あるいは `target=template_debug` を指定してビルドしたGodotバイナリには、デフォルトでデバッグシンボルが含まれています。
   * **macOS:** `godot/Godot.app/Contents/MacOS/Godot`
   * **Windows:** `godot/bin/godot.windows.editor.x86_64.exe` 等
   * **Linux:** `godot/bin/godot.linuxbsd.editor.x86_64` 等

2. **デバッガのアタッチと起動引数**
   VSCode、Visual Studio、XcodeなどのIDEやコマンドラインのデバッガ（LLDB/GDB）から、上記のバイナリを起動プログラムとして指定します。
   引数として対象プロジェクトへのパス（例: `--path examples/dev_module`）を渡すことで、エディタ画面を挟まずに直接プロジェクトを開いてデバッグを開始できます。

**VSCode (`launch.json`) の設定例 (macOS / LLDB の場合):**

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug Godot Custom Module",
            "type": "lldb",
            "request": "launch",
            "program": "${workspaceFolder}/godot/Godot.app/Contents/MacOS/Godot",
            "args": [
                "--path",
                "${workspaceFolder}/examples/dev_module"
            ],
            "cwd": "${workspaceFolder}"
        }
    ]
}
```

## エクスポート機能のテスト・デバッグ方法

開発したカスタムモジュールやGDExtensionが、ビルド後のアプリで正常に動作するか（エクスポートが成功するか）を確認・デバッグするには、CLIを用いたヘッドレスエクスポートが便利です。

> [!IMPORTANT]
> エクスポート時の共通の注意点（全プラットフォーム）:
> - **エクスポートする `target` を実際にビルドしておく。** `editor` ビルドだけでは不十分です。`--export-debug` は `template_debug` ライブラリを、`--export-release` は `template_release` を必要とします。これらを未ビルドのままだとエクスポートは「成功」しても、拡張ライブラリが空／欠落した状態で書き出されます（macOS では同梱 framework に対する `CodeSign: Invalid binary format` エラーとして表面化します）。
> - **拡張のアーキテクチャは手元の runtime スライスに依存する。** GDExtension は `ss_player/runtime/libs/<platform>/` の `libssruntime` をリンクするため、そこに存在するアーキテクチャしかビルドできません。例えば macOS の runtime が `arm64` のみの場合、`universal` / `x86_64` の拡張は作れません。プリセットの `binary_format/architecture` を合わせてください（例: `arm64`）。エンジンテンプレートは `universal` のままで構いません（単一アーキの拡張が、欠けているアーキでロードされないだけです）。
> - **`universal` / `arm64` / モバイル向けは ETC2/ASTC を有効化する。** プロジェクトに `rendering/textures/vram_compression/import_etc2_astc=true` を設定してください（`project.godot` の `[rendering]`）。未設定だと *「ETC2/ASTC テクスチャフォーマットが無効なため … エクスポートできません」* でエクスポートが中断します。
> - **エクスポートテンプレートはエディタのバージョンと一致が必要。** `custom_template/debug` / `custom_template/release` を指定するとこのバージョンチェックを回避できます（同梱の `godot/` ソースからビルドしたテンプレートを流用できるのはこのためです）。debug のみのエクスポートでも debug / release **両方** のパスを設定してください。Godot は両方を検証し、未設定だと release テンプレートが見つからないと報告します。

以下の流れで、手元でテンプレートをビルド・インストールし、サンプルプロジェクトをエクスポートします。（以下はmacOSの例です）

1. **ランタイムとテンプレートのビルド**
   事前に `libssruntime` を用意し、対象プラットフォームのリリーススクリプトを実行します。
   ```bash
   # (必要に応じて) ランタイムをリリースビルドで用意
   ./scripts/build-runtime.sh build=release platform=macos

   # エクスポート用テンプレートのビルド
   ./scripts/release-macos.sh
   ```

2. **テンプレートのインストール**
   ビルドしたテンプレートを、Godotが認識するローカルの所定ディレクトリへインストールします。
   **macOS / Linux:**
   ```bash
   ./scripts/install-template.sh macos
   ```
   **Windows (PowerShell):**
   ```powershell
   .\scripts\install-template.ps1 windows
   ```

3. **CLIからのエクスポート実行**
   ビルドしたGodotエディタ（headlessモード）を使い、コマンドラインから直接エクスポート処理を呼び出します。
   ```bash
   # 例: dev_module プロジェクトを macOS 向けにエクスポート (.appとして直接出力)
   ./godot/Godot.app/Contents/MacOS/Godot --path ./examples/dev_module/ --headless --export-debug "macOS" output.app
   ```
   > **Note:** エクスポートを実行するには、対象プロジェクト内の `export_presets.cfg` に指定したプラットフォーム名（上記の場合は `"macOS"`）のプリセットが存在し、必要な識別子（バンドルIDなど）が正しく設定されている必要があります。

### Webプラットフォームのエクスポートと動作確認

Web向けのエクスポートでは複数のファイル（`.html`, `.wasm`, `.pck`等）が出力されるため、専用のディレクトリを作成してエクスポートします。また、ブラウザのセキュリティ制限を回避するためローカルサーバーでの確認が必要です。

```bash
# 1. 出力用ディレクトリを作成し、Web向けにエクスポート
mkdir -p build_web
./godot/Godot.app/Contents/MacOS/Godot --path ./examples/dev_module/ --headless --export-debug "Web" ../../build_web/index.html

# 2. ローカルHTTPサーバーを起動
cd build_web
python3 -m http.server 8000
```
サーバー起動後、ブラウザで `http://localhost:8000` にアクセスすると動作確認ができます。
（本プラグインは Web においては `nothread` での動作となるため、特殊なCORSヘッダーなしの単純なHTTPサーバーで起動可能です）

#### Web での GDExtension（Extensions Support / `dlink`）

公式の Godot Web エクスポートテンプレートは GDExtension ライブラリに **対応していません**。**GDExtension** 形態を素のテンプレートで Web エクスポートすると、起動時に次のエラーで失敗します。

> GDExtension libraries are not supported by this engine version. Enable "Extensions Support" for your export preset and/or build your custom template with "dlink_enabled=yes".

Web で GDExtension を読み込むには、動的リンクを有効にした（`dlink_enabled=yes`）エンジンテンプレートが必要です。同梱の `godot/` ソースから、拡張の `threads` モードに合わせてビルドします（本プラグインは `nothread` 版を提供するため `threads=no`）。

```bash
# dlink 対応の Web テンプレートをソースからビルド（nothreads）
cd godot
scons platform=web target=template_debug   dlink_enabled=yes threads=no
scons platform=web target=template_release dlink_enabled=yes threads=no
# → godot/bin/godot.web.template_debug.wasm32.nothreads.dlink.zip
# → godot/bin/godot.web.template_release.wasm32.nothreads.dlink.zip
```

続いて、ビルドしたテンプレートをインストールし、Godot が名前で解決できるようにします。

```bash
./scripts/install-template.sh web
# インストール先 → <export_templates>/<version>/web_nothreads_dlink_debug.zip
#                                              web_nothreads_dlink_release.zip
```

インストール後は、Web プリセットで **Extensions Support** を ON にするだけでエクスポートできます。Godot が名前一致で `..._dlink_...` テンプレートを自動選択するため、`custom_template` の指定は不要です。プリセット側の操作（利用者視点）は [プロジェクトのエクスポート → Web](../workflow/export.md#web) を参照してください。

> Godot のテンプレートフォルダにインストールしたくない場合は、代わりに `Web` プリセットの `custom_template/debug` / `custom_template/release` にビルドした `*.dlink.zip` を直接指定することもできます。

dlink テンプレートはエンジンを小さなメインモジュールと大きな `godot.side.wasm` に分割します。エクスポート後に `index.wasm` と並んで `index.side.wasm` が出力されていれば、dlink テンプレートが使われた証拠です。

> [!NOTE]
> `dlink` / *Extensions Support* が必要なのは **GDExtension** 形態のみです。**カスタムモジュール** 形態はプラグインをエンジンにコンパイルするため、その Web テンプレートは `release-web.sh`（内部で `build.sh`）が生成する通常の（非 `dlink`）`web_nothreads_{debug,release}.zip` で、同じ手順でインストールします。`dlink_enabled` もプリセットの *Extensions Support* も不要です。

### Androidプラットフォームのエクスポートと動作確認

Android のエクスポートテンプレートは、エンジンの Gradle プロジェクトでパッケージされるテンプレートAPKです。そのためデスクトップ系と異なり、`install-template.sh` の前にテンプレートのビルドが3段階（ランタイム → ABIごとのエンジンライブラリ → Gradleパッケージ）になります。

**事前準備**
- Android SDK と NDK、`cargo-ndk`、および Rust の Android ターゲット（`aarch64-linux-android`, `armv7-linux-androideabi`, `x86_64-linux-android`）をインストールしておきます。
- ビルドスクリプトが参照できるよう、SDK/NDK のパスを環境変数に設定します（環境に合わせて調整してください）:
  ```bash
  export ANDROID_HOME="$HOME/Library/Android/sdk"
  export ANDROID_NDK_ROOT="$ANDROID_HOME/ndk/<ndk-version>"
  ```
- Godot のエディタ設定で、Android SDK パスとデバッグ用キーストアを設定します（`export/android/android_sdk_path`, `export/android/debug_keystore`, `export/android/debug_keystore_pass`）。

1. **ランタイムとテンプレートのビルド**
   まず Rust ランタイムを一度ビルドし、次に ABI／ターゲットごとにエンジンの共有ライブラリをビルドして、Gradle でテンプレートAPKにパッケージします。
   ```bash
   # Rust ランタイム（1回のreleaseビルドを両テンプレートで共用します）
   ./scripts/build-runtime.sh platform=android build=release
   # ビルドの代わりに、全プラットフォームのプリビルドランタイムを取得することもできます:
   #   ./scripts/download-sdk.sh

   # ABIごとのエンジン .so。arch: arm64 / arm32 / x86_64, target: template_release / template_debug
   ./scripts/build.sh platform=android arch=arm64  target=template_release
   ./scripts/build.sh platform=android arch=arm32  target=template_release
   ./scripts/build.sh platform=android arch=x86_64 target=template_release
   ./scripts/build.sh platform=android arch=arm64  target=template_debug   # 必要に応じて他のABIも

   # android_release.apk / android_debug.apk / android_source.zip としてパッケージ
   (cd godot/platform/android/java && ./gradlew generateGodotTemplates)
   ```
   > `arch` は Godot の名称（`arm64`, `arm32`, `x86_64`, `x86_32`）で指定しますが、ランタイムライブラリは対応する Android ABI ディレクトリ（`arm64-v8a`, `armeabi-v7a`, `x86_64`, `x86`）に配置されます。実行する端末／エミュレータの ABI のみビルドすれば十分です。

2. **テンプレートのインストール**
   ```bash
   ./scripts/install-template.sh android
   ```

3. **CLIからのエクスポート実行**
   ```bash
   mkdir -p bin_export
   ./godot/Godot.app/Contents/MacOS/Godot --path ./examples/dev_module/ --headless --export-debug "Android" "$(pwd)/bin_export/dev_module_debug.apk"
   ```
   > **Note:** Android のエクスポートには ETC2/ASTC テクスチャインポートの有効化が必要です。無効のままだとメッセージが空の設定エラーでエクスポートが中断します。プロジェクト設定に `rendering/textures/vram_compression/import_etc2_astc=true` を設定してください（`examples/dev_module` では設定済みです）。

4. **端末／エミュレータへのインストールと実行**
   ```bash
   adb install -r bin_export/dev_module_debug.apk
   adb shell am start -n com.crimw.devmodule/com.godot.game.GodotAppLauncher
   adb logcat -s godot        # エンジンのログ／エラー確認
   adb exec-out screencap -p > screen.png   # 描画結果のキャプチャ
   ```
   `dev_module` サンプルは `SpriteStudioPlayer2D` で `Knight_arrow` アニメーションを autoplay 再生するため、正常に動作すればキャラクターが画面に描画されます。

### iOSプラットフォームのエクスポートと動作確認

iOS エクスポートには **macOS + Xcode** が必要です。Godot は **Xcode プロジェクト**（すぐ動くアプリではない）を生成し、続けて実機向けに `xcodebuild archive` を自動実行します。これには Apple Developer アカウントが必要なため、**プリセットに App Store Team ID の設定が必須**で、有効な署名が無いと実機 archive 段階で失敗します。署名なしで手早く確認するには、生成された Xcode プロジェクトを自分で **iOS シミュレータ**向けにビルドします。

> [!IMPORTANT]
> **自前ビルドの iOS テンプレートは MoltenVK を同梱しません。** Godot の iOS エンジンは Vulkan 有効でビルドされるため、アプリのバイナリが `@rpath/MoltenVK.framework/MoltenVK` を**ハードリンク**します（`otool -L` で確認可能）。これは*エンジンのビルド*の性質であり、プロジェクトのレンダラ設定とは無関係です（GL Compatibility のプロジェクトでもリンクされます）。**公式**の Godot iOS テンプレートは `MoltenVK.xcframework` を同梱しますが、`release-ios.sh` が生成するテンプレートは**同梱しません**。自前テンプレートで動作確認する場合は自分で供給する必要があり、無いと起動時に `Library not loaded: @rpath/MoltenVK.framework/MoltenVK` でクラッシュします。

自前テンプレートのシミュレータ動作確認（カスタムモジュールの例。GDExtension も `dev_gdextension` で同じ流れ）：

```bash
# 1. Xcode プロジェクトを生成（Godot の実機 archive 段階は署名で失敗するが想定内）
mkdir -p bin_export/ios
./godot/Godot.app/Contents/MacOS/Godot --path ./examples/dev_module/ --headless \
    --export-debug "iOS" "$(pwd)/bin_export/ios/dev_module.xcodeproj" || true

# 2. Xcode プロジェクトが期待する場所へ MoltenVK を配置（例: Vulkan SDK から）
MVK="$HOME/VulkanSDK/<ver>/iOS/lib/MoltenVK.xcframework"
cp -R "$MVK" bin_export/ios/MoltenVK.xcframework

# 3. シミュレータ向けに署名なしでビルド
cd bin_export/ios
xcodebuild -project dev_module.xcodeproj -scheme dev_module \
    -sdk iphonesimulator -configuration Debug -derivedDataPath ./DerivedData \
    CODE_SIGNING_ALLOWED=NO CODE_SIGNING_REQUIRED=NO build
APP="DerivedData/Build/Products/Debug-iphonesimulator/dev_module.app"

# 4. MoltenVK をアプリに埋め込み、ad-hoc 署名し直す（simulator は SDK 由来の署名を拒否するため）
cp -R "$MVK/ios-arm64_x86_64-simulator/MoltenVK.framework" "$APP/Frameworks/"
codesign --force --sign - "$APP/Frameworks/MoltenVK.framework"
codesign --force --sign - "$APP"

# 5. シミュレータを起動してインストール・起動・スクショ
DEV=$(xcrun simctl list devices available | grep -m1 -oE '\([0-9A-F-]{36}\)' | tr -d '()')
xcrun simctl boot "$DEV" && xcrun simctl install "$DEV" "$APP"
xcrun simctl launch "$DEV" com.crimw.devmodule
xcrun simctl io "$DEV" screenshot screen.png
```

> **Note:** 本来の解決策は、iOS テンプレート自体（`install-template.sh` / `misc/dist/apple_embedded_xcode`）に `MoltenVK.xcframework` を同梱し、エクスポート時に自動で埋め込み・署名させて公式テンプレートと同等にすることです。上記の手動手順が要るのは**自前ビルド**のテンプレートの場合のみで、公式 Godot エディタ＋公式エクスポートテンプレートを使う一般利用者には影響しません。

### GDExtension のエクスポートと動作確認

GDExtension（例: `dev_gdextension`）の場合、**エンジン本体の再ビルドやカスタムテンプレートのインストールは不要**です。Godot公式が配布している標準のGodotエディタとエクスポートテンプレートを使用して、そのままエクスポートが可能です。

1. **GDExtension プラグインのリリースビルド**
   事前に対象プラットフォーム向けのビルドスクリプトを実行し、プロジェクトの `addons/` ディレクトリにライブラリ（`.so`, `.xcframework`, `.dll` 等）を出力しておきます。
   **macOS / Linux:**
   ```bash
   ./scripts/release-gdextension-macos.sh
   ```
   **Windows (PowerShell):**
   ```powershell
   .\scripts\release-gdextension-windows.ps1
   ```

2. **CLIからのエクスポート実行**
   公式のGodotバイナリ（あるいは各自のGodotコマンド）を使って、プロジェクトをエクスポートします。
   **macOS / Linux:**
   ```bash
   # ※ ここでの "godot" は、パスが通っている公式のGodotエディタ実行ファイル等を指します
   godot --path ./examples/dev_gdextension/ --headless --export-debug "macOS" output.app
   ```
   **Windows (PowerShell):**
   ```powershell
   # ※ "godot.exe" は公式のGodot実行ファイル
   godot.exe --path .\examples\dev_gdextension\ --headless --export-debug "Windows Desktop" output.exe
   ```
   > 実行時に `.so`, `.framework`, `.dll` などのプラグインファイルは Godot が自動的にエクスポート成果物の中に同梱してくれます。

   **Android の注意点:** Android では上記のデスクトップ手順に加えて、いくつか追加のステップが必要です。
   - Android 用プラグインライブラリを `./scripts/release-gdextension-android.sh` でビルドします（Android SDK/NDK、`cargo-ndk`、Rust の Android ターゲット、および Android ランタイムが必要。ランタイムは `./scripts/build-runtime.sh platform=android build=release` でビルドするか、`./scripts/download-sdk.sh` でプリビルドを取得します）。`arch` は Godot の名称（`arm64`, `arm32`, `x86_64`）で指定しますが、ランタイムは対応する Android ABI ディレクトリ（`arm64-v8a`, `armeabi-v7a`, `x86_64`）からリンクされます。
   - **ホスト用**プラグイン（例: `./scripts/release-gdextension-macos.sh`）もビルドしてください。エクスポート時の `.ssab` リソースの読み込みはホストマシン上で行われるため、動作するホストライブラリが無いと `Failed loading resource` エラーで `.ssab` が除外され、生成された APK にアニメーションデータが入りません。
   - 使用する公式 Godot のバージョンに対応した **Android** のエクスポートテンプレートを導入します（エディタの *エクスポートテンプレートの管理* から、または `.tpz` の中身を `.../export_templates/<version>/` に配置）。
   - プロジェクト設定で `rendering/textures/vram_compression/import_etc2_astc=true` を有効にします（`examples/dev_gdextension` では設定済み）。無効のままだとメッセージが空の設定エラーでエクスポートが中断します。
   - あとはエクスポートして端末／エミュレータで実行します。
     ```bash
     mkdir -p bin_export
     godot --path ./examples/dev_gdextension/ --headless --export-debug "Android" "$(pwd)/bin_export/dev_gdextension_debug.apk"
     adb install -r bin_export/dev_gdextension_debug.apk
     adb shell am start -n com.crimw.devgdext/com.godot.game.GodotAppLauncher
     adb logcat -s godot
     ```

## GDExtension のデバッグ方法

GDExtension の場合も、カスタムモジュールとほぼ同じ手順でC++コードをデバッグできます。違いは起動するプログラムが「公式のGodotエディタ」になる点のみです。

1. **GDExtensionのデバッグビルド確認**
   通常のビルドスクリプト（`build-extension.sh` 等）はデフォルトで `target=template_debug` となっており、出力されるライブラリ（`.dll` や `.dylib`）にはデバッグシンボルが含まれています。

2. **デバッガのアタッチと起動引数**
   VSCode や Visual Studio 等のデバッガから、**公式のGodotバイナリ** を起動プログラムとして指定し、引数でプロジェクトのパス（`--path examples/dev_gdextension`）を渡します。Godotが起動時にプラグインを動的ロードした時点でブレークポイントにヒットするようになります。

**VSCode (`launch.json`) の設定例:**

**macOS / Linux (LLDBの場合):**
```json
{
    "name": "Debug GDExtension (macOS)",
    "type": "lldb",
    "request": "launch",
    "program": "/Applications/Godot.app/Contents/MacOS/Godot", // 公式バイナリのパス
    "args": [ "--path", "${workspaceFolder}/examples/dev_gdextension" ],
    "cwd": "${workspaceFolder}"
}
```

**Windows (Visual Studio デバッガの場合):**
```json
{
    "name": "Debug GDExtension (Windows)",
    "type": "cppvsdbg",
    "request": "launch",
    "program": "C:\\path\\to\\Godot_v4.x-stable_win64.exe", // 公式バイナリのパス
    "args": [ "--path", "${workspaceFolder}\\examples\\dev_gdextension" ],
    "cwd": "${workspaceFolder}"
}
```

## SpriteStudio-SDK 開発者向け

> 以降のセクションは **SpriteStudio-SDK 自体を手元で開発・カスタマイズしながら Godot 側もビルドしたい場合のみ** 必要です。SpriteStudio-SDK のリリース成果物を使う一般的な Godot ビルダーは読み飛ばして構いません。

### 追加で必要なもの

`libssruntime` を自前でビルドするための環境 (Rust ツールチェーン等) のセットアップ手順は [SpriteStudio-SDK の README](https://github.com/cri-middleware/SpriteStudio-SDK?tab=readme-ov-file#for-sdk-developers) を参照してください。

FlatBuffers のヘッダを再生成する場合は別途 `flatc` (FlatBuffers コンパイラ) も必要です。

### libssruntime を自前でビルドする

SpriteStudio-SDK サブモジュール (`ss_player/SpriteStudio-SDK/`) が初期化済みであることが前提です。未初期化の場合は以下で取得してください。

```bash
git submodule update --init --recursive
```

その後、以下を実行すると Rust ランタイム/コンバータがビルドされ、`ss_player/runtime/` 配下に成果物が自動配置されます。

**macOS / Linux**

```sh
./scripts/build-runtime.sh [platform=<platform>] [arch=<arch>] [build=<build>] [ios_simulator=<yes|no>]
```

**Windows (PowerShell)**

```powershell
.\scripts\build-runtime.ps1 [platform=<platform>] [arch=<arch>] [build=<build>] [ios_simulator=<yes|no>]
```

| オプション | 値 | デフォルト |
| --- | --- | --- |
| `platform` | `windows`, `macos`, `linux`, `android`, `ios`, `web` | ホストOS |
| `arch` | `x86_64`, `arm64`, `universal`, `wasm32` など | ホストアーキ |
| `build` | `debug`, `release` | `debug` |
| `ios_simulator` | `yes`, `no` | `no` |

### FlatBuffers ヘッダの再生成

SpriteStudio-SDK の `.fbs` を変更した場合は、以下で `ss_player/format/` 配下のヘッダを再生成します (`flatc` が必要)。

**macOS / Linux**

```sh
./scripts/generate-runtime-code.sh
```

**Windows (PowerShell)**

```powershell
.\scripts\generate-runtime-code.ps1
```

### SpriteStudio-SDK 内部ドキュメント

サブモジュール初期化済みであれば、ランタイムの内部仕様や移植時の注意点は以下を参照できます。

- `ss_player/SpriteStudio-SDK/libs/ssruntime/docs/README.ja.md`
