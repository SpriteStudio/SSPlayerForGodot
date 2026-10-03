# SpriteStudio Player for Godot — Player Roadmap

This is the **Godot Player** roadmap for SpriteStudio Player for Godot. It tracks the Godot-specific implementation tasks for capabilities exposed by the **Rust runtime/converter** (`SpriteStudio-SDK/ROADMAP.md`).

Most items selectively bring worthwhile capabilities from the legacy SS6 players into this SS7-based Godot player — reimagined for SS7 and Godot's architecture, not ported verbatim.

## Design principle — Godot-native integration

SpriteStudio Player for Godot leverages Godot's `CanvasItem` API and `Node2D` paradigms. Features should be designed to fit Godot's idioms naturally:

- **Use Godot's built-in systems:** Rely on `CanvasItem::set_modulate()` and tree inheritance rather than recreating hierarchical color systems. Use Godot's process modes and `Engine::get_time_scale()` instead of custom delta management where possible.
- **Avoid node bloat:** Keep the core playback in `SsInternalPlayer` rendering directly via `RenderingServer` / `CanvasItem` draw calls. Only expose child Nodes (like `SpriteStudioPartAttachment2D`) when the user explicitly needs them.
- **Naming and Style:** Use Godot's GDScript conventions for the public API (`snake_case` methods, proper property hints, Godot Signals for callbacks).

## Status legend

- ☐ **Ready** — Player-only, no SDK dependency; can start now
- ⛔ **Blocked on SDK** — needs an `SpriteStudio-SDK/ROADMAP.md` phase first
- 🕒 **Deferred ("あとで")** — intentionally postponed; detailed here so it can be picked up later

---

## ☐ Tier 4 — Label / frame-range / index playback (Player-only)

- **Goal**: Play a named label range, start-offset, and play by animation index.
- **Key fact**: Labels are already available in the `.ssab` FlatBuffers payload (`AnimationData.Labels`). No runtime/SDK change is needed.
- **Steps**:
  1. `SsInternalPlayer`: Add `bool try_resolve_label_frame(const String& name, float& r_frame)` to iterate and match the label name.
  2. `SpriteStudioPlayer2D`: Add `bool play_range(const String& start_label, int start_offset, const String& end_label, int end_offset)` which seeks the start frame, sets the section, and starts playback.
  3. `SpriteStudioPlayer2D`: Add `bool play_by_index(int index)` to resolve `index` to a name and call `play()`.
- **Done when**: A sample project triggers label-based playback via GDScript and stays within the loop range.

## ☐/⛔ Tier 3 — Runtime material swap

- **Goal**: Replace a part's `ShaderMaterial` at runtime.
- **Status**: Currently `_partcolor_materials` assigns internal shaders. Investigate if users need the ability to inject custom Godot `ShaderMaterial` instances per-part for custom visual effects.

## ☐ Tier 6 — UI integration (`Control`) (Player-only)

- **Goal**: A player that behaves like a Godot UI node, not merely one that draws inside a UI screen.
- **Key fact**: `SpriteStudioPlayer2D` is a `Node2D`. A `Node2D` under a `Control` draws, but it is invisible
  to every UI system that matters — `Container` layout, anchors and offsets, `_gui_input`, focus traversal,
  and `theme`. Everything else here follows from that one gap.
- **Steps**:
  1. `SpriteStudioPlayerUI` (`Control`): the same `SsInternalPlayer` behind a Control. `_get_minimum_size()`
     returns the animation's content box so a `VBoxContainer` can size it, and the draw fits the animation
     into the node's rect. The box is the **authored canvas**, which `get_canvas_size()` /
     `get_canvas_rect()` already report, and no measurement. Expose the fit in Godot's vocabulary
     (`stretch_mode` / `expand_mode`, as `TextureRect` does) over the family's `fit` (`contain` /
     `cover` / `none`) + `align`; the names, defaults and arithmetic are the SDK porting guide's
     `50_features/70_placement.md`.
  2. Part rect read-out — `get_part_rect(part) -> Rect2` in player-local space, built from the `PartState`
     fields already delivered every frame (`size_x` / `size_y`, `anchor`, `pivot`) and the part transform.
     The auto-sizing collider under *Collider integration* below reads the same thing.
  3. Per-part hit testing — `hit_test_part(point) -> int` and a `_has_point()` override on the Control, so an
     irregular button stops eating clicks around its artwork and mouse filtering lets what is behind through.
  4. UI state binding — map `normal` / `hover` / `pressed` / `disabled` / `focused` to an animation or a
     label, driven by a `BaseButton`'s signals. Keep it properties on the node rather than a second node
     (*Avoid node bloat*).
  5. The two part kinds authored for UI, neither of which draws today
     (`ss_internal_player.cpp:1755`). **Nines** is the cheap one — the runtime hands over finished vertices
     (`get_nines_*`), so it is a draw path and nothing more. **Text** arrives as a batch with no geometry
     plus the authored string: draw it through Godot's `Font` / `TextServer`, and allow substituting the
     string at runtime so a UI label can be localized.
- **Done when**: a demo screen puts a player inside a `VBoxContainer`, resizes with the window, receives
  `gui_input` on its own artwork, and changes animation as a `Button` is hovered and pressed.

## ☐/⛔ The six blend modes that draw as Mix (Player-only)

- **Goal**: a part authored as Screen, Exclusion, Invert, Div2, Screen2 or Overlay2 composites in that mode
  instead of as ordinary alpha blending.
- **Key fact**: `gpu_blend_for` resolves every part's blend to one of the four a canvas item's `render_mode`
  can express — Mix / Mul / Add / Sub; it also offers `blend_premul_alpha` and `blend_disabled`, and takes
  no blend factors of its own. Mulalpha and Mul2 draw as Mul, the state the reference Player
  (SSPlayerForWgpu's `blend_state`) and `ssplayer-pixi` give them, which no one has checked against the
  Editor. The six left split two ways:
  - **Backdrop as a factor** — Screen, Screen2 (which wgpu draws with Screen's state), Invert and Exclusion
    take `1 − dst` as the source factor, which `render_mode` cannot say. Here the shader has to read the
    backdrop (`hint_screen_texture`) and write the composite itself.
  - **Undefined** — Div2 and Overlay2 have no formula with a source anywhere in the family. Every Player
    draws them as Mix, and wgpu's roadmap blocks them on a formula, a fixture and agreement across the
    Players; they stay Mix here until that lands.
- **Why it earns a slot**: `SSProjectGenerator/fixtures/spine/downloaded/xiaoz_sspj` has 21 of its 510 parts
  in Screen — light effects painted over a black ground — and Mix draws every one of them as a black panel
  behind the character.
- **Steps**:
  1. Screen / Screen2 / Invert / Exclusion: a partcolor shader variant that samples `hint_screen_texture`
     and writes the composite. Godot copies the back buffer for the first screen-reading item of a frame
     only and later readers reuse that copy, so two overlapping Screen parts would each miss the other:
     mark each such batch's canvas item with `canvas_item_set_copy_to_backbuffer` (what `BackBufferCopy`
     does), its rect cut to the batch's bounds.
     - **Cost**: one copy per such batch per frame, all of it on the GPU. `xiaoz`'s 21 Screen parts reach
       the GPU as 11 batches on every frame of all three animations. Standing in 11 `BackBufferCopy` +
       screen-reading quads for them over the rig, on an M4 Max at 1920×1080 under Vulkan: the rig alone
       draws in 0.13–0.14 ms, with full-viewport copies in 0.94–1.29 ms, with 300×300 rect copies in
       0.43–0.53 ms; CPU time does not move. Unmeasured: the Compatibility renderer, whose GPU timer reads
       0 on macOS, and any tile-based mobile GPU, where each copy also ends and restarts the render pass.
     - **Copy-free alternative for Screen / Screen2**: `blend_premul_alpha`, with the output alpha set to
       `max(r, g, b)` of the premultiplied source — `s + d·(1 − max(s))` against the exact
       `s + d·(1 − s)`. Exact for grey light; a coloured light darkens the backdrop's weaker channels
       slightly. Untried. Invert and Exclusion have no such form, so they need the copy either way.
     - Settle between the two on a mobile measurement.
  2. Rewrite the *Blend Modes* section of `docs/en/limitations.md` and `docs/ja/limitations.md` to the modes
     that remain.
- **Done when**: `xiaoz`'s `亮小照_纯亮版` draws its light effects without the black panels and matches
  wgpu's drawing of the same frame.

---

## 🕒 Deferred ("あとで")

### 🕒 Sequence player node (`SpriteStudioSequence2D`) (⛔ SDK Phase 3)

- **Goal**: Play `.ssqe` playlists.
- **Status**: The loader `SSQBResource` is implemented, but there is no node to execute it yet.
- **Blocked on**: SDK Phase 3 **State machine → Sequence playback**.
- **Task**: Once the SDK implements sequence/state-machine advancing, create a `SpriteStudioSequence2D` node (or expand `SpriteStudioPlayer2D`) to utilize the SSQB resource and surface step callbacks to Godot signals.

### 🕒 Stretch a 9-slice part to a layout rect (⛔ needs SDK)

- **Goal**: A Nines part that follows the size of the `Control` it lives in, so an authored window frame is a
  real UI panel rather than a picture of one.
- **Status**: Drawing a Nines part at all is Tier 6 step 5; this is the step after it. The runtime evaluates
  the part's size from its keyframes and hands over finished vertices, and the Override Layer covers colour /
  cell / visibility only — so nothing can resize one from the host.
- **Blocked on**: the SDK roadmap's **Per-part size override** (Phase 3) — the Override Layer extended with
  `size_x` / `size_y`. A Nines part's size is inherited by its children, so a label anchored in a stretched
  panel moves with the panel; that cascade is settled SDK-side, not here.

### 🕒 Collider integration

- **Goal**: Per-part collision shapes with callbacks.
- **Status**: In Godot, users can already use `SpriteStudioPartAttachment2D` and parent a `CollisionShape2D` or `Area2D` to it. This naturally delegates collision to Godot's physics engine.
- **Task**: Evaluate if an auto-sizing `SpriteStudioPartCollider2D` (which reads the part's vertex bounds and updates a BoxCollider/PolygonCollider) is genuinely necessary, or if manual setup on attachments suffices.

### 🕒 Replicate (crowd / mesh sharing) (⛔ SDK Phase 3)

- **Goal**: Draw many instances of the same animation efficiently.
- **Blocked on**: SDK Phase 3 **Instance Lifecycle → Animation Instancing** (Shared evaluation context).
- **Task**: Once `ssruntime` supports computing `FrameData` once and rendering it N times, create a node (e.g. `SpriteStudioReplicate2D`) that binds to an original player's context and simply submits the evaluated batches with a different root `Transform2D`, saving Godot CPU time.

### 🕒 Dynamic instance swap (⛔ SDK Phase 3)

- **Goal**: Replace the animation mounted on an Instance part at runtime (e.g., for equipment or character variations).
- **Blocked on**: SDK Phase 3 **Instance Lifecycle → Dynamic instance swap**.
- **Task**: Expose an API on `SpriteStudioPlayer2D` to swap an instance part's targeted animation pack/name dynamically using the SDK's shared SSAB registry.
