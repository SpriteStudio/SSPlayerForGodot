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

#include "gd_resource_ssab.h"
static GdResourceSsabResourceFormatLoader *ssab_loader;
static GdResourceSsabResourceFormatSaver *ssab_saver;

void register_gd_spritestudio_types() {

#ifndef SPRITESTUDIO_GODOT_EXTENSION
    ClassDB::register_class<GdResourceSsabResourceFormatLoader>();
    ClassDB::register_class<GdResourceSsabResourceFormatSaver>();
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    ssab_loader = memnew(GdResourceSsabResourceFormatLoader);
    ResourceLoader::get_singleton()->add_resource_format_loader(ssab_loader);

    ssab_saver = memnew(GdResourceSsabResourceFormatSaver);
    ResourceSaver::get_singleton()->add_resource_format_saver(ssab_saver);
#else
    ssab_loader = memnew(GdResourceSsabResourceFormatLoader);
    ResourceLoader::add_resource_format_loader(ssab_loader);

    ssab_saver = memnew(GdResourceSsabResourceFormatSaver);
    ResourceSaver::add_resource_format_saver(ssab_saver);
#endif

}

void unregister_gd_spritestudio_types() {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    ResourceLoader::get_singleton()->remove_resource_format_loader(ssab_loader);
    ResourceSaver::get_singleton()->remove_resource_format_saver(ssab_saver);
#else
    ResourceLoader::remove_resource_format_loader(ssab_loader);
    ResourceSaver::remove_resource_format_saver(ssab_saver);
#endif
}

void initialize_gd_spritestudio_module(ModuleInitializationLevel level) {
	if (level == MODULE_INITIALIZATION_LEVEL_CORE) {
        ClassDB::register_class<GdResourceSsabResourceFormatLoader>();
        ClassDB::register_class<GdResourceSsabResourceFormatSaver>();
	    return;        
    }
    if (level != MODULE_INITIALIZATION_LEVEL_SCENE) return;
	register_gd_spritestudio_types();
}

void uninitialize_gd_spritestudio_module(ModuleInitializationLevel level) {
	unregister_gd_spritestudio_types();

	if (level != MODULE_INITIALIZATION_LEVEL_CORE) return;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
extern "C" GDExtensionBool GDE_EXPORT spritestudio_godot_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
	init_obj.register_initializer(initialize_gd_spritestudio_module);
	init_obj.register_terminator(uninitialize_gd_spritestudio_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
	return init_obj.init();
}
#endif
