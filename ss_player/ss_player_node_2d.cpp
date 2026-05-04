#include "ss_player_node_2d.h"
#include "format/ssab.h"
#include "ssruntime.h"
#include "format/framedata.h"

#include <algorithm>
#include <vector>

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#else
#include "core/io/resource_loader.h"
#include "servers/rendering/rendering_server.h"
#endif


SpriteStudioPlayer2D::SpriteStudioPlayer2D() {
    runtime_ctx = ss_runtime_create();
    _reconfigure();
}

void SpriteStudioPlayer2D::_reconfigure() {
    if (runtime_ctx != nullptr) {
        ss_context_set_coordinate_system(runtime_ctx, 1);
    }
}

SpriteStudioPlayer2D::~SpriteStudioPlayer2D() {
    _clear_canvas_items();
    if (runtime_ctx != nullptr) {
        ss_runtime_destroy(runtime_ctx);
        runtime_ctx = nullptr;
    }
    if (rutime_res != nullptr) {
        ss_resource_destroy(rutime_res);
        rutime_res = nullptr;
    }
}

void SpriteStudioPlayer2D::_clear_canvas_items() {
    RenderingServer *rs = RenderingServer::get_singleton();
    for (int i = 0; i < _canvas_items.size(); i++) {
        rs->free_rid(_canvas_items[i]);
    }
    _canvas_items.clear();
    _blend_materials.clear();
}

void SpriteStudioPlayer2D::setSSABResource( const Ref<SSABResource>& ssabRes ) {
	_ssabRes = ssabRes;
    _strAnimationSelected = "";

    if ( !_ssabRes.is_null() ) {
        if (!_ssabRes->is_valid()) {
            ERR_PRINT("SSAB Error: Assigned resource is invalid (missing parts or animations).");
            _ssabRes = Ref<SSABResource>(); // 無効な場合はリセット
        } else {
            auto vecAnimeName = _ssabRes->get_animation_names();
            if ( vecAnimeName.size() > 0 )
                _strAnimationSelected = vecAnimeName[0];
            loadTextures(_ssabRes);
        }
    }

	fetchAnimation();
	NOTIFY_PROPERTY_LIST_CHANGED();
}

Ref<SSABResource> SpriteStudioPlayer2D::getSSABResource() const {
	return	_ssabRes;
}

void SpriteStudioPlayer2D::setAnimation( const String& strName ) {
    _strAnimationSelected = strName;

    // postAnimationChanged( _strAnimationSelected );

	fetchAnimation();
	NOTIFY_PROPERTY_LIST_CHANGED();
}

String SpriteStudioPlayer2D::getAnimation() const {
    return	_strAnimationSelected;
}

bool SpriteStudioPlayer2D::isPlaying() const {
    return ss_runtime_is_playing(runtime_ctx);
}

void SpriteStudioPlayer2D::play( float p_start_frame ) {
    if ( p_start_frame >= 0.0f ) {
        ss_runtime_play_with_start_frame(runtime_ctx, p_start_frame);
    } else {
        ss_runtime_play(runtime_ctx);
    }
}

bool SpriteStudioPlayer2D::isPausing() const {
    return ss_runtime_is_pausing(runtime_ctx);
}

void SpriteStudioPlayer2D::pause() {
    ss_runtime_pause(runtime_ctx);
}

void SpriteStudioPlayer2D::stop() {
    ss_runtime_stop(runtime_ctx);
}

void SpriteStudioPlayer2D::setSpeed( float p_speed ) {
    _speed_rate = p_speed;
    ss_runtime_set_animation_speed(runtime_ctx, p_speed);
}

float SpriteStudioPlayer2D::getSpeed() const {
    return _speed_rate;
}

void SpriteStudioPlayer2D::setFrame( float p_frame ) {
    if (runtime_ctx) {
        ss_runtime_set_frame_no(runtime_ctx, p_frame);
        float frame_no = ss_runtime_get_frame_no(runtime_ctx);
        float draw_frame = _sub_frame_enabled ? frame_no : (float)((int)frame_no);
        previous_frame_no = draw_frame;
        drawAnimation(draw_frame);
    }
}

float SpriteStudioPlayer2D::getFrame() const {
    return ss_runtime_get_frame_no(runtime_ctx);
}

int SpriteStudioPlayer2D::getTotalFrames() const {
    return ss_runtime_get_end_frame(runtime_ctx) - ss_runtime_get_start_frame(runtime_ctx) + 1;
}

void SpriteStudioPlayer2D::setFrameRate( int p_fps ) {
    ss_runtime_set_frame_rate(runtime_ctx, p_fps);
}

int SpriteStudioPlayer2D::getFrameRate() const {
    return ss_runtime_get_fps(runtime_ctx);
}

void SpriteStudioPlayer2D::setAnimationSection( int p_start, int p_end ) {
    ss_runtime_set_animation_section(runtime_ctx, p_start, p_end);
}

int SpriteStudioPlayer2D::getAnimationSectionStart() const {
    return ss_runtime_get_start_frame(runtime_ctx);
}

int SpriteStudioPlayer2D::getAnimationSectionEnd() const {
    return ss_runtime_get_end_frame(runtime_ctx);
}

void SpriteStudioPlayer2D::setPlaybackDirection( int p_direction, int p_style ) {
    ss_runtime_set_playback_direction(runtime_ctx, p_direction, p_style);
}

int SpriteStudioPlayer2D::getPlaybackDirection() const {
    return ss_runtime_get_playback_direction(runtime_ctx);
}

int SpriteStudioPlayer2D::getPlaybackStyle() const {
    return ss_runtime_get_playback_style(runtime_ctx);
}

void SpriteStudioPlayer2D::setLoop( int p_count ) {
    ss_runtime_set_loop(runtime_ctx, p_count);
}

int SpriteStudioPlayer2D::getLoop() const {
    return ss_runtime_get_loops(runtime_ctx);
}

void SpriteStudioPlayer2D::setSkipFrames( bool p_skip ) {
    ss_runtime_set_skip_frames(runtime_ctx, p_skip);
}

bool SpriteStudioPlayer2D::isSkipFrames() const {
    return ss_runtime_get_skip_frames(runtime_ctx);
}

void SpriteStudioPlayer2D::setSubFrameEnabled( bool p_enabled ) {
    _sub_frame_enabled = p_enabled;
    if (runtime_ctx) {
        float frame_no = ss_runtime_get_frame_no(runtime_ctx);
        float draw_frame = _sub_frame_enabled ? frame_no : (float)((int)frame_no);
        previous_frame_no = draw_frame;
        drawAnimation(draw_frame);
    }
}

bool SpriteStudioPlayer2D::isSubFrameEnabled() const {
    return _sub_frame_enabled;
}


void SpriteStudioPlayer2D::_bind_methods() {
    ClassDB::bind_method( D_METHOD( "set_ssab_resource", "res_ssab" ), &SpriteStudioPlayer2D::setSSABResource );
    ClassDB::bind_method( D_METHOD( "get_ssab_resource" ), &SpriteStudioPlayer2D::getSSABResource );
    ClassDB::bind_method( D_METHOD( "set_animation", "name" ), &SpriteStudioPlayer2D::setAnimation );
    ClassDB::bind_method( D_METHOD( "get_animation" ), &SpriteStudioPlayer2D::getAnimation );

    ClassDB::bind_method( D_METHOD( "is_playing" ), &SpriteStudioPlayer2D::isPlaying );
    ClassDB::bind_method( D_METHOD( "play", "start_frame" ), &SpriteStudioPlayer2D::play, DEFVAL(-1.0f) );
    ClassDB::bind_method( D_METHOD( "is_pausing" ), &SpriteStudioPlayer2D::isPausing );
    ClassDB::bind_method( D_METHOD( "pause" ), &SpriteStudioPlayer2D::pause );
    ClassDB::bind_method( D_METHOD( "stop" ), &SpriteStudioPlayer2D::stop );

    ClassDB::bind_method( D_METHOD( "set_speed", "speed" ), &SpriteStudioPlayer2D::setSpeed );
    ClassDB::bind_method( D_METHOD( "get_speed" ), &SpriteStudioPlayer2D::getSpeed );
    ClassDB::bind_method( D_METHOD( "set_frame", "frame" ), &SpriteStudioPlayer2D::setFrame );
    ClassDB::bind_method( D_METHOD( "get_frame" ), &SpriteStudioPlayer2D::getFrame );

    ClassDB::bind_method( D_METHOD( "get_total_frames" ), &SpriteStudioPlayer2D::getTotalFrames );

    ClassDB::bind_method( D_METHOD( "set_frame_rate", "fps" ), &SpriteStudioPlayer2D::setFrameRate );
    ClassDB::bind_method( D_METHOD( "get_frame_rate" ), &SpriteStudioPlayer2D::getFrameRate );

    ClassDB::bind_method( D_METHOD( "set_animation_section", "start", "end" ), &SpriteStudioPlayer2D::setAnimationSection );
    ClassDB::bind_method( D_METHOD( "get_animation_section_start" ), &SpriteStudioPlayer2D::getAnimationSectionStart );
    ClassDB::bind_method( D_METHOD( "get_animation_section_end" ), &SpriteStudioPlayer2D::getAnimationSectionEnd );

    ClassDB::bind_method( D_METHOD( "set_playback_direction", "direction", "style" ), &SpriteStudioPlayer2D::setPlaybackDirection );
    ClassDB::bind_method( D_METHOD( "get_playback_direction" ), &SpriteStudioPlayer2D::getPlaybackDirection );
    ClassDB::bind_method( D_METHOD( "get_playback_style" ), &SpriteStudioPlayer2D::getPlaybackStyle );

    ClassDB::bind_method( D_METHOD( "set_loop", "count" ), &SpriteStudioPlayer2D::setLoop );
    ClassDB::bind_method( D_METHOD( "get_loop" ), &SpriteStudioPlayer2D::getLoop );

    ClassDB::bind_method( D_METHOD( "set_skip_frames", "skip" ), &SpriteStudioPlayer2D::setSkipFrames );
    ClassDB::bind_method( D_METHOD( "is_skip_frames" ), &SpriteStudioPlayer2D::isSkipFrames );

    ClassDB::bind_method( D_METHOD( "set_sub_frame_enabled", "enabled" ), &SpriteStudioPlayer2D::setSubFrameEnabled );
    ClassDB::bind_method( D_METHOD( "is_sub_frame_enabled" ), &SpriteStudioPlayer2D::isSubFrameEnabled );

	ADD_SIGNAL(
		MethodInfo(
			"user_data",
			PropertyInfo(
				Variant::INT,
				"flag"
			),
			PropertyInfo(
				Variant::INT,
				"int_value"
			),
			PropertyInfo(
				Variant::RECT2,
				"rect_value"
			),
			PropertyInfo(
				Variant::VECTOR2,
				"point_value"
			),
			PropertyInfo(
				Variant::STRING,
				"string_value"
			)
		)
	);
	ADD_SIGNAL(
		MethodInfo(
			"signal",
			PropertyInfo(
				Variant::STRING,
				"command"
			),
			PropertyInfo(
				Variant::DICTIONARY,
				"value"
			)
		)
	);

	ADD_PROPERTY(
		PropertyInfo(
			Variant::OBJECT,
			"ssab",
			PropertyHint::PROPERTY_HINT_RESOURCE_TYPE,
			"SSABResource"
		),
		"set_ssab_resource",
		"get_ssab_resource"
	);

	ADD_PROPERTY(
		PropertyInfo(
			Variant::FLOAT,
			"speed"
		),
		"set_speed",
		"get_speed"
	);

	ADD_PROPERTY(
		PropertyInfo(
			Variant::INT,
			"loop"
		),
		"set_loop",
		"get_loop"
	);

    ADD_PROPERTY(
        PropertyInfo(
            Variant::BOOL,
            "skip_frames"
        ),
        "set_skip_frames",
        "is_skip_frames"
    );

    ADD_PROPERTY(
        PropertyInfo(
            Variant::BOOL,
            "sub_frame_enabled"
        ),
        "set_sub_frame_enabled",
        "is_sub_frame_enabled"
    );

	ADD_GROUP( "Animation Settings", "" );
}

bool SpriteStudioPlayer2D::_set( const StringName& p_name, const Variant& p_property ) {
	if ( p_name == StringName("animation")) {
		setAnimation( p_property );

		return	true;
	} else if ( p_name == StringName("frame")) {
		setFrame( p_property );

		return	true;
	} else if ( p_name == StringName("loop")) {
		setLoop( p_property );

		return	true;
	} else if ( p_name == StringName("speed")) {
		setSpeed( p_property );

		return	true;
	} else if ( p_name == StringName("skip_frames")) {
		setSkipFrames( p_property );

		return	true;
	} else if ( p_name == StringName("sub_frame_enabled")) {
		setSubFrameEnabled( p_property );

		return	true;
	} else if ( p_name == StringName("playing")) {
        if (p_property) {
            play();
        } else {
            stop();
        }
		return	true;
	}

	return	false;
}

bool SpriteStudioPlayer2D::_get( const StringName& p_name, Variant& r_property ) const {
    if ( p_name == StringName("animation")) {
        r_property = getAnimation();

        return	true;
    } else if ( p_name == StringName("frame") ) {
        r_property = getFrame();

        return	true;
    } else if ( p_name == StringName("loop") ) {
        r_property = getLoop();

        return	true;
    } else if ( p_name == StringName("speed") ) {
        r_property = getSpeed();

        return	true;
    } else if ( p_name == StringName("skip_frames") ) {
        r_property = isSkipFrames();

        return	true;
    } else if ( p_name == StringName("sub_frame_enabled") ) {
        r_property = isSubFrameEnabled();

        return	true;
    } else if ( p_name == StringName("playing") ) {
        r_property = isPlaying();

        return	true;
    }

    return	false;
}

void SpriteStudioPlayer2D::_get_property_list( List<PropertyInfo>* p_list ) const {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
	PackedStringArray vecAnimeName;
#else
	Vector<String>	vecAnimeName;
#endif

	vecAnimeName.insert(0, "-- Empty --");

	if (!_ssabRes.is_null()) {
		vecAnimeName = _ssabRes->get_animation_names();
	}

	PropertyInfo animasPropertyInfo;
	animasPropertyInfo.name = "animation";
	animasPropertyInfo.type = Variant::STRING;
	animasPropertyInfo.usage = PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE;
	animasPropertyInfo.hint_string = String( "," ).join( vecAnimeName );
	animasPropertyInfo.hint = PROPERTY_HINT_ENUM;
    p_list->push_back( animasPropertyInfo );

    animasPropertyInfo.name = "playing";
    animasPropertyInfo.type = Variant::BOOL;
    animasPropertyInfo.usage = PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE;
    animasPropertyInfo.hint = PROPERTY_HINT_NONE;
	p_list->push_back( animasPropertyInfo );

    animasPropertyInfo.name = "sub_frame_enabled";
    animasPropertyInfo.type = Variant::BOOL;
    animasPropertyInfo.usage = PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE;
    animasPropertyInfo.hint = PROPERTY_HINT_NONE;
    p_list->push_back( animasPropertyInfo );

    animasPropertyInfo.name = "frame";
    animasPropertyInfo.type = Variant::FLOAT;
    animasPropertyInfo.usage = PROPERTY_USAGE_EDITOR;
    animasPropertyInfo.hint = PROPERTY_HINT_RANGE;
    animasPropertyInfo.hint_string = "0," + String::num(getTotalFrames()-1) + ",0.01";
    p_list->push_back( animasPropertyInfo );

}

void SpriteStudioPlayer2D::_notification( int p_notification ) {
    switch ( p_notification ) {
 	case NOTIFICATION_READY:
        set_process_internal( true );

        break;
    case NOTIFICATION_INTERNAL_PROCESS:
		updateAnimation( (float)get_process_delta_time() );

        break;
    default:
        break;
	}
}

void SpriteStudioPlayer2D::loadTextures(const Ref<SSABResource>& ssabRes) {
    auto a = ssabRes->get_ss_anime_binary();
    _textures.clear();
    if (a->cellmaps() != nullptr) {
        for (int i = 0; i < a->cellmaps()->size(); i++) {
            auto cellmap = a->cellmaps()->Get(i);
            String strImage = _ssabRes->get_parent_dir().path_join(String::utf8(cellmap->image_path()->c_str()));
            Ref<Texture2D> texture =
            #ifdef SPRITESTUDIO_GODOT_EXTENSION
            ResourceLoader::get_singleton()->load( strImage, "", ResourceLoader::CACHE_MODE_REUSE);
            #else
            ResourceLoader::load( strImage, "", ResourceFormatLoader::CACHE_MODE_REUSE, nullptr );
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
            ResourceLoader::get_singleton()->load( strImage, "", ResourceLoader::CACHE_MODE_REUSE);
            #else
            ResourceLoader::load( strImage, "", ResourceFormatLoader::CACHE_MODE_REUSE, nullptr );
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

    struct SsUvComputeParams {
        Rect2 src_rect;
        Vector2 uv_translation;
        float uv_rotation_z = 0.0f;
        Vector2 uv_scale;
        bool part_flip_h = false;
        bool part_flip_v = false;
        bool img_flip_h = false;
        bool img_flip_v = false;
        bool rotated = false;
    };

    bool compute_uvs(const SsUvComputeParams& params, float* out_u, float* out_v) {
        return ss_uv_compute_local(
            params.src_rect.position.x, params.src_rect.position.y,
            params.src_rect.position.x + params.src_rect.size.x,
            params.src_rect.position.y + params.src_rect.size.y,
            params.uv_translation.x * params.src_rect.size.x,
            params.uv_translation.y * params.src_rect.size.y,
            Math::rad_to_deg(params.uv_rotation_z),
            params.uv_scale.x, params.uv_scale.y,
            params.part_flip_h, params.part_flip_v,
            params.img_flip_h, params.img_flip_v,
            params.rotated,
            out_u, out_v
        );
    }

    Transform2D matrix_to_transform2d(const float* m) {
        Transform2D t;
        t.columns[0] = Vector2(m[0], m[1]);
        t.columns[1] = Vector2(m[4], m[5]);
        t.columns[2] = Vector2(m[12], m[13]);
        return t;
    }
}

void SpriteStudioPlayer2D::updateAnimation( float delta ) {
    if (ss_runtime_is_playing(runtime_ctx)) {
        auto d = delta * 1000.0f;
        float frame_no = ss_runtime_update(runtime_ctx, d);

        float draw_frame = _sub_frame_enabled ? frame_no : (float)((int)frame_no);

        if (previous_frame_no == draw_frame) {
            return;
        }

        // TODO: implement event handling
        /*
        if (_currentAnimationData->events() != nullptr) {
            int event_count = ss_runtime_get_passed_event_count(runtime_ctx);
            for (int i = 0; i < event_count; i++) {
                int event_idx = ss_runtime_get_passed_event_index(runtime_ctx, i);
                auto events_per_frame = _currentAnimationData->events()->Get(event_idx);

                if (auto users = events_per_frame->users()) {
                    for (auto user : *users) {
                        // TODO: impl
                    }
                }

                if (auto signals = events_per_frame->signals()) {
                    for (auto signal : *signals) {
                        // TODO: impl
                    }
                }

                if (auto audios = events_per_frame->audios()) {
                    for (auto audio : *audios) {
                        // TODO: impl
                    }
                }
            }
        }
        */
        previous_frame_no = draw_frame;
        drawAnimation(draw_frame);
    }
}

void SpriteStudioPlayer2D::drawAnimation(float frame_no) {
    unsigned char *data = nullptr;
    uintptr_t len = 0;
    ss_runtime_get_frame_data(runtime_ctx, frame_no, &data, &len);
    if (!data) return;

    const float *world_matrices = nullptr;
    uintptr_t world_matrices_len = 0;
    ss_runtime_get_world_matrices(runtime_ctx, &world_matrices, &world_matrices_len);

    const int32_t *z_order = nullptr;
    uintptr_t z_order_len = 0;
    ss_runtime_get_z_order(runtime_ctx, &z_order, &z_order_len);

    auto frameData = ss::runtime::GetFrameData(data);
    auto parts = frameData->parts();
    if (!parts) return;

    RenderingServer *rs = RenderingServer::get_singleton();
    // Clear all canvas items once per frame to ensure a clean state
    for (RID ci : _canvas_items) {
        rs->canvas_item_clear(ci);
    }

    auto binary = _ssabRes->get_ss_anime_binary();

    for (uint32_t i = 0; i < parts->size(); i++) {
        auto part = parts->Get(i);
        int p_idx = part->part_index();
        if (p_idx < 0 || p_idx >= (int)_canvas_items.size()) continue;

        RID ci = _canvas_items[p_idx];
        // Use the Z-order rank provided by the runtime to strictly enforce the determined order
        if (z_order && (uintptr_t)p_idx < z_order_len) {
            rs->canvas_item_set_z_index(ci, z_order[p_idx]);
        }

        if (part->hide()) continue;

        const float *drawing_m = nullptr;
        if (world_matrices && ((uintptr_t)p_idx * 16 < world_matrices_len)) {
            drawing_m = world_matrices + (p_idx * 16);
        } else {
            continue;
        }

        auto partBinary = binary->parts()->Get(p_idx);

        auto frameDataCellIndex = part->cell();
        auto frameDataCell = frameData->cells()->Get(frameDataCellIndex);
        auto cellmap = binary->cellmaps()->Get(frameDataCell->map_id());

        uint32_t texHash = cellmap->name_hash();
        if (!_textures.has(texHash)) continue;
        Ref<Texture2D> tex = _textures[texHash];

        auto cell = cellmap->cells()->LookupByKey(frameDataCell->name_hash());
        if (!cell) continue;

        _draw_part(rs, ci, frameData, part, partBinary, tex, cell, drawing_m);
    }
}

void SpriteStudioPlayer2D::_draw_part(RenderingServer *rs, RID ci, const ss::runtime::FrameData *frameData, const ss::runtime::PartState *part, const ss::format::PartData *partBinary, const Ref<Texture2D> &tex, const ss::format::Cell *cell, const float *draw_m) {
    // 1. Blend Mode
    ss::format::BlendType ss_blend = partBinary->blend_type();
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

    // 2. Matrix Calculation (Hierarchy aware)
    uint64_t flags = part->update_flag();
    auto rect = cell->rectangle();
    auto pivot = cell->pivot();
    Rect2 src_rect(rect->x1(), rect->y1(), rect->x2(), rect->y2());

    bool use_advanced = (flags & (ss::runtime::UpdateAttributeFlags_AttributeVertex | ss::runtime::UpdateAttributeFlags_AttributePartColor |
                                  ss::runtime::UpdateAttributeFlags_AttributeUvtX | ss::runtime::UpdateAttributeFlags_AttributeUvtY |
                                  ss::runtime::UpdateAttributeFlags_AttributeUvrZ | ss::runtime::UpdateAttributeFlags_AttributeUvsX |
                                  ss::runtime::UpdateAttributeFlags_AttributeUvsY));

    if (!use_advanced && partBinary->part_type_type() != ss::format::PartType_PartTypeMesh) {
        Transform2D t = matrix_to_transform2d(draw_m);
        rs->canvas_item_set_transform(ci, t);

        Vector2 draw_pos = Vector2(-src_rect.size.x * (pivot->v1() + 0.5f),
                                   -src_rect.size.y * (0.5f - pivot->v2()));
        rs->canvas_item_add_texture_rect_region(ci, Rect2(draw_pos, src_rect.size), tex->get_rid(), src_rect, Color(1, 1, 1, part->alpha()));
    } else if (partBinary->part_type_type() != ss::format::PartType_PartTypeMesh) {
        rs->canvas_item_set_transform(ci, Transform2D());

        // Use the 5-vertex triangle fan around the center only when needed
        // (per-vertex deform or per-vertex parts color). Otherwise the cheaper
        // 4-vertex 2-triangle quad is enough — center vertex is unused / zero
        // in the FrameData for those parts.
        const bool needs_center = (flags & (ss::runtime::UpdateAttributeFlags_AttributeVertex | ss::runtime::UpdateAttributeFlags_AttributePartColor)) != 0;
        const int vert_count = needs_center ? MAX_VERTICES_COUNT : CORNERS_COUNT;

        // Triangle fan around center vertex (LT=0, RT=1, LB=2, RB=3, Center=4).
        // Constant per build, so initialize once via function-local static.
        #ifdef SPRITESTUDIO_GODOT_EXTENSION
        static const PackedInt32Array INDICES_FAN_5 = []() {
            PackedInt32Array a; a.resize(INDICES_COUNT_PENTAGON);
            const int idxs[INDICES_COUNT_PENTAGON] = { 0,1,4, 1,3,4, 3,2,4, 2,0,4 };
            for (int k = 0; k < INDICES_COUNT_PENTAGON; k++) a.set(k, idxs[k]);
            return a;
        }();
        // 2-triangle quad split (LT-RT-LB and RT-RB-LB).
        static const PackedInt32Array INDICES_QUAD_4 = []() {
            PackedInt32Array a; a.resize(INDICES_COUNT_QUAD);
            const int idxs[INDICES_COUNT_QUAD] = { 0,1,2, 1,3,2 };
            for (int k = 0; k < INDICES_COUNT_QUAD; k++) a.set(k, idxs[k]);
            return a;
        }();
        PackedVector2Array p_verts; p_verts.resize(vert_count);
        PackedVector2Array p_uvs; p_uvs.resize(vert_count);
        PackedColorArray p_colors; p_colors.resize(vert_count);
        #else
        static const Vector<int> INDICES_FAN_5 = []() {
            Vector<int> a; a.resize(INDICES_COUNT_PENTAGON);
            const int idxs[INDICES_COUNT_PENTAGON] = { 0,1,4, 1,3,4, 3,2,4, 2,0,4 };
            for (int k = 0; k < INDICES_COUNT_PENTAGON; k++) a.set(k, idxs[k]);
            return a;
        }();
        static const Vector<int> INDICES_QUAD_4 = []() {
            Vector<int> a; a.resize(INDICES_COUNT_QUAD);
            const int idxs[INDICES_COUNT_QUAD] = { 0,1,2, 1,3,2 };
            for (int k = 0; k < INDICES_COUNT_QUAD; k++) a.set(k, idxs[k]);
            return a;
        }();
        Vector<Vector2> p_verts; p_verts.resize(vert_count);
        Vector<Vector2> p_uvs; p_uvs.resize(vert_count);
        Vector<Color> p_colors; p_colors.resize(vert_count);
        #endif

        // Pre-computed local vertices come directly from the Brain; pivot, size,
        // image-flip, and raw deform offsets are already applied. Center is only
        // populated by the Brain when needs_center is true.
        const ss::runtime::PartAttributeVertex* vd = frameData->vertices()->Get(part->vertex());
        const float out_x[MAX_VERTICES_COUNT] = { vd->lt().x(), vd->rt().x(), vd->lb().x(), vd->rb().x(), vd->center().x() };
        const float out_y[MAX_VERTICES_COUNT] = { vd->lt().y(), vd->rt().y(), vd->lb().y(), vd->rb().y(), vd->center().y() };

        float out_u[MAX_VERTICES_COUNT], out_v[MAX_VERTICES_COUNT];
        SsUvComputeParams uv_params;
        uv_params.src_rect = src_rect;
        uv_params.uv_translation = Vector2(part->uv_translation_x(), part->uv_translation_y());
        uv_params.uv_rotation_z = part->uv_rotation_z();
        uv_params.uv_scale = Vector2(part->uv_scale_x(), part->uv_scale_y());
        uv_params.part_flip_h = part->flip_h();
        uv_params.part_flip_v = part->flip_v();
        uv_params.img_flip_h = part->img_flip_h();
        uv_params.img_flip_v = part->img_flip_v();
        uv_params.rotated = cell->rotated();

        if (!compute_uvs(uv_params, out_u, out_v)) {
            return;
        }

        Vector2 tex_size = tex->get_size();
        Color corner_colors[CORNERS_COUNT] = { Color(1, 1, 1, part->alpha()), Color(1, 1, 1, part->alpha()), Color(1, 1, 1, part->alpha()), Color(1, 1, 1, part->alpha()) };
        if (flags & ss::runtime::UpdateAttributeFlags_AttributePartColor) {
            auto pc = frameData->parts_color()->Get(part->part_color());
            // The hierarchical alpha is already pre-multiplied into c.rgba().a() by the Brain.
            auto to_color = [](const ss::runtime::SsAttributePartColorKeyValueColor &c) { return Color(c.rgba().r()/255.0f, c.rgba().g()/255.0f, c.rgba().b()/255.0f, c.rgba().a()/255.0f); };
            corner_colors[0] = to_color(pc->lt()); corner_colors[1] = to_color(pc->rt()); corner_colors[2] = to_color(pc->lb()); corner_colors[3] = to_color(pc->rb());
        }

        Transform2D draw_transform = matrix_to_transform2d(draw_m);

        for (int j = 0; j < CORNERS_COUNT; j++) {
            p_verts.set(j, draw_transform.xform(Vector2(out_x[j], out_y[j])));
            p_uvs.set(j, Vector2(out_u[j] / tex_size.x, out_v[j] / tex_size.y));
            p_colors.set(j, corner_colors[j]);
        }

        if (needs_center) {
            p_verts.set(CORNERS_COUNT, draw_transform.xform(Vector2(out_x[CORNERS_COUNT], out_y[CORNERS_COUNT])));
            p_uvs.set(CORNERS_COUNT, Vector2(out_u[CORNERS_COUNT] / tex_size.x, out_v[CORNERS_COUNT] / tex_size.y));
            p_colors.set(CORNERS_COUNT, (corner_colors[0] + corner_colors[1] + corner_colors[2] + corner_colors[3]) * 0.25f);
        }

        rs->canvas_item_add_triangle_array(ci, needs_center ? INDICES_FAN_5 : INDICES_QUAD_4, p_verts, p_colors, p_uvs, {}, {}, tex->get_rid());
    }
}

void SpriteStudioPlayer2D::fetchAnimation() {
	if ( _strAnimationSelected.is_empty() || _ssabRes.is_null() ) {
        ss_runtime_reset(runtime_ctx);
        _reconfigure();
        if (rutime_res != nullptr) {
            ss_resource_destroy(rutime_res);
            rutime_res = nullptr;
        }
        _currentAnimationData = nullptr;
        _clear_canvas_items();
        return;
    }

    // Initialize RenderingServer CanvasItems
    _clear_canvas_items();

    if (rutime_res != nullptr) {
        ss_resource_destroy(rutime_res);
        rutime_res = nullptr;
    }

    rutime_res = ss_resource_create_borrow(_ssabRes->get_data_ptr(), _ssabRes->get_data_size());
    if (rutime_res == nullptr) {
        ERR_PRINT( "SSAB Resource Create Failed" );
        return;
    }

    bool binded = ss_runtime_bind_resource(runtime_ctx, rutime_res);
    if ( !binded ) {
        ERR_PRINT( "SSAB Resource Bind Failed" );
        return;
    }

    auto binary = _ssabRes->get_ss_anime_binary();
    if (binary->parts() != nullptr) {
            RenderingServer *rs = RenderingServer::get_singleton();
            for (int i = 0; i < binary->parts()->size(); i++) {
                RID ci = rs->canvas_item_create();
                rs->canvas_item_set_parent(ci, get_canvas_item());
                _canvas_items.push_back(ci);
            }
        }

		auto c = _strAnimationSelected.utf8();
        auto animation = _ssabRes->find_animation( _strAnimationSelected );
		if ( !animation ) {
			ERR_PRINT( "Select Anime is Null" );
            return;
		}
        _currentAnimationData = animation;
        bool setup = ss_runtime_setup_animation(runtime_ctx, c.get_data());
        if ( !setup ) {
            ERR_PRINT( "SSAB Setup Animation Failed: " + _strAnimationSelected );
            return;
        }

        float frame_no = ss_runtime_get_frame_no(runtime_ctx);
        float draw_frame = _sub_frame_enabled ? frame_no : (float)((int)frame_no);
        previous_frame_no = draw_frame;
        drawAnimation(draw_frame);
}
