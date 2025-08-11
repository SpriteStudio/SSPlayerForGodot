/*!
* \file		gd_resource_sscellmap.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_resource_sscellmap.h"

#include "ss_macros.h"
#include "ss_container.h"
#include "gd_notifier.h"

SsSdkUsing

GdResourceSsCellMap::GdResourceSsCellMap()
{
	m_pCellMap = NULL;
}

GdResourceSsCellMap::~GdResourceSsCellMap()
{
	if ( m_pCellMap ) {
		SsContainer::getInstance().unloadCellMap( m_pCellMap );
		m_pCellMap = NULL;
	}
}

void GdResourceSsCellMap::setTexture( Ref<Texture> texture )
{
	m_Texture = texture;

	GdNotifier::getInstance().notifyResourceCellMapChanged( this );
}

Ref<Texture> GdResourceSsCellMap::getTexture() const
{
	return	m_Texture;
}

Error GdResourceSsCellMap::loadFromFile( const String& strPath, const String& strOrgPath )
{
	Error	err;
	String	strRel = strOrgPath.length() == 0 ? strPath : strOrgPath;
		
	m_pCellMap = SsContainer::getInstance().loadCellMapFromFile( strPath, false );

	if ( !m_pCellMap ) {
		ERR_PRINT( String( "SpriteStudio ssce file load failed : " ) + strPath );
		return FAILED;
	}

	{
		auto			c = SsContainer::getInstance().getImageDir().utf8();
		SsString		strRoot = c.ptr() ? SsString( c.ptr() ) : SsString( "" );

		m_pCellMap->imagePath = strRoot + m_pCellMap->imagePath;
	}

	auto	c = strRel.utf8();

	m_pCellMap->fname = c.ptr() ? SsString( c.ptr() ) : SsString( "" );

	String	str = String::utf8( m_pCellMap->imagePath.c_str() );

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    m_Texture = ResourceLoader::get_singleton()->load( str, "", ResourceLoader::CACHE_MODE_REUSE);
	err = OK; // TODO: improve
#else
#ifdef GD_V4
	m_Texture = ResourceLoader::load( str, "", ResourceFormatLoader::CACHE_MODE_REUSE, &err );
#endif
#ifdef GD_V3
	m_Texture = ResourceLoader::load( str, "", false, &err );
#endif
#endif

	return	err;
}

Error GdResourceSsCellMap::saveToFile( const String& strPath, const Ref<Resource>& res )
{
	return	FAILED;
}

SsCellMap* GdResourceSsCellMap::getCellMap() const
{
	return	m_pCellMap;
}

void GdResourceSsCellMap::_bind_methods()
{
	ClassDB::bind_method( D_METHOD( "set_texture", "texture" ), &GdResourceSsCellMap::setTexture );
	ClassDB::bind_method( D_METHOD( "get_texture" ), &GdResourceSsCellMap::getTexture );

	ADD_PROPERTY(
		PropertyInfo(
			Variant::OBJECT,
			"texture",
			PropertyHint::PROPERTY_HINT_RESOURCE_TYPE,
			"Texture"
		),
		"set_texture",
		"get_texture"
	);
}
