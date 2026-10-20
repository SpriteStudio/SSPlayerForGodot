#include "ss_internal_player.h"
#include "format/ssab.h"
#include "format/effect_draw_plan.h"
#include "ssruntime.h"
#include "format/framedata.h"
#include <mutex>
#include <cstring>
#include <cmath>

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#else
#include "core/core_bind.h"
#include "servers/rendering/rendering_server.h"
#endif

namespace {
// Bridges ssruntime's `ss_log!` output into the Godot console. Without this
// the runtime's log callback stays null and FFI-side diagnostics are silent.
void ss_runtime_log_bridge(int level, const char *message) {
    (void)level;
    if (!message) {
        return;
    }
    WARN_PRINT(String("[ssruntime] ") + String::utf8(message));
}

#include "ss_shader_setup.h"

static HashMap<uint32_t, EmbeddedShader> s_shader_catalog_map;
static uint32_t s_default_shader_id_hash = 0;
static std::once_flag s_shader_catalog_init_flag;

// Composite cache key for (shader_id_hash, blend_type). Packed into a u64 so
// HashMap keys stay scalar — Godot's default Hash<uint64_t> just works.
inline uint64_t make_partcolor_cache_key(uint32_t shader_id_hash, ss::format::BlendType blend_type) {
    return ((uint64_t)shader_id_hash << 32) | (uint32_t)(int)blend_type;
}

// The framebuffer blend a part is drawn with — one of the four a canvas item's
// render_mode can express, which is all partcolor_render_mode_str emits.
// Mulalpha and Mul2 draw as Mul; the modes that need the backdrop as a blend
// factor (Screen, Exclusion, Invert, ...) have no render_mode and draw as Mix.
inline ss::format::BlendType gpu_blend_for(ss::format::BlendType blend_type) {
    switch (blend_type) {
        case ss::format::BlendType_Mix:
        case ss::format::BlendType_Add:
        case ss::format::BlendType_Sub:
        case ss::format::BlendType_Mul:
            return blend_type;
        case ss::format::BlendType_Mulalpha:
        case ss::format::BlendType_Mul2:
            return ss::format::BlendType_Mul;
        default:
            return ss::format::BlendType_Mix;
    }
}

// Round a desired pixel extent up to a power-of-two size class in
// [MIN_DIM, MAX_DIM]. Quantizing makes the coverage viewport size change only
// when the mask crosses a class boundary, so the render target is reused (no
// per-frame realloc) across the common case of a continuously-animating mask.
int ss_quantize_coverage_dim(float px, int min_dim, int max_dim) {
    int v = (int)std::ceil(px);
    if (v <= min_dim) return min_dim;
    if (v >= max_dim) return max_dim;
    int c = min_dim;
    while (c < v) c <<= 1;
    return c;
}

// Like ss_quantize_coverage_dim, but sticky: only shrink to a smaller class once
// the footprint is `shrink_hyst` below the class boundary. This keeps a mask that
// hovers on a boundary from flip-flopping (each flip is a coverage transition).
// Growing is prompt (no hysteresis) so the coverage never under-resolves.
int ss_hysteretic_coverage_dim(float px, int current, int min_dim, int max_dim, float shrink_hyst) {
    int desired = ss_quantize_coverage_dim(px, min_dim, max_dim);
    if (desired < current && current > min_dim) {
        // Class `current` covers (current/2, current]; require px comfortably
        // below current/2 before dropping down.
        const float lower_edge = (float)current * 0.5f;
        if (px > lower_edge * (1.0f - shrink_hyst)) {
            desired = current; // stay in the larger class
        }
    }
    return desired;
}
} // namespace

// One pooled coverage render target of a fixed size class. Global scope so the
// header's opaque forward declaration matches.
struct SsMaskCoverageTarget {
    RID viewport;
    RID canvas;                   // tile 0
    // Tiles 1.. of a bitmap holding more writers than one tile's bits, created
    // on first use. Each is its own canvas on the viewport, so it carries its
    // own transform into its region of the texture.
    RID tile_canvases[SS_MASK_MAX_TILES - 1];
    Vector<RID> canvas_items;     // pooled, one per rendered writer
    LocalVector<SsDynamicMesh> meshes; // what each of canvas_items draws, by the same index
    int canvas_items_in_use = 0;
    int w = 0;
    int h = 0;
    RID parent_viewport;          // host viewport this target currently renders under
};

namespace {
// Process-global pool of coverage targets shared by all players. A player
// borrows a target of the size class it needs while masking and returns it when
// it stops, so VRAM tracks peak concurrent masking rather than the number of
// players that ever masked. Targets are freed when the last user (a player that
// ever borrowed) is destroyed. Single-threaded (RenderingServer main thread).
class SsMaskCoveragePool {
public:
    static SsMaskCoveragePool& get() {
        static SsMaskCoveragePool inst;
        return inst;
    }

    void add_user() { _users++; }
    void remove_user(RenderingServer* rs) {
        if (--_users <= 0) {
            _users = 0;
            free_all(rs);
        }
    }

    SsMaskCoverageTarget* acquire(RenderingServer* rs, int w, int h, RID parent_viewport) {
        const uint64_t key = ((uint64_t)(uint32_t)w << 32) | (uint32_t)h;
        Vector<SsMaskCoverageTarget*>* lst = _free.getptr(key);
        if (lst && !lst->is_empty()) {
            SsMaskCoverageTarget* t = (*lst)[lst->size() - 1];
            lst->remove_at(lst->size() - 1);
            _set_parent(rs, t, parent_viewport);
            return t;
        }
        SsMaskCoverageTarget* t = memnew(SsMaskCoverageTarget);
        t->w = w;
        t->h = h;
        t->viewport = rs->viewport_create();
        // UPDATE_ONCE: re-requested each frame the target is rendered. The
        // target renders under the borrowing player's host viewport (see
        // `_set_parent`), so its coverage is available to that player's parts
        // in the same frame.
        rs->viewport_set_update_mode(t->viewport, RS_VIEWPORT_UPDATE_ONCE);
        rs->viewport_set_clear_mode(t->viewport, RS_VIEWPORT_CLEAR_ALWAYS);
        rs->viewport_set_transparent_background(t->viewport, true);
        rs->viewport_set_disable_3d(t->viewport, true);
        rs->viewport_set_size(t->viewport, w, h);
        // Parent before activating: activation is what flags the server's
        // active-viewport order dirty (see `_set_parent`).
        rs->viewport_set_parent_viewport(t->viewport, parent_viewport);
        t->parent_viewport = parent_viewport;
        rs->viewport_set_active(t->viewport, true);
        t->canvas = rs->canvas_create();
        rs->viewport_attach_canvas(t->viewport, t->canvas);
        _all.push_back(t);
        return t;
    }

    // Keep a still-borrowed target's parent link in sync with its borrower's
    // host viewport (the node may have moved to another viewport since acquire).
    void reparent(RenderingServer* rs, SsMaskCoverageTarget* t, RID parent_viewport) {
        if (t) _set_parent(rs, t, parent_viewport);
    }

    void release(RenderingServer* rs, SsMaskCoverageTarget* t) {
        if (!t) {
            return;
        }
        for (int i = 0; i < t->canvas_items.size(); i++) {
            if (t->canvas_items[i].is_valid()) {
                rs->canvas_item_clear(t->canvas_items[i]);
                rs->canvas_item_set_visible(t->canvas_items[i], false);
            }
        }
        t->canvas_items_in_use = 0;
        // Drop the parent link while idle. The server never detaches children
        // when a viewport is freed, and a viewport whose parent RID no longer
        // resolves is dropped from the render order entirely — so an idle
        // target must not keep pointing at a host that may go away.
        _set_parent(rs, t, RID());
        const uint64_t key = ((uint64_t)(uint32_t)t->w << 32) | (uint32_t)t->h;
        _free[key].push_back(t);
    }

private:
    // Re-parent a target's viewport. Targets are shared between players, which
    // may live under different viewports, so this runs on every acquire (and on
    // release, to clear the link).
    //
    // `viewport_set_parent_viewport` only assigns the parent; it does NOT flag
    // the server's sorted active-viewport list dirty. `viewport_set_active`
    // does, so toggle it to force the re-sort that puts this target ahead of
    // its host. Without that, the new parent is ignored until something else
    // dirties the order.
    void _set_parent(RenderingServer* rs, SsMaskCoverageTarget* t, RID parent_viewport) {
        if (t->parent_viewport == parent_viewport) {
            return;
        }
        rs->viewport_set_parent_viewport(t->viewport, parent_viewport);
        t->parent_viewport = parent_viewport;
        rs->viewport_set_active(t->viewport, false);
        rs->viewport_set_active(t->viewport, true);
    }

    void free_all(RenderingServer* rs) {
        for (int i = 0; i < _all.size(); i++) {
            SsMaskCoverageTarget* t = _all[i];
            if (rs) {
                for (int j = 0; j < t->canvas_items.size(); j++) {
                    if (t->canvas_items[j].is_valid()) rs->free_rid(t->canvas_items[j]);
                }
                for (uint32_t j = 0; j < t->meshes.size(); j++) {
                    ss_free_dynamic_mesh(rs, t->meshes[j]);
                }
                if (t->canvas.is_valid()) rs->free_rid(t->canvas);
                for (int j = 0; j < SS_MASK_MAX_TILES - 1; j++) {
                    if (t->tile_canvases[j].is_valid()) rs->free_rid(t->tile_canvases[j]);
                }
                if (t->viewport.is_valid()) rs->free_rid(t->viewport);
            }
            memdelete(t);
        }
        _all.clear();
        _free.clear();
    }

    HashMap<uint64_t, Vector<SsMaskCoverageTarget*>> _free;
    Vector<SsMaskCoverageTarget*> _all;
    int _users = 0;
};
} // namespace

SsInternalPlayer::SsInternalPlayer() {
    // Register once; the runtime stores the callback in a process-global slot.
    static bool log_callback_registered = false;
    if (!log_callback_registered) {
        ss_runtime_set_log_callback(&ss_runtime_log_bridge);
        log_callback_registered = true;
    }
    runtime_ctx = ss_runtime_create();
    _reconfigure();

    std::call_once(s_shader_catalog_init_flag, []() {
        for (const auto& shader : EMBEDDED_SHADERS) {
            uint32_t hash = ss_runtime_hash_string(shader.id_name);
            s_shader_catalog_map[hash] = { shader.id_name, shader.fs_source, shader.is_per_part };
            if (strcmp(shader.id_name, "Default") == 0) {
                s_default_shader_id_hash = hash;
            }
        }
    });

    // _root_ci anchors all per-batch canvas items so the host can move /
    // hide / re-parent the whole player without touching individual batches.
    // Re-parenting happens via setParentCanvasItem; until then it floats
    // free of any canvas (which is fine — invisible until attached).
    RenderingServer* rs = RenderingServer::get_singleton();
    _root_ci = rs->canvas_item_create();
}

SsInternalPlayer::~SsInternalPlayer() {
    _clear_instance_children();
    _clear_effect_slots();
    _clear_batch_canvas_items();
    _free_per_part_canvas_items();
    _free_mask_targets();
    // Release this player's pool-user reference; the last user frees the pool's
    // shared targets (while the RenderingServer is still alive).
    if (_mask_pool_registered) {
        SsMaskCoveragePool::get().remove_user(RenderingServer::get_singleton());
        _mask_pool_registered = false;
    }

    RenderingServer* rs = RenderingServer::get_singleton();
    for (uint32_t i = 0; i < _mesh_pool.size(); i++) {
        ss_free_dynamic_mesh(rs, _mesh_pool[i]);
    }
    _mesh_pool.clear();

    if (_root_ci.is_valid()) {
        rs->free_rid(_root_ci);
        _root_ci = RID();
    }

    if (runtime_ctx != nullptr) {
        ss_runtime_destroy(runtime_ctx);
        runtime_ctx = nullptr;
    }
    _parsed.unref();
}

void SsInternalPlayer::_reconfigure() {
    if (runtime_ctx != nullptr) {
        ss_runtime_set_coordinate_system(runtime_ctx, 1);
    }
}

void SsInternalPlayer::setParentCanvasItem(RID p_parent_ci) {
    RenderingServer* rs = RenderingServer::get_singleton();
    rs->canvas_item_set_parent(_root_ci, p_parent_ci);
}

void SsInternalPlayer::setEventSink(SsPlayerEventSink* p_sink) {
    _event_sink = p_sink;
}

void SsInternalPlayer::_clear_batch_canvas_items() {
    RenderingServer* rs = RenderingServer::get_singleton();
    for (int i = 0; i < _batch_canvas_items.size(); i++) {
        rs->free_rid(_batch_canvas_items[i]);
    }
    _batch_canvas_items.clear();
    _batch_canvas_items_in_use = 0;
    _blend_materials.clear();
}

RID SsInternalPlayer::_ensure_batch_ci(int batch_idx) {
    RenderingServer* rs = RenderingServer::get_singleton();
    while (_batch_canvas_items.size() <= batch_idx) {
        RID ci = rs->canvas_item_create();
        rs->canvas_item_set_parent(ci, _root_ci);
        _apply_canvas_item_defaults(ci);
        _batch_canvas_items.push_back(ci);
    }
    return _batch_canvas_items[batch_idx];
}

namespace {
// Every failure below reports what the caller asked for and what was there
// instead. Required is what the caller
// cannot get another way: which pack, and what they asked it for. Listing the
// names that WOULD have worked is deliberately not here -- `get_animation_names()`
// and the inspector's Animation dropdown both already show them.

// The bound pack, as the user would find it in the FileSystem dock. A pack
// built in memory has no path, so say that rather than print empty quotes.
String ssab_label(const Ref<SSABResource>& p_res) {
    if (p_res.is_null()) {
        return String("<no SSAB assigned>");
    }
    const String path = p_res->get_path();
    return path.is_empty() ? String("<in-memory SSAB>") : path;
}

// What the caller wrote. `setAnimationByHash` is the only way in without a
// name, and there the hash IS what they wrote, so it is the honest thing to
// echo back.
String animation_label(const String& p_name, uint32_t p_hash) {
    return p_name.is_empty() ? vformat("name hash 0x%x", p_hash) : vformat("\"%s\"", p_name);
}

// The part an authored reference sits on. The index is what the loop has; the
// name is what the user can search for in SpriteStudio.
String part_label(const ss::format::PartData* p_part, int p_index) {
    if (p_part && p_part->name()) {
        return vformat("\"%s\"", String::utf8(p_part->name()->c_str()));
    }
    return vformat("#%d", p_index);
}
} // namespace

void SsInternalPlayer::setSSABResource(const Ref<SSABResource>& ssabRes) {
    _ssabRes = ssabRes;
    _strAnimationSelected = "";
    _animationSelectedHash = 0;
    // A different resource — or the same one reloaded from disk, which may have
    // moved its buffer — invalidates the borrow the runtime holds.
    _res_rebind_pending = true;
    // Derived from the new resource's part tree on next use.
    _static_mask_capable = -1;

    if (!_ssabRes.is_null()) {
        if (!_ssabRes->is_valid()) {
            ERR_PRINT(vformat("[SS] %s is not a usable SSAB: it carries no parts or no animations. Re-convert the .sspj it came from.", ssab_label(_ssabRes)));
            _ssabRes = Ref<SSABResource>();
        } else {
            auto vecAnimeName = _ssabRes->get_animation_names();
            if (vecAnimeName.size() > 0) {
                _strAnimationSelected = vecAnimeName[0];
            }
            _loadTextures(_ssabRes);
        }
    }

    // Resolve external instance dependencies before _setup_instance_children
    // (called from _fetchAnimation below) needs to find ref animations that
    // live in sibling .ssab files. Done unconditionally so a nested instance
    // child whose own SSAB references its own siblings can resolve them
    // (Godot's ResourceLoader CACHE_MODE_REUSE keeps duplicate loads cheap).
    _load_external_ssabs();

    _fetchAnimation();

    // Build the part name -> index map for the query API. Done after the
    // resource is validated; parts() is animation-independent (it lives on the
    // binary), so one rebuild per resource covers every animation.
    _rebuild_part_index_map();
}

void SsInternalPlayer::_rebuild_part_index_map() {
    _part_name_to_index.clear();
    _part_names.clear();
    _part_skinned.clear();
    if (_ssabRes.is_null()) return;
    const ss::format::SsAnimeBinary* binary = _ssabRes->get_ss_anime_binary();
    if (!binary || !binary->parts()) return;
    auto parts = binary->parts();
    const uint32_t n = parts->size();
    _part_names.resize(n);
    _part_skinned.resize(n);
    for (uint32_t i = 0; i < n; i++) {
        const auto* pd = parts->Get(i);
        String nm = (pd && pd->name()) ? String::utf8(pd->name()->c_str()) : String();
        _part_names[i] = nm;
        // A mesh part is "skinned" when its binding carries influence bones; a
        // rigid (deform-only) mesh has none and follows its node transform
        // faithfully. Same test every sibling player uses (PartMeshBinding
        // influence_bone length > 0), so the flag agrees across engines.
        const auto* mb = pd ? pd->mesh_binding() : nullptr;
        const auto* bones = mb ? mb->influence_bone() : nullptr;
        _part_skinned[i] = (bones && bones->size() > 0) ? 1 : 0;
        // First occurrence wins; duplicate part names are not expected within a
        // single binary, but guard against clobbering a lower index just in case.
        if (!nm.is_empty() && !_part_name_to_index.has(nm)) {
            _part_name_to_index[nm] = (int)i;
        }
    }
}

int SsInternalPlayer::resolve_part_index(const String& p_name) const {
    const int* idx = _part_name_to_index.getptr(p_name);
    return idx ? *idx : -1;
}

bool SsInternalPlayer::try_get_part_local_transform(int p_part_index, Transform2D& r_xf) const {
    if (p_part_index < 0 || runtime_ctx == nullptr) return false;
    const float* wm = nullptr;
    uintptr_t len = 0;
    ss_runtime_get_world_matrices(runtime_ctx, &wm, &len);
    if (!wm) return false;
    constexpr int FLOATS_PER_MATRIX = 16;
    if ((uintptr_t)p_part_index * FLOATS_PER_MATRIX + FLOATS_PER_MATRIX > len) return false;
    const float* m = wm + (p_part_index * FLOATS_PER_MATRIX);
    // Same column-major layout the draw path consumes (see matrix_to_transform2d).
    r_xf = Transform2D(m[0], m[1], m[4], m[5], m[12], m[13]);
    return true;
}

bool SsInternalPlayer::try_get_part_hidden(int p_part_index, bool& r_hidden) const {
    r_hidden = false;
    if (p_part_index < 0 || (uint32_t)p_part_index >= _part_hidden.size()) return false;
    r_hidden = _part_hidden[p_part_index] != 0;
    return true;
}

bool SsInternalPlayer::try_get_part_skinned_mesh(int p_part_index, bool& r_skinned) const {
    r_skinned = false;
    if (p_part_index < 0 || (uint32_t)p_part_index >= _part_skinned.size()) return false;
    r_skinned = _part_skinned[p_part_index] != 0;
    return true;
}

int SsInternalPlayer::get_part_count() const {
    return (int)_part_names.size();
}

String SsInternalPlayer::get_part_name(int p_part_index) const {
    if (p_part_index < 0 || (uint32_t)p_part_index >= _part_names.size()) return String();
    return _part_names[p_part_index];
}

// ---- Override Layer (Phase 2) --------------------------------------------

bool SsInternalPlayer::_override_applied(bool p_ok) {
    if (p_ok) _overrides_dirty = true;
    return p_ok;
}

bool SsInternalPlayer::set_part_visibility_override(int p_part_index, bool p_force_hidden, bool p_cascade) {
    if (p_part_index < 0 || runtime_ctx == nullptr) return false;
    // force: 0 = clear/revert, 1 = force-hide (2 = force-visible is reserved).
    return _override_applied(ss_runtime_set_part_visibility_override(runtime_ctx, (uint32_t)p_part_index, p_force_hidden ? 1 : 0, p_cascade ? 1 : 0));
}

bool SsInternalPlayer::clear_part_visibility_override(int p_part_index) {
    if (p_part_index < 0 || runtime_ctx == nullptr) return false;
    return _override_applied(ss_runtime_clear_part_visibility_override(runtime_ctx, (uint32_t)p_part_index));
}

// Godot Color (gamma/sRGB 0..1 under Compatibility 2D) -> packed 0xRRGGBBAA,
// the same 8-bit sRGB space the runtime does its color math in.
static uint32_t pack_color_rgba(const Color& p_color) {
    auto to_u8 = [](float v) -> uint32_t { return (uint32_t)(int)(CLAMP(v, 0.0f, 1.0f) * 255.0f + 0.5f); };
    return (to_u8(p_color.r) << 24) | (to_u8(p_color.g) << 16) | (to_u8(p_color.b) << 8) | to_u8(p_color.a);
}

bool SsInternalPlayer::set_part_color_override(int p_part_index, const Color& p_color, int p_blend_op, int p_priority) {
    if (p_part_index < 0 || runtime_ctx == nullptr) return false;
    return _override_applied(ss_runtime_set_part_color_override(runtime_ctx, (uint32_t)p_part_index, (unsigned char)p_blend_op, pack_color_rgba(p_color), (unsigned char)p_priority));
}

bool SsInternalPlayer::set_part_color_override_corners(int p_part_index, const Color& p_left_top, const Color& p_right_top,
                                                       const Color& p_left_bottom, const Color& p_right_bottom,
                                                       int p_blend_op, int p_priority) {
    if (p_part_index < 0 || runtime_ctx == nullptr) return false;
    // ssruntime expects the four corners in `lt, rt, lb, rb` order.
    const uint32_t rgba[4] = {
        pack_color_rgba(p_left_top),
        pack_color_rgba(p_right_top),
        pack_color_rgba(p_left_bottom),
        pack_color_rgba(p_right_bottom),
    };
    return _override_applied(ss_runtime_set_part_color_override_corners(runtime_ctx, (uint32_t)p_part_index, (unsigned char)p_blend_op, rgba, (unsigned char)p_priority));
}

bool SsInternalPlayer::clear_part_color_override(int p_part_index) {
    if (p_part_index < 0 || runtime_ctx == nullptr) return false;
    return _override_applied(ss_runtime_clear_part_color_override(runtime_ctx, (uint32_t)p_part_index));
}

bool SsInternalPlayer::set_part_cell_override(int p_part_index, const String& p_cellmap_name, const String& p_cell_name, int p_priority) {
    if (p_part_index < 0 || runtime_ctx == nullptr) return false;
    // Names are FNV-1a hashed to match the SSAB's own cell identity (cellmap
    // name is without the `.ssce` extension).
    CharString cm = p_cellmap_name.utf8();
    CharString cn = p_cell_name.utf8();
    uint32_t cellmap_hash = ss_runtime_hash_string(cm.get_data());
    uint32_t cell_hash = ss_runtime_hash_string(cn.get_data());
    return _override_applied(ss_runtime_set_part_cell_override(runtime_ctx, (uint32_t)p_part_index, cellmap_hash, cell_hash, (unsigned char)p_priority));
}

bool SsInternalPlayer::clear_part_cell_override(int p_part_index) {
    if (p_part_index < 0 || runtime_ctx == nullptr) return false;
    return _override_applied(ss_runtime_clear_part_cell_override(runtime_ctx, (uint32_t)p_part_index));
}

bool SsInternalPlayer::clear_all_part_overrides() {
    if (runtime_ctx == nullptr) return false;
    return _override_applied(ss_runtime_clear_all_part_overrides(runtime_ctx));
}

// ---- Animation mixing ----------------------------------------------------

bool SsInternalPlayer::_mixing_applied(bool p_ok) {
    if (p_ok) {
        // A stopped player, or one in MANUAL mode, has to show the change the
        // way it shows an override: without stepping the animation.
        _overrides_dirty = true;
    }
    return p_ok;
}

uint32_t SsInternalPlayer::_find_animation_hash(const String& p_name) const {
    if (_ssabRes.is_null()) return 0;
    const ss::format::AnimationData* animation = _ssabRes->find_animation(p_name);
    return animation ? animation->name_hash() : 0;
}

void SsInternalPlayer::_track_zero_switched(const ss::format::AnimationData* p_animation) {
    // The instance children and effect slots stay: they come from the pack's
    // parts, which every animation of it shares, so the running children carry
    // on under the new animation's keys.
    _currentAnimationData = p_animation;
    if (_animationSelectedHash == p_animation->name_hash()) return;
    _animationSelectedHash = p_animation->name_hash();
    _strAnimationSelected = p_animation->name() ? String::utf8(p_animation->name()->c_str()) : String();
    // No animation_started: the incoming animation is not started, it carries
    // on whatever play state track 0 had.
    if (_event_sink) {
        _event_sink->onAnimationChanged(_strAnimationSelected);
    }
}

// Tracks and sources are addressed by u32 in the runtime; a negative number
// from GDScript names none, and is refused here rather than wrapped around.
#define SS_TRACK_OR_FAIL(p_track, r_fail) \
    if (runtime_ctx == nullptr || _currentAnimationData == nullptr || (p_track) < 0) return r_fail
#define SS_SOURCE_OR_FAIL(p_source, r_fail) \
    if (runtime_ctx == nullptr || _currentAnimationData == nullptr || (p_source) < 0 || (p_source) >= (int64_t)SS_RUNTIME_INVALID_HANDLE) return r_fail

bool SsInternalPlayer::setTrackAnimation(int p_track, const String& p_name) {
    SS_TRACK_OR_FAIL(p_track, false);
    const ss::format::AnimationData* animation = _ssabRes->find_animation(p_name);
    if (animation == nullptr) return false;
    if (!_mixing_applied(ss_runtime_set_track_animation_by_hash(runtime_ctx, (uint32_t)p_track, animation->name_hash()))) return false;
    if (p_track == 0) _track_zero_switched(animation);
    return true;
}

int SsInternalPlayer::getTrackCount() const {
    if (runtime_ctx == nullptr || _currentAnimationData == nullptr) return 0;
    return (int)ss_runtime_get_track_count(runtime_ctx);
}

bool SsInternalPlayer::clearTrack(int p_track) {
    SS_TRACK_OR_FAIL(p_track, false);
    return _mixing_applied(ss_runtime_clear_track(runtime_ctx, (uint32_t)p_track));
}

bool SsInternalPlayer::setTrackAlpha(int p_track, float p_alpha) {
    SS_TRACK_OR_FAIL(p_track, false);
    return _mixing_applied(ss_runtime_set_track_alpha(runtime_ctx, (uint32_t)p_track, p_alpha));
}

float SsInternalPlayer::getTrackAlpha(int p_track) const {
    SS_TRACK_OR_FAIL(p_track, 0.0f);
    const float alpha = ss_runtime_get_track_alpha(runtime_ctx, (uint32_t)p_track);
    return std::isnan(alpha) ? 0.0f : alpha;
}

bool SsInternalPlayer::setTrackBlend(int p_track, int p_blend) {
    SS_TRACK_OR_FAIL(p_track, false);
    return _mixing_applied(ss_runtime_set_track_blend(runtime_ctx, (uint32_t)p_track, (uint32_t)p_blend));
}

int SsInternalPlayer::getTrackBlend(int p_track) const {
    SS_TRACK_OR_FAIL(p_track, SS_RUNTIME_TRACK_BLEND_MIX);
    const uint32_t blend = ss_runtime_get_track_blend(runtime_ctx, (uint32_t)p_track);
    return blend == SS_RUNTIME_INVALID_HANDLE ? (int)SS_RUNTIME_TRACK_BLEND_MIX : (int)blend;
}

bool SsInternalPlayer::setTrackSync(int p_track, int p_sync) {
    SS_TRACK_OR_FAIL(p_track, false);
    // Not a change to the pose: it only decides where the next animation laid on
    // the track starts, so it does not mark the stack as in use.
    return ss_runtime_set_track_sync(runtime_ctx, (uint32_t)p_track, (uint32_t)p_sync);
}

int SsInternalPlayer::getTrackSync(int p_track) const {
    SS_TRACK_OR_FAIL(p_track, SS_RUNTIME_TRACK_SYNC_FREE);
    const uint32_t sync = ss_runtime_get_track_sync(runtime_ctx, (uint32_t)p_track);
    return sync == SS_RUNTIME_INVALID_HANDLE ? (int)SS_RUNTIME_TRACK_SYNC_FREE : (int)sync;
}

bool SsInternalPlayer::setTrackFrame(int p_track, float p_frame) {
    SS_TRACK_OR_FAIL(p_track, false);
    return _mixing_applied(ss_runtime_set_track_frame(runtime_ctx, (uint32_t)p_track, p_frame));
}

float SsInternalPlayer::getTrackFrame(int p_track) const {
    SS_TRACK_OR_FAIL(p_track, 0.0f);
    const float frame = ss_runtime_get_track_frame(runtime_ctx, (uint32_t)p_track);
    return std::isnan(frame) ? 0.0f : frame;
}

bool SsInternalPlayer::setTrackSpeed(int p_track, float p_speed) {
    SS_TRACK_OR_FAIL(p_track, false);
    return _mixing_applied(ss_runtime_set_track_speed(runtime_ctx, (uint32_t)p_track, p_speed));
}

// The playback settings below decide how the tracks move, not what they show,
// so they do not mark the stack as in use; a track above 0 that moves already
// holds an animation, which did.

float SsInternalPlayer::getTrackSpeed(int p_track) const {
    SS_TRACK_OR_FAIL(p_track, 0.0f);
    const float speed = ss_runtime_get_track_speed(runtime_ctx, (uint32_t)p_track);
    return std::isnan(speed) ? 0.0f : speed;
}

bool SsInternalPlayer::setTrackFrameRate(int p_track, int p_fps) {
    SS_TRACK_OR_FAIL(p_track, false);
    return ss_runtime_set_track_frame_rate(runtime_ctx, (uint32_t)p_track, p_fps);
}

int SsInternalPlayer::getTrackFrameRate(int p_track) const {
    SS_TRACK_OR_FAIL(p_track, 0);
    const int32_t fps = ss_runtime_get_track_frame_rate(runtime_ctx, (uint32_t)p_track);
    return fps < 0 ? 0 : (int)fps;
}

bool SsInternalPlayer::setTrackSkipFrames(int p_track, bool p_skip) {
    SS_TRACK_OR_FAIL(p_track, false);
    return ss_runtime_set_track_skip_frames(runtime_ctx, (uint32_t)p_track, p_skip);
}

bool SsInternalPlayer::getTrackSkipFrames(int p_track) const {
    SS_TRACK_OR_FAIL(p_track, false);
    return ss_runtime_get_track_skip_frames(runtime_ctx, (uint32_t)p_track);
}

bool SsInternalPlayer::setTrackPlaybackDirection(int p_track, int p_direction, int p_style) {
    SS_TRACK_OR_FAIL(p_track, false);
    return ss_runtime_set_track_playback_direction(runtime_ctx, (uint32_t)p_track, p_direction, p_style);
}

int SsInternalPlayer::getTrackPlaybackDirection(int p_track) const {
    SS_TRACK_OR_FAIL(p_track, 0);
    const int32_t direction = ss_runtime_get_track_playback_direction(runtime_ctx, (uint32_t)p_track);
    return direction < 0 ? 0 : (int)direction;
}

int SsInternalPlayer::getTrackPlaybackStyle(int p_track) const {
    SS_TRACK_OR_FAIL(p_track, 0);
    const int32_t style = ss_runtime_get_track_playback_style(runtime_ctx, (uint32_t)p_track);
    return style < 0 ? 0 : (int)style;
}

bool SsInternalPlayer::setTrackLoopCount(int p_track, int p_count) {
    SS_TRACK_OR_FAIL(p_track, false);
    return ss_runtime_set_track_loop_count(runtime_ctx, (uint32_t)p_track, p_count);
}

int SsInternalPlayer::getTrackLoopCount(int p_track) const {
    // The runtime's 0 already means "no animation": a count reads back positive,
    // or -1 for looping forever.
    SS_TRACK_OR_FAIL(p_track, 0);
    return (int)ss_runtime_get_track_loop_count(runtime_ctx, (uint32_t)p_track);
}

bool SsInternalPlayer::isTrackFinished(int p_track) const {
    SS_TRACK_OR_FAIL(p_track, false);
    return ss_runtime_is_track_finished(runtime_ctx, (uint32_t)p_track);
}

bool SsInternalPlayer::trackJustLooped(int p_track) const {
    SS_TRACK_OR_FAIL(p_track, false);
    return ss_runtime_track_just_looped(runtime_ctx, (uint32_t)p_track);
}

bool SsInternalPlayer::setTrackOutgoingEventThreshold(int p_track, float p_threshold) {
    SS_TRACK_OR_FAIL(p_track, false);
    return ss_runtime_set_track_outgoing_event_threshold(runtime_ctx, (uint32_t)p_track, p_threshold);
}

float SsInternalPlayer::getTrackOutgoingEventThreshold(int p_track) const {
    SS_TRACK_OR_FAIL(p_track, 0.0f);
    const float threshold = ss_runtime_get_track_outgoing_event_threshold(runtime_ctx, (uint32_t)p_track);
    return std::isnan(threshold) ? 0.0f : threshold;
}

bool SsInternalPlayer::crossfade(int p_track, const String& p_name, float p_duration_seconds) {
    SS_TRACK_OR_FAIL(p_track, false);
    const ss::format::AnimationData* animation = _ssabRes->find_animation(p_name);
    if (animation == nullptr) return false;
    // The runtime's fades run in the milliseconds `update` hands it.
    if (!_mixing_applied(ss_runtime_crossfade_by_hash(runtime_ctx, (uint32_t)p_track, animation->name_hash(), p_duration_seconds * 1000.0f))) return false;
    // The incoming animation takes over track 0 when the fade begins, not when
    // it completes.
    if (p_track == 0) _track_zero_switched(animation);
    return true;
}

bool SsInternalPlayer::fadeTrack(int p_track, float p_target_alpha, float p_duration_seconds) {
    SS_TRACK_OR_FAIL(p_track, false);
    return _mixing_applied(ss_runtime_fade_track(runtime_ctx, (uint32_t)p_track, p_target_alpha, p_duration_seconds * 1000.0f));
}

float SsInternalPlayer::getTrackFadeProgress(int p_track) const {
    // -1 rather than 0 for "not fading": 0 is a fade that has just begun.
    SS_TRACK_OR_FAIL(p_track, -1.0f);
    const float progress = ss_runtime_get_track_fade_progress(runtime_ctx, (uint32_t)p_track);
    return std::isnan(progress) ? -1.0f : progress;
}

bool SsInternalPlayer::cancelTrackFade(int p_track) {
    SS_TRACK_OR_FAIL(p_track, false);
    return _mixing_applied(ss_runtime_cancel_track_fade(runtime_ctx, (uint32_t)p_track));
}

int64_t SsInternalPlayer::getTrackMainSource(int p_track) const {
    // -1 rather than 0 for "none": 0 is a valid handle.
    SS_TRACK_OR_FAIL(p_track, -1);
    const uint32_t handle = ss_runtime_get_track_main_source(runtime_ctx, (uint32_t)p_track);
    return handle == SS_RUNTIME_INVALID_HANDLE ? -1 : (int64_t)handle;
}

int64_t SsInternalPlayer::addTrackSource(int p_track, const String& p_name, float p_weight) {
    SS_TRACK_OR_FAIL(p_track, -1);
    const uint32_t hash = _find_animation_hash(p_name);
    if (hash == 0) return -1;
    const uint32_t handle = ss_runtime_add_track_source_by_hash(runtime_ctx, (uint32_t)p_track, hash, p_weight);
    if (!_mixing_applied(handle != SS_RUNTIME_INVALID_HANDLE)) return -1;
    return (int64_t)handle;
}

bool SsInternalPlayer::setSourceWeight(int64_t p_source, float p_weight) {
    SS_SOURCE_OR_FAIL(p_source, false);
    return _mixing_applied(ss_runtime_set_source_weight(runtime_ctx, (uint32_t)p_source, p_weight));
}

float SsInternalPlayer::getSourceWeight(int64_t p_source) const {
    SS_SOURCE_OR_FAIL(p_source, 0.0f);
    const float weight = ss_runtime_get_source_weight(runtime_ctx, (uint32_t)p_source);
    return std::isnan(weight) ? 0.0f : weight;
}

bool SsInternalPlayer::setSourceFrame(int64_t p_source, float p_frame) {
    SS_SOURCE_OR_FAIL(p_source, false);
    return _mixing_applied(ss_runtime_set_source_frame(runtime_ctx, (uint32_t)p_source, p_frame));
}

float SsInternalPlayer::getSourceFrame(int64_t p_source) const {
    SS_SOURCE_OR_FAIL(p_source, 0.0f);
    const float frame = ss_runtime_get_source_frame(runtime_ctx, (uint32_t)p_source);
    return std::isnan(frame) ? 0.0f : frame;
}

bool SsInternalPlayer::setSourceEventThreshold(int64_t p_source, float p_threshold) {
    SS_SOURCE_OR_FAIL(p_source, false);
    // Decides which events are reported, not what is drawn.
    return ss_runtime_set_source_event_threshold(runtime_ctx, (uint32_t)p_source, p_threshold);
}

float SsInternalPlayer::getSourceEventThreshold(int64_t p_source) const {
    SS_SOURCE_OR_FAIL(p_source, 0.0f);
    const float threshold = ss_runtime_get_source_event_threshold(runtime_ctx, (uint32_t)p_source);
    return std::isnan(threshold) ? 0.0f : threshold;
}

#undef SS_TRACK_OR_FAIL
#undef SS_SOURCE_OR_FAIL

void SsInternalPlayer::onSSABReloaded() {
    String prev_anim = _strAnimationSelected;
    Ref<SSABResource> current = _ssabRes;
    setSSABResource(current);

    if (!prev_anim.is_empty() && _ssabRes.is_valid()) {
        auto names = _ssabRes->get_animation_names();
        for (int i = 0; i < names.size(); i++) {
            if (names[i] == prev_anim) {
                setAnimation(prev_anim);
                break;
            }
        }
    }
}

void SsInternalPlayer::setAnimation(const String& strName) {
    if (_strAnimationSelected == strName) {
        return;
    }
    _strAnimationSelected = strName;
    _animationSelectedHash = 0;
    _fetchAnimation();
    if (_event_sink) {
        _event_sink->onAnimationChanged(_strAnimationSelected);
    }
}

void SsInternalPlayer::setAnimationByHash(uint32_t p_hash) {
    if (_animationSelectedHash == p_hash) {
        return;
    }
    _animationSelectedHash = p_hash;
    _strAnimationSelected = "";
    _fetchAnimation();
    if (_event_sink) {
        _event_sink->onAnimationChanged(_strAnimationSelected);
    }
}

bool SsInternalPlayer::isPlaying() const {
    return ss_runtime_is_playing(runtime_ctx);
}

bool SsInternalPlayer::justLooped() const {
    return ss_runtime_just_looped(runtime_ctx);
}

bool SsInternalPlayer::isPlayingForward() const {
    return ss_runtime_is_playing_forward(runtime_ctx);
}

void SsInternalPlayer::play(float p_start_frame) {
    // play() always heads out (rewinds to a start frame); resuming a hold is
    // resume(), not play(). So a fresh play must clear each slot's "last started
    // key" memory, otherwise a re-played animation would debounce an already-
    // fired independent effect/instance as the same key and never re-fire it.
    _reset_slots_playback_state();
    if (p_start_frame >= 0.0f) {
        ss_runtime_play_with_start_frame(runtime_ctx, p_start_frame);
    } else {
        ss_runtime_play(runtime_ctx);
    }
    if (_event_sink) _event_sink->onAnimationStarted(_strAnimationSelected);
}

bool SsInternalPlayer::isPausing() const {
    return ss_runtime_is_pausing(runtime_ctx);
}

bool SsInternalPlayer::isFinished() const {
    return ss_runtime_is_finished(runtime_ctx);
}

void SsInternalPlayer::pause() {
    ss_runtime_pause(runtime_ctx);
}

void SsInternalPlayer::resume() {
    ss_runtime_resume(runtime_ctx);
}

void SsInternalPlayer::stop() {
    ss_runtime_stop(runtime_ctx);
}

void SsInternalPlayer::setSpeed(float p_speed) {
    ss_runtime_set_speed_scale(runtime_ctx, p_speed);
}

float SsInternalPlayer::getSpeed() const {
    // The runtime, not a copy of what was last set: it clamps a zero or negative
    // rate to a stop, so a cache would report the -1.0 that was asked for while
    // playback is actually held at 0.0.
    return ss_runtime_get_speed_scale(runtime_ctx);
}

void SsInternalPlayer::setFrameNo(float p_frame) {
    if (runtime_ctx) {
        ss_runtime_set_frame_no(runtime_ctx, p_frame);
        _seek_and_redraw(ss_runtime_get_frame_no(runtime_ctx), 0.0f, false);
    }
}

float SsInternalPlayer::getFrameNo() const {
    return ss_runtime_get_frame_no(runtime_ctx);
}

int SsInternalPlayer::getTotalFrames() const {
    return ss_runtime_get_end_frame(runtime_ctx) - ss_runtime_get_start_frame(runtime_ctx) + 1;
}

void SsInternalPlayer::setFrameRate(int p_fps) {
    ss_runtime_set_frame_rate(runtime_ctx, p_fps);
}

int SsInternalPlayer::getFrameRate() const {
    return ss_runtime_get_frame_rate(runtime_ctx);
}

void SsInternalPlayer::setAnimationSection(int p_start, int p_end) {
    ss_runtime_set_animation_section(runtime_ctx, p_start, p_end);
}

int SsInternalPlayer::getAnimationSectionStart() const {
    return ss_runtime_get_start_frame(runtime_ctx);
}

int SsInternalPlayer::getAnimationSectionEnd() const {
    return ss_runtime_get_end_frame(runtime_ctx);
}

void SsInternalPlayer::setPlaybackDirection(int p_direction, int p_style) {
    ss_runtime_set_playback_direction(runtime_ctx, p_direction, p_style);
}

int SsInternalPlayer::getPlaybackDirection() const {
    return ss_runtime_get_playback_direction(runtime_ctx);
}

int SsInternalPlayer::getPlaybackStyle() const {
    return ss_runtime_get_playback_style(runtime_ctx);
}

void SsInternalPlayer::setLoop(int p_count) {
    ss_runtime_set_loop_count(runtime_ctx, p_count);
}

int SsInternalPlayer::getLoop() const {
    return ss_runtime_get_loop_count(runtime_ctx);
}

void SsInternalPlayer::setSkipFrames(bool p_skip) {
    ss_runtime_set_skip_frames(runtime_ctx, p_skip);
}

bool SsInternalPlayer::isSkipFrames() const {
    return ss_runtime_get_skip_frames(runtime_ctx);
}

void SsInternalPlayer::setSubFrameEnabled(bool p_enabled) {
    _sub_frame_enabled = p_enabled;
    for (uint32_t i = 0; i < _instance_children.size(); i++) {
        SsInternalPlayer* child = _instance_children[i].player;
        if (child) {
            child->setSubFrameEnabled(p_enabled);
        }
    }
    if (runtime_ctx) {
        _seek_and_redraw(ss_runtime_get_frame_no(runtime_ctx), 0.0f, false);
    }
}

bool SsInternalPlayer::isSubFrameEnabled() const {
    return _sub_frame_enabled;
}

void SsInternalPlayer::setParentDriven(bool p_enabled) {
    _parent_driven = p_enabled;
}

void SsInternalPlayer::setRootTransform(const Transform2D& p_xf) {
    RenderingServer* rs = RenderingServer::get_singleton();
    rs->canvas_item_set_transform(_root_ci, p_xf);
}

void SsInternalPlayer::setRootVisible(bool p_visible) {
    RenderingServer* rs = RenderingServer::get_singleton();
    rs->canvas_item_set_visible(_root_ci, p_visible);
}

void SsInternalPlayer::setRootModulate(const Color& p_modulate) {
    RenderingServer* rs = RenderingServer::get_singleton();
    rs->canvas_item_set_modulate(_root_ci, p_modulate);
}

void SsInternalPlayer::_apply_canvas_item_defaults(RID p_ci) const {
    RenderingServer* rs = RenderingServer::get_singleton();
    rs->canvas_item_set_default_texture_filter(p_ci, (RS_CANVAS_ITEM_TEXTURE_FILTER)_texture_filter);
    rs->canvas_item_set_default_texture_repeat(p_ci, (RS_CANVAS_ITEM_TEXTURE_REPEAT)_texture_repeat);
    rs->canvas_item_set_light_mask(p_ci, _light_mask);
}

void SsInternalPlayer::setCanvasItemDefaults(int p_texture_filter, int p_texture_repeat, uint32_t p_light_mask) {
    if (_texture_filter == p_texture_filter && _texture_repeat == p_texture_repeat && _light_mask == p_light_mask) {
        return;
    }
    _texture_filter = p_texture_filter;
    _texture_repeat = p_texture_repeat;
    _light_mask = p_light_mask;
    // Pooled canvas items keep what was set on them, so the ones that exist are
    // set here and new ones as they are created. The mask coverage items are left
    // alone: they draw into their own viewport, which nothing lights.
    for (int i = 0; i < _batch_canvas_items.size(); i++) {
        _apply_canvas_item_defaults(_batch_canvas_items[i]);
    }
    for (int i = 0; i < _per_part_canvas_items.size(); i++) {
        _apply_canvas_item_defaults(_per_part_canvas_items[i]);
    }
    for (const EffectSlotState& slot : _effect_slots) {
        for (const RID& e_ci : slot.emitter_cis) {
            _apply_canvas_item_defaults(e_ci);
        }
    }
    for (uint32_t i = 0; i < _instance_children.size(); i++) {
        if (_instance_children[i].player) {
            _instance_children[i].player->setCanvasItemDefaults(p_texture_filter, p_texture_repeat, p_light_mask);
        }
    }
}

void SsInternalPlayer::setCellMapOverrideTexture(uint32_t cellmap_name_hash, const Ref<Texture2D>& texture) {
    if (cellmap_name_hash == 0) return;
    if (texture.is_valid()) {
        _textures[cellmap_name_hash] = texture;
    } else {
        if (_ssabRes.is_valid()) {
            auto a = _ssabRes->get_ss_anime_binary();
            if (a->cellmaps() != nullptr) {
                for (int i = 0; i < a->cellmaps()->size(); i++) {
                    auto cellmap = a->cellmaps()->Get(i);
                    if (cellmap->name_hash() == cellmap_name_hash) {
                        String strImage = _ssabRes->get_parent_dir().path_join(String::utf8(cellmap->image_path()->c_str()));
                        Ref<Texture2D> original_tex = SsResourceLoader::get_singleton()->load(strImage);
                        _textures[cellmap_name_hash] = original_tex;
                        return;
                    }
                }
            }
            if (a->external_textures() != nullptr) {
                for (int i = 0; i < a->external_textures()->size(); i++) {
                    auto etexture = a->external_textures()->Get(i);
                    if (etexture->name_hash() == cellmap_name_hash) {
                        String strImage = _ssabRes->get_parent_dir().path_join(String::utf8(etexture->name()->c_str()));
                        Ref<Texture2D> original_tex = SsResourceLoader::get_singleton()->load(strImage);
                        _textures[cellmap_name_hash] = original_tex;
                        return;
                    }
                }
            }
        }
    }
}

Ref<Texture2D> SsInternalPlayer::getCellMapTexture(uint32_t cellmap_name_hash) const {
    if (_textures.has(cellmap_name_hash)) {
        return _textures[cellmap_name_hash];
    }
    return Ref<Texture2D>();
}

void SsInternalPlayer::_loadTextures(const Ref<SSABResource>& ssabRes) {
    auto a = ssabRes->get_ss_anime_binary();
    _textures.clear();
    if (a->cellmaps() != nullptr) {
        for (int i = 0; i < a->cellmaps()->size(); i++) {
            auto cellmap = a->cellmaps()->Get(i);
            String strImage = _ssabRes->get_parent_dir().path_join(String::utf8(cellmap->image_path()->c_str()));
            Ref<Texture2D> texture = SsResourceLoader::get_singleton()->load(strImage);
            _textures[cellmap->name_hash()] = texture;
        }
    }
    if (a->external_textures() != nullptr) {
        for (int i = 0; i < a->external_textures()->size(); i++) {
            auto etexture = a->external_textures()->Get(i);
            String strImage = _ssabRes->get_parent_dir().path_join(String::utf8(etexture->name()->c_str()));
            Ref<Texture2D> texture = SsResourceLoader::get_singleton()->load(strImage);
            _textures[etexture->name_hash()] = texture;
        }
    }
}


namespace {
    constexpr int CORNERS_COUNT = 4;
    constexpr int MAX_VERTICES_COUNT = 5;
    constexpr int INDICES_COUNT_PENTAGON = 12;
    constexpr int INDICES_COUNT_QUAD = 6;

    inline Vector2 xform_raw(const float* m, float x, float y) {
        return Vector2(
            m[0] * x + m[4] * y + m[12],
            m[1] * x + m[5] * y + m[13]
        );
    }

    inline Transform2D matrix_to_transform2d(const float* m) {
        return Transform2D(m[0], m[1], m[4], m[5], m[12], m[13]);
    }
}

bool SsInternalPlayer::_needs_continuous_update() const {
    for (const auto& slot : _effect_slots) {
        if (slot.effect_slot && ss_effect_slot_is_independent(slot.effect_slot)) return true;
    }
    for (const auto& st : _instance_children) {
        if (st.player && st.instance_slot && ss_instance_slot_is_independent(st.instance_slot)) return true;
    }
    return false;
}

void SsInternalPlayer::update(float delta_seconds) {
    // Parent-driven players are stepped by the parent's
    // `_update_instance_children`; they must not run their own controller
    // tick (would race the parent's deterministic seek).
    if (_parent_driven) return;
    // A re-import frees the buffer the runtime borrowed. Checked ahead of the
    // is_playing early-return, since a stopped player still redraws.
    if (_subtree_borrow_stale()) {
        onSSABReloaded();
    }
    // `is_playing` describes track 0, not the character's clock: overlays and
    // fades run on after track 0 has finished, until the clock is stopped. A
    // stopped clock moves nothing, fades included, so it is skipped whatever
    // the stack holds; neither is a stack that has played out ticked. The
    // runtime says whether anything beyond track 0's frame still moves, since
    // it releases tracks and sources on its own as fades complete.
    const bool stack_in_motion = ss_runtime_is_stack_in_motion(runtime_ctx);
    if (!ss_runtime_is_playing(runtime_ctx)
        && !(stack_in_motion && ss_runtime_is_clock_playing(runtime_ctx))) {
        // A stopped or finished player does not reach the draw-dedup at the
        // bottom at all, so an override taken here would wait for the next
        // play(). Redraw the frame it is already holding. (A *paused* player
        // is still "playing" to the runtime and goes the other way, where the
        // dedup is what the override has to defeat.)
        redraw_pending_overrides();
        return;
    }

    // `is_finished` is sticky until play() clears it, so the signal is sent on
    // the tick it turns true, which a stopped track 0 under a running stack
    // would otherwise repeat every tick.
    const bool was_finished = ss_runtime_is_finished(runtime_ctx);
    auto d = delta_seconds * 1000.0f;
    float frame_no = ss_runtime_update(runtime_ctx, d);

    // `just_looped` is a pulse the runtime clears on entry to every update, so
    // reading it right after the tick is what catches it.
    const bool was_looped = ss_runtime_just_looped(runtime_ctx);
    if (was_looped) {
        if (_event_sink) _event_sink->onAnimationLooped(_strAnimationSelected);
    }
    if (!was_finished && ss_runtime_is_finished(runtime_ctx)) {
        if (_event_sink) _event_sink->onAnimationFinished(_strAnimationSelected);
    }

    // Process passed events (user data / signal / audio) BEFORE the draw-dedup
    // early-return below. `ss_runtime_update` reports every frame it stepped
    // through this tick — including the start frame on the first update after
    // play() (the runtime's `needs_evaluation`). Gating event handling on a
    // change of integer draw frame would drop a start-frame event (e.g. an audio
    // key on frame 0) whenever that first tick doesn't advance the integer frame
    // — a timing-dependent loss seen in the editor preview but not a running
    // project. Events must fire whenever the runtime reports them, regardless of
    // whether we also redraw.
    // Each index addresses the event table of the animation of the source that
    // fired it, so the table is looked up per event. Reading them all against
    // the animation set up here reports other events, or none, once a crossfade
    // on track 0 has handed the track to another animation.
    const int event_count = ss_runtime_get_passed_event_count(runtime_ctx);
    const auto* animations = (event_count > 0 && _currentAnimationData && _ssabRes.is_valid())
                             ? _ssabRes->get_ss_anime_binary()->animations() : nullptr;
    if (animations != nullptr) {
        for (int i = 0; i < event_count; i++) {
            const uint32_t source = ss_runtime_get_passed_event_source_handle(runtime_ctx, i);
            const int32_t anim_idx = ss_runtime_get_source_animation_index(runtime_ctx, source);
            if (anim_idx < 0 || (uint32_t)anim_idx >= animations->size()) continue;
            const auto* events = animations->Get((uint32_t)anim_idx)->events();
            if (events == nullptr) continue;
            int event_idx = ss_runtime_get_passed_event_index(runtime_ctx, i);
            // A not-found (-1), or an index past that table, would read past the
            // FlatBuffer.
            if (event_idx < 0 || (uint32_t)event_idx >= events->size()) continue;
            auto events_per_frame = events->Get((uint32_t)event_idx);
            if (!events_per_frame) continue;

            // The frame the keys were authored on, which is not necessarily the
            // frame they were noticed on: one tick can step across several.
            const int frame_no = (int)events_per_frame->frame_index();

            if (auto users = events_per_frame->users()) {
                for (uint32_t j = 0; j < users->size(); j++) {
                    auto user = users->Get(j);
                    if (!user || !user->value()) continue;
                    auto val = user->value();

                    const int part_index = (int)user->part_index();

                    Dictionary payload;
                    payload["part_index"] = part_index;
                    payload["part_name"] = get_part_name(part_index);
                    payload["frame_no"] = frame_no;
                    payload["source"] = (int64_t)source;
                    if (auto i = val->integer()) payload["integer"] = i->value();
                    if (auto r = val->rect())    payload["rect"]    = Rect2(r->left(), r->top(), r->right() - r->left(), r->bottom() - r->top());
                    if (auto p = val->point())   payload["point"]   = Vector2(p->v1(), p->v2());
                    if (auto s = val->str())     payload["string"]  = String::utf8(s->c_str());

                    if (_event_sink) _event_sink->onUserData(payload);
                }
            }

            if (auto signals_vec = events_per_frame->signals()) {
                for (uint32_t j = 0; j < signals_vec->size(); j++) {
                    auto sig_event = signals_vec->Get(j);
                    if (!sig_event || !sig_event->value()) continue;
                    auto val = sig_event->value();
                    if (!val->active() || !val->command_id()) continue;

                    String command = String::utf8(val->command_id()->c_str());
                    const int part_index = (int)sig_event->part_index();

                    // Kept apart from `value_dict`, whose keys are the parameter
                    // ids the animator authored — a fixed key placed in there
                    // could shadow one of them.
                    Dictionary info;
                    info["part_index"] = part_index;
                    info["part_name"] = get_part_name(part_index);
                    info["frame_no"] = frame_no;
                    info["source"] = (int64_t)source;

                    Dictionary value_dict;

                    if (auto params = val->params()) {
                        for (uint32_t k = 0; k < params->size(); k++) {
                            auto param = params->Get(k);
                            if (!param || !param->param_id()) continue;
                            String p_id = String::utf8(param->param_id()->c_str());

                            switch (param->value_type()) {
                                case ss::format::SignalCommandParamValue_Bool: {
                                    if (auto b = param->value_as_Bool()) value_dict[p_id] = b->value();
                                    break;
                                }
                                case ss::format::SignalCommandParamValue_Index: {
                                    if (auto i = param->value_as_Index()) value_dict[p_id] = i->value();
                                    break;
                                }
                                case ss::format::SignalCommandParamValue_Integer: {
                                    if (auto i = param->value_as_Integer()) value_dict[p_id] = i->value();
                                    break;
                                }
                                case ss::format::SignalCommandParamValue_Floating: {
                                    if (auto f = param->value_as_Floating()) value_dict[p_id] = f->value();
                                    break;
                                }
                                case ss::format::SignalCommandParamValue_Text: {
                                    if (auto t = param->value_as_Text()) {
                                        if (t->value()) value_dict[p_id] = String::utf8(t->value()->c_str());
                                    }
                                    break;
                                }
                                default:
                                    break;
                            }
                        }
                    }

                    if (_event_sink) _event_sink->onSignal(command, value_dict, info);
                }
            }

            if (auto audios = events_per_frame->audios()) {
                for (uint32_t j = 0; j < audios->size(); j++) {
                    auto audio_event = audios->Get(j);
                    if (!audio_event || !audio_event->value()) continue;
                    auto val = audio_event->value();

                    const int part_index = (int)audio_event->part_index();

                    Dictionary payload;
                    payload["part_index"] = part_index;
                    payload["part_name"] = get_part_name(part_index);
                    payload["frame_no"] = frame_no;
                    payload["source"] = (int64_t)source;
                    payload["sound_list_name_hash"] = (int64_t)val->sound_list_name_hash();
                    payload["sound_name_hash"] = (int64_t)val->sound_name_hash();
                    if (val->sound_name()) payload["sound_name"] = String::utf8(val->sound_name()->c_str());
                    payload["loop_num"] = val->loop_num();

                    if (_event_sink) _event_sink->onAudio(payload);
                }
            }
        }
    }

    float draw_frame = _sub_frame_enabled ? frame_no : floorf(frame_no);
    // An override changes what the frame draws without changing which frame it
    // is, so it has to defeat the dedup the same way a held-frame effect does.
    // So does a mask coverage waiting on the size class its last draw picked,
    // and a track stack in motion, whose overlays and fades move the pose while
    // track 0's frame stands still — unless a pause holds them all.
    if (previous_frame_no == draw_frame && !_needs_continuous_update() && !_overrides_dirty
        && !_mask_resize_pending
        && !(stack_in_motion && !ss_runtime_is_clock_pausing(runtime_ctx))) return;

    _seek_and_redraw(frame_no, delta_seconds, was_looped);
}

void SsInternalPlayer::redraw_pending_overrides() {
    if ((!_overrides_dirty && !_mask_resize_pending) || runtime_ctx == nullptr) return;
    _seek_and_redraw(ss_runtime_get_frame_no(runtime_ctx), 0.0f, false);
}

bool SsInternalPlayer::_build_mask_writers(const DrawFrame& f) {
    _mask_writers.clear();
    // Clear before any early return: a part that stopped masking this frame must
    // stop being suppressed by the emit paths.
    if (!_part_pure_mask.is_empty()) {
        memset(_part_pure_mask.ptr(), 0, _part_pure_mask.size());
    }
    if (!f.frameData || !f.binary) return false;
    auto parts_meta = f.binary->parts();
    auto draw_order = f.frameData->draw_order();
    if (!parts_meta || !draw_order) return false;

    const int total_meta = (int)parts_meta->size();
    if ((int)_part_pure_mask.size() != total_meta) {
        _part_pure_mask.resize(total_meta);
        if (total_meta > 0) memset(_part_pure_mask.ptr(), 0, total_meta);
    }
    uint8_t* pure_mask_flags = _part_pure_mask.ptr();
    const uint32_t n = draw_order->size();
    for (uint32_t rank = 0; rank < n; rank++) {
        const int p_idx = (int)draw_order->Get(rank);
        if (p_idx < 0 || p_idx >= total_meta) continue;
        const auto* pd = parts_meta->Get(p_idx);
        if (!pd) continue;

        const auto pt = pd->part_type_type();
        const bool is_mask_part = (pt == ss::format::PartType_PartTypeMask);
        // A "pure" mask draws no colour and masks the parts drawn BEFORE it: a
        // Mask part, or a drawing part flagged via the unified draw_as_mask flag.
        // A write_mask (clipping) writer instead draws normally AND masks the
        // parts drawn AFTER it.
        const bool pure_mask = is_mask_part || pd->draw_as_mask();
        const bool writes = pure_mask || pd->mask_write();
        if (!writes) continue;

        // Flag it even when the writer list is already full: a pure mask that does
        // not fit the bitmap still masks nothing, so it must not fall back to
        // drawing its own colour over the scene.
        if (pure_mask) pure_mask_flags[p_idx] = 1;

        // A writer only writes where its per-frame mask is non-zero and it is not
        // hidden. Both live in FrameData, so this is the one
        // half of the classification PartData cannot answer: an authored MASK of 0
        // is how a writer is switched off over a range of frames, and the cutout
        // threshold cannot stand in for it — a shape mask samples no texture, so
        // its coverage is its whole geometry whatever the threshold says. A part
        // with no PartState this frame was not evaluated and writes nothing, which
        // is what `_bake_coverage_geometry` would decide about it anyway.
        // Tested before the bitmap budget so a switched-off writer costs no bit.
        const auto* ps = (p_idx < (int)_parts_by_idx.size()) ? _parts_by_idx[p_idx] : nullptr;
        if (!ps || ps->mask() == 0.0f || ps->hide()) continue;

        // Bitmap holds 24 writers; keep scanning past that so the pure masks that
        // did not fit are still flagged above.
        if ((int)_mask_writers.size() >= MAX_MASK_WRITERS) continue;

        MaskWriter w;
        w.part_index = p_idx;
        w.draw_rank = (uint16_t)rank;
        // The op follows the effective mask_influence: inside a
        // mask_influence==0 instance the writer composes with the caller's masks
        // as a union (counter plane) rather than cancelling against them.
        w.op_invert = pd->mask_influence() && _inherited_mask.influence;
        // Scope: write_mask (clipping) writers mask the parts drawn AFTER them;
        // pure masks (Mask / shape/text/nines mask) mask the parts BEFORE them
        // and draw no colour, regardless of write_mask.
        w.is_clipping = pd->mask_write() && !pure_mask;
        _mask_writers.push_back(w);
    }
    return !_mask_writers.is_empty();
}

void SsInternalPlayer::setHostViewport(RID p_viewport) {
    if (_host_viewport == p_viewport) {
        return;
    }
    _host_viewport = p_viewport;
    // A target borrowed while under the old viewport would keep rendering (and
    // sorting) under it, so move it across immediately. Pooled targets pick the
    // current host up at acquire time.
    if (_mask_target) {
        SsMaskCoveragePool::get().reparent(RenderingServer::get_singleton(), _mask_target, _host_viewport);
    }
}

void SsInternalPlayer::_acquire_mask_target(int w, int h) {
    if (_mask_write_shader.is_null()) {
        _mask_write_shader.instantiate();
        _mask_write_shader->set_code(String(SS_MASK_WRITE_SHADER));
    }
    if (!_mask_pool_registered) {
        SsMaskCoveragePool::get().add_user();
        _mask_pool_registered = true;
    }
    if (_mask_target && _mask_target->w == w && _mask_target->h == h) {
        return; // already holding the right size class
    }
    RenderingServer* rs = RenderingServer::get_singleton();
    if (_mask_target) {
        // Size-class transition. The freshly acquired target is rendered before
        // this player draws (it is parented to the host viewport), so it carries
        // this frame's coverage and the outgoing one can go back right away.
        SsMaskCoveragePool::get().release(rs, _mask_target);
        _mask_target = nullptr;
    }
    _mask_target = SsMaskCoveragePool::get().acquire(rs, w, h, _host_viewport);
}

void SsInternalPlayer::_release_mask_target() {
    RenderingServer* rs = RenderingServer::get_singleton();
    if (_mask_target) {
        SsMaskCoveragePool::get().release(rs, _mask_target);
        _mask_target = nullptr;
    }
    _mask_coverage_valid = false;
}

void SsInternalPlayer::_free_mask_targets() {
    // Return the borrowed target but keep this player registered as a pool user
    // (registration is released only in the dtor). Called on resource teardown.
    _release_mask_target();
    _mask_write_materials.clear();
    _mask_write_materials_in_use = 0;
    _mask_write_shader = Ref<Shader>();
    _mask_coverage_valid = false;
}

RID SsInternalPlayer::_mask_tile_canvas(int tile) {
    SsMaskCoverageTarget* t = _mask_target;
    if (tile <= 0) return t->canvas;
    RID& c = t->tile_canvases[tile - 1];
    if (!c.is_valid()) {
        RenderingServer* rs = RenderingServer::get_singleton();
        c = rs->canvas_create();
        rs->viewport_attach_canvas(t->viewport, c);
    }
    return c;
}

RID SsInternalPlayer::_acquire_mask_canvas_item(int tile, SsDynamicMesh*& r_mesh) {
    RenderingServer* rs = RenderingServer::get_singleton();
    SsMaskCoverageTarget* t = _mask_target;
    if (t->canvas_items_in_use >= t->canvas_items.size()) {
        t->canvas_items.push_back(rs->canvas_item_create());
        t->meshes.push_back(SsDynamicMesh());
    }
    r_mesh = &t->meshes[t->canvas_items_in_use];
    RID ci = t->canvas_items[t->canvas_items_in_use++];
    // Pooled items move between tiles as the writer set changes.
    rs->canvas_item_set_parent(ci, _mask_tile_canvas(tile));
    rs->canvas_item_clear(ci);
    rs->canvas_item_set_visible(ci, true);
    return ci;
}

Ref<ShaderMaterial> SsInternalPlayer::_acquire_mask_write_material() {
    if (_mask_write_materials_in_use >= _mask_write_materials.size()) {
        Ref<ShaderMaterial> mat; mat.instantiate();
        mat->set_shader(_mask_write_shader);
        _mask_write_materials.push_back(mat);
    }
    return _mask_write_materials[_mask_write_materials_in_use++];
}

void SsInternalPlayer::_render_mask_coverage(const DrawFrame& f) {
    _mask_coverage_valid = false;
    _mask_resize_pending = false;
    _mask_tree_spans.clear();
    _mask_meta_array.clear();
    if (!f.frameData) return;
    // Borrow a pooled target of the size class decided last frame (stable thanks
    // to quantization), then re-request its one-shot render this frame.
    _acquire_mask_target(_mask_next_w, _mask_next_h);
    RenderingServer* rs = f.rs;
    rs->viewport_set_update_mode(_mask_target->viewport, RS_VIEWPORT_UPDATE_ONCE);

    // Recycle the target's CI pool + this player's material pool (hide all,
    // cursors back to 0).
    for (int i = 0; i < _mask_target->canvas_items.size(); i++) {
        rs->canvas_item_clear(_mask_target->canvas_items[i]);
        rs->canvas_item_set_visible(_mask_target->canvas_items[i], false);
    }
    _mask_target->canvas_items_in_use = 0;
    _mask_write_materials_in_use = 0;

    bool have_bbox = false;
    Vector2 bmin, bmax;

    // Every writer of the tree goes into this one bitmap: this player's own, then
    // each visible instance child's, however deep. The tree sequence they are
    // placed on was numbered by the caller.
    _mask_writers_dropped = false;
    _mask_collect_tree(this, f, Transform2D(), -1.0f, rs, have_bbox, bmin, bmax);
    if (_mask_writers_dropped) {
        WARN_PRINT_ONCE("SpriteStudio: an animation masks with more than 96 mask parts at once (instances count each time they are placed); the rest are ignored. Restructure the data to use fewer.");
    }

    if (!have_bbox) {
        _mask_tree_spans.clear();
        _mask_meta_array.clear();
        return;
    }
    Vector2 bsize = bmax - bmin;
    if (bsize.x < CMP_EPSILON) bsize.x = CMP_EPSILON;
    if (bsize.y < CMP_EPSILON) bsize.y = CMP_EPSILON;

    // Tiles: one while the writers fit a tile's bits, then 2 side by side, then
    // 2x2. They split the texture rather than growing it.
    const int count = (int)_mask_tree_spans.size();
    const int tiles = count <= SS_MASK_TILE_BITS ? 1 : (count <= SS_MASK_TILE_BITS * 2 ? 2 : 4);
    const int cols = tiles >= 2 ? 2 : 1;
    const int rows = tiles >= 4 ? 2 : 1;
    _mask_tiles = Vector2((float)cols, (float)rows);

    // The borrowed target's actual pixel size. The bbox is mapped onto each
    // tile's full [0,tw]x[0,th] region via its canvas transform, and the UV
    // transform below maps the same bbox to [0,1] (the shader offsets it into
    // the tile), so sampling is independent of these dims.
    const int tw = _mask_target->w / cols;
    const int th = _mask_target->h / rows;

    // canvas_transform: player-local -> coverage viewport pixels (bbox -> one tile).
    const Vector2 s((float)tw / bsize.x, (float)th / bsize.y);
    for (int tile = 0; tile < tiles; tile++) {
        const float ox = (float)((tile % cols) * tw);
        const float oy = (float)((tile / cols) * th);
        Transform2D ct;
        ct.columns[0] = Vector2(s.x, 0);
        ct.columns[1] = Vector2(0, s.y);
        ct.columns[2] = Vector2(ox - bmin.x * s.x, oy - bmin.y * s.y);
        rs->viewport_set_canvas_transform(_mask_target->viewport, _mask_tile_canvas(tile), ct);
    }

    // player-local -> coverage UV [0,1], handed to maskable shaders (P3).
    Transform2D uv;
    uv.columns[0] = Vector2(1.0f / bsize.x, 0);
    uv.columns[1] = Vector2(0, 1.0f / bsize.y);
    uv.columns[2] = Vector2(-bmin.x / bsize.x, -bmin.y / bsize.y);
    // The coverage target renders under this player's host viewport, so what the
    // shaders sample this frame is the coverage rasterized this frame, with the
    // `ct` above. Publish the matching UV transform immediately: holding it back
    // a frame would map positions through a bbox the sampled texels were never
    // rasterized with, sliding the mask off its target as the bbox moves.
    _mask_local_to_uv = uv;

    // Frame mask state consumed by the maskable emit path (P3).
    _mask_coverage_tex = rs->viewport_get_texture(_mask_target->viewport);

    // Decide the size class to borrow next frame from this frame's footprint.
    // Screen-linked: ~one coverage texel per on-screen pixel of the bbox, in
    // each tile — until the whole texture reaches the size cap, past which the
    // tiles give up resolution instead of the texture growing.
    _mask_next_w = ss_hysteretic_coverage_dim(bsize.x * _coverage_screen_scale * cols, _mask_target->w,
                                              MASK_COVERAGE_MIN_DIM, MASK_COVERAGE_MAX_DIM, MASK_SIZE_SHRINK_HYSTERESIS);
    _mask_next_h = ss_hysteretic_coverage_dim(bsize.y * _coverage_screen_scale * rows, _mask_target->h,
                                              MASK_COVERAGE_MIN_DIM, MASK_COVERAGE_MAX_DIM, MASK_SIZE_SHRINK_HYSTERESIS);

    // A playing animation picks the new class up on its next frame; a held one
    // (paused, stopped, the editor) would otherwise keep this coverage — from
    // the starting size class — for as long as it holds.
    _mask_resize_pending = _mask_next_w != _mask_target->w || _mask_next_h != _mask_target->h;

    // The target was re-requested (UPDATE_ONCE) above and renders ahead of this
    // player's host viewport, so its coverage is this frame's — valid from the
    // very first frame of masking and across size-class transitions alike.
    _mask_coverage_valid = true;
}

void SsInternalPlayer::_mask_collect_tree(SsInternalPlayer* p, const DrawFrame& pf,
                                          const Transform2D& to_root, float start_seq,
                                          RenderingServer* rs, bool& have_bbox,
                                          Vector2& bmin, Vector2& bmax) {
    auto draw_batches = pf.frameData->draw_batches();
    auto draw_order = pf.frameData->draw_order();
    if (!draw_batches || !draw_order) return;
    const uint16_t* draw_order_data = draw_order->data();
    const uint32_t n = draw_order->size();
    // `_mask_seq` numbers the draw_order `p` was last drawn with, which is the
    // one `pf` holds; a mismatch would place its writers on the wrong interval.
    if (p->_mask_seq.size() != n) return;

    // A draw_as_mask part comes as a Mask batch and its own colour batch at the
    // same rank; bake it once; a second pass under the same bit would carry
    // into the next bit under the additive blend.
    LocalVector<int> baked;
    const auto* parts_meta = pf.binary ? pf.binary->parts() : nullptr;
    for (uint32_t bi = 0; bi < draw_batches->size() && !p->_mask_writers.is_empty(); bi++) {
        const auto* batch = draw_batches->Get(bi);
        if (!batch) continue;
        const auto kind = batch->kind();
        if (kind != ss::runtime::DrawBatchKind_Normal && kind != ss::runtime::DrawBatchKind_Mask
            && kind != ss::runtime::DrawBatchKind_Mesh && kind != ss::runtime::DrawBatchKind_Shape) {
            continue;
        }

        RID tex_rid;
        Vector2 inv_tex_size(1, 1);
        if (batch->texture_hash() != 0 && p->_textures.has(batch->texture_hash())) {
            Ref<Texture2D> tex = p->_textures[batch->texture_hash()];
            if (tex.is_valid()) {
                tex_rid = tex->get_rid();
                const Vector2 ts = tex->get_size();
                if (ts.x > 0 && ts.y > 0) inv_tex_size = Vector2(1.0f / ts.x, 1.0f / ts.y);
            }
        }

        const uint16_t count = batch->count();
        for (uint16_t k = 0; k < count; k++) {
            const uint32_t rank = batch->start_rank() + k;
            const int p_idx = (int)draw_order_data[rank];
            const MaskWriter* w = nullptr;
            for (int wi = 0; wi < p->_mask_writers.size(); wi++) {
                if (p->_mask_writers[wi].part_index == p_idx) { w = &p->_mask_writers[wi]; break; }
            }
            if (!w || baked.has(p_idx)) continue;
            // A draw_as_mask shape's Mask batch has no shape geometry — its
            // Shape batch, at the same rank, is the one to bake.
            if (kind == ss::runtime::DrawBatchKind_Mask && parts_meta && p_idx < (int)parts_meta->size()
                && parts_meta->Get(p_idx)->part_type_type() == ss::format::PartType_PartTypeShape) {
                continue;
            }
            if ((int)_mask_tree_spans.size() >= MAX_MASK_WRITERS) { // bitmap full
                _mask_writers_dropped = true;
                return;
            }
            const uint8_t bit = (uint8_t)_mask_tree_spans.size();
            if (!_bake_coverage_geometry(pf, p, kind, tex_rid, inv_tex_size, p_idx,
                                         bit, to_root, rs, have_bbox, bmin, bmax)) {
                continue;
            }
            baked.push_back(p_idx);
            // Where the writer holds its bit on the tree sequence (open interval):
            // a clipping writer from itself to the end of the tree, so one written
            // inside an instance keeps clipping the caller's later parts; a pure
            // mask from the start of its own animation up to itself, so it closes
            // before anything its animation's caller draws next.
            const float seq = p->_mask_seq[rank];
            const Vector2 span = w->is_clipping ? Vector2(seq, MASK_SEQ_END) : Vector2(start_seq, seq);
            _mask_tree_spans.push_back(span);
            _mask_meta_array.push_back(Vector4(span.x, span.y, (float)bit, w->op_invert ? 1.0f : 0.0f));
        }
    }

    // Then every visible instance child's, in draw order, placed by the chain of
    // instance matrices. Its frame is still current: children draw before the
    // player that holds them.
    for (uint32_t rank = 0; rank < n; rank++) {
        const int p_idx = (int)draw_order_data[rank];
        if (p_idx < 0 || (uint32_t)p_idx >= p->_instance_children.size()) continue;
        const InstanceChildState& ics = p->_instance_children[p_idx];
        if (!ics.player || !ics.visible_this_frame) continue;
        if (!ics.player->_subtree_has_mask_writers()) continue;
        const float* im = pf.get_world_matrix(p_idx);
        if (!im) continue;
        DrawFrame cf = {};
        if (!ics.player->_fill_frame_from_runtime(cf)) continue;
        // Asking the runtime for the frame again rebuilds its buffer, which the
        // child's part pointers from its own draw still point into; re-index
        // them so the writers read this frame's mask value, not stale memory.
        ics.player->_index_frame_parts(cf);
        _mask_collect_tree(ics.player, cf, to_root * matrix_to_transform2d(im), p->_mask_seq[rank],
                           rs, have_bbox, bmin, bmax);
    }
}

void SsInternalPlayer::_mask_number_tree(float& counter) {
    const uint32_t n = _mask_draw_order.size();
    if (_mask_seq.size() != n) _mask_seq.resize(n);
    for (uint32_t rank = 0; rank < n; rank++) {
        _mask_seq[rank] = counter;
        counter += 1.0f;
        // A visible instance's sub-animation is drawn in its instance part's
        // place, so it is numbered right there, ahead of the next part.
        const uint16_t p_idx = _mask_draw_order[rank];
        if (p_idx >= _instance_children.size()) continue;
        const InstanceChildState& ics = _instance_children[p_idx];
        if (ics.player && ics.visible_this_frame) ics.player->_mask_number_tree(counter);
    }
}

bool SsInternalPlayer::_subtree_has_mask_writers() const {
    if (!_mask_writers.is_empty()) return true;
    for (uint32_t i = 0; i < _instance_children.size(); i++) {
        const InstanceChildState& ics = _instance_children[i];
        if (ics.player && ics.visible_this_frame && ics.player->_subtree_has_mask_writers()) return true;
    }
    return false;
}

bool SsInternalPlayer::_subtree_has_active_clip() const {
    for (int wi = 0; wi < _mask_writers.size(); wi++) {
        if (_mask_writers[wi].is_clipping) return true;
    }
    for (uint32_t i = 0; i < _instance_children.size(); i++) {
        const InstanceChildState& ics = _instance_children[i];
        if (ics.player && ics.visible_this_frame && ics.player->_subtree_has_active_clip()) return true;
    }
    return false;
}

int SsInternalPlayer::_find_carry_open_rank() const {
    const uint32_t n = _mask_draw_order.size();
    for (uint32_t rank = 0; rank < n; rank++) {
        const uint16_t p_idx = _mask_draw_order[rank];
        if (p_idx >= _instance_children.size()) continue;
        const InstanceChildState& ics = _instance_children[p_idx];
        if (ics.player && ics.visible_this_frame && ics.player->_subtree_has_active_clip()) return (int)rank;
    }
    return -1;
}

bool SsInternalPlayer::_mask_seq_covered(uint16_t rank) const {
    if (rank >= _mask_seq.size()) return false;
    const float s = _mask_seq[rank];
    for (uint32_t i = 0; i < _mask_tree_spans.size(); i++) {
        if (s > _mask_tree_spans[i].x && s < _mask_tree_spans[i].y) return true;
    }
    return false;
}

bool SsInternalPlayer::_bake_coverage_geometry(
        const DrawFrame& src_f, SsInternalPlayer* src, ss::runtime::DrawBatchKind kind,
        RID tex_rid, const Vector2& inv_tex_size, int p_idx, uint8_t bit,
        const Transform2D& to_owner, RenderingServer* rs,
        bool& have_bbox, Vector2& bmin, Vector2& bmax) {
    if (p_idx < 0 || p_idx >= (int)src->_parts_by_idx.size()) return false;
    const auto* part = src->_parts_by_idx[p_idx];
    if (!part) return false;
    const float* draw_m = src_f.get_world_matrix(p_idx);
    if (!draw_m) return false;

    // Build the writer's geometry per batch kind (from `src`'s buffers). Mesh
    // verts already arrive world-space; Normal/Mask/Shape apply draw_m inside
    // their builders.
    const SsVec2Array* gverts = nullptr;
    const SsVec2Array* guvs = nullptr;
    const SsIntArray* gindices = nullptr;
    bool no_cutout = false; // Shapes have no texture; the geometry is the mask.

    int nv = 0;
    int ni = 0;
    if (kind == ss::runtime::DrawBatchKind_Mesh) {
        if (!src->_build_mesh_geometry(src_f, p_idx, part, inv_tex_size, src->_mesh_buf)) return false;
        gverts = &src->_mesh_buf.verts; guvs = &src->_mesh_buf.uvs;
        gindices = &src->_mesh_buf.indices;
        nv = src->_mesh_buf.vert_count;
        ni = src->_mesh_buf.index_count;
    } else if (kind == ss::runtime::DrawBatchKind_Shape) {
        if (!src->_build_shape_geometry(src_f, p_idx, part, draw_m, src->_shape_buf)) return false;
        gverts = &src->_shape_buf.verts; guvs = &src->_shape_buf.uvs;
        gindices = &src->_shape_buf.indices;
        no_cutout = true;
        nv = src->_shape_buf.vert_count;
        ni = src->_shape_buf.index_count;
    } else {
        if (src->_per_part_normal_verts.size() < MAX_VERTICES_COUNT) {
            src->_per_part_normal_verts.resize(MAX_VERTICES_COUNT);
            src->_per_part_normal_uvs.resize(MAX_VERTICES_COUNT);
            src->_per_part_normal_colors.resize(MAX_VERTICES_COUNT);
            src->_per_part_normal_custom0.resize(MAX_VERTICES_COUNT * 4);
        }
        if (src->_per_part_normal_indices.size() < INDICES_COUNT_PENTAGON) {
            src->_per_part_normal_indices.resize(INDICES_COUNT_PENTAGON);
        }
        Vector2* verts_ptr = src->_per_part_normal_verts.ptrw();
        Vector2* uvs_ptr = src->_per_part_normal_uvs.ptrw();
        Color* colors_ptr = src->_per_part_normal_colors.ptrw();
        float* custom0_ptr = src->_per_part_normal_custom0.ptrw();
        const int vc = src->_build_normal(src_f, p_idx, part, draw_m, inv_tex_size,
                                          verts_ptr, uvs_ptr,
                                          colors_ptr, custom0_ptr, 0);
        if (vc <= 0) return false;
        int32_t* iptr = (int32_t*)src->_per_part_normal_indices.ptrw();
        if (vc == MAX_VERTICES_COUNT) {
            const int pent[INDICES_COUNT_PENTAGON] = { 0,1,4, 1,3,4, 3,2,4, 2,0,4 };
            for (int j = 0; j < INDICES_COUNT_PENTAGON; j++) iptr[j] = pent[j];
            ni = INDICES_COUNT_PENTAGON;
        } else {
            const int quad[INDICES_COUNT_QUAD] = { 0,1,2, 1,3,2 };
            for (int j = 0; j < INDICES_COUNT_QUAD; j++) iptr[j] = quad[j];
            ni = INDICES_COUNT_QUAD;
        }
        gverts = &src->_per_part_normal_verts; guvs = &src->_per_part_normal_uvs;
        gindices = &src->_per_part_normal_indices;
        nv = vc;
    }

    if (nv <= 0) return false;

    // Map into owner-local space for a bubbled writer (identity for own writers).
    const bool needs_xform = to_owner != Transform2D();
    const Vector2* vp;
    if (needs_xform) {
        if (_cov_xform_verts.size() < nv) _cov_xform_verts.resize(nv);
        Vector2* xp = _cov_xform_verts.ptrw();
        const Vector2* sp = gverts->ptr();
        for (int j = 0; j < nv; j++) xp[j] = to_owner.xform(sp[j]);
        vp = xp;
    } else {
        vp = gverts->ptr();
    }

    // Accumulate the writer bounding box in owner-local space.
    for (int j = 0; j < nv; j++) {
        if (!have_bbox) { bmin = bmax = vp[j]; have_bbox = true; }
        else {
            if (vp[j].x < bmin.x) bmin.x = vp[j].x;
            if (vp[j].y < bmin.y) bmin.y = vp[j].y;
            if (vp[j].x > bmax.x) bmax.x = vp[j].x;
            if (vp[j].y > bmax.y) bmax.y = vp[j].y;
        }
    }

    // Encode this writer's bit into R/G/B of its tile (24 per tile; alpha
    // reserved as the premultiplied-blend coverage accumulator).
    const int tile = bit / SS_MASK_TILE_BITS;
    const int tile_bit = bit % SS_MASK_TILE_BITS;
    const int chan = tile_bit / 8;
    const uint8_t bit_val = 1 << (tile_bit % 8);
    uint8_t r = (chan == 0) ? bit_val : 0;
    uint8_t g = (chan == 1) ? bit_val : 0;
    uint8_t b = (chan == 2) ? bit_val : 0;
    Color bit_color = Color::from_rgba8(r, g, b, 0);

    float threshold = no_cutout ? -1.0f
                    : (float)(255 - part->mask()) / 255.0f;
    if (threshold > 1.0f) threshold = 1.0f;

    SsDynamicMesh* mask_mesh = nullptr;
    RID mask_ci = _acquire_mask_canvas_item(tile, mask_mesh);
    Ref<ShaderMaterial> mat = _acquire_mask_write_material();
    mat->set_shader_parameter("mask_bit_color", bit_color);
    mat->set_shader_parameter("mask_threshold", threshold);
    rs->canvas_item_set_material(mask_ci, mat->get_rid());
    rs->canvas_item_set_transform(mask_ci, Transform2D());
    // uvs/indices are unaffected by the placement transform; only verts move
    // into owner space. The write shader takes its colour from a uniform, so
    // the geometry carries none.
    SsMeshStreams streams;
    streams.verts = vp;
    streams.uvs = guvs->ptr();
    streams.vertex_count = nv;
    streams.indices = gindices->ptr();
    streams.index_count = ni;
    _draw_dynamic_mesh(rs, *mask_mesh, mask_ci, streams, tex_rid);
    return true;
}

void SsInternalPlayer::_index_frame_parts(const DrawFrame& f) {
    auto parts = f.frameData->parts();
    const int total = f.binary->parts() ? (int)f.binary->parts()->size() : 0;
    if ((int)_parts_by_idx.size() != total) _parts_by_idx.resize(total);
    if ((int)_part_hidden.size() != total) _part_hidden.resize(total);
    if (total > 0) {
        memset(_parts_by_idx.ptr(), 0, total * sizeof(void*));
        // Default to visible; parts absent from this frame's PartState list
        // (not active) read as not-hidden via the query API.
        memset(_part_hidden.ptr(), 0, total * sizeof(uint8_t));
    }
    if (!parts) return;
    for (uint32_t i = 0; i < parts->size(); i++) {
        auto p = parts->Get(i);
        int idx = p->part_index();
        if (idx >= 0 && idx < total) {
            _parts_by_idx[idx] = p;
            _part_hidden[idx] = p->hide() ? 1 : 0;
        }
    }
}

bool SsInternalPlayer::_fill_frame_from_runtime(DrawFrame& f) {
    if (!runtime_ctx || _ssabRes.is_null()) return false;
    const unsigned char* data = nullptr;
    uintptr_t len = 0;
    ss_runtime_get_frame_data(runtime_ctx, previous_frame_no, &data, &len);
    if (!data) return false;
    f.frameData = ss::runtime::GetFrameData(data);
    f.binary = _ssabRes->get_ss_anime_binary();
    if (!f.frameData || !f.binary) return false;
    f.rs = RenderingServer::get_singleton();
    ss_runtime_get_world_matrices(runtime_ctx, &f.world_matrices, &f.world_matrices_len);
    ss_runtime_get_local_uvs(runtime_ctx, &f.local_uvs, &f.local_uvs_len);
    ss_runtime_get_cell_meta(runtime_ctx, &f.cell_meta, &f.cell_meta_len);
    ss_runtime_get_local_vertices(runtime_ctx, &f.local_vertices, &f.local_vertices_len);
    ss_runtime_get_shape_vertices(runtime_ctx, &f.shape_vertices, &f.shape_vertices_len);
    ss_runtime_get_shape_vertex_box_coords(runtime_ctx, &f.shape_box_coords, &f.shape_box_coords_len);
    ss_runtime_get_shape_vertex_counts(runtime_ctx, &f.shape_vertex_counts, &f.shape_vertex_counts_len);
    ss_runtime_get_mesh_vertices_x(runtime_ctx, &f.mesh_vertices_x, &f.mesh_vertices_x_len);
    ss_runtime_get_mesh_vertices_y(runtime_ctx, &f.mesh_vertices_y, &f.mesh_vertices_y_len);
    ss_runtime_get_mesh_vertex_offsets(runtime_ctx, &f.mesh_vertex_offsets, &f.mesh_vertex_offsets_len);
    ss_runtime_get_mesh_uvs(runtime_ctx, &f.mesh_uvs, &f.mesh_uvs_len);
    ss_runtime_get_mesh_indices(runtime_ctx, &f.mesh_indices, &f.mesh_indices_len);
    ss_runtime_get_mesh_index_offsets(runtime_ctx, &f.mesh_index_offsets, &f.mesh_index_offsets_len);
    return true;
}

bool SsInternalPlayer::_is_pure_mask_part(int p_idx) const {
    if (p_idx < 0 || (uint32_t)p_idx >= _part_pure_mask.size()) return false;
    return _part_pure_mask[p_idx] != 0;
}

void SsInternalPlayer::_set_mask_uv_uniform(Ref<ShaderMaterial> mat, const Transform2D& local_to_uv) {
    if (mat.is_null()) return;
    mat->set_shader_parameter("ss_mask_uv_basis",
        Vector4(local_to_uv.columns[0].x, local_to_uv.columns[1].x,
                local_to_uv.columns[0].y, local_to_uv.columns[1].y));
    mat->set_shader_parameter("ss_mask_uv_off", local_to_uv.columns[2]);
}

void SsInternalPlayer::_apply_mask_uniforms(Ref<ShaderMaterial> mat, uint16_t rank, bool visible_inside) {
    const int count = (int)_mask_tree_spans.size();
    mat->set_shader_parameter("ss_mask_enabled", true);
    _set_mask_uv_uniform(mat, _mask_local_to_uv);
    mat->set_shader_parameter("ss_mask_count", count);
    mat->set_shader_parameter("ss_mask_meta", _mask_meta_array);
    mat->set_shader_parameter("ss_mask_tiles", _mask_tiles);
    mat->set_shader_parameter("ss_mask_rank", rank < _mask_seq.size() ? _mask_seq[rank] : 0.0f);
    mat->set_shader_parameter("ss_mask_visible_inside", visible_inside ? 1.0f : 0.0f);
    if (count > 0 && _mask_coverage_tex.is_valid()) {
        // Bind the coverage viewport's texture (RID) to the sampler uniform.
        RenderingServer::get_singleton()->material_set_param(mat->get_rid(), "ss_mask_coverage", _mask_coverage_tex);
    }
}

void SsInternalPlayer::_stamp_part_mask(const Ref<ShaderMaterial>& mat, uint16_t rank,
                                        const PartMaskDecision& md) {
    if (mat.is_null()) return;
    if (!md.masked) {
        // Reuse of a pooled material — make sure masking is off.
        mat->set_shader_parameter("ss_mask_enabled", false);
        return;
    }
    if (!_parent_driven) {
        // This player owns the coverage and the tree sequence, both settled
        // before its parts are emitted.
        _apply_mask_uniforms(mat, rank, md.visible_inside);
        return;
    }
    // An instance child: the owner's coverage and the tree sequence do not exist
    // yet. Bake the composed polarity now; the owner's walk turns the test on
    // and binds the rest.
    mat->set_shader_parameter("ss_mask_enabled", false);
    mat->set_shader_parameter("ss_mask_visible_inside", md.visible_inside ? 1.0f : 0.0f);
    InheritedMaskMaterial rec;
    rec.mat = mat;
    rec.rank = rank;
    _inherited_mask_materials.push_back(rec);
}

void SsInternalPlayer::_set_inherited_mask_context(const InheritedMaskContext& ctx) {
    if (_inherited_mask == ctx) return;
    _inherited_mask = ctx;
    // The composed values are baked into per-part materials while building, so a
    // child whose frame did not advance would otherwise keep the old polarity.
    _inherited_mask_dirty = true;
}

SsInternalPlayer::InheritedMaskContext
SsInternalPlayer::_compose_mask_context(const ss::format::PartData* pd) const {
    InheritedMaskContext c = _inherited_mask;
    if (!pd) return c;
    c.influence = c.influence && pd->mask_influence();
    c.visible_inside = c.visible_inside || pd->visible_inside_mask();
    return c;
}

SsInternalPlayer::InheritedMaskContext
SsInternalPlayer::_child_mask_context(const ss::format::PartData* pd) const {
    InheritedMaskContext c;
    if (!pd) return c;
    // Only the instance part's own flags reach the sub-animation — not what this
    // player itself inherited — and they do whether or not anything above masks
    // (SpriteStudio 7.5 on InstancePropagationTired3).
    c.influence = pd->mask_influence();
    c.visible_inside = pd->visible_inside_mask();
    // Whether a mask from here or above can reach the sub-animation's parts.
    c.masked = _mask_active();
    return c;
}

SsInternalPlayer::PartMaskDecision
SsInternalPlayer::_resolve_part_mask(const DrawFrame& f, int p_idx, uint16_t rank) const {
    PartMaskDecision d;
    const auto* pm = f.binary ? f.binary->parts() : nullptr;
    const ss::format::PartData* pd =
        (pm && p_idx >= 0 && p_idx < (int)pm->size()) ? pm->Get(p_idx) : nullptr;
    const InheritedMaskContext c = _compose_mask_context(pd);
    // A part opts out with mask_influence == 0, which the composition has
    // already folded in. For a clipping writer the *same* mask_influence is both
    // its write op and its target flag, so mask_write must not
    // force it to be a target: a mask_influence == 0 clipping part is opted out,
    // and its own colour is only clipped by *other* masks per its real influence.
    if (!c.influence) return d;

    d.visible_inside = c.visible_inside;
    // A part that draws only inside a mask draws nowhere that no mask covers —
    // even when the animation has no mask at all (SpriteStudio 7.5), so it is
    // always tested. With no writer the test fails everywhere.
    if (c.visible_inside) {
        d.masked = true;
        return d;
    }
    // Otherwise a part is a mask target only where masking is honoured for this
    // player — it has mask parts, or a caller's mask reaches it — or after an
    // instance whose sub-animation left a clipping mask open.
    const bool carried = _mask_carry_open_rank >= 0 && (int)rank > _mask_carry_open_rank;
    if (!_mask_active() && !carried) return d;
    // An instance child cannot tell yet which of the tree's writers reach it, so
    // every target runs the test. The owner knows them all: a part no writer
    // covers would pass the test anyway.
    d.masked = _parent_driven || _mask_seq_covered(rank);
    return d;
}

void SsInternalPlayer::_apply_inherited_mask(RID coverage_tex, const Array& meta, int count,
                                             const Vector2& tiles, const Transform2D& local_to_uv) {
    RenderingServer* rs = RenderingServer::get_singleton();
    for (int i = 0; i < _inherited_mask_materials.size(); i++) {
        const InheritedMaskMaterial& rec = _inherited_mask_materials[i];
        const Ref<ShaderMaterial>& mat = rec.mat;
        if (mat.is_null()) continue;
        mat->set_shader_parameter("ss_mask_enabled", true);
        _set_mask_uv_uniform(mat, local_to_uv);
        mat->set_shader_parameter("ss_mask_count", count);
        mat->set_shader_parameter("ss_mask_meta", meta);
        mat->set_shader_parameter("ss_mask_tiles", tiles);
        // The owner numbered this player's ranks on the tree sequence just now.
        mat->set_shader_parameter("ss_mask_rank", rec.rank < _mask_seq.size() ? _mask_seq[rec.rank] : 0.0f);
        // `ss_mask_visible_inside` was stamped per part while building, with the
        // composed polarity — flattening it here is exactly the bug this walk
        // exists to avoid.
        if (count > 0 && coverage_tex.is_valid()) {
            rs->material_set_param(mat->get_rid(), "ss_mask_coverage", coverage_tex);
        }
    }
    // Nested instances: this frame's coverage only exists at its owner, so the
    // walk continues down. Each level folds in its slot placement.
    for (int i = 0; i < _inherited_mask_children.size(); i++) {
        const InheritedMaskChild& c = _inherited_mask_children[i];
        if (!c.player) continue;
        c.player->_apply_inherited_mask(coverage_tex, meta, count, tiles, local_to_uv * c.slot_xf);
    }
}

void SsInternalPlayer::_drawAnimation(float frame_no, float delta_seconds, bool parent_looped) {
    // Nothing set up, so there is no frame to draw -- the same check
    // `_fill_frame_from_runtime` makes before the same FFI call, and for a
    // stronger reason here: asking the runtime for frame data it does not have
    // is an error on its side, and it logs one line per call. The transport
    // setters reach here before an animation exists (the node's constructor
    // alone does, through `setSubFrameEnabled`, and so does every Instance
    // child at setup), which is ordinary rather than a failure. Bailing out
    // below on `!data` is one call too late to say so.
    if (!runtime_ctx || !_currentAnimationData) return;
    const unsigned char* data = nullptr;
    uintptr_t len = 0;
    ss_runtime_get_frame_data(runtime_ctx, frame_no, &data, &len);
    if (!data) return;

    // Reset the low-level mesh pool cursor so we reuse mesh RIDs from the
    // front of the pool rather than allocating new ones.
    _mesh_pool_in_use = 0;
    // Walk the per-part material / canvas_item pool cursors back to 0 so any
    // per-part rendering during this frame re-uses entries from the front of
    // each pool rather than growing them. No-op when no per-part variants
    // are dispatched (pools stay empty).
    _reset_per_part_pools();
    // Rebuilt below as parts resolve their composed mask decision; the coverage
    // owner walks these once its coverage exists.
    _inherited_mask_materials.clear();
    _inherited_mask_children.clear();
    _inherited_mask_dirty = false;
    // This draw is the one that picks the override layer up, whichever path
    // reached here — the owner's seek or the parent's child walk.
    _overrides_dirty = false;

    DrawFrame f = {};
    f.rs = RenderingServer::get_singleton();
    f.frameData = ss::runtime::GetFrameData(data);
    f.binary = _ssabRes->get_ss_anime_binary();
    f.frame_no = frame_no;
    f.delta_seconds = delta_seconds;
    f.parent_looped = parent_looped;

    // Fetch all SoA buffers from runtime in one block.
    ss_runtime_get_world_matrices(runtime_ctx, &f.world_matrices, &f.world_matrices_len);
    ss_runtime_get_local_uvs(runtime_ctx, &f.local_uvs, &f.local_uvs_len);
    ss_runtime_get_cell_meta(runtime_ctx, &f.cell_meta, &f.cell_meta_len);
    ss_runtime_get_local_vertices(runtime_ctx, &f.local_vertices, &f.local_vertices_len);
    ss_runtime_get_shape_vertices(runtime_ctx, &f.shape_vertices, &f.shape_vertices_len);
    ss_runtime_get_shape_vertex_box_coords(runtime_ctx, &f.shape_box_coords, &f.shape_box_coords_len);
    ss_runtime_get_shape_vertex_counts(runtime_ctx, &f.shape_vertex_counts, &f.shape_vertex_counts_len);
    ss_runtime_get_mesh_vertices_x(runtime_ctx, &f.mesh_vertices_x, &f.mesh_vertices_x_len);
    ss_runtime_get_mesh_vertices_y(runtime_ctx, &f.mesh_vertices_y, &f.mesh_vertices_y_len);
    ss_runtime_get_mesh_vertex_offsets(runtime_ctx, &f.mesh_vertex_offsets, &f.mesh_vertex_offsets_len);
    ss_runtime_get_mesh_uvs(runtime_ctx, &f.mesh_uvs, &f.mesh_uvs_len);
    ss_runtime_get_mesh_indices(runtime_ctx, &f.mesh_indices, &f.mesh_indices_len);
    ss_runtime_get_mesh_index_offsets(runtime_ctx, &f.mesh_index_offsets, &f.mesh_index_offsets_len);

    auto draw_order = f.frameData->draw_order();
    auto draw_batches = f.frameData->draw_batches();

    _index_frame_parts(f);

    // CBP masking: collect this frame's mask writers. The top-root player owns
    // the mask state for the whole instance tree: it numbers the tree and bakes
    // every writer in it into one coverage bitmap. Instance children have drawn
    // already (they draw before the player that holds them), so their writers
    // and draw orders are current here.
    _build_mask_writers(f);
    {
        const uint32_t n = draw_order->size();
        if (_mask_draw_order.size() != n) _mask_draw_order.resize(n);
        if (n > 0) memcpy(_mask_draw_order.ptr(), draw_order->data(), n * sizeof(uint16_t));
    }
    _mask_carry_open_rank = _find_carry_open_rank();
    if (!_parent_driven) {
        float seq = 0.0f;
        _mask_number_tree(seq);
    }
    if (!_parent_driven && _subtree_has_mask_writers()) {
        _render_mask_coverage(f);
    } else {
        // Not masking this frame — return any borrowed coverage target to the pool.
        _release_mask_target();
        _mask_tree_spans.clear();
        _mask_meta_array.clear();
        _mask_resize_pending = false;
    }

    const uint16_t* draw_order_data = draw_order->data();
    const uint32_t batch_count = draw_batches->size();

    // Hide / clear unused entries in the pool (peak-retain policy).
    for (int i = (int)batch_count; i < _batch_canvas_items_in_use; i++) {
        f.rs->canvas_item_clear(_batch_canvas_items[i]);
        f.rs->canvas_item_set_visible(_batch_canvas_items[i], false);
    }

    // Effect emitter canvas items are owned by EffectSlotState (persistent
    // across frames) and hang off whatever batch CI they were last parented
    // to. When an effect part stops producing an Effect batch (HIDE'd) or its
    // slot reports not_visible, `_emit_effect_slot` is skipped or early-returns
    // without touching `emitter_cis` — leaving stale particle geometry on a
    // pool slot that gets recycled by another (visible) batch. Hide every
    // emitter CI up-front; `_emit_effect_slot`'s visible branch re-shows only
    // the ones it actually draws this frame.
    for (const EffectSlotState& slot : _effect_slots) {
        for (const RID& e_ci : slot.emitter_cis) {
            f.rs->canvas_item_set_visible(e_ci, false);
        }
    }

    for (uint32_t i = 0; i < _instance_children.size(); i++) {
        if (_instance_children[i].player) _instance_children[i].player->setRootVisible(false);
    }

    _step_effect_slots(f);

    // Per-frame draw-order counter: batch CIs and per-part CIs draw in the
    // order they are emitted (rank order), not in CI-pool allocation order.
    _draw_seq = 0;
    _run = DefaultRun();
    for (uint32_t bi = 0; bi < batch_count; bi++) {
        const auto* batch = draw_batches->Get(bi);
        const auto kind = batch->kind();
        // Drawn before this batch's canvas item takes its draw index below.
        if (_run.open && !_run_can_take(kind, batch)) {
            _flush_default_run(f.rs);
        }
        RID ci = _ensure_batch_ci((int)bi);

        // Reset batch CI state before use. Pool reuse can leak properties
        // from previous frames / different kinds (e.g. Effect transform,
        // Effect part-alpha modulate).
        f.rs->canvas_item_clear(ci);
        f.rs->canvas_item_set_visible(ci, true);
        f.rs->canvas_item_set_transform(ci, Transform2D());
        f.rs->canvas_item_set_material(ci, RID());
        f.rs->canvas_item_set_modulate(ci, Color(1, 1, 1, 1));

        // We use explicit draw indices for internal Z sorting within the player.
        // Using global z_index would cause character parts to interleave with
        // other nodes in the scene. Tree order (child index) is not usable here
        // because our CI pool is reused and ordering is fixed at allocation.
        f.rs->canvas_item_set_draw_index(ci, _draw_seq++);
        _batch_ci = ci;
        _batch_ci_free = true;

        // Pure masks feed only the coverage bitmap and must not draw their own
        // colour (PartTypeMask has no draw branch below; this also suppresses
        // shape/text/nines *_mask color draws so they read as holes, not fills).
        // Normal / Shape / Mesh batches group several parts by texture + blend
        // and can mix mask and non-mask parts, so those test per part inside
        // their emit loops; the single-part kinds are handled here.
        if (kind == ss::runtime::DrawBatchKind_Normal) {
            _emit_normal_batch(f, batch, draw_order_data);
        } else if (kind == ss::runtime::DrawBatchKind_Shape) {
            _emit_shape_batch(f, batch, draw_order_data);
        } else if (kind == ss::runtime::DrawBatchKind_Instance) {
            int p_idx = (int)draw_order_data[batch->start_rank()];
            if (_is_pure_mask_part(p_idx)) continue;
            const auto* part = (p_idx >= 0 && p_idx < (int)_parts_by_idx.size()) ? _parts_by_idx[p_idx] : nullptr;
            if (!part) continue;
            const float* drawing_m = f.get_world_matrix(p_idx);
            if (!drawing_m) continue;
            _emit_instance_slot(f, ci, p_idx, drawing_m);
        } else if (kind == ss::runtime::DrawBatchKind_Effect) {
            int p_idx = (int)draw_order_data[batch->start_rank()];
            if (_is_pure_mask_part(p_idx)) continue;
            const auto* part = (p_idx >= 0 && p_idx < (int)_parts_by_idx.size()) ? _parts_by_idx[p_idx] : nullptr;
            if (!part) continue;
            const float* drawing_m = f.get_world_matrix(p_idx);
            if (!drawing_m) continue;
            _emit_effect_slot(f, ci, p_idx, drawing_m);
        } else if (kind == ss::runtime::DrawBatchKind_Mesh) {
            _emit_mesh_batch(f, batch, draw_order_data);
        }
        // DrawBatchKind_Text / Nines: not yet implemented. Mask is not missing —
        // it is consumed by the mask planner above, not emitted as a batch here.
    }
    _flush_default_run(f.rs);

    _batch_canvas_items_in_use = (int)batch_count;
}

bool SsInternalPlayer::_run_can_take(ss::runtime::DrawBatchKind kind, const ss::runtime::DrawBatch* batch) const {
    switch (kind) {
        case ss::runtime::DrawBatchKind_Mask:
        case ss::runtime::DrawBatchKind_Text:
        case ss::runtime::DrawBatchKind_Nines:
            return true;
        case ss::runtime::DrawBatchKind_Normal:
        case ss::runtime::DrawBatchKind_Mesh:
            return !_run.shape && _run.texture_hash == batch->texture_hash()
                && _run.blend == (ss::format::BlendType)batch->blend_type();
        case ss::runtime::DrawBatchKind_Shape:
            return _run.shape && _run.blend == (ss::format::BlendType)batch->blend_type();
        default:
            return false;
    }
}

void SsInternalPlayer::_run_begin(bool shape, uint32_t texture_hash, const RID& texture, ss::format::BlendType blend) {
    _run.open = true;
    _run.shape = shape;
    _run.texture_hash = texture_hash;
    _run.texture = texture;
    _run.blend = blend;
    _run.ci = _batch_ci_free ? _batch_ci : RID();
    _batch_ci_free = false;
    _run.vertex_count = 0;
    _run.index_count = 0;
}

void SsInternalPlayer::_run_reserve(int vertices, int indices) {
    RunStreams& s = _run_streams;
    const int need_v = _run.vertex_count + vertices;
    if (need_v > s.vertex_capacity) {
        s.vertex_capacity = MAX(need_v, s.vertex_capacity + s.vertex_capacity / 2);
        _normal_verts.resize(s.vertex_capacity);
        _normal_uvs.resize(s.vertex_capacity);
        _normal_colors.resize(s.vertex_capacity);
        _normal_custom0.resize(s.vertex_capacity * 4);
        s.verts = _normal_verts.ptrw();
        s.uvs = _normal_uvs.ptrw();
        s.colors = _normal_colors.ptrw();
        s.custom0 = _normal_custom0.ptrw();
    }
    const int need_i = _run.index_count + indices;
    if (need_i > s.index_capacity) {
        s.index_capacity = MAX(need_i, s.index_capacity + s.index_capacity / 2);
        _normal_indices.resize(s.index_capacity);
        s.indices = (int32_t*)_normal_indices.ptrw();
    }
}

void SsInternalPlayer::_run_append(const Vector2* verts, const Vector2* uvs, const Color* colors, const float* custom0,
                                   int vertex_count, const int32_t* indices, int index_count) {
    _run_reserve(vertex_count, index_count);
    const RunStreams& s = _run_streams;
    const int vbase = _run.vertex_count;
    memcpy(s.verts + vbase, verts, vertex_count * sizeof(Vector2));
    memcpy(s.uvs + vbase, uvs, vertex_count * sizeof(Vector2));
    memcpy(s.colors + vbase, colors, vertex_count * sizeof(Color));
    memcpy(s.custom0 + vbase * 4, custom0, vertex_count * 4 * sizeof(float));
    int32_t* dst = s.indices + _run.index_count;
    for (int i = 0; i < index_count; i++) {
        dst[i] = vbase + indices[i];
    }
    _run.vertex_count += vertex_count;
    _run.index_count += index_count;
}

void SsInternalPlayer::_flush_default_run(RenderingServer* rs) {
    if (!_run.open) return;
    _run.open = false;
    if (_run.index_count == 0) return;
    RID run_ci = _run.ci;
    if (run_ci.is_valid()) {
        // The batch gave its canvas item a placeholder draw index before the
        // run's place in the rank order was known; this is that place.
        rs->canvas_item_set_draw_index(run_ci, _draw_seq++);
    } else {
        run_ci = _acquire_per_part_canvas_item(); // draw_index = _draw_seq++
    }
    _apply_partcolor_material(rs, run_ci, s_default_shader_id_hash, _run.blend);
    rs->canvas_item_set_transform(run_ci, Transform2D());
    const RunStreams& s = _run_streams;
    _emit_partcolor_mesh(rs, run_ci, s.indices, _run.index_count, s.verts, s.colors, s.uvs,
                         s.custom0, _run.vertex_count, _run.texture);
}





void SsInternalPlayer::_load_external_ssabs() {
    _external_ssabs.clear();
    _external_ssabs_by_pack_hash.clear();
    if (_ssabRes.is_null()) return;
    auto binary = _ssabRes->get_ss_anime_binary();
    if (!binary) return;
    if (!binary->external_instances() || binary->external_instances()->size() == 0) return;

    auto exts = binary->external_instances();
    String parent_dir = _ssabRes->get_parent_dir();
    HashSet<String> loaded_packs;
    for (uint32_t i = 0; i < exts->size(); i++) {
        auto entry = exts->Get(i);
        if (!entry || !entry->anime_pack_name()) continue;
        String pack = String::utf8(entry->anime_pack_name()->c_str());
        if (pack.is_empty() || loaded_packs.has(pack)) continue;
        loaded_packs.insert(pack);
        String path = parent_dir.path_join(pack + ".ssab");
        Ref<Resource> res = SsResourceLoader::get_singleton()->load(path);
        Ref<SSABResource> ssab = res;
        if (ssab.is_null()) {
            ERR_PRINT(vformat("[SS] external SSAB load failed: %s", path));
            continue;
        }
        if (!ssab->is_valid()) {
            ERR_PRINT(vformat("[SS] external SSAB invalid: %s", path));
            continue;
        }
        _external_ssabs.push_back(ssab);
        if (ssab->get_ss_anime_binary()) {
            _external_ssabs_by_pack_hash[ssab->get_ss_anime_binary()->name_hash()] = ssab;
        }
    }
}

bool SsInternalPlayer::_subtree_borrow_stale() const {
    if (!_ssabRes.is_null() && _parsed.is_valid() &&
        _ssabRes->get_generation() != _borrowed_generation) {
        return true;
    }
    // Children borrow external .ssab files, which nothing watches for
    // "changed". Walking them is what catches a re-import of those. Terminates:
    // it visits children that exist, not the reference graph behind them.
    for (uint32_t i = 0; i < _instance_children.size(); i++) {
        const SsInternalPlayer* child = _instance_children[i].player;
        if (child && child->_subtree_borrow_stale()) {
            return true;
        }
    }
    return false;
}

Ref<SSABResource> SsInternalPlayer::_resolve_ssab_by_hash(uint32_t pack_hash, uint32_t name_hash) const {
    if (pack_hash != 0) {
        if (auto* ext_ptr = _external_ssabs_by_pack_hash.getptr(pack_hash)) {
            auto anim = (*ext_ptr)->find_animation_by_hash(name_hash);
            if (anim) {
                return *ext_ptr;
            }
        }
    }
    // Not external or not found; check current binary.
    if (!_ssabRes.is_null()) {
        auto anim = _ssabRes->find_animation_by_hash(name_hash);
        if (anim) return _ssabRes;
    }
    return Ref<SSABResource>();
}

void SsInternalPlayer::_clear_instance_children() {
    for (uint32_t i = 0; i < _instance_children.size(); i++) {
        InstanceChildState& st = _instance_children[i];
        if (st.player) {
            memdelete(st.player);
            st.player = nullptr;
        }
        if (st.instance_slot) {
            ss_instance_slot_destroy(st.instance_slot);
            st.instance_slot = nullptr;
        }
    }
    _instance_children.clear();
    // These borrow the child pointers just freed; the next draw refills them.
    _inherited_mask_children.clear();
}

void SsInternalPlayer::_setup_instance_children() {
    _clear_instance_children();
    if (_ssabRes.is_null()) return;
    auto binary = _ssabRes->get_ss_anime_binary();
    if (!binary || !binary->parts()) return;

    auto parts = binary->parts();
    _instance_children.resize(parts->size());
    for (uint32_t i = 0; i < parts->size(); i++) {
        _instance_children[i] = InstanceChildState{};
    }
    for (int i = 0; i < (int)parts->size(); i++) {
        auto p = parts->Get(i);
        if (!p) continue;
        if (p->part_type_type() != ss::format::PartType_PartTypeInstance) continue;
        auto pt = p->part_type_as_PartTypeInstance();
        if (!pt) continue;

        String pack_name = pt->ref_anime_pack() ? String::utf8(pt->ref_anime_pack()->c_str()) : String();
        String anim_name = pt->ref_anime_name() ? String::utf8(pt->ref_anime_name()->c_str()) : String();
        uint32_t pack_name_hash = pt->ref_anime_pack_hash();
        uint32_t anim_name_hash = pt->ref_anime_name_hash();

        Ref<SSABResource> source = _resolve_ssab_by_hash(pack_name_hash, anim_name_hash);

        if (source.is_null()) {
            ERR_PRINT(vformat("[SS] %s: instance part %s references animation \"%s\" in pack \"%s\", which is in neither this SSAB nor the ones beside it.",
                              ssab_label(_ssabRes), part_label(p, i), anim_name, pack_name));
            continue;
        }

        SsInternalPlayer* child = memnew(SsInternalPlayer);
        child->setParentDriven(true);
        // Before the resource: binding it draws, and that creates canvas items.
        child->setCanvasItemDefaults(_texture_filter, _texture_repeat, _light_mask);
        child->setSubFrameEnabled(_sub_frame_enabled);
        // Hand the child the SSAB that actually contains the referenced
        // animation — may be `_ssabRes` itself or an external sibling.
        child->setSSABResource(source);
        child->setAnimationByHash(anim_name_hash);
        child->stop();
        // Keep the child hidden by default; _update_instance_children flips
        // it visible only once an EventInstance becomes active for the slot.
        child->setRootVisible(false);
        // Parenting under the slot's batch canvas_item is performed each
        // frame in _emit_instance_slot — batch CIs are recyclable so a
        // setup-time parenting wouldn't survive batch-list shifts.
        InstanceChildState st;
        st.player = child;
        st.instance_slot = ss_instance_slot_create();
        _instance_children[i] = st;
    }
}

void SsInternalPlayer::_clear_effect_slots() {
    RenderingServer* rs = RenderingServer::get_singleton();
    for (uint32_t i = 0; i < _effect_slots.size(); i++) {
        EffectSlotState& slot = _effect_slots[i];
        if (slot.effect_slot) {
            ss_effect_slot_destroy(slot.effect_slot);
            slot.effect_slot = nullptr;
        }
        for (int e = 0; e < slot.emitter_cis.size(); e++) {
            rs->free_rid(slot.emitter_cis[e]);
        }
        slot.emitter_cis.clear();
        for (uint32_t e = 0; e < slot.emitter_meshes.size(); e++) {
            ss_free_dynamic_mesh(rs, slot.emitter_meshes[e]);
        }
        slot.emitter_meshes.clear();
    }
    _effect_slots.clear();
}

void SsInternalPlayer::_setup_effect_slots() {
    _clear_effect_slots();
    if (_ssabRes.is_null() || _parsed.is_null()) return;
    auto binary = _ssabRes->get_ss_anime_binary();
    if (!binary || !binary->parts()) return;

    auto parts = binary->parts();
    _effect_slots.resize(parts->size());
    for (uint32_t i = 0; i < parts->size(); i++) {
        _effect_slots[i] = EffectSlotState{};
    }
    for (int i = 0; i < (int)parts->size(); i++) {
        auto p = parts->Get(i);
        if (!p) continue;
        if (p->part_type_type() != ss::format::PartType_PartTypeEffect) continue;
        auto pt = p->part_type_as_PartTypeEffect();
        if (!pt) continue;

        const uint32_t seed = (uint32_t)i ^ pt->ref_effect_name_hash();
        void* effect_slot = ss_effect_slot_create(_parsed->get_handle(), pt->ref_effect_name_hash(), seed);
        if (!effect_slot) {
            // The hash is all the authored reference carries -- the effect's name
            // is not in the part -- so it stays in the message, next to the part
            // name the user can actually find in SpriteStudio.
            ERR_PRINT(vformat("[SS] %s: effect part %s references an effect (name hash 0x%x) that is not in this SSAB.",
                              ssab_label(_ssabRes), part_label(p, i), pt->ref_effect_name_hash()));
            continue;
        }
        _effect_slots[i].effect_slot = effect_slot;
    }
}

void SsInternalPlayer::_reset_slots_playback_state() {
    for (uint32_t i = 0; i < _effect_slots.size(); i++) {
        if (_effect_slots[i].effect_slot) ss_effect_slot_reset(_effect_slots[i].effect_slot);
    }
    for (uint32_t i = 0; i < _instance_children.size(); i++) {
        InstanceChildState& st = _instance_children[i];
        if (st.instance_slot) ss_instance_slot_reset(st.instance_slot);
        // Recurse so a re-play also restarts independent effects/instances
        // nested inside instance children (their slots live on the child).
        if (st.player) st.player->_reset_slots_playback_state();
    }
}

void SsInternalPlayer::_update_instance_children(float parent_frame_no, float delta_seconds, bool parent_looped) {
    // Cleared each tick; `_drive_instance_slot` re-arms it for the slots whose
    // EventInstance is active this frame (read by the coverage pass).
    for (uint32_t i = 0; i < _instance_children.size(); i++) {
        _instance_children[i].visible_this_frame = false;
    }
    if (!_currentAnimationData) {
        // No animation selected on the parent: keep all children hidden.
        for (uint32_t i = 0; i < _instance_children.size(); i++) {
            SsInternalPlayer* child = _instance_children[i].player;
            if (child) child->setRootVisible(false);
        }
        return;
    }

    const auto* binary = _ssabRes.is_null() ? nullptr : _ssabRes->get_ss_anime_binary();
    const auto* parts_meta = binary ? binary->parts() : nullptr;

    for (uint32_t p_idx = 0; p_idx < _instance_children.size(); p_idx++) {
        InstanceChildState& state = _instance_children[p_idx];
        SsInternalPlayer* child = state.player;
        if (!child || !state.instance_slot) continue;

        // Compose before the child builds: it bakes the polarity into its own
        // per-part materials, which is what keeps the callee's settings in play
        // instead of the instance part's overwriting them.
        const ss::format::PartData* pd =
            (parts_meta && p_idx < parts_meta->size()) ? parts_meta->Get(p_idx) : nullptr;
        child->_set_inherited_mask_context(_child_mask_context(pd));

        const ss_event_instance_info info = ss_runtime_get_active_event_instance(runtime_ctx, p_idx);
        _drive_instance_slot(state, child, info, parent_frame_no, delta_seconds, parent_looped);
    }
}

bool SsInternalPlayer::_has_mask_capable_parts() const {
    if (_static_mask_capable >= 0) return _static_mask_capable != 0;
    bool found = false;
    const auto* binary = _ssabRes.is_null() ? nullptr : _ssabRes->get_ss_anime_binary();
    const auto* parts_meta = binary ? binary->parts() : nullptr;
    if (parts_meta) {
        for (uint32_t i = 0; i < parts_meta->size() && !found; i++) {
            const auto* pd = parts_meta->Get(i);
            if (!pd) continue;
            found = pd->part_type_type() == ss::format::PartType_PartTypeMask
                    || pd->draw_as_mask() || pd->mask_write();
        }
    }
    _static_mask_capable = found ? 1 : 0;
    return found;
}

void SsInternalPlayer::_drive_instance_slot(InstanceChildState& state,
                                            SsInternalPlayer* child,
                                            const ss_event_instance_info& info,
                                            float parent_frame_no,
                                            float delta_seconds,
                                            bool parent_looped) {
    // All per-slot lifecycle (transition detection, label resolution, child
    // playback config, frame stepping, child-loop detection) runs inside
    // ss_instance_slot_step against the *child* runtime context. The host
    // is left with what ssruntime does not do: starting the child on a
    // transition (play()), and the redraw.
    const ss_instance_step_result r = ss_instance_slot_step(
        state.instance_slot, info, child->runtime_ctx,
        parent_frame_no, delta_seconds, parent_looped);

    // The coverage pass reads this to decide whose clipping writers to bubble up.
    state.visible_this_frame = r.visible;

    if (r.transitioned) {
        // step() configured the child for the new key (section, direction,
        // loop count) but did not start it. play() does: it snaps the child to
        // the section start and clears its own slots' key memory. Without it an
        // independent child never advances. The child has no event sink, so no
        // animation_started is emitted for it.
        child->play();
    }

    if (!r.visible) {
        return;
    }

    child->_update_instance_children(r.child_frame_no, delta_seconds, r.child_looped);

    _redraw_child_if_frame_changed(child, r.child_frame_no, delta_seconds, r.child_looped);
}

void SsInternalPlayer::_redraw_child_if_frame_changed(SsInternalPlayer* child, float frame_no, float delta_seconds, bool parent_looped) {
    const float draw_frame = child->_sub_frame_enabled ? frame_no : floorf(frame_no);
    // A changed inherited mask context has to rebuild even on a held frame: the
    // composed polarity is baked into the child's per-part materials. So does a
    // clipping mask opening or closing below it, which moves where its own parts
    // start being mask targets.
    if (child->previous_frame_no == draw_frame && !child->_needs_continuous_update()
        && !child->_inherited_mask_dirty && !child->_overrides_dirty
        && child->_find_carry_open_rank() == child->_mask_carry_open_rank) return;
    child->previous_frame_no = draw_frame;
    child->_drawAnimation(draw_frame, delta_seconds, parent_looped);
}

void SsInternalPlayer::_seek_and_redraw(float frame_no, float delta_seconds, bool parent_looped) {
    // Seeking draws without going through `update`, so it needs the same check.
    // Children are covered by their parent's walk. Falls through afterwards so
    // the frame the caller asked for is the one drawn.
    if (!_parent_driven && _subtree_borrow_stale()) {
        onSSABReloaded();
    }
    const float draw_frame = _sub_frame_enabled ? frame_no : floorf(frame_no);
    previous_frame_no = draw_frame;
    _update_instance_children(draw_frame, delta_seconds, parent_looped);
    _drawAnimation(draw_frame, delta_seconds, parent_looped);
}

void SsInternalPlayer::_emit_instance_slot(const DrawFrame& f, RID ci, int p_idx, const float* slot_matrix) {
    if (p_idx < 0 || (uint32_t)p_idx >= _instance_children.size()) return;
    SsInternalPlayer* child = _instance_children[p_idx].player;
    if (!child) return;

    // Re-parent every frame: the batch CI pool can shuffle as draw_batches
    // ordering changes, so the slot CI for a given Instance part is not
    // guaranteed to be the same RID across frames.
    const Transform2D slot_xf = matrix_to_transform2d(slot_matrix);
    child->setParentCanvasItem(ci);
    child->setRootTransform(slot_xf);
    child->setRootVisible(true);

    const float part_alpha = _parts_by_idx[p_idx] ? _parts_by_idx[p_idx]->alpha() : 1.0f;
    f.rs->canvas_item_set_modulate(ci, Color(1, 1, 1, part_alpha));

    // Which of the child's parts are mask targets, and with what polarity, was
    // decided when it built (composed in `_update_instance_children`). The owner
    // of the coverage binds them now; below it, hand the child on so the owner's
    // walk continues through here.
    if (!_parent_driven) {
        child->_apply_inherited_mask(_mask_coverage_tex, _mask_meta_array,
                                     (int)_mask_tree_spans.size(), _mask_tiles,
                                     _mask_local_to_uv * slot_xf);
    } else {
        InheritedMaskChild rec;
        rec.player = child;
        rec.slot_xf = slot_xf;
        _inherited_mask_children.push_back(rec);
    }
}

void SsInternalPlayer::_step_effect_slots(const DrawFrame& f) {
    // Every slot, every frame, whether or not its part is in this frame's draw
    // order: a hidden effect part has no Effect batch, and stepping only the
    // drawn ones froze an independently playing effect for as long as its part
    // was hidden. A hidden part's slot steps with `draw` off, which moves its
    // clock and builds no plan. The draw then reads each step back
    // (`_emit_effect_slot`).
    //
    // All per-slot effect lifecycle — event resolution, transition edges,
    // accumulator, dead-effect skip, simulator update, emitter resource
    // resolution, particle quad emission — runs inside ss_effect_slot_step.
    // The returned EffectDrawPlan carries everything Godot needs to draw:
    // commands keyed by cellmap_hash + blend, plus flat verts/uvs/colors/indices.
    // _currentAnimationData is null whenever _fetchAnimation bailed out; the
    // effect simulator still needs a sane frame rate to step with.
    const float fps = (_currentAnimationData && _currentAnimationData->fps() > 0)
                      ? (float)_currentAnimationData->fps() : 60.0f;
    const auto* parts = f.frameData->parts();
    for (uint32_t i = 0; i < _effect_slots.size(); i++) {
        EffectSlotState& slot = _effect_slots[i];
        if (!slot.effect_slot) continue;
        const ss_effect_event_info ev = ss_runtime_get_active_effect_event(runtime_ctx, i);
        // The runtime leaves a hidden part out of the draw order.
        const bool draw = parts && i < parts->size() && !parts->Get(i)->hide();
        const ss_effect_step_result step = ss_effect_slot_step(
            slot.effect_slot, ev, f.frame_no, f.delta_seconds, fps, f.parent_looped,
            /*y_flip*/ true, /*vert_stride*/ 2, draw);
        slot.step_visible = step.visible;
        slot.step_plan = step.draw_plan_buf;
    }
}

void SsInternalPlayer::_emit_effect_slot(const DrawFrame& f, RID ci, int p_idx, const float* slot_matrix) {
    if (p_idx < 0 || (uint32_t)p_idx >= _effect_slots.size()) return;
    EffectSlotState& slot = _effect_slots[p_idx];
    if (!slot.effect_slot) {
        f.rs->canvas_item_set_visible(ci, false);
        return;
    }

    if (!slot.step_visible) {
        f.rs->canvas_item_set_visible(ci, false);
        return;
    }
    f.rs->canvas_item_set_visible(ci, true);

    const auto* plan = ss::runtime::GetEffectDrawPlan(slot.step_plan);
    if (!plan) {
        f.rs->canvas_item_clear(ci);
        return;
    }

    f.rs->canvas_item_set_transform(ci, matrix_to_transform2d(slot_matrix));

    // Cascade the Effect slot part's alpha attribute down to all child
    // emitter CIs via modulate. The runtime / EffectDrawPlan only carries
    // per-particle simulator colors, so without this the part's own alpha
    // curve (e.g. a fade-out keyed on the Effect part itself) is ignored.
    const float part_alpha = _parts_by_idx[p_idx] ? _parts_by_idx[p_idx]->alpha() : 1.0f;
    f.rs->canvas_item_set_modulate(ci, Color(1, 1, 1, part_alpha));

    const uint32_t cmd_count = plan->commands()->size();


    // PackedVector2Array stores Vector2 = (real_t, real_t). The plan emits
    // flat float pairs (vert_stride == 2), so cast the storage as float* —
    // only valid when real_t == float.
    static_assert(sizeof(Vector2) == 2 * sizeof(float),
                  "EffectDrawPlan vert_stride=2 requires Godot built with real_t == float");
    // The plan's colours are copied into Color storage as float quadruples, and
    // the draw reads them back the same way.
    static_assert(sizeof(Color) == 4 * sizeof(float), "the colour stream is read as RGBA floats");

    const float* V = plan->verts()->data();
    const float* U = plan->uvs()->data();
    const float* C = plan->colors()->data();
    const int32_t* I = plan->indices()->data();

    uint32_t drawn_cis = 0;

    for (uint32_t e_idx = 0; e_idx < cmd_count; ) {
        const auto* cmd = plan->commands()->Get(e_idx);
        if (!cmd || cmd->quad_count() == 0) {
            e_idx++;
            continue;
        }

        const uint32_t cellmap_hash = cmd->cellmap_hash();
        if (cellmap_hash == 0 || !_textures.has(cellmap_hash)) {
            e_idx++;
            continue;
        }
        Ref<Texture2D> tex = _textures[cellmap_hash];
        if (tex.is_null()) {
            e_idx++;
            continue;
        }

        uint32_t merge_qc = cmd->quad_count();
        uint32_t merge_end = e_idx + 1;
        while (merge_end < cmd_count) {
            const auto* next_cmd = plan->commands()->Get(merge_end);
            if (next_cmd && next_cmd->quad_count() > 0 &&
                next_cmd->cellmap_hash() == cellmap_hash &&
                next_cmd->blend() == cmd->blend()) {
                merge_qc += next_cmd->quad_count();
                merge_end++;
            } else {
                break;
            }
        }

        while ((uint32_t)slot.emitter_cis.size() <= drawn_cis) {
            RID e_ci = f.rs->canvas_item_create();
            f.rs->canvas_item_set_parent(e_ci, ci);
            _apply_canvas_item_defaults(e_ci);
            slot.emitter_cis.push_back(e_ci);
            slot.emitter_meshes.push_back(SsDynamicMesh());
        }

        RID e_ci = slot.emitter_cis[drawn_cis];
        f.rs->canvas_item_set_parent(e_ci, ci);
        f.rs->canvas_item_clear(e_ci);
        f.rs->canvas_item_set_draw_index(e_ci, (int)drawn_cis);
        f.rs->canvas_item_set_visible(e_ci, true);

        const ss::format::BlendType blend = (cmd->blend() == 1) ? ss::format::BlendType_Add : ss::format::BlendType_Mix;
        _apply_blend_material(f.rs, e_ci, blend);

        // Grown, never shrunk: the draw below takes the counts, not the sizes.
        if (_effect_verts.size() < (int)merge_qc * 4) {
            _effect_verts.resize(merge_qc * 4);
            _effect_uvs.resize(merge_qc * 4);
            _effect_colors.resize(merge_qc * 4);
            _effect_indices.resize(merge_qc * 6);
        }
        SsVec2Array&  p_verts   = _effect_verts;
        SsVec2Array&  p_uvs     = _effect_uvs;
        SsColorArray& p_colors  = _effect_colors;
        SsIntArray&   p_indices = _effect_indices;

        uint32_t dest_qc = 0;
        for (uint32_t m = e_idx; m < merge_end; m++) {
            const auto* m_cmd = plan->commands()->Get(m);
            uint32_t qc = m_cmd->quad_count();
            uint32_t off = m_cmd->particle_offset();
            
            memcpy(p_verts.ptrw() + dest_qc * 4,   V + off * 8,  qc * 8  * sizeof(float));
            memcpy(p_uvs.ptrw() + dest_qc * 4,     U + off * 8,  qc * 8  * sizeof(float));
            memcpy(p_colors.ptrw() + dest_qc * 4,  C + off * 16, qc * 16 * sizeof(float));
            
            int32_t* dst_idx = p_indices.ptrw() + dest_qc * 6;
            const int32_t* src_idx = I + off * 6;
            int32_t v_offset = dest_qc * 4;
            for (uint32_t i = 0; i < qc * 6; i++) {
                dst_idx[i] = src_idx[i] + v_offset;
            }

            dest_qc += qc;
        }

        SsMeshStreams streams;
        streams.verts = p_verts.ptr();
        streams.uvs = p_uvs.ptr();
        streams.colors = (const float*)p_colors.ptr();
        streams.vertex_count = (int)merge_qc * 4;
        streams.indices = p_indices.ptr();
        streams.index_count = (int)merge_qc * 6;
        _draw_dynamic_mesh(f.rs, slot.emitter_meshes[drawn_cis], e_ci, streams, tex->get_rid());

        drawn_cis++;
        e_idx = merge_end;
    }

    for (int e = (int)drawn_cis; e < slot.emitter_cis.size(); e++) {
        f.rs->canvas_item_clear(slot.emitter_cis[e]);
        f.rs->canvas_item_set_visible(slot.emitter_cis[e], false);
        f.rs->canvas_item_set_parent(slot.emitter_cis[e], RID());
    }
}

int SsInternalPlayer::_build_normal(const DrawFrame& f, int p_idx,
                                    const ss::runtime::PartState* part,
                                    const float* draw_m,
                                    const Vector2& inv_tex_size,
                                    Vector2* verts_ptr,
                                    Vector2* uvs_ptr,
                                    Color* colors_ptr,
                                    float* custom0_ptr,
                                    int vbase)
{
    const float* part_cell_meta = f.get_cell_meta(p_idx);
    const float* part_uvs = f.get_local_uvs(p_idx);
    const float* part_verts = f.get_local_vertices(p_idx);
    if (!part_cell_meta || !part_uvs || !part_verts) return 0;

    const uint64_t flags = part->update_flag();
    const bool needs_center = (flags & (ss::runtime::UpdateAttributeFlags_AttributeVertex | ss::runtime::UpdateAttributeFlags_AttributePartColor)) != 0;
    const int vert_count = needs_center ? MAX_VERTICES_COUNT : CORNERS_COUNT;

    const float out_x[MAX_VERTICES_COUNT] = { part_verts[0], part_verts[2], part_verts[4], part_verts[6], part_verts[8] };
    const float out_y[MAX_VERTICES_COUNT] = { part_verts[1], part_verts[3], part_verts[5], part_verts[7], part_verts[9] };
    const float out_u[MAX_VERTICES_COUNT] = { part_uvs[0], part_uvs[2], part_uvs[4], part_uvs[6], part_uvs[8] };
    const float out_v[MAX_VERTICES_COUNT] = { part_uvs[1], part_uvs[3], part_uvs[5], part_uvs[7], part_uvs[9] };

    // Default (no PartColor): white modulate, part alpha only, rate=0 makes
    // the shader formula reduce to pixel pass-through regardless of blend_idx.
    const float part_alpha = part->alpha();
    Color corner_colors[CORNERS_COUNT] = {
        Color(1, 1, 1, part_alpha), Color(1, 1, 1, part_alpha),
        Color(1, 1, 1, part_alpha), Color(1, 1, 1, part_alpha)
    };
    float corner_rates[CORNERS_COUNT] = { 0.0f, 0.0f, 0.0f, 0.0f };
    int blend_idx = 0;

    const auto partColorIndex = part->part_color();
    if ((flags & ss::runtime::UpdateAttributeFlags_AttributePartColor) && partColorIndex >= 0 && f.frameData->parts_color() != nullptr) {
        auto pc = f.frameData->parts_color()->Get(partColorIndex);
        // SDK converter has applied SS6-style mediation; ssab fields now carry
        // a consistent semantic regardless of the source (blend_type, target):
        //   pc->lt/rt/lb/rb->rgba.a  = rateAlpha (final-alpha source, u8)
        //   pc->lt/rt/lb/rb->rate    = colorA    (blend weight, float)
        auto to_color = [](const ss::runtime::SsAttributePartColorKeyValueColor& c) {
            return Color(c.rgba().r()/255.0f, c.rgba().g()/255.0f, c.rgba().b()/255.0f,
                         c.rgba().a()/255.0f);
        };
        corner_colors[0] = to_color(pc->lt());
        corner_colors[1] = to_color(pc->rt());
        corner_colors[2] = to_color(pc->lb());
        corner_colors[3] = to_color(pc->rb());
        corner_rates[0] = pc->lt().rate();
        corner_rates[1] = pc->rt().rate();
        corner_rates[2] = pc->lb().rate();
        corner_rates[3] = pc->rb().rate();
        // PartColor blend_type is 0..3 (Mix/Mul/Add/Sub). Higher SpriteStudio values are
        // GPU framebuffer blends, not PartColor formulas — clamp defensively.
        blend_idx = (int)pc->blend_type();
        if (blend_idx < 0 || blend_idx > 3) blend_idx = 0;
    }

    // Output premultiplied-alpha flag. Held at 0 (disabled); the shader path
    // reads this per-vertex so PMA can be enabled globally or per texture.
    const float pma_flag = 0.0f;

    for (int j = 0; j < CORNERS_COUNT; j++) {
        verts_ptr[vbase + j] = xform_raw(draw_m, out_x[j], out_y[j]);
        uvs_ptr[vbase + j] = Vector2(out_u[j] * inv_tex_size.x, out_v[j] * inv_tex_size.y);
        colors_ptr[vbase + j] = corner_colors[j];
        const int c0 = (vbase + j) * 4;
        custom0_ptr[c0 + 0] = corner_rates[j];
        custom0_ptr[c0 + 1] = (float)blend_idx;
        custom0_ptr[c0 + 2] = pma_flag;
        custom0_ptr[c0 + 3] = 0.0f;
    }
    if (needs_center) {
        verts_ptr[vbase + CORNERS_COUNT] = xform_raw(draw_m, out_x[CORNERS_COUNT], out_y[CORNERS_COUNT]);
        uvs_ptr[vbase + CORNERS_COUNT] = Vector2(out_u[CORNERS_COUNT] * inv_tex_size.x, out_v[CORNERS_COUNT] * inv_tex_size.y);
        colors_ptr[vbase + CORNERS_COUNT] = (corner_colors[0] + corner_colors[1] + corner_colors[2] + corner_colors[3]) * 0.25f;
        const float center_rate = (corner_rates[0] + corner_rates[1] + corner_rates[2] + corner_rates[3]) * 0.25f;
        const int c0 = (vbase + CORNERS_COUNT) * 4;
        custom0_ptr[c0 + 0] = center_rate;
        custom0_ptr[c0 + 1] = (float)blend_idx;
        custom0_ptr[c0 + 2] = pma_flag;
        custom0_ptr[c0 + 3] = 0.0f;
    }
    return vert_count;
}

void SsInternalPlayer::_apply_blend_material(RenderingServer* rs, RID ci, ss::format::BlendType ss_blend) {
    if (!_blend_materials.has((int)ss_blend)) {
        Ref<CanvasItemMaterial> mat; mat.instantiate();
        switch (ss_blend) {
            case ss::format::BlendType_Mix: mat->set_blend_mode(CanvasItemMaterial::BLEND_MODE_MIX); break;
            case ss::format::BlendType_Add: mat->set_blend_mode(CanvasItemMaterial::BLEND_MODE_ADD); break;
            case ss::format::BlendType_Sub: mat->set_blend_mode(CanvasItemMaterial::BLEND_MODE_SUB); break;
            case ss::format::BlendType_Mul: mat->set_blend_mode(CanvasItemMaterial::BLEND_MODE_MUL); break;
            default: mat->set_blend_mode(CanvasItemMaterial::BLEND_MODE_MIX); break;
        }
        _blend_materials[(int)ss_blend] = mat;
    }
    rs->canvas_item_set_material(ci, _blend_materials[(int)ss_blend]->get_rid());
}

Ref<Shader> SsInternalPlayer::_ensure_partcolor_shader(uint32_t shader_id_hash, ss::format::BlendType blend_type) {
    const uint64_t key = make_partcolor_cache_key(shader_id_hash, blend_type);
    if (_partcolor_shaders.has(key)) {
        return _partcolor_shaders[key];
    }
    if (!s_shader_catalog_map.has(shader_id_hash)) {
        shader_id_hash = s_default_shader_id_hash;
    }
    const EmbeddedShader& variant = s_shader_catalog_map[shader_id_hash];
    Ref<Shader> shader; shader.instantiate();
    String src = String(SHADER_HEADER)
               + String(partcolor_render_mode_str((int)blend_type))
               + String(LIBRARY_VS)
               + String(LIBRARY_FS)
               + String(DEFAULT_VS)
               + String(variant.fs_source);
    shader->set_code(src);
    _partcolor_shaders[key] = shader;
    return shader;
}

void SsInternalPlayer::_reset_per_part_pools() {
    RenderingServer* rs = RenderingServer::get_singleton();
    // Pool stays warm across frames; the in_use cursor walks back to 0 so
    // previously-handed-out materials are re-issued from the front.
    for (KeyValue<uint64_t, PerPartMaterialPool>& kv : _per_part_material_pools) {
        kv.value.in_use = 0;
    }
    // Hide all canvas items by default; `_acquire_per_part_canvas_item` makes
    // re-acquired entries visible. Unused entries this frame remain hidden.
    for (int i = 0; i < _per_part_canvas_items.size(); i++) {
        rs->canvas_item_clear(_per_part_canvas_items[i]);
        rs->canvas_item_set_visible(_per_part_canvas_items[i], false);
    }
    _per_part_canvas_items_in_use = 0;
}

void SsInternalPlayer::_free_per_part_canvas_items() {
    RenderingServer* rs = RenderingServer::get_singleton();
    for (int i = 0; i < _per_part_canvas_items.size(); i++) {
        rs->free_rid(_per_part_canvas_items[i]);
    }
    _per_part_canvas_items.clear();
    _per_part_canvas_items_in_use = 0;
}

Ref<ShaderMaterial> SsInternalPlayer::_acquire_per_part_material(uint32_t shader_id_hash, ss::format::BlendType blend_type) {
    const ss::format::BlendType resolved = gpu_blend_for(blend_type);
    const uint64_t key = make_partcolor_cache_key(shader_id_hash, resolved);
    PerPartMaterialPool& pool = _per_part_material_pools[key];
    if (pool.in_use >= pool.materials.size()) {
        Ref<ShaderMaterial> mat; mat.instantiate();
        mat->set_shader(_ensure_partcolor_shader(shader_id_hash, resolved));
        pool.materials.push_back(mat);
    }
    return pool.materials[pool.in_use++];
}

RID SsInternalPlayer::_acquire_per_part_canvas_item() {
    RenderingServer* rs = RenderingServer::get_singleton();
    if (_per_part_canvas_items_in_use >= _per_part_canvas_items.size()) {
        RID ci = rs->canvas_item_create();
        rs->canvas_item_set_parent(ci, _root_ci);
        _apply_canvas_item_defaults(ci);
        _per_part_canvas_items.push_back(ci);
    }
    RID ci = _per_part_canvas_items[_per_part_canvas_items_in_use++];
    rs->canvas_item_set_visible(ci, true);
    rs->canvas_item_set_draw_index(ci, _draw_seq++);
    return ci;
}

Ref<Texture2D> SsInternalPlayer::_resolve_map_texture(uint32_t cellmap_name_hash) {
    if (cellmap_name_hash == 0) return Ref<Texture2D>();
    if (!_textures.has(cellmap_name_hash)) return Ref<Texture2D>();
    return _textures[cellmap_name_hash];
}

Vector4 SsInternalPlayer::_resolve_cell_rect_uv(const DrawFrame& f, int p_idx, const Vector2& inv_tex_size) {
    if (!f.cell_meta || (uintptr_t)p_idx * 6 + 6 > f.cell_meta_len) {
        return Vector4(0, 0, 0, 0);
    }
    const float* m = f.cell_meta + (p_idx * 6);
    // cell_meta layout: [pivot_x, pivot_y, size_w, size_h, rect_left, rect_top].
    // See ssruntime/src/core/framedata.rs::evaluate_layer (cell_meta_per_part write).
    const float size_w    = m[2];
    const float size_h    = m[3];
    const float rect_left = m[4];
    const float rect_top  = m[5];
    return Vector4(
        rect_left * inv_tex_size.x,
        rect_top  * inv_tex_size.y,
        (rect_left + size_w) * inv_tex_size.x,
        (rect_top  + size_h) * inv_tex_size.y
    );
}

SsInternalPlayer::PartShaderInfo SsInternalPlayer::_resolve_part_shader_info(const DrawFrame& f, const ss::runtime::PartState* part) {
    PartShaderInfo psi = {};
    psi.id_hash = s_default_shader_id_hash;
    if (part) {
        const uint64_t flags = part->update_flag();
        const int16_t shader_idx = part->shader();
        if ((flags & ss::runtime::UpdateAttributeFlags_AttributeShader) && shader_idx >= 0
            && f.frameData->shaders() != nullptr
            && shader_idx < (int16_t)f.frameData->shaders()->size()) {
            auto sh = f.frameData->shaders()->Get(shader_idx);
            psi.id_hash = sh->id_hash();
            psi.params[0] = sh->param0();
            psi.params[1] = sh->param1();
            psi.params[2] = sh->param2();
            psi.params[3] = sh->param3();
            psi.params[4] = sh->param4();
            psi.params[5] = sh->param5();
            psi.params[6] = sh->param6();
            psi.params[7] = sh->param7();
            psi.map0 = _resolve_map_texture(sh->map0_cellmap_name_hash());
            psi.map1 = _resolve_map_texture(sh->map1_cellmap_name_hash());
        }
    }
    if (!s_shader_catalog_map.has(psi.id_hash)) {
        psi.id_hash = s_default_shader_id_hash;
    }
    const EmbeddedShader& variant = s_shader_catalog_map[psi.id_hash];
    psi.is_per_part = variant.is_per_part;
    return psi;
}

void SsInternalPlayer::_apply_per_part_uniforms(Ref<ShaderMaterial> mat, const float params[8],
                                                const Ref<Texture2D>& map0, const Ref<Texture2D>& map1,
                                                const Vector4& cell_rect) {
    if (mat.is_null() || params == nullptr) return;
    // Uniform names match the declarations in shaders/ss_library_fs.glsl.
    // Setting an unknown uniform is harmless in Godot — it just no-ops.
    mat->set_shader_parameter("ss_param0", params[0]);
    mat->set_shader_parameter("ss_param1", params[1]);
    mat->set_shader_parameter("ss_param2", params[2]);
    mat->set_shader_parameter("ss_param3", params[3]);
    mat->set_shader_parameter("ss_param4", params[4]);
    mat->set_shader_parameter("ss_param5", params[5]);
    mat->set_shader_parameter("ss_param6", params[6]);
    mat->set_shader_parameter("ss_param7", params[7]);
    mat->set_shader_parameter("map0", map0);
    mat->set_shader_parameter("map1", map1);
    mat->set_shader_parameter("ss_cell_rect", cell_rect);
}

void SsInternalPlayer::_apply_partcolor_material(RenderingServer* rs, RID ci, uint32_t shader_id_hash, ss::format::BlendType ss_blend) {
    // The per-vertex CUSTOM0 drives PartColor compositing whatever the blend.
    const ss::format::BlendType resolved = gpu_blend_for(ss_blend);
    const uint64_t key = make_partcolor_cache_key(shader_id_hash, resolved);
    if (!_partcolor_materials.has(key)) {
        Ref<ShaderMaterial> mat; mat.instantiate();
        mat->set_shader(_ensure_partcolor_shader(shader_id_hash, resolved));
        _partcolor_materials[key] = mat;
    }
    rs->canvas_item_set_material(ci, _partcolor_materials[key]->get_rid());
}

void SsInternalPlayer::_emit_partcolor_mesh(RenderingServer* rs, RID ci,
                                            const int32_t* indices, int index_count,
                                            const Vector2* verts, const Color* colors,
                                            const Vector2* uvs, const float* custom0,
                                            int vertex_count, const RID& texture_rid) {
    // The PartColor goes in CUSTOM1 and there is no COLOR stream, so the
    // shader's COLOR is the canvas item's modulate alone (see ss_shader_setup.h).
    // Quantised the way the engine quantises ARRAY_COLOR -- to 8 bits,
    // truncated, with the product in double -- so a part draws exactly as it
    // did when the colour rode in COLOR. Carried as floats rather than as
    // RGBA8_UNORM, which would match ARRAY_COLOR byte for byte: the
    // Compatibility renderer does not read an RGBA8 custom stream back
    // correctly, and part colours lose their green and blue.
    if (_surface_custom1.size() < vertex_count * 4) {
        _surface_custom1.resize(vertex_count * 4);
    }
    float* dst = _surface_custom1.ptrw();
    for (int i = 0; i < vertex_count; i++) {
        const float channels[4] = { colors[i].r, colors[i].g, colors[i].b, colors[i].a };
        for (int c = 0; c < 4; c++) {
            dst[i * 4 + c] = float(uint8_t(CLAMP(channels[c] * 255.0, 0.0, 255.0))) / 255.0f;
        }
    }

    if (_mesh_pool_in_use >= _mesh_pool.size()) {
        _mesh_pool.push_back(SsDynamicMesh());
    }
    SsMeshStreams streams;
    streams.verts = verts;
    streams.uvs = uvs;
    streams.custom0 = custom0;
    streams.custom1 = _surface_custom1.ptr();
    streams.vertex_count = vertex_count;
    streams.indices = indices;
    streams.index_count = index_count;
    _draw_dynamic_mesh(rs, _mesh_pool[_mesh_pool_in_use++], ci, streams, texture_rid);
}

void SsInternalPlayer::_draw_dynamic_mesh(RenderingServer* rs, SsDynamicMesh& m, RID ci,
                                          const SsMeshStreams& s, const RID& texture_rid) {
    if (s.vertex_count <= 0 || s.index_count <= 0) {
        return;
    }
    uint64_t format = Mesh::ARRAY_FORMAT_VERTEX | Mesh::ARRAY_FORMAT_TEX_UV | Mesh::ARRAY_FORMAT_INDEX
                    | Mesh::ARRAY_FLAG_USE_2D_VERTICES | Mesh::ARRAY_FLAG_USE_DYNAMIC_UPDATE;
    if (s.colors) {
        format |= Mesh::ARRAY_FORMAT_COLOR;
    }
    if (s.custom0) {
        format |= Mesh::ARRAY_FORMAT_CUSTOM0 | ((uint64_t)Mesh::ARRAY_CUSTOM_RGBA_FLOAT << Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT);
    }
    if (s.custom1) {
        format |= Mesh::ARRAY_FORMAT_CUSTOM1 | ((uint64_t)Mesh::ARRAY_CUSTOM_RGBA_FLOAT << Mesh::ARRAY_FORMAT_CUSTOM1_SHIFT);
    }

    if (!m.mesh.is_valid() || m.format != format
        || s.vertex_count > m.vertex_capacity || s.index_count > m.index_capacity) {
        // Half as much again each time, so a count that keeps creeping up -- an
        // effect whose particles are still being born -- rebuilds a few times
        // rather than on every frame.
        const bool grow = m.mesh.is_valid() && m.format == format;
        const int grown_vertices = grow ? m.vertex_capacity + m.vertex_capacity / 2 : 0;
        const int grown_indices = grow ? m.index_capacity + m.index_capacity / 2 : 0;
        const int vertex_capacity = s.vertex_count > grown_vertices ? s.vertex_count : grown_vertices;
        int index_capacity = s.index_count > grown_indices ? s.index_count : grown_indices;
        index_capacity += (3 - index_capacity % 3) % 3;

        // The surface is created empty and filled by the write below, the same
        // write every other frame makes. Zero indices are what the write leaves
        // past the frame's own, too: triangles with no area, which draw nothing.
        SsVec2Array zero_vec2;
        zero_vec2.resize(vertex_capacity);
        SsIntArray zero_indices;
        zero_indices.resize(index_capacity);
        zero_indices.fill(0);
        Array arrays;
        arrays.resize(Mesh::ARRAY_MAX);
        arrays[Mesh::ARRAY_VERTEX] = zero_vec2;
        arrays[Mesh::ARRAY_TEX_UV] = zero_vec2;
        arrays[Mesh::ARRAY_INDEX] = zero_indices;
        if (s.colors) {
            SsColorArray zero_colors;
            zero_colors.resize(vertex_capacity);
            arrays[Mesh::ARRAY_COLOR] = zero_colors;
        }
        if (s.custom0 || s.custom1) {
            SsFloatArray zero_custom;
            zero_custom.resize(vertex_capacity * 4);
            zero_custom.fill(0.0f);
            if (s.custom0) arrays[Mesh::ARRAY_CUSTOM0] = zero_custom;
            if (s.custom1) arrays[Mesh::ARRAY_CUSTOM1] = zero_custom;
        }
        if (!m.mesh.is_valid()) {
            m.mesh = rs->mesh_create();
        }
        rs->mesh_clear(m.mesh);
        rs->mesh_add_surface_from_arrays(m.mesh, RS_PRIMITIVE_TRIANGLES, arrays, Array(), Dictionary(), format);

        // The layout is the engine's to decide; ask it rather than assume one.
        m.format = format;
        m.vertex_capacity = vertex_capacity;
        m.index_capacity = index_capacity;
        m.vertex_stride = rs->mesh_surface_get_format_vertex_stride(format, vertex_capacity);
        m.attribute_stride = rs->mesh_surface_get_format_attribute_stride(format, vertex_capacity);
        m.index_stride = rs->mesh_surface_get_format_index_stride(format, vertex_capacity);
        m.uv_offset = rs->mesh_surface_get_format_offset(format, vertex_capacity, Mesh::ARRAY_TEX_UV);
        m.color_offset = s.colors ? rs->mesh_surface_get_format_offset(format, vertex_capacity, Mesh::ARRAY_COLOR) : 0;
        m.custom0_offset = s.custom0 ? rs->mesh_surface_get_format_offset(format, vertex_capacity, Mesh::ARRAY_CUSTOM0) : 0;
        m.custom1_offset = s.custom1 ? rs->mesh_surface_get_format_offset(format, vertex_capacity, Mesh::ARRAY_CUSTOM1) : 0;
        m.vertex_bytes.resize(vertex_capacity * m.vertex_stride);
        m.attribute_bytes.resize(vertex_capacity * m.attribute_stride);
        m.index_bytes.resize(index_capacity * m.index_stride);
        m.vertex_bytes.fill(0);
        m.attribute_bytes.fill(0);
    }

    // Vertices past `vertex_count` keep what an earlier frame wrote; no index
    // reaches them.
    uint8_t* vertex_out = m.vertex_bytes.ptrw();
    uint8_t* attribute_out = m.attribute_bytes.ptrw();
    Vector2 aabb_min = s.verts[0];
    Vector2 aabb_max = s.verts[0];
    for (int i = 0; i < s.vertex_count; i++) {
        const Vector2 v = s.verts[i];
        aabb_min = aabb_min.min(v);
        aabb_max = aabb_max.max(v);
        const float position[2] = { (float)v.x, (float)v.y };
        memcpy(vertex_out + i * m.vertex_stride, position, sizeof(position));
        uint8_t* attribute = attribute_out + i * m.attribute_stride;
        const float uv[2] = { (float)s.uvs[i].x, (float)s.uvs[i].y };
        memcpy(attribute + m.uv_offset, uv, sizeof(uv));
        if (s.colors) {
            // Quantised the way the engine quantises an ARRAY_COLOR it is handed.
            const float* c = s.colors + i * 4;
            const uint8_t color8[4] = {
                uint8_t(CLAMP(c[0] * 255.0, 0.0, 255.0)), uint8_t(CLAMP(c[1] * 255.0, 0.0, 255.0)),
                uint8_t(CLAMP(c[2] * 255.0, 0.0, 255.0)), uint8_t(CLAMP(c[3] * 255.0, 0.0, 255.0))
            };
            memcpy(attribute + m.color_offset, color8, sizeof(color8));
        }
        if (s.custom0) {
            memcpy(attribute + m.custom0_offset, s.custom0 + i * 4, 4 * sizeof(float));
        }
        if (s.custom1) {
            memcpy(attribute + m.custom1_offset, s.custom1 + i * 4, 4 * sizeof(float));
        }
    }
    uint8_t* index_out = m.index_bytes.ptrw();
    if (m.index_stride == 2) {
        uint16_t* index16 = (uint16_t*)index_out;
        for (int i = 0; i < s.index_count; i++) {
            index16[i] = (uint16_t)s.indices[i];
        }
    } else {
        memcpy(index_out, s.indices, s.index_count * sizeof(int32_t));
    }
    memset(index_out + s.index_count * m.index_stride, 0, (m.index_capacity - s.index_count) * m.index_stride);

    rs->mesh_surface_update_vertex_region(m.mesh, 0, 0, m.vertex_bytes);
    rs->mesh_surface_update_attribute_region(m.mesh, 0, 0, m.attribute_bytes);
    rs->mesh_surface_update_index_region(m.mesh, 0, 0, m.index_bytes);
    // The surface's own bounds are those of the empty geometry it was created
    // with; the canvas culls the item by these instead.
    rs->mesh_set_custom_aabb(m.mesh, AABB(Vector3(aabb_min.x, aabb_min.y, 0.0f),
                                          Vector3(aabb_max.x - aabb_min.x, aabb_max.y - aabb_min.y, 0.0f)));
    rs->canvas_item_add_mesh(ci, m.mesh, Transform2D(), Color(1, 1, 1, 1), texture_rid);
}

bool SsInternalPlayer::_build_shape_geometry(const DrawFrame& f, int p_idx,
                                             const ss::runtime::PartState* part,
                                             const float* draw_m,
                                             ShapeGeometryBuffers& out) {
    const float* part_shape_verts = f.get_shape_vertices(p_idx);
    const float* part_shape_box_coords = f.get_shape_box_coords(p_idx);
    int32_t part_shape_count = f.get_shape_vertex_count(p_idx);
    if (!part_shape_verts || !part_shape_box_coords || part_shape_count < 3) return false;

    out.vert_count = part_shape_count;

    const uint64_t flags = part->update_flag();
    const float part_alpha = part->alpha();
    // Default (no PartColor): white tint, part alpha only, rate=0 — the
    // shader formula collapses to "pixel pass-through" which means white
    // (the default sampler's value) for shapes. blend_idx is irrelevant in
    // that case.
    Color corner_colors[CORNERS_COUNT] = {
        Color(1, 1, 1, part_alpha), Color(1, 1, 1, part_alpha),
        Color(1, 1, 1, part_alpha), Color(1, 1, 1, part_alpha)
    };
    float corner_rates[CORNERS_COUNT] = { 0.0f, 0.0f, 0.0f, 0.0f };
    int blend_idx = 0;

    const auto partColorIndex = part->part_color();
    if ((flags & ss::runtime::UpdateAttributeFlags_AttributePartColor) && partColorIndex >= 0 && f.frameData->parts_color() != nullptr) {
        auto pc = f.frameData->parts_color()->Get(partColorIndex);
        // SDK-mediated semantic — see _build_normal for the field meanings.
        // Shape paths additionally get the (blend_type == Mix → rgba.a = 255)
        // override applied at convert time.
        auto to_color = [](const ss::runtime::SsAttributePartColorKeyValueColor& c) {
            return Color(c.rgba().r()/255.0f, c.rgba().g()/255.0f, c.rgba().b()/255.0f,
                         c.rgba().a()/255.0f);
        };
        corner_colors[0] = to_color(pc->lt());
        corner_colors[1] = to_color(pc->rt());
        corner_colors[2] = to_color(pc->lb());
        corner_colors[3] = to_color(pc->rb());
        // Shape parts intentionally skip the PartColor compositing formula —
        // the spec says "per-vertex color = pc.rgba directly, no calculation".
        // Force the shader's per-vertex inputs to Mix mode with rate=1.0 so
        // ss_partcolor_blend(white, pc.rgb, varg) collapses to pc.rgb. The
        // original pc->blend_type() is preserved in ssab for inspection; the
        // GPU framebuffer blend comes from batch->blend_type() (= the part's
        // alpha_blend_type, set by the artist).
        for (int i = 0; i < CORNERS_COUNT; i++) corner_rates[i] = 1.0f;
        blend_idx = 0;
    }

    const float pma_flag = 0.0f;

    if (out.verts.size() < part_shape_count) {
        out.verts.resize(part_shape_count);
        out.uvs.resize(part_shape_count);
        out.colors.resize(part_shape_count);
        out.custom0.resize(part_shape_count * 4);
    }
    Vector2* v_ptr = out.verts.ptrw();
    Vector2* u_ptr = out.uvs.ptrw();
    Color*   c_ptr = out.colors.ptrw();
    float*   x_ptr = out.custom0.ptrw();

    for (int i = 0; i < part_shape_count; i++) {
        const float vx = part_shape_verts[i * 2 + 0];
        const float vy = part_shape_verts[i * 2 + 1];
        const float fx = part_shape_box_coords[i * 2 + 0];
        const float fy = part_shape_box_coords[i * 2 + 1];
        const float wLT = (1.0f - fx) * (1.0f - fy);
        const float wRT =          fx * (1.0f - fy);
        const float wLB = (1.0f - fx) *          fy;
        const float wRB =          fx *          fy;
        c_ptr[i] = corner_colors[0] * wLT + corner_colors[1] * wRT + corner_colors[2] * wLB + corner_colors[3] * wRB;
        v_ptr[i] = xform_raw(draw_m, vx, vy);
        // UV is unused — the shape has no bound texture; the canvas_item
        // shader's default sampler returns white for any UV.
        u_ptr[i] = Vector2(0.0f, 0.0f);
        const float v_rate = corner_rates[0] * wLT + corner_rates[1] * wRT + corner_rates[2] * wLB + corner_rates[3] * wRB;
        x_ptr[i*4 + 0] = v_rate;
        x_ptr[i*4 + 1] = (float)blend_idx;
        x_ptr[i*4 + 2] = pma_flag;
        x_ptr[i*4 + 3] = 0.0f;
    }

    // All shapes use a center-first TRIANGLE_FAN ([center, perimeter...,
    // closing]), so a deformed / per-corner-coloured shape interpolates from
    // its center instead of across a diagonal seam.
    const int tri_count = part_shape_count - 2;
    out.index_count = tri_count * 3;
    if (out.indices.size() < out.index_count) {
        out.indices.resize(out.index_count);
    }
    int32_t* i_ptr = (int32_t*)out.indices.ptrw();
    for (int i = 0; i < tri_count; i++) {
        i_ptr[i*3 + 0] = 0;
        i_ptr[i*3 + 1] = i + 1;
        i_ptr[i*3 + 2] = i + 2;
    }
    return true;
}

void SsInternalPlayer::_emit_normal_batch(const DrawFrame& f,
                                          const ss::runtime::DrawBatch* batch,
                                          const uint16_t* draw_order_data) {
    if (!batch || !draw_order_data) return;
    const uint16_t count = batch->count();
    if (count == 0) return;

    RenderingServer* rs = f.rs;

    // The whole Normal batch shares `texture_hash` (batch invariant). Resolve
    // once here; the per-part build helper takes `tex_size` for UV
    // normalisation, no per-part texture lookup is performed.
    Ref<Texture2D> tex;
    if (_textures.has(batch->texture_hash())) {
        tex = _textures[batch->texture_hash()];
    }
    if (tex.is_null()) return;
    const Vector2 tex_size = tex->get_size();
    const Vector2 inv_tex_size = Vector2(1.0f / tex_size.x, 1.0f / tex_size.y);

    // batch->blend_type() is the runtime BlendType; converted to ssab BlendType
    // by raw u8 value (the two enums are layout-equivalent).
    const auto ssab_blend = (ss::format::BlendType)batch->blend_type();
    const RID tex_rid = tex->get_rid();

    // Pre-size the per-part scratch buffers to fit one Normal part at max
    // (5 verts, 12 indices). Resized once here, reused across per-part emits.
    if (_per_part_normal_verts.size() < MAX_VERTICES_COUNT) {
        _per_part_normal_verts.resize(MAX_VERTICES_COUNT);
    }
    if (_per_part_normal_uvs.size() < MAX_VERTICES_COUNT) {
        _per_part_normal_uvs.resize(MAX_VERTICES_COUNT);
    }
    if (_per_part_normal_colors.size() < MAX_VERTICES_COUNT) {
        _per_part_normal_colors.resize(MAX_VERTICES_COUNT);
    }
    if (_per_part_normal_custom0.size() < MAX_VERTICES_COUNT * 4) {
        _per_part_normal_custom0.resize(MAX_VERTICES_COUNT * 4);
    }
    if (_per_part_normal_indices.size() < INDICES_COUNT_PENTAGON) {
        _per_part_normal_indices.resize(INDICES_COUNT_PENTAGON);
    }

    for (uint16_t k = 0; k < count; k++) {
        int p_idx = (int)draw_order_data[batch->start_rank() + k];
        if (p_idx < 0 || p_idx >= (int)_parts_by_idx.size()) continue;
        // Pure masks feed the coverage bitmap only; drawing their colour would
        // paint the mask art over the scene instead of cutting a hole in it.
        if (_is_pure_mask_part(p_idx)) continue;
        const auto* part = _parts_by_idx[p_idx];
        if (!part) continue;

        const float* drawing_m = f.get_world_matrix(p_idx);
        if (!drawing_m) continue;

        // CBP masking: parts inside an active writer's scope must run the mask
        // test, which needs per-part uniforms (rank / polarity) — so force the
        // per-part path for them even when their shader is otherwise batchable.
        // visible_inside_mask parts draw ONLY inside a mask, so they must run the
        // test even when out of scope. A part opts OUT of masking entirely with
        // mask_influence=0 (SS "not affected by mask"); write_mask clipping parts
        // are always treated as affected (their mask_influence encodes the op).
        const uint16_t rank = (uint16_t)(batch->start_rank() + k);
        const PartMaskDecision md = _resolve_part_mask(f, p_idx, rank);
        const bool masked = md.masked;

        PartShaderInfo psi = _resolve_part_shader_info(f, part);
        if (psi.is_per_part || masked) {
            // Draw the default parts gathered ahead of this per-part part first,
            // so they keep a lower (behind) draw_index than it — this per-part
            // CI is acquired (and indexed) right below. Without it the run's
            // parts would all sink behind the batch's per-part parts, which hid
            // the rank-1 "face" behind the rank-0 shader-driven "head".
            _flush_default_run(rs);
            // Per-part path: build into the small scratch buffers (vbase=0),
            // acquire dedicated canvas_item + ShaderMaterial, emit one mesh.
            Vector2* p_verts_ptr = _per_part_normal_verts.ptrw();
            Vector2* p_uvs_ptr = _per_part_normal_uvs.ptrw();
            Color* p_colors_ptr = _per_part_normal_colors.ptrw();
            float* p_custom0_ptr = _per_part_normal_custom0.ptrw();

            const int vc = _build_normal(f, p_idx, part, drawing_m, inv_tex_size,
                                         p_verts_ptr, p_uvs_ptr,
                                         p_colors_ptr, p_custom0_ptr, 0);
            if (vc == 0) continue;
            int32_t* iptr = (int32_t*)_per_part_normal_indices.ptrw();
            int idx_count;
            if (vc == MAX_VERTICES_COUNT) {
                const int p[INDICES_COUNT_PENTAGON] = { 0,1,4, 1,3,4, 3,2,4, 2,0,4 };
                for (int j = 0; j < INDICES_COUNT_PENTAGON; j++) iptr[j] = p[j];
                idx_count = INDICES_COUNT_PENTAGON;
            } else {
                const int q[INDICES_COUNT_QUAD] = { 0,1,2, 1,3,2 };
                for (int j = 0; j < INDICES_COUNT_QUAD; j++) iptr[j] = q[j];
                idx_count = INDICES_COUNT_QUAD;
            }

            RID part_ci = _acquire_per_part_canvas_item();
            Ref<ShaderMaterial> mat = _acquire_per_part_material(psi.id_hash, ssab_blend);
            const Vector4 cell_rect = _resolve_cell_rect_uv(f, p_idx, inv_tex_size);
            _apply_per_part_uniforms(mat, psi.params, psi.map0, psi.map1, cell_rect);
            _stamp_part_mask(mat, rank, md);
            rs->canvas_item_set_material(part_ci, mat->get_rid());
            rs->canvas_item_set_transform(part_ci, Transform2D());
            _emit_partcolor_mesh(rs, part_ci, iptr, idx_count, p_verts_ptr, p_colors_ptr,
                                 p_uvs_ptr, p_custom0_ptr, vc, tex_rid);
            continue;
        }

        // Default / shared path: built straight into the default run.
        if (!_run.open) _run_begin(false, batch->texture_hash(), tex_rid, ssab_blend);
        _run_reserve(MAX_VERTICES_COUNT, INDICES_COUNT_PENTAGON);
        const int vbase = _run.vertex_count;
        const RunStreams& s = _run_streams;
        const int vert_count = _build_normal(f, p_idx, part, drawing_m, inv_tex_size,
                                             s.verts, s.uvs, s.colors, s.custom0, vbase);
        if (vert_count == 0) continue;

        // Append per-part indices with vertex base offset.
        int32_t* indices_ptr = s.indices + _run.index_count;
        if (vert_count == MAX_VERTICES_COUNT) {
            const int p[INDICES_COUNT_PENTAGON] = { 0,1,4, 1,3,4, 3,2,4, 2,0,4 };
            for (int j = 0; j < INDICES_COUNT_PENTAGON; j++) indices_ptr[j] = vbase + p[j];
            _run.index_count += INDICES_COUNT_PENTAGON;
        } else {
            const int q[INDICES_COUNT_QUAD] = { 0,1,2, 1,3,2 };
            for (int j = 0; j < INDICES_COUNT_QUAD; j++) indices_ptr[j] = vbase + q[j];
            _run.index_count += INDICES_COUNT_QUAD;
        }
        _run.vertex_count += vert_count;
    }
}

void SsInternalPlayer::_emit_shape_batch(const DrawFrame& f,
                                          const ss::runtime::DrawBatch* batch,
                                          const uint16_t* draw_order_data) {
    if (!batch || !draw_order_data) return;
    const uint16_t count = batch->count();
    if (count == 0) return;

    // batch->blend_type() is the runtime BlendType; converted to the ssab
    // BlendType by raw u8 value (same convention as _emit_normal_batch). The
    // shape pipeline now shares the PartColor shader with Normal/Mesh, so
    // the GPU framebuffer blend is selected via _apply_partcolor_material.
    const auto ssab_blend = (ss::format::BlendType)batch->blend_type();

    for (uint16_t k = 0; k < count; k++) {
        int p_idx = (int)draw_order_data[batch->start_rank() + k];
        if (p_idx < 0 || p_idx >= (int)_parts_by_idx.size()) continue;
        // A shape flagged as a mask is coverage-only (see _emit_normal_batch).
        if (_is_pure_mask_part(p_idx)) continue;
        const auto* part = _parts_by_idx[p_idx];
        if (!part) continue;

        const float* drawing_m = f.get_world_matrix(p_idx);
        if (!drawing_m) continue;

        if (!_build_shape_geometry(f, p_idx, part, drawing_m, _shape_buf)) continue;
        // No texture bound; the canvas_item shader's TEXTURE sampler then
        // returns white (Godot's default) for any UV. The PartColor formula
        // therefore composites against an implicit white "shape pixel".

        // CBP masking: a maskable shape must run the per-part mask test, which
        // needs per-part uniforms (rank / polarity) — so force the per-part path
        // for it. See _emit_normal_batch for the flag semantics.
        const uint16_t rank = (uint16_t)(batch->start_rank() + k);
        const PartMaskDecision md = _resolve_part_mask(f, p_idx, rank);
        const bool masked = md.masked;

        PartShaderInfo psi = _resolve_part_shader_info(f, part);
        if (psi.is_per_part || masked) {
            _flush_default_run(f.rs);
            RID part_ci = _acquire_per_part_canvas_item();
            Ref<ShaderMaterial> mat = _acquire_per_part_material(psi.id_hash, ssab_blend);
            // Shape parts have no bound cellmap texture; pass a zero
            // cell_rect (degenerates ss-circle / ss-spot to discard, which
            // is the safest fallback when the variant is misapplied to a Shape).
            _apply_per_part_uniforms(mat, psi.params, psi.map0, psi.map1, Vector4(0, 0, 0, 0));
            _stamp_part_mask(mat, rank, md);
            f.rs->canvas_item_set_material(part_ci, mat->get_rid());
            _emit_partcolor_mesh(f.rs, part_ci, _shape_buf.indices.ptr(), _shape_buf.index_count,
                                 _shape_buf.verts.ptr(), _shape_buf.colors.ptr(), _shape_buf.uvs.ptr(),
                                 _shape_buf.custom0.ptr(), _shape_buf.vert_count, RID());
        } else {
            if (!_run.open) _run_begin(true, 0, RID(), ssab_blend);
            _run_append(_shape_buf.verts.ptr(), _shape_buf.uvs.ptr(), _shape_buf.colors.ptr(),
                        _shape_buf.custom0.ptr(), _shape_buf.vert_count,
                        _shape_buf.indices.ptr(), _shape_buf.index_count);
        }
    }
}

void SsInternalPlayer::_emit_mesh_batch(const DrawFrame& f,
                                        const ss::runtime::DrawBatch* batch,
                                        const uint16_t* draw_order_data) {
    if (!batch || !draw_order_data) return;
    const uint16_t count = batch->count();
    if (count == 0) return;

    // The whole mesh shares one cellmap texture (batch invariant); resolve it
    // via texture_hash, exactly like a Normal batch.
    Ref<Texture2D> tex;
    if (_textures.has(batch->texture_hash())) {
        tex = _textures[batch->texture_hash()];
    }
    if (tex.is_null()) return;
    const Vector2 tex_size = tex->get_size();
    const Vector2 inv_tex_size = Vector2(1.0f / tex_size.x, 1.0f / tex_size.y);

    // batch->blend_type() is the runtime BlendType; converted to the ssab
    // BlendType by raw u8 value (same convention as _emit_normal_batch).
    const auto ssab_blend = (ss::format::BlendType)batch->blend_type();
    const RID tex_rid = tex->get_rid();

    for (uint16_t k = 0; k < count; k++) {
        int p_idx = (int)draw_order_data[batch->start_rank() + k];
        if (p_idx < 0 || p_idx >= (int)_parts_by_idx.size()) continue;
        // A mesh flagged as a mask is coverage-only (see _emit_normal_batch).
        if (_is_pure_mask_part(p_idx)) continue;
        const auto* part = _parts_by_idx[p_idx];
        if (!part) continue;

        if (!_build_mesh_geometry(f, p_idx, part, inv_tex_size, _mesh_buf)) continue;

        // CBP masking: a masked target must run the mask test. A part opts out
        // with mask_influence=0; write_mask clipping parts are always affected.
        const uint16_t rank = (uint16_t)(batch->start_rank() + k);
        const PartMaskDecision md = _resolve_part_mask(f, p_idx, rank);
        const bool masked = md.masked;

        PartShaderInfo psi = _resolve_part_shader_info(f, part);
        if (psi.is_per_part || masked) {
            _flush_default_run(f.rs);
            RID part_ci = _acquire_per_part_canvas_item();
            Ref<ShaderMaterial> mat = _acquire_per_part_material(psi.id_hash, ssab_blend);
            const Vector4 cell_rect = _resolve_cell_rect_uv(f, p_idx, inv_tex_size);
            _apply_per_part_uniforms(mat, psi.params, psi.map0, psi.map1, cell_rect);
            _stamp_part_mask(mat, rank, md);
            f.rs->canvas_item_set_material(part_ci, mat->get_rid());
            _emit_partcolor_mesh(f.rs, part_ci, _mesh_buf.indices.ptr(), _mesh_buf.index_count,
                                 _mesh_buf.verts.ptr(), _mesh_buf.colors.ptr(), _mesh_buf.uvs.ptr(),
                                 _mesh_buf.custom0.ptr(), _mesh_buf.vert_count, tex_rid);
        } else {
            if (!_run.open) _run_begin(false, batch->texture_hash(), tex_rid, ssab_blend);
            _run_append(_mesh_buf.verts.ptr(), _mesh_buf.uvs.ptr(), _mesh_buf.colors.ptr(),
                        _mesh_buf.custom0.ptr(), _mesh_buf.vert_count,
                        _mesh_buf.indices.ptr(), _mesh_buf.index_count);
        }
    }
}

bool SsInternalPlayer::_build_mesh_geometry(const DrawFrame& f, int p_idx,
                                            const ss::runtime::PartState* part,
                                            const Vector2& inv_tex_size,
                                            MeshGeometryBuffers& out) {
    // Per-part vertex range from the CSR offsets (shared by positions + UVs).
    if (!f.mesh_vertex_offsets || (uintptr_t)(p_idx + 1) >= f.mesh_vertex_offsets_len) return false;
    const uint32_t v0 = f.mesh_vertex_offsets[p_idx];
    const uint32_t v1 = f.mesh_vertex_offsets[p_idx + 1];
    if (v1 <= v0) return false;
    const int vert_count = (int)(v1 - v0);

    // Per-part triangle-index range.
    if (!f.mesh_index_offsets || (uintptr_t)(p_idx + 1) >= f.mesh_index_offsets_len) return false;
    const uint32_t i0 = f.mesh_index_offsets[p_idx];
    const uint32_t i1 = f.mesh_index_offsets[p_idx + 1];
    if (i1 <= i0 || (i1 - i0) % 3 != 0) return false;
    const int index_count = (int)(i1 - i0);

    // Bounds-check the source buffers before slicing.
    if (!f.mesh_vertices_x || !f.mesh_vertices_y) return false;
    if ((uintptr_t)v1 > f.mesh_vertices_x_len || (uintptr_t)v1 > f.mesh_vertices_y_len) return false;
    if (!f.mesh_uvs || (uintptr_t)v1 * 2 > f.mesh_uvs_len) return false;
    if (!f.mesh_indices || (uintptr_t)i1 > f.mesh_indices_len) return false;

    out.vert_count = vert_count;
    out.index_count = index_count;
    if (out.verts.size() < vert_count) {
        out.verts.resize(vert_count);
        out.uvs.resize(vert_count);
        out.colors.resize(vert_count);
        out.custom0.resize(vert_count * 4);
    }
    if (out.indices.size() < index_count) {
        out.indices.resize(index_count);
    }

    // Mesh part color is "Overall" only (a single flat color for the whole
    // part — see SS6 DrawMesh); resolve once, apply to every vertex. After
    // SDK converter mediation: rgba.a = rateAlpha, rate = colorA, regardless
    // of the source target.
    const float part_alpha = part->alpha();
    Color mesh_color(1, 1, 1, part_alpha);
    float mesh_rate = 0.0f;
    int blend_idx = 0;
    const uint64_t flags = part->update_flag();
    const auto partColorIndex = part->part_color();
    if ((flags & ss::runtime::UpdateAttributeFlags_AttributePartColor) && partColorIndex >= 0 && f.frameData->parts_color() != nullptr) {
        auto pc = f.frameData->parts_color()->Get(partColorIndex);
        auto to_color = [](const ss::runtime::SsAttributePartColorKeyValueColor& c) {
            return Color(c.rgba().r()/255.0f, c.rgba().g()/255.0f, c.rgba().b()/255.0f,
                         c.rgba().a()/255.0f);
        };
        mesh_color = to_color(pc->lt());
        mesh_rate = pc->lt().rate();
        blend_idx = (int)pc->blend_type();
        if (blend_idx < 0 || blend_idx > 3) blend_idx = 0;
    }

    const float pma_flag = 0.0f;

    const float* mx = f.mesh_vertices_x + v0;
    const float* my = f.mesh_vertices_y + v0;
    const float* uv = f.mesh_uvs + (uintptr_t)v0 * 2;
    Vector2* v_ptr = out.verts.ptrw();
    Vector2* u_ptr = out.uvs.ptrw();
    Color*   c_ptr = out.colors.ptrw();
    float*   x_ptr = out.custom0.ptrw();
    for (int i = 0; i < vert_count; i++) {
        // mesh_vertices are already world-space — no xform_raw needed.
        v_ptr[i] = Vector2(mx[i], my[i]);
        u_ptr[i] = Vector2(uv[i * 2] * inv_tex_size.x, uv[i * 2 + 1] * inv_tex_size.y);
        c_ptr[i] = mesh_color;
        x_ptr[i*4 + 0] = mesh_rate;
        x_ptr[i*4 + 1] = (float)blend_idx;
        x_ptr[i*4 + 2] = pma_flag;
        x_ptr[i*4 + 3] = 0.0f;
    }

    // Triangle indices are part-local 0-based; the default run rebases them
    // when it takes the part.
    int32_t* i_ptr = (int32_t*)out.indices.ptrw();
    const int32_t* src = f.mesh_indices + i0;
    for (int i = 0; i < index_count; i++) {
        i_ptr[i] = src[i];
    }
    return true;
}

void SsInternalPlayer::_fetchAnimation() {
    if ((_strAnimationSelected.is_empty() && _animationSelectedHash == 0) || _ssabRes.is_null()) {
        ss_runtime_reset(runtime_ctx);
        _reconfigure();
        _res_rebind_pending = true;
        _currentAnimationData = nullptr;
        // Free instance children before their parent canvas items — each
        // child's _root_ci is parented to a batch CI from
        // `_batch_canvas_items` per-frame, so freeing those first would
        // leave the children with a dangling parent RID until their own
        // dtor runs.
        _clear_instance_children();
        _clear_effect_slots();
        _clear_batch_canvas_items();
        _free_per_part_canvas_items();
        _free_mask_targets();
        // Reset has unbound the runtime, so the parse can go.
        _parsed.unref();
        return;
    }

    _clear_instance_children();
    _clear_effect_slots();
    _clear_batch_canvas_items();
    _free_per_part_canvas_items();
    _free_mask_targets();

    // Drop the previous animation up-front: it points into the OLD resource's
    // FlatBuffer, which setSSABResource may already have freed by releasing the
    // last Ref. Every failure path below returns without re-assigning it, and
    // update() / _emit_effect_slot dereference it on the next tick.
    _currentAnimationData = nullptr;

    // Re-borrow and re-bind only when the SSAB itself changed. Binding tells the
    // runtime the part identities may be new, so it drops every part override —
    // correct for a new SSAB, wrong for an animation change within one, where
    // Permanent-priority overrides are documented to persist (the runtime's
    // setup step keeps them). Re-binding here made every `animation = ...`
    // discard them.
    if (_res_rebind_pending || _parsed.is_null()) {
        // The resource's parse of its current buffer, shared with every other
        // player of it (and the instance children that play from it); a buffer
        // replaced in place gets a new one.
        Ref<SsParsedSsab> parsed = _ssabRes->get_parsed();
        if (parsed.is_null()) {
            ERR_PRINT(vformat("[SS] %s: the runtime could not read these %d bytes as an SSAB. Re-convert the .sspj it came from.", ssab_label(_ssabRes), _ssabRes->get_data_size()));
            return;
        }

        bool binded = ss_runtime_bind_resource(runtime_ctx, parsed->get_handle());
        if (!binded) {
            // Read before any other call on the context, which would clear it.
            const String reason = String::utf8(ss_runtime_error_message(ss_runtime_get_last_error(runtime_ctx)));
            ERR_PRINT(vformat("[SS] %s: the runtime read the SSAB but would not bind it%s.", ssab_label(_ssabRes),
                              reason.is_empty() ? String() : ": " + reason));
            return;
        }
        // The previous parse goes only now that the runtime is bound to this one.
        _parsed = parsed;
        _res_rebind_pending = false;
        // Which incarnation of the buffer this borrow refers to.
        _borrowed_generation = _ssabRes->get_generation();
    }

    // Per-batch canvas_item pool grows on demand inside _drawAnimation via
    // _ensure_batch_ci(). No per-part CIs are pre-allocated here.

    const ss::format::AnimationData* animation = nullptr;
    if (_animationSelectedHash != 0) {
        animation = _ssabRes->find_animation_by_hash(_animationSelectedHash);
        if (animation && animation->name()) {
            _strAnimationSelected = String::utf8(animation->name()->c_str());
        }
    }
    
    if (!animation && !_strAnimationSelected.is_empty()) {
        animation = _ssabRes->find_animation(_strAnimationSelected);
        if (animation) {
            _animationSelectedHash = animation->name_hash();
        }
    }

    if (!animation) {
        // Reported once per wrong name without a guard of its own: setAnimation
        // stores the name before calling in here and returns early when it is
        // asked for the same one again, so a state machine re-asserting a typo
        // every frame logs a single line.
        ERR_PRINT(vformat("[SS] %s has no animation %s.",
                          ssab_label(_ssabRes),
                          animation_label(_strAnimationSelected, _animationSelectedHash)));
        return;
    }
    _currentAnimationData = animation;
    bool setup = ss_runtime_setup_animation_by_hash(runtime_ctx, _animationSelectedHash);
    if (!setup) {
        // The animation IS in the pack -- the lookup above found it -- so this is
        // the runtime declining to set it up, not a wrong name. Say which one
        // anyway: it is the only handle the user has on the failure.
        ERR_PRINT(vformat("[SS] %s: the runtime would not set up animation %s.",
                          ssab_label(_ssabRes),
                          animation_label(_strAnimationSelected, _animationSelectedHash)));
        // The runtime has no animation bound, so nothing may consume this frame's
        // animation data either.
        _currentAnimationData = nullptr;
        return;
    }

    // Recursive setup — instance children also populate their own
    // `_instance_children`, so nested Instance parts are supported.
    _setup_instance_children();
    _setup_effect_slots();

    _seek_and_redraw(ss_runtime_get_frame_no(runtime_ctx), 0.0f, false);
}
