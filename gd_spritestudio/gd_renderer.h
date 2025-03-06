/*!
* \file		gd_renderer.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_RENDERER_H
#define GD_RENDERER_H

#include "gd_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/rid.hpp>
using namespace godot;
#else
#ifdef GD_V4
#include "core/templates/rid.h"
#endif
#ifdef GD_V3
#include "core/rid.h"
#endif
#include "core/math/rect2.h"
#endif

class GdRenderer
{
public :
	GdRenderer();
	virtual ~GdRenderer();

	void init();
	void term();

	void setTranslate( float fX, float fY );
	void setSize( int iWidth, int iHeight );

	Rect2 getSize() const;

	RID getTextureRid() const;
	RID getCanvasRid() const;
	RID getCanvasItemRid() const;

private :
	RID				m_ViewPortId;
	RID				m_TextureId;
	RID				m_CanvasId;
	RID				m_CanvasItemId;

	Rect2			m_Size;
};

#endif // GD_RENDERER_H
