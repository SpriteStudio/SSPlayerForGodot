/*!
* \file		gd_resource_ssanimepack.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_resource_ssanimepack.h"

#include "ss_macros.h"
#include "ss_container.h"
#include "gd_notifier.h"

SsSdkUsing

GdResourceSsAnimePack::GdResourceSsAnimePack()
{
	m_pAnimePack = NULL;
}

GdResourceSsAnimePack::~GdResourceSsAnimePack()
{
	if ( m_pAnimePack ) {
		SsContainer::getInstance().unloadAnimePack( m_pAnimePack );
		m_pAnimePack = NULL;
	}
}

Error GdResourceSsAnimePack::loadFromFile( const String& strPath, const String& strOrgPath )
{
	m_pAnimePack = SsContainer::getInstance().loadAnimePackFromFile( strPath, false );

	if ( !m_pAnimePack ) {
		ERR_PRINT( String( "SpriteStudio ssae file load failed : " ) + strPath );
		return	FAILED;
	}

	return	OK;
}

Error GdResourceSsAnimePack::saveToFile( const String& strPath, const Ref<Resource>& res )
{
	return	FAILED;
}

int GdResourceSsAnimePack::getAnimationCount() const
{
	if ( m_pAnimePack ) {
		return	m_pAnimePack->animeList.size();
	}

	return	0;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray GdResourceSsAnimePack::getAnimationNames() const
#else
Vector<String> GdResourceSsAnimePack::getAnimationNames() const
#endif
{
#ifdef SPRITESTUDIO_GODOT_EXTENSION
	PackedStringArray	vec;
#else
	Vector<String>	vec;
#endif

	if ( m_pAnimePack ) {
		for ( int i = 0; i < m_pAnimePack->animeList.size(); i++ ) {
			SsAnimation*	pAnimation = m_pAnimePack->animeList[i];

			vec.push_back( Variant( String::utf8( pAnimation->name.c_str() ) ) );
		}
	}

	return	vec;
}

SsAnimePack* GdResourceSsAnimePack::getAnimePack() const
{
	return	m_pAnimePack;
}

SsModel* GdResourceSsAnimePack::getModel() const
{
	if ( m_pAnimePack ) {
		return	&m_pAnimePack->Model;
	}

	return	NULL;
}

SsAnimation* GdResourceSsAnimePack::findAnimation( String strName ) const
{
	return	NULL;
}

void GdResourceSsAnimePack::_bind_methods()
{
	ClassDB::bind_method( D_METHOD( "get_animation_count" ), &GdResourceSsAnimePack::getAnimationCount );

	ClassDB::bind_method( D_METHOD( "get_animation_names" ), &GdResourceSsAnimePack::getAnimationNames );
}
