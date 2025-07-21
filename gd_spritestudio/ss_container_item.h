/*!
* \file		ss_container_item.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef SS_CONTAINER_ITEM_H
#define SS_CONTAINER_ITEM_H

#include "gd_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/templates/vector.hpp>
using namespace godot;
#else
#ifdef GD_V4
#include "core/string/ustring.h"
#endif
#ifdef GD_V3
#include "core/ustring.h"
#endif
#endif

#include "SpriteStudio6-SDK/Common/Loader/ssloader_sspj.h"

#include "ss_macros.h"

SsSdkUsing

template<typename T, typename U>
class SsContainerItem
{
public :
	SsContainerItem()
	{
		setAutoDelete( true );
		setRef( 0 );
		setKey( T() );
		setData( NULL );
	}

	virtual ~SsContainerItem()
	{
		if ( m_bAutoDelete && m_pData ) {
			delete m_pData;
			m_pData = NULL;
		}
	}

	void setAutoDelete( bool bAutoDelete )
	{
		m_bAutoDelete = bAutoDelete;
	}

	void incRef()
	{
		m_iRef++;
	}

	void decRef()
	{
		m_iRef--;
	}

	void setRef( int iRef )
	{
		m_iRef = iRef;
	}

	int getRef() const
	{
		return	m_iRef;
	}

	void setKey( T key )
	{
		m_Key = key;
	}

	T getKey() const
	{
		return	m_Key;
	}

	void setData( U* pData )
	{
		m_pData = pData;
	}

	U* getData() const
	{
		return	m_pData;
	}

private :
	bool	m_bAutoDelete;
	int		m_iRef;
	T		m_Key;
	U*		m_pData;
};

typedef SsContainerItem<String,SsProject>		SsContainerItemProject;
typedef SsContainerItem<String,SsAnimePack>		SsContainerItemAnimePack;
typedef SsContainerItem<String,SsCellMap>		SsContainerItemCellMap;
typedef SsContainerItem<String,SsEffectFile>	SsContainerItemEffect;

#endif // SS_CONTAINER_ITEM_H
