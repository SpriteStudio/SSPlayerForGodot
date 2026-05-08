#pragma once

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/canvas_item_material.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/templates/hash_map.hpp>
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
#include "core/templates/vector.h"
#include "core/variant/dictionary.h"
#include "scene/resources/canvas_item_material.h"
#include "scene/resources/texture.h"
#include "servers/rendering/rendering_server.h"
#endif

#include "ssab_resource.h"

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
    virtual void onUserData(int flag, int int_value, const Rect2& rect_value, const Vector2& point_value, const String& string_value) {}
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
    void setFrameRelative(float p_diff);
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
    // no-op (the parent calls `setFrameRelative` every frame). Also disables
    // recursive instance setup so a misauthored cycle in the SSAB doesn't
    // self-reference. Defaults to false.
    void setInstanceChildMode(bool p_enabled);
    bool isInstanceChildMode() const { return _instance_child_mode; }

    // Per-tick advance (in seconds; the host typically passes
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
    String _strAnimationSelected;
    const ss::format::AnimationData* _currentAnimationData = nullptr;
    void* runtime_ctx = nullptr;
    void* runtime_res = nullptr;
    float previous_frame_no = -1.0f;
    float _speed_rate = 1.0f;
    bool _sub_frame_enabled = false;
    bool _instance_child_mode = false;

    SsPlayerEventSink* _event_sink = nullptr;

    // One child SsInternalPlayer per Instance part of this player's
    // animation, indexed by part_index. Slots for non-Instance parts hold
    // nullptr. Owned: each pointer is `memdelete`d in `_clear_instance_players`.
    Vector<SsInternalPlayer*> _instance_players;
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

    // Per-part geometry buffers in world space, concatenated across the
    // parts in a Normal batch into a single `canvas_item_add_triangle_array`
    // call. The build helpers below produce these without touching any RID.
    struct NormalBuffers {
        int vert_count;          // 4 (quad) or 5 (pentagon fan w/ center)
        // Texture is resolved by the caller (from `DrawBatch.texture_hash` for
        // batched draws); not stored here.
#ifdef SPRITESTUDIO_GODOT_EXTENSION
        PackedVector2Array verts;
        PackedVector2Array uvs;
        PackedColorArray colors;
#else
        Vector<Vector2> verts;
        Vector<Vector2> uvs;
        Vector<Color> colors;
#endif
    };

    struct ShapeGeometryBuffers {
        int vert_count;          // 3..12, derived from runtime shape_vertex_counts
#ifdef SPRITESTUDIO_GODOT_EXTENSION
        PackedVector2Array verts;
        PackedColorArray colors;
        PackedInt32Array indices;
#else
        Vector<Vector2> verts;
        Vector<Color> colors;
        Vector<int> indices;
#endif
    };

    void _reconfigure();
    void _loadTextures(const Ref<SSABResource>& res);
    void _fetchAnimation();
    void _drawAnimation(float frame_no);
    // Per-part-type emit. Normal is consumed by `_emit_normal_batch` directly
    // through the geometry helper, so no `_draw_part_normal` exists.
    void _draw_part_shape(const DrawFrame& f, RID ci, int p_idx, const ss::runtime::PartState* part, const ss::format::PartData* partBinary, const float* draw_m);
    void _draw_part_instance(const DrawFrame& f, RID ci, int p_idx, const ss::runtime::PartState* part, const ss::format::PartData* partBinary, const float* draw_m);

    // Geometry-build helpers — fill `out` with world-space verts/uvs/colors
    // for a single part. Return false when the part should be skipped (no
    // texture / no cell / out-of-range buffers / etc.). No RID side effects.
    bool _build_normal(const DrawFrame& f, int p_idx,
                                const ss::runtime::PartState* part,
                                const float* draw_m,
                                const Vector2& tex_size,
                                NormalBuffers& out);
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
                            const uint16_t* draw_order_data,
                            const Vector<const ss::runtime::PartState*>& parts_by_idx);
    void _emit_shape_singleton(const DrawFrame& f, RID ci, int p_idx,
                               const ss::runtime::PartState* part);

    // Pool helpers
    RID _ensure_batch_ci(int batch_idx);

    void _setup_instance_players();
    void _clear_instance_players();
    void _load_external_ssabs();
    String _resolve_animation_by_hash(uint32_t name_hash, Ref<SSABResource>& out_source) const;
    void _apply_blend_material(RenderingServer* rs, RID ci, ss::format::BlendType blend_type);

    void _clear_batch_canvas_items();
};
