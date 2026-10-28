# SpriteStudio Player for Godot — Roadmap

This file lists unfinished work only. Technical constraints are in [Limitations](./docs/en/limitations.md); work that
waits on the runtime is under *Needs SDK* and tracked in the
[SDK ROADMAP](https://github.com/cri-middleware/SpriteStudio-SDK/blob/HEAD/ROADMAP.md).

## Playback & rendering

- [ ] **Six blend modes draw as Mix** — Screen, Exclusion, Invert, Div2, Screen2 and Overlay2 composite as ordinary
  alpha blending, because a canvas item's `render_mode` expresses only Mix / Mul / Add / Sub (Mulalpha and Mul2 draw
  as Mul, as in SpriteStudio Player for wgpu and `ssplayer-pixi`, unchecked against the Editor). A Screen light over a
  black ground draws as a black panel; done when such an animation draws without the panels and matches wgpu's frame.
  - [ ] **Screen / Screen2 / Invert / Exclusion** — these take `1 − dst` as the source factor, which `render_mode`
    cannot say, so a partcolor shader variant samples `hint_screen_texture`, with `canvas_item_set_copy_to_backbuffer`
    on each such batch cut to its bounds. That is one GPU copy per batch per frame — about half a millisecond for 21
    Screen parts on a desktop GPU under Vulkan, unmeasured on the Compatibility renderer and tile-based mobile GPUs.
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
  is invisible to `Container` layout, anchors and offsets, `_gui_input`, focus traversal and `theme`.
  `SpriteStudioButton` closes that gap for a button; artwork that is not one — a portrait in a dialog box, a panel —
  needs the same `SsInternalPlayer` behind a `Control` of its own. The check is a demo screen that puts a player inside
  a `VBoxContainer`, resizes with the window and receives `gui_input` on its own artwork. Being a class of its own, it
  costs `SpriteStudioPlayer2D` users nothing.
  - [ ] **Sizing** — `_get_minimum_size()` returns the authored canvas (already reported by `get_canvas_size()`), and
    the draw fits the animation into the node's rect with `SpriteStudioButton`'s `stretch_mode` /
    `alignment_horizontal` / `alignment_vertical`.
  - [ ] **Nines and Text parts** — the two part kinds authored for UI; neither draws today. Nines is a draw path and
    nothing more, because the runtime hands over finished vertices (`get_nines_*`); Text arrives as a batch with no
    geometry plus the authored string, to draw through Godot's `Font` / `TextServer` with the string substitutable at
    runtime so a UI label can be localized.

## Needs SDK

- [ ] **Sequence player node (`SpriteStudioSequence2D`)** — play sequences (`.ssqb`, converted from `.ssqe`): `SSQBResource` loads them but no
  node executes one. Once the runtime advances sequences, add the node (or extend `SpriteStudioPlayer2D`) and surface
  step callbacks as Godot signals. Waits for: State machine implementation (Sequence playback).
- [ ] **Stretch a 9-slice part to a layout rect** — a Nines part that follows the size of the `Control` it lives in, so
  an authored window frame is a real UI panel; the Override Layer covers colour / cell / visibility only, so nothing
  can resize one from the host, and drawing one at all is *Nines and Text parts* above. Waits for: Per-part size
  override.
- [ ] **Replicate (crowd / mesh sharing)** — draw many instances of the same animation efficiently. Once `ssruntime`
  computes `FrameData` once and renders it N times, add a node (e.g. `SpriteStudioReplicate2D`) that binds to an
  original player's context and submits the evaluated batches with a different root `Transform2D`. Waits for:
  Instance Lifecycle API (Animation Instancing).
- [ ] **Dynamic instance swap** — replace the animation mounted on an Instance part at runtime (equipment, character
  variations) through an API on `SpriteStudioPlayer2D` that swaps an instance part's animation pack / name using the
  SDK's shared SSAB registry. Waits for: Instance Lifecycle API (Dynamic instance swap).

## Backlog

- [ ] **Bundle MoltenVK into the self-built iOS template** — an iOS export from a template built here needs
  `MoltenVK.xcframework` embedded and signed by hand (the Build Guide's iOS export steps). Bundling it into the
  template itself (`install-template.sh` / `misc/dist/apple_embedded_xcode`) would let the export embed and sign it, as
  the official templates do.
- [ ] **One parallel pass for every player's frame** — each `SpriteStudioPlayer2D` evaluates its frame and steps its
  effects on the main thread inside its own tick, one player after another, where SpriteStudio Player for Unity runs
  both for all its players in one pass across worker threads. Not done until such a pass keeps what each node's tick
  guarantees today (the order signals reach scripts in, `frame_updated` before the draw, the process modes and a
  hand-driven `advance()`, pause) and the gain is measured at the player counts a game has on screen.
