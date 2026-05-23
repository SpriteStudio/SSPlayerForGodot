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
#include "core/io/resource_loader.h"
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

    RenderingServer* rs = RenderingServer::get_singleton();
    if (_root_ci.is_valid()) {
        rs->free_rid(_root_ci);
        _root_ci = RID();
    }

    if (runtime_ctx != nullptr) {
        ss_runtime_destroy(runtime_ctx);
        runtime_ctx = nullptr;
    }
    if (runtime_res != nullptr) {
        ss_resource_destroy(runtime_res);
        runtime_res = nullptr;
    }
}

void SsInternalPlayer::_reconfigure() {
    if (runtime_ctx != nullptr) {
        ss_context_set_coordinate_system(runtime_ctx, 1);
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
    _blend_materials.clear();
}

RID SsInternalPlayer::_ensure_batch_ci(int batch_idx) {
    RenderingServer* rs = RenderingServer::get_singleton();
    while (_batch_canvas_items.size() <= batch_idx) {
        RID ci = rs->canvas_item_create();
        rs->canvas_item_set_parent(ci, _root_ci);
        _batch_canvas_items.push_back(ci);
    }
    return _batch_canvas_items[batch_idx];
}

void SsInternalPlayer::setSSABResource(const Ref<SSABResource>& ssabRes) {
    _ssabRes = ssabRes;
    _strAnimationSelected = "";

    if (!_ssabRes.is_null()) {
        if (!_ssabRes->is_valid()) {
            ERR_PRINT("SSAB Error: Assigned resource is invalid (missing parts or animations).");
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
}

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
    _fetchAnimation();
    if (_event_sink) {
        _event_sink->onAnimationChanged(_strAnimationSelected);
    }
}

bool SsInternalPlayer::isPlaying() const {
    return ss_runtime_is_playing(runtime_ctx);
}

void SsInternalPlayer::play(float p_start_frame) {
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

void SsInternalPlayer::pause() {
    ss_runtime_pause(runtime_ctx);
}

void SsInternalPlayer::stop() {
    ss_runtime_stop(runtime_ctx);
}

void SsInternalPlayer::setSpeed(float p_speed) {
    _speed_rate = p_speed;
    ss_runtime_set_animation_speed(runtime_ctx, p_speed);
}

float SsInternalPlayer::getSpeed() const {
    return _speed_rate;
}

void SsInternalPlayer::setFrame(float p_frame) {
    if (runtime_ctx) {
        ss_runtime_set_frame_no(runtime_ctx, p_frame);
        _seek_and_redraw(ss_runtime_get_frame_no(runtime_ctx), 0.0f, false);
    }
}

float SsInternalPlayer::getFrame() const {
    return ss_runtime_get_frame_no(runtime_ctx);
}

int SsInternalPlayer::getTotalFrames() const {
    return ss_runtime_get_end_frame(runtime_ctx) - ss_runtime_get_start_frame(runtime_ctx) + 1;
}

void SsInternalPlayer::setFrameRate(int p_fps) {
    ss_runtime_set_frame_rate(runtime_ctx, p_fps);
}

int SsInternalPlayer::getFrameRate() const {
    return ss_runtime_get_fps(runtime_ctx);
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
    ss_runtime_set_loop(runtime_ctx, p_count);
}

int SsInternalPlayer::getLoop() const {
    return ss_runtime_get_loops(runtime_ctx);
}

void SsInternalPlayer::setSkipFrames(bool p_skip) {
    ss_runtime_set_skip_frames(runtime_ctx, p_skip);
}

bool SsInternalPlayer::isSkipFrames() const {
    return ss_runtime_get_skip_frames(runtime_ctx);
}

void SsInternalPlayer::setSubFrameEnabled(bool p_enabled) {
    _sub_frame_enabled = p_enabled;
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
                        Ref<Texture2D> original_tex =
                        #ifdef SPRITESTUDIO_GODOT_EXTENSION
                        ResourceLoader::get_singleton()->load(strImage, "", ResourceLoader::CACHE_MODE_REUSE);
                        #else
                        ResourceLoader::load(strImage, "", ResourceFormatLoader::CACHE_MODE_REUSE, nullptr);
                        #endif
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
                        Ref<Texture2D> original_tex =
                        #ifdef SPRITESTUDIO_GODOT_EXTENSION
                        ResourceLoader::get_singleton()->load(strImage, "", ResourceLoader::CACHE_MODE_REUSE);
                        #else
                        ResourceLoader::load(strImage, "", ResourceFormatLoader::CACHE_MODE_REUSE, nullptr);
                        #endif
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
            Ref<Texture2D> texture =
            #ifdef SPRITESTUDIO_GODOT_EXTENSION
            ResourceLoader::get_singleton()->load(strImage, "", ResourceLoader::CACHE_MODE_REUSE);
            #else
            ResourceLoader::load(strImage, "", ResourceFormatLoader::CACHE_MODE_REUSE, nullptr);
            #endif
            _textures[cellmap->name_hash()] = texture;
        }
    }
    if (a->external_textures() != nullptr) {
        for (int i = 0; i < a->external_textures()->size(); i++) {
            auto etexture = a->external_textures()->Get(i);
            String strImage = _ssabRes->get_parent_dir().path_join(String::utf8(etexture->name()->c_str()));
            Ref<Texture2D> texture =
            #ifdef SPRITESTUDIO_GODOT_EXTENSION
            ResourceLoader::get_singleton()->load(strImage, "", ResourceLoader::CACHE_MODE_REUSE);
            #else
            ResourceLoader::load(strImage, "", ResourceFormatLoader::CACHE_MODE_REUSE, nullptr);
            #endif
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
    if (!ss_runtime_is_playing(runtime_ctx)) return;

    auto d = delta_seconds * 1000.0f;
    float frame_no = ss_runtime_update(runtime_ctx, d);

    const bool was_looped = ss_runtime_is_looped(runtime_ctx);
    if (was_looped) {
        if (_event_sink) _event_sink->onAnimationLooped(_strAnimationSelected);
    }
    if (ss_runtime_is_end_frame_reached(runtime_ctx)) {
        if (_event_sink) _event_sink->onAnimationFinished(_strAnimationSelected);
    }

    float draw_frame = _sub_frame_enabled ? frame_no : floorf(frame_no);
    if (previous_frame_no == draw_frame && !_needs_continuous_update()) return;

    if (_currentAnimationData && _currentAnimationData->events() != nullptr) {
        int event_count = ss_runtime_get_passed_event_count(runtime_ctx);
        for (int i = 0; i < event_count; i++) {
            int event_idx = ss_runtime_get_passed_event_index(runtime_ctx, i);
            auto events_per_frame = _currentAnimationData->events()->Get(event_idx);

            if (auto users = events_per_frame->users()) {
                for (uint32_t j = 0; j < users->size(); j++) {
                    auto user = users->Get(j);
                    if (!user || !user->value()) continue;
                    auto val = user->value();

                    Dictionary payload;
                    if (auto i = val->integer()) payload["integer"] = i->value();
                    if (auto r = val->rect())    payload["rect"]    = Rect2(r->x1(), r->y1(), r->x2() - r->x1(), r->y2() - r->y1());
                    if (auto p = val->point())   payload["point"]   = Vector2(p->v1(), p->v2());
                    if (auto s = val->str())     payload["string"]  = String::utf8(s->c_str());

                    if (_event_sink) _event_sink->onUserData(payload);
                }
            }

            if (auto audios = events_per_frame->audios()) {
                // TODO: Audio integration
            }
        }
    }

    _seek_and_redraw(frame_no, delta_seconds, was_looped);
}

bool SsInternalPlayer::_build_mask_writers(const DrawFrame& f) {
    _mask_writers.clear();
    if (!f.frameData || !f.binary) return false;
    auto parts_meta = f.binary->parts();
    auto draw_order = f.frameData->draw_order();
    if (!parts_meta || !draw_order) return false;

    const int total_meta = (int)parts_meta->size();
    const uint32_t n = draw_order->size();
    for (uint32_t rank = 0; rank < n; rank++) {
        const int p_idx = (int)draw_order->Get(rank);
        if (p_idx < 0 || p_idx >= total_meta) continue;
        const auto* pd = parts_meta->Get(p_idx);
        if (!pd) continue;

        const auto pt = pd->part_type_type();
        const bool is_mask_part = (pt == ss::format::PartType_PartTypeMask);
        // A "pure" mask draws no colour and masks the parts drawn BEFORE it: a
        // Mask part, or a shape/text/nines part flagged as a mask via its
        // per-type *_mask flag. A write_mask (clipping) writer instead draws
        // normally AND masks the parts drawn AFTER it.
        bool pure_mask = is_mask_part;
        if (!pure_mask && pt == ss::format::PartType_PartTypeShape) {
            const auto* s = pd->part_type_as_PartTypeShape();
            pure_mask = s && s->shape_mask();
        }
        if (!pure_mask && pt == ss::format::PartType_PartTypeText) {
            const auto* t = pd->part_type_as_PartTypeText();
            pure_mask = t && t->text_mask();
        }
        if (!pure_mask && pt == ss::format::PartType_PartTypeNines) {
            const auto* nn = pd->part_type_as_PartTypeNines();
            pure_mask = nn && nn->nines_mask();
        }
        const bool writes = pure_mask || pd->mask_write();
        if (!writes) continue;

        if ((int)_mask_writers.size() >= MAX_MASK_WRITERS) break; // bitmap holds 32

        MaskWriter w;
        w.part_index = p_idx;
        w.draw_rank = (uint16_t)rank;
        w.bit = (uint8_t)_mask_writers.size();
        w.op_invert = pd->mask_influence();
        // Scope: write_mask (clipping) writers mask the parts drawn AFTER them;
        // pure masks (Mask / shape/text/nines mask) mask the parts BEFORE them
        // and draw no colour, regardless of write_mask.
        w.is_clipping = pd->mask_write() && !pure_mask;
        _mask_writers.push_back(w);
    }
    return !_mask_writers.is_empty();
}

void SsInternalPlayer::_ensure_mask_targets() {
    RenderingServer* rs = RenderingServer::get_singleton();
    if (_mask_write_shader.is_null()) {
        _mask_write_shader.instantiate();
        _mask_write_shader->set_code(String(SS_MASK_WRITE_SHADER));
    }
    if (!_mask_viewport.is_valid()) {
        _mask_viewport = rs->viewport_create();
        // UPDATE_ALWAYS: the coverage viewport renders every frame; maskable
        // parts sample the previous frame's result (one-frame latency), which
        // avoids depending on inter-viewport render ordering for a first cut.
        rs->viewport_set_update_mode(_mask_viewport, RenderingServer::VIEWPORT_UPDATE_ALWAYS);
        rs->viewport_set_clear_mode(_mask_viewport, RenderingServer::VIEWPORT_CLEAR_ALWAYS);
        rs->viewport_set_transparent_background(_mask_viewport, true);
        rs->viewport_set_disable_3d(_mask_viewport, true);
        rs->viewport_set_active(_mask_viewport, true);
        rs->viewport_set_size(_mask_viewport, 1, 1);
    }
    if (!_mask_canvas.is_valid()) {
        _mask_canvas = rs->canvas_create();
        rs->viewport_attach_canvas(_mask_viewport, _mask_canvas);
    }
}

void SsInternalPlayer::_free_mask_targets() {
    RenderingServer* rs = RenderingServer::get_singleton();
    for (int i = 0; i < _mask_canvas_items.size(); i++) {
        if (_mask_canvas_items[i].is_valid()) rs->free_rid(_mask_canvas_items[i]);
    }
    _mask_canvas_items.clear();
    _mask_canvas_items_in_use = 0;
    _mask_write_materials.clear();
    _mask_write_materials_in_use = 0;
    if (_mask_canvas.is_valid()) { rs->free_rid(_mask_canvas); _mask_canvas = RID(); }
    if (_mask_viewport.is_valid()) { rs->free_rid(_mask_viewport); _mask_viewport = RID(); }
    _mask_write_shader = Ref<Shader>();
    _mask_coverage_valid = false;
}

RID SsInternalPlayer::_acquire_mask_canvas_item() {
    RenderingServer* rs = RenderingServer::get_singleton();
    if (_mask_canvas_items_in_use >= _mask_canvas_items.size()) {
        RID ci = rs->canvas_item_create();
        rs->canvas_item_set_parent(ci, _mask_canvas);
        _mask_canvas_items.push_back(ci);
    }
    RID ci = _mask_canvas_items[_mask_canvas_items_in_use++];
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
    if (_mask_writers.is_empty() || !f.frameData) return;
    _ensure_mask_targets();
    RenderingServer* rs = f.rs;

    auto draw_batches = f.frameData->draw_batches();
    auto draw_order = f.frameData->draw_order();
    if (!draw_batches || !draw_order) return;
    const uint16_t* draw_order_data = draw_order->data();

    // Recycle the coverage CI / material pools (hide all, cursors back to 0).
    for (int i = 0; i < _mask_canvas_items.size(); i++) {
        rs->canvas_item_clear(_mask_canvas_items[i]);
        rs->canvas_item_set_visible(_mask_canvas_items[i], false);
    }
    _mask_canvas_items_in_use = 0;
    _mask_write_materials_in_use = 0;

    bool have_bbox = false;
    Vector2 bmin, bmax;

    for (uint32_t bi = 0; bi < draw_batches->size(); bi++) {
        const auto* batch = draw_batches->Get(bi);
        if (!batch) continue;
        const auto kind = batch->kind();
        if (kind != ss::runtime::DrawBatchKind_Normal && kind != ss::runtime::DrawBatchKind_Mask
            && kind != ss::runtime::DrawBatchKind_Mesh && kind != ss::runtime::DrawBatchKind_Shape) {
            continue;
        }

        RID tex_rid;
        Vector2 inv_tex_size(1, 1);
        if (batch->texture_hash() != 0 && _textures.has(batch->texture_hash())) {
            Ref<Texture2D> tex = _textures[batch->texture_hash()];
            if (tex.is_valid()) {
                tex_rid = tex->get_rid();
                const Vector2 ts = tex->get_size();
                if (ts.x > 0 && ts.y > 0) inv_tex_size = Vector2(1.0f / ts.x, 1.0f / ts.y);
            }
        }

        const uint16_t count = batch->count();
        for (uint16_t k = 0; k < count; k++) {
            const int p_idx = (int)draw_order_data[batch->start_rank() + k];
            const MaskWriter* w = nullptr;
            for (int wi = 0; wi < _mask_writers.size(); wi++) {
                if (_mask_writers[wi].part_index == p_idx) { w = &_mask_writers[wi]; break; }
            }
            if (!w) continue;
            if (p_idx < 0 || p_idx >= (int)_parts_by_idx.size()) continue;
            const auto* part = _parts_by_idx[p_idx];
            if (!part) continue;
            const float* draw_m = f.get_world_matrix(p_idx);
            if (!draw_m) continue;

            // Build the writer's geometry per batch kind into common pointers.
            // Mesh verts already arrive world-space; Normal/Mask/Shape apply
            // draw_m inside their builders.
            const SsVec2Array* gverts = nullptr;
            const SsVec2Array* guvs = nullptr;
            const SsColorArray* gcolors = nullptr;
            const SsIntArray* gindices = nullptr;
            bool no_cutout = false; // Shapes have no texture; the geometry is the mask.

            if (kind == ss::runtime::DrawBatchKind_Mesh) {
                if (!_build_mesh_geometry(f, p_idx, part, inv_tex_size, _mesh_buf)) continue;
                gverts = &_mesh_buf.verts; guvs = &_mesh_buf.uvs;
                gcolors = &_mesh_buf.colors; gindices = &_mesh_buf.indices;
            } else if (kind == ss::runtime::DrawBatchKind_Shape) {
                if (!_build_shape_geometry(f, p_idx, part, draw_m, _shape_buf)) continue;
                gverts = &_shape_buf.verts; guvs = &_shape_buf.uvs;
                gcolors = &_shape_buf.colors; gindices = &_shape_buf.indices;
                no_cutout = true;
            } else {
                _per_part_normal_verts.resize(MAX_VERTICES_COUNT);
                _per_part_normal_uvs.resize(MAX_VERTICES_COUNT);
                _per_part_normal_colors.resize(MAX_VERTICES_COUNT);
                _per_part_normal_custom0.resize(MAX_VERTICES_COUNT * 4);
                const int vc = _build_normal(f, p_idx, part, draw_m, inv_tex_size,
                                             _per_part_normal_verts, _per_part_normal_uvs,
                                             _per_part_normal_colors, _per_part_normal_custom0, 0);
                if (vc <= 0) continue;
                if (vc == MAX_VERTICES_COUNT) {
                    const int pent[INDICES_COUNT_PENTAGON] = { 0,1,4, 1,3,4, 3,2,4, 2,0,4 };
                    _per_part_normal_indices.resize(INDICES_COUNT_PENTAGON);
                    int32_t* iptr = (int32_t*)_per_part_normal_indices.ptrw();
                    for (int j = 0; j < INDICES_COUNT_PENTAGON; j++) iptr[j] = pent[j];
                } else {
                    const int quad[INDICES_COUNT_QUAD] = { 0,1,2, 1,3,2 };
                    _per_part_normal_indices.resize(INDICES_COUNT_QUAD);
                    int32_t* iptr = (int32_t*)_per_part_normal_indices.ptrw();
                    for (int j = 0; j < INDICES_COUNT_QUAD; j++) iptr[j] = quad[j];
                }
                _per_part_normal_verts.resize(vc);
                _per_part_normal_uvs.resize(vc);
                _per_part_normal_colors.resize(vc);
                gverts = &_per_part_normal_verts; guvs = &_per_part_normal_uvs;
                gcolors = &_per_part_normal_colors; gindices = &_per_part_normal_indices;
            }

            const int nv = gverts->size();
            if (nv <= 0) continue;

            // Accumulate the writer bounding box in player-local space.
            const Vector2* vp = gverts->ptr();
            for (int j = 0; j < nv; j++) {
                if (!have_bbox) { bmin = bmax = vp[j]; have_bbox = true; }
                else {
                    if (vp[j].x < bmin.x) bmin.x = vp[j].x;
                    if (vp[j].y < bmin.y) bmin.y = vp[j].y;
                    if (vp[j].x > bmax.x) bmax.x = vp[j].x;
                    if (vp[j].y > bmax.y) bmax.y = vp[j].y;
                }
            }

            // Encode this writer's bit into R/G/B (24 writers; alpha reserved
            // as the premultiplied-blend coverage accumulator).
            const int chan = w->bit / 8;
            const uint8_t bit_val = 1 << (w->bit % 8);
            uint8_t r = (chan == 0) ? bit_val : 0;
            uint8_t g = (chan == 1) ? bit_val : 0;
            uint8_t b = (chan == 2) ? bit_val : 0;
            Color bit_color = Color::from_rgba8(r, g, b, 0);

            // Cutout threshold. Pure masks (PartTypeMask) fade by the MASK
            // strength attribute (0..255; 0 -> empty). Clipping (write_mask)
            // writers carry no strength — they clip with the full sprite/mesh
            // shape (alpha > 0). Shapes have no texture (no_cutout).
            float threshold = no_cutout ? -1.0f
                            : (w->is_clipping ? 0.0f : (float)(255 - part->mask()) / 255.0f);
            if (threshold > 1.0f) threshold = 1.0f;

            RID mask_ci = _acquire_mask_canvas_item();
            Ref<ShaderMaterial> mat = _acquire_mask_write_material();
            mat->set_shader_parameter("mask_bit_color", bit_color);
            mat->set_shader_parameter("mask_threshold", threshold);
            rs->canvas_item_set_material(mask_ci, mat->get_rid());
            rs->canvas_item_set_transform(mask_ci, Transform2D());
            rs->canvas_item_add_triangle_array(mask_ci, *gindices, *gverts, *gcolors, *guvs, {}, {}, tex_rid);
        }
    }

    if (!have_bbox) return;
    Vector2 bsize = bmax - bmin;
    if (bsize.x < CMP_EPSILON) bsize.x = CMP_EPSILON;
    if (bsize.y < CMP_EPSILON) bsize.y = CMP_EPSILON;
    const float longest = bsize.x > bsize.y ? bsize.x : bsize.y;
    const float scale = (float)MASK_COVERAGE_MAX_DIM / longest;
    int vw = (int)std::ceil(bsize.x * scale);
    int vh = (int)std::ceil(bsize.y * scale);
    if (vw < 1) vw = 1; else if (vw > MASK_COVERAGE_MAX_DIM) vw = MASK_COVERAGE_MAX_DIM;
    if (vh < 1) vh = 1; else if (vh > MASK_COVERAGE_MAX_DIM) vh = MASK_COVERAGE_MAX_DIM;
    rs->viewport_set_size(_mask_viewport, vw, vh);

    // canvas_transform: player-local -> coverage viewport pixels.
    const Vector2 s((float)vw / bsize.x, (float)vh / bsize.y);
    Transform2D ct;
    ct.columns[0] = Vector2(s.x, 0);
    ct.columns[1] = Vector2(0, s.y);
    ct.columns[2] = Vector2(-bmin.x * s.x, -bmin.y * s.y);
    rs->viewport_set_canvas_transform(_mask_viewport, _mask_canvas, ct);

    // player-local -> coverage UV [0,1], handed to maskable shaders (P3).
    Transform2D uv;
    uv.columns[0] = Vector2(1.0f / bsize.x, 0);
    uv.columns[1] = Vector2(0, 1.0f / bsize.y);
    uv.columns[2] = Vector2(-bmin.x / bsize.x, -bmin.y / bsize.y);
    _mask_local_to_uv = uv;

    // Frame mask state consumed by the maskable emit path (P3).
    _mask_uv_xform = Vector4(1.0f / bsize.x, 1.0f / bsize.y, -bmin.x / bsize.x, -bmin.y / bsize.y);
    _mask_coverage_tex = rs->viewport_get_texture(_mask_viewport);
    _mask_max_mask_slot = -1.0f;
    _mask_min_clip_slot = -1.0f;
    _mask_meta_array.clear();
    _mask_meta_array.resize(_mask_writers.size());
    for (int i = 0; i < _mask_writers.size(); i++) {
        const MaskWriter& w = _mask_writers[i];
        _mask_meta_array[i] = Vector4((float)w.draw_rank, (float)w.bit,
                                      w.op_invert ? 1.0f : 0.0f, w.is_clipping ? 1.0f : 0.0f);
        if (w.is_clipping) {
            if (_mask_min_clip_slot < 0.0f || (float)w.draw_rank < _mask_min_clip_slot) {
                _mask_min_clip_slot = (float)w.draw_rank;
            }
        } else if ((float)w.draw_rank > _mask_max_mask_slot) {
            _mask_max_mask_slot = (float)w.draw_rank;
        }
    }
    _mask_coverage_valid = true;
}

bool SsInternalPlayer::_part_in_mask_scope(uint16_t rank) const {
    const float r = (float)rank;
    if (_mask_max_mask_slot >= 0.0f && r < _mask_max_mask_slot) return true;
    if (_mask_min_clip_slot >= 0.0f && r > _mask_min_clip_slot) return true;
    return false;
}

bool SsInternalPlayer::_is_pure_mask_part(int p_idx) const {
    for (int i = 0; i < _mask_writers.size(); i++) {
        if (_mask_writers[i].part_index == p_idx) return !_mask_writers[i].is_clipping;
    }
    return false;
}

void SsInternalPlayer::_apply_mask_uniforms(Ref<ShaderMaterial> mat, uint16_t rank, bool visible_inside) {
    mat->set_shader_parameter("ss_mask_enabled", true);
    mat->set_shader_parameter("ss_mask_uv_xform", _mask_uv_xform);
    mat->set_shader_parameter("ss_mask_count", (int)_mask_writers.size());
    mat->set_shader_parameter("ss_mask_meta", _mask_meta_array);
    mat->set_shader_parameter("ss_mask_rank", (float)rank);
    mat->set_shader_parameter("ss_mask_visible_inside", visible_inside ? 1.0f : 0.0f);
    if (_mask_coverage_tex.is_valid()) {
        // Bind the coverage viewport's texture (RID) to the sampler uniform.
        RenderingServer::get_singleton()->material_set_param(mat->get_rid(), "ss_mask_coverage", _mask_coverage_tex);
    }
}

void SsInternalPlayer::_drawAnimation(float frame_no, float delta_seconds, bool parent_looped) {
    unsigned char* data = nullptr;
    uintptr_t len = 0;
    ss_runtime_get_frame_data(runtime_ctx, frame_no, &data, &len);
    if (!data) return;

    // Release the previous frame's ArrayMesh references. The CanvasItem
    // commands recorded last frame are about to be overwritten by
    // canvas_item_clear in the per-batch loop below, so dropping these refs
    // is safe (the renderer no longer needs the mesh data).
    _frame_meshes.clear();
    // Walk the per-part material / canvas_item pool cursors back to 0 so any
    // per-part rendering during this frame re-uses entries from the front of
    // each pool rather than growing them. No-op when no per-part variants
    // are dispatched (pools stay empty).
    _reset_per_part_pools();

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

    auto parts = f.frameData->parts();
    auto draw_order = f.frameData->draw_order();
    auto draw_batches = f.frameData->draw_batches();

    {
        const int total = f.binary->parts() ? (int)f.binary->parts()->size() : 0;
        if ((int)_parts_by_idx.size() != total) _parts_by_idx.resize(total);
        if (total > 0) {
            for (int i = 0; i < total; i++) _parts_by_idx[i] = nullptr;
        }
        for (uint32_t i = 0; i < parts->size(); i++) {
            auto p = parts->Get(i);
            int idx = p->part_index();
            if (idx >= 0 && idx < total) _parts_by_idx[idx] = p;
        }
    }

    // CBP masking: collect this frame's mask writers, then render the coverage
    // bitmap. The top-root player owns the mask state (instance children are
    // masked by it in P3), so only render coverage when not parent-driven.
    if (_build_mask_writers(f) && !_parent_driven) {
        _render_mask_coverage(f);
    }

    const uint16_t* draw_order_data = draw_order->data();
    const uint32_t batch_count = draw_batches->size();

    // Hide / clear unused entries in the pool (peak-retain policy).
    for (int i = (int)batch_count; i < _batch_canvas_items.size(); i++) {
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

    // Per-frame draw-order counter: batch CIs and per-part CIs draw in the
    // order they are emitted (rank order), not in CI-pool allocation order.
    _draw_seq = 0;
    for (uint32_t bi = 0; bi < batch_count; bi++) {
        const auto* batch = draw_batches->Get(bi);
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

        const auto kind = batch->kind();
        // Pure masks feed only the coverage bitmap and must not draw their own
        // colour (PartTypeMask has no draw branch below; this also suppresses
        // shape/text/nines *_mask color draws so they read as holes, not fills).
        if (_mask_coverage_valid && batch->count() > 0
            && _is_pure_mask_part((int)draw_order_data[batch->start_rank()])) {
            continue;
        }
        if (kind == ss::runtime::DrawBatchKind_Normal) {
            _emit_normal_batch(f, ci, batch, draw_order_data);
        } else if (kind == ss::runtime::DrawBatchKind_Shape) {
            _emit_shape_batch(f, ci, batch, draw_order_data);
        } else if (kind == ss::runtime::DrawBatchKind_Instance) {
            int p_idx = (int)draw_order_data[batch->start_rank()];
            const auto* part = (p_idx >= 0 && p_idx < (int)_parts_by_idx.size()) ? _parts_by_idx[p_idx] : nullptr;
            if (!part) continue;
            const float* drawing_m = f.get_world_matrix(p_idx);
            if (!drawing_m) continue;
            _emit_instance_slot(f, ci, p_idx, drawing_m);
        } else if (kind == ss::runtime::DrawBatchKind_Effect) {
            int p_idx = (int)draw_order_data[batch->start_rank()];
            const auto* part = (p_idx >= 0 && p_idx < (int)_parts_by_idx.size()) ? _parts_by_idx[p_idx] : nullptr;
            if (!part) continue;
            const float* drawing_m = f.get_world_matrix(p_idx);
            if (!drawing_m) continue;
            _emit_effect_slot(f, ci, p_idx, drawing_m);
        } else if (kind == ss::runtime::DrawBatchKind_Mesh) {
            _emit_mesh_batch(f, ci, batch, draw_order_data);
        }
        // DrawBatchKind_Text / Nines / Mask: not yet implemented.
    }
}

static String find_anim_name_in(const Ref<SSABResource>& res, uint32_t name_hash) {
    if (res.is_null()) return String();
    auto binary = res->get_ss_anime_binary();
    if (!binary || !binary->animations()) return String();
    auto anims = binary->animations();
    for (uint32_t i = 0; i < anims->size(); i++) {
        auto a = anims->Get(i);
        if (a && a->name_hash() == name_hash) {
            return a->name() ? String::utf8(a->name()->c_str()) : String();
        }
    }
    return String();
}

String SsInternalPlayer::_resolve_animation_by_hash(uint32_t name_hash, Ref<SSABResource>& out_source) const {
    out_source = Ref<SSABResource>();

    if (!_ssabRes.is_null()) {
        auto binary = _ssabRes->get_ss_anime_binary();
        if (binary && binary->external_instances()) {
            // external_instances is sorted by anime_name_hash, so binary-search
            // for the hint, then resolve the owning pack via the pack_hash map.
            const auto* entry = binary->external_instances()->LookupByKey(name_hash);
            if (entry) {
                const uint32_t pack_hash = entry->anime_pack_name_hash();
                if (auto* ext_ptr = _external_ssabs_by_pack_hash.getptr(pack_hash)) {
                    String found = find_anim_name_in(*ext_ptr, name_hash);
                    if (!found.is_empty()) {
                        out_source = *ext_ptr;
                        return found;
                    }
                }
            }
        }
    }

    String found = find_anim_name_in(_ssabRes, name_hash);
    if (!found.is_empty()) {
        out_source = _ssabRes;
        return found;
    }
    for (int i = 0; i < _external_ssabs.size(); i++) {
        const Ref<SSABResource>& ext = _external_ssabs[i];
        found = find_anim_name_in(ext, name_hash);
        if (!found.is_empty()) {
            out_source = ext;
            return found;
        }
    }
    return String();
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
    for (uint32_t i = 0; i < exts->size(); i++) {
        auto entry = exts->Get(i);
        if (!entry || !entry->anime_pack_name()) continue;
        const uint32_t pack_hash = entry->anime_pack_name_hash();
        if (_external_ssabs_by_pack_hash.has(pack_hash)) continue;

        String pack = String::utf8(entry->anime_pack_name()->c_str());
        if (pack.is_empty()) continue;
        String path = parent_dir.path_join(pack + ".ssab");
        Ref<Resource> res =
        #ifdef SPRITESTUDIO_GODOT_EXTENSION
            ResourceLoader::get_singleton()->load(path, "", ResourceLoader::CACHE_MODE_REUSE);
        #else
            ResourceLoader::load(path, "", ResourceFormatLoader::CACHE_MODE_REUSE, nullptr);
        #endif
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
        _external_ssabs_by_pack_hash[pack_hash] = ssab;
    }
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

        Ref<SSABResource> source;
        uint32_t ref_hash = pt->ref_anime_hash();
        String anim_name = _resolve_animation_by_hash(ref_hash, source);
        if (anim_name.is_empty() || source.is_null()) {
            ERR_PRINT(vformat("[SS] instance part %d: ref_anime_hash=0x%x not found in current or external SSABs", i, ref_hash));
            continue;
        }

        SsInternalPlayer* child = memnew(SsInternalPlayer);
        child->setParentDriven(true);
        // Hand the child the SSAB that actually contains the referenced
        // animation — may be `_ssabRes` itself or an external sibling.
        child->setSSABResource(source);
        child->setAnimation(anim_name);
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
    }
    _effect_slots.clear();
}

void SsInternalPlayer::_setup_effect_slots() {
    _clear_effect_slots();
    if (_ssabRes.is_null() || runtime_res == nullptr) return;
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
        void* effect_slot = ss_effect_slot_create(runtime_res, pt->ref_effect_name_hash(), seed);
        if (!effect_slot) {
            ERR_PRINT(vformat("[SS] effect part %d: ref_effect_name_hash=0x%x not found in current SSAB", i, pt->ref_effect_name_hash()));
            continue;
        }
        _effect_slots[i].effect_slot = effect_slot;
    }
}

void SsInternalPlayer::_update_instance_children(float parent_frame_no, float delta_seconds, bool parent_looped) {
    if (!_currentAnimationData) {
        // No animation selected on the parent: keep all children hidden.
        for (uint32_t i = 0; i < _instance_children.size(); i++) {
            SsInternalPlayer* child = _instance_children[i].player;
            if (child) child->setRootVisible(false);
        }
        return;
    }

    for (uint32_t p_idx = 0; p_idx < _instance_children.size(); p_idx++) {
        InstanceChildState& state = _instance_children[p_idx];
        SsInternalPlayer* child = state.player;
        if (!child || !state.instance_slot) continue;

        const ss_event_instance_info info = ss_runtime_get_active_event_instance(runtime_ctx, p_idx);
        _drive_instance_slot(state, child, info, parent_frame_no, delta_seconds, parent_looped);
    }
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
    // is left with the side effects ssruntime cannot perform: firing the
    // child Player's event sink on transition (via play()), and the redraw.
    const ss_instance_step_result r = ss_instance_slot_step(
        state.instance_slot, info, child->runtime_ctx,
        child->previous_frame_no, parent_frame_no, delta_seconds, parent_looped);

    if (r.transitioned) {
        // Controller is already configured + playing inside step(); play()
        // here is for its host-side side effect (onAnimationStarted callback).
        child->play();
    }

    child->setRootVisible(false);
    if (!r.visible) {
        return;
    }

    _redraw_child_if_frame_changed(child, r.child_frame_no, delta_seconds, r.child_looped);
}

void SsInternalPlayer::_redraw_child_if_frame_changed(SsInternalPlayer* child, float frame_no, float delta_seconds, bool parent_looped) {
    const float draw_frame = child->_sub_frame_enabled ? frame_no : floorf(frame_no);
    if (child->previous_frame_no == draw_frame && !child->_needs_continuous_update()) return;
    child->previous_frame_no = draw_frame;
    child->_drawAnimation(draw_frame, delta_seconds, parent_looped);
}

void SsInternalPlayer::_seek_and_redraw(float frame_no, float delta_seconds, bool parent_looped) {
    const float draw_frame = _sub_frame_enabled ? frame_no : floorf(frame_no);
    previous_frame_no = draw_frame;
    _update_instance_children(draw_frame, delta_seconds, parent_looped);
    _drawAnimation(draw_frame, delta_seconds, parent_looped);
}

void SsInternalPlayer::_emit_instance_slot(const DrawFrame& /*f*/, RID ci, int p_idx, const float* slot_matrix) {
    if (p_idx < 0 || (uint32_t)p_idx >= _instance_children.size()) return;
    SsInternalPlayer* child = _instance_children[p_idx].player;
    if (!child) return;

    // Re-parent every frame: the batch CI pool can shuffle as draw_batches
    // ordering changes, so the slot CI for a given Instance part is not
    // guaranteed to be the same RID across frames.
    child->setParentCanvasItem(ci);
    child->setRootTransform(matrix_to_transform2d(slot_matrix));
    child->setRootVisible(true);
}

void SsInternalPlayer::_emit_effect_slot(const DrawFrame& f, RID ci, int p_idx, const float* slot_matrix) {
    if (p_idx < 0 || (uint32_t)p_idx >= _effect_slots.size()) return;
    EffectSlotState& slot = _effect_slots[p_idx];
    if (!slot.effect_slot) {
        f.rs->canvas_item_set_visible(ci, false);
        return;
    }

    // All per-slot effect lifecycle — event resolution, transition edges,
    // accumulator, dead-effect skip, simulator update, emitter resource
    // resolution, particle quad emission — runs inside ss_effect_slot_step.
    // The returned EffectDrawPlan carries everything Godot needs to draw:
    // commands keyed by cellmap_hash + blend, plus flat verts/uvs/colors/indices.
    const ss_effect_event_info ev = ss_runtime_get_active_effect_event(runtime_ctx, (uint32_t)p_idx);
    const float fps = _currentAnimationData->fps() > 0 ? (float)_currentAnimationData->fps() : 60.0f;
    const ss_effect_step_result step = ss_effect_slot_step(
        slot.effect_slot, ev, f.frame_no, f.delta_seconds, fps, f.parent_looped,
        /*y_flip*/ true, /*vert_stride*/ 2);
    if (!step.visible) {
        f.rs->canvas_item_set_visible(ci, false);
        return;
    }
    f.rs->canvas_item_set_visible(ci, true);

    const auto* plan = ss::runtime::GetEffectDrawPlan(step.draw_plan_buf);
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
    while ((uint32_t)slot.emitter_cis.size() < cmd_count) {
        RID e_ci = f.rs->canvas_item_create();
        slot.emitter_cis.push_back(e_ci);
    }
    for (uint32_t e = 0; e < cmd_count; e++) {
        f.rs->canvas_item_set_parent(slot.emitter_cis[e], ci);
        f.rs->canvas_item_set_draw_index(slot.emitter_cis[e], (int)e);
    }
    for (int e = (int)cmd_count; e < slot.emitter_cis.size(); e++) {
        f.rs->canvas_item_clear(slot.emitter_cis[e]);
        f.rs->canvas_item_set_visible(slot.emitter_cis[e], false);
        f.rs->canvas_item_set_parent(slot.emitter_cis[e], RID());
    }

    // PackedVector2Array stores Vector2 = (real_t, real_t). The plan emits
    // flat float pairs (vert_stride == 2), so cast the storage as float* —
    // only valid when real_t == float.
    static_assert(sizeof(Vector2) == 2 * sizeof(float),
                  "EffectDrawPlan vert_stride=2 requires Godot built with real_t == float");

    const float* V = plan->verts()->data();
    const float* U = plan->uvs()->data();
    const float* C = plan->colors()->data();
    const int32_t* I = plan->indices()->data();

    for (uint32_t e_idx = 0; e_idx < cmd_count; e_idx++) {
        const auto* cmd = plan->commands()->Get(e_idx);
        RID e_ci = slot.emitter_cis[e_idx];
        f.rs->canvas_item_clear(e_ci);

        if (!cmd || cmd->quad_count() == 0) {
            f.rs->canvas_item_set_visible(e_ci, false);
            continue;
        }

        const uint32_t cellmap_hash = cmd->cellmap_hash();
        if (cellmap_hash == 0 || !_textures.has(cellmap_hash)) {
            f.rs->canvas_item_set_visible(e_ci, false);
            continue;
        }
        Ref<Texture2D> tex = _textures[cellmap_hash];
        if (tex.is_null()) {
            f.rs->canvas_item_set_visible(e_ci, false);
            continue;
        }

        f.rs->canvas_item_set_visible(e_ci, true);
        const ss::format::BlendType blend = (cmd->blend() == 1) ? ss::format::BlendType_Add : ss::format::BlendType_Mix;
        _apply_blend_material(f.rs, e_ci, blend);

        const uint32_t qc = cmd->quad_count();
        const uint32_t off = cmd->particle_offset();
        _effect_verts.resize(qc * 4);
        _effect_uvs.resize(qc * 4);
        _effect_colors.resize(qc * 4);
        _effect_indices.resize(qc * 6);
        SsVec2Array&  p_verts   = _effect_verts;
        SsVec2Array&  p_uvs     = _effect_uvs;
        SsColorArray& p_colors  = _effect_colors;
        SsIntArray&   p_indices = _effect_indices;

        memcpy(p_verts.ptrw(),   V + off * 8,  qc * 8  * sizeof(float));
        memcpy(p_uvs.ptrw(),     U + off * 8,  qc * 8  * sizeof(float));
        memcpy(p_colors.ptrw(),  C + off * 16, qc * 16 * sizeof(float));
        memcpy(p_indices.ptrw(), I + off * 6,  qc * 6  * sizeof(int32_t));

        f.rs->canvas_item_add_triangle_array(e_ci, p_indices, p_verts, p_colors, p_uvs, {}, {}, tex->get_rid());
    }
}

int SsInternalPlayer::_build_normal(const DrawFrame& f, int p_idx,
                                    const ss::runtime::PartState* part,
                                    const float* draw_m,
                                    const Vector2& inv_tex_size,
                                    SsVec2Array& verts,
                                    SsVec2Array& uvs,
                                    SsColorArray& colors,
                                    SsFloatArray& custom0,
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
    if ((flags & ss::runtime::UpdateAttributeFlags_AttributePartColor) && partColorIndex >= 0) {
        auto pc = f.frameData->parts_color()->Get(partColorIndex);
        // SDK converter has applied SS6-style mediation; ssab fields now carry
        // a consistent semantic regardless of the source (blend_type, target):
        //   pc->lt/rt/lb/rb->rgba.a  = rateAlpha (final-alpha source, u8)
        //   pc->lt/rt/lb/rb->rate    = colorA    (blend weight, float)
        auto to_color = [part_alpha](const ss::runtime::SsAttributePartColorKeyValueColor& c) {
            return Color(c.rgba().r()/255.0f, c.rgba().g()/255.0f, c.rgba().b()/255.0f,
                         (c.rgba().a()/255.0f) * part_alpha);
        };
        corner_colors[0] = to_color(pc->lt());
        corner_colors[1] = to_color(pc->rt());
        corner_colors[2] = to_color(pc->lb());
        corner_colors[3] = to_color(pc->rb());
        corner_rates[0] = pc->lt().rate();
        corner_rates[1] = pc->rt().rate();
        corner_rates[2] = pc->lb().rate();
        corner_rates[3] = pc->rb().rate();
        // PartColor blend_type is 0..3 (Mix/Mul/Add/Sub). Higher SS7 values are
        // GPU framebuffer blends, not PartColor formulas — clamp defensively.
        blend_idx = (int)pc->blend_type();
        if (blend_idx < 0 || blend_idx > 3) blend_idx = 0;
    }

    // Output PMA flag — parked at 0 for now; wired so the shader path is
    // ready when the host turns PMA on globally or per texture.
    const float pma_flag = 0.0f;

    Vector2* v_ptr = verts.ptrw();
    Vector2* u_ptr = uvs.ptrw();
    Color*   c_ptr = colors.ptrw();
    float*   x_ptr = custom0.ptrw();

    for (int j = 0; j < CORNERS_COUNT; j++) {
        v_ptr[vbase + j] = xform_raw(draw_m, out_x[j], out_y[j]);
        u_ptr[vbase + j] = Vector2(out_u[j] * inv_tex_size.x, out_v[j] * inv_tex_size.y);
        c_ptr[vbase + j] = corner_colors[j];
        const int c0 = (vbase + j) * 4;
        x_ptr[c0 + 0] = corner_rates[j];
        x_ptr[c0 + 1] = (float)blend_idx;
        x_ptr[c0 + 2] = pma_flag;
        x_ptr[c0 + 3] = 0.0f;
    }
    if (needs_center) {
        v_ptr[vbase + CORNERS_COUNT] = xform_raw(draw_m, out_x[CORNERS_COUNT], out_y[CORNERS_COUNT]);
        u_ptr[vbase + CORNERS_COUNT] = Vector2(out_u[CORNERS_COUNT] * inv_tex_size.x, out_v[CORNERS_COUNT] * inv_tex_size.y);
        c_ptr[vbase + CORNERS_COUNT] = (corner_colors[0] + corner_colors[1] + corner_colors[2] + corner_colors[3]) * 0.25f;
        const float center_rate = (corner_rates[0] + corner_rates[1] + corner_rates[2] + corner_rates[3]) * 0.25f;
        const int c0 = (vbase + CORNERS_COUNT) * 4;
        x_ptr[c0 + 0] = center_rate;
        x_ptr[c0 + 1] = (float)blend_idx;
        x_ptr[c0 + 2] = pma_flag;
        x_ptr[c0 + 3] = 0.0f;
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
    const uint64_t key = make_partcolor_cache_key(shader_id_hash, blend_type);
    PerPartMaterialPool& pool = _per_part_material_pools[key];
    if (pool.in_use >= pool.materials.size()) {
        Ref<ShaderMaterial> mat; mat.instantiate();
        mat->set_shader(_ensure_partcolor_shader(shader_id_hash, blend_type));
        pool.materials.push_back(mat);
    }
    return pool.materials[pool.in_use++];
}

RID SsInternalPlayer::_acquire_per_part_canvas_item() {
    RenderingServer* rs = RenderingServer::get_singleton();
    if (_per_part_canvas_items_in_use >= _per_part_canvas_items.size()) {
        RID ci = rs->canvas_item_create();
        rs->canvas_item_set_parent(ci, _root_ci);
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
    // Debug override: when set, force the dispatch through a specific catalog
    // entry. Useful while verifying the per-part path before authoring .sspj
    // content that actually exercises a custom shader id.
    if (_test_shader_id_hash_override != 0) {
        psi.id_hash = _test_shader_id_hash_override;
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
    // Only Mix/Add/Sub/Mul are supported as GPU-side framebuffer blend modes
    // here; any other batch blend_type falls back to Mix at the material level
    // (the rest of the 12 SS7 blends are deferred — see ROADMAP). The
    // per-vertex CUSTOM0 still drives PartColor compositing regardless.
    ss::format::BlendType resolved = ss_blend;
    switch (ss_blend) {
        case ss::format::BlendType_Mix:
        case ss::format::BlendType_Add:
        case ss::format::BlendType_Sub:
        case ss::format::BlendType_Mul:
            break;
        default:
            resolved = ss::format::BlendType_Mix;
            break;
    }
    const uint64_t key = make_partcolor_cache_key(shader_id_hash, resolved);
    if (!_partcolor_materials.has(key)) {
        Ref<ShaderMaterial> mat; mat.instantiate();
        mat->set_shader(_ensure_partcolor_shader(shader_id_hash, resolved));
        _partcolor_materials[key] = mat;
    }
    rs->canvas_item_set_material(ci, _partcolor_materials[key]->get_rid());
}

void SsInternalPlayer::_emit_partcolor_mesh(RenderingServer* rs, RID ci,
                                            const SsIntArray& indices,
                                            const SsVec2Array& verts,
                                            const SsColorArray& colors,
                                            const SsVec2Array& uvs,
                                            const SsFloatArray& custom0,
                                            const RID& texture_rid) {
    Ref<ArrayMesh> mesh; mesh.instantiate();
    Array arrays;
    arrays.resize(Mesh::ARRAY_MAX);
    arrays[Mesh::ARRAY_VERTEX] = verts;
    arrays[Mesh::ARRAY_TEX_UV] = uvs;
    arrays[Mesh::ARRAY_COLOR]  = colors;
    arrays[Mesh::ARRAY_CUSTOM0] = custom0;
    arrays[Mesh::ARRAY_INDEX]  = indices;
    // CUSTOM0 carries 4 floats per vertex (ARRAY_CUSTOM_RGBA_FLOAT). The 2D
    // vertex flag is auto-detected from the PackedVector2Array contents.
    const uint64_t flags = (uint64_t)Mesh::ARRAY_CUSTOM_RGBA_FLOAT << Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT;
    mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays, Array(), Dictionary(), flags);
    _frame_meshes.push_back(mesh);
    rs->canvas_item_add_mesh(ci, mesh->get_rid(), Transform2D(), Color(1, 1, 1, 1), texture_rid);
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
    if ((flags & ss::runtime::UpdateAttributeFlags_AttributePartColor) && partColorIndex >= 0) {
        auto pc = f.frameData->parts_color()->Get(partColorIndex);
        // SDK-mediated semantic — see _build_normal for the field meanings.
        // Shape paths additionally get the (blend_type == Mix → rgba.a = 255)
        // override applied at convert time, so multiplying by part_alpha here
        // gives `vertex.a = part_alpha` for Mix shapes and `vertex.a = dataA ×
        // part_alpha` for non-Mix shapes — both matching the SS shape spec.
        auto to_color = [part_alpha](const ss::runtime::SsAttributePartColorKeyValueColor& c) {
            return Color(c.rgba().r()/255.0f, c.rgba().g()/255.0f, c.rgba().b()/255.0f,
                         (c.rgba().a()/255.0f) * part_alpha);
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

    out.verts.resize(part_shape_count);
    out.uvs.resize(part_shape_count);
    out.colors.resize(part_shape_count);
    out.custom0.resize(part_shape_count * 4);
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

    if (part_shape_count == 4) {
        const int idx[6] = { 0,1,2, 1,3,2 };
        out.indices.resize(6);
        int32_t* i_ptr = (int32_t*)out.indices.ptrw();
        for (int i = 0; i < 6; i++) i_ptr[i] = idx[i];
    } else {
        const int tri_count = part_shape_count - 2;
        out.indices.resize(tri_count * 3);
        int32_t* i_ptr = (int32_t*)out.indices.ptrw();
        for (int i = 0; i < tri_count; i++) {
            i_ptr[i*3 + 0] = 0;
            i_ptr[i*3 + 1] = i + 1;
            i_ptr[i*3 + 2] = i + 2;
        }
    }
    return true;
}

void SsInternalPlayer::_emit_normal_batch(const DrawFrame& f, RID ci,
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

    _normal_verts.resize((int)batch->vertex_count());
    _normal_uvs.resize((int)batch->vertex_count());
    _normal_colors.resize((int)batch->vertex_count());
    _normal_custom0.resize((int)batch->vertex_count() * 4);
    _normal_indices.resize((int)batch->index_count());
    SsVec2Array&  verts   = _normal_verts;
    SsVec2Array&  uvs     = _normal_uvs;
    SsColorArray& colors  = _normal_colors;
    SsFloatArray& custom0 = _normal_custom0;
    SsIntArray&   indices = _normal_indices;
    int32_t* indices_ptr = (int32_t*)indices.ptrw();

    int vbase = 0;
    int ibase = 0;
    bool any_default_emitted = false;

    // batch->blend_type() is the runtime BlendType; converted to ssab BlendType
    // by raw u8 value (the two enums are layout-equivalent, see chapter 9 §7).
    const auto ssab_blend = (ss::format::BlendType)batch->blend_type();
    const RID tex_rid = tex->get_rid();

    // Pre-size the per-part scratch buffers to fit one Normal part at max
    // (5 verts, 12 indices). Resized once here, reused across per-part emits.
    _per_part_normal_verts.resize(MAX_VERTICES_COUNT);
    _per_part_normal_uvs.resize(MAX_VERTICES_COUNT);
    _per_part_normal_colors.resize(MAX_VERTICES_COUNT);
    _per_part_normal_custom0.resize(MAX_VERTICES_COUNT * 4);
    _per_part_normal_indices.resize(INDICES_COUNT_PENTAGON);

    const bool masking_active = _mask_coverage_valid;

    for (uint16_t k = 0; k < count; k++) {
        int p_idx = (int)draw_order_data[batch->start_rank() + k];
        if (p_idx < 0 || p_idx >= (int)_parts_by_idx.size()) continue;
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
        bool vis_inside = false;
        bool mask_target = false;
        if (masking_active) {
            auto pm = f.binary ? f.binary->parts() : nullptr;
            if (pm && p_idx >= 0 && p_idx < (int)pm->size()) {
                const auto* pdv = pm->Get(p_idx);
                if (pdv) {
                    vis_inside = pdv->visible_inside_mask();
                    mask_target = pdv->mask_influence() || pdv->mask_write();
                }
            }
        }
        const bool masked = masking_active && mask_target && (_part_in_mask_scope(rank) || vis_inside);

        PartShaderInfo psi = _resolve_part_shader_info(f, part);
        if (psi.is_per_part || masked) {
            // Per-part path: build into the small scratch buffers (vbase=0),
            // acquire dedicated canvas_item + ShaderMaterial, emit one mesh.
            const int vc = _build_normal(f, p_idx, part, drawing_m, inv_tex_size,
                                         _per_part_normal_verts, _per_part_normal_uvs,
                                         _per_part_normal_colors, _per_part_normal_custom0, 0);
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
            // Trim the index buffer for this emit; _emit_partcolor_mesh reads
            // size() so a stale-tail Pentagon footprint would emit phantom
            // triangles for a 4-vert Quad part.
            _per_part_normal_indices.resize(idx_count);
            _per_part_normal_verts.resize(vc);
            _per_part_normal_uvs.resize(vc);
            _per_part_normal_colors.resize(vc);
            _per_part_normal_custom0.resize(vc * 4);

            RID part_ci = _acquire_per_part_canvas_item();
            Ref<ShaderMaterial> mat = _acquire_per_part_material(psi.id_hash, ssab_blend);
            const Vector4 cell_rect = _resolve_cell_rect_uv(f, p_idx, inv_tex_size);
            _apply_per_part_uniforms(mat, psi.params, psi.map0, psi.map1, cell_rect);
            if (masked) {
                _apply_mask_uniforms(mat, rank, vis_inside);
            } else {
                // Reuse of a pooled material — make sure masking is off.
                mat->set_shader_parameter("ss_mask_enabled", false);
            }
            rs->canvas_item_set_material(part_ci, mat->get_rid());
            rs->canvas_item_set_transform(part_ci, Transform2D());
            _emit_partcolor_mesh(rs, part_ci,
                                 _per_part_normal_indices, _per_part_normal_verts,
                                 _per_part_normal_colors, _per_part_normal_uvs,
                                 _per_part_normal_custom0, tex_rid);

            // Restore per-part scratch buffers to max footprint for the next
            // per-part emit in this batch.
            _per_part_normal_verts.resize(MAX_VERTICES_COUNT);
            _per_part_normal_uvs.resize(MAX_VERTICES_COUNT);
            _per_part_normal_colors.resize(MAX_VERTICES_COUNT);
            _per_part_normal_custom0.resize(MAX_VERTICES_COUNT * 4);
            _per_part_normal_indices.resize(INDICES_COUNT_PENTAGON);
            continue;
        }

        // Default / shared path: accumulate into the batch arrays.
        const int vert_count = _build_normal(f, p_idx, part, drawing_m, inv_tex_size,
                                             verts, uvs, colors, custom0, vbase);
        if (vert_count == 0) continue;

        // Append per-part indices with vertex base offset.
        if (vert_count == MAX_VERTICES_COUNT) {
            const int p[INDICES_COUNT_PENTAGON] = { 0,1,4, 1,3,4, 3,2,4, 2,0,4 };
            for (int j = 0; j < INDICES_COUNT_PENTAGON && ibase + j < indices.size(); j++) {
                indices_ptr[ibase + j] = vbase + p[j];
            }
            ibase += INDICES_COUNT_PENTAGON;
        } else {
            const int q[INDICES_COUNT_QUAD] = { 0,1,2, 1,3,2 };
            for (int j = 0; j < INDICES_COUNT_QUAD && ibase + j < indices.size(); j++) {
                indices_ptr[ibase + j] = vbase + q[j];
            }
            ibase += INDICES_COUNT_QUAD;
        }
        vbase += vert_count;
        any_default_emitted = true;
    }

    if (!any_default_emitted) {
        // Pure per-part batch (every part took the per-part route). The batch
        // canvas_item has nothing to draw; clear any stale commands.
        rs->canvas_item_clear(ci);
        return;
    }

    // Shrink the accumulator to exactly what the Default-pass produced. The
    // batch's vertex_count / index_count were sized for "all parts go through
    // the shared mesh"; if some took the per-part path those slots are unused.
    indices.resize(ibase);
    verts.resize(vbase);
    uvs.resize(vbase);
    colors.resize(vbase);
    custom0.resize(vbase * 4);

    _apply_partcolor_material(rs, ci, s_default_shader_id_hash, ssab_blend);
    rs->canvas_item_set_transform(ci, Transform2D());
    _emit_partcolor_mesh(rs, ci, indices, verts, colors, uvs, custom0, tex_rid);
}

void SsInternalPlayer::_emit_shape_batch(const DrawFrame& f, RID ci,
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
    bool batch_material_applied = false;

    for (uint16_t k = 0; k < count; k++) {
        int p_idx = (int)draw_order_data[batch->start_rank() + k];
        if (p_idx < 0 || p_idx >= (int)_parts_by_idx.size()) continue;
        const auto* part = _parts_by_idx[p_idx];
        if (!part) continue;

        const float* drawing_m = f.get_world_matrix(p_idx);
        if (!drawing_m) continue;

        if (!_build_shape_geometry(f, p_idx, part, drawing_m, _shape_buf)) continue;
        // No texture bound; the canvas_item shader's TEXTURE sampler then
        // returns white (Godot's default) for any UV. The PartColor formula
        // therefore composites against an implicit white "shape pixel".

        PartShaderInfo psi = _resolve_part_shader_info(f, part);
        if (psi.is_per_part) {
            RID part_ci = _acquire_per_part_canvas_item();
            Ref<ShaderMaterial> mat = _acquire_per_part_material(psi.id_hash, ssab_blend);
            // Shape parts have no bound cellmap texture; pass a zero
            // cell_rect (degenerates ss-circle / ss-spot to discard, which
            // is the safest fallback when the variant is misapplied to a Shape).
            _apply_per_part_uniforms(mat, psi.params, psi.map0, psi.map1, Vector4(0, 0, 0, 0));
            f.rs->canvas_item_set_material(part_ci, mat->get_rid());
            _emit_partcolor_mesh(f.rs, part_ci, _shape_buf.indices, _shape_buf.verts,
                                 _shape_buf.colors, _shape_buf.uvs, _shape_buf.custom0,
                                 RID());
        } else {
            if (!batch_material_applied) {
                _apply_partcolor_material(f.rs, ci, s_default_shader_id_hash, ssab_blend);
                batch_material_applied = true;
            }
            _emit_partcolor_mesh(f.rs, ci, _shape_buf.indices, _shape_buf.verts,
                                 _shape_buf.colors, _shape_buf.uvs, _shape_buf.custom0,
                                 RID());
        }
    }
    if (!batch_material_applied) {
        // Every Shape part in this batch took the per-part route. Clear stale
        // commands on the batch CI so the previous frame's draws don't linger.
        f.rs->canvas_item_clear(ci);
    }
}

void SsInternalPlayer::_emit_mesh_batch(const DrawFrame& f, RID ci,
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
    bool batch_material_applied = false;
    const bool masking_active = _mask_coverage_valid;

    for (uint16_t k = 0; k < count; k++) {
        int p_idx = (int)draw_order_data[batch->start_rank() + k];
        if (p_idx < 0 || p_idx >= (int)_parts_by_idx.size()) continue;
        const auto* part = _parts_by_idx[p_idx];
        if (!part) continue;

        if (!_build_mesh_geometry(f, p_idx, part, inv_tex_size, _mesh_buf)) continue;

        // CBP masking: a masked target must run the mask test. A part opts out
        // with mask_influence=0; write_mask clipping parts are always affected.
        const uint16_t rank = (uint16_t)(batch->start_rank() + k);
        bool vis_inside = false;
        bool mask_target = false;
        if (masking_active) {
            auto pm = f.binary ? f.binary->parts() : nullptr;
            if (pm && p_idx >= 0 && p_idx < (int)pm->size()) {
                const auto* pdv = pm->Get(p_idx);
                if (pdv) {
                    vis_inside = pdv->visible_inside_mask();
                    mask_target = pdv->mask_influence() || pdv->mask_write();
                }
            }
        }
        const bool masked = masking_active && mask_target && (_part_in_mask_scope(rank) || vis_inside);

        PartShaderInfo psi = _resolve_part_shader_info(f, part);
        if (psi.is_per_part || masked) {
            RID part_ci = _acquire_per_part_canvas_item();
            Ref<ShaderMaterial> mat = _acquire_per_part_material(psi.id_hash, ssab_blend);
            const Vector4 cell_rect = _resolve_cell_rect_uv(f, p_idx, inv_tex_size);
            _apply_per_part_uniforms(mat, psi.params, psi.map0, psi.map1, cell_rect);
            if (masked) {
                _apply_mask_uniforms(mat, rank, vis_inside);
            } else {
                mat->set_shader_parameter("ss_mask_enabled", false);
            }
            f.rs->canvas_item_set_material(part_ci, mat->get_rid());
            _emit_partcolor_mesh(f.rs, part_ci, _mesh_buf.indices, _mesh_buf.verts,
                                 _mesh_buf.colors, _mesh_buf.uvs, _mesh_buf.custom0,
                                 tex_rid);
        } else {
            if (!batch_material_applied) {
                _apply_partcolor_material(f.rs, ci, s_default_shader_id_hash, ssab_blend);
                batch_material_applied = true;
            }
            _emit_partcolor_mesh(f.rs, ci, _mesh_buf.indices, _mesh_buf.verts,
                                 _mesh_buf.colors, _mesh_buf.uvs, _mesh_buf.custom0,
                                 tex_rid);
        }
    }
    if (!batch_material_applied) {
        f.rs->canvas_item_clear(ci);
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
    out.verts.resize(vert_count);
    out.uvs.resize(vert_count);
    out.colors.resize(vert_count);
    out.custom0.resize(vert_count * 4);
    out.indices.resize(index_count);

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
    if ((flags & ss::runtime::UpdateAttributeFlags_AttributePartColor) && partColorIndex >= 0) {
        auto pc = f.frameData->parts_color()->Get(partColorIndex);
        auto to_color = [part_alpha](const ss::runtime::SsAttributePartColorKeyValueColor& c) {
            return Color(c.rgba().r()/255.0f, c.rgba().g()/255.0f, c.rgba().b()/255.0f,
                         (c.rgba().a()/255.0f) * part_alpha);
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

    // Triangle indices are part-local 0-based; this is a singleton batch, so
    // no vertex-base rebasing is required.
    int32_t* i_ptr = (int32_t*)out.indices.ptrw();
    const int32_t* src = f.mesh_indices + i0;
    for (int i = 0; i < index_count; i++) {
        i_ptr[i] = src[i];
    }
    return true;
}

void SsInternalPlayer::_fetchAnimation() {
    if (_strAnimationSelected.is_empty() || _ssabRes.is_null()) {
        ss_runtime_reset(runtime_ctx);
        _reconfigure();
        if (runtime_res != nullptr) {
            ss_resource_destroy(runtime_res);
            runtime_res = nullptr;
        }
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
        return;
    }

    _clear_instance_children();
    _clear_effect_slots();
    _clear_batch_canvas_items();
    _free_per_part_canvas_items();

    if (runtime_res != nullptr) {
        ss_resource_destroy(runtime_res);
        runtime_res = nullptr;
    }

    runtime_res = ss_resource_create_borrow(_ssabRes->get_data_ptr(), _ssabRes->get_data_size());
    if (runtime_res == nullptr) {
        ERR_PRINT("SSAB Resource Create Failed");
        return;
    }

    bool binded = ss_runtime_bind_resource(runtime_ctx, runtime_res);
    if (!binded) {
        ERR_PRINT("SSAB Resource Bind Failed");
        return;
    }

    // Per-batch canvas_item pool grows on demand inside _drawAnimation via
    // _ensure_batch_ci(). No per-part CIs are pre-allocated here.

    auto c = _strAnimationSelected.utf8();
    auto animation = _ssabRes->find_animation(_strAnimationSelected);
    if (!animation) {
        ERR_PRINT("Select Anime is Null");
        return;
    }
    _currentAnimationData = animation;
    bool setup = ss_runtime_setup_animation(runtime_ctx, c.get_data());
    if (!setup) {
        ERR_PRINT("SSAB Setup Animation Failed: " + _strAnimationSelected);
        return;
    }

    // Recursive setup — instance children also populate their own
    // `_instance_children`, so any depth of nested Instance parts is
    // supported. Cross-SSAB cycles (A → B → A authoring mistakes) are not
    // detected; they recurse until stack overflow on load. Trust the
    // converter / authoring tool to keep references acyclic.
    _setup_instance_children();
    _setup_effect_slots();

    _seek_and_redraw(ss_runtime_get_frame_no(runtime_ctx), 0.0f, false);
}
