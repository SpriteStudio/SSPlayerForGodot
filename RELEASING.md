# Releasing SSPlayerForGodot

Maintainers only. Contributors do not run any of this — see [CONTRIBUTING.md](./CONTRIBUTING.md).

## Cutting a release

Anyone with the Write role on this repository can cut a release: every step is a branch, a pull request, a tag, a workflow run or a Release, and the signing credentials are organization secrets, so nothing depends on whose machine runs it beyond the two checks in step 3 that no CI covers. You need `git` and `gh` 2.87 or later (run `gh auth login` once), plus the toolchain in [CONTRIBUTING.md](./CONTRIBUTING.md) for those checks. The repository needs GitHub Pages enabled with GitHub Actions as its source, and a `v*` tag rule on its `github-pages` environment, or publishing a Release deploys nothing; both are settings a repository admin makes. The commands are bash: in zsh, run `setopt interactive_comments` first, so that the `#` comments are not taken as arguments; on Windows, use Git Bash. Steps 1, 4 and 7 switch branches, so run them in a checkout of their own — a clone, or one made with `git worktree add` — not in one where other work is going on.

A release is a tag on `main`, pushed first and built second. The version is written in one place, `ss_player/VERSION.txt`, and it holds the tag itself — `v7.0.0-beta.1` — which the build stamps, without the `v`, into the binaries. What a Release carries is the GDExtension add-on, `ssplayer-godot-extension-<godot>.zip` for the Godot API that `release.yml` builds against; the custom-module engine is built from source by whoever uses it. The SDK it ships is pinned on its own, in `scripts/SDK_VERSION.txt`. Branches follow git-flow: `develop` integrates, `main` holds released commits only, and `release/X.Y` lives for one release — cut from `develop` when the release ships `develop`'s work, or from the line's latest tag when it ships only fixes on top of it, and deleted once the release is out.

Every block runs as written in a new shell: it works out what it needs from the checkout, git and GitHub. Two values are set by hand, written as placeholders to fill in: `<version>`, the version being released, as in `7.0.1`, in steps 1 and 2; and `<sdk-tag>`, the SpriteStudio-SDK release it ships, as in `v7.0.0`, in step 2. Left as they are, the line stops with a syntax error. A value that would not stop a command by itself when empty is read as `${VAR:?}`: an empty `SDK_TAG` would pin no SDK, and an empty `RUN` would let the signing check in step 3 print nothing and pass.

An agent can run every block, but two steps are a person's to decide: the merge in step 4, which puts the release on `main`, and step 6, which publishes it for good.

### 1. Branch

A release that ships `develop`'s work is cut from `develop`: a new `X.Y`, or `7.0.0` after `7.0.0-beta.1`. A patch release ships only fixes: it is cut from the line's latest tag, and its fixes are committed or cherry-picked onto it. Either way, an earlier `release/X.Y` that is still there is replaced. Run the block that fits.

For `develop`'s work:

```bash
VERSION=<version>
BRANCH=release/$(echo "${VERSION:?}" | cut -d. -f1,2)
git fetch origin
git switch -C "$BRANCH" origin/develop
```

For a patch:

```bash
VERSION=<version>
BRANCH=release/$(echo "${VERSION:?}" | cut -d. -f1,2)
git fetch origin --tags
BASE=$(git -c versionsort.suffix=- tag --list "v${BRANCH#release/}.*" --sort=-v:refname | head -n 1)   # the line's latest tag
git switch -C "$BRANCH" "${BASE:?}"
```

### 2. SDK, version and changelog

In one commit. Pin the SpriteStudio-SDK release this one ships, in both places it is read from: `scripts/SDK_VERSION.txt`, the tag `download-sdk` fetches the runtime by, and the `ss_player/SpriteStudio-SDK` submodule. A new checkout has not initialized the submodule, and until it does, `git -C` on that directory reaches this repository instead, so the block initializes it first and goes no further if that fails. A release that keeps its SDK pins the tag `scripts/SDK_VERSION.txt` already holds. The block then sets `ss_player/VERSION.txt`; `<version>` is the one from step 1.

```bash
VERSION=<version>
SDK_TAG=<sdk-tag>
git submodule update --init ss_player/SpriteStudio-SDK &&
  git -C ss_player/SpriteStudio-SDK fetch --tags origin &&
  git -C ss_player/SpriteStudio-SDK checkout "${SDK_TAG:?}" &&
  printf '%s\n' "$SDK_TAG" > scripts/SDK_VERSION.txt &&
  printf 'v%s\n' "${VERSION:?}" > ss_player/VERSION.txt
```

When the SDK's `.fbs` schemas changed from the previous pin, regenerate the FlatBuffers headers (`flatc` required). This block does that only when they did:

```bash
git -C ss_player/SpriteStudio-SDK diff --quiet "$(git rev-parse HEAD:ss_player/SpriteStudio-SDK)" HEAD -- libs/ssruntime/fbs libs/ssab/fbs ||
  scripts/generate-fbs-code.sh
```

Then date the version's heading in `CHANGELOG.md` as `## [X.Y.Z] - YYYY-MM-DD`, adding the heading if the version has none yet, and commit:

```bash
git add scripts/SDK_VERSION.txt ss_player/SpriteStudio-SDK ss_player/format ss_player/VERSION.txt CHANGELOG.md
git commit -m "chore(release): $(cat ss_player/VERSION.txt)"
```

### 3. Pull request, QA build and local checks

Open the pull request into `main`; its CI builds the GDExtension for Linux and the Web.

```bash
BRANCH=$(git branch --show-current | grep '^release/')
git push -u origin "${BRANCH:?}"
gh pr create -B main -H "${BRANCH:?}" --title "Release $(cat ss_player/VERSION.txt)" --body ""
```

Then dispatch the release workflow from the branch. A run from a branch builds all six platforms, signs the macOS and iOS binaries, assembles the add-on and checks it, keeps the result on the run (`ssplayer-godot-release-dist-<godot>`), and creates no Release. Fix anything on the branch and run this block again, until the run, the pull request's CI and the two checks below all pass.

```bash
BRANCH=$(git branch --show-current | grep '^release/')
git push origin "${BRANCH:?}" &&
  RUN=$(gh workflow run release.yml --ref "${BRANCH:?}" | sed 's|.*/||')
gh run watch "${RUN:?}" --exit-status
gh run view "${RUN:?}" --log | grep -E 'not set|Skipping' | grep -v 'echo '   # must print nothing
```

A missing signing secret does not fail the macOS and iOS jobs: they skip signing or notarization, say so in the log, and pass. The block's last command looks for that, here and in step 5, and must print nothing. If it prints anything, no change on the branch fixes it: the organization's signing secrets are not reaching this repository. An organization owner decides which repositories they reach, and on GitHub Free they cannot reach a private one. Dispatch again once they do; in step 5, delete the draft first.

Two things no CI runs, so run them on the branch: the headless test suite (the Test section of [CONTRIBUTING.md](./CONTRIBUTING.md)) and a build of the custom-module engine (2-B in the [build guide](./docs/en/setup/build.md)). Neither needs a person once the toolchain is in place, but the suite's verdict is its `RESULT` line, the marker after it and its `ENGINE` line, not its exit code.

### 4. Merge and tag

Merge with a merge commit, never a squash or a rebase, so that `main` keeps the commits the QA build built rather than copies of them. Before merging, check that a QA run built the branch's head green, and that merging it changes nothing in its tree. If it would, `main` holds changes the branch does not, and the tag would go on a tree no QA run built: merge `main` into the branch and go back to step 3.

```bash
BRANCH=$(git branch --show-current | grep '^release/')
git fetch origin
SHA=$(git rev-parse --verify -q "origin/${BRANCH:?}")
gh run list -w release.yml -c "${SHA:?}" -s success -L 1                                             # must list a run
git diff --stat "origin/${BRANCH:?}" "$(git merge-tree --write-tree origin/main "origin/$BRANCH")"   # must print nothing
```

**A person runs the merge**, which puts the release on `main`: choose **Create a merge commit** on the pull request's page, or run

```bash
BRANCH=$(git branch --show-current | grep '^release/')
gh pr merge "${BRANCH:?}" --merge
```

Then tag `main`. The tag is read from `ss_player/VERSION.txt` rather than typed, because nothing else compares the two, and it goes on the merge commit only when its tree is the branch's, which `git diff` confirms by printing nothing:

```bash
git switch main && git pull --ff-only &&
  git diff --stat --exit-code HEAD^2 HEAD &&
  TAG=$(cat ss_player/VERSION.txt) &&
  git tag -a "$TAG" -m "$TAG" && git push origin "$TAG"
```

### 5. Build the Release

Dispatch from the tag on `main`. The run builds again and creates a **draft** Release carrying the add-on zip, `SHA256SUMS` and generated notes; nothing is public yet. A tag that already has a Release, draft or published, keeps it as it is: the run builds, says so in a notice and creates nothing, so delete a draft before dispatching again ([Undoing a release](#undoing-a-release)). With `-f upload_release=false` a run from the tag only builds, its files kept on the run. The block's last command checks the signing as in step 3.

```bash
git fetch origin --tags
TAG=$(git describe --exact-match --tags --match 'v[0-9]*' origin/main)
RUN=$(gh workflow run release.yml --ref "${TAG:?}" -f upload_release=true | sed 's|.*/||')
gh run watch "${RUN:?}" --exit-status
gh run view "${RUN:?}" --log | grep -E 'not set|Skipping' | grep -v 'echo '   # must print nothing
```

### 6. Publish

**A person runs this step**: the notes and the choice between a pre-release and the latest release are judgments, and a published Release is permanent. Open the draft on the [Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases) page, check that the add-on zip and `SHA256SUMS` are attached, edit the notes, choose **Set as a pre-release** or **Set as the latest release** (a tag with a `-` can be either), and **Publish release**.

Publishing also starts `pages.yml`, which deploys the [documentation site](https://cri-middleware.github.io/SSPlayerForGodot/). Watch the run through, then open the site: a failed run leaves the Release published and the site as it was. Once the cause is fixed, re-run it with `gh run rerun "${RUN:?}"`; a re-run builds the same tag.

```bash
git fetch origin --tags
TAG=$(git describe --exact-match --tags --match 'v[0-9]*' origin/main)
RUN=$(gh run list -w pages.yml -b "${TAG:?}" -L 1 --json databaseId -q '.[0].databaseId')
gh run watch "${RUN:?}" --exit-status             # RUN is empty until the run appears: repeat the last two lines
```

### 7. Merge back

Merge `main` into `develop`. After a patch release this conflicts where `develop` has moved on: keep `develop`'s version and its `Unreleased` heading, with the patch's section below it. When the next version is decided, set it in `ss_player/VERSION.txt` on `develop` and open a `## [X.Y.Z] - Unreleased` heading for it.

```bash
git fetch origin --tags
TAG=$(git describe --exact-match --tags --match 'v[0-9]*' origin/main)
git switch develop && git pull --ff-only &&
  git merge --no-ff origin/main && git push origin develop
git push origin --delete "release/$(echo "${TAG:?}" | sed 's/^v//' | cut -d. -f1,2)"
```

If the button on its pull request already deleted the release branch, the last line only reports that the branch does not exist. Nothing needs it afterwards: the line's next release cuts it again in step 1.

## Undoing a release

Until the Release is published, it can be undone. The draft goes first, if the run made one:

```bash
git fetch origin --tags
TAG=$(git describe --exact-match --tags --match 'v[0-9]*' origin/main)
gh release delete "${TAG:?}" --yes
```

A run that failed for a transient reason is then dispatched again from the same tag, with step 5's block. A fault that needs new commits takes the tag down too, on GitHub and here; then fix it on the branch — restore it from its pull request if it was deleted — and go back to step 3.

```bash
git fetch origin --tags
TAG=$(git describe --exact-match --tags --match 'v[0-9]*' origin/main)
git push origin ":refs/tags/${TAG:?}" && git tag -d "${TAG:?}"
```

**Once published, the tag and its assets are permanent** — users have the add-on, and the version stamped into it names the tag — so a fix is a new patch version.

## Replaying the packaging

`scripts/build-release.sh` is the package: the `addons/spritestudio/` folder a user drops into a project — the descriptor, the icons it points at, every licence the shipped binaries carry, the documentation (`README.md` and the English pages under `docs/`, from the same commit), and `bin/<platform>/` for all six — then the zip and `SHA256SUMS`. `release.yml`'s package job **is** this script. It builds nothing, so it replays a matrix run in seconds — any run from the last week, as the workflow keeps its artifacts for seven days. `<run-id>` is that run's ID. The block checks out the commit the run built first: the script takes the documentation from the checkout, and the tag it names the release by too.

```bash
RUN=<run-id>
git checkout "$(gh run view "${RUN:?}" --json headSha -q .headSha)" &&
  gh run download "${RUN:?}" -D artifacts &&
  scripts/build-release.sh
```

Its check is the one nothing else in the pipeline does. `misc/spritestudio.gdextension` names a file per platform and build target — nineteen paths — plus three icons, and **Godot resolves them at load time**: a name that does not match what shipped fails no build and no zip, and the extension simply does not load, on that one platform, for whoever downloaded it. Every path in the descriptor is looked up inside the finished archive, and so is every page `README.md` links to.

<br>

---

# SSPlayerForGodot のリリース手順

維持者向けです。コントリビューターがこの手順を実行することはありません（[CONTRIBUTING.md](./CONTRIBUTING.md) を参照してください）。

## リリースの手順

このリポジトリの Write ロールを持つ人なら誰でもリリースできます。どの手順もブランチ・プルリクエスト・タグ・ワークフローの実行・Release のいずれかで、署名の資格情報は組織の Secret なので、CI が受け持たない手順 3 の 2 つの確認を除けば、誰の手元で実行しても結果は変わりません。必要なのは `git` と `gh` 2.87 以降（最初に一度 `gh auth login`）で、その 2 つの確認には [CONTRIBUTING.md](./CONTRIBUTING.md) のツールチェーンも要ります。リポジトリ側では、GitHub Pages を有効にしてソースを GitHub Actions にし、`github-pages` environment に `v*` のタグのルールを足しておく必要があります。どちらかが欠けると、Release を公開しても何もデプロイされません。どちらもリポジトリの admin が行う設定です。コマンドは bash です。zsh では、`#` のコメントが引数として渡されないよう、先に `setopt interactive_comments` を実行します。Windows では Git Bash を使います。手順 1・4・7 はブランチを切り替えるので、ほかの作業をしていない専用のチェックアウト（クローンか、`git worktree add` で作ったもの）で実行してください。

リリースは `main` 上のタグです。先にタグを push し、ビルドはその後です。バージョンが書かれているのは `ss_player/VERSION.txt` の 1 か所だけで、中身はタグそのもの（`v7.0.0-beta.1`）です。ビルドは `v` を除いた値をバイナリに刻みます。Release に載るのは GDExtension のアドオン、`release.yml` がビルドする Godot API 向けの `ssplayer-godot-extension-<godot>.zip` です。custom module 版のエンジンは、使う人がソースからビルドします。同梱する SDK のバージョンはこれとは別で、`scripts/SDK_VERSION.txt` で固定します。ブランチは git-flow です。`develop` が統合ブランチで、`main` にはリリース済みのコミットだけが入ります。`release/X.Y` は 1 回のリリースの間だけ使います。`develop` の作業を出すリリースなら `develop` から、タグの上に修正だけを載せるリリースならその系統の最新のタグから切り、リリースが済んだら消します。

どのブロックも、新しいシェルで書かれたとおりに実行できます。必要なものはチェックアウト・git・GitHub から求めます。手で設定する値は 2 つで、埋めるプレースホルダとして書いてあります。手順 1 と 2 の `<version>` はリリースするバージョン（`7.0.1` など）、手順 2 の `<sdk-tag>` は同梱する SpriteStudio-SDK のリリース（`v7.0.0` など）です。埋めずに実行すると、その行は構文エラーで止まります。空でもそれだけでは止まらない値は `${VAR:?}` で読みます。`SDK_TAG` が空だと SDK を何も固定せず、`RUN` が空だと手順 3 の署名の確認が何も出力せずに通ってしまうためです。

どのブロックもエージェントに実行させられますが、2 つの手順は人が判断します。リリースを `main` に入れる手順 4 のマージと、取り消せない公開を行う手順 6 です。

### 1. ブランチ

`develop` の作業を出すリリースは `develop` から切ります。新しい `X.Y` や、`7.0.0-beta.1` の後の `7.0.0` がこれに当たります。パッチリリースは修正だけを出すもので、その系統の最新のタグから切り、修正をそこへコミットするか cherry-pick します。どちらの場合も、以前の `release/X.Y` が残っていれば置き換えます。当てはまる方のブロックを実行します。

`develop` の作業を出すとき:

```bash
VERSION=<version>
BRANCH=release/$(echo "${VERSION:?}" | cut -d. -f1,2)
git fetch origin
git switch -C "$BRANCH" origin/develop
```

パッチのとき:

```bash
VERSION=<version>
BRANCH=release/$(echo "${VERSION:?}" | cut -d. -f1,2)
git fetch origin --tags
BASE=$(git -c versionsort.suffix=- tag --list "v${BRANCH#release/}.*" --sort=-v:refname | head -n 1)   # その系統の最新のタグ
git switch -C "$BRANCH" "${BASE:?}"
```

### 2. SDK、バージョン、CHANGELOG

1 コミットで行います。同梱する SpriteStudio-SDK のリリースを、読まれる 2 か所の両方で固定します。`download-sdk` がランタイムを取得するときに使うタグ `scripts/SDK_VERSION.txt` と、`ss_player/SpriteStudio-SDK` submodule です。新しいチェックアウトでは submodule が初期化されておらず、初期化するまではそのディレクトリへの `git -C` がこのリポジトリに届いてしまいます。そのためブロックは先に初期化し、失敗したらそこで止まります。SDK を変えないリリースでは、`scripts/SDK_VERSION.txt` にいまあるタグを固定します。続けてブロックは `ss_player/VERSION.txt` を設定します。`<version>` は手順 1 と同じものです。

```bash
VERSION=<version>
SDK_TAG=<sdk-tag>
git submodule update --init ss_player/SpriteStudio-SDK &&
  git -C ss_player/SpriteStudio-SDK fetch --tags origin &&
  git -C ss_player/SpriteStudio-SDK checkout "${SDK_TAG:?}" &&
  printf '%s\n' "$SDK_TAG" > scripts/SDK_VERSION.txt &&
  printf 'v%s\n' "${VERSION:?}" > ss_player/VERSION.txt
```

SDK の `.fbs` スキーマが前の固定から変わったときは、FlatBuffers のヘッダーを生成し直します（`flatc` が必要です）。次のブロックは、変わったときだけそれを行います。

```bash
git -C ss_player/SpriteStudio-SDK diff --quiet "$(git rev-parse HEAD:ss_player/SpriteStudio-SDK)" HEAD -- libs/ssruntime/fbs libs/ssab/fbs ||
  scripts/generate-fbs-code.sh
```

続けて `CHANGELOG.md` のそのバージョンの見出しに日付を入れ、`## [X.Y.Z] - YYYY-MM-DD` とします。そのバージョンの見出しがまだ無ければ足します。そのあとコミットします。

```bash
git add scripts/SDK_VERSION.txt ss_player/SpriteStudio-SDK ss_player/format ss_player/VERSION.txt CHANGELOG.md
git commit -m "chore(release): $(cat ss_player/VERSION.txt)"
```

### 3. プルリクエスト、QA ビルド、手元での確認

`main` へのプルリクエストを作ります。その CI が GDExtension を Linux と Web 向けにビルドします。

```bash
BRANCH=$(git branch --show-current | grep '^release/')
git push -u origin "${BRANCH:?}"
gh pr create -B main -H "${BRANCH:?}" --title "Release $(cat ss_player/VERSION.txt)" --body ""
```

続けて、ブランチからリリースワークフローを実行します。ブランチからの実行では、6 プラットフォームすべてをビルドし、macOS と iOS のバイナリに署名し、アドオンを組み立てて検査し、結果を run に残し（`ssplayer-godot-release-dist-<godot>`）、Release は作りません。問題があればブランチで直し、このブロックをもう一度実行します。run、プルリクエストの CI、下の 2 つの確認がすべて通るまで繰り返します。

```bash
BRANCH=$(git branch --show-current | grep '^release/')
git push origin "${BRANCH:?}" &&
  RUN=$(gh workflow run release.yml --ref "${BRANCH:?}" | sed 's|.*/||')
gh run watch "${RUN:?}" --exit-status
gh run view "${RUN:?}" --log | grep -E 'not set|Skipping' | grep -v 'echo '   # 何も出力されないこと
```

署名の Secret が欠けていても macOS と iOS のジョブは失敗しません。署名や公証を飛ばし、その旨をログに出して成功します。ブロックの最後のコマンドはそれを探すもので、ここでも手順 5 でも何も出力されてはいけません。何か出力されたら、ブランチをどう変えても直りません。組織の署名用 Secret がこのリポジトリに届いていません。どのリポジトリに届けるかは組織のオーナーが決めますが、GitHub Free では private のリポジトリには届けられません。届くようになったら実行し直してください。手順 5 では先に下書きを消します。

どの CI も実行しないものが 2 つあるので、ブランチ上で実行してください。ヘッドレスのテストスイート（[CONTRIBUTING.md](./CONTRIBUTING.md) のテストの節）と、custom module 版エンジンのビルド（[ビルドガイド](./docs/ja/setup/build.md)の 2-B）です。ツールチェーンが揃っていればどちらも人の手は要りませんが、スイートの判定は終了コードではなく、`RESULT` の行、その後のマーカー、`ENGINE` の行で行います。

### 4. マージとタグ

マージはマージコミットで行い、squash や rebase は使いません。QA ビルドしたコミットの写しではなく、そのものを `main` に残すためです。マージの前に、ブランチの先頭を QA の run が緑でビルドしていること、マージしてもブランチのツリーが変わらないことを確かめます。変わるなら `main` にブランチの持たない変更があり、どの QA の run もビルドしていないツリーにタグが付きます。そのときは `main` をブランチへマージして手順 3 に戻ります。

```bash
BRANCH=$(git branch --show-current | grep '^release/')
git fetch origin
SHA=$(git rev-parse --verify -q "origin/${BRANCH:?}")
gh run list -w release.yml -c "${SHA:?}" -s success -L 1                                             # run が 1 つ表示されること
git diff --stat "origin/${BRANCH:?}" "$(git merge-tree --write-tree origin/main "origin/$BRANCH")"   # 何も出力されないこと
```

**マージは人が行います。** リリースが `main` に入るためです。プルリクエストのページで **Create a merge commit** を選ぶか、次を実行します。

```bash
BRANCH=$(git branch --show-current | grep '^release/')
gh pr merge "${BRANCH:?}" --merge
```

続けて `main` にタグを打ちます。タグは手で打たず `ss_player/VERSION.txt` から読みます。この 2 つを照らし合わせるものが他に無いためです。タグはマージコミットのツリーがブランチと同じときにだけ打たれます。`git diff` が何も出力しないことがその確認です。

```bash
git switch main && git pull --ff-only &&
  git diff --stat --exit-code HEAD^2 HEAD &&
  TAG=$(cat ss_player/VERSION.txt) &&
  git tag -a "$TAG" -m "$TAG" && git push origin "$TAG"
```

### 5. Release のビルド

`main` 上のタグからワークフローを実行します。もう一度ビルドし、アドオンの zip と `SHA256SUMS`、自動生成のリリースノートを持つ**下書き**の Release を作ります。まだ何も公開されていません。Release がすでにあるタグでは、下書きでも公開済みでも、その Release に触れません。run はビルドして notice を出すだけなので、もう一度実行するときは先に下書きを消します（[リリースの取り消し](#リリースの取り消し)）。`-f upload_release=false` を付けると、タグからの実行でもビルドだけを行い、成果物を run に残します。ブロックの最後のコマンドで、手順 3 と同じように署名を確認します。

```bash
git fetch origin --tags
TAG=$(git describe --exact-match --tags --match 'v[0-9]*' origin/main)
RUN=$(gh workflow run release.yml --ref "${TAG:?}" -f upload_release=true | sed 's|.*/||')
gh run watch "${RUN:?}" --exit-status
gh run view "${RUN:?}" --log | grep -E 'not set|Skipping' | grep -v 'echo '   # 何も出力されないこと
```

### 6. 公開

**この手順は人が行います。** リリースノートの中身と、pre-release か latest かの選択は判断が要り、公開した Release は取り消せないためです。[Releases](https://github.com/cri-middleware/SSPlayerForGodot/releases) ページで下書きを開き、アドオンの zip と `SHA256SUMS` が添付されていることを確かめ、リリースノートを編集し、**Set as a pre-release** か **Set as the latest release** を選んで（`-` を含むタグはどちらも選べます）、**Publish release** を押します。

公開すると `pages.yml` が動き、[ドキュメントサイト](https://cri-middleware.github.io/SSPlayerForGodot/) をデプロイします。run を最後まで見届けてから、サイトを開いて確かめます。run が失敗しても Release は公開されたままで、サイトは元のまま残ります。原因を直したら `gh run rerun "${RUN:?}"` で再実行します。再実行は同じタグをビルドします。

```bash
git fetch origin --tags
TAG=$(git describe --exact-match --tags --match 'v[0-9]*' origin/main)
RUN=$(gh run list -w pages.yml -b "${TAG:?}" -L 1 --json databaseId -q '.[0].databaseId')
gh run watch "${RUN:?}" --exit-status             # run が出るまで RUN は空。最後の 2 行を繰り返す
```

### 7. develop へ戻す

`main` を `develop` へマージします。パッチリリースの後は、`develop` が先へ進んだ箇所で衝突します。`develop` のバージョンと `Unreleased` の見出しを残し、パッチの節をその下に置いてください。次のバージョンが決まったら、`develop` の `ss_player/VERSION.txt` を設定し、`## [X.Y.Z] - Unreleased` の見出しを立てます。

```bash
git fetch origin --tags
TAG=$(git describe --exact-match --tags --match 'v[0-9]*' origin/main)
git switch develop && git pull --ff-only &&
  git merge --no-ff origin/main && git push origin develop
git push origin --delete "release/$(echo "${TAG:?}" | sed 's/^v//' | cut -d. -f1,2)"
```

プルリクエストのページのボタンでリリースブランチを消してあれば、最後の行は「ブランチが無い」と出るだけです。同じ系統の次のリリースは手順 1 で切り直すので、この後ブランチが要ることはありません。

## リリースの取り消し

Release を公開するまでは取り消せます。run が下書きを作っていれば、先にそれを消します。

```bash
git fetch origin --tags
TAG=$(git describe --exact-match --tags --match 'v[0-9]*' origin/main)
gh release delete "${TAG:?}" --yes
```

一時的な理由で失敗した run は、そのあと手順 5 のブロックで同じタグからもう一度実行します。新しいコミットが要る不具合なら、タグも GitHub と手元の両方から取り下げ、ブランチで直して（消していたらプルリクエストのページから復元して）手順 3 に戻ります。

```bash
git fetch origin --tags
TAG=$(git describe --exact-match --tags --match 'v[0-9]*' origin/main)
git push origin ":refs/tags/${TAG:?}" && git tag -d "${TAG:?}"
```

**公開した後のタグとアセットは恒久的です。** 利用者はそのアドオンを持っていて、アドオンに刻まれたバージョンがそのタグを名指ししているので、修正は新しいパッチバージョンで出します。

## パッケージングの再実行

`scripts/build-release.sh` がパッケージです。ユーザーがプロジェクトに置く `addons/spritestudio/` フォルダ — 記述子、そこが参照するアイコン、同梱バイナリのライセンス一式、ドキュメント（同じコミットの `README.md` と、`docs/` の英語ページ）、6 プラットフォーム分の `bin/<platform>/` — と、その zip および `SHA256SUMS` を作ります。`release.yml` のパッケージジョブはこのスクリプトです。ビルドは一切行わないので、マトリクス実行の成果物を数秒で流し直せます。ワークフローは成果物を 7 日間保持するので、対象は直近 1 週間の run です。`<run-id>` はその run の ID です。スクリプトはドキュメントも、リリースの名前にするタグもチェックアウトから取るので、ブロックは先にその run がビルドしたコミットをチェックアウトします。

```bash
RUN=<run-id>
git checkout "$(gh run view "${RUN:?}" --json headSha -q .headSha)" &&
  gh run download "${RUN:?}" -D artifacts &&
  scripts/build-release.sh
```

このスクリプトの検査は、パイプラインの他のどこもやっていないものです。`misc/spritestudio.gdextension` はプラットフォームとビルドターゲットごとにファイルを 1 つずつ、計 19 個のパスとアイコン 3 つを指名し、**Godot はそれをロード時に解決します**。実際に同梱された名前と食い違っていても、ビルドも zip も失敗しません。ダウンロードした人の、そのプラットフォームでだけ、拡張が読み込まれないだけです。記述子のすべてのパスを、出来上がったアーカイブの中で引き当てます。`README.md` がリンクするページも同じように引き当てます。
