# Releasing SSPlayerForGodot

Maintainers only. Contributors do not run any of this — see [CONTRIBUTING.md](./CONTRIBUTING.md).

## Building the documentation site

```bash
scripts/prepare-docs.sh          # once: .venv + the pins in docs/requirements.txt
scripts/build-docs.sh            # English then Japanese, both --strict
scripts/build-pages.sh serve=yes # the published tree, both locales -> http://localhost:8000/
```

Both locales, English first: it clears `site/`, which contains `site/ja`, so an English-only build
leaves a stale Japanese site behind and a page you just broke still looks fine. `mkdocs.base.yml`
sets `strict: true`; `zensical serve` validates nothing, `--strict` or not, and serves one locale at
a time, so `build-pages.sh serve=yes` is the only way to see the language selector resolve.
`pages.yml`'s build job **is** that script with every option spelled out, so CI and the local gate
cannot drift. `.ps1` twins on Windows, `key=value` options and `--help` on both.

## Cutting a release

A release is a tag, pushed first and built second: push `v<version>`, then dispatch **release
gdextension** from that tag with `upload_release=true`. The tag names the Release, and
`upload_release=true` from anything else fails the run rather than quietly producing none. The
default dispatch off a `release/X.Y` branch is a QA build and creates no Release. Releases are
always created as drafts — a human reviews the assets and the generated notes, then publishes from
the UI, choosing pre-release or latest there.

`scripts/build-release.sh` is the package: the `addons/spritestudio/` folder a user drops into a
project — the descriptor, the icons it points at, every licence the shipped binaries carry, and
`bin/<platform>/` for all six — then the zip and `SHA256SUMS`. `release.yml`'s package job **is**
this script with every option spelled out. It builds nothing, so it replays a matrix run in seconds:

```bash
gh run download <run-id> -D artifacts
scripts/build-release.sh
```

Its check is the one nothing else in the pipeline does. `misc/spritestudio.gdextension` names a
file per platform and build target — nineteen paths — plus three icons, and **Godot resolves them
at load time**: a name that does not match what shipped fails no build and no zip, and the
extension simply does not load, on that one platform, for whoever downloaded it. Every path in the
descriptor is looked up inside the finished archive.

<br>

---

# SSPlayerForGodot のリリース手順

維持者向けです。コントリビューターがこの手順を実行することはありません（[CONTRIBUTING.md](./CONTRIBUTING.md) を参照してください）。

## ドキュメントサイトのビルド

```bash
scripts/prepare-docs.sh          # 一度だけ: .venv と docs/requirements.txt のピン
scripts/build-docs.sh            # 英語 → 日本語の順に、どちらも --strict
scripts/build-pages.sh serve=yes # 公開ツリー、両ロケール -> http://localhost:8000/
```

必ず両ロケールを、英語を先に。英語ビルドは `site/`（その中に `site/ja` がある）を消すため、英語だけ建てると古い日本語サイトが残り、壊したページが無傷に見えます。`mkdocs.base.yml` が `strict: true` を設定します。`zensical serve` は `--strict` を付けても何も検証せず、1 ロケールずつしか配信しないため、言語セレクタの動作を確認できるのは `build-pages.sh serve=yes` だけです。`pages.yml` のビルドジョブはそのスクリプトをすべてのオプションを明示して実行するので、CI と手元のゲートがずれません。Windows は
`.ps1` 双子、どちらも `key=value` オプションと `--help` を取ります。

## リリースの手順

リリースはタグです。先にタグを push し、ビルドはその後です。`v<version>` を push し、そのタグから
**release gdextension** を `upload_release=true` でディスパッチします。Release の名前はタグで決まり、それ以外からの `upload_release=true` は、Release を作らずに済ませるのではなく実行を失敗させます。
`release/X.Y` ブランチからの既定のディスパッチは QA 用ビルドで、Release は作りません。Release は必ず下書きで作られます。人が asset と生成された notes を確認し、pre-release / latest を選んで UI から公開してください。

`scripts/build-release.sh` がパッケージです。ユーザーがプロジェクトに置く `addons/spritestudio/`
フォルダ — 記述子、そこが参照するアイコン、同梱バイナリのライセンス一式、6 プラットフォーム分の
`bin/<platform>/` — と、その zip および `SHA256SUMS` を作ります。`release.yml` のパッケージジョブはこのスクリプト（オプションはすべて明示）です。ビルドは一切行わないので、マトリクス実行の成果物を数秒で流し直せます。

```bash
gh run download <run-id> -D artifacts
scripts/build-release.sh
```

このスクリプトの検査は、パイプラインの他のどこもやっていないものです。
`misc/spritestudio.gdextension` はプラットフォームとビルドターゲットごとにファイルを 1 つずつ、計 19 個のパスとアイコン 3 つを指名し、**Godot はそれをロード時に解決します**。実際に同梱された名前と食い違っていても、ビルドも zip も失敗しません。ダウンロードした人の、そのプラットフォームでだけ、拡張が読み込まれないだけです。記述子のすべてのパスを、出来上がったアーカイブの中で引き当てます。
