/*!
* \file		gd_node_ssplayer.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_NODE_SSPLAYER_H
#define GD_NODE_SSPLAYER_H


#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/node2d.hpp>
using namespace godot;
#else
#include "scene/2d/node_2d.h"
#endif


#include "SpriteStudio6-SDK/Common/Animator/ssplayer_animedecode.h"

#include "gd_macros.h"
#include "ss_macros.h"
#include "gd_resource_ssplayer.h"
#include "gd_resource_ssanimepack.h"
#include "ss_renderer_impl.h"
#include "gd_notifier_item.h"

SsSdkUsing

class GdNodeSsPlayer : public Node2D, public GdNotifierItem
{
	GDCLASS( GdNodeSsPlayer, Node2D )

public :
	GdNodeSsPlayer();
	virtual ~GdNodeSsPlayer();

	virtual void resourcePlayerChanged( const Ref<GdResourceSsPlayer>& resPlayer ) override;
	virtual void resourceProjectChanged( const Ref<GdResourceSsProject>& resProject ) override;
	virtual void resourceAnimePackChanged( const Ref<GdResourceSsAnimePack>& resAnimePack ) override;
	virtual void resourceCellMapChanged( const Ref<GdResourceSsCellMap>& resCellMap ) override;

	void setPlayerResource( const Ref<GdResourceSsPlayer>& resPlayer );
	Ref<GdResourceSsPlayer> getPlayerResource() const;

	void setAnimePack( const String& strName );
	String getAnimePack() const;
	void setAnimation( const String& strName );
	String getAnimation() const;
	void setFrame( int iFrame );
	int getFrame() const;

	int getStartFrame() const;
	int getEndFrame() const;
	int getFps() const;

	void setTextureInterpolate( bool bSwitch );
	bool getTextureInterpolate() const;

	void setLoop( bool bLoop );
	bool getLoop() const;
	void setPlay( bool bPlay );
	bool getPlay() const;
	void play();
	void pause( bool b );
	void stop();

protected :
	GdMultilevelCall static void _bind_methods();

	GdMultilevelCall bool _set( const StringName& p_name, const Variant& p_property );
	GdMultilevelCall bool _get( const StringName& p_name, Variant& r_property ) const;
	GdMultilevelCall void _get_property_list( List<PropertyInfo>* p_list ) const;
	GdMultilevelCall void _notification( int p_notification );

private :
	void postAnimePackChanged( const String& strName );
	void postAnimationChanged( const String& strName );
	void postAnimationFinished( const String& strName );
	void postFrameChanged( int iFrame );
	void postUserData( int iFlag, int iIntValue, const Rect2& rectValue, const Vector2& pointValue, const String& strStringValue );
	void postSignal( const String& strName, const Dictionary& dicParam );

	void updateAnimation( float delta );
	void drawAnimation();

	void fetchAnimation();

	Ref<GdResourceSsPlayer>				m_ResPlayer;
	Ref<GdResourceSsAnimePack>			m_ResAnimePack;

	SsRendererImpl						m_Renderer;

	bool								m_bAnimeDecoder;
	std::unique_ptr<SsAnimeDecoder>		m_AnimeDecoder;
	std::unique_ptr<SsCellMapList>		m_CellMapList;

	String								m_strAnimePackSelected;
	String								m_strAnimationSelected;
	int									m_iFrame;
	float								m_fDelta;
	int									m_iLive;
	float*								m_pLive;

	bool								m_bLoop;
	bool								m_bPlay;
	bool								m_bPause;

	bool								m_bTextureInterpolate;
};

#endif // GD_NODE_SSPLAYER_H
