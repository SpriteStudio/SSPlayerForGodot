/*!
* \file		gd_node_ssplayer.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_node_ssplayer.h"

#include "SpriteStudio6-SDK/Common/Animator/ssplayer_render.h"

#include "ss_macros.h"
#include "ss_loader_texture_impl.h"
#include "ss_texture_impl.h"
#include "gd_notifier.h"

SsSdkUsing

static SsTextureImpl		s_TextureLoader;
static SSTextureFactory		s_TextureFactory( &s_TextureLoader, false );

GdNodeSsPlayer::GdNodeSsPlayer()
: m_strAnimePackSelected( "" )
, m_strAnimationSelected( "" )
, m_iFrame( 0 )
, m_fDelta( 0.0f )
, m_iLive( 0 )
, m_pLive( NULL )
, m_bLoop( false )
, m_bPlay( false )
, m_bPause( false )
, m_bTextureInterpolate( true )
{
	GdNotifier::getInstance().addItem( this );

	SsLoaderTextureImpl::setCallbacks();

	m_Renderer.m_iSetup = 0;
	SsCurrentRenderer::SetCurrentRender( &m_Renderer );

	m_bAnimeDecoder = false;
	m_AnimeDecoder.reset( new SsAnimeDecoder() );

	m_CellMapList.reset( new SsCellMapList() );
}

GdNodeSsPlayer::~GdNodeSsPlayer()
{
	GdNotifier::getInstance().removeItem( this );

	m_Renderer.m_iSetup = 0;
	SsCurrentRenderer::SetCurrentRender( &m_Renderer );

	// 二重解放対策：SsAnimeDecoder 側に生ポインタの所有権を渡していない場合に限り解放する。
	if (m_bAnimeDecoder)
	{
		// 解放のみ
		m_CellMapList.release();
	}
	else
	{
		// ここで削除
		m_CellMapList.reset();
	}

	m_bAnimeDecoder = false;
	m_AnimeDecoder.reset();
	//m_CellMapList.reset();

	m_bAnimeDecoder = false;
	//m_AnimeDecoder = 0;
	//m_CellMapList = 0;
}

void GdNodeSsPlayer::resourcePlayerChanged( const Ref<GdResourceSsPlayer>& resPlayer )
{
	fetchAnimation();

	NOTIFY_PROPERTY_LIST_CHANGED();
}

void GdNodeSsPlayer::resourceProjectChanged( const Ref<GdResourceSsProject>& resProject )
{
	fetchAnimation();

	NOTIFY_PROPERTY_LIST_CHANGED();
}

void GdNodeSsPlayer::resourceAnimePackChanged( const Ref<GdResourceSsAnimePack>& resAnimePack )
{
	fetchAnimation();

	NOTIFY_PROPERTY_LIST_CHANGED();
}

void GdNodeSsPlayer::resourceCellMapChanged( const Ref<GdResourceSsCellMap>& resCellMap )
{
	fetchAnimation();

	NOTIFY_PROPERTY_LIST_CHANGED();
}

void GdNodeSsPlayer::setPlayerResource( const Ref<GdResourceSsPlayer>& resPlayer )
{
	m_ResPlayer = resPlayer;

	m_strAnimationSelected.resize(0);
	fetchAnimation();

	NOTIFY_PROPERTY_LIST_CHANGED();
}

Ref<GdResourceSsPlayer> GdNodeSsPlayer::getPlayerResource() const
{
	return	m_ResPlayer;
}

void GdNodeSsPlayer::setAnimePack( const String& strName )
{
	m_strAnimePackSelected = strName;

	Ref<GdResourceSsProject>	resProject = m_ResPlayer->getProjectResource();
	Ref<GdResourceSsAnimePack>	resAnimePack;

	if ( !resProject.is_null() ) {
		resAnimePack = resProject->getAnimePackResource( m_strAnimePackSelected );
	}

	m_ResAnimePack = resAnimePack;

	if ( m_ResAnimePack.is_null() ) {
		ERR_PRINT( "AnimePack Load Error : " + m_strAnimePackSelected );
		return;
	}

	postAnimePackChanged( m_strAnimePackSelected );

	m_strAnimationSelected.resize(0);
	fetchAnimation();

	NOTIFY_PROPERTY_LIST_CHANGED();
}

String GdNodeSsPlayer::getAnimePack() const
{
	return	m_strAnimePackSelected;
}

void GdNodeSsPlayer::setAnimation( const String& strName )
{
	m_strAnimationSelected = strName;

	postAnimationChanged( m_strAnimationSelected );

	fetchAnimation();

	NOTIFY_PROPERTY_LIST_CHANGED();
}

String GdNodeSsPlayer::getAnimation() const
{
	return	m_strAnimationSelected;
}

void GdNodeSsPlayer::setFrame( int iFrame )
{
	m_iFrame = iFrame;

	if ( m_ResAnimePack.is_null() ) {
		return;
	}

	auto			c = m_strAnimationSelected.utf8();
	SsString		strAnimationName = c.ptr() ? SsString( c.ptr() ) : SsString( "" );
	SsAnimePack*	pAnimePack = m_ResAnimePack->getAnimePack();

	if ( !pAnimePack ) {
		return;
	}

	SsAnimation*	pAnimation = pAnimePack->findAnimation( strAnimationName );

	if ( pAnimation ) {
		int		iRange = pAnimation->settings.endFrame - pAnimation->settings.startFrame + 1;
		int		iRel = iFrame - pAnimation->settings.startFrame;

		while ( iRel < 0 ) {
			iRel += iRange;
		}
		while ( iRel >= iRange ) {
			iRel -= iRange;
		}

		iFrame = iRel + pAnimation->settings.startFrame;
	}

	if ( m_bAnimeDecoder && m_AnimeDecoder ) {
		m_Renderer.m_iSetup = 0;
		SsCurrentRenderer::SetCurrentRender( &m_Renderer );

		m_AnimeDecoder->setPlayFrame( iFrame );

		auto	mapUserData = m_Renderer.getUserData();

		if ( mapUserData.find( iFrame ) != mapUserData.end() ) {
			auto	userData = mapUserData[iFrame];
			int		iFlag;
			int		iIntValue;
			Rect2	rectValue;
			Point2	pointValue;
			String	strStringValue;

			iFlag = 0;
			if ( userData.useInteger ) iFlag |= 0x01;
			if ( userData.useRect ) iFlag |= 0x02;
			if ( userData.usePoint ) iFlag |= 0x04;
			if ( userData.useString ) iFlag |= 0x08;

			iIntValue = userData.integer;

			rectValue.position.x = userData.rect.x;
			rectValue.position.y = userData.rect.y;
			rectValue.size.width = userData.rect.w;
			rectValue.size.height = userData.rect.h;

			pointValue.x = userData.point.x;
			pointValue.y = userData.point.y;

			strStringValue = String::utf8( userData.string.c_str() );

			postUserData( iFlag, iIntValue, rectValue, pointValue, strStringValue );
		}

		auto	mapSignal = m_Renderer.getSignal();

		if ( mapSignal.find( iFrame ) != mapSignal.end() ) {
			auto	signalAttr = mapSignal[iFrame];

			const std::vector<SsSignalCommand>&		vec = signalAttr.commands;

			for ( int j = 0; j < vec.size(); j++ ) {
				const SsSignalCommand&	command = vec.at( j );
				String					strCommandId = String::utf8( command.commandId.c_str() );

				if ( command.active ) {
					Dictionary	dic;

					for ( int k = 0; k < command.params.size(); k++ ) {
						const SsSignalParam&	param = command.params.at( k );
						String					strParamId = String::utf8( param.paramId.c_str() );

						switch ( static_cast<int>(param.type) ) {
						case SsSignalParamType::index :
						case SsSignalParamType::integer :
							dic[strParamId] = Variant( param.value.i );
							break;
						case SsSignalParamType::floating :
							dic[strParamId] = Variant( param.value.f );
							break;
						}
					}

					postSignal( strCommandId, dic );
				}
			}
		}
	}

	postFrameChanged( m_iFrame );
}

int GdNodeSsPlayer::getFrame() const
{
	return	m_iFrame;
}

int GdNodeSsPlayer::getStartFrame() const
{
	if ( !m_ResAnimePack.is_null() ) {
		auto			c = m_strAnimationSelected.utf8();
		SsString		strAnimationName = c.ptr() ? SsString( c.ptr() ) : SsString( "" );
		SsAnimePack*	pAnimePack = m_ResAnimePack->getAnimePack();

		if ( !pAnimePack ) {
			return	0;
		}

		SsAnimation*	pAnimation = pAnimePack->findAnimation( strAnimationName );

		if ( pAnimation ) {
			return	pAnimation->settings.startFrame;
		}
	}

	return	0;
}

int GdNodeSsPlayer::getEndFrame() const
{
	if ( !m_ResAnimePack.is_null() ) {
		auto			c = m_strAnimationSelected.utf8();
		SsString		strAnimationName = c.ptr() ? SsString( c.ptr() ) : SsString( "" );
		SsAnimePack*	pAnimePack = m_ResAnimePack->getAnimePack();

		if ( !pAnimePack ) {
			return	0;
		}

		SsAnimation*	pAnimation = pAnimePack->findAnimation( strAnimationName );

		if ( pAnimation ) {
			return	pAnimation->settings.endFrame;
		}
	}

	return	0;
}

int GdNodeSsPlayer::getFps() const
{
	if ( !m_ResAnimePack.is_null() ) {
		auto			c = m_strAnimationSelected.utf8();
		SsString		strAnimationName = c.ptr() ? SsString( c.ptr() ) : SsString( "" );
		SsAnimePack*	pAnimePack = m_ResAnimePack->getAnimePack();

		if ( !pAnimePack ) {
			return	0;
		}

		SsAnimation*	pAnimation = pAnimePack->findAnimation( strAnimationName );

		if ( pAnimation ) {
			return	pAnimation->settings.fps;
		}
	}

	return	0;
}

void GdNodeSsPlayer::setTextureInterpolate( bool bSwitch )
{
	m_bTextureInterpolate = bSwitch;
}

bool GdNodeSsPlayer::getTextureInterpolate() const
{
	return m_bTextureInterpolate;
}

void GdNodeSsPlayer::setLoop( bool bLoop )
{
	m_bLoop = bLoop;
}

bool GdNodeSsPlayer::getLoop() const
{
	return	m_bLoop;
}

void GdNodeSsPlayer::setPlay( bool bPlay )
{
	m_bPlay = bPlay;

	if ( bPlay ) {
		int		iFrame = getFrame();
		int		iRange = getEndFrame() - getStartFrame() + 1;

		if ( false ) {
			if ( iFrame <= getStartFrame() ) {
				while ( iFrame <= getStartFrame() ) {
					iFrame += iRange;
				}
				setFrame( iFrame );
			}
		}else{
			if ( iFrame >= getEndFrame() ) {
				while ( iFrame >= getEndFrame() ) {
					iFrame -= iRange;
				}
				setFrame( iFrame );
			}
		}

		pause( false );
	}

	if ( m_AnimeDecoder ) {
		std::vector<SsPartAndAnime>&	anim = m_AnimeDecoder->getPartAnime();

		if ( m_pLive && anim.size() != m_iLive ) {
			delete[] m_pLive;
			m_pLive = NULL;
		}
		m_iLive = anim.size();
		if ( !m_pLive && m_iLive ) {
			m_pLive = new float[m_iLive];
		}
		if ( m_pLive && m_iLive ) {
			memset( m_pLive, 0, sizeof( float ) * m_iLive );
		}
	}
}

bool GdNodeSsPlayer::getPlay() const
{
	return	m_bPlay;
}

void GdNodeSsPlayer::play()
{
	setFrame( 0 );
	setPlay( true );
}

void GdNodeSsPlayer::pause( bool b )
{
	m_bPause = b;
}

void GdNodeSsPlayer::stop()
{
	setPlay( false );
	setFrame( 0 );
}

void GdNodeSsPlayer::_bind_methods()
{
	ClassDB::bind_method( D_METHOD( "set_player_resource", "res_player" ), &GdNodeSsPlayer::setPlayerResource );
	ClassDB::bind_method( D_METHOD( "get_player_resource" ), &GdNodeSsPlayer::getPlayerResource );
	ClassDB::bind_method( D_METHOD( "set_anime_pack", "name" ), &GdNodeSsPlayer::setAnimePack );
	ClassDB::bind_method( D_METHOD( "get_anime_pack" ), &GdNodeSsPlayer::getAnimePack );
	ClassDB::bind_method( D_METHOD( "set_animation", "name" ), &GdNodeSsPlayer::setAnimation );
	ClassDB::bind_method( D_METHOD( "get_animation" ), &GdNodeSsPlayer::getAnimation );
	ClassDB::bind_method( D_METHOD( "set_frame", "frame" ), &GdNodeSsPlayer::setFrame );
	ClassDB::bind_method( D_METHOD( "get_frame" ), &GdNodeSsPlayer::getFrame );
	ClassDB::bind_method( D_METHOD( "get_start_frame" ), &GdNodeSsPlayer::getStartFrame );
	ClassDB::bind_method( D_METHOD( "get_end_frame" ), &GdNodeSsPlayer::getEndFrame );
	ClassDB::bind_method( D_METHOD( "get_fps" ), &GdNodeSsPlayer::getFps );
	ClassDB::bind_method( D_METHOD( "set_loop", "loop" ), &GdNodeSsPlayer::setLoop );
	ClassDB::bind_method( D_METHOD( "get_loop" ), &GdNodeSsPlayer::getLoop );
	ClassDB::bind_method( D_METHOD( "set_play", "play" ), &GdNodeSsPlayer::setPlay );
	ClassDB::bind_method( D_METHOD( "get_play" ), &GdNodeSsPlayer::getPlay );
	ClassDB::bind_method( D_METHOD( "set_texture_interpolate", "interpolate" ), &GdNodeSsPlayer::setTextureInterpolate );
	ClassDB::bind_method( D_METHOD( "get_texture_interpolate" ), &GdNodeSsPlayer::getTextureInterpolate );
	ClassDB::bind_method( D_METHOD( "play" ), &GdNodeSsPlayer::play );
	ClassDB::bind_method( D_METHOD( "pause", "b" ), &GdNodeSsPlayer::pause );
	ClassDB::bind_method( D_METHOD( "stop" ), &GdNodeSsPlayer::stop );

	ADD_SIGNAL( MethodInfo( "anime_pack_changed", PropertyInfo( Variant::STRING, GdUiText( "name" ) ) ) );
	ADD_SIGNAL( MethodInfo( "animation_changed", PropertyInfo( Variant::STRING, GdUiText( "name" ) ) ) );
	ADD_SIGNAL( MethodInfo( "animation_finished", PropertyInfo( Variant::STRING, GdUiText( "name" ) ) ) );
	ADD_SIGNAL( MethodInfo( "frame_changed", PropertyInfo( Variant::INT, GdUiText( "frame" ) ) ) );

	ADD_SIGNAL(
		MethodInfo(
			"user_data",
			PropertyInfo(
				Variant::INT,
				GdUiText( "flag" )
			),
			PropertyInfo(
				Variant::INT,
				GdUiText( "int_value" )
			),
			PropertyInfo(
				Variant::RECT2,
				GdUiText( "rect_value" )
			),
			PropertyInfo(
				Variant::VECTOR2,
				GdUiText( "point_value" )
			),
			PropertyInfo(
				Variant::STRING,
				GdUiText( "string_value" )
			)
		)
	);
	ADD_SIGNAL(
		MethodInfo(
			"signal",
			PropertyInfo(
				Variant::STRING,
				GdUiText( "command" )
			),
			PropertyInfo(
				Variant::DICTIONARY,
				GdUiText( "value" )
			)
		)
	);

	ADD_PROPERTY(
		PropertyInfo(
			Variant::OBJECT,
			GdUiText( "res_player" ),
			PropertyHint::PROPERTY_HINT_RESOURCE_TYPE,
			"GdResourceSsPlayer"
		),
		"set_player_resource",
		"get_player_resource"
	);

	ADD_GROUP( GdUiText( "Animation Settings" ), "" );
}

bool GdNodeSsPlayer::_set( const StringName& p_name, const Variant& p_property )
{
	if ( p_name == StringName(GdUiText( "anime_pack" )) ) {
		setAnimePack( p_property );

		if ( !m_ResAnimePack.is_null() ) {
			SsAnimePack*	pAnimePack = m_ResAnimePack->getAnimePack();

			if ( !pAnimePack ) {
				return	true;
			}

			const std::vector<SsAnimation*>&	listAnimation = pAnimePack->animeList;

			for ( int i = 0; i < listAnimation.size(); i++ ) {
				SsAnimation*	pAnimation = listAnimation[i];

				if ( !pAnimation->isSetup ) {
					String	strName = String::utf8( pAnimation->name.c_str() );

					setAnimation( strName );
					break;
				}
			}
		}

		return	true;
	}else
	if ( p_name == StringName(GdUiText( "animation" )) ) {
		setAnimation( p_property );

		return	true;
	}else
	if ( p_name == StringName(GdUiText( "frame" )) ) {
		setFrame( p_property );

		return	true;
	}else
	if ( p_name == StringName(GdUiText( "loop" )) ) {
		setLoop( p_property );

		return	true;
	}else
	if ( p_name == StringName(GdUiText( "playing" )) ) {
		setPlay( p_property );

		return	true;
	}
	if ( p_name == StringName(GdUiText( "texture_interpolate" )) ) {
		setTextureInterpolate( p_property );

		return	true;
	}

	return	false;
}

bool GdNodeSsPlayer::_get( const StringName& p_name, Variant& r_property ) const
{
	if ( p_name == StringName(GdUiText( "anime_pack" )) ) {
		r_property = getAnimePack();

		return	true;
	}else
	if ( p_name == StringName(GdUiText( "animation" )) ) {
		r_property = getAnimation();

		return	true;
	}else
	if ( p_name == StringName(GdUiText( "frame" )) ) {
		r_property = getFrame();

		return	true;
	}else
	if ( p_name == StringName(GdUiText( "loop" )) ) {
		r_property = getLoop();

		return	true;
	}else
	if ( p_name == StringName(GdUiText( "playing" )) ) {
		r_property = getPlay();

		return	true;
	}
	if ( p_name == StringName(GdUiText( "texture_interpolate" )) ) {
		r_property = getTextureInterpolate();

		return	true;
	}

	return	false;
}

void GdNodeSsPlayer::_get_property_list( List<PropertyInfo>* p_list ) const
{
#ifdef SPRITESTUDIO_GODOT_EXTENSION
	PackedStringArray vecAnimePackName;
#else
	Vector<String>	vecAnimePackName;
#endif

	vecAnimePackName.insert( 0, GdUiText( "-- Empty --" ) );

	if ( !m_ResPlayer.is_null() ) {
		Ref<GdResourceSsProject>	resProject = m_ResPlayer->getProjectResource();

		if ( !resProject.is_null() ) {
			auto			vec = resProject->getAnimePackNames();

			for ( int i = 0; i < vec.size(); i++ ) {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
				String		str = vec[ i ];
#else
				String		str = vec.get( i );
#endif

				vecAnimePackName.push_back( str );
			}
		}
	}

	PropertyInfo	animePacksPropertyInfo;

	animePacksPropertyInfo.name = GdUiText( "anime_pack" );
	animePacksPropertyInfo.type = Variant::STRING;
	animePacksPropertyInfo.usage = PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE;
	animePacksPropertyInfo.hint_string = String( "," ).join( vecAnimePackName );
	animePacksPropertyInfo.hint = PROPERTY_HINT_ENUM;

	p_list->push_back( animePacksPropertyInfo );

	if ( !m_ResAnimePack.is_null() ) {
		PropertyInfo	animationsPropertyInfo;
#ifdef SPRITESTUDIO_GODOT_EXTENSION
		PackedStringArray	vecAnimationName = m_ResAnimePack->getAnimationNames();
#else
		Vector<String>	vecAnimationName = m_ResAnimePack->getAnimationNames();
#endif

		animationsPropertyInfo.name = GdUiText( "animation" );
		animationsPropertyInfo.type = Variant::STRING;
		animationsPropertyInfo.usage = PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE;
		animationsPropertyInfo.hint_string = String( "," ).join( vecAnimationName );
		animationsPropertyInfo.hint = PROPERTY_HINT_ENUM;

		p_list->push_back( animationsPropertyInfo );

		auto			c = m_strAnimationSelected.utf8();
		SsString		strAnimationName = c.ptr() ? SsString( c.ptr() ) : SsString( "" );
		SsAnimePack*	pAnimePack = m_ResAnimePack->getAnimePack();

		if ( !pAnimePack ) {
			return;
		}

		SsAnimation*	pAnimation = pAnimePack->findAnimation( strAnimationName );

		if ( pAnimation ) {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
			PackedStringArray	vecRange;
#else
			Vector<String>	vecRange;
#endif

			vecRange.push_back( String::num( pAnimation->settings.startFrame ) );
			vecRange.push_back( String::num( pAnimation->settings.endFrame ) );
			vecRange.push_back( String::num( 0.1 ) );

			animationsPropertyInfo.name = GdUiText( "frame" );
			animationsPropertyInfo.type = VARIANT_FLOAT;

			animationsPropertyInfo.usage = PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE;
			animationsPropertyInfo.hint_string = String( "," ).join( vecRange );
			animationsPropertyInfo.hint = PROPERTY_HINT_RANGE;

			p_list->push_back( animationsPropertyInfo );

			animationsPropertyInfo.name = GdUiText( "loop" );
			animationsPropertyInfo.type = Variant::BOOL;
			animationsPropertyInfo.usage = PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE;
			animationsPropertyInfo.hint = PROPERTY_HINT_NONE;

			p_list->push_back( animationsPropertyInfo );

			animationsPropertyInfo.name = GdUiText( "playing" );
			animationsPropertyInfo.type = Variant::BOOL;
			animationsPropertyInfo.usage = PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE;
			animationsPropertyInfo.hint = PROPERTY_HINT_NONE;

			p_list->push_back( animationsPropertyInfo );

			animationsPropertyInfo.name = GdUiText( "texture_interpolate" );
			animationsPropertyInfo.type = Variant::BOOL;
			animationsPropertyInfo.usage = PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE;
			animationsPropertyInfo.hint = PROPERTY_HINT_NONE;

			p_list->push_back( animationsPropertyInfo );
		}
	}
}

void GdNodeSsPlayer::_notification( int p_notification )
{
	switch ( p_notification ) {
	case NOTIFICATION_READY :
		set_process_internal( true );

		break;
	case NOTIFICATION_INTERNAL_PROCESS :
		updateAnimation( get_process_delta_time() );

		break;
	case NOTIFICATION_DRAW :
		drawAnimation();

		break;
	default :
		break;
	}
}

void GdNodeSsPlayer::postAnimePackChanged( const String& strName )
{
	Error	err;

	err = emit_signal( "anime_pack_changed", Variant( strName ) );
}

void GdNodeSsPlayer::postAnimationChanged( const String& strName )
{
	Error	err;

	err = emit_signal( "animation_changed", Variant( strName ) );
}

void GdNodeSsPlayer::postAnimationFinished( const String& strName )
{
	Error	err;

	err = emit_signal( "animation_finished", Variant( strName ) );
}

void GdNodeSsPlayer::postFrameChanged( int iFrame )
{
	Error	err;

	err = emit_signal( "frame_changed", Variant( iFrame ) );
}

void GdNodeSsPlayer::postUserData( int iFlag, int iIntValue, const Rect2& rectValue, const Vector2& pointValue, const String& strStringValue )
{
	Error	err;

	err = emit_signal( "user_data", Variant( iFlag ), Variant( iIntValue ), Variant( rectValue ), Variant( pointValue ), Variant( strStringValue ) );
}

void GdNodeSsPlayer::postSignal( const String& strName, const Dictionary& dicParam )
{
	Error	err;

	err = emit_signal( "signal", Variant( strName ), Variant( dicParam ) );
}

void GdNodeSsPlayer::updateAnimation( float delta )
{
	int		iDelta = 0;

	if ( getPlay() ) {
		if ( m_bPause ) {
		}else{
			m_fDelta += delta * m_Renderer.getFps();

			iDelta = (int)m_fDelta;

			if ( iDelta != 0 ) {
				int		iFrame = getFrame() + iDelta;
				int		iRange = getEndFrame() - getStartFrame() + 1;

				m_fDelta -= iDelta;

				if ( iFrame < getStartFrame() ) {
					if ( getLoop() ) {
						while ( iFrame < getStartFrame() ) {
							iFrame += iRange;

							postAnimationFinished( m_strAnimationSelected );
						}
					}else{
						iFrame = getStartFrame();

						postAnimationFinished( m_strAnimationSelected );

						pause( true );
					}
				}else
				if ( iFrame > getEndFrame() ) {
					if ( getLoop() ) {
						while ( iFrame > getEndFrame() ) {
							iFrame -= iRange;

							postAnimationFinished( m_strAnimationSelected );
						}
					}else{
						iFrame = getEndFrame();

						postAnimationFinished( m_strAnimationSelected );

						pause( true );
					}
				}

				set( "frame", iFrame );
			}
		}
	}

	if ( iDelta != 0 ) {
		std::vector<SsPartAndAnime>&	anim = m_AnimeDecoder->getPartAnime();
		std::vector<SsPartState>&		stat = m_AnimeDecoder->getPartState();

		if ( m_pLive && anim.size() != m_iLive ) {
			delete[] m_pLive;
			m_pLive = NULL;
		}
		m_iLive = anim.size();
		if ( !m_pLive && m_iLive ) {
			m_pLive = new float[m_iLive];
			memset( m_pLive, 0, sizeof( float ) * m_iLive );
		}

		int		i = 0;

		for ( auto it = anim.begin(); it != anim.end(); it++ ) {
			SsPart*			part = it->first;
			SsPartAnime*	anime = it->second;
			SsPartState*	state = &stat[i];

			i++;

			if ( part->type == SsPartType::instance ) {
				SsInstanceAttr&						attr = state->instanceValue;

				if ( attr.independent ) {
					attr.liveFrame -= iDelta * ( attr.speed - 1 );
				}else{
					if ( m_pLive && i < m_iLive ) {
						attr.liveFrame = m_pLive[i];
					}
					attr.liveFrame += iDelta;
				}

				if ( m_pLive && i < m_iLive ) {
					m_pLive[i] = attr.liveFrame;
				}
			}else
			if ( part->type == SsPartType::effect ) {
				SsEffectAttr&						attr = state->effectValue;

				if ( attr.independent ) {
					float	fNext = state->effectTimeTotal + iDelta * state->effectValue.speed;

					if ( fNext > state->refEffect->getEffectTimeLength() ) {
						fNext = state->refEffect->getEffectTimeLength();
						fNext -= iDelta * state->effectValue.speed;
						fNext -= 1;

						state->effectTimeTotal = fNext;
					}
				}
			}
		}
	}

	if ( m_bAnimeDecoder && m_AnimeDecoder ) {
		m_Renderer.m_iSetup = 0;
		SsCurrentRenderer::SetCurrentRender( &m_Renderer );

		m_AnimeDecoder->update( iDelta );
	}

#if defined(GD_V4) || defined(SPRITESTUDIO_GODOT_EXTENSION)
	queue_redraw();
#endif
#ifdef GD_V3
	update();
#endif
}

void GdNodeSsPlayer::drawAnimation()
{
	if ( m_bAnimeDecoder && m_AnimeDecoder ) {
		m_Renderer.m_iSetup = 0;
		SsCurrentRenderer::SetCurrentRender( &m_Renderer );

		m_AnimeDecoder->draw();

		m_Renderer.setTextureInterpolate( m_bTextureInterpolate );
		m_Renderer.draw( get_canvas_item() );
	}
}

void GdNodeSsPlayer::fetchAnimation()
{
	m_bAnimeDecoder = false;
	if ( !EMPTY(m_strAnimationSelected) ) {
		if ( m_ResPlayer.is_null() ) {
			return;
		}
		if ( m_ResAnimePack.is_null() ) {
			return;
		}

		Ref<GdResourceSsProject>	resProject = m_ResPlayer->getProjectResource();

		if ( resProject.is_null() ) {
			return;
		}

		SsProject*		pProject = resProject->getProject();

		if ( !pProject ) {
			return;
		}

		auto			c = m_strAnimationSelected.utf8();
		SsString		strAnimationName = c.ptr() ? SsString( c.ptr() ) : SsString( "" );
		SsAnimePack*	pAnimePack = m_ResAnimePack->getAnimePack();

		if ( !pAnimePack ) {
			return;
		}

		SsAnimation*	pAnimation = pAnimePack->findAnimation( strAnimationName );

		if ( !pAnimation ) {
			return;
		}

		if ( !pAnimation ) {
			ERR_PRINT( "Select Anime is Null" );
		}

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
//				m_Renderer.setTextureInterpolate( m_bTextureInterpolate );	/* Updated in drawAnimation() */
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
	}
}
