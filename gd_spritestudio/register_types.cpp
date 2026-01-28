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

#ifdef TOOLS_ENABLED
#include "gd_clickable_label.h"
#include "gd_ss_editor_plugin.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/editor_plugin_registration.hpp>
using namespace godot;
#else
#include "editor/editor_node.h"

static void editor_init_callback() {
	EditorNode::get_singleton()->add_editor_plugin(memnew(GdSsEditorPlugin(EditorNode::get_singleton())));
}
#endif
#endif


#include "gd_resource_ssab.h"
static GdSsabResourceFormatLoader *ssab_loader = nullptr;
static GdSsabResourceFormatSaver *ssab_saver = nullptr;

void register_gd_spritestudio_types() {
#ifdef TOOLS_ENABLED
    GDREGISTER_CLASS(GdSsImportControl);
    GDREGISTER_CLASS(GdClickableLabel);
#endif

#ifndef SPRITESTUDIO_GODOT_EXTENSION
    GDREGISTER_CLASS(GdSsabResourceFormatLoader);
    GDREGISTER_CLASS(GdSsabResourceFormatSaver);
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    ssab_loader = memnew(GdSsabResourceFormatLoader);
    ResourceLoader::get_singleton()->add_resource_format_loader(ssab_loader);

    ssab_saver = memnew(GdSsabResourceFormatSaver);
    ResourceSaver::get_singleton()->add_resource_format_saver(ssab_saver);
#else
    ssab_loader = memnew(GdSsabResourceFormatLoader);
    ResourceLoader::add_resource_format_loader(ssab_loader);

    ssab_saver = memnew(GdSsabResourceFormatSaver);
    ResourceSaver::add_resource_format_saver(ssab_saver);
#endif

}

void unregister_gd_spritestudio_types() {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    if (ssab_loader) {
        ResourceLoader::get_singleton()->remove_resource_format_loader(ssab_loader);
        ssab_loader = nullptr;
    }
    if (ssab_saver) {
        ResourceSaver::get_singleton()->remove_resource_format_saver(ssab_saver);
        ssab_saver = nullptr;
    }
#else
    if (ssab_loader) {
        ResourceLoader::remove_resource_format_loader(ssab_loader);
        ssab_loader = nullptr;
    }
    if (ssab_saver) {
        ResourceSaver::remove_resource_format_saver(ssab_saver);
        ssab_saver = nullptr;
    }
#endif
}

void initialize_gd_spritestudio_module(ModuleInitializationLevel level) {
#ifdef TOOLS_ENABLED		
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    if (level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
        GDREGISTER_CLASS(GdSsEditorPlugin);
        EditorPlugins::add_plugin_class(StringName("SpriteStudioEditorPlugin"));
		return;
    }
#else
    if (level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
        EditorNode::add_init_callback(editor_init_callback);
        return;
    }    
#endif
#endif
    if (level == MODULE_INITIALIZATION_LEVEL_CORE) {
        GDREGISTER_CLASS(GdSsabResourceFormatLoader);
        GDREGISTER_CLASS(GdSsabResourceFormatSaver);
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
