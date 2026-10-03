# Security Policy

## Supported Versions

SpriteStudio Player for Godot is in beta. Fixes are issued against the latest release only — if you
are on an older one, please reproduce on the current release before reporting.

| Version | Supported |
|---|---|
| Latest `7.0.x` release | ✅ |
| Anything older | ❌ |

## Reporting a Vulnerability

**Please do not open a public GitHub Issue for a security problem.** Report it privately through the
official inquiry form:

👉 [**CRI Middleware Inquiry Form (English)**](https://www.webtech.co.jp/contact/en.html)

The English form has no file upload. If you need to send a reproduction, use the
[Japanese Help Center form](https://www.webtech.co.jp/help/ja/spritestudio7/inquiries/ssplayer_tool/),
which takes a zip of up to 8 MB.

Please include:

- What the impact is — crash, out-of-bounds read, arbitrary code execution, information disclosure.
- Steps to reproduce, ideally with a minimal `.ssab` / `.sspj` and the smallest project needed.
- The player version (release tag or commit), whether you use the GDExtension or the custom module
  build, your Godot version, the platform and the export target.
- Whether the input that triggers it comes from a trusted source in your project or from somewhere
  untrusted (downloaded content, user-supplied files).

We will confirm receipt, tell you whether we can reproduce it, and let you know when a fix ships.
Please give us a chance to release that fix before disclosing publicly.

### Which repository to report to

Playback is driven by the runtime shared with the other SpriteStudio players, which lives in
SpriteStudio-SDK. If the same problem reproduces on a different SpriteStudio player, it is most likely
in that shared core. Report it here all the same and say so; we will route it.

## Scope Notes

The player loads binary animation data (`.ssab` / `.ssqb`) and hands it to the runtime.

**Packs are verified when they are loaded.** `SSABResource` and `SSQBResource` run the FlatBuffers
verifier over the whole buffer and reject one that fails, so a truncated or malformed file is an error
rather than a read out of bounds.

**The player is built for assets you author and ship with your game.** Loading `.ssab` / `.ssqb`
obtained from untrusted third parties at runtime is not a supported configuration: treat packs as you
would any other executable content in your project.

---

# セキュリティポリシー

## サポート対象バージョン

SpriteStudio Player for Godot は beta 版です。修正は最新リリースに対してのみ提供されます。古いバージョンをご利用の場合は、最新リリースで再現することをご確認のうえご報告ください。

| バージョン | サポート |
|---|---|
| 最新の `7.0.x` リリース | ✅ |
| それ以前 | ❌ |

## 脆弱性の報告

**セキュリティに関する問題は、公開の GitHub Issue に投稿しないでください。** 再現データ（8 MB までの zip ファイル）の添付も可能なヘルプセンターより、非公開でご報告ください。

👉 [**ヘルプセンター（お問い合わせフォーム）**](https://www.webtech.co.jp/help/ja/spritestudio7/inquiries/ssplayer_tool/)

ご報告には以下を含めてください。

- 影響の内容（クラッシュ、境界外読み取り、任意コード実行、情報漏洩など）
- 再現手順。可能であれば最小限の `.ssab` / `.sspj` と、必要最小限のプロジェクト
- プレイヤーのバージョン（リリースタグまたはコミット）、GDExtension 版とカスタムモジュール版のどちらか、Godot のバージョン、プラットフォーム、エクスポート先
- 引き金となる入力が、プロジェクト内の信頼できるデータか、信頼できない経路（ダウンロードコンテンツ、ユーザーが用意したファイル）由来か

受領の確認、再現可否、修正版の公開時期についてご連絡します。修正を公開するまでの間、公表をお待ちいただけますようお願いします。

### 報告先リポジトリの判断

再生処理は、他の SpriteStudio Player と共通のランタイムが担っており、SpriteStudio-SDK で開発されています。他の SpriteStudio Player でも同じ問題が再現する場合、共通コア側の問題である可能性が高いため、その旨を添えて本リポジトリへご報告ください。こちらで振り分けます。

## 既知の注意点

本プレイヤーはバイナリのアニメーションデータ（`.ssab` / `.ssqb`）を読み込み、ランタイムへ渡します。

**パックは読み込み時に検証されます。** `SSABResource` と `SSQBResource` はバッファ全体に FlatBuffers の verifier をかけ、通らないものを拒否します。途中で切れたファイルや破損したファイルは、境界外読み取りではなくエラーになります。

**本プレイヤーは、利用者自身が作成してゲームに同梱するアセットを前提としています。** 信頼できない第三者から入手した `.ssab` / `.ssqb` を実行時に読み込む構成はサポート対象外です。プロジェクト内の他の実行可能なコンテンツと同様に扱ってください。
