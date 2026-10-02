#include "register_types.h"
#include "defs.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/godot.hpp>
using namespace godot;
#else
#include "core/core_bind.h"
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
      memnew(SSEditorPlugin));
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

  SsResourceLoader::get_singleton()->add_resource_format_loader(ssab_loader, false);
  SsResourceSaver::get_singleton()->add_resource_format_saver(ssab_saver, false);
  SsResourceLoader::get_singleton()->add_resource_format_loader(ssqb_loader, false);
  SsResourceSaver::get_singleton()->add_resource_format_saver(ssqb_saver, false);

  GDREGISTER_CLASS(SpriteStudioAudioBackend);
  GDREGISTER_CLASS(SpriteStudioPlayer2D);
  GDREGISTER_CLASS(SpriteStudioPartAttachment2D);
}

void unregister_ss_player_types() {
  if (ssab_loader.is_valid()) {
    SsResourceLoader::get_singleton()->remove_resource_format_loader(ssab_loader);
    ssab_loader.unref();
  }
  if (ssab_saver.is_valid()) {
    SsResourceSaver::get_singleton()->remove_resource_format_saver(ssab_saver);
    ssab_saver.unref();
  }

  if (ssqb_loader.is_valid()) {
    SsResourceLoader::get_singleton()->remove_resource_format_loader(ssqb_loader);
    ssqb_loader.unref();
  }
  if (ssqb_saver.is_valid()) {
    SsResourceSaver::get_singleton()->remove_resource_format_saver(ssqb_saver);
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

    // Internal, not GDREGISTER_CLASS: this is the editor's own UI, not scripting
    // API. When it writes the class reference (`--doctool`) the engine builds a
    // default instance of every exposed class, and the constructors here reach
    // into the editor settings and the project's files -- which crashes when no
    // editor is running. An internal class is left out of that, and out of the
    // class reference, but is still created by the plugin as before.
    GDREGISTER_INTERNAL_CLASS(SSImporter);
    GDREGISTER_INTERNAL_CLASS(SSImportControl);
    GDREGISTER_INTERNAL_CLASS(SSFileSystemContextMenu);
    GDREGISTER_INTERNAL_CLASS(SSResourceInspectorPlugin);
    GDREGISTER_INTERNAL_CLASS(SSClickableLabel);
    GDREGISTER_INTERNAL_CLASS(SSProgressDialog);
    GDREGISTER_INTERNAL_CLASS(SSCanvasDropOverlay);
    GDREGISTER_INTERNAL_CLASS(SSPlaybackPanel);

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    GDREGISTER_INTERNAL_CLASS(SSEditorPlugin);
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
