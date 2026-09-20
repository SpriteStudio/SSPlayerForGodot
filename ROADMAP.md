# SpriteStudioPlayer for Godot — Player Roadmap

This is the **Godot Player** roadmap for SSPlayerForGodot. It tracks the Godot-specific implementation tasks for capabilities exposed by the **Rust runtime/converter** (`SpriteStudio-SDK/ROADMAP.md`).

Most items selectively bring worthwhile capabilities from the legacy SS6 players into this SS7-based Godot player — reimagined for SS7 and Godot's architecture, not ported verbatim.

## Design principle — Godot-native integration

SSPlayerForGodot leverages Godot's `CanvasItem` API and `Node2D` paradigms. Features should be designed to fit Godot's idioms naturally:

- **Use Godot's built-in systems:** Rely on `CanvasItem::set_modulate()` and tree inheritance rather than recreating hierarchical color systems. Use Godot's process modes and `Engine::get_time_scale()` instead of custom delta management where possible.
- **Avoid node bloat:** Keep the core playback in `SsInternalPlayer` rendering directly via `RenderingServer` / `CanvasItem` draw calls. Only expose child Nodes (like `SpriteStudioPartAttachment2D`) when the user explicitly needs them.
- **Naming and Style:** Use Godot's GDScript conventions for the public API (`snake_case` methods, proper property hints, Godot Signals for callbacks).

## Status legend

- ☑ **Shipped** — implemented and documented; kept here for the record
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

## ☑ Tier 5 — Manual update / Custom delta (Player-only)

- **Shipped.** `ANIMATION_PROCESS_MANUAL` stops the node advancing itself, and `advance(delta)` steps
  playback and emits `frame_updated` exactly as an automatic tick does — so part attachments stay in
  step. The node keeps its idle notification under `MANUAL` because fire-and-forget audio voices and
  the mask coverage scale still need a per-frame tick.
- **Note on Hierarchical Color**: SS6's `AdditionalColor` is natively covered by Godot's `CanvasItem::set_modulate()` and `self_modulate`. No custom work is needed here unless per-vertex multiplier logic specifically requires it.
- **Documented in**: [Performance Tuning → Driving Playback Yourself](./docs/en/workflow/tips.md).

## ☑ Tier 2 — Per-part runtime overrides (color / cell / visibility) (SDK Phase 2)

- **Shipped.** The SDK's Override Layer API landed and the player wraps it: `set_part_color_override()`
  (single colour and four-corner gradient), `set_part_cell_override()`, `set_part_visibility_override()`,
  the matching `clear_*` calls, `clear_all_part_overrides()`, and a `*_by_index()` variant of each that
  skips the name lookup. Priority modes (`NEXT_KEYFRAME` / `UNTIL_ANIMATION_CHANGE` / `PERMANENT`) are
  exposed as constants.
- **Documented in**: [Scripting & Events → Part Overrides](./docs/en/workflow/usage_scripting.md) and
  [SpriteStudioPlayer2D](./docs/en/api/player.md).

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
     `get_canvas_rect()` already report — the same source wgpu settled `fit` / `align` on, and no
     measurement. Expose the fit in Godot's vocabulary (`stretch_mode` / `expand_mode`, as
     `TextureRect` does) over the family's `fit` (`contain` / `cover` / `none`) + `align`.
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

## ☐ A pure mask inside an Instance part (Player-only)

- **Goal**: a mask part written inside a sub-animation clips that sub-animation's own parts, the way it
  does when the same pack is played directly.
- **Key fact**: `_drawAnimation` calls `_render_mask_coverage` only when `!_parent_driven`, so an Instance
  child never rasterises its own writers, and `_bubble_child_clip_writers` carries **clipping** writers up
  and nothing else. A pure mask inside an instance is therefore dropped: the pack clips correctly played
  on its own and draws unclipped through an Instance part. The SDK's `40_mask.md` §2-7 has a pure mask
  closing *within* the sub-animation, so this is a gap rather than the design.
- **Why it earns a slot**: a sub-animation is the only way SpriteStudio can express **more than one
  independent clipping group** in one animation — scope is draw priority and nothing else, so a second
  mask reaches the first one's targets. Adobe Animate's `Clpb` has no such limit and real exports carry
  several, which is what holds the conversion in `SSProjectGenerator/ROADMAP.md`.
- **Steps**:
  1. Decide where the coverage comes from: a private pass for the child (a second borrowed target, and the
     owner's UV transform no longer describing it), or the child's pure-mask writers bubbled into the
     owner's coverage with a scope confined to the child's own draw-order window — `ss_mask_meta` carries
     `(slot, bit, op, is_clipping)` today and would need the window as well.
  2. Whichever it is, keep `§2-6`: the instance part's composed `mask_influence` / `visible_inside_mask`
     still decide whether the *owner's* mask reaches in, independently of the child's own writers.
- **Done when**: a pack of one drawing part plus the mask that clips it draws the same mounted on an
  Instance part as it does played directly.

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
