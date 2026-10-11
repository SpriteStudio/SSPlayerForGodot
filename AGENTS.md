# SSPlayerForGodot — AI Agent Guide

## Operating rules

*   **Unrequested hardening is a suggestion, not a commit.** A defect you hit in code you touched is not:
    fix it, or say why you did not.
*   **Anything that exists twice changes twice** — a page and its translation, a `.sh` and its `.ps1`.
    Missing one half is this family's most common defect.
*   **Never break a line between two Japanese characters**; Chrome renders the break as a space.
*   **Finish with this repository's checks**: build the extension, then `scripts/run-tests.sh`, and for docs
    the both-locale build — all in [CONTRIBUTING.md](./CONTRIBUTING.md). Read what a run says it verified,
    not its exit code — several checks pass, or skip themselves, without a prerequisite.
*   **Do not restate what another file owns**; the table below says which file. A fact no file owns goes
    where its staleness will be noticed.
*   **Never commit inside the `ss_player/SpriteStudio-SDK` submodule.** A core change is an SDK pull
    request, then a submodule bump here.
*   **Playback and rendering semantics are the SDK's**: its
    [AGENTS.md](./ss_player/SpriteStudio-SDK/AGENTS.md) and the porting guide in
    `ss_player/SpriteStudio-SDK/libs/ssruntime/docs/`. Where the guide is silent, SSPlayerForWgpu
    decides. Do not reimplement either.
*   **Code comments never cite the porting guide** (sections, paths, quotes): it moves, and nothing checks
    the citation. Say what breaks instead.

## What this repo is

A Godot integration for SpriteStudio: a C++ `SpriteStudioPlayer2D` node, built as a **GDExtension** or
a **custom module**, that drives the Rust `libssruntime` over its C-API and converts `.sspj` at import
time with `libssconverter`. `test_gdextension/` is the headless suite, not a sample.

## Where each answer is written

| Question | File |
|---|---|
| Use and install | [README.md](./README.md), the [docs site](./docs/en/) |
| Constraints, build, test | [CONTRIBUTING.md](./CONTRIBUTING.md), [Build Guide](./docs/en/setup/build.md) |
| Releases, maintainers only | [RELEASING.md](./RELEASING.md) |
| Plans and non-goals | [ROADMAP.md](./ROADMAP.md) |
| What a script takes | its `--help` and header comment |

## Invariants

*   **Release every Rust-allocated handle with its `ss_*_destroy`**; nothing else frees the runtime,
    resource, converter or slot handles.
*   **No per-frame allocations in the playback hot path.** Render from the runtime's `DrawBatch` plans as
    they are.
*   **`scripts/SDK_VERSION.txt` pins the SDK release**, and `ss_player/runtime/` must hold that version's
    binaries; bumping one alone is a silent ABI mismatch.
*   **A new C++ source file needs its `SConstruct` / `SCsub` entry** in the same change, or one build shape
    stops linking while the other keeps working.
*   **Both build shapes share one copy of the playback logic.** The `#ifdef SPRITESTUDIO_GODOT_EXTENSION`
    adapters carry only includes and type conversions; keep behaviour out of them, since the module
    build's only guard is that it compiles.
*   **The headless run is expected to be silent**: whatever its `ENGINE` line lists is a defect or a newly
    tolerated message, never background.
