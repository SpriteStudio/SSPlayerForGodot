# SpriteStudio Player for Godot — Roadmap

This file lists unfinished work only. Technical constraints are in [Limitations](./docs/en/limitations.md); work that
waits on the runtime is under *Needs SDK* and tracked in the
[SDK ROADMAP](https://github.com/cri-middleware/SpriteStudio-SDK/blob/HEAD/ROADMAP.md).

## Design principles

SpriteStudio Player for Godot leverages Godot's `CanvasItem` API and `Node2D` paradigms. Features should be designed to fit Godot's idioms naturally:

- **Use Godot's built-in systems:** Rely on `CanvasItem::set_modulate()` and tree inheritance rather than recreating hierarchical color systems. Use Godot's process modes and `Engine::get_time_scale()` instead of custom delta management where possible.
- **Avoid node bloat:** Keep the core playback in `SsInternalPlayer` rendering directly via `RenderingServer` / `CanvasItem` draw calls. Only expose child Nodes (like `SpriteStudioPartAttachment2D`) when the user explicitly needs them.
- **Naming and Style:** Use Godot's GDScript conventions for the public API (`snake_case` methods, proper property hints, Godot Signals for callbacks).

## Playback & rendering

- [ ] **Motion blending / crossfade** — layer and cross-fade animations on one `SpriteStudioPlayer2D` through the
  runtime's track stack. The runtime ships it and exports `ss_runtime_set_track_animation` / `ss_runtime_crossfade` /
  `ss_runtime_add_track_source` / `ss_runtime_fade_track`, but nothing in `ss_player/` calls them and the node has no
  track or crossfade method; SpriteStudio Player for wgpu is the reference (`set_track_animation` / `crossfade` /
  `fade_track` / …), with the stems in the SDK porting guide's `20_design/30_api_conventions.md` (*Animation mixing*)
  and the design in its `50_features/10_animation_mixing.md`.
- [ ] **Label playback** — set the playback section by a start and an end label, each with an offset. Labels already
  ride in the `.ssab` payload (`AnimationData.Labels`), so no SDK change is needed: resolve them in `SsInternalPlayer`
  and add `set_animation_section_by_label(start_label, end_label, start_offset, end_offset)` on `SpriteStudioPlayer2D`,
  beside the existing `set_animation_section`, under the stem the SDK's API conventions register.
- [ ] **Six blend modes draw as Mix** — Screen, Exclusion, Invert, Div2, Screen2 and Overlay2 composite as ordinary
  alpha blending, because `gpu_blend_for` maps every blend to the four a canvas item's `render_mode` can express
  (Mix / Mul / Add / Sub; Mulalpha and Mul2 draw as Mul, as SpriteStudio Player for wgpu and `ssplayer-pixi` do, which
  no one has checked against the Editor). A converted Spine rig with 21 of its 510 parts in Screen — light effects over
  a black ground — draws each as a black panel; the check is that its light-effect animation draws without the
  panels and matches wgpu's drawing of the same frame.
  - [ ] **Screen / Screen2 / Invert / Exclusion** — these take `1 − dst` as the source factor, which `render_mode`
    cannot say, so a partcolor shader variant has to sample `hint_screen_texture` and write the composite. Godot
    copies the back buffer only for the first screen-reading item of a frame, so mark each such batch's canvas item
    with `canvas_item_set_copy_to_backbuffer`, its rect cut to the batch's bounds: one GPU copy per batch per frame,
    about half a millisecond on the rig above on a desktop GPU under Vulkan; the Compatibility renderer and
    tile-based mobile GPUs are unmeasured.
  - [ ] **Copy-free Screen / Screen2** — `blend_premul_alpha` with the output alpha set to `max(r, g, b)` of the
    premultiplied source: exact for grey light, slightly darkens a coloured light's weaker channels, untried. Invert
    and Exclusion have no such form, so they need the copy either way; settle between the two on a mobile
    measurement.
  - [ ] **Div2 / Overlay2** — no formula with a source exists anywhere in the family and every Player draws them as
    Mix, so they stay Mix here. Blocked on: a formula, a fixture and agreement across the Players, as the wgpu
    ROADMAP says.
  - [ ] **Limitations page** — rewrite the *Blend Modes* section of `docs/en/limitations.md` and
    `docs/ja/limitations.md` to the modes that remain.

## UI integration

- [ ] **`SpriteStudioPlayerUI` (`Control`)** — `SpriteStudioPlayer2D` is a `Node2D`, so under a `Control` it draws but
  is invisible to `Container` layout, anchors and offsets, `_gui_input`, focus traversal and `theme`; the same
  `SsInternalPlayer` behind a `Control` closes that gap. The check is a demo screen that puts a player inside a
  `VBoxContainer`, resizes with the window, receives `gui_input` on its own artwork, and changes animation as a
  `Button` is hovered and pressed.
  - [ ] **Sizing and fit** — `_get_minimum_size()` returns the authored canvas (already reported by
    `get_canvas_size()` / `get_canvas_rect()`) and the draw fits the animation into the node's rect, exposed as
    `stretch_mode` / `expand_mode` as `TextureRect` does over the family's `fit` (`contain` / `cover` / `none`) +
    `align`; the names, defaults and arithmetic are the SDK porting guide's `50_features/70_placement.md`.
  - [ ] **Part rect read-out** — `get_part_rect(part) -> Rect2` in player-local space, built from the `PartState`
    fields delivered every frame (`size_x` / `size_y`, `anchor`, `pivot`) and the part transform; the auto-sizing
    collider under *Collider integration* reads the same thing.
  - [ ] **Per-part hit testing** — `hit_test_part(point) -> int` and a `_has_point()` override on the `Control`, so an
    irregular button stops eating clicks around its artwork and mouse filtering lets what is behind through.
  - [ ] **UI state binding** — map `normal` / `hover` / `pressed` / `disabled` / `focused` to an animation or a label,
    driven by a `BaseButton`'s signals, as properties on the node rather than a second node.
  - [ ] **Nines and Text parts** — the two part kinds authored for UI; neither draws today. Nines is a draw path and
    nothing more, because the runtime hands over finished vertices (`get_nines_*`); Text arrives as a batch with no
    geometry plus the authored string, to draw through Godot's `Font` / `TextServer` with the string substitutable at
    runtime so a UI label can be localized.

## Needs SDK

- [ ] **Sequence player node (`SpriteStudioSequence2D`)** — play `.ssqe` playlists: `SSQBResource` loads them but no
  node executes one. Once the runtime advances sequences, add the node (or extend `SpriteStudioPlayer2D`) and surface
  step callbacks as Godot signals. Waits for: State machine implementation (Sequence playback).
- [ ] **Stretch a 9-slice part to a layout rect** — a Nines part that follows the size of the `Control` it lives in, so
  an authored window frame is a real UI panel; nothing can resize one from the host because the Override Layer covers
  colour / cell / visibility only, and drawing a Nines part at all is *Nines and Text parts* under UI integration. A
  Nines part's size is inherited by its children, so a label anchored in a stretched panel moves with it; that
  cascade is settled SDK-side. Waits for: Per-part size override.
- [ ] **Replicate (crowd / mesh sharing)** — draw many instances of the same animation efficiently. Once `ssruntime`
  computes `FrameData` once and renders it N times, add a node (e.g. `SpriteStudioReplicate2D`) that binds to an
  original player's context and submits the evaluated batches with a different root `Transform2D`. Waits for:
  Instance Lifecycle API (Animation Instancing).
- [ ] **Dynamic instance swap** — replace the animation mounted on an Instance part at runtime (equipment, character
  variations) through an API on `SpriteStudioPlayer2D` that swaps an instance part's animation pack / name using the
  SDK's shared SSAB registry. Waits for: Instance Lifecycle API (Dynamic instance swap).

## Backlog

- [ ] **Runtime material swap** — replace a part's `ShaderMaterial` at runtime: `_partcolor_materials` assigns internal
  shaders, so investigate whether users need to inject their own `ShaderMaterial` per part for custom effects.
- [ ] **Collider integration** — per-part collision shapes with callbacks. A `CollisionShape2D` or `Area2D` parented to
  a `SpriteStudioPartAttachment2D` already delegates collision to Godot's physics, so evaluate whether an auto-sizing
  `SpriteStudioPartCollider2D` (reading the part's vertex bounds) is needed at all.
