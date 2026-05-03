#pragma once

#ifdef TOOLS_ENABLED

#include "ss_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>

#include <godot_cpp/classes/v_box_container.hpp> 
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_file_dialog.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
using namespace godot;
#else
#include "scene/gui/control.h"
#include "scene/gui/panel.h"
#include "scene/gui/label.h"
#include "scene/resources/style_box_flat.h"

#include "scene/gui/box_container.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/button.h"
#include "editor/gui/editor_file_dialog.h"
#include "core/config/project_settings.h"
#include "scene/gui/scroll_container.h"
#endif

class SSImportControl : public VBoxContainer {
  GDCLASS(SSImportControl, VBoxContainer)

protected:
  static void _bind_methods();

public:
  SSImportControl();
  ~SSImportControl();

  void _notification(int p_what);

  void* process_file(const String &source_sspj_path, const String &dst_dir_path);

  void start_intercepting();
  void stop_intercepting();

private:
  Callable original_drop_handler;
  bool is_intercepting = false;
  bool is_reemitting = false;

  Vector<void*> import_contexts;
  Vector<String> import_dst_dirs;
  class SSProgressDialog* import_dialog = nullptr;
  Vector<bool> import_finished_contexts;
  int import_prev_num = 0;
  bool is_importing = false;

#ifdef SPRITESTUDIO_GODOT_EXTENSION
  void _on_window_files_dropped(const PackedStringArray &p_files);
  void _perform_default_drop_logic(const PackedStringArray &p_files);
#else
  void _on_window_files_dropped(const Vector<String> &p_files);
  void _perform_default_drop_logic(const Vector<String> &p_files);
#endif

  Label *instruction_label = nullptr;
  Panel *background_panel = nullptr;

  LineEdit *path_line_edit = nullptr;
  Button *browse_button = nullptr;
  Button *reset_button = nullptr;
  EditorFileDialog *file_dialog = nullptr;

  VBoxContainer *recent_vbox = nullptr;
  Label *recent_label = nullptr;

  const String SETTING_KEY = "spritestudio/output_directory";
  const String RECENT_FILES_KEY = "spritestudio/recent_sspj_files";
  const String DEFAULT_PATH = "res://ssab_generated";

  void _on_line_edit_submitted(const String& p_path);
  void _on_browse_button_pressed();
  void _on_reset_button_pressed();
  void _on_dir_selected(const String &p_path);
  void _on_recent_file_pressed(const String &p_path);
  void _on_clear_history_pressed();
  void _update_recent_files_ui();
  void _add_to_recent_files(const String &p_path);
#ifdef SPRITESTUDIO_GODOT_EXTENSION
  void _start_import(const PackedStringArray &p_sspj_files);
#else
  void _start_import(const Vector<String> &p_sspj_files);
#endif
  void _load_settings();
  void _save_settings();
};
#endif // #ifdef TOOLS_ENABLED
