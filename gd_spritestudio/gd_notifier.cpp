/*!
* \file		gd_notifier.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_notifier.h"

GdNotifier::GdNotifier()
{
	m_vecItem.clear();
}

GdNotifier::~GdNotifier()
{
	m_vecItem.clear();
}

GdNotifier& GdNotifier::getInstance()
{
	static GdNotifier	m_Self;

	return	m_Self;
}

void GdNotifier::addItem( GdNotifierItem* pItem )
{
	if ( !pItem ) {
		return;
	}

	if ( m_vecItem.find( pItem ) >= 0 ) {
		return;
	}

	m_vecItem.push_back( pItem );
}

void GdNotifier::removeItem( GdNotifierItem* pItem )
{
	int		iIndex = m_vecItem.find( pItem );

	if ( iIndex < 0 ) {
		return;
	}

#if defined(GD_V4) || defined(SPRITESTUDIO_GODOT_EXTENSION)
	m_vecItem.remove_at( iIndex );
#endif
#ifdef GD_V3
	m_vecItem.remove( iIndex );
#endif
}

void GdNotifier::notifyResourcePlayerChanged( const Ref<GdResourceSsPlayer>& resPlayer )
{
	for ( int i = 0; i < m_vecItem.size(); i++ ) {
		auto	e = m_vecItem[i];

		e->resourcePlayerChanged( resPlayer );
	}
}

void GdNotifier::notifyResourceProjectChanged( const Ref<GdResourceSsProject>& resProject )
{
	for ( int i = 0; i < m_vecItem.size(); i++ ) {
		auto	e = m_vecItem[i];

		e->resourceProjectChanged( resProject );
	}
}

void GdNotifier::notifyResourceAnimePackChanged( const Ref<GdResourceSsAnimePack>& resAnimePack )
{
	for ( int i = 0; i < m_vecItem.size(); i++ ) {
		auto	e = m_vecItem[i];

		e->resourceAnimePackChanged( resAnimePack );
	}
}

void GdNotifier::notifyResourceCellMapChanged( const Ref<GdResourceSsCellMap>& resCellMap )
{
	for ( int i = 0; i < m_vecItem.size(); i++ ) {
		auto	e = m_vecItem[i];

		e->resourceCellMapChanged( resCellMap );
	}
}
