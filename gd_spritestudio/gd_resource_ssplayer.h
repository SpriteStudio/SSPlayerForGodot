/*!
* \file		gd_resource_ssplayer.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_RESOURCE_SSPLAYER_H
#define GD_RESOURCE_SSPLAYER_H

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

#include "gd_resource_ssproject.h"

class GdResourceSsPlayer : public Resource
{
	GDCLASS( GdResourceSsPlayer, Resource );

public :
	GdResourceSsPlayer();
	virtual ~GdResourceSsPlayer();

	void setProjectResource( const Ref<GdResourceSsProject>& resProject );
	Ref<GdResourceSsProject> getProjectResource() const;

protected :
	GdMultilevelCall static void _bind_methods();

private :
	Ref<GdResourceSsProject>	m_ResProject;
};


#endif // GD_RESOURCE_SSPLAYER_H
