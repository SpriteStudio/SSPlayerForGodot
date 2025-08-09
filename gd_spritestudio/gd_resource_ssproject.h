/*!
* \file		gd_resource_ssproject.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_RESOURCE_SSPROJECT_H
#define GD_RESOURCE_SSPROJECT_H

#include "gd_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/templates/vmap.hpp>
using namespace godot;
#else
#ifdef GD_V4
#include "core/io/resource.h"
#include "core/io/resource_loader.h"
#include "core/templates/vmap.h"
#endif
#ifdef GD_V3
#include "core/resource.h"
#endif
#endif

#include "SpriteStudio6-SDK/Common/Loader/ssloader_sspj.h"

#include "ss_macros.h"
#include "gd_resource_ssanimepack.h"
#include "gd_resource_sscellmap.h"
#include "gd_resource_sseffect.h"

SsSdkUsing

class GdResourceSsProject : public Resource
{
	GDCLASS( GdResourceSsProject, Resource );

public :
	GdResourceSsProject();
	virtual ~GdResourceSsProject();

	void setAnimePackResource( const Ref<GdResourceSsAnimePack>& resAnimePack, const String& strName );
	Ref<GdResourceSsAnimePack> getAnimePackResource( const String& strName ) const;

	void setCellMapResource( const Ref<GdResourceSsCellMap>& resCellMap, int iIndex );
	Ref<GdResourceSsCellMap> getCellMapResourceFromIndex( int iIndex ) const;
	Ref<GdResourceSsCellMap> getCellMapResourceFromName( const String& strName ) const;
	Ref<GdResourceSsCellMap> getCellMapResource( const SsString& strName ) const;

	void setEffectResource( const Ref<GdResourceSsEffect>& resEffect, int iIndex );
	Ref<GdResourceSsEffect> getEffectResource( int iIndex ) const;

	Error loadFromFile( const String& strPath, const String& strOrgPath = "" );
	Error saveToFile( const String& strPath, const Ref<Resource>& res );

	int getAnimePackCount() const;
	int getCellMapCount() const;
	int getEffectCount() const;

#ifdef SPRITESTUDIO_GODOT_EXTENSION
	PackedStringArray getAnimePackNames() const;
	PackedStringArray getCellMapNames() const;
	PackedStringArray getEffectNames() const;
#else
	Vector<String> getAnimePackNames() const;
	Vector<String> getCellMapNames() const;
	Vector<String> getEffectNames() const;
#endif

	SsProject* getProject() const;
	String getRoot() const;

protected :
	GdMultilevelCall static void _bind_methods();

	GdMultilevelCall bool _set( const StringName& p_name, const Variant& p_property );
	GdMultilevelCall bool _get( const StringName& p_name, Variant& r_property ) const;
	GdMultilevelCall void _get_property_list( List<PropertyInfo>* p_list ) const;

private :
	SsProject*	m_pProject;
	String		m_strRoot;
	String		m_strAnimePack;
	String		m_strCellMap;
	String		m_strImage;
	String		m_strEffect;

	VMap<String,Ref<GdResourceSsAnimePack>>	m_mapResAnimePack;
	Vector<Ref<GdResourceSsCellMap>>		m_vecResCellMap;
	Vector<Ref<GdResourceSsEffect>>			m_vecResEffect;
};

#endif // GD_RESOURCE_SSPROJECT_H
