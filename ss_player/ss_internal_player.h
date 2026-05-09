#pragma once

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/canvas_item_material.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/vector2.hpp>
using namespace godot;
#else
#include "core/object/ref_counted.h"
#include "core/string/ustring.h"
#include "core/templates/hash_map.h"
#include "core/templates/local_vector.h"
#include "core/templates/vector.h"
#include "core/variant/dictionary.h"
#include "scene/resources/canvas_item_material.h"
#include "scene/resources/texture.h"
#include "servers/rendering/rendering_server.h"
#endif

#include "ssab_resource.h"

// Forward declaration; full definition comes from runtime/include/ssruntime.h.
struct ss_event_instance_info;

namespace ss {
namespace runtime {
struct FrameData;
struct PartState;
struct DrawBatch;
}
namespace format {
struct PartData;
struct SsAnimeBinary;
struct AnimationData;
struct PartAttributeInstance;
}
}

// Receiver for animation events that need to bubble up to engine-level
// signals (GDScript signals, Unity events, etc.). The host (Node2D wrapper,
// editor preview, etc.) implements this. SsInternalPlayer never assumes a
// sink is present — instance-child players run with `nullptr` so their
// events stay internal.
class SsPlayerEventSink {
public:
    virtual ~SsPlayerEventSink() = default;
    virtual void onAnimationStarted(const String& anim_name) {}
    virtual void onAnimationFinished(const String& anim_name) {}
    virtual void onAnimationLooped(const String& anim_name) {}
    virtual void onUserData(const Dictionary& payload) {}
    virtual void onSignal(const String& command, const Dictionary& value) {}
};

// Engine-integration-agnostic SpriteStudio player core. Owns the
// libssruntime context, an SSAB binding, the per-batch canvas items, and a
// recursive `Vector<SsInternalPlayer*>` of children for Instance parts. Has
// no Node tree dependency: a single `_root_ci` canvas item carries this
// player's transform / visibility, and all per-batch canvas items hang off it
// via `canvas_item_set_parent`. Hosts (Node2D, editor previews, ...) wrap
// it and provide a parent canvas RID + an event sink.
class SsInternalPlayer {
public:
    SsInternalPlayer();
    ~SsInternalPlayer();

    // Non-copyable: holds raw FFI / RID resources.
    SsInternalPlayer(const SsInternalPlayer&) = delete;
    SsInternalPlayer& operator=(const SsInternalPlayer&) = delete;

    // Re-parent _root_ci so this player draws under `p_parent_ci`. Pass an
    // invalid RID to detach (used when the host leaves the tree). Safe to
    // call multiple times; new part canvas items inherit the current parent.
    void setParentCanvasItem(RID p_parent_ci);
    RID getRootCanvasItem() const { return _root_ci; }

    // Replaces the host's `emit_signal` calls. Pass `nullptr` for instance
    // children so their internal events don't reach user-facing signals.
    // The sink is borrowed; the host owns its lifetime.
    void setEventSink(SsPlayerEventSink* p_sink);

    // Resource / animation
    void setSSABResource(const Ref<SSABResource>& p_res);
    Ref<SSABResource> getSSABResource() const { return _ssabRes; }
    void setAnimation(const String& p_name);
    String getAnimation() const { return _strAnimationSelected; }
    // Cached AnimationData* for the currently selected animation. Null until
    // setAnimation+_fetchAnimation succeed; lets callers skip a redundant
    // name->AnimationData lookup in their own binary.
    const ss::format::AnimationData* getCurrentAnimationData() const { return _currentAnimationData; }

    // Playback control — 1:1 mirror of the previous SpriteStudioPlayer2D API.
    bool isPlaying() const;
    void play(float p_start_frame = -1.0f);
    bool isPausing() const;
    void pause();
    void stop();

    void setSpeed(float p_speed);
    float getSpeed() const;

    void setFrame(float p_frame);
    float getFrame() const;

    int getTotalFrames() const;

    void setFrameRate(int p_fps);
    int getFrameRate() const;

    void setAnimationSection(int p_start, int p_end);
    int getAnimationSectionStart() const;
    int getAnimationSectionEnd() const;

    // Direction encoding follows the FFI convention: 0 = Forward,
    // non-zero = Backward. Style: 0 = Normal, 1 = PingPong. Note this is
    // distinct from the Rust PlaybackDirection enum's int values
    // (Forward=1, Backward=-1) — pass 0/1 here.
    void setPlaybackDirection(int p_direction, int p_style);
    int getPlaybackDirection() const;
    int getPlaybackStyle() const;

    void setLoop(int p_count);
    int getLoop() const;

    void setSkipFrames(bool p_skip);
    bool isSkipFrames() const;

    void setSubFrameEnabled(bool p_enabled);
    bool isSubFrameEnabled() const;

    // Marks this player as parent-driven — its own per-tick `update` is a
    // no-op because the parent's `_update_instance_children` seeks the
    // child's frame each tick (synced) or steps it via `ss_runtime_update`
    // (independent). Defaults to false; `_setup_instance_children` flips
    // it to true on every Instance child it spawns.
    void setParentDriven(bool p_enabled);
    bool isParentDriven() const { return _parent_driven; }

    // Per-tick update (in seconds; the host typically passes
    // `process_delta_time`). No-op when paused or in instance-child mode.
    void update(float delta_seconds);

    // Transform / visibility on the root canvas item. The Node2D wrapper
    // never calls these — the Node's own transform handles that. Used by
    // parent SsInternalPlayer when this player is an Instance child.
    void setRootTransform(const Transform2D& p_xf);
    void setRootVisible(bool p_visible);

    // Re-apply current resource after the underlying binary changes on disk
    // (mirrors the previous Resource::changed signal handler).
    void onSSABReloaded();

private:
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    using SsVec2Array = PackedVector2Array;
    using SsColorArray = PackedColorArray;
    using SsIntArray = PackedInt32Array;
#else
    using SsVec2Array = Vector<Vector2>;
    using SsColorArray = Vector<Color>;
    using SsIntArray = Vector<int>;
#endif

    // Root canvas item that all per-batch canvas items hang off. Created in
    // ctor, freed in dtor; transform / visibility / parent on this RID is
    // what makes Node-less hierarchical composition work.
    RID _root_ci;

    Ref<SSABResource> _ssabRes;
    HashMap<uint32_t, Ref<Texture2D>> _textures;
    HashMap<int, Ref<CanvasItemMaterial>> _blend_materials;
    // Per-batch canvas_item pool. Index == draw_batches[i] order. Recyclable
    // across frames; pool grows monotonically to peak batch count, unused
    // entries are hidden rather than freed.
    Vector<RID> _batch_canvas_items;
    LocalVector<const ss::runtime::PartState*> _parts_by_idx;
    String _strAnimationSelected;
    const ss::format::AnimationData* _currentAnimationData = nullptr;
    void* runtime_ctx = nullptr;
    void* runtime_res = nullptr;
    float previous_frame_no = -1.0f;
    float _speed_rate = 1.0f;
    bool _sub_frame_enabled = false;
    bool _parent_driven = false;

    SsPlayerEventSink* _event_sink = nullptr;

    // Per-Instance-part state: child player + transition tracking. Indexed by
    // part_index; non-Instance slots hold a default-constructed entry with
    // `player == nullptr`. Identity of the last applied EventInstance setup
    // is compared against `ss_event_instance_info` returned from ssruntime to
    // detect transitions and re-apply playback config only on the edge.
    struct InstanceChildState {
        SsInternalPlayer* player = nullptr;
        // -1 = uninitialized; otherwise the last applied event_frame. Reset
        // to -1 on `parent_looped` so the same event re-fires on next loop.
        int last_event_frame = -1;
        bool last_is_synthetic = false;
    };
    LocalVector<InstanceChildState> _instance_children;

    struct EffectSlotState {
        void* effect_ctx = nullptr;
        int last_event_frame = -1;
        bool last_is_synthetic = false;
        bool last_independent = false;
        float accumulated_time = 0.0f;
        float last_parent_frame = -1.0f;
        Vector<RID> emitter_cis;
    };
    LocalVector<EffectSlotState> _effect_slots;

    // External SSAB resources auto-loaded from the parent's directory based
    // on `external_instances`. Cleared and rebuilt on every setSSABResource.
    Vector<Ref<SSABResource>> _external_ssabs;
    // Same set as `_external_ssabs`, keyed by SsAnimeBinary.name_hash so
    // hash-based pack resolution is O(1). Built by `_load_external_ssabs`.
    HashMap<uint32_t, Ref<SSABResource>> _external_ssabs_by_pack_hash;

    // Per-frame draw context: SoA pointers and frame-shared bindings fetched
    // once at the top of `drawAnimation`.
    struct DrawFrame {
        RenderingServer* rs;
        const ss::runtime::FrameData* frameData;
        const ss::format::SsAnimeBinary* binary;
        float frame_no;

        const float* world_matrices;         uintptr_t world_matrices_len;
        const float* local_uvs;              uintptr_t local_uvs_len;
        const float* cell_meta;              uintptr_t cell_meta_len;
        const float* local_vertices;         uintptr_t local_vertices_len;
        const float* shape_vertices;         uintptr_t shape_vertices_len;
        const float* shape_box_coords;       uintptr_t shape_box_coords_len;
        const int32_t* shape_vertex_counts;  uintptr_t shape_vertex_counts_len;
    };

    struct ShapeGeometryBuffers {
        int vert_count;          // 3..12, derived from runtime shape_vertex_counts
        SsVec2Array verts;
        SsColorArray colors;
        SsIntArray indices;
    };

    void _reconfigure();
    void _loadTextures(const Ref<SSABResource>& res);
    void _fetchAnimation();
    void _drawAnimation(float frame_no);
    // Per-part-type emit. Normal is consumed by `_emit_normal_batch` directly
    // through the geometry helper, so no `_draw_part_normal` exists.
    void _draw_part_shape(const DrawFrame& f, RID ci, int p_idx, const ss::runtime::PartState* part, const ss::format::PartData* partBinary, const float* draw_m);
    // Instance slot emit: re-parent the child's _root_ci under this slot's
    // batch CI and apply the slot's world matrix as the child's root
    // transform. The child's own draw + simulation already happened earlier
    // in `_update_instance_children` (sim phase), so this is positioning
    // only — no event scanning, no playback config, no frame stepping.
    void _emit_instance_slot(const DrawFrame& f, RID ci, int p_idx, const float* slot_matrix);
    void _emit_effect_slot(const DrawFrame& f, RID ci, int p_idx, const float* slot_matrix);

    int _build_normal(const DrawFrame& f, int p_idx,
                      const ss::runtime::PartState* part,
                      const float* draw_m,
                      const Vector2& tex_size,
                      SsVec2Array& verts,
                      SsVec2Array& uvs,
                      SsColorArray& colors,
                      int vbase);
    bool _build_shape_geometry(const DrawFrame& f, int p_idx,
                               const ss::runtime::PartState* part,
                               const float* draw_m,
                               ShapeGeometryBuffers& out);

    // Per-batch emit helpers. `ci` is the batch's canvas_item from
    // `_batch_canvas_items`; caller has already cleared it and set z_index.
    // For Normal batches multiple parts' geometry is concatenated into a
    // single canvas_item_add_triangle_array call.
    void _emit_normal_batch(const DrawFrame& f, RID ci,
                            const ss::runtime::DrawBatch* batch,
                            const uint16_t* draw_order_data);
    void _emit_shape_singleton(const DrawFrame& f, RID ci, int p_idx,
                               const ss::runtime::PartState* part);

    // Pool helpers
    RID _ensure_batch_ci(int batch_idx);

    void _setup_instance_children();
    void _clear_instance_children();
    void _setup_effect_slots();
    void _clear_effect_slots();
    // Per-tick instance child driver. Runs in the sim phase (called from
    // `update`), before `_drawAnimation`. Per slot, queries
    // `ss_runtime_get_active_event_instance` (returns SS6 default semantics
    // when no event is active), then delegates to `_drive_instance_slot`.
    // `_emit_instance_slot` later positions the already-drawn child under
    // the slot's batch CI.
    //
    // `delta_seconds` is the wall-clock tick this update represents and is
    // forwarded to children whose EventInstance has `independent = true`
    // (those children run their own controller via `ss_runtime_update`
    // rather than being seeked deterministically from `parent_frame_no`).
    // Pass 0 from non-tick callers (`setFrame`, `setSubFrameEnabled`,
    // `_fetchAnimation`) — synced children still update via parent-frame
    // seek; independent children stay where they are.
    //
    // `parent_looped` re-arms transition detection. When the parent's
    // controller wraps (`ss_runtime_is_looped` true) the same EventInstance
    // identity matches a child's stored `last_event_frame`, so without this
    // flag the loop edge would be invisible and finite-loop independent
    // children would stay frozen at their previous end_frame.
    void _update_instance_children(float parent_frame_no, float delta_seconds, bool parent_looped);

    // Per-slot driver: detects the transition edge (info identity changed
    // vs. `state.last_*`), resolves label hashes against the *child's*
    // runtime context (so dynamic-swap of the child's ssab keeps working),
    // applies playback config + `play()` only on transition, then steps the
    // child synced or independent.
    void _drive_instance_slot(InstanceChildState& state,
                              SsInternalPlayer* child,
                              const ss_event_instance_info& info,
                              float parent_frame_no,
                              float delta_seconds);

    // Common: clamp `frame_no` to integer (unless sub-frame mode), redraw
    // only if changed since the last frame.
    static void _redraw_child_if_frame_changed(SsInternalPlayer* child, float frame_no);

    // Apply the current `frame_no` to this player's draw state in one shot:
    // resolve `draw_frame` per `_sub_frame_enabled`, store it as
    // `previous_frame_no`, drive Instance children for that frame, and
    // redraw. Used by `setFrame`, `setSubFrameEnabled`, `_fetchAnimation`
    // (delta=0, parent_looped=false — non-tick callers don't step
    // independent children) and the per-tick `update` path.
    void _seek_and_redraw(float frame_no, float delta_seconds, bool parent_looped);

    void _load_external_ssabs();
    String _resolve_animation_by_hash(uint32_t name_hash, Ref<SSABResource>& out_source) const;
    void _apply_blend_material(RenderingServer* rs, RID ci, ss::format::BlendType blend_type);

    void _clear_batch_canvas_items();
};
