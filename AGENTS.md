# SSPlayerForGodot — AI Agent Guide

## Operating rules

*   Keep changes scoped to the requested task. Unrequested hardening is a suggestion, not a commit; a defect
    you actually hit in the code you touched is not — fix it, or say plainly why you did not.
*   Do not commit, push, or open PRs unless the user explicitly asks.
*   Before editing, read enough surrounding context to understand callers and invariants, the ones below
    included. If the read tool truncated a file, read the remaining ranges first.
*   Follow the existing style of the file you touch (naming, type usage, control flow, error handling).
*   **Anything that exists twice changes twice** — a page and its translation, a `.sh` and its `.ps1`. A
    change that lands in one half is the defect this family produces most.
*   **Never break a line between two Japanese characters**: Chrome draws that break as a visible space.
*   **Finish by running this repository's checks**: the commands in [CONTRIBUTING.md](./CONTRIBUTING.md) —
    build the extension, then `scripts/run-tests.sh`; for docs, the both-locale build in
    [RELEASING.md](./RELEASING.md). Read what a run says it verified rather than its exit code — several
    checks here pass, or skip themselves, when a prerequisite is missing.
*   **Do not restate here what another file owns.** The table below says which file that is; a fact that
    belongs to none of them goes where it will be seen to go stale.
*   **Never commit inside the `ss_player/SpriteStudio-SDK` submodule.** A change the core needs is a pull
    request in the SDK repository, followed by a submodule bump here.
*   **Playback semantics are the SDK's** — see [its AGENTS.md](./ss_player/SpriteStudio-SDK/AGENTS.md); do
    not reimplement the rules it owns.
*   **Code comments do not cite the SDK's porting guide** — no section numbers, page paths or quotes.
    Its sections move as it is revised and nothing checks a citation inside a comment; say what breaks
    if the code changes, and leave the rule to the guide.
*   **Never quote or include `godot` or `godot-cpp` source** in responses or suggestions; they are external
    dependencies.

## What this repo is

A Godot integration for SpriteStudio: a C++ `SpriteStudioPlayer2D` node, buildable either as a
**GDExtension** or as a **custom module**, driving the Rust `libssruntime` over a C-API to play
`.ssab` binaries. `.sspj` projects are converted at import time with `libssconverter`. The C++ side
owns node lifecycle, resources and batch rendering.

| Path | Role |
|---|---|
| `ss_player/` | C++ source: node bindings, editor import dock, FFI wrappers. |
| `ss_player/runtime/` | Binary artifacts (`libssruntime`, `libssconverter`) and FFI headers. |
| `ss_player/format/` | FlatBuffers-generated headers. |
| `ss_player/SpriteStudio-SDK/` | Submodule: the Rust runtime and converter. |
| `test_gdextension/` | The headless GDScript suite. Not a sample, and not under `examples/`. |

## Where each answer is written

| Question | File |
|---|---|
| What the plugin does, how to install and use it | [README.md](./README.md), the [docs site](./docs/en/) |
| How to build and test from a checkout | [CONTRIBUTING.md](./CONTRIBUTING.md), [Build Guide](./docs/en/setup/build.md) |
| How the site is built and a release is cut | [RELEASING.md](./RELEASING.md), maintainers only |
| What to build next, and what is out of scope | [ROADMAP.md](./ROADMAP.md) |
| What each script does and takes | that script's `--help`, and its header comment |
| Why the headless run seeds `.godot/extension_list.cfg` | the comment block in `scripts/run-tests.sh` |

## Invariants

*   **Release every Rust-allocated handle.** `SsState` and its siblings come back from the C-API owned; the
    matching `*_release` is the only thing that frees them.
*   **No per-frame allocations in the playback hot path.** Render from the `DrawBatch` plans the runtime
    emits, as they are.
*   **`scripts/SDK_VERSION.txt` pins the SDK release**, and the binaries in `ss_player/runtime/` must be
    that version. Bumping one without the other is a silent ABI mismatch.
*   **A new C++ source file needs its `SConstruct` / `SCsub` entry** in the same change, or one of the two
    build shapes stops linking while the other keeps working.
*   **Both build shapes share one copy of the playback logic**, separated only by a layer of `#ifdef
    SPRITESTUDIO_GODOT_EXTENSION` adapters — includes and type conversions. Keep behaviour out of those
    adapters; the module build's only guard is that it still compiles.
*   **The headless suite covers neither drawing nor the custom-module build.** `--headless` installs a dummy
    rasteriser, so there are no pixels to compare, and a module binary cannot even open `test_gdextension/`
    — it registers the classes a second time and aborts.
*   **A case that records nothing is a failure, not a pass.** GDScript answers a bad call by abandoning that
    one function and carrying on, so a case that never ran would otherwise be indistinguishable from one
    that passed. Cases step with `advance()` under `ANIMATION_PROCESS_MANUAL`, never the frame clock, so a
    result cannot depend on frame timing.
*   **The headless run is expected to be silent.** The suite ends with an `ENGINE` line counting what Godot
    itself printed, because a case can only assert what the API returned — not that producing that answer
    also made the engine or the runtime complain. Whatever it lists is a defect or a newly tolerated
    message, never background.
*   **Documentation is written twice**, `docs/en/<path>` and `docs/ja/<path>`, with per-locale `assets/` and
    source-relative asset paths — see [CONTRIBUTING.md](./CONTRIBUTING.md).
