#include "gd_ssplayer_node2d.h"
#include "runtime/ssab.h"
#include "runtime/ssruntime.h"
#include "runtime/framedata.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#else
#include "core/io/resource_loader.h"
#include "servers/rendering/rendering_server.h"
#endif


GdSsPlayerNode2D::GdSsPlayerNode2D() {
    runtime_ctx = ss_runtime_create();
    _reconfigure();
}

void GdSsPlayerNode2D::_reconfigure() {
    if (runtime_ctx != nullptr) {
        ss_context_set_coordinate_system(runtime_ctx, 1);
    }
}

GdSsPlayerNode2D::~GdSsPlayerNode2D() {
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

void GdSsPlayerNode2D::_clear_canvas_items() {
    RenderingServer *rs = RenderingServer::get_singleton();
    for (int i = 0; i < _canvas_items.size(); i++) {
        rs->free_rid(_canvas_items[i]);
    }
    _canvas_items.clear();
    _blend_materials.clear();
}

void GdSsPlayerNode2D::setSsabResource( const Ref<GdSsabResource>& ssabRes ) {
	_ssabRes = ssabRes;
    _strAnimationSelected = "";

    if ( !_ssabRes.is_null() ) {
        if (!_ssabRes->is_valid()) {
            ERR_PRINT("SSAB Error: Assigned resource is invalid (missing parts or animations).");
            _ssabRes = Ref<GdSsabResource>(); // 無効な場合はリセット
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

Ref<GdSsabResource> GdSsPlayerNode2D::getSsabResource() const {
	return	_ssabRes;
}

void GdSsPlayerNode2D::setAnimation( const String& strName ) {
    _strAnimationSelected = strName;

    // postAnimationChanged( _strAnimationSelected );

	fetchAnimation();
	NOTIFY_PROPERTY_LIST_CHANGED();
}

String GdSsPlayerNode2D::getAnimation() const {
    return	_strAnimationSelected;
}

bool GdSsPlayerNode2D::isPlaying() const {
    return ss_runtime_is_playing(runtime_ctx);
}

void GdSsPlayerNode2D::play( int p_start_frame ) {
    if ( p_start_frame >= 0 ) {
        ss_runtime_play_with_start_frame(runtime_ctx, p_start_frame);
    } else {
        ss_runtime_play(runtime_ctx);
    }
}

bool GdSsPlayerNode2D::isPausing() const {
    return ss_runtime_is_pausing(runtime_ctx);
}

void GdSsPlayerNode2D::pause() {
    ss_runtime_pause(runtime_ctx);
}

void GdSsPlayerNode2D::stop() {
    ss_runtime_stop(runtime_ctx);
}

void GdSsPlayerNode2D::setSpeed( float p_speed ) {
    _speed_rate = p_speed;
    ss_runtime_set_animation_speed(runtime_ctx, p_speed);
}

float GdSsPlayerNode2D::getSpeed() const {
    return _speed_rate;
}

void GdSsPlayerNode2D::setFrame( int p_frame ) {
    ss_runtime_set_frame_no(runtime_ctx, p_frame);
}

int GdSsPlayerNode2D::getFrame() const {
    return ss_runtime_get_frame_no(runtime_ctx);
}

float GdSsPlayerNode2D::getFrameDecimal() const {
    return ss_runtime_get_frame_no_decimal(runtime_ctx);
}

int GdSsPlayerNode2D::getTotalFrames() const {
    return ss_runtime_get_end_frame(runtime_ctx) - ss_runtime_get_start_frame(runtime_ctx) + 1;
}

void GdSsPlayerNode2D::setFrameRate( int p_fps ) {
    ss_runtime_set_frame_rate(runtime_ctx, p_fps);
}

int GdSsPlayerNode2D::getFrameRate() const {
    return ss_runtime_get_fps(runtime_ctx);
}

void GdSsPlayerNode2D::setAnimationSection( int p_start, int p_end ) {
    ss_runtime_set_animation_section(runtime_ctx, p_start, p_end);
}

int GdSsPlayerNode2D::getAnimationSectionStart() const {
    return ss_runtime_get_start_frame(runtime_ctx);
}

int GdSsPlayerNode2D::getAnimationSectionEnd() const {
    return ss_runtime_get_end_frame(runtime_ctx);
}

void GdSsPlayerNode2D::setPlaybackDirection( int p_direction, int p_style ) {
    ss_runtime_set_playback_direction(runtime_ctx, p_direction, p_style);
}

int GdSsPlayerNode2D::getPlaybackDirection() const {
    return ss_runtime_get_playback_direction(runtime_ctx);
}

int GdSsPlayerNode2D::getPlaybackStyle() const {
    return ss_runtime_get_playback_style(runtime_ctx);
}

void GdSsPlayerNode2D::setLoop( int p_count ) {
    ss_runtime_set_loop(runtime_ctx, p_count);
}

int GdSsPlayerNode2D::getLoop() const {
    return ss_runtime_get_loops(runtime_ctx);
}

void GdSsPlayerNode2D::setSkipFrames( bool p_skip ) {
    ss_runtime_set_skip_frames(runtime_ctx, p_skip);
}

bool GdSsPlayerNode2D::isSkipFrames() const {
    return ss_runtime_get_skip_frames(runtime_ctx);
}


void GdSsPlayerNode2D::_bind_methods() {
    ClassDB::bind_method( D_METHOD( "set_ssab_resource", "res_ssab" ), &GdSsPlayerNode2D::setSsabResource );
    ClassDB::bind_method( D_METHOD( "get_ssab_resource" ), &GdSsPlayerNode2D::getSsabResource );
    ClassDB::bind_method( D_METHOD( "set_animation", "name" ), &GdSsPlayerNode2D::setAnimation );
    ClassDB::bind_method( D_METHOD( "get_animation" ), &GdSsPlayerNode2D::getAnimation );

    ClassDB::bind_method( D_METHOD( "is_playing" ), &GdSsPlayerNode2D::isPlaying );
    ClassDB::bind_method( D_METHOD( "play", "start_frame" ), &GdSsPlayerNode2D::play, DEFVAL(-1) );
    ClassDB::bind_method( D_METHOD( "is_pausing" ), &GdSsPlayerNode2D::isPausing );
    ClassDB::bind_method( D_METHOD( "pause" ), &GdSsPlayerNode2D::pause );
    ClassDB::bind_method( D_METHOD( "stop" ), &GdSsPlayerNode2D::stop );

    ClassDB::bind_method( D_METHOD( "set_speed", "speed" ), &GdSsPlayerNode2D::setSpeed );
    ClassDB::bind_method( D_METHOD( "get_speed" ), &GdSsPlayerNode2D::getSpeed );
    ClassDB::bind_method( D_METHOD( "set_frame", "frame" ), &GdSsPlayerNode2D::setFrame );
    ClassDB::bind_method( D_METHOD( "get_frame" ), &GdSsPlayerNode2D::getFrame );
    ClassDB::bind_method( D_METHOD( "get_frame_decimal" ), &GdSsPlayerNode2D::getFrameDecimal );

    ClassDB::bind_method( D_METHOD( "get_total_frames" ), &GdSsPlayerNode2D::getTotalFrames );

    ClassDB::bind_method( D_METHOD( "set_frame_rate", "fps" ), &GdSsPlayerNode2D::setFrameRate );
    ClassDB::bind_method( D_METHOD( "get_frame_rate" ), &GdSsPlayerNode2D::getFrameRate );

    ClassDB::bind_method( D_METHOD( "set_animation_section", "start", "end" ), &GdSsPlayerNode2D::setAnimationSection );
    ClassDB::bind_method( D_METHOD( "get_animation_section_start" ), &GdSsPlayerNode2D::getAnimationSectionStart );
    ClassDB::bind_method( D_METHOD( "get_animation_section_end" ), &GdSsPlayerNode2D::getAnimationSectionEnd );

    ClassDB::bind_method( D_METHOD( "set_playback_direction", "direction", "style" ), &GdSsPlayerNode2D::setPlaybackDirection );
    ClassDB::bind_method( D_METHOD( "get_playback_direction" ), &GdSsPlayerNode2D::getPlaybackDirection );
    ClassDB::bind_method( D_METHOD( "get_playback_style" ), &GdSsPlayerNode2D::getPlaybackStyle );

    ClassDB::bind_method( D_METHOD( "set_loop", "count" ), &GdSsPlayerNode2D::setLoop );
    ClassDB::bind_method( D_METHOD( "get_loop" ), &GdSsPlayerNode2D::getLoop );

    ClassDB::bind_method( D_METHOD( "set_skip_frames", "skip" ), &GdSsPlayerNode2D::setSkipFrames );
    ClassDB::bind_method( D_METHOD( "is_skip_frames" ), &GdSsPlayerNode2D::isSkipFrames );

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
			"GdSsabResource"
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

	ADD_GROUP( "Animation Settings", "" );
}

bool GdSsPlayerNode2D::_set( const StringName& p_name, const Variant& p_property ) {
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

bool GdSsPlayerNode2D::_get( const StringName& p_name, Variant& r_property ) const {
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
    } else if ( p_name == StringName("playing") ) {
        r_property = isPlaying();

        return	true;
    }

    return	false;
}

void GdSsPlayerNode2D::_get_property_list( List<PropertyInfo>* p_list ) const {
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

    animasPropertyInfo.name = "frame";
    animasPropertyInfo.type = Variant::INT;
    animasPropertyInfo.usage = PROPERTY_USAGE_EDITOR;
    animasPropertyInfo.hint = PROPERTY_HINT_RANGE;
    animasPropertyInfo.hint_string = "0," + String::num(getTotalFrames()-1) + ",1";
    p_list->push_back( animasPropertyInfo );

}

void GdSsPlayerNode2D::_notification( int p_notification ) {
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

void GdSsPlayerNode2D::loadTextures(const Ref<GdSsabResource>& ssabRes) {
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
    struct SsVertexComputeParams {
        Vector2 size;
        Vector2 pivot;
        bool flip_h = false;
        bool flip_v = false;
        bool use_5_vertices = true;
        bool has_deform = false;
        Vector2 deform_lt, deform_rt, deform_lb, deform_rb;
    };

    int compute_vertices(const SsVertexComputeParams& params, float* out_x, float* out_y) {
        float deform_x[4], deform_y[4];
        const float *p_dx = nullptr, *p_dy = nullptr;
        if (params.has_deform) {
            deform_x[0] = params.deform_lt.x; deform_x[1] = params.deform_rt.x;
            deform_x[2] = params.deform_lb.x; deform_x[3] = params.deform_rb.x;
            deform_y[0] = -params.deform_lt.y; deform_y[1] = -params.deform_rt.y;
            deform_y[2] = -params.deform_lb.y; deform_y[3] = -params.deform_rb.y;
            p_dx = deform_x; p_dy = deform_y;
        }

        int v_count = ss_vertex_compute_local(
            params.size.x, params.size.y,
            params.pivot.x, params.pivot.y,
            0, 0, // pivot_offset
            params.flip_h, params.flip_v,
            params.use_5_vertices,
            p_dx, p_dy,
            out_x, out_y
        );

        // Convert from Y-up (ssruntime) to Y-down (Godot)
        for (int i = 0; i < v_count; i++) {
            out_y[i] = -out_y[i];
        }

        return v_count;
    }

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
        bool use_5_vertices = true;
    };

    int compute_uvs(const SsUvComputeParams& params, float* out_u, float* out_v) {
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
            params.use_5_vertices,
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

void GdSsPlayerNode2D::updateAnimation( float delta ) {
    if (ss_runtime_is_playing(runtime_ctx)) {
        auto d = delta * 1000.0f;
        auto frame_no = ss_runtime_update(runtime_ctx, d);

        if (previous_frame_no == frame_no) {
            // print_line("skip: " + String::num(frame_no));
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
        previous_frame_no = frame_no;
        drawAnimation(frame_no);
    }
}

void GdSsPlayerNode2D::drawAnimation(int frame_no) {
    unsigned char *data = nullptr;
    uintptr_t len = 0;
    ss_runtime_get_frame_data(runtime_ctx, frame_no, &data, &len);
    if (!data) return;

    auto frameData = ss::runtime::GetFrameData(data);
    auto parts = frameData->parts();
    auto binary = _ssabRes->get_ss_anime_binary();
    RenderingServer *rs = RenderingServer::get_singleton();

    int num_parts = binary->parts()->size();
    if (_inheritance_matrices.size() < num_parts * 16) {
        _inheritance_matrices.resize(num_parts * 16);
    }

    for (uint32_t i = 0; i < parts->size(); i++) {
        auto part = parts->Get(i);
        auto partBinary = binary->parts()->Get(part->part_index());

        int parent_idx = partBinary->parent_index();
        float *parent_m = (parent_idx >= 0) ? _inheritance_matrices.ptrw() + (parent_idx * 16) : nullptr;
        float drawing_m[16];
        float *inheritance_m = _inheritance_matrices.ptrw() + (part->part_index() * 16);

        // TODO: Pass parent size if using NineSlice/etc. (not fully supported here yet)
        ss_matrix_compute_world(inheritance_m, drawing_m, part, parent_m, false, 0.0f, 0.0f);

        int p_idx = part->part_index();
        if (p_idx < 0 || p_idx >= (int)_canvas_items.size()) continue;

        RID ci = _canvas_items[p_idx];
        rs->canvas_item_clear(ci);
        rs->canvas_item_set_z_index(ci, (int)part->priority());
        if (part->hide()) continue;

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

void GdSsPlayerNode2D::_draw_part(RenderingServer *rs, RID ci, const ss::runtime::FrameData *frameData, const ss::runtime::PartState *part, const ss::format::PartData *partBinary, const Ref<Texture2D> &tex, const ss::format::Cell *cell, const float *draw_m) {
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

        constexpr int CORNERS_COUNT = 4;
        constexpr int MAX_VERTICES_COUNT = 5;
        constexpr int INDICES_COUNT_QUAD = 6;
        constexpr int INDICES_COUNT_PENTAGON = 12;

        float out_x[MAX_VERTICES_COUNT], out_y[MAX_VERTICES_COUNT];
        SsVertexComputeParams params;
        params.size = src_rect.size;
        params.pivot = Vector2(pivot->v1(), pivot->v2());
        params.flip_h = part->flip_h();
        params.flip_v = part->flip_v();
        params.use_5_vertices = true;

        if (flags & ss::runtime::UpdateAttributeFlags_AttributeVertex) {
            auto vd = frameData->vertices()->Get(part->vertex());
            params.has_deform = true;
            params.deform_lt = Vector2(vd->lt().x(), vd->lt().y());
            params.deform_rt = Vector2(vd->rt().x(), vd->rt().y());
            params.deform_lb = Vector2(vd->lb().x(), vd->lb().y());
            params.deform_rb = Vector2(vd->rb().x(), vd->rb().y());
        }

        int v_count = compute_vertices(params, out_x, out_y);

        if (v_count < CORNERS_COUNT) {
            return;
        }

        #ifdef SPRITESTUDIO_GODOT_EXTENSION
        PackedVector2Array p_verts; p_verts.resize(v_count);
        PackedVector2Array p_uvs; p_uvs.resize(v_count);
        PackedColorArray p_colors; p_colors.resize(v_count);
        PackedInt32Array p_indices; p_indices.resize((v_count == MAX_VERTICES_COUNT) ? INDICES_COUNT_PENTAGON : INDICES_COUNT_QUAD);
        #else
        Vector<Vector2> p_verts; p_verts.resize(v_count);
        Vector<Vector2> p_uvs; p_uvs.resize(v_count);
        Vector<Color> p_colors; p_colors.resize(v_count);
        Vector<int> p_indices; p_indices.resize((v_count == MAX_VERTICES_COUNT) ? INDICES_COUNT_PENTAGON : INDICES_COUNT_QUAD);
        #endif

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
        uv_params.use_5_vertices = true;

        compute_uvs(uv_params, out_u, out_v);

        Vector2 tex_size = tex->get_size();
        Color corner_colors[CORNERS_COUNT] = { Color(1, 1, 1, part->alpha()), Color(1, 1, 1, part->alpha()), Color(1, 1, 1, part->alpha()), Color(1, 1, 1, part->alpha()) };
        if (flags & ss::runtime::UpdateAttributeFlags_AttributePartColor) {
            auto pc = frameData->parts_color()->Get(part->part_color());
            auto to_color = [&](const ss::runtime::SsAttributePartColorKeyValueColor &c) { return Color(c.rgba().r()/255.0f, c.rgba().g()/255.0f, c.rgba().b()/255.0f, (c.rgba().a()/255.0f)*part->alpha()); };
            corner_colors[0] = to_color(pc->lt()); corner_colors[1] = to_color(pc->rt()); corner_colors[2] = to_color(pc->lb()); corner_colors[3] = to_color(pc->rb());
        }

        Transform2D draw_transform = matrix_to_transform2d(draw_m);

        for (int j = 0; j < CORNERS_COUNT; j++) {
            p_verts.set(j, draw_transform.xform(Vector2(out_x[j], out_y[j])));
            p_uvs.set(j, Vector2(out_u[j] / tex_size.x, out_v[j] / tex_size.y));
            p_colors.set(j, corner_colors[j]);
        }
        if (v_count == MAX_VERTICES_COUNT) {
            p_verts.set(CORNERS_COUNT, draw_transform.xform(Vector2(out_x[CORNERS_COUNT], out_y[CORNERS_COUNT])));
            p_uvs.set(CORNERS_COUNT, Vector2(out_u[CORNERS_COUNT] / tex_size.x, out_v[CORNERS_COUNT] / tex_size.y));
            p_colors.set(CORNERS_COUNT, (corner_colors[0] + corner_colors[1] + corner_colors[2] + corner_colors[3]) * 0.25f);
            int idxs[] = { 0,1,4, 1,3,4, 3,2,4, 2,0,4 };
            for(int k=0; k<INDICES_COUNT_PENTAGON; k++) p_indices.set(k, idxs[k]);
        } else {
            int idxs[] = { 0,1,2, 1,3,2 };
            for(int k=0; k<INDICES_COUNT_QUAD; k++) p_indices.set(k, idxs[k]);
        }
        rs->canvas_item_add_triangle_array(ci, p_indices, p_verts, p_colors, p_uvs, {}, {}, tex->get_rid());
    }
}

void GdSsPlayerNode2D::fetchAnimation() {
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

        previous_frame_no = -1;
        drawAnimation(ss_runtime_get_frame_no(runtime_ctx));
}
