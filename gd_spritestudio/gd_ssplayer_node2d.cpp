#include "gd_ssplayer_node2d.h"
#include "runtime/ssab.h"
#include "runtime/ssruntime.h"

GdSsPlayerNode2D::GdSsPlayerNode2D() {
    rutime_ctx = ss_runtime_create();
}

GdSsPlayerNode2D::~GdSsPlayerNode2D() {
    ss_runtime_destroy(rutime_ctx);
    rutime_ctx = nullptr;
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
    return ss_runtime_is_playing(rutime_ctx);
}

void GdSsPlayerNode2D::play() {
    ss_runtime_play(rutime_ctx);
}

bool GdSsPlayerNode2D::isPausing() const {
    return ss_runtime_is_pausing(rutime_ctx);
}

void GdSsPlayerNode2D::pause() {
    ss_runtime_pause(rutime_ctx);
}

void GdSsPlayerNode2D::stop() {
    ss_runtime_stop(rutime_ctx);
}


void GdSsPlayerNode2D::_bind_methods() {
    ClassDB::bind_method( D_METHOD( "set_ssab_resource", "res_ssab" ), &GdSsPlayerNode2D::setSsabResource );
    ClassDB::bind_method( D_METHOD( "get_ssab_resource" ), &GdSsPlayerNode2D::getSsabResource );
    ClassDB::bind_method( D_METHOD( "set_animation", "name" ), &GdSsPlayerNode2D::setAnimation );
    ClassDB::bind_method( D_METHOD( "get_animation" ), &GdSsPlayerNode2D::getAnimation );
    ClassDB::bind_method( D_METHOD( "play" ), &GdSsPlayerNode2D::play );
    ClassDB::bind_method( D_METHOD( "pause" ), &GdSsPlayerNode2D::pause );
    ClassDB::bind_method( D_METHOD( "stop" ), &GdSsPlayerNode2D::stop );

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

	ADD_GROUP( "Animation Settings", "" );
}

bool GdSsPlayerNode2D::_set( const StringName& p_name, const Variant& p_property ) {
	if ( p_name == StringName("animation")) {
		setAnimation( p_property );

		return	true;
	} else if ( p_name == StringName("frame")) {
		//setFrame( p_property );

		return	true;
	} else if ( p_name == StringName("loop")) {
		//setLoop( p_property );

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
        // r_property = getFrame();

        return	true;
    } else if ( p_name == StringName("loop") ) {
        // r_property = getLoop();

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
        // drawAnimation();

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
    if (ss_runtime_is_playing(rutime_ctx)) {
        auto d = delta * 1000.0f;
        auto frame_no = ss_runtime_update(rutime_ctx, d);

        if (previous_frame_no == frame_no) {
            // print_line("skip");
            return;
        }

        // print_line("delta: " + String::num(d) + " Current Frame: " + String::num(frame_no));

        if (_currentAnimationData->events() != nullptr && _currentAnimationData->events()->size() > 0) {
            int min_frame;
            int max_frame;

            int prev = previous_frame_no == -1 ? ss_runtime_get_start_frame(rutime_ctx) : previous_frame_no;
            if (prev < frame_no) {
                min_frame = prev;
                max_frame = frame_no;
            } else {
                min_frame = frame_no;
                max_frame = prev;
            }

            // check Events
            for (int i=min_frame; i<=max_frame; i++) {
                auto eventsPerFrame = _currentAnimationData->events()->LookupByKey(i);
                if (eventsPerFrame == nullptr) continue;
                auto users = eventsPerFrame->users();
                if (users) {
                    // TODO: impl
                }
                auto singals = eventsPerFrame->signals();
                if (singals) {
                    // TODO: impl
                }
                auto instances = eventsPerFrame->instances();
                if (instances) {
                    // TODO: impl
                }
                auto effects = eventsPerFrame->effects();
                if (effects) {
                    // TODO: impl
                }
                auto audios = eventsPerFrame->audios();
                if (audios) {
                    // TODO: impl
                }
            }
        }

        previous_frame_no = frame_no;
    }
}


void GdSsPlayerNode2D::fetchAnimation() {
	if ( !_strAnimationSelected.is_empty() ) {
        if ( _ssabRes.is_null() ) {
            return;
        }

        bool loaded = ss_runtime_load_ssab_borrow(rutime_ctx, _ssabRes->get_data_ptr(), _ssabRes->get_data_size());
        if ( !loaded ) {
            ERR_PRINT( "SSAB Load Failed" );
            return;
        }

		auto c = _strAnimationSelected.utf8();
        auto animation = _ssabRes->find_animation( _strAnimationSelected );
		if ( !animation ) {
			ERR_PRINT( "Select Anime is Null" );
            return;
		}
        _currentAnimationData = animation;
        bool setup = ss_runtime_setup_animation(rutime_ctx, c.get_data());
        if ( !setup ) {
            ERR_PRINT( "SSAB Setup Animation Failed" );
            return;
        }

        previous_frame_no = -1;

    /*
		m_CellMapList->clear();

		int		idx = 0;

		for ( int i = 0; i < pAnimePack->cellmapNames.size(); i++ ) {
			Ref<GdResourceSsCellMap>	resCellMap = resProject->getCellMapResource( pAnimePack->cellmapNames[i] );

			if ( resCellMap.is_null() ) {
				continue;
			}

			SsCellMap*		pCellMap = resCellMap->getCellMap();

			if ( pCellMap ) {
				m_CellMapList->addIndex( pCellMap );
				m_CellMapList->addMap( pCellMap );

				( (SsTextureImpl*)m_CellMapList->getCellMapLink( idx++ )->tex )->setTexture( resCellMap->getTexture() );
			}
		}
		for ( int i = 0; i < pProject->cellmapNames.size(); i++ ) {
			Ref<GdResourceSsCellMap>	resCellMap = resProject->getCellMapResource( pProject->cellmapNames[i] );

			if ( resCellMap.is_null() ) {
				continue;
			}

			SsCellMap*		pCellMap = resCellMap->getCellMap();

			if ( pCellMap ) {
				m_CellMapList->addIndex( pCellMap );
				m_CellMapList->addMap( pCellMap );

				( (SsTextureImpl*)m_CellMapList->getCellMapLink( idx++ )->tex )->setTexture( resCellMap->getTexture() );
			}
		}

		if ( m_AnimeDecoder ) {
			m_Renderer.m_iSetup = 0;
			SsCurrentRenderer::SetCurrentRender( &m_Renderer );

			if ( pAnimation ) {
				float	fW = pAnimation->settings.canvasSize.x;
				float	fH = pAnimation->settings.canvasSize.y;
				float	fX = ( pAnimation->settings.pivot.x + 0.5f ) * fW;
				float	fY = ( pAnimation->settings.pivot.y + 0.5f ) * fH;
				int		iFps = pAnimation->settings.fps;

				m_Renderer.setCanvasItem( get_canvas_item() );
				m_Renderer.setCanvasSize( fW, fH );
				m_Renderer.setCanvasCenter( fX, fY );
				m_Renderer.setFps( iFps );
//				m_Renderer.setTextureInterpolate( m_bTextureInterpolate );	// Updated in drawAnimation()
			}

			m_Renderer.createPartSprites( &pAnimePack->Model, pProject );

			m_bAnimeDecoder = true;
			m_AnimeDecoder->setAnimation(
				&pAnimePack->Model,
				pAnimation,
				m_CellMapList.get(),
				pProject
			);

			setFrame( m_iFrame );
		}
    */
	}

}
