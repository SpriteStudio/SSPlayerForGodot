/*!
* \file		ss_texture_impl.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef SS_TEXTURE_IMPL_H
#define SS_TEXTURE_IMPL_H

#include "gd_macros.h"

#include "SpriteStudio6-SDK/Common/Helper/IsshTexture.h"

#include "scene/resources/texture.h"

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
#ifdef GD_V4
	Ref<Texture2D>	m_Texture;
#endif
#ifdef GD_V3
	Ref<Texture>	m_Texture;
#endif
};

#endif // SS_TEXTURE_IMPL_H
