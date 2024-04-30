/*!
* \file		gd_notifier.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_NOTIFIER_H
#define GD_NOTIFIER_H

#include "gd_notifier_item.h"

class GdNotifier
{
private :
	GdNotifier();

public :
	virtual ~GdNotifier();

	static GdNotifier& getInstance();

	void addItem( GdNotifierItem* pItem );
	void removeItem( GdNotifierItem* pItem );

	void notifyResourcePlayerChanged( const Ref<GdResourceSsPlayer>& resPlayer );
	void notifyResourceProjectChanged( const Ref<GdResourceSsProject>& resProject );
	void notifyResourceAnimePackChanged( const Ref<GdResourceSsAnimePack>& resAnimePack );
	void notifyResourceCellMapChanged( const Ref<GdResourceSsCellMap>& resCellMap );

private :
	Vector<GdNotifierItem*>		m_vecItem;
};

#endif // GD_NOTIFIER_H
