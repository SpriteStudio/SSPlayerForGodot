# Security Policy

## Supported Versions

Security fixes are applied to the most recent release. Fixes are not backported to earlier major versions.

| Version | Supported          |
| ------- | ------------------ |
| 7.0.x   | :white_check_mark: |
| 1.x     | :x:                |

## Reporting a Vulnerability

**Please do not report security issues through public GitHub Issues.** A public report exposes the problem to users before a fix is available.

Instead, use the official Help Center inquiry form, which is a private channel and also supports file uploads for reproduction data:

👉 [**CRI Middleware Help Center (Inquiry Form)**](https://www.webtech.co.jp/help/en/spritestudio7/inquiries/ssplayer_tool/)

Please include as much of the following as you can:

- The version of SSPlayerForGodot (release tag or commit hash) and whether you use the GDExtension or custom module build.
- Your Godot Engine version, platform, and export target.
- A description of the issue and its impact.
- Steps to reproduce, and a minimal project or data file if one is needed.

We will acknowledge your report and keep you informed as we investigate. Please give us a reasonable opportunity to release a fix before disclosing the issue publicly. If you would like credit in the release notes, say so in your report.

## Scope

This project plays animation binaries (`.ssab` / `.ssqb`) through `libssruntime`, which is maintained in [SpriteStudio-SDK](https://github.com/cri-middleware/SpriteStudio-SDK). Report issues here if they reproduce through the Godot plugin; we will route runtime-level problems to the SDK.

Animation binaries are structurally verified when they are loaded. Even so, this plugin is designed for assets you author and ship with your game. Loading `.ssab` / `.ssqb` files obtained from untrusted third parties at runtime is outside the threat model — treat them as you would any other executable content in your project.

For non-security bugs and feature requests, see [SUPPORT.md](./SUPPORT.md).

---

## 日本語 (Security Policy in Japanese)

### サポート対象バージョン

セキュリティ修正は最新リリースに対して行います。過去のメジャーバージョンへのバックポートは行いません。

| バージョン | サポート           |
| ---------- | ------------------ |
| 7.0.x      | :white_check_mark: |
| 1.x        | :x:                |

### 脆弱性の報告

**セキュリティに関する問題を公開の GitHub Issues に投稿しないでください。** 修正が用意される前に問題が利用者に露出してしまいます。

非公開の窓口である公式ヘルプセンターのお問い合わせフォームをご利用ください。再現用データのファイル添付にも対応しています。

👉 [**ヘルプセンター（お問い合わせフォーム）**](https://www.webtech.co.jp/help/ja/spritestudio7/inquiries/ssplayer_tool/)

可能な範囲で以下をお知らせください。

- SSPlayerForGodot のバージョン（リリースタグまたはコミットハッシュ）と、GDExtension 版 / カスタムモジュール版のどちらか。
- Godot Engine のバージョン、プラットフォーム、エクスポート先。
- 問題の内容と影響。
- 再現手順、および必要であれば最小構成のプロジェクトやデータファイル。

ご報告は受領後に確認し、調査の経過をお知らせします。公開の前に修正をリリースする時間をいただけますようお願いします。リリースノートへのクレジット記載をご希望の場合は、その旨をご記入ください。

### 対象範囲

本プロジェクトは、[SpriteStudio-SDK](https://github.com/cri-middleware/SpriteStudio-SDK) で開発されている `libssruntime` を通じてアニメーションバイナリ（`.ssab` / `.ssqb`）を再生します。Godot プラグイン経由で再現する問題は本リポジトリへご報告ください。ランタイム側の問題であればこちらから SDK へ引き継ぎます。

アニメーションバイナリは読み込み時に構造検証を行っていますが、本プラグインは利用者自身が作成しゲームに同梱するアセットを前提としています。信頼できない第三者から入手した `.ssab` / `.ssqb` を実行時に読み込む用途は想定範囲外です。プロジェクト内の他の実行可能なコンテンツと同様に扱ってください。

セキュリティに関わらない不具合や機能要望については [SUPPORT.md](./SUPPORT.md) をご覧ください。
