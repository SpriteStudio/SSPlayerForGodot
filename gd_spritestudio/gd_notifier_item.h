/*!
* \file		gd_notifier_item.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_NOTIFIER_ITEM_H
#define GD_NOTIFIER_ITEM_H

#include "gd_resource_ssplayer.h"
#include "gd_resource_ssproject.h"
#include "gd_resource_ssanimepack.h"
#include "gd_resource_sscellmap.h"

class GdNotifierItem
{
public :
	GdNotifierItem();
	virtual ~GdNotifierItem();

	virtual void resourcePlayerChanged( const Ref<GdResourceSsPlayer>& resPlayer );
	virtual void resourceProjectChanged( const Ref<GdResourceSsProject>& resProject );
	virtual void resourceAnimePackChanged( const Ref<GdResourceSsAnimePack>& resAnimePack );
	virtual void resourceCellMapChanged( const Ref<GdResourceSsCellMap>& resCellMap );
};

#endif // GD_NOTIFIER_ITEM_H
