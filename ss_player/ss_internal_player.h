#pragma once

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/canvas_item_material.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
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
#include "core/templates/hash_set.h"
#include "core/templates/local_vector.h"
#include "core/templates/vector.h"
#include "core/variant/dictionary.h"
#include "scene/resources/canvas_item_material.h"
#include "scene/resources/mesh.h"
#include "scene/resources/shader.h"
#include "scene/resources/material.h"
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

// One pooled mask-coverage render target (viewport + canvas + canvas-item pool)
// of a fixed size class, shared across players via a process-global pool. Full
// definition lives in the .cpp; players only hold an opaque handle.
struct SsMaskCoverageTarget;

// Receiver for animation events that need to bubble up to engine-level
// signals (GDScript signals, Unity events, etc.). The host (Node2D wrapper,
// editor preview, etc.) implements this. SsInternalPlayer never assumes a
// sink is present — instance-child players run with `nullptr` so their
// events stay internal.
class SsPlayerEventSink {
public:
    virtual ~SsPlayerEventSink() = default;
    virtual void onAnimationStarted(const String& anim_name) {}
    virtual void onAnimationChanged(const String& anim_name) {}
    virtual void onAnimationFinished(const String& anim_name) {}
    virtual void onAnimationLooped(const String& anim_name) {}
    // `payload` / `info` carry the event's origin — `part_index`, `part_name`
    // and `frame_no` — alongside the authored values. Signals keep theirs in a
    // separate `info` dictionary because `value` is keyed by author-defined
    // parameter ids, which a fixed key could collide with.
    virtual void onUserData(const Dictionary& payload) {}
    virtual void onSignal(const String& command, const Dictionary& value, const Dictionary& info) {}
    virtual void onAudio(const Dictionary& payload) {}
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
    void setAnimationByHash(uint32_t p_hash);
    String getAnimation() const { return _strAnimationSelected; }
    // Cached AnimationData* for the currently selected animation. Null until
    // setAnimation+_fetchAnimation succeed; lets callers skip a redundant
    // name->AnimationData lookup in their own binary.
    const ss::format::AnimationData* getCurrentAnimationData() const { return _currentAnimationData; }

    // Playback control — 1:1 mirror of the previous SpriteStudioPlayer2D API.
    bool isPlaying() const;
    // True when the effective playback direction is forward (speed > 0 and not
    // reversed). Audio playback is gated on this: the runtime fires audio events
    // on reverse playback too, but reverse audio is a documented limitation.
    bool isPlayingForward() const;
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

    // Effective local-unit -> on-screen-pixel scale of the owning Node2D
    // (global transform composed with the viewport/camera transform). The
    // mask coverage pass uses it to size the coverage target to the mask's
    // actual on-screen footprint instead of a fixed maximum. Set by the Node2D
    // wrapper each frame before update(); 1.0 until then.
    void setCoverageScreenScale(float p_scale) { _coverage_screen_scale = (p_scale > 0.0f) ? p_scale : 1.0f; }

    // Viewport the owning Node2D renders into. The mask coverage target is
    // parented to it so the server renders coverage before this player's parts
    // sample it, in the same frame. Set by the Node2D wrapper on tree changes
    // (an invalid RID when outside the tree, where no coverage pass runs); a
    // target already borrowed is re-parented on the spot.
    void setHostViewport(RID p_viewport);

    void setCellMapOverrideTexture(uint32_t cellmap_name_hash, const Ref<Texture2D>& texture);
    Ref<Texture2D> getCellMapTexture(uint32_t cellmap_name_hash) const;

    // Re-apply current resource after the underlying binary changes on disk
    // (mirrors the previous Resource::changed signal handler).
    void onSSABReloaded();

    // ---- Part query API (for follow / attachment nodes) -------------------
    // Read-only access to the current frame's part poses. The player never
    // owns or drives external nodes; these just expose what it already
    // computes so a SpriteStudioPartAttachment2D can mirror a part.
    //
    // Resolve a part name to its index in the current binary; -1 if unknown.
    int resolve_part_index(const String& p_name) const;
    // Player-local Transform2D of the part for the current frame (the same
    // matrix the part draws with, via matrix_to_transform2d of its world
    // matrix). False if the index is out of range or no frame data exists.
    bool try_get_part_local_transform(int p_part_index, Transform2D& r_xf) const;
    // Per-part hide flag for the current frame. False (with r_hidden=false)
    // when the index is out of range.
    bool try_get_part_hidden(int p_part_index, bool& r_hidden) const;
    // Number of parts in the current binary, and the name of part i ("" if out
    // of range). Used to populate the editor part-name dropdown.
    int get_part_count() const;
    String get_part_name(int p_part_index) const;

    // ---- Override Layer (Phase 2): per-part runtime overrides ------------
    // Thin wrappers over the ssruntime override FFI. Each is a no-op returning
    // false when the part index is invalid or no runtime context is bound.
    // priority: 0=on-next-keyframe (NON), 1=until-next-animation, 2=permanent
    // (visibility takes no priority). blend_op: 0=Mix 1=Mul 2=Add 3=Sub.
    bool set_part_visibility_override(int p_part_index, bool p_force_hidden, bool p_cascade);
    bool clear_part_visibility_override(int p_part_index);
    bool set_part_color_override(int p_part_index, const Color& p_color, int p_blend_op, int p_priority);
    // Four-corner (per-vertex) variant of the color override. Corners are named
    // in the runtime's own order: left-top, right-top, left-bottom, right-bottom.
    // Shares the single color-override slot with set_part_color_override, so
    // clear_part_color_override clears either kind.
    bool set_part_color_override_corners(int p_part_index, const Color& p_left_top, const Color& p_right_top,
                                         const Color& p_left_bottom, const Color& p_right_bottom,
                                         int p_blend_op, int p_priority);
    bool clear_part_color_override(int p_part_index);
    bool set_part_cell_override(int p_part_index, const String& p_cellmap_name, const String& p_cell_name, int p_priority);
    bool clear_part_cell_override(int p_part_index);
    bool clear_all_part_overrides();

private:
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    using SsVec2Array = PackedVector2Array;
    using SsColorArray = PackedColorArray;
    using SsIntArray = PackedInt32Array;
    using SsFloatArray = PackedFloat32Array;
#else
    using SsVec2Array = Vector<Vector2>;
    using SsColorArray = Vector<Color>;
    using SsIntArray = Vector<int>;
    using SsFloatArray = Vector<float>;
#endif

    // Root canvas item that all per-batch canvas items hang off. Created in
    // ctor, freed in dtor; transform / visibility / parent on this RID is
    // what makes Node-less hierarchical composition work.
    RID _root_ci;

    Ref<SSABResource> _ssabRes;
    HashMap<uint32_t, Ref<Texture2D>> _textures;
    // Materials used by Shape and Effect batches; no PartColor mediation, the
    // CanvasItem blend_mode does the work.
    HashMap<int, Ref<CanvasItemMaterial>> _blend_materials;
    // Shared ShaderMaterial cache for shader variants whose catalog entry has
    // `is_per_part=false` (Default and any future shareable variants). Key is
    // a composite of (shader_id_hash << 32 | blend_type) so the same material
    // is reused across parts with matching shader+blend. Per-part variants
    // are not cached here — they come from a per-shader_id pool (TBD).
    HashMap<uint64_t, Ref<ShaderMaterial>> _partcolor_materials;
    // Source Shader resources keyed by the same (shader_id_hash, blend_type)
    // composite. Shaders themselves are always shareable (the per-part
    // distinction lives on the material, not the underlying shader code).
    HashMap<uint64_t, Ref<Shader>> _partcolor_shaders;

    // Per-part ShaderMaterial pool for variants whose catalog entry has
    // `is_per_part=true`. Godot binds material state per-material (not
    // per-draw), so parts whose uniform values differ each need their own
    // ShaderMaterial instance. Each pool entry is keyed by (shader_id_hash,
    // blend_type) so materials pre-bound to the right Shader RID can be
    // reused. Within a pool the `in_use` count walks forward each frame
    // (acquire → set uniforms → bind) and is reset to 0 by
    // `_reset_per_part_pools()` at the next frame's start.
    struct PerPartMaterialPool {
        Vector<Ref<ShaderMaterial>> materials;
        int in_use = 0;
    };
    HashMap<uint64_t, PerPartMaterialPool> _per_part_material_pools;

    // Per-part canvas_item pool. Parts that use a per-part material need
    // their own canvas_item too (the material's uniform state otherwise leaks
    // across draws in the same frame). RIDs survive across frames; at frame
    // start every entry is cleared+hidden by `_reset_per_part_pools()`, then
    // `_acquire_per_part_canvas_item()` re-shows entries as they are
    // re-acquired. RIDs are freed in the destructor.
    Vector<RID> _per_part_canvas_items;
    int _per_part_canvas_items_in_use = 0;
    // Monotonic canvas-item draw-order counter, reset each frame and bumped for
    // every batch CI and per-part CI as they are emitted (in rank order). Keeps
    // per-part (masked) items correctly ordered against batch items and each
    // other — without it the per-part pool's allocation order leaks into the
    // overdraw and a writer's own colour can land on top of its masked targets.
    int _draw_seq = 0;
    // Single-part scratch buffers used by the per-part Normal emit path.
    // Pre-sized to the per-part maximum (5 verts, 12 indices). Reused across
    // per-part emits in a frame so we don't reallocate per part.
    SsVec2Array  _per_part_normal_verts;
    SsVec2Array  _per_part_normal_uvs;
    SsColorArray _per_part_normal_colors;
    SsFloatArray _per_part_normal_custom0;
    SsIntArray   _per_part_normal_indices;

    // Resolved SS Shader-attribute data for one part. Populated each emit by
    // `_resolve_part_shader_info()`. `is_per_part` mirrors the catalog flag
    // so callers can branch between the shared-material batch path and the
    // per-part-material path without duplicating the lookup.
    struct PartShaderInfo {
        uint32_t id_hash;
        bool is_per_part;
        float params[8];
        Ref<Texture2D> map0;
        Ref<Texture2D> map1;
    };
    // Low-level mesh RID pool. Reused across frames to avoid the overhead of
    // instantiating Ref<ArrayMesh> resources. Allocated RIDs are freed in the destructor.
    Vector<RID> _mesh_pool;
    int _mesh_pool_in_use = 0;
    // Reused scratch for _emit_partcolor_mesh so the per-part/per-frame draw
    // path does not heap-allocate a fresh surface Array (plus the empty
    // blend-shape / LOD arguments) on every call. The element slots are cleared
    // after each surface build so they never pin the caller's scratch buffers
    // via copy-on-write.
    Array _surface_arrays;
    Array _surface_empty_blend_shapes;
    Dictionary _surface_empty_lods;
    // Per-batch canvas_item pool. Index == draw_batches[i] order. Recyclable
    // across frames; pool grows monotonically to peak batch count, unused
    // entries are hidden rather than freed.
    Vector<RID> _batch_canvas_items;
    int _batch_canvas_items_in_use = 0;
    LocalVector<const ss::runtime::PartState*> _parts_by_idx;
    // Part name -> index map + ordered names, rebuilt from the binary's
    // PartData by `_rebuild_part_index_map` on setSSABResource. Backs the part
    // query API (follow / attachment nodes). Indices match the part_index
    // space the world matrices and `_parts_by_idx` use.
    HashMap<String, int> _part_name_to_index;
    LocalVector<String> _part_names;
    // Per-part hide flag for the current frame, indexed by part_index. Rebuilt
    // each _drawAnimation; 0 (visible) for parts absent from the frame.
    LocalVector<uint8_t> _part_hidden;
    void _rebuild_part_index_map();
    String _strAnimationSelected;
    uint32_t _animationSelectedHash = 0;
    const ss::format::AnimationData* _currentAnimationData = nullptr;
    void* runtime_ctx = nullptr;
    void* runtime_res = nullptr;
    // Set whenever the assigned SSAB (or its buffer) changes, cleared once
    // `_fetchAnimation` has re-borrowed and re-bound it. An animation change
    // within the same SSAB must NOT re-bind: binding drops every part override,
    // including the Permanent-priority ones documented to survive it.
    bool _res_rebind_pending = true;
    // `_ssabRes` generation at the time `runtime_res` borrowed its buffer. A
    // mismatch means the resource reloaded in place and the borrow (plus
    // `_currentAnimationData`, which points into it) dangles.
    uint32_t _borrowed_generation = 0;
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
        // Owns transition detection, child-ctx playback config, and frame
        // stepping — all delegated to ssruntime InstanceSlot via
        // `ss_instance_slot_step`.
        void* instance_slot = nullptr;
    };
    LocalVector<InstanceChildState> _instance_children;

    struct EffectSlotState {
        // Owns the entire per-slot effect lifecycle: transition detection,
        // accumulator, dead-effect skip, simulator update, emitter resource
        // resolution (cellmap/cell/UV), particle quad emission, and the
        // resulting EffectDrawPlan FlatBuffer. Godot side keeps only the
        // canvas_item pool — texture lookup happens at draw time via
        // `cellmap_hash` returned per draw command.
        void* effect_slot = nullptr;
        Vector<RID> emitter_cis;
    };
    LocalVector<EffectSlotState> _effect_slots;

    // External SSAB resources auto-loaded from the parent's directory based
    // on `external_instances`. Cleared and rebuilt on every setSSABResource.
    Vector<Ref<SSABResource>> _external_ssabs;
    HashMap<uint32_t, Ref<SSABResource>> _external_ssabs_by_pack_hash;

    // Per-frame draw context: SoA pointers and frame-shared bindings fetched
    // once at the top of `drawAnimation`.
    struct DrawFrame {
        RenderingServer* rs;
        const ss::runtime::FrameData* frameData;
        const ss::format::SsAnimeBinary* binary;
        float frame_no;
        float delta_seconds = 0.0f;
        bool parent_looped = false;

        const float* world_matrices;         uintptr_t world_matrices_len;
        const float* local_uvs;              uintptr_t local_uvs_len;
        const float* cell_meta;              uintptr_t cell_meta_len;
        const float* local_vertices;         uintptr_t local_vertices_len;
        const float* shape_vertices;         uintptr_t shape_vertices_len;
        const float* shape_box_coords;       uintptr_t shape_box_coords_len;
        const int32_t* shape_vertex_counts;  uintptr_t shape_vertex_counts_len;

        // Skinned mesh SoA buffers. Vertex positions are world-space (already
        // skinned); UVs are texture-pixel space; indices are part-local. The
        // CSR offset arrays (length num_parts + 1) slice the per-part ranges.
        const float* mesh_vertices_x;        uintptr_t mesh_vertices_x_len;
        const float* mesh_vertices_y;        uintptr_t mesh_vertices_y_len;
        const uint32_t* mesh_vertex_offsets; uintptr_t mesh_vertex_offsets_len;
        const float* mesh_uvs;               uintptr_t mesh_uvs_len;
        const int32_t* mesh_indices;         uintptr_t mesh_indices_len;
        const uint32_t* mesh_index_offsets;  uintptr_t mesh_index_offsets_len;

        inline const float* get_world_matrix(int p_idx) const {
            constexpr int FLOATS_PER_MATRIX = 16;
            if (world_matrices && (uintptr_t)p_idx * FLOATS_PER_MATRIX + FLOATS_PER_MATRIX <= world_matrices_len) {
                return world_matrices + (p_idx * FLOATS_PER_MATRIX);
            }
            return nullptr;
        }

        inline const float* get_cell_meta(int p_idx) const {
            constexpr int STRIDE = 6;
            if (cell_meta && (uintptr_t)p_idx * STRIDE + STRIDE <= cell_meta_len) {
                return cell_meta + (p_idx * STRIDE);
            }
            return nullptr;
        }

        inline const float* get_local_uvs(int p_idx) const {
            constexpr int STRIDE = 10;
            if (local_uvs && (uintptr_t)p_idx * STRIDE + STRIDE <= local_uvs_len) {
                return local_uvs + (p_idx * STRIDE);
            }
            return nullptr;
        }

        inline const float* get_local_vertices(int p_idx) const {
            constexpr int STRIDE = 10;
            if (local_vertices && (uintptr_t)p_idx * STRIDE + STRIDE <= local_vertices_len) {
                return local_vertices + (p_idx * STRIDE);
            }
            return nullptr;
        }

        inline const float* get_shape_vertices(int p_idx) const {
            constexpr int STRIDE = 24;
            if (shape_vertices && (uintptr_t)p_idx * STRIDE + STRIDE <= shape_vertices_len) {
                return shape_vertices + (p_idx * STRIDE);
            }
            return nullptr;
        }

        inline const float* get_shape_box_coords(int p_idx) const {
            constexpr int STRIDE = 24;
            if (shape_box_coords && (uintptr_t)p_idx * STRIDE + STRIDE <= shape_box_coords_len) {
                return shape_box_coords + (p_idx * STRIDE);
            }
            return nullptr;
        }

        inline int32_t get_shape_vertex_count(int p_idx) const {
            if (shape_vertex_counts && (uintptr_t)p_idx < shape_vertex_counts_len) {
                return shape_vertex_counts[p_idx];
            }
            return 0;
        }
    };

    // Shape batches share the shader pipeline with Normal/Mesh. Without a
    // texture, the shader's default sampler returns white, so the SS6 PartColor
    // compositing formula reduces to a meaningful tint over white. The CUSTOM0
    // stream carries the same (rate, blend_idx, pma_flag, reserved) tuple; UVs
    // are zero-filled because there's no texture to sample meaningfully.
    struct ShapeGeometryBuffers {
        int vert_count;          // 3..12, derived from runtime shape_vertex_counts
        SsVec2Array verts;
        SsVec2Array uvs;
        SsColorArray colors;
        SsFloatArray custom0;
        SsIntArray indices;
    };

    // Mesh batches sample a cellmap texture and apply the PartColor formula,
    // so they ride on the shader-based pipeline and carry CUSTOM0 per vertex.
    struct MeshGeometryBuffers {
        int vert_count;          // mesh vertex count, derived from mesh_vertex_offsets
        SsVec2Array verts;
        SsVec2Array uvs;
        SsColorArray colors;
        // 4 floats per vertex: (rate, blend_idx, pma_flag, reserved). See the
        // shader source comment for the role of each component.
        SsFloatArray custom0;
        SsIntArray indices;
    };

    SsVec2Array  _normal_verts;
    SsVec2Array  _normal_uvs;
    SsColorArray _normal_colors;
    // 4 floats per vertex, same layout as MeshGeometryBuffers::custom0.
    SsFloatArray _normal_custom0;
    SsIntArray   _normal_indices;
    SsVec2Array  _effect_verts;
    SsVec2Array  _effect_uvs;
    SsColorArray _effect_colors;
    SsIntArray   _effect_indices;
    ShapeGeometryBuffers _shape_buf;
    MeshGeometryBuffers _mesh_buf;

    // ---- CBP masking (clever bit packing) ----------------------------------
    // One entry per part that writes the mask this frame. Rebuilt by
    // `_build_mask_writers` each frame from draw_order + static PartData.
    // `bit` is the writer's slot in the coverage bitmap (0..MAX_MASK_WRITERS-1).
    // `op_invert` comes from PartData.mask_influence (false=increment / true=
    // invert). `is_clipping` selects the scope direction: a pure Mask part
    // (PartTypeMask) masks parts drawn BEFORE it (draw_rank < its slot); a
    // clipping writer (write_mask, and shape/text/nines *_mask) masks parts
    // drawn AFTER it (draw_rank > its slot), to frame end.
    struct MaskWriter {
        int part_index = -1;
        uint16_t draw_rank = 0;   // slot = position in draw_order
        uint8_t bit = 0;          // coverage-bitmap bit index
        bool op_invert = false;   // mask_influence: false=increment, true=invert
        bool is_clipping = false; // false=Mask part (before), true=clipping (after)
    };
    // Coverage bitmap packs writer bits into RGB only (the alpha channel is the
    // premultiplied-blend coverage accumulator), so cap at 24 writers / frame.
    static constexpr int MAX_MASK_WRITERS = 24;
    Vector<MaskWriter> _mask_writers;
    // Per part index (parallel to `_parts_by_idx`): 1 when the part is a "pure"
    // mask this frame. Rebuilt with `_mask_writers` so the emit paths can test it
    // once per part without walking the writer list.
    LocalVector<uint8_t> _part_pure_mask;
    // Populate `_mask_writers` from this frame's draw_order + static PartData.
    // Returns true if at least one writer is present (i.e., masking is active
    // this frame). Only the top-root player owns the mask state.
    bool _build_mask_writers(const DrawFrame& f);

    // ---- CBP coverage bitmap (offscreen RGBA8 = 32 mask bits) --------------
    // The coverage render target (viewport + canvas + canvas-item pool) is a
    // size-classed resource borrowed from a process-global pool while this
    // player is actively masking, and returned when it stops. The mask writers'
    // geometry is rendered into it by `_render_mask_coverage`; maskable shaders
    // sample the result (P3). Acquired by `_acquire_mask_target`, returned by
    // `_release_mask_target` / `_free_mask_targets`. The viewport renders with
    // UPDATE_ONCE, parented to `_host_viewport` so it is drawn before the parts
    // that sample it.
    SsMaskCoverageTarget* _mask_target = nullptr; // borrowed; null when idle
    RID _host_viewport;                        // viewport the owning node draws into
    bool _mask_pool_registered = false;        // this player counts as a pool user
    Ref<Shader> _mask_write_shader;            // loaded from SS_MASK_WRITE
    Vector<Ref<ShaderMaterial>> _mask_write_materials; // per-instance, pooled
    int _mask_write_materials_in_use = 0;
    // Maps this player's local space (the space batch geometry lives in, world
    // matrices already applied) -> coverage UV [0,1]. Set per frame by the
    // coverage pass and read by maskable shaders. Derived from the same writer
    // bbox the coverage was rasterized with this frame. Identity until masking
    // runs.
    Transform2D _mask_local_to_uv;
    bool _mask_coverage_valid = false;         // true if this frame drew coverage
    // Coverage-bitmap dimension bounds in pixels. The per-axis size is the
    // mask's on-screen footprint, quantized up to a power-of-two size class in
    // [MIN, MAX]; quantizing keeps the viewport size stable frame-to-frame so
    // the render target is not reallocated as the mask animates.
    static constexpr int MASK_COVERAGE_MAX_DIM = 1024;
    static constexpr int MASK_COVERAGE_MIN_DIM = 64;
    // Hysteresis on shrinking the size class: only drop to a smaller class once
    // the footprint is this fraction below the boundary, so a mask hovering on a
    // class boundary does not flip-flop and thrash a transition every frame.
    // Growing stays prompt to avoid under-resolution.
    static constexpr float MASK_SIZE_SHRINK_HYSTERESIS = 0.15f;
    // Local-unit -> screen-pixel scale fed by the Node2D wrapper (see setter).
    float _coverage_screen_scale = 1.0f;
    // Size class to borrow next frame, decided from this frame's footprint. The
    // target is picked one frame ahead so the writer geometry can be rendered in
    // one pass; quantization keeps it stable, so the lag only shows (as a touch
    // of resolution) on the rare frame a mask crosses a class boundary.
    int _mask_next_w = MASK_COVERAGE_MIN_DIM;
    int _mask_next_h = MASK_COVERAGE_MIN_DIM;

    // Borrow a pooled coverage target of the given size class (swapping if the
    // current one differs), registering this player as a pool user on first use.
    void _acquire_mask_target(int w, int h);
    // Return the borrowed target to the pool (kept registered as a user).
    void _release_mask_target();
    void _free_mask_targets();
    RID _acquire_mask_canvas_item();
    Ref<ShaderMaterial> _acquire_mask_write_material();
    // Render `_mask_writers` into the coverage bitmap. Assumes the list is
    // populated and non-empty. Computes the writer bounding box, sizes the
    // viewport, and sets `_mask_local_to_uv` / `_mask_coverage_valid`.
    void _render_mask_coverage(const DrawFrame& f);

    // Frame mask state derived by _render_mask_coverage and consumed when
    // emitting maskable parts (P3). `_mask_meta_array` is one Vec4 per writer:
    // (draw_rank, bit, op_invert, is_clipping). The two slot bounds bracket
    // which ranks can be affected (Mask: rank < max mask slot; clipping: rank >
    // min clip slot) so out-of-scope parts skip the test.
    float _mask_max_mask_slot = -1.0f;
    float _mask_min_clip_slot = -1.0f;
    Array _mask_meta_array;
    RID _mask_coverage_tex;
    bool _part_in_mask_scope(uint16_t rank) const;
    // True if the part is a "pure" mask (PartTypeMask or a shape/text/nines
    // *_mask): it feeds the coverage bitmap but must not draw its own colour.
    // write_mask (clipping) writers are NOT pure — they draw AND mask. Valid for
    // any player with writers this frame, coverage rendered or not (an instance
    // child's mask parts must stay invisible even though the parent owns the
    // coverage pass), so callers must NOT gate it on `_mask_coverage_valid`.
    bool _is_pure_mask_part(int p_idx) const;
    void _apply_mask_uniforms(Ref<ShaderMaterial> mat, uint16_t rank, bool visible_inside);
    void _set_mask_uv_uniform(Ref<ShaderMaterial> mat, const Transform2D& local_to_uv);

    // ---- Instance-hierarchy mask composition (SDK masking guide 2-6) --------
    // A mask reaching an instance part applies to the whole sub-animation, but
    // the calling part's settings do NOT replace the callee's: they compose.
    // `mask_influence` chains with AND, `visible_inside_mask` with OR. The
    // identity is (influence = true, visible_inside = false), so a top-level
    // animation resolves to each part's own flags. `mask_write` stays out of the
    // composition — writing a mask is a local fact, not something descendants
    // inherit.
    struct InheritedMaskContext {
        bool active = false;         // an ancestor's coverage reaches this player
        bool influence = true;       // AND-chain of mask_influence
        bool visible_inside = false; // OR-chain of visible_inside_mask
        bool operator==(const InheritedMaskContext& o) const {
            return active == o.active && influence == o.influence && visible_inside == o.visible_inside;
        }
    };
    InheritedMaskContext _inherited_mask;
    // Set when `_inherited_mask` changed since the last build, so a child whose
    // frame did not advance still rebuilds against the new context.
    bool _inherited_mask_dirty = false;
    void _set_inherited_mask_context(const InheritedMaskContext& ctx);
    // Compose `_inherited_mask` with the part's own flags. Used for the parts of
    // an instance child, and (via the identity) for a top-level player's own.
    InheritedMaskContext _compose_mask_context(const ss::format::PartData* pd) const;
    // True when the part tree holds any mask writer. Static per resource (the
    // part tree is pack-level, so selecting another animation cannot change it),
    // so it is computed once and reset in `setSSABResource`.
    bool _has_mask_capable_parts() const;
    mutable int8_t _static_mask_capable = -1; // -1 unknown, 0 no, 1 yes

    // The resolved per-part outcome for one frame.
    struct PartMaskDecision {
        bool masked = false;         // run the mask test for this part
        bool visible_inside = false; // polarity, after composition
    };
    // Single seam for "is this part masked, and with which polarity" — the emit
    // paths all go through here so composition (and any future caching or
    // shared-material fast path) lives in one place.
    PartMaskDecision _resolve_part_mask(const DrawFrame& f, int p_idx, uint16_t rank) const;
    // Write the decision onto the part's material, and enrol it in the inherited
    // walk when an ancestor owns the coverage.
    void _stamp_part_mask(const Ref<ShaderMaterial>& mat, uint16_t rank, const PartMaskDecision& md);

    // Materials this player handed to parts that an ancestor's mask covers.
    // Filled while building (which is when the composed polarity is known) and
    // consumed by the coverage owner in `_apply_inherited_mask`, which is the
    // only point where this frame's coverage texture exists. Instance children
    // covered by the same ancestor are recorded alongside so one walk from the
    // owner reaches the whole sub-tree.
    Vector<Ref<ShaderMaterial>> _inherited_mask_materials;
    struct InheritedMaskChild {
        SsInternalPlayer* player = nullptr;
        Transform2D slot_xf;
    };
    Vector<InheritedMaskChild> _inherited_mask_children;
    void _apply_inherited_mask(bool active, RID coverage_tex, const Array& meta,
                               int count, const Transform2D& local_to_uv,
                               float rank);

    void _reconfigure();
    void _loadTextures(const Ref<SSABResource>& res);
    void _fetchAnimation();
    void _drawAnimation(float frame_no, float delta_seconds = 0.0f, bool parent_looped = false);
    bool _needs_continuous_update() const;
    // Instance slot emit: re-parent the child's _root_ci under this slot's
    // batch CI and apply the slot's world matrix as the child's root
    // transform. The child's own draw + simulation already happened earlier
    // in `_update_instance_children` (sim phase), so this is positioning
    // only — no event scanning, no playback config, no frame stepping.
    void _emit_instance_slot(const DrawFrame& f, RID ci, int p_idx, const float* slot_matrix, uint16_t rank);
    void _emit_effect_slot(const DrawFrame& f, RID ci, int p_idx, const float* slot_matrix);

    int _build_normal(const DrawFrame& f, int p_idx,
                      const ss::runtime::PartState* part,
                      const float* draw_m,
                      const Vector2& inv_tex_size,
                      Vector2* verts_ptr,
                      Vector2* uvs_ptr,
                      Color* colors_ptr,
                      float* custom0_ptr,
                      int vbase);
    bool _build_shape_geometry(const DrawFrame& f, int p_idx,
                               const ss::runtime::PartState* part,
                               const float* draw_m,
                               ShapeGeometryBuffers& out);
    bool _build_mesh_geometry(const DrawFrame& f, int p_idx,
                              const ss::runtime::PartState* part,
                              const Vector2& inv_tex_size,
                              MeshGeometryBuffers& out);

    // Per-batch emit helpers. `ci` is the batch's canvas_item from
    // `_batch_canvas_items`; caller has already cleared it and set z_index.
    // For Normal batches multiple parts' geometry is concatenated into a
    // single canvas_item_add_triangle_array call.
    void _emit_normal_batch(const DrawFrame& f, RID ci,
                            const ss::runtime::DrawBatch* batch,
                            const uint16_t* draw_order_data);
    void _emit_shape_batch(const DrawFrame& f, RID ci,
                           const ss::runtime::DrawBatch* batch,
                           const uint16_t* draw_order_data);
    // Mesh singleton emit: skinned mesh parts are non-batchable, so each gets
    // its own batch (like Shape). Vertices arrive world-space from the runtime,
    // so no world-matrix multiply is applied here.
    void _emit_mesh_batch(const DrawFrame& f, RID ci,
                          const ss::runtime::DrawBatch* batch,
                          const uint16_t* draw_order_data);

    // Pool helpers
    RID _ensure_batch_ci(int batch_idx);

    void _setup_instance_children();
    void _clear_instance_children();
    void _setup_effect_slots();
    void _clear_effect_slots();

    // Reset every effect/instance slot's "last started key" memory (and recurse
    // into instance children) so an explicit head-out re-fires independent
    // effects/instances from scratch instead of debouncing the first key as the
    // same one the slot last started. Called from `play()`. The reset itself
    // lives in the runtime (`ss_effect_slot_reset` / `ss_instance_slot_reset`);
    // the host only iterates the slots it owns.
    void _reset_slots_playback_state();
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
                              float delta_seconds,
                              bool parent_looped);

    // Common: clamp `frame_no` to integer (unless sub-frame mode), redraw
    // only if changed since the last frame.
    static void _redraw_child_if_frame_changed(SsInternalPlayer* child, float frame_no, float delta_seconds, bool parent_looped);

    // Apply the current `frame_no` to this player's draw state in one shot:
    // resolve `draw_frame` per `_sub_frame_enabled`, store it as
    // `previous_frame_no`, drive Instance children for that frame, and
    // redraw. Used by `setFrame`, `setSubFrameEnabled`, `_fetchAnimation`
    // (delta=0, parent_looped=false — non-tick callers don't step
    // independent children) and the per-tick `update` path.
    void _seek_and_redraw(float frame_no, float delta_seconds, bool parent_looped);

    void _load_external_ssabs();
    Ref<SSABResource> _resolve_ssab_by_hash(uint32_t pack_hash, uint32_t name_hash) const;
    // True when this player, or any Instance child below it, borrows a buffer
    // its resource has since replaced.
    bool _subtree_borrow_stale() const;

    void _apply_blend_material(RenderingServer* rs, RID ci, ss::format::BlendType blend_type);
    // ShaderMaterial variant for Normal and Mesh batches. PartColor is handled
    // in the shader (per-vertex CUSTOM0 carries rate / blend_idx / pma flag).
    // `shader_id_hash` selects the fragment shader variant from SHADER_CATALOG
    // (see ss_internal_player.cpp); parts without an SS Shader attribute pass
    // the Default id_hash so all callers share one dispatch path.
    void _apply_partcolor_material(RenderingServer* rs, RID ci, uint32_t shader_id_hash, ss::format::BlendType blend_type);
    Ref<Shader> _ensure_partcolor_shader(uint32_t shader_id_hash, ss::format::BlendType blend_type);
    // Per-part material pool helpers. See member-doc above for the lifecycle.
    void _reset_per_part_pools();
    void _free_per_part_canvas_items();
    Ref<ShaderMaterial> _acquire_per_part_material(uint32_t shader_id_hash, ss::format::BlendType blend_type);
    RID _acquire_per_part_canvas_item();
    // Set the SS Shader-attribute uniforms (param0..7, map0/map1, cell rect)
    // on a per-part material. `params` must point to 8 floats. Texture refs
    // may be null when the variant does not bind that map. `cell_rect` is
    // (left_u, top_v, right_u, bottom_v) in UV space — used by ss-circle /
    // ss-spot; other variants leave it unused.
    void _apply_per_part_uniforms(Ref<ShaderMaterial> mat, const float params[8],
                                  const Ref<Texture2D>& map0, const Ref<Texture2D>& map1,
                                  const Vector4& cell_rect);
    // Resolve the SS Shader-attribute map reference to a Godot Texture2D.
    // `cellmap_name_hash` is fnv1a of the cellmap name without .ssce — the
    // same hash that keys `_textures` (which is populated by `_loadTextures`
    // from cellmap entries). Returns a null Ref when the hash is 0 (no map
    // bound) or the cellmap is not loaded. `cell_name_hash` is currently
    // unused — the per-cell pixel rect inside the cellmap atlas is left to
    // the shader / future task. See ss_library_fs.glsl uniforms.
    Ref<Texture2D> _resolve_map_texture(uint32_t cellmap_name_hash);
    // Read the part's SS Shader attribute (if any) out of the current
    // frame's PartAttributeShader vector and pair it with the catalog entry.
    // When no attribute is present, `id_hash` defaults to "Default" and
    // `is_per_part` resolves to false (the shared batch path).
    PartShaderInfo _resolve_part_shader_info(const DrawFrame& f, const ss::runtime::PartState* part);
    // Compute the part's cell-rectangle UV bounds for the ss_cell_rect
    // uniform. Returns (left_u, top_v, right_u, bottom_v). Inputs come from
    // `f.cell_meta` (rect_left/top + size_w/h in pixel space) divided by the
    // bound texture's pixel dimensions (`inv_tex_size = 1 / tex_size`).
    // Returns zero when cell_meta is unavailable; ss-circle / ss-spot then
    // degenerate to "nothing inside the rect" which is the safest fallback.
    Vector4 _resolve_cell_rect_uv(const DrawFrame& f, int p_idx, const Vector2& inv_tex_size);
    // Build a low-level mesh from the geometry arrays and attach it to `ci`.
    // The mesh RID is acquired from the pool `_mesh_pool`.
    void _emit_partcolor_mesh(RenderingServer* rs, RID ci,
                              const SsIntArray& indices,
                              const SsVec2Array& verts,
                              const SsColorArray& colors,
                              const SsVec2Array& uvs,
                              const SsFloatArray& custom0,
                              const RID& texture_rid);

    RID _acquire_mesh_rid(RenderingServer* rs);

    void _clear_batch_canvas_items();
};
