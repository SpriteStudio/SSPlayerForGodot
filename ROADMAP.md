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

### 🕒 Dynamic instance swap (⛔ SDK Phase 3)

- **Goal**: Replace the animation mounted on an Instance part at runtime (e.g., for equipment or character variations).
- **Blocked on**: SDK Phase 3 **Instance Lifecycle → Dynamic instance swap**.
- **Task**: Expose an API on `SpriteStudioPlayer2D` to swap an instance part's targeted animation pack/name dynamically using the SDK's shared SSAB registry.
