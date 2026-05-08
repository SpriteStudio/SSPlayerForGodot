#include "ss_internal_player.h"
#include "format/ssab.h"
#include "ssruntime.h"
#include "format/framedata.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#else
#include "core/io/resource_loader.h"
#include "servers/rendering/rendering_server.h"
#endif


SsInternalPlayer::SsInternalPlayer() {
    runtime_ctx = ss_runtime_create();
    _reconfigure();

    // _root_ci anchors all per-batch canvas items so the host can move /
    // hide / re-parent the whole player without touching individual batches.
    // Re-parenting happens via setParentCanvasItem; until then it floats
    // free of any canvas (which is fine — invisible until attached).
    RenderingServer* rs = RenderingServer::get_singleton();
    _root_ci = rs->canvas_item_create();
}

SsInternalPlayer::~SsInternalPlayer() {
    _clear_instance_players();
    _clear_batch_canvas_items();

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

    // Resolve external instance dependencies before _setup_instance_players
    // (called from _fetchAnimation below) needs to find ref animations that
    // live in sibling .ssab files.
    if (!_instance_child_mode) {
        _load_external_ssabs();
    }

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
    _strAnimationSelected = strName;
    _fetchAnimation();
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
        float frame_no = ss_runtime_get_frame_no(runtime_ctx);
        float draw_frame = _sub_frame_enabled ? frame_no : floorf(frame_no);
        previous_frame_no = draw_frame;
        _drawAnimation(draw_frame);
    }
}

void SsInternalPlayer::setFrameRelative(float p_diff) {
    if (runtime_ctx) {
        ss_runtime_set_frame_relative(runtime_ctx, p_diff);
        float frame_no = ss_runtime_get_frame_no(runtime_ctx);
        float draw_frame = _sub_frame_enabled ? frame_no : floorf(frame_no);
        previous_frame_no = draw_frame;
        _drawAnimation(draw_frame);
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
        float frame_no = ss_runtime_get_frame_no(runtime_ctx);
        float draw_frame = _sub_frame_enabled ? frame_no : floorf(frame_no);
        previous_frame_no = draw_frame;
        _drawAnimation(draw_frame);
    }
}

bool SsInternalPlayer::isSubFrameEnabled() const {
    return _sub_frame_enabled;
}

void SsInternalPlayer::setInstanceChildMode(bool p_enabled) {
    _instance_child_mode = p_enabled;
}

void SsInternalPlayer::setRootTransform(const Transform2D& p_xf) {
    RenderingServer* rs = RenderingServer::get_singleton();
    rs->canvas_item_set_transform(_root_ci, p_xf);
}

void SsInternalPlayer::setRootVisible(bool p_visible) {
    RenderingServer* rs = RenderingServer::get_singleton();
    rs->canvas_item_set_visible(_root_ci, p_visible);
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

    Transform2D matrix_to_transform2d(const float* m) {
        Transform2D t;
        t.columns[0] = Vector2(m[0], m[1]);
        t.columns[1] = Vector2(m[4], m[5]);
        t.columns[2] = Vector2(m[12], m[13]);
        return t;
    }
}

void SsInternalPlayer::update(float delta_seconds) {
    // Instance-child players are advanced by the parent via setFrameRelative;
    // they must not run their own controller tick.
    if (_instance_child_mode) return;
    if (!ss_runtime_is_playing(runtime_ctx)) return;

    auto d = delta_seconds * 1000.0f;
    float frame_no = ss_runtime_update(runtime_ctx, d);

    if (ss_runtime_is_looped(runtime_ctx)) {
        if (_event_sink) _event_sink->onAnimationLooped(_strAnimationSelected);
    }
    if (ss_runtime_is_end_frame_reached(runtime_ctx)) {
        if (_event_sink) _event_sink->onAnimationFinished(_strAnimationSelected);
    }

    float draw_frame = _sub_frame_enabled ? frame_no : floorf(frame_no);
    if (previous_frame_no == draw_frame) return;

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

                    int flag = 0;
                    int int_val = 0;
                    Rect2 rect_val;
                    Vector2 point_val;
                    String str_val;

                    if (val->integer()) {
                        flag |= 1;
                        int_val = val->integer()->value();
                    }
                    if (val->rect()) {
                        flag |= 2;
                        rect_val = Rect2(val->rect()->x1(), val->rect()->y1(), val->rect()->x2() - val->rect()->x1(), val->rect()->y2() - val->rect()->y1());
                    }
                    if (val->point()) {
                        flag |= 4;
                        point_val = Vector2(val->point()->v1(), val->point()->v2());
                    }
                    if (val->str()) {
                        flag |= 8;
                        str_val = String::utf8(val->str()->c_str());
                    }

                    if (_event_sink) _event_sink->onUserData(flag, int_val, rect_val, point_val, str_val);
                }
            }

            if (auto audios = events_per_frame->audios()) {
                // TODO: Audio integration
            }
        }
    }

    previous_frame_no = draw_frame;
    _drawAnimation(draw_frame);
}

void SsInternalPlayer::_drawAnimation(float frame_no) {
    unsigned char* data = nullptr;
    uintptr_t len = 0;
    ss_runtime_get_frame_data(runtime_ctx, frame_no, &data, &len);
    if (!data) return;

    DrawFrame f = {};
    f.rs = RenderingServer::get_singleton();
    f.frameData = ss::runtime::GetFrameData(data);
    f.binary = _ssabRes->get_ss_anime_binary();
    f.frame_no = frame_no;
    ss_runtime_get_world_matrices(runtime_ctx, &f.world_matrices, &f.world_matrices_len);
    ss_runtime_get_local_uvs(runtime_ctx, &f.local_uvs, &f.local_uvs_len);
    ss_runtime_get_cell_meta(runtime_ctx, &f.cell_meta, &f.cell_meta_len);
    ss_runtime_get_local_vertices(runtime_ctx, &f.local_vertices, &f.local_vertices_len);
    ss_runtime_get_shape_vertices(runtime_ctx, &f.shape_vertices, &f.shape_vertices_len);
    ss_runtime_get_shape_vertex_box_coords(runtime_ctx, &f.shape_box_coords, &f.shape_box_coords_len);
    ss_runtime_get_shape_vertex_counts(runtime_ctx, &f.shape_vertex_counts, &f.shape_vertex_counts_len);

    auto parts = f.frameData->parts();
    auto draw_order = f.frameData->draw_order();
    auto draw_batches = f.frameData->draw_batches();
    if (!parts || !draw_order || !draw_batches) return;

    {
        const int total = f.binary->parts() ? (int)f.binary->parts()->size() : 0;
        _parts_by_idx.resize(total);
        for (int i = 0; i < total; i++) _parts_by_idx[i] = nullptr;
        for (uint32_t i = 0; i < parts->size(); i++) {
            auto p = parts->Get(i);
            int idx = p->part_index();
            if (idx >= 0 && idx < total) _parts_by_idx[idx] = p;
        }
    }

    const uint16_t* draw_order_data = draw_order->data();
    const uint32_t batch_count = draw_batches->size();

    // Hide / clear unused entries in the pool (peak-retain policy).
    for (int i = (int)batch_count; i < _batch_canvas_items.size(); i++) {
        f.rs->canvas_item_clear(_batch_canvas_items[i]);
        f.rs->canvas_item_set_visible(_batch_canvas_items[i], false);
    }

    for (uint32_t bi = 0; bi < batch_count; bi++) {
        const auto* batch = draw_batches->Get(bi);
        RID ci = _ensure_batch_ci((int)bi);
        f.rs->canvas_item_clear(ci);
        f.rs->canvas_item_set_visible(ci, true);
        f.rs->canvas_item_set_z_index(ci, (int)bi);

        const auto kind = batch->kind();
        if (kind == ss::runtime::DrawBatchKind_Normal) {
            _emit_normal_batch(f, ci, batch, draw_order_data);
        } else if (kind == ss::runtime::DrawBatchKind_Shape) {
            int p_idx = (int)draw_order_data[batch->start_rank()];
            const auto* part = (p_idx >= 0 && p_idx < (int)_parts_by_idx.size()) ? _parts_by_idx[p_idx] : nullptr;
            if (part) _emit_shape_singleton(f, ci, p_idx, part);
        } else if (kind == ss::runtime::DrawBatchKind_Instance) {
            int p_idx = (int)draw_order_data[batch->start_rank()];
            const auto* part = (p_idx >= 0 && p_idx < (int)_parts_by_idx.size()) ? _parts_by_idx[p_idx] : nullptr;
            if (!part) continue;
            const float* drawing_m = (f.world_matrices && (uintptr_t)p_idx * 16 < f.world_matrices_len)
                ? f.world_matrices + (p_idx * 16) : nullptr;
            if (!drawing_m) continue;
            auto partBinary = f.binary->parts()->Get(p_idx);
            _draw_part_instance(f, ci, p_idx, part, partBinary, drawing_m);
        }
        // DrawBatchKind_Effect / Mesh / Text / Nines / Mask: not yet implemented.
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

void SsInternalPlayer::_clear_instance_players() {
    for (int i = 0; i < _instance_players.size(); i++) {
        SsInternalPlayer* child = _instance_players[i];
        if (child) {
            memdelete(child);
        }
    }
    _instance_players.clear();
}

void SsInternalPlayer::_setup_instance_players() {
    _clear_instance_players();
    if (_ssabRes.is_null()) return;
    auto binary = _ssabRes->get_ss_anime_binary();
    if (!binary || !binary->parts()) return;

    auto parts = binary->parts();
    _instance_players.resize(parts->size());
    for (int i = 0; i < (int)parts->size(); i++) {
        _instance_players.set(i, nullptr);
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
        child->setInstanceChildMode(true);
        // Hand the child the SSAB that actually contains the referenced
        // animation — may be `_ssabRes` itself or an external sibling.
        child->setSSABResource(source);
        child->setAnimation(anim_name);
        child->stop();
        // Keep the child hidden by default; _draw_part_instance toggles it
        // visible only when an EventInstance / InitialEvent triggers it.
        child->setRootVisible(false);
        // Parenting under the slot's batch canvas_item is performed each
        // frame in _draw_part_instance — batch CIs are recyclable so a
        // setup-time parenting wouldn't survive batch-list shifts.
        _instance_players.set(i, child);
    }
}

void SsInternalPlayer::_draw_part_instance(const DrawFrame& f, RID ci, int p_idx, const ss::runtime::PartState* part, const ss::format::PartData* partBinary, const float* draw_m) {
    if (p_idx < 0 || p_idx >= _instance_players.size()) return;
    SsInternalPlayer* child = _instance_players[p_idx];
    if (!child) return;

    // Re-parent the child's root canvas_item under this Instance batch's CI
    // every frame: the batch CI pool may shuffle as draw_batches changes
    // ordering, so the slot CI for a given Instance part is not guaranteed
    // to be the same RID across frames.
    child->setParentCanvasItem(ci);

    if (!_currentAnimationData) {
        child->setRootVisible(false);
        return;
    }
    int parent_frame_int = (int)f.frame_no;

    const ss::format::PartAttributeInstance* active_attr = nullptr;
    int active_event_frame = 0;
    if (auto events = _currentAnimationData->events()) {
        for (int i = (int)events->size() - 1; i >= 0; i--) {
            auto epf = events->Get(i);
            if (!epf) continue;
            int frame_index = epf->frame_index();
            if (frame_index > parent_frame_int) continue;
            if (!epf->instances()) continue;
            for (uint32_t j = 0; j < epf->instances()->size(); j++) {
                auto ev = epf->instances()->Get(j);
                if (!ev || ev->part_index() != (uint16_t)p_idx) continue;
                active_attr = ev->value();
                active_event_frame = frame_index;
                break;
            }
            if (active_attr) break;
        }
    }
    // Default playback config used when no EventInstance has fired yet for this
    // slot. Matches the default-constructed `SsInstanceAttr` in SS6
    // (sstypes.h:1294): loopNum=1, infinity=false, full "_start".."_end"
    // range, speed=1, curKeyframe=0. This makes the instance play through
    // once and clamp at end_frame (animedecode.cpp:1670 clamps `reftime` to
    // `inst_scale - 1` once `nowloop >= loopNum`), rather than looping.
    int child_total = child->getTotalFrames();
    int default_end = child_total > 0 ? child_total - 1 : 0;

    int start_frame = 0;
    int end_frame = default_end;
    int loops = 1;
    bool pingpong = false;
    bool reverse = false;
    float speed = 1.0f;

    if (active_attr) {
        // The child has already resolved its selected animation in
        // _fetchAnimation(); reuse that pointer instead of scanning the child's
        // animations array by name. Labels are sorted by name_hash (ssab schema
        // marks `Label.name_hash (key)`), so LookupByKey gives us O(log n).
        const ss::format::AnimationData* child_anim = child->getCurrentAnimationData();
        auto resolve_label = [&](uint32_t label_hash, int fallback) -> int {
            if (label_hash == 0) return fallback;
            if (!child_anim || !child_anim->labels()) return fallback;
            const auto* lab = child_anim->labels()->LookupByKey(label_hash);
            return lab ? lab->time() : fallback;
        };

        start_frame = resolve_label(active_attr->start_label_hash(), 0) + active_attr->start_offset();
        end_frame = resolve_label(active_attr->end_label_hash(), default_end) + active_attr->end_offset();
        if (end_frame < start_frame) end_frame = start_frame;

        loops = active_attr->loop_num();
        pingpong = active_attr->pingpong();
        reverse = active_attr->reverse();
        speed = active_attr->speed();
    }

    child->setAnimationSection(start_frame, end_frame);
    child->setLoop(loops);
    child->setPlaybackDirection(reverse ? 1 : 0, pingpong ? 1 : 0);

    float diff = (f.frame_no - (float)active_event_frame) * speed;
    Transform2D xf = matrix_to_transform2d(draw_m);
    child->setRootTransform(xf);
    child->setRootVisible(true);
    child->setFrameRelative(diff);
}

int SsInternalPlayer::_build_normal(const DrawFrame& f, int p_idx,
                                    const ss::runtime::PartState* part,
                                    const float* draw_m,
                                    const Vector2& tex_size,
                                    SsVec2Array& verts,
                                    SsVec2Array& uvs,
                                    SsColorArray& colors,
                                    int vbase)
{
    const float* part_cell_meta = nullptr;
    if (f.cell_meta && (uintptr_t)p_idx * 6 + 6 <= f.cell_meta_len) {
        part_cell_meta = f.cell_meta + (p_idx * 6);
    }
    const float* part_uvs = nullptr;
    if (f.local_uvs && (uintptr_t)p_idx * 10 + 10 <= f.local_uvs_len) {
        part_uvs = f.local_uvs + (p_idx * 10);
    }
    const float* part_verts = nullptr;
    if (f.local_vertices && (uintptr_t)p_idx * 10 + 10 <= f.local_vertices_len) {
        part_verts = f.local_vertices + (p_idx * 10);
    }
    if (!part_cell_meta || !part_uvs || !part_verts) return 0;

    const uint64_t flags = part->update_flag();
    const bool needs_center = (flags & (ss::runtime::UpdateAttributeFlags_AttributeVertex | ss::runtime::UpdateAttributeFlags_AttributePartColor)) != 0;
    const int vert_count = needs_center ? MAX_VERTICES_COUNT : CORNERS_COUNT;

    const float out_x[MAX_VERTICES_COUNT] = { part_verts[0], part_verts[2], part_verts[4], part_verts[6], part_verts[8] };
    const float out_y[MAX_VERTICES_COUNT] = { part_verts[1], part_verts[3], part_verts[5], part_verts[7], part_verts[9] };
    const float out_u[MAX_VERTICES_COUNT] = { part_uvs[0], part_uvs[2], part_uvs[4], part_uvs[6], part_uvs[8] };
    const float out_v[MAX_VERTICES_COUNT] = { part_uvs[1], part_uvs[3], part_uvs[5], part_uvs[7], part_uvs[9] };

    Color corner_colors[CORNERS_COUNT] = { Color(1, 1, 1, part->alpha()), Color(1, 1, 1, part->alpha()), Color(1, 1, 1, part->alpha()), Color(1, 1, 1, part->alpha()) };
    const auto partColorIndex = part->part_color();
    if ((flags & ss::runtime::UpdateAttributeFlags_AttributePartColor) && partColorIndex >= 0) {
        auto pc = f.frameData->parts_color()->Get(partColorIndex);
        auto to_color = [](const ss::runtime::SsAttributePartColorKeyValueColor& c) { return Color(c.rgba().r()/255.0f, c.rgba().g()/255.0f, c.rgba().b()/255.0f, c.rgba().a()/255.0f); };
        corner_colors[0] = to_color(pc->lt()); corner_colors[1] = to_color(pc->rt()); corner_colors[2] = to_color(pc->lb()); corner_colors[3] = to_color(pc->rb());
    }

    Transform2D draw_transform = matrix_to_transform2d(draw_m);

    for (int j = 0; j < CORNERS_COUNT; j++) {
        verts.set(vbase + j, draw_transform.xform(Vector2(out_x[j], out_y[j])));
        uvs.set(vbase + j, Vector2(out_u[j] / tex_size.x, out_v[j] / tex_size.y));
        colors.set(vbase + j, corner_colors[j]);
    }
    if (needs_center) {
        verts.set(vbase + CORNERS_COUNT, draw_transform.xform(Vector2(out_x[CORNERS_COUNT], out_y[CORNERS_COUNT])));
        uvs.set(vbase + CORNERS_COUNT, Vector2(out_u[CORNERS_COUNT] / tex_size.x, out_v[CORNERS_COUNT] / tex_size.y));
        colors.set(vbase + CORNERS_COUNT, (corner_colors[0] + corner_colors[1] + corner_colors[2] + corner_colors[3]) * 0.25f);
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

void SsInternalPlayer::_draw_part_shape(const DrawFrame& f, RID ci, int p_idx, const ss::runtime::PartState* part, const ss::format::PartData* partBinary, const float* draw_m) {
    RenderingServer* rs = f.rs;

    ShapeGeometryBuffers bufs;
    if (!_build_shape_geometry(f, p_idx, part, draw_m, bufs)) return;

    _apply_blend_material(rs, ci, partBinary->blend_type());
    rs->canvas_item_set_transform(ci, Transform2D());
    rs->canvas_item_add_triangle_array(ci, bufs.indices, bufs.verts, bufs.colors);
}

bool SsInternalPlayer::_build_shape_geometry(const DrawFrame& f, int p_idx,
                                             const ss::runtime::PartState* part,
                                             const float* draw_m,
                                             ShapeGeometryBuffers& out) {
    const float* part_shape_verts = nullptr;
    const float* part_shape_box_coords = nullptr;
    int32_t part_shape_count = 0;
    if (f.shape_vertices && (uintptr_t)p_idx * 24 + 24 <= f.shape_vertices_len) {
        part_shape_verts = f.shape_vertices + (p_idx * 24);
    }
    if (f.shape_box_coords && (uintptr_t)p_idx * 24 + 24 <= f.shape_box_coords_len) {
        part_shape_box_coords = f.shape_box_coords + (p_idx * 24);
    }
    if (f.shape_vertex_counts && (uintptr_t)p_idx < f.shape_vertex_counts_len) {
        part_shape_count = f.shape_vertex_counts[p_idx];
    }
    if (!part_shape_verts || !part_shape_box_coords || part_shape_count < 3) return false;

    out.vert_count = part_shape_count;

    const uint64_t flags = part->update_flag();
    Color corner_colors[CORNERS_COUNT] = { Color(1, 1, 1, part->alpha()), Color(1, 1, 1, part->alpha()), Color(1, 1, 1, part->alpha()), Color(1, 1, 1, part->alpha()) };
    const auto partColorIndex = part->part_color();
    if ((flags & ss::runtime::UpdateAttributeFlags_AttributePartColor) && partColorIndex >= 0) {
        auto pc = f.frameData->parts_color()->Get(partColorIndex);
        auto to_color = [](const ss::runtime::SsAttributePartColorKeyValueColor& c) { return Color(c.rgba().r()/255.0f, c.rgba().g()/255.0f, c.rgba().b()/255.0f, c.rgba().a()/255.0f); };
        corner_colors[0] = to_color(pc->lt()); corner_colors[1] = to_color(pc->rt()); corner_colors[2] = to_color(pc->lb()); corner_colors[3] = to_color(pc->rb());
    }

    Transform2D draw_transform = matrix_to_transform2d(draw_m);

    out.verts.resize(part_shape_count);
    out.colors.resize(part_shape_count);

    for (int i = 0; i < part_shape_count; i++) {
        const float vx = part_shape_verts[i * 2 + 0];
        const float vy = part_shape_verts[i * 2 + 1];
        const float fx = part_shape_box_coords[i * 2 + 0];
        const float fy = part_shape_box_coords[i * 2 + 1];
        const float wLT = (1.0f - fx) * (1.0f - fy);
        const float wRT =          fx * (1.0f - fy);
        const float wLB = (1.0f - fx) *          fy;
        const float wRB =          fx *          fy;
        out.colors.set(i, corner_colors[0] * wLT + corner_colors[1] * wRT + corner_colors[2] * wLB + corner_colors[3] * wRB);
        out.verts.set(i, draw_transform.xform(Vector2(vx, vy)));
    }

    if (part_shape_count == 4) {
        const int idx[6] = { 0,1,2, 1,3,2 };
        out.indices.resize(6);
        for (int i = 0; i < 6; i++) out.indices.set(i, idx[i]);
    } else {
        const int tri_count = part_shape_count - 2;
        out.indices.resize(tri_count * 3);
        for (int i = 0; i < tri_count; i++) {
            out.indices.set(i*3 + 0, 0);
            out.indices.set(i*3 + 1, i + 1);
            out.indices.set(i*3 + 2, i + 2);
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

    SsVec2Array  verts;   verts.resize((int)batch->vertex_count());
    SsVec2Array  uvs;     uvs.resize((int)batch->vertex_count());
    SsColorArray colors;  colors.resize((int)batch->vertex_count());
    SsIntArray   indices; indices.resize((int)batch->index_count());

    int vbase = 0;
    int ibase = 0;
    bool any_emitted = false;

    for (uint16_t k = 0; k < count; k++) {
        int p_idx = (int)draw_order_data[batch->start_rank() + k];
        if (p_idx < 0 || p_idx >= (int)_parts_by_idx.size()) continue;
        const auto* part = _parts_by_idx[p_idx];
        if (!part) continue;

        const float* drawing_m = nullptr;
        if (f.world_matrices && (uintptr_t)p_idx * 16 < f.world_matrices_len) {
            drawing_m = f.world_matrices + (p_idx * 16);
        }
        if (!drawing_m) continue;

        const int vert_count = _build_normal(f, p_idx, part, drawing_m, tex_size,
                                             verts, uvs, colors, vbase);
        if (vert_count == 0) continue;

        // Append per-part indices with vertex base offset.
        if (vert_count == MAX_VERTICES_COUNT) {
            const int p[INDICES_COUNT_PENTAGON] = { 0,1,4, 1,3,4, 3,2,4, 2,0,4 };
            for (int j = 0; j < INDICES_COUNT_PENTAGON && ibase + j < indices.size(); j++) {
                indices.set(ibase + j, vbase + p[j]);
            }
            ibase += INDICES_COUNT_PENTAGON;
        } else {
            const int q[INDICES_COUNT_QUAD] = { 0,1,2, 1,3,2 };
            for (int j = 0; j < INDICES_COUNT_QUAD && ibase + j < indices.size(); j++) {
                indices.set(ibase + j, vbase + q[j]);
            }
            ibase += INDICES_COUNT_QUAD;
        }
        vbase += vert_count;
        any_emitted = true;
    }

    if (!any_emitted) return;

    // batch->blend_type() is the runtime BlendType; converted to ssab BlendType
    // by raw u8 value (the two enums are layout-equivalent, see chapter 9 §7).
    const auto ssab_blend = (ss::format::BlendType)batch->blend_type();
    _apply_blend_material(rs, ci, ssab_blend);
    rs->canvas_item_set_transform(ci, Transform2D());
    rs->canvas_item_add_triangle_array(ci, indices, verts, colors, uvs, {}, {}, tex->get_rid());
}

void SsInternalPlayer::_emit_shape_singleton(const DrawFrame& f, RID ci, int p_idx,
                                             const ss::runtime::PartState* part) {
    if (!part) return;
    const float* drawing_m = nullptr;
    if (f.world_matrices && (uintptr_t)p_idx * 16 < f.world_matrices_len) {
        drawing_m = f.world_matrices + (p_idx * 16);
    }
    if (!drawing_m) return;
    auto partBinary = f.binary->parts()->Get(p_idx);
    _draw_part_shape(f, ci, p_idx, part, partBinary, drawing_m);
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
        _clear_instance_players();
        _clear_batch_canvas_items();
        return;
    }

    _clear_instance_players();
    _clear_batch_canvas_items();

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

    // Skip when this player is itself an instance child to cap recursion at
    // depth 1 — nested Instance parts are not yet supported and would
    // self-reference / cycle through the same SSABResource.
    if (!_instance_child_mode) {
        _setup_instance_players();
    }

    float frame_no = ss_runtime_get_frame_no(runtime_ctx);
    float draw_frame = _sub_frame_enabled ? frame_no : floorf(frame_no);
    previous_frame_no = draw_frame;
    _drawAnimation(draw_frame);
}
