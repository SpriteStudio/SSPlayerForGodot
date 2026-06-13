# SpriteStudioPlayer for Godot — Player Roadmap

This is the **Godot Player** roadmap for SSPlayerForGodot. It tracks the Godot-specific implementation tasks for capabilities exposed by the **Rust runtime/converter** (`SpriteStudio7-SDK/ROADMAP.md`).

Most items selectively bring worthwhile capabilities from the legacy SS6 players into this SS7-based Godot player — reimagined for SS7 and Godot's architecture, not ported verbatim.

## Design principle — Godot-native integration

SSPlayerForGodot leverages Godot's `CanvasItem` API and `Node2D` paradigms. Features should be designed to fit Godot's idioms naturally:

- **Use Godot's built-in systems:** Rely on `CanvasItem::set_modulate()` and tree inheritance rather than recreating hierarchical color systems. Use Godot's process modes and `Engine::get_time_scale()` instead of custom delta management where possible.
- **Avoid node bloat:** Keep the core playback in `SsInternalPlayer` rendering directly via `RenderingServer` / `CanvasItem` draw calls. Only expose child Nodes (like `SpriteStudioPartAttachment2D`) when the user explicitly needs them.
- **Naming and Style:** Use Godot's GDScript conventions for the public API (`snake_case` methods, proper property hints, Godot Signals for callbacks).

## Status legend

- ☐ **Ready** — Player-only, no SDK dependency; can start now
- ⛔ **Blocked on SDK** — needs an `SpriteStudio7-SDK/ROADMAP.md` phase first
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

## ☐ Tier 5 — Manual update / Custom delta (Player-only)

- **Goal**: Allow fine-grained control over playback updates from GDScript (e.g. for custom pause-groups, slow-mo, or custom stepping).
- **Steps**:
  1. Add a generic `advance(float delta)` method to `SpriteStudioPlayer2D` and an `ANIMATION_PROCESS_MANUAL` to `AnimationProcessMode`.
  2. When set to `MANUAL`, the node skips its internal `_notification` delta update and relies entirely on the user calling `advance()`.
- **Note on Hierarchical Color**: SS6's `AdditionalColor` is natively covered by Godot's `CanvasItem::set_modulate()` and `self_modulate`. No custom work is needed here unless per-vertex multiplier logic specifically requires it.
- **Done when**: A script manually steps the animation in `_process` and playback correctly evaluates at the custom delta.

## ⛔ Tier 2 — Per-part runtime overrides (color / cell / visibility) (SDK Phase 2)

- **Blocked on**: `SpriteStudio7-SDK/ROADMAP.md` Phase 2 **Override Layer API**.
- **Goal**: Override color, cell reference, and visibility of specific parts programmatically.
- **Steps (after SDK FFI exists)**:
  1. Wrap the new FFI in `SsInternalPlayer` (using `resolve_part_index`).
  2. Expose `set_part_color_override(part_name, color, blend_op)`, `set_part_cell_override(part_name, map_name, cell_name)`, etc. to GDScript.
- **Done when**: A GDScript changes a part's color or swaps its cell at runtime.

## ☐/⛔ Tier 3 — Runtime material swap

- **Goal**: Replace a part's `ShaderMaterial` at runtime.
- **Status**: Currently `_partcolor_materials` assigns internal shaders. Investigate if users need the ability to inject custom Godot `ShaderMaterial` instances per-part for custom visual effects.

---

## 🕒 Deferred ("あとで")

### 🕒 Sequence player node (`SpriteStudioSequence2D`) (⛔ SDK Phase 3)

- **Goal**: Play `.ssqe` playlists.
- **Status**: The loader `SSQBResource` is implemented, but there is no node to execute it yet.
- **Blocked on**: SDK Phase 3 **State machine → Sequence playback**.
- **Task**: Once the SDK implements sequence/state-machine advancing, create a `SpriteStudioSequence2D` node (or expand `SpriteStudioPlayer2D`) to utilize the SSQB resource and surface step callbacks to Godot signals.

### 🕒 Collider integration

- **Goal**: Per-part collision shapes with callbacks.
- **Status**: In Godot, users can already use `SpriteStudioPartAttachment2D` and parent a `CollisionShape2D` or `Area2D` to it. This naturally delegates collision to Godot's physics engine.
- **Task**: Evaluate if an auto-sizing `SpriteStudioPartCollider2D` (which reads the part's vertex bounds and updates a BoxCollider/PolygonCollider) is genuinely necessary, or if manual setup on attachments suffices.

### 🕒 Replicate (crowd / mesh sharing) (⛔ SDK Phase 3)

- **Goal**: Draw many instances of the same animation efficiently.
- **Blocked on**: SDK Phase 3 **Instance Lifecycle → Animation Instancing** (Shared evaluation context).
- **Task**: Once `ssruntime` supports computing `FrameData` once and rendering it N times, create a node (e.g. `SpriteStudioReplicate2D`) that binds to an original player's context and simply submits the evaluated batches with a different root `Transform2D`, saving Godot CPU time.

### 🕒 Animation blending / Crossfade (⛔ SDK Phase 3)

- **Goal**: Blend multiple animations or crossfade between them.
- **Blocked on**: SDK Phase 3 **Animation Mixing/Blending** and **State machine implementation**.
- **Task**: Surface the blending/crossfade FFI capabilities to `SpriteStudioPlayer2D`, allowing users to smoothly transition between animations or manage layered blending.

### 🕒 Dynamic instance swap (⛔ SDK Phase 3)

- **Goal**: Replace the animation mounted on an Instance part at runtime (e.g., for equipment or character variations).
- **Blocked on**: SDK Phase 3 **Instance Lifecycle → Dynamic instance swap**.
- **Task**: Expose an API on `SpriteStudioPlayer2D` to swap an instance part's targeted animation pack/name dynamically using the SDK's shared SSAB registry.
