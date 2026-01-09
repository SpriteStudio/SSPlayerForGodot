/*!
* \file		register_types.cpp
* \author	CRI Middleware Co., Ltd.
*/

#include "defs.h"
#include "register_types.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/godot.hpp>
#include <godot_cpp/core/class_db.hpp>
using namespace godot;
#else
#include "core/object/class_db.h"
#endif

void register_gd_spritestudio_types()
{
}

void unregister_gd_spritestudio_types()
{
}

#if defined(GD_V4) || defined(SPRITESTUDIO_GODOT_EXTENSION)
void initialize_gd_spritestudio_module( ModuleInitializationLevel p_level )
{
	if ( p_level != MODULE_INITIALIZATION_LEVEL_SCENE ) {
		return;
	}

	register_gd_spritestudio_types();
}

void uninitialize_gd_spritestudio_module( ModuleInitializationLevel p_level )
{
	if ( p_level != MODULE_INITIALIZATION_LEVEL_SCENE ) {
		return;
	}

	unregister_gd_spritestudio_types();
}
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
extern "C" GDExtensionBool GDE_EXPORT spritestudio_godot_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
	init_obj.register_initializer(initialize_gd_spritestudio_module);
	init_obj.register_terminator(uninitialize_gd_spritestudio_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
	return init_obj.init();
}
#endif