/*!
* \file		gd_renderer.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_renderer.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/rendering_server.hpp>
#define	VisualServer	RenderingServer
using namespace godot;
#else
#ifdef GD_V4
#include "servers/rendering_server.h"
#define	VisualServer	RenderingServer
#endif
#ifdef GD_V3
#include "servers/visual_server.h"
#endif
#endif

static inline void safeFree( RID& rid )
{
	if ( rid.is_valid() ) {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
		VisualServer::get_singleton()->free_rid( rid );
#else
		VisualServer::get_singleton()->free( rid );
#endif
		rid = RID();
	}
}

GdRenderer::GdRenderer()
{
	m_ViewPortId = RID();
	m_TextureId = RID();
	m_CanvasId = RID();
	m_CanvasItemId = RID();

	m_Size = Rect2();
}

GdRenderer::~GdRenderer()
{
	term();
}

void GdRenderer::init()
{
	VisualServer*	pVisualServer = VisualServer::get_singleton();

	term();

	m_ViewPortId = pVisualServer->viewport_create();
	m_CanvasId = pVisualServer->canvas_create();
	m_CanvasItemId = pVisualServer->canvas_item_create();

	setSize( 16, 16 );

	pVisualServer->canvas_item_set_parent( m_CanvasItemId, m_CanvasId );

	pVisualServer->viewport_set_transparent_background( m_ViewPortId, true );

	pVisualServer->viewport_attach_canvas( m_ViewPortId, m_CanvasId );

#ifdef SPRITESTUDIO_GODOT_EXTENSION
	RID			viewPortTextureId = pVisualServer->viewport_get_texture( m_ViewPortId );
	Ref<Image>	viewPortTextureData = pVisualServer->texture_2d_get( viewPortTextureId );

	m_TextureId = pVisualServer->texture_2d_create( viewPortTextureData );

//	pVisualServer->texture_set_proxy( m_TextureId, viewPortTextureId );
	m_TextureId = viewPortTextureId;
#else
#ifdef GD_V4
	RID			viewPortTextureId = pVisualServer->viewport_get_texture( m_ViewPortId );
	Ref<Image>	viewPortTextureData = pVisualServer->texture_2d_get( viewPortTextureId );

	m_TextureId = pVisualServer->texture_2d_create( viewPortTextureData );

//	pVisualServer->texture_set_proxy( m_TextureId, viewPortTextureId );
	m_TextureId = viewPortTextureId;
#endif
#ifdef GD_V3
	RID			viewPortTextureId = pVisualServer->viewport_get_texture( m_ViewPortId );
	Ref<Image>	viewPortTextureData = pVisualServer->texture_get_data( viewPortTextureId );

	m_TextureId = pVisualServer->texture_create_from_image( viewPortTextureData );

	pVisualServer->texture_set_proxy( m_TextureId, viewPortTextureId );
#endif
#endif

	pVisualServer->viewport_set_active( m_ViewPortId, true );
}

void GdRenderer::term()
{
	safeFree( m_ViewPortId );
	safeFree( m_TextureId );
	safeFree( m_CanvasId );
	safeFree( m_CanvasItemId );
}

void GdRenderer::setTranslate( float fX, float fY )
{
	VisualServer*	pVisualServer = VisualServer::get_singleton();
	Transform2D		trans;

#if defined(GD_V4) || defined(SPRITESTUDIO_GODOT_EXTENSION)
	trans.translate_local( fX, fY );
#endif
#ifdef GD_V3
	trans.translate( fX, fY );
#endif

	pVisualServer->viewport_set_canvas_transform( m_ViewPortId, m_CanvasId, trans );
}

void GdRenderer::setSize( int iWidth, int iHeight )
{
	VisualServer*	pVisualServer = VisualServer::get_singleton();

	m_Size = Rect2( 0, 0, iWidth, iHeight );

	pVisualServer->viewport_set_size( m_ViewPortId, iWidth, iHeight );
}

Rect2 GdRenderer::getSize() const
{
	return	m_Size;
}

RID GdRenderer::getTextureRid() const
{
	return	m_TextureId;
}

RID GdRenderer::getCanvasRid() const
{
	return	m_CanvasId;
}

RID GdRenderer::getCanvasItemRid() const
{
	return	m_CanvasItemId;
}
