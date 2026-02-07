#include "register_types.h"
#include "defs.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>
using namespace godot;
#else
#include "core/object/class_db.h"
#endif

#ifdef TOOLS_ENABLED
#include "gd_clickable_label.h"
#include "gd_progress_dialog.h"
#include "gd_ss_editor_plugin.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/editor_plugin_registration.hpp>
#else
#include "editor/editor_node.h"

static void editor_init_callback() {
  EditorNode::get_singleton()->add_editor_plugin(
      memnew(GdSsEditorPlugin(EditorNode::get_singleton())));
}
#endif
#endif

#include "gd_ssab_resource.h"
#include "gd_ssplayer_node.h"
#include "gd_ssplayer_resource.h"
#include "gd_ssqb_resource.h"

static GdSsabResourceFormatLoader *ssab_loader = nullptr;
static GdSsabResourceFormatSaver *ssab_saver = nullptr;
static GdSsqbResourceFormatLoader *ssqb_loader = nullptr;
static GdSsqbResourceFormatSaver *ssqb_saver = nullptr;

void register_gd_spritestudio_types() {

  GDREGISTER_CLASS(GdSsabResourceFormatLoader);
  GDREGISTER_CLASS(GdSsabResourceFormatSaver);
  GDREGISTER_CLASS(GdSsqbResourceFormatLoader);
  GDREGISTER_CLASS(GdSsqbResourceFormatSaver);

#ifdef SPRITESTUDIO_GODOT_EXTENSION
  ssab_loader = memnew(GdSsabResourceFormatLoader);
  ResourceLoader::get_singleton()->add_resource_format_loader(ssab_loader);

  ssab_saver = memnew(GdSsabResourceFormatSaver);
  ResourceSaver::get_singleton()->add_resource_format_saver(ssab_saver);

  ssqb_loader = memnew(GdSsqbResourceFormatLoader);
  ResourceLoader::get_singleton()->add_resource_format_loader(ssqb_loader);

  ssqb_saver = memnew(GdSsqbResourceFormatSaver);
  ResourceSaver::get_singleton()->add_resource_format_saver(ssqb_saver);

#else
  ssab_loader = memnew(GdSsabResourceFormatLoader);
  ResourceLoader::add_resource_format_loader(ssab_loader);

  ssab_saver = memnew(GdSsabResourceFormatSaver);
  ResourceSaver::add_resource_format_saver(ssab_saver);

  ssqb_loader = memnew(GdSsqbResourceFormatLoader);
  ResourceLoader::add_resource_format_loader(ssqb_loader);

  ssqb_saver = memnew(GdSsqbResourceFormatSaver);
  ResourceSaver::add_resource_format_saver(ssqb_saver);

#endif

  GDREGISTER_CLASS(GdSsPlayerResource);
  GDREGISTER_CLASS(GdSsPlayerNode);
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

  if (ssqb_loader) {
    ResourceLoader::get_singleton()->remove_resource_format_loader(ssqb_loader);
    ssqb_loader = nullptr;
  }
  if (ssqb_saver) {
    ResourceSaver::get_singleton()->remove_resource_format_saver(ssqb_saver);
    ssqb_saver = nullptr;
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

  if (ssqb_loader) {
    ResourceLoader::remove_resource_format_loader(ssqb_loader);
    ssqb_loader = nullptr;
  }
  if (ssqb_saver) {
    ResourceSaver::remove_resource_format_saver(ssqb_saver);
    ssqb_saver = nullptr;
  }

#endif
}

void initialize_gd_spritestudio_module(ModuleInitializationLevel level) {
  if (level == MODULE_INITIALIZATION_LEVEL_SCENE) {
    register_gd_spritestudio_types();
  }

#ifdef TOOLS_ENABLED
  if (level == MODULE_INITIALIZATION_LEVEL_EDITOR) {

    GDREGISTER_CLASS(GdSsImportControl);
    GDREGISTER_CLASS(GdClickableLabel);
    GDREGISTER_CLASS(GdProgressDialog);

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    GDREGISTER_CLASS(GdSsEditorPlugin);
    EditorPlugins::add_by_type<GdSsEditorPlugin>();
#else
    EditorNode::add_init_callback(editor_init_callback);
#endif
  }
#endif
}

void uninitialize_gd_spritestudio_module(ModuleInitializationLevel level) {
  if (level == MODULE_INITIALIZATION_LEVEL_SCENE) {
    unregister_gd_spritestudio_types();
  }
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
extern "C" GDExtensionBool GDE_EXPORT spritestudio_godot_library_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization) {
  godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library,
                                                 r_initialization);
  init_obj.register_initializer(initialize_gd_spritestudio_module);
  init_obj.register_terminator(uninitialize_gd_spritestudio_module);

  init_obj.set_minimum_library_initialization_level(
      MODULE_INITIALIZATION_LEVEL_SCENE);

  return init_obj.init();
}
#endif