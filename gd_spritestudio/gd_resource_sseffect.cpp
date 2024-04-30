/*!
* \file		gd_resource_sseffect.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_resource_sseffect.h"

#include "ss_macros.h"
#include "ss_container.h"

SsSdkUsing

GdResourceSsEffect::GdResourceSsEffect()
{
	m_pEffect = NULL;
}

GdResourceSsEffect::~GdResourceSsEffect()
{
	if ( m_pEffect ) {
		SsContainer::getInstance().unloadEffect( m_pEffect );
		m_pEffect = NULL;
	}
}

Error GdResourceSsEffect::loadFromFile( const String& strPath, const String& strOrgPath )
{
	m_pEffect = SsContainer::getInstance().loadEffectFromFile( strPath, false );

	if ( !m_pEffect ) {
		ERR_PRINT( String( "SpriteStudio ssee file load failed : " ) + strPath );
		return	FAILED;
	}

	return	OK;
}

Error GdResourceSsEffect::saveToFile( const String& strPath, const Ref<Resource>& res )
{
	return	FAILED;
}

SsEffectFile* GdResourceSsEffect::getEffect() const
{
	return	m_pEffect;
}

void GdResourceSsEffect::_bind_methods()
{
}
