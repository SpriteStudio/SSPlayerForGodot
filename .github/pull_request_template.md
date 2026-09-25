<!--
Thanks for contributing! See CONTRIBUTING.md for the constraints this project works under
(every Rust handle released, no per-frame allocation, core logic in the SDK).
CONTRIBUTING.md に本プロジェクトの制約が記載されています。日本語でも英語でも構いません。
-->

## What does this change? / 変更内容

<!-- What it does and why. Link any related issue: Fixes #123 -->

## How was it verified? / 検証方法

<!--
Which checks ran, and on what. If you could not run them, say so —
that is useful information, not a problem.
実行した確認と環境。実行できなかった場合はその旨をお書きください。
-->

- [ ] `./scripts/run-tests.sh` — the verdict is the RESULT line and the `ENGINE` line, not the exit code
- [ ] `./scripts/build-extension.sh`
- [ ] Ran the `examples/` projects in the editor / エディタで `examples/` を確認した
- [ ] Not run — see notes below / 未実行（理由は下記）

**Environment / 環境:** Godot version, rendering method (Forward+ / Mobile / Compatibility), platform

## Checklist

- [ ] Scoped to one thing. Unrelated fixes are easier to review as their own PR.
- [ ] Follows the style of the files it touches.
- [ ] Docs updated if behaviour changed.
- [ ] Anything that exists twice changed twice — a page and its translation, a `.sh` and its `.ps1`.
- [ ] Every Rust-allocated handle is released with its destroy function (`ss_runtime_destroy`, `ss_resource_destroy`, `ss_converter_destroy`).
- [ ] No per-frame allocations: rendering goes straight from DrawBatch.
- [ ] Core logic goes to SpriteStudio-SDK, and nothing is committed inside the submodule.

## Anything reviewers should know? / レビュー時の注意点

<!-- Trade-offs you made, alternatives you rejected, parts you are unsure about. -->
