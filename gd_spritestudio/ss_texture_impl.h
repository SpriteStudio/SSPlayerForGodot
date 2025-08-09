/*!
* \file		ss_texture_impl.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef SS_TEXTURE_IMPL_H
#define SS_TEXTURE_IMPL_H

#include "gd_macros.h"

#include "SpriteStudio6-SDK/Common/Helper/IsshTexture.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/texture.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_format_loader.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/image_texture.hpp>
using namespace godot;
#else
#include "scene/resources/texture.h"
#ifdef GD_V4
#include "core/io/resource_loader.h"
#endif
#endif

#include "ss_macros.h"

SsSdkUsing

class SsTextureImpl : public ISSTexture
{
public :
	SsTextureImpl();
	virtual ~SsTextureImpl();

	virtual int	getWidth() override;
	virtual int	getHeight() override;

	virtual bool Load( const char* filename ) override;
	virtual ISSTexture* create() override;

	void setTexture( Ref<Texture> texture );
	Ref<Texture> getTexture() const;

private :

#ifdef SPRITESTUDIO_GODOT_EXTENSION
	Ref<Texture2D>	m_Texture;
#else
#ifdef GD_V4
	Ref<Texture2D>	m_Texture;
#endif
#ifdef GD_V3
	Ref<Texture>	m_Texture;
#endif
#endif
};

#endif // SS_TEXTURE_IMPL_H
