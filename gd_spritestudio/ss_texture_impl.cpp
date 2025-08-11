/*!
* \file		ss_texture_impl.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "ss_texture_impl.h"

#include "gd_macros.h"

SsTextureImpl::SsTextureImpl()
{
}

SsTextureImpl::~SsTextureImpl()
{
}

int	SsTextureImpl::getWidth()
{
	if ( m_Texture.is_valid() ) {
		return	m_Texture->get_width();
	}

	return	0;
}

int	SsTextureImpl::getHeight()
{
	if ( m_Texture.is_valid() ) {
		return	m_Texture->get_height();
	}

	return	0;
}

bool SsTextureImpl::Load( const char* filename )
{
	Error	err;


#ifdef SPRITESTUDIO_GODOT_EXTENSION
	m_Texture = ResourceLoader::get_singleton()->load( filename, "", ResourceLoader::CACHE_MODE_REUSE);
	err = OK; // TODO: improve
#else
#ifdef GD_V4
	m_Texture = ResourceLoader::load( filename, "", ResourceFormatLoader::CacheMode::CACHE_MODE_REUSE);
	err = OK; // TODO: improve
#endif
#ifdef GD_V3
	m_Texture = ResourceLoader::load( filename, "", false, &err );
#endif
#endif

	if ( err != OK ) {
		ERR_PRINT( "SS Texture Load error : " + String( filename ) );
		return	false;
	}

	return	true;
}

ISSTexture* SsTextureImpl::create()
{
	return	new SsTextureImpl();
}

void SsTextureImpl::setTexture( Ref<Texture> texture )
{
	m_Texture = texture;
}

Ref<Texture> SsTextureImpl::getTexture() const
{
	return	m_Texture;
}
