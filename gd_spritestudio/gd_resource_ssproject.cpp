/*!
* \file		gd_resource_ssproject.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_resource_ssproject.h"

#include "ss_macros.h"
#include "ss_container.h"
#include "gd_notifier.h"

SsSdkUsing

GdResourceSsProject::GdResourceSsProject()
{
	m_pProject = NULL;
	m_strRoot = "";

	for ( int i = 0; i < m_mapResAnimePack.size(); i++ ) {
		m_mapResAnimePack.erase( m_mapResAnimePack.getk( i ) );
	}
	m_vecResCellMap.clear();
	m_vecResEffect.clear();
}

GdResourceSsProject::~GdResourceSsProject()
{
	if ( m_pProject ) {
		SsContainer::getInstance().unloadProject( m_pProject );
		m_pProject = NULL;
	}

	for ( int i = 0; i < m_mapResAnimePack.size(); i++ ) {
		m_mapResAnimePack.erase( m_mapResAnimePack.getk( i ) );
	}
	m_vecResCellMap.clear();
	m_vecResEffect.clear();
}

void GdResourceSsProject::setAnimePackResource( const Ref<GdResourceSsAnimePack>& resAnimePack, const String& strName )
{
	if ( !m_mapResAnimePack.has( strName ) ) {
		return;
	}

	m_mapResAnimePack[strName] = resAnimePack;

	NOTIFY_PROPERTY_LIST_CHANGED();

	GdNotifier::getInstance().notifyResourceProjectChanged( this );
}

Ref<GdResourceSsAnimePack> GdResourceSsProject::getAnimePackResource( const String& strName ) const
{
	if ( !m_mapResAnimePack.has( strName ) ) {
		return	NULL;
	}

	return	m_mapResAnimePack[strName];
}

void GdResourceSsProject::setCellMapResource( const Ref<GdResourceSsCellMap>& resCellMap, int iIndex )
{
	if ( ( iIndex < 0 ) || ( iIndex >= getCellMapCount() ) ) {
		return;
	}

	m_vecResCellMap.set( iIndex, resCellMap );

	NOTIFY_PROPERTY_LIST_CHANGED();

	GdNotifier::getInstance().notifyResourceProjectChanged( this );
}

Ref<GdResourceSsCellMap> GdResourceSsProject::getCellMapResourceFromIndex( int iIndex ) const
{
	if ( ( iIndex < 0 ) || ( iIndex >= getCellMapCount() ) ) {
		return	NULL;
	}

	return	m_vecResCellMap.get( iIndex );
}

Ref<GdResourceSsCellMap> GdResourceSsProject::getCellMapResourceFromName( const String& strName ) const
{
	if ( m_pProject ) {
		for ( int i = 0; i < m_pProject->cellmapNames.size(); i++ ) {
			if ( m_pProject->cellmapNames[i].c_str() == strName ) {
				return	getCellMapResourceFromIndex( i );
			}
		}
	}

	return	NULL;
}

Ref<GdResourceSsCellMap> GdResourceSsProject::getCellMapResource( const SsString& strName ) const
{
	if ( m_pProject ) {
		for ( int i = 0; i < m_pProject->cellmapNames.size(); i++ ) {
			if ( m_pProject->cellmapNames[i] == strName ) {
				return	getCellMapResourceFromIndex( i );
			}
		}
	}

	return	NULL;
}

void GdResourceSsProject::setEffectResource( const Ref<GdResourceSsEffect>& resEffect, int iIndex )
{
	if ( ( iIndex < 0 ) || ( iIndex >= getEffectCount() ) ) {
		return;
	}

	m_vecResEffect.set( iIndex, resEffect );

	NOTIFY_PROPERTY_LIST_CHANGED();

	GdNotifier::getInstance().notifyResourceProjectChanged( this );
}

Ref<GdResourceSsEffect> GdResourceSsProject::getEffectResource( int iIndex ) const
{
	if ( ( iIndex < 0 ) || ( iIndex >= getEffectCount() ) ) {
		return	NULL;
	}

	return	m_vecResEffect.get( iIndex );
}

Error GdResourceSsProject::loadFromFile( const String& strPath, const String& strOrgPath )
{
	Error	err;
	String	strRel = strOrgPath.length() == 0 ? strPath : strOrgPath;

	m_vecResCellMap.clear();
	m_vecResEffect.clear();

	m_pProject = SsContainer::getInstance().loadProjectFromFile( strPath, false );
	m_strRoot = "";

	if ( !m_pProject ) {
		ERR_PRINT( String( "SpriteStudio sspj file load failed : " ) + strPath );
		return	FAILED;
	}

	m_strRoot = strRel.get_base_dir();
	m_strRoot = m_strRoot.replace( "res://", "" );
	if ( !EMPTY(m_strRoot) && !m_strRoot.ends_with( "/" ) )
	{
		m_strRoot += "/";
	}

	m_strAnimePack = m_strRoot + String::utf8( m_pProject->settings.animeBaseDirectory.c_str() );
	if ( EMPTY(m_strAnimePack) && !m_strAnimePack.ends_with( "/" ) )
	{
		m_strAnimePack += "/";
	}
	m_strCellMap = m_strRoot + String::utf8( m_pProject->settings.cellMapBaseDirectory.c_str() );
	if ( EMPTY(m_strCellMap) && !m_strCellMap.ends_with( "/" ) )
	{
		m_strCellMap += "/";
	}
	m_strImage = m_strRoot + String::utf8( m_pProject->settings.imageBaseDirectory.c_str() );
	if ( EMPTY(m_strImage) && !m_strImage.ends_with( "/" ) )
	{
		m_strImage += "/";
	}
	m_strEffect = m_strRoot + String::utf8( m_pProject->settings.effectBaseDirectory.c_str() );
	if ( EMPTY(m_strEffect) && !m_strEffect.ends_with( "/" ) )
	{
		m_strEffect += "/";
	}

	SsContainer::getInstance().setAnimePackDir( m_strAnimePack );
	SsContainer::getInstance().setCellMapDir( m_strCellMap );
	SsContainer::getInstance().setImageDir( m_strImage );
	SsContainer::getInstance().setEffectDir( m_strEffect );

	for ( int i = 0; i < m_pProject->animepackNames.size(); i++ ) {
		String	strName = String::utf8( m_pProject->animepackNames[i].c_str() );
#ifdef SPRITESTUDIO_GODOT_EXTENSION
		Ref<GdResourceSsAnimePack>	resAnimePack = ResourceLoader::get_singleton()->load( m_strAnimePack + strName, "", ResourceLoader::CacheMode::CACHE_MODE_REUSE);
		err = OK; // TODO: improve
#else
#ifdef GD_V4
		Ref<GdResourceSsAnimePack>	resAnimePack = ResourceLoader::load( m_strAnimePack + strName, "", ResourceFormatLoader::CacheMode::CACHE_MODE_REUSE);
		err = OK; // TODO: improve
#endif
#ifdef GD_V3
		Ref<GdResourceSsAnimePack>	resAnimePack = ResourceLoader::load( m_strAnimePack + strName, "", false, &err );
#endif
#endif
		m_mapResAnimePack[strName] = resAnimePack;

		if ( err != OK ) {
			ERR_PRINT( String( "SpriteStudio sspj ref ssae file load failed : " ) + strName );
		}

		SsAnimePack*	pAnimePack = resAnimePack->getAnimePack();

		if ( !pAnimePack ) {
			continue;
		}

		m_pProject->animeList.push_back( std::move( std::unique_ptr<SsAnimePack>( pAnimePack ) ) );
	}

	for ( int i = 0; i < m_pProject->cellmapNames.size(); i++ ) {
		String	strName = String::utf8( m_pProject->cellmapNames[i].c_str() );

#ifdef SPRITESTUDIO_GODOT_EXTENSION
		Ref<GdResourceSsCellMap>	resCellMap = ResourceLoader::get_singleton()->load( m_strCellMap + strName, "", ResourceLoader::CacheMode::CACHE_MODE_REUSE);
		err = OK; // TODO: improve
#else
#ifdef GD_V4
		Ref<GdResourceSsCellMap>	resCellMap = ResourceLoader::load( m_strCellMap + strName, "", ResourceFormatLoader::CACHE_MODE_REUSE, &err );
#endif
#ifdef GD_V3
		Ref<GdResourceSsCellMap>	resCellMap = ResourceLoader::load( m_strCellMap + strName, "", false, &err );
#endif
#endif

		m_vecResCellMap.push_back( resCellMap );

		if ( err != OK ) {
			ERR_PRINT( String( "SpriteStudio sspj ref ssce file load failed : " ) + strName );
		}

		SsCellMap*		pCellMap = resCellMap->getCellMap();

		if ( !pCellMap ) {
			continue;
		}

		auto	c = strName.utf8();

		pCellMap->loadFilepath = c.ptr() ? SsString( c.ptr() ) : SsString( "" );

		m_pProject->cellmapList.push_back( std::move( std::unique_ptr<SsCellMap>( pCellMap ) ) );
	}

	for ( int i = 0; i < m_pProject->effectFileNames.size(); i++ ) {
		String	strName = String::utf8( m_pProject->effectFileNames[i].c_str() );


#ifdef SPRITESTUDIO_GODOT_EXTENSION
		Ref<GdResourceSsEffect>		resEffect = ResourceLoader::get_singleton()->load( m_strEffect + strName, "", ResourceLoader::CACHE_MODE_REUSE);
		err = OK; // TODO: improve
#else
#ifdef GD_V4
		Ref<GdResourceSsEffect>		resEffect = ResourceLoader::load( m_strEffect + strName, "", ResourceFormatLoader::CACHE_MODE_REUSE, &err );
#endif
#ifdef GD_V3
		Ref<GdResourceSsEffect>		resEffect = ResourceLoader::load( m_strEffect + strName, "", false, &err );
#endif
#endif

		m_vecResEffect.push_back( resEffect );

		if ( err != OK ) {
			ERR_PRINT( String( "SpriteStudio sspj ref ssee file load failed : " ) + strName );
		}

		SsEffectFile*	pEffect = resEffect->getEffect();

		if ( !pEffect ) {
			continue;
		}

		ssloader_ssee::loadPostProcessing( pEffect, m_pProject );

		m_pProject->effectfileList.push_back( std::move( std::unique_ptr<SsEffectFile>( pEffect ) ) );
	}

	SsContainer::getInstance().setAnimePackDir( "" );
	SsContainer::getInstance().setCellMapDir( "" );
	SsContainer::getInstance().setImageDir( "" );
	SsContainer::getInstance().setEffectDir( "" );

	return	OK;
}

Error GdResourceSsProject::saveToFile( const String& strPath, const Ref<Resource>& res )
{
	return	FAILED;
}

int GdResourceSsProject::getAnimePackCount() const
{
	if ( m_pProject ) {
		return	m_pProject->getAnimePackNum();
	}

	return	0;
}

int GdResourceSsProject::getCellMapCount() const
{
	if ( m_pProject ) {
		return	m_pProject->getCellMapNum();
	}

	return	0;
}

int GdResourceSsProject::getEffectCount() const
{
	if ( m_pProject ) {
		return	m_pProject->getEffectFileNum();
	}

	return	0;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray GdResourceSsProject::getAnimePackNames() const
#else
Vector<String> GdResourceSsProject::getAnimePackNames() const
#endif
{
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    PackedStringArray vec;
#else
	Vector<String>	vec;
#endif

	if ( m_pProject ) {
		for ( int i = 0; i < m_pProject->getAnimePackNum(); i++ ) {
			SsString	strPath = m_pProject->getAnimePackFilePath( i, false );

			vec.push_back( Variant( String::utf8( strPath.c_str() ) ) );
		}
	}

	return	vec;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray GdResourceSsProject::getCellMapNames() const
#else
Vector<String> GdResourceSsProject::getCellMapNames() const
#endif
{
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    PackedStringArray vec;
#else
	Vector<String>	vec;
#endif

	if ( m_pProject ) {
		for ( int i = 0; i < m_pProject->getCellMapNum(); i++ ) {
			SsString	strPath = m_pProject->getCelMapFileOriginalPath( i );

			vec.push_back( Variant( String::utf8( strPath.c_str() ) ) );
		}
	}

	return	vec;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray GdResourceSsProject::getEffectNames() const
#else
Vector<String> GdResourceSsProject::getEffectNames() const
#endif
{
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    PackedStringArray vec;
#else
	Vector<String>	vec;
#endif

	if ( m_pProject ) {
		for ( int i = 0; i < m_pProject->getEffectFileNum(); i++ ) {
			SsString	strPath = m_pProject->getEffectFilePath( i );

			vec.push_back( Variant( String::utf8( strPath.c_str() ) ) );
		}
	}

	return	vec;
}

SsProject* GdResourceSsProject::getProject() const
{
	return	m_pProject;
}

String GdResourceSsProject::getRoot() const
{
	return	m_strRoot;
}

void GdResourceSsProject::_bind_methods()
{
	ClassDB::bind_method( D_METHOD( "get_anime_pack_count" ), &GdResourceSsProject::getAnimePackCount );
	ClassDB::bind_method( D_METHOD( "get_cell_map_count" ), &GdResourceSsProject::getCellMapCount );
	ClassDB::bind_method( D_METHOD( "get_effect_count" ), &GdResourceSsProject::getEffectCount );

	ClassDB::bind_method( D_METHOD( "get_anime_pack_names" ), &GdResourceSsProject::getAnimePackNames );
	ClassDB::bind_method( D_METHOD( "get_cell_map_names" ), &GdResourceSsProject::getCellMapNames );
	ClassDB::bind_method( D_METHOD( "get_effect_names" ), &GdResourceSsProject::getEffectNames );

	ClassDB::bind_method( D_METHOD( "set_anime_pack_resource", "res_anime_pack", "name" ), &GdResourceSsProject::setAnimePackResource );
	ClassDB::bind_method( D_METHOD( "get_anime_pack_resource", "name" ), &GdResourceSsProject::getAnimePackResource );

	ClassDB::bind_method( D_METHOD( "set_cell_map_resource", "res_cell_map", "index" ), &GdResourceSsProject::setCellMapResource );
	ClassDB::bind_method( D_METHOD( "get_cell_map_resource_from_index", "index" ), &GdResourceSsProject::getCellMapResourceFromIndex );
	ClassDB::bind_method( D_METHOD( "get_cell_map_resource_from_name", "name" ), &GdResourceSsProject::getCellMapResourceFromName );

	ClassDB::bind_method( D_METHOD( "set_effect_resource", "res_effect", "index" ), &GdResourceSsProject::setEffectResource );
	ClassDB::bind_method( D_METHOD( "get_effect_resource", "index" ), &GdResourceSsProject::getEffectResource );

	ADD_GROUP( GdUiText( "Cellmap Settings" ), "" );
//	ADD_GROUP( GdUiText( "effect_settings" ), "" );
}

bool GdResourceSsProject::_set( const StringName& p_name, const Variant& p_property )
{
	if ( !m_pProject ) {
		return	false;
	}

	for ( int i = 0; i < getCellMapCount(); i++ ) {
		auto	e = m_vecResCellMap[i];
		String	strName = String::utf8( m_pProject->cellmapNames[i].c_str() );

		if ( p_name == strName ) {
			const Ref<GdResourceSsCellMap>&		resCellMap = p_property;

			if ( resCellMap.is_null() ) {
				continue;
			}

			SsCellMap*	pCellMap = const_cast<SsCellMap*>( resCellMap->getCellMap() );

			if ( !pCellMap ) {
				continue;
			}

			auto	c = strName.utf8();

			pCellMap->loadFilepath = c.ptr() ? SsString( c.ptr() ) : SsString( "" );

			setCellMapResource( p_property, i );

			return	true;
		}
	}
/*
	for ( int i = 0; i < getEffectCount(); i++ ) {
		auto	e = m_vecResEffect[i];
		String	strName = String::utf8( m_pProject->effectFileNames[i].c_str() );

		if ( p_name == strName ) {
			setEffectResource( p_property, i );

			return	true;
		}
	}
*/
	return	false;
}

bool GdResourceSsProject::_get( const StringName& p_name, Variant& r_property ) const
{
	if ( !m_pProject ) {
		return	false;
	}

	for ( int i = 0; i < getCellMapCount(); i++ ) {
		auto	e = m_vecResCellMap[i];
		String	strName = String::utf8( m_pProject->cellmapNames[i].c_str() );

		if ( p_name == strName ) {
			r_property = getCellMapResourceFromIndex( i );

			return	true;
		}
	}
/*
	for ( int i = 0; i < getEffectCount(); i++ ) {
		auto	e = m_vecResEffect[i];
		String	strName = String::utf8( m_pProject->effectFileNames[i].c_str() );

		if ( p_name == strName ) {
			r_property = getEffectResource( i );

			return	true;
		}
	}
*/
	return	false;
}

void GdResourceSsProject::_get_property_list( List<PropertyInfo>* p_list ) const
{
	if ( !m_pProject ) {
		return;
	}

	PropertyInfo	propertyInfo;

	for ( int i = 0; i < getCellMapCount(); i++ ) {
		auto	e = m_vecResCellMap[i];
		String	strName = String::utf8( m_pProject->cellmapNames[i].c_str() );

		propertyInfo.name = strName;
		propertyInfo.type = Variant::OBJECT;
		propertyInfo.hint_string = "GdResourceSsCellMap";
		propertyInfo.hint = PROPERTY_HINT_RESOURCE_TYPE;

		p_list->push_back( propertyInfo );
	}
/*
	for ( int i = 0; i < getEffectCount(); i++ ) {
		auto	e = m_vecResEffect[i];
		String	strName = String::utf8( m_pProject->effectFileNames[i].c_str() );

		propertyInfo.name = strName;
		propertyInfo.type = Variant::OBJECT;
		propertyInfo.hint_string = "GdResourceSsEffect";
		propertyInfo.hint = PROPERTY_HINT_RESOURCE_TYPE;

		p_list->push_back( propertyInfo );
	}
*/
}
