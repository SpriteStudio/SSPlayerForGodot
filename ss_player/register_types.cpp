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
#include "ss_canvas_drop_overlay.h"
#include "ss_clickable_label.h"
#include "ss_filesystem_menu.h"
#include "ss_progress_dialog.h"
#include "ss_importer.h"
#include "ss_playback_panel.h"
#include "ss_resource_inspector.h"
#include "ss_editor_plugin.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/editor_plugin_registration.hpp>
#else
#include "editor/editor_node.h"

static void editor_init_callback() {
  EditorNode::get_singleton()->add_editor_plugin(
      memnew(SSEditorPlugin(EditorNode::get_singleton())));
}
#endif
#endif

#include "ssab_resource.h"
#include "ss_player_node_2d.h"
#include "ss_part_attachment_2d.h"
#include "ssqb_resource.h"

static SSABResourceFormatLoader *ssab_loader = nullptr;
static SSABResourceFormatSaver *ssab_saver = nullptr;
static SSQBResourceFormatLoader *ssqb_loader = nullptr;
static SSQBResourceFormatSaver *ssqb_saver = nullptr;

void register_ss_player_types() {

  GDREGISTER_CLASS(SSABResource);
  GDREGISTER_CLASS(SSABResourceFormatLoader);
  GDREGISTER_CLASS(SSABResourceFormatSaver);
  GDREGISTER_CLASS(SSQBResource);
  GDREGISTER_CLASS(SSQBResourceFormatLoader);
  GDREGISTER_CLASS(SSQBResourceFormatSaver);

#ifdef SPRITESTUDIO_GODOT_EXTENSION
  ssab_loader = memnew(SSABResourceFormatLoader);
  ResourceLoader::get_singleton()->add_resource_format_loader(ssab_loader);

  ssab_saver = memnew(SSABResourceFormatSaver);
  ResourceSaver::get_singleton()->add_resource_format_saver(ssab_saver);

  ssqb_loader = memnew(SSQBResourceFormatLoader);
  ResourceLoader::get_singleton()->add_resource_format_loader(ssqb_loader);

  ssqb_saver = memnew(SSQBResourceFormatSaver);
  ResourceSaver::get_singleton()->add_resource_format_saver(ssqb_saver);

#else
  ssab_loader = memnew(SSABResourceFormatLoader);
  ResourceLoader::add_resource_format_loader(ssab_loader);

  ssab_saver = memnew(SSABResourceFormatSaver);
  ResourceSaver::add_resource_format_saver(ssab_saver);

  ssqb_loader = memnew(SSQBResourceFormatLoader);
  ResourceLoader::add_resource_format_loader(ssqb_loader);

  ssqb_saver = memnew(SSQBResourceFormatSaver);
  ResourceSaver::add_resource_format_saver(ssqb_saver);

#endif

  GDREGISTER_CLASS(SpriteStudioPlayer2D);
  GDREGISTER_CLASS(SpriteStudioPartAttachment2D);
}

void unregister_ss_player_types() {
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

void initialize_ss_player_module(ModuleInitializationLevel level) {
  if (level == MODULE_INITIALIZATION_LEVEL_SCENE) {
    register_ss_player_types();
  }

#ifdef TOOLS_ENABLED
  if (level == MODULE_INITIALIZATION_LEVEL_EDITOR) {

    GDREGISTER_CLASS(SSImporter);
    GDREGISTER_CLASS(SSImportControl);
    GDREGISTER_CLASS(SSFileSystemContextMenu);
    GDREGISTER_CLASS(SSResourceInspectorPlugin);
    GDREGISTER_CLASS(SSClickableLabel);
    GDREGISTER_CLASS(SSProgressDialog);
    GDREGISTER_CLASS(SSCanvasDropOverlay);
    GDREGISTER_CLASS(SSPlaybackPanel);

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    GDREGISTER_CLASS(SSEditorPlugin);
    EditorPlugins::add_by_type<SSEditorPlugin>();
#else
    EditorNode::add_init_callback(editor_init_callback);
#endif
  }
#endif
}

void uninitialize_ss_player_module(ModuleInitializationLevel level) {
  if (level == MODULE_INITIALIZATION_LEVEL_SCENE) {
    unregister_ss_player_types();
  }
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
extern "C" GDExtensionBool GDE_EXPORT ss_player_library_init(
    GDExtensionInterfaceGetProcAddress p_get_proc_address,
    GDExtensionClassLibraryPtr p_library,
    GDExtensionInitialization *r_initialization) {
  godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library,
                                                 r_initialization);
  init_obj.register_initializer(initialize_ss_player_module);
  init_obj.register_terminator(uninitialize_ss_player_module);

  init_obj.set_minimum_library_initialization_level(
      MODULE_INITIALIZATION_LEVEL_SCENE);

  return init_obj.init();
}
#endif
