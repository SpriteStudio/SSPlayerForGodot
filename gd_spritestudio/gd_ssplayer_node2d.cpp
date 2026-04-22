#include "gd_ssplayer_node2d.h"
#include "runtime/ssab.h"
#include "runtime/ssruntime.h"
#include "runtime/framedata.h"

GdSsPlayerNode2D::GdSsPlayerNode2D() {
    runtime_ctx = ss_runtime_create();
}

GdSsPlayerNode2D::~GdSsPlayerNode2D() {
    ss_runtime_destroy(runtime_ctx);
    runtime_ctx = nullptr;
}

void GdSsPlayerNode2D::setSsabResource( const Ref<GdSsabResource>& ssabRes ) {
	_ssabRes = ssabRes;
    _strAnimationSelected = "";
    if ( !_ssabRes.is_null() ) {
        auto vecAnimeName = _ssabRes->get_animation_names();
        if ( vecAnimeName.size() > 0 )
            _strAnimationSelected = vecAnimeName[0];
        loadTextures(_ssabRes);
    }

	fetchAnimation();
	NOTIFY_PROPERTY_LIST_CHANGED();

	// GdNotifier::getInstance().notifyResourcePlayerChanged( this );
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
    case NOTIFICATION_DRAW:
        drawAnimation();

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
        queue_redraw();
    }
}

void GdSsPlayerNode2D::drawAnimation() {
    if (_ssabRes.is_null() || !runtime_ctx) {
        return;
    }

    unsigned char *data = nullptr;
    uintptr_t len = 0;
    ss_runtime_get_frame_data(runtime_ctx, ss_runtime_get_frame_no(runtime_ctx), &data, &len);
    if (!data) {
        return;
    }

    auto frameData = ss::runtime::GetFrameData(data);
    auto parts = frameData->parts();
    if (!parts) {
        return;
    }

    auto binary = _ssabRes->get_ss_anime_binary();
    // print_line("draw frame: " + String::num(ss_runtime_get_frame_no(runtime_ctx)) + " parts: " + String::num(parts->size()));
    for (uint32_t i = 0; i < parts->size(); i++) {
        auto part = parts->Get(i);
        auto updateFlag = part->update_flag();
        if (updateFlag == 0) {
            continue;
        }

        auto frameDataCellIndex = part->cell();
        auto frameDataCell = frameData->cells()->Get(frameDataCellIndex);

        // 1. CellMap (テクスチャ情報) の取得
        int mapIdx = frameDataCell->map_id();
        if (mapIdx < 0 || mapIdx >= binary->cellmaps()->size()) {
            continue;
        }
        auto cellmap = binary->cellmaps()->Get(mapIdx);

        // 2. テクスチャの取得 (CellMap のハッシュを使用)
        uint32_t texHash = cellmap->name_hash();
        if (!_textures.has(texHash)) {
             ERR_PRINT("Texture not found for hash: " + String::num(texHash));
             continue;
        }
        Ref<Texture2D> tex = _textures[texHash];

        // 3. Cell (矩形情報) の取得
        uint32_t cellHash = frameDataCell->name_hash();
        const ss::format::Cell* cell = nullptr;
        if (cellmap->cells() != nullptr) {
            cell = cellmap->cells()->LookupByKey(cellHash);
        }

        if (!cell) {
            continue;
        }

        auto rect = cell->rectangle();
        auto pivot = cell->pivot();

         // --- 描画準備 ---
        // 1. パーツの Transform を計算
        Transform2D t;
        t.set_origin(Vector2(part->position_x(), part->position_y()));
        t.set_rotation(part->rotation_z());
        t.set_scale(Vector2(part->scale_x(), part->scale_y()));

        // Node2D 自体の Transform も考慮して描画座標系を設定
        draw_set_transform_matrix(get_transform() * t);

        // 2. セルの矩形 (Source Rect)
        Rect2 src_rect(rect->x1(), rect->y1(), rect->x2() - rect->x1(), rect->y2() - rect->y1());

        // 3. 描画位置のオフセット (Pivot反映)
        Vector2 draw_pos = Vector2(-src_rect.size.x * (pivot->v1() + 0.5f),
                                   -src_rect.size.y * (0.5f - pivot->v2()));

        // 4. 描画
        draw_texture_rect_region(tex, Rect2(draw_pos, src_rect.size), src_rect, Color(1, 1, 1, part->alpha()));
    }

    // 描画後は座標系をリセット
    draw_set_transform_matrix(Transform2D());
}

void GdSsPlayerNode2D::fetchAnimation() {
	if ( _strAnimationSelected.is_empty() || _ssabRes.is_null() ) {
        ss_runtime_reset(runtime_ctx);
        if (rutime_res != nullptr) {
            ss_resource_destroy(rutime_res);
            rutime_res = nullptr;
        }
        _currentAnimationData = nullptr;
    } else {
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
	}
}
