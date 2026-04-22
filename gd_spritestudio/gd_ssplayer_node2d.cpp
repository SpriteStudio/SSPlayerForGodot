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
    ss_context_set_coordinate_system(runtime_ctx, 1);
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

                if (auto instances = events_per_frame->instances()) {
                    for (auto instance : *instances) {
                        // TODO: impl
                    }
                }

                if (auto effects = events_per_frame->effects()) {
                    for (auto effect : *effects) {
                        // TODO: impl
                    }
                }
            }
        }
        */
        previous_frame_no = frame_no;
        drawAnimation();
    }
}

void GdSsPlayerNode2D::drawAnimation() {
    unsigned char *data = nullptr;
    uintptr_t len = 0;
    ss_runtime_get_frame_data(runtime_ctx, ss_runtime_get_frame_no(runtime_ctx), &data, &len);
    if (!data) {
        return;
    }

    auto frameData = ss::runtime::GetFrameData(data);
    auto parts = frameData->parts();

    auto binary = _ssabRes->get_ss_anime_binary();
    RenderingServer *rs = RenderingServer::get_singleton();

    for (uint32_t i = 0; i < parts->size(); i++) {
        auto part = parts->Get(i);
        if (i >= (uint32_t)_canvas_items.size()) {
            break;
        }

        if (part->update_flag() == 0) {
            continue;
        }

        RID ci = _canvas_items[i];
        rs->canvas_item_clear(ci);

        if (part->hide()) {
            continue;
        }

        auto frameDataCellIndex = part->cell();
        auto frameDataCell = frameData->cells()->Get(frameDataCellIndex);
        auto cellmap = binary->cellmaps()->Get(frameDataCell->map_id());

        uint32_t texHash = cellmap->name_hash();
        if (!_textures.has(texHash)) {
             continue;
        }
        Ref<Texture2D> tex = _textures[texHash];

        auto cell = cellmap->cells()->LookupByKey(frameDataCell->name_hash());
        if (!cell) {
            continue;
        }

        auto rect = cell->rectangle();
        auto pivot = cell->pivot();

        // 1. Blend Mode 設定
        auto partBinary = binary->parts()->Get(part->part_index());
        ss::format::BlendType ss_blend = partBinary->blend_type();
        if (!_blend_materials.has((int)ss_blend)) {
            Ref<CanvasItemMaterial> mat;
            mat.instantiate();
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

        // 2. 高度な描画が必要か判定
        uint64_t flags = part->update_flag();
        bool use_advanced = (flags & ss::runtime::UpdateAttributeFlags_AttributeVertex);
        use_advanced |= (flags & ss::runtime::UpdateAttributeFlags_AttributePartColor);
        use_advanced |= (flags & (ss::runtime::UpdateAttributeFlags_AttributeUvtX | ss::runtime::UpdateAttributeFlags_AttributeUvtY |
                                  ss::runtime::UpdateAttributeFlags_AttributeUvrZ | ss::runtime::UpdateAttributeFlags_AttributeUvsX |
                                  ss::runtime::UpdateAttributeFlags_AttributeUvsY));

        Rect2 src_rect(rect->x1(), rect->y1(), rect->x2(), rect->y2());

        if (!use_advanced && partBinary->part_type_type() != ss::format::PartType_PartTypeMesh) {
            // --- 通常パス: Transform2D を利用した高速描画 ---
            Transform2D t;
            t.set_origin(Vector2(part->position_x(), part->position_y()));
            t.set_rotation(part->rotation_z());
            t.set_scale(Vector2(part->scale_x(), part->scale_y()));
            rs->canvas_item_set_transform(ci, t);

            Vector2 draw_pos = Vector2(-src_rect.size.x * (pivot->v1() + 0.5f),
                                       -src_rect.size.y * (0.5f - pivot->v2()));

            rs->canvas_item_add_texture_rect_region(ci, Rect2(draw_pos, src_rect.size), tex->get_rid(), src_rect, Color(1, 1, 1, part->alpha()));
        } else if (partBinary->part_type_type() != ss::format::PartType_PartTypeMesh) {
            // --- 高度パス: 頂点配列 (Triangle Array) による描画 (Normalパーツのみ) ---
            rs->canvas_item_set_transform(ci, Transform2D());

            float w = src_rect.size.x;
            float h = src_rect.size.y;
            float px = -w * (pivot->v1() + 0.5f);
            float py = -h * (0.5f - pivot->v2());

            Vector2 verts[4] = {
                Vector2(px, py),
                Vector2(px + w, py),
                Vector2(px, py + h),
                Vector2(px + w, py + h)
            };

            if (flags & ss::runtime::UpdateAttributeFlags_AttributeVertex) {
                auto vd = frameData->vertices()->Get(part->vertex());
                verts[0] += Vector2(vd->lt().x(), vd->lt().y());
                verts[1] += Vector2(vd->rt().x(), vd->rt().y());
                verts[2] += Vector2(vd->lb().x(), vd->lb().y());
                verts[3] += Vector2(vd->rb().x(), vd->rb().y());
            }

            Transform2D t;
            t.set_origin(Vector2(part->position_x(), part->position_y()));
            t.set_rotation(part->rotation_z());
            t.set_scale(Vector2(part->scale_x(), part->scale_y()));
            for (int j = 0; j < 4; j++) {
                verts[j] = t.xform(verts[j]);
            }

            Vector2 uvs[4] = {
                Vector2(src_rect.position.x, src_rect.position.y),
                Vector2(src_rect.position.x + src_rect.size.x, src_rect.position.y),
                Vector2(src_rect.position.x, src_rect.position.y + src_rect.size.y),
                Vector2(src_rect.position.x + src_rect.size.x, src_rect.position.y + src_rect.size.y)
            };

            if (flags & (ss::runtime::UpdateAttributeFlags_AttributeUvtX | ss::runtime::UpdateAttributeFlags_AttributeUvtY |
                         ss::runtime::UpdateAttributeFlags_AttributeUvrZ | ss::runtime::UpdateAttributeFlags_AttributeUvsX |
                         ss::runtime::UpdateAttributeFlags_AttributeUvsY)) {
                Vector2 uv_center = src_rect.position + src_rect.size * 0.5f;
                for (int j = 0; j < 4; j++) {
                    Vector2 uv = uvs[j];
                    uv.x = (uv.x - uv_center.x) * part->uv_scale_x() + uv_center.x;
                    uv.y = (uv.y - uv_center.y) * part->uv_scale_y() + uv_center.y;
                    if (part->uv_rotation_z() != 0.0f) {
                        float s = Math::sin(part->uv_rotation_z());
                        float c = Math::cos(part->uv_rotation_z());
                        float rel_x = uv.x - uv_center.x;
                        float rel_y = uv.y - uv_center.y;
                        uv.x = rel_x * c - rel_y * s + uv_center.x;
                        uv.y = rel_x * s + rel_y * c + uv_center.y;
                    }
                    uv.x += part->uv_translation_x() * src_rect.size.x;
                    uv.y += part->uv_translation_y() * src_rect.size.y;
                    uvs[j] = uv;
                }
            }
            
            Vector2 tex_size = tex->get_size();
            for (int j = 0; j < 4; j++) {
                uvs[j].x /= tex_size.x;
                uvs[j].y /= tex_size.y;
            }

            Color colors[4] = {
                Color(1, 1, 1, part->alpha()),
                Color(1, 1, 1, part->alpha()),
                Color(1, 1, 1, part->alpha()),
                Color(1, 1, 1, part->alpha())
            };

            if (flags & ss::runtime::UpdateAttributeFlags_AttributePartColor) {
                auto pc = frameData->parts_color()->Get(part->part_color());
                colors[0] = Color(pc->lt().rgba().r() / 255.0f, pc->lt().rgba().g() / 255.0f, pc->lt().rgba().b() / 255.0f, (pc->lt().rgba().a() / 255.0f) * part->alpha());
                colors[1] = Color(pc->rt().rgba().r() / 255.0f, pc->rt().rgba().g() / 255.0f, pc->rt().rgba().b() / 255.0f, (pc->rt().rgba().a() / 255.0f) * part->alpha());
                colors[2] = Color(pc->lb().rgba().r() / 255.0f, pc->lb().rgba().g() / 255.0f, pc->lb().rgba().b() / 255.0f, (pc->lb().rgba().a() / 255.0f) * part->alpha());
                colors[3] = Color(pc->rb().rgba().r() / 255.0f, pc->rb().rgba().g() / 255.0f, pc->rb().rgba().b() / 255.0f, (pc->rb().rgba().a() / 255.0f) * part->alpha());
            }

            #ifdef SPRITESTUDIO_GODOT_EXTENSION
            PackedVector2Array p_verts; p_verts.resize(4);
            PackedVector2Array p_uvs; p_uvs.resize(4);
            PackedColorArray p_colors; p_colors.resize(4);
            PackedInt32Array p_indices; p_indices.resize(6);
            #else
            Vector<Vector2> p_verts; p_verts.resize(4);
            Vector<Vector2> p_uvs; p_uvs.resize(4);
            Vector<Color> p_colors; p_colors.resize(4);
            Vector<int> p_indices; p_indices.resize(6);
            #endif

            for(int j=0; j<4; j++) {
                p_verts.set(j, verts[j]);
                p_uvs.set(j, uvs[j]);
                p_colors.set(j, colors[j]);
            }
            p_indices.set(0, 0); p_indices.set(1, 1); p_indices.set(2, 2);
            p_indices.set(3, 1); p_indices.set(4, 3); p_indices.set(5, 2);

            rs->canvas_item_add_triangle_array(ci, p_indices, p_verts, p_colors, p_uvs, {}, {}, tex->get_rid());
        }
    }
}

void GdSsPlayerNode2D::fetchAnimation() {
	if ( _strAnimationSelected.is_empty() || _ssabRes.is_null() ) {
        ss_runtime_reset(runtime_ctx);
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
        drawAnimation();
}
