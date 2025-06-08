/*!
* \file		ss_container.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "ss_container.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/variant/variant.hpp>
using namespace godot;
#else
#ifdef GD_V4
#include "core/variant/variant.h"
#endif
#ifdef GD_V3
#include "core/variant.h"
#endif
#endif

#include "gd_io.h"
#include "ss_io.h"

SsContainer::SsContainer()
{
	m_vecItemProject.clear();
	m_vecItemAnimePack.clear();
	m_vecItemCellMap.clear();
	m_vecItemEffect.clear();
}

SsContainer::~SsContainer()
{
	for ( int i = 0; i < m_vecItemProject.size(); i++ ) {
		auto	e = m_vecItemProject[i];

		delete e;
	}

	for ( int i = 0; i < m_vecItemAnimePack.size(); i++ ) {
		auto	e = m_vecItemAnimePack[i];

		delete e;
	}

	for ( int i = 0; i < m_vecItemCellMap.size(); i++ ) {
		auto	e = m_vecItemCellMap[i];

		delete e;
	}

	for ( int i = 0; i < m_vecItemEffect.size(); i++ ) {
		auto	e = m_vecItemEffect[i];

		delete e;
	}

	m_vecItemProject.clear();
	m_vecItemAnimePack.clear();
	m_vecItemCellMap.clear();
	m_vecItemEffect.clear();
}

SsContainer& SsContainer::getInstance()
{
	static SsContainer	m_Self;

	return	m_Self;
}

SsProject* SsContainer::loadProjectFromFile( const String& strPath, bool bAutoDelete )
{
	for ( int i = 0; i < m_vecItemProject.size(); i++ ) {
		auto	e = m_vecItemProject[i];

		if ( e->getKey() == strPath ) {
			e->incRef();

			return	e->getData();
		}
	}

	SsProject*	pProject = NULL;
	String		strExt = strPath.get_extension();

	if ( strExt == "sspj" ) {
		String		strXml = GdIO::loadStringFromFile( strPath );
		auto		c = strXml.utf8();

		if ( c.length() > 0 ) {
			pProject = ssloader_sspj::Parse_ProjectOnly( c, c.length() );
		}
	}else
	if ( strExt == "gdsspj" ) {
		Variant		val = GdIO::loadVariantFromFile( strPath );

		pProject = SsIO::createProjectFromVariant( val );
	}

	if ( !pProject ) {
		return	NULL;
	}

	SsContainerItemProject*		pItem = new SsContainerItemProject();

	pItem->setAutoDelete( bAutoDelete );
	pItem->setKey( strPath );
	pItem->setData( pProject );
	pItem->incRef();

	m_vecItemProject.push_back( pItem );

	return	pProject;
}

SsAnimePack* SsContainer::loadAnimePackFromFile( const String& strPath, bool bAutoDelete )
{
	for ( int i = 0; i < m_vecItemAnimePack.size(); i++ ) {
		auto	e = m_vecItemAnimePack[i];

		if ( e->getKey() == strPath ) {
			e->incRef();

			return	e->getData();
		}
	}

	SsAnimePack*	pAnimePack = NULL;
	String			strExt = strPath.get_extension();

	if ( strExt == "ssae" ) {
		String			strXml = GdIO::loadStringFromFile( strPath );
		auto			c = strXml.utf8();

		if ( c.length() > 0 ) {
			pAnimePack = ssloader_ssae::Parse( c, c.length() );
		}
	}else
	if ( strExt == "gdssae" ) {
		Variant		val = GdIO::loadVariantFromFile( strPath );

		pAnimePack = SsIO::createAnimePackFromVariant( val );
	}

	if ( !pAnimePack ) {
		return	NULL;
	}

	SsContainerItemAnimePack*	pItem = new SsContainerItemAnimePack();

	pItem->setAutoDelete( bAutoDelete );
	pItem->setKey( strPath );
	pItem->setData( pAnimePack );
	pItem->incRef();

	m_vecItemAnimePack.push_back( pItem );

	return	pAnimePack;
}

SsCellMap* SsContainer::loadCellMapFromFile( const String& strPath, bool bAutoDelete )
{
	for ( int i = 0; i < m_vecItemCellMap.size(); i++ ) {
		auto	e = m_vecItemCellMap[i];

		if ( e->getKey() == strPath ) {
			e->incRef();

			return	e->getData();
		}
	}

	SsCellMap*		pCellMap = NULL;
	String			strExt = strPath.get_extension();

	if ( strExt == "ssce" ) {
		String			strXml = GdIO::loadStringFromFile( strPath );
		auto			c = strXml.utf8();

		if ( c.length() > 0 ) {
			pCellMap = ssloader_ssce::Parse( c, c.length() );
		}
	}else
	if ( strExt == "gdssce" ) {
		Variant		val = GdIO::loadVariantFromFile( strPath );

		pCellMap = SsIO::createCellMapFromVariant( val );
	}

	if ( !pCellMap ) {
		return	NULL;
	}

	SsContainerItemCellMap*		pItem = new SsContainerItemCellMap();

	pItem->setAutoDelete( bAutoDelete );
	pItem->setKey( strPath );
	pItem->setData( pCellMap );
	pItem->incRef();

	m_vecItemCellMap.push_back( pItem );

	return	pCellMap;
}

SsEffectFile* SsContainer::loadEffectFromFile( const String& strPath, bool bAutoDelete )
{
	for ( int i = 0; i < m_vecItemEffect.size(); i++ ) {
		auto	e = m_vecItemEffect[i];

		if ( e->getKey() == strPath ) {
			e->incRef();

			return	e->getData();
		}
	}

	SsEffectFile*	pEffect = NULL;
	String			strExt = strPath.get_extension();

	if ( strExt == "ssee" ) {
		String			strXml = GdIO::loadStringFromFile( strPath );
		auto			c = strXml.utf8();

		pEffect = NULL;//ssloader_ssee::Parse( c, c.length() );

		if ( c.length() > 0 ) {
			libXML::XMLDocument xml;

			libXML::XMLError xmlerr = xml.Parse(c, c.length());

			if ( libXML::XML_SUCCESS == xmlerr)
			{
				SsXmlIArchiver ar( xml.GetDocument() , "SpriteStudioEffect" );

				pEffect = new SsEffectFile();
				pEffect->__Serialize( &ar );
			}
		}
	}else
	if ( strExt == "gdssee" ) {
		Variant		val = GdIO::loadVariantFromFile( strPath );

		pEffect = SsIO::createEffectFromVariant( val );
	}

	if ( !pEffect ) {
		return	NULL;
	}

	SsContainerItemEffect*		pItem = new SsContainerItemEffect();

	pItem->setAutoDelete( bAutoDelete );
	pItem->setKey( strPath );
	pItem->setData( pEffect );
	pItem->incRef();

	m_vecItemEffect.push_back( pItem );

	return	pEffect;
}

void SsContainer::unloadProject( SsProject* pProject )
{
	for ( int i = 0; i < m_vecItemProject.size(); i++ ) {
		auto	e = m_vecItemProject[i];

		if ( e->getData() == pProject ) {
			e->decRef();

			if ( e->getRef() <= 0 ) {
				delete e;

#if defined(GD_V4) || defined(SPRITESTUDIO_GODOT_EXTENSION)
				m_vecItemProject.remove_at( i );
#endif
#ifdef GD_V3
				m_vecItemProject.remove( i );
#endif
			}

			return;
		}
	}
}

void SsContainer::unloadAnimePack( SsAnimePack* pAnimePack )
{
	for ( int i = 0; i < m_vecItemAnimePack.size(); i++ ) {
		auto	e = m_vecItemAnimePack[i];

		if ( e->getData() == pAnimePack ) {
			e->decRef();

			if ( e->getRef() <= 0 ) {
				delete e;

#if defined(GD_V4) || defined(SPRITESTUDIO_GODOT_EXTENSION)
				m_vecItemAnimePack.remove_at( i );
#endif
#ifdef GD_V3
				m_vecItemAnimePack.remove( i );
#endif
			}

			return;
		}
	}
}

void SsContainer::unloadCellMap( SsCellMap* pCellMap )
{
	for ( int i = 0; i < m_vecItemCellMap.size(); i++ ) {
		auto	e = m_vecItemCellMap[i];

		if ( e->getData() == pCellMap ) {
			e->decRef();

			if ( e->getRef() <= 0 ) {
				delete e;

#if defined(GD_V4) || defined(SPRITESTUDIO_GODOT_EXTENSION)
				m_vecItemCellMap.remove_at( i );
#endif
#ifdef GD_V3
				m_vecItemCellMap.remove( i );
#endif
			}

			return;
		}
	}
}

void SsContainer::unloadEffect( SsEffectFile* pEffect )
{
	for ( int i = 0; i < m_vecItemEffect.size(); i++ ) {
		auto	e = m_vecItemEffect[i];

		if ( e->getData() == pEffect ) {
			e->decRef();

			if ( e->getRef() <= 0 ) {
				delete e;

#if defined(GD_V4) || defined(SPRITESTUDIO_GODOT_EXTENSION)
				m_vecItemEffect.remove_at( i );
#endif
#ifdef GD_V3
				m_vecItemEffect.remove( i );
#endif
			}

			return;
		}
	}
}

void SsContainer::setAnimePackDir( const String& strPath )
{
	m_strAnimePack = strPath;
}

void SsContainer::setCellMapDir( const String& strPath )
{
	m_strCellMap = strPath;
}

void SsContainer::setImageDir( const String& strPath )
{
	m_strImage = strPath;
}

void SsContainer::setEffectDir( const String& strPath )
{
	m_strEffect = strPath;
}

const String& SsContainer::getAnimePackDir() const
{
	return	m_strAnimePack;
}

const String& SsContainer::getCellMapDir() const
{
	return	m_strCellMap;
}

const String& SsContainer::getImageDir() const
{
	return	m_strImage;
}

const String& SsContainer::getEffectDir() const
{
	return	m_strEffect;
}
