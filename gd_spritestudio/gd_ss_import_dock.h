#pragma once

#ifdef TOOLS_ENABLED

#include "gd_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/label.hpp>

#include <godot_cpp/classes/v_box_container.hpp> 
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_file_dialog.hpp>
#include <godot_cpp/classes/project_settings.hpp>
using namespace godot;
#else
#include "scene/gui/control.h"
#include "scene/gui/panel.h"
#include "scene/gui/label.h"

#include "scene/gui/box_container.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/button.h"
#include "editor/gui/editor_file_dialog.h"
#include "core/config/project_settings.h"
#endif

class GdSsImportControl : public VBoxContainer {
  GDCLASS(GdSsImportControl, VBoxContainer)

protected:
  static void _bind_methods();

public:
  GdSsImportControl();
  ~GdSsImportControl();

  void _notification(int p_what);

  void* process_file(const String &source_sspj_path, const String &dst_dir_path);

  void start_intercepting();
  void stop_intercepting();

private:
  Callable original_drop_handler;
  bool is_intercepting = false;
  bool is_reemitting = false;
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
  EditorFileDialog *file_dialog = nullptr;

  const String SETTING_KEY = "spritestudio/output_directory";
  const String DEFAULT_PATH = "res://ssab_generated";

  void _on_browse_button_pressed();
  void _on_dir_selected(const String &p_path);
  void _load_settings();
  void _save_settings();
};
#endif // #ifdef TOOLS_ENABLED
