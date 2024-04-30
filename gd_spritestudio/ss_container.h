/*!
* \file		ss_container.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef SS_CONTAINER_H
#define SS_CONTAINER_H

#include "ss_container_item.h"

class SsContainer
{
private :
	SsContainer();

public :
	virtual ~SsContainer();

	static SsContainer& getInstance();

	SsProject* loadProjectFromFile( const String& strPath, bool bAutoDelete = true );
	SsAnimePack* loadAnimePackFromFile( const String& strPath, bool bAutoDelete = true );
	SsCellMap* loadCellMapFromFile( const String& strPath, bool bAutoDelete = true );
	SsEffectFile* loadEffectFromFile( const String& strPath, bool bAutoDelete = true );

	void unloadProject( SsProject* pProject );
	void unloadAnimePack( SsAnimePack* pAnimePack );
	void unloadCellMap( SsCellMap* pCellMap );
	void unloadEffect( SsEffectFile* pEffect );

	void setAnimePackDir( const String& strPath );
	void setCellMapDir( const String& strPath );
	void setImageDir( const String& strPath );
	void setEffectDir( const String& strPath );

	const String& getAnimePackDir() const;
	const String& getCellMapDir() const;
	const String& getImageDir() const;
	const String& getEffectDir() const;

private :
	Vector<SsContainerItemProject*>		m_vecItemProject;
	Vector<SsContainerItemAnimePack*>	m_vecItemAnimePack;
	Vector<SsContainerItemCellMap*>		m_vecItemCellMap;
	Vector<SsContainerItemEffect*>		m_vecItemEffect;

	String		m_strAnimePack;
	String		m_strCellMap;
	String		m_strImage;
	String		m_strEffect;
};

#endif // SS_CONTAINER_H
