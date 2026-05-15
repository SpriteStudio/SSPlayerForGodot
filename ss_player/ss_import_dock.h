#pragma once

#ifdef TOOLS_ENABLED

#include "ss_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_file_dialog.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/line_edit.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/popup_menu.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scroll_container.hpp>
#include <godot_cpp/classes/style_box_flat.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
using namespace godot;
#else
#include "core/config/project_settings.h"
#include "core/input/input_event.h"
#include "editor/gui/editor_file_dialog.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/panel.h"
#include "scene/gui/popup_menu.h"
#include "scene/gui/scroll_container.h"
#include "scene/resources/style_box_flat.h"
#endif

class SSImporter;

class SSImportControl : public VBoxContainer {
  GDCLASS(SSImportControl, VBoxContainer)

protected:
  static void _bind_methods();

public:
  SSImportControl();
  ~SSImportControl();

  void _notification(int p_what);

  void set_importer(SSImporter *p_importer) { importer = p_importer; }

  void start_intercepting();
  void stop_intercepting();

private:
  enum RecentMenuId {
    RECENT_MENU_OPEN_IN_EDITOR,
    RECENT_MENU_REVEAL,
    RECENT_MENU_RECONVERT,
    RECENT_MENU_REMOVE,
  };

  SSImporter *importer = nullptr;

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
  Panel *drop_panel = nullptr;

  LineEdit *path_line_edit = nullptr;
  Button *browse_button = nullptr;
  Button *reset_button = nullptr;
  EditorFileDialog *file_dialog = nullptr;

  VBoxContainer *recent_vbox = nullptr;
  Label *recent_label = nullptr;
  Label *recent_empty_label = nullptr;
  Button *clear_recent_button = nullptr;

  PopupMenu *recent_popup = nullptr;
  String pending_recent_path;

  const String SETTING_KEY = "spritestudio/output_directory";
  const String RECENT_FILES_KEY = "spritestudio/recent_sspj_files";
  const String DEFAULT_PATH = "res://ssab_generated";
  static const int RECENT_FILES_CAP = 20;

  void _on_line_edit_submitted(const String &p_path);
  void _on_browse_button_pressed();
  void _on_reset_button_pressed();
  void _on_dir_selected(const String &p_path);
  void _on_recent_file_pressed(const String &p_path);
  void _on_recent_gui_input(const Ref<InputEvent> &p_event, const String &p_path);
  void _show_recent_context_menu(const String &p_path);
  void _on_recent_menu_id_pressed(int p_id);
  void _on_clear_recent_pressed();
  void _remove_from_recent_files(const String &p_path);
  void _update_recent_files_ui();
  void _add_to_recent_files(const String &p_path);
  void _reconvert_sspj(const String &p_sspj_path);
#ifdef SPRITESTUDIO_GODOT_EXTENSION
  void _start_import(const PackedStringArray &p_sspj_files);
#else
  void _start_import(const Vector<String> &p_sspj_files);
#endif
  void _load_settings();
  void _save_settings();
  void _ensure_output_dir_exists();
};
#endif // #ifdef TOOLS_ENABLED
