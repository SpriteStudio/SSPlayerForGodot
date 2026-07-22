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
#include "ss_audio_backend.h"
#include "ss_player_node_2d.h"
#include "ss_part_attachment_2d.h"
#include "ssqb_resource.h"
#include "ss_translation.h"

static Ref<SSABResourceFormatLoader> ssab_loader;
static Ref<SSABResourceFormatSaver> ssab_saver;
static Ref<SSQBResourceFormatLoader> ssqb_loader;
static Ref<SSQBResourceFormatSaver> ssqb_saver;

void register_ss_player_types() {

  GDREGISTER_CLASS(SSABResource);
  GDREGISTER_CLASS(SSABResourceFormatLoader);
  GDREGISTER_CLASS(SSABResourceFormatSaver);
  GDREGISTER_CLASS(SSQBResource);
  GDREGISTER_CLASS(SSQBResourceFormatLoader);
  GDREGISTER_CLASS(SSQBResourceFormatSaver);

  ssab_loader = memnew(SSABResourceFormatLoader);
  ssab_saver = memnew(SSABResourceFormatSaver);
  ssqb_loader = memnew(SSQBResourceFormatLoader);
  ssqb_saver = memnew(SSQBResourceFormatSaver);

#ifdef SPRITESTUDIO_GODOT_EXTENSION
  ResourceLoader::get_singleton()->add_resource_format_loader(ssab_loader);
  ResourceSaver::get_singleton()->add_resource_format_saver(ssab_saver);
  ResourceLoader::get_singleton()->add_resource_format_loader(ssqb_loader);
  ResourceSaver::get_singleton()->add_resource_format_saver(ssqb_saver);
#else
  ResourceLoader::add_resource_format_loader(ssab_loader);
  ResourceSaver::add_resource_format_saver(ssab_saver);
  ResourceLoader::add_resource_format_loader(ssqb_loader);
  ResourceSaver::add_resource_format_saver(ssqb_saver);
#endif

  GDREGISTER_CLASS(SpriteStudioAudioBackend);
  GDREGISTER_CLASS(SpriteStudioPlayer2D);
  GDREGISTER_CLASS(SpriteStudioPartAttachment2D);
}

void unregister_ss_player_types() {
  if (ssab_loader.is_valid()) {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    ResourceLoader::get_singleton()->remove_resource_format_loader(ssab_loader);
#else
    ResourceLoader::remove_resource_format_loader(ssab_loader);
#endif
    ssab_loader.unref();
  }
  if (ssab_saver.is_valid()) {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    ResourceSaver::get_singleton()->remove_resource_format_saver(ssab_saver);
#else
    ResourceSaver::remove_resource_format_saver(ssab_saver);
#endif
    ssab_saver.unref();
  }

  if (ssqb_loader.is_valid()) {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    ResourceLoader::get_singleton()->remove_resource_format_loader(ssqb_loader);
#else
    ResourceLoader::remove_resource_format_loader(ssqb_loader);
#endif
    ssqb_loader.unref();
  }
  if (ssqb_saver.is_valid()) {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    ResourceSaver::get_singleton()->remove_resource_format_saver(ssqb_saver);
#else
    ResourceSaver::remove_resource_format_saver(ssqb_saver);
#endif
    ssqb_saver.unref();
  }
}

void initialize_ss_player_module(ModuleInitializationLevel level) {
  if (level == MODULE_INITIALIZATION_LEVEL_SCENE) {
    register_ss_player_types();
    register_ss_translations();
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
    unregister_ss_translations();
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
