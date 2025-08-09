/*!
* \file		gd_resource_sscellmap.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_RESOURCE_SSCELLMAP_H
#define GD_RESOURCE_SSCELLMAP_H

#include "gd_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/texture.hpp>
using namespace godot;
#else
#ifdef GD_V4
#include "core/io/resource.h"
#endif
#ifdef GD_V3
#include "core/resource.h"
#endif
#include "scene/resources/texture.h"
#endif

#include "SpriteStudio6-SDK/Common/Loader/ssloader_ssce.h"

#include "ss_macros.h"

SsSdkUsing

class GdResourceSsCellMap : public Resource
{
	GDCLASS( GdResourceSsCellMap, Resource );

public :
	GdResourceSsCellMap();
	virtual ~GdResourceSsCellMap();

	void setTexture( Ref<Texture> texture );
	Ref<Texture> getTexture() const;

	Error loadFromFile( const String& strPath, const String& strOrgPath = "" );
	Error saveToFile( const String& strPath, const Ref<Resource>& res );

	SsCellMap* getCellMap() const;
	String getRoot() const;

protected :
	GdMultilevelCall static void _bind_methods();

private :
	SsCellMap*		m_pCellMap;

	Ref<Texture>	m_Texture;
};

#endif // GD_RESOURCE_SSCELLMAP_H
