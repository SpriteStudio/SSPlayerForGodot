/*!
* \file		gd_resource_sseffect.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_RESOURCE_SSEFFECT_H
#define GD_RESOURCE_SSEFFECT_H

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

#include "SpriteStudio6-SDK/Common/Loader/ssloader_ssee.h"

#include "ss_macros.h"

SsSdkUsing

class GdResourceSsEffect : public Resource
{
	GDCLASS( GdResourceSsEffect, Resource );

public :
	GdResourceSsEffect();
	virtual ~GdResourceSsEffect();

	Error loadFromFile( const String& strPath, const String& strOrgPath = "" );
	Error saveToFile( const String& strPath, const Ref<Resource>& res );

	SsEffectFile* getEffect() const;

protected :
	GdMultilevelCall static void _bind_methods();

private :
	SsEffectFile*	m_pEffect;
};

#endif // GD_RESOURCE_SSEFFECT_H
