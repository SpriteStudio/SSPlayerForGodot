#pragma once

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

#include "gd_ssab_resource.h"

class GdSsPlayerResource : public Resource {
	GDCLASS( GdSsPlayerResource, Resource );

public :
	void setSsabResource( const Ref<GdSsabResource>& ssabRes );
	Ref<GdSsabResource> getSsabResource() const;

protected :
	static void _bind_methods();

private :
	Ref<GdSsabResource>	_ssabRes;
};

