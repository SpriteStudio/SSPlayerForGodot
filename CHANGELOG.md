# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [7.0.0-beta.1] - Unreleased

First release of the SpriteStudio 7 generation of the plugin, and a rewrite of the 1.x
player. Playback is now driven by `libssruntime` from
[SpriteStudio-SDK](https://github.com/cri-middleware/SpriteStudio-SDK) and consumes
converted binaries (`.ssab`) instead of parsing `.sspj` at runtime. See the
[Migration Guide](./docs/en/migration_from_v1.md) for moving a 1.x project across.

### Added
- **`SpriteStudioPlayer2D` node**: plays an `.ssab` animation as an ordinary `Node2D`.
  Transport (`play` / `pause` / `stop`), frame seeking, playback sections, direction,
  speed scale, loop count, frame skipping and sub-frame interpolation.
- **Asset pipeline**: the **SS Import Dock** converts a SpriteStudio project (`.sspj`) to
  `.ssab` by drag & drop and reconverts from the Inspector. `ssconverter-cli` performs the
  same conversion for CI/CD.
- **Resource classes**: `SSABResource` (animation binary) and `SSQBResource` (sequence
  binary). `.ssab` is loaded zero-copy, so playback starts without a parse step.
- **Per-part Override Layer API**: override a part's color (single or per-corner gradient),
  cell and visibility at runtime, addressed by part name or part index, each with a matching
  `clear_*` call.
- **CellMap overrides**: swap an animation's textures at runtime for equipment changes and
  color variants. Cell map / cell names are enumerable from the player and the resource.
- **`SpriteStudioPartAttachment2D` node**: mirrors one part's pose onto a `Node2D` so Godot
  content can be pinned to a part. Modeled on `RemoteTransform2D`.
- **Signals**: timeline `user_data` and `signal_emitted` events, audio events, animation
  lifecycle (`animation_started` / `_changed` / `_finished` / `_looped`), and `frame_updated`.
- **AnimationPlayer integration**: drive playback from Godot's own animation tooling.
- **Audio playback**: audio parts play through Godot, with volume and backend selection.
- **Builds**: GDExtension and custom module, for Windows, macOS, Linux, Android, iOS and
  Web, targeting Godot 4.7.
- **Documentation**: bilingual (EN/JA) documentation site, class reference and contribution
  guidelines.

### Known Limitations
- Text / bitmap font rendering is not yet validated.
- Sequence playback is not implemented. `SSQBResource` loads `.ssqb` files, but no node
  consumes them yet.
- macOS / iOS binaries are not code-signed.
- Web builds are single-threaded only and require WebAssembly SIMD, plus an engine template
  built with `dlink_enabled=yes`.
- For the playback constraints inherited from the shared runtime, see
  [Limitations & Scope](./docs/en/limitations.md).
