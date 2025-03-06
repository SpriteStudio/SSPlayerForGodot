/*!
* \file		gd_resource_ssanimepack.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_RESOURCE_SSANIMEPACK_H
#define GD_RESOURCE_SSANIMEPACK_H

#include "gd_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/resource.hpp>
using namespace godot;
#else
#ifdef GD_V4
#include "core/io/resource.h"
#endif
#ifdef GD_V3
#include "core/resource.h"
#endif
#endif

#include "SpriteStudio6-SDK/Common/Loader/ssloader_ssae.h"

#include "ss_macros.h"
#include "gd_resource_sscellmap.h"

SsSdkUsing

class GdResourceSsAnimePack : public Resource
{
	GDCLASS( GdResourceSsAnimePack, Resource );

public :
	GdResourceSsAnimePack();
	virtual ~GdResourceSsAnimePack();

	void setCellMapResource( const Ref<GdResourceSsCellMap>& resCellMap, int iIndex );
	Ref<GdResourceSsCellMap> getCellMapResource( int iIndex ) const;

	Error loadFromFile( const String& strPath, const String& strOrgPath = "" );
	Error saveToFile( const String& strPath, const Ref<Resource>& res );

	int getAnimationCount() const;

#ifdef SPRITESTUDIO_GODOT_EXTENSION
	PackedStringArray getAnimationNames() const;
#else
	Vector<String> getAnimationNames() const;
#endif

	SsAnimePack* getAnimePack() const;

	SsModel* getModel() const;

	SsAnimation* findAnimation( String strName ) const;

protected :
	GdMultilevelCall static void _bind_methods();

private :
	SsAnimePack*	m_pAnimePack;
};

#endif // GD_RESOURCE_SSANIMEPACK_H
