#pragma once

#ifdef TOOLS_ENABLED

#include "ss_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/editor_context_menu_plugin.hpp>
#include <godot_cpp/classes/editor_file_dialog.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
using namespace godot;
#else
#include "core/string/ustring.h"
#include "core/templates/vector.h"
#include "editor/gui/editor_file_dialog.h"
#include "editor/inspector/editor_context_menu_plugin.h"
#endif

class SSImporter;

class SSFileSystemContextMenu : public EditorContextMenuPlugin {
  GDCLASS(SSFileSystemContextMenu, EditorContextMenuPlugin)

protected:
  static void _bind_methods();

public:
  SSFileSystemContextMenu();
  ~SSFileSystemContextMenu();

  void set_importer(SSImporter *p_importer) { importer = p_importer; }

#ifdef SPRITESTUDIO_GODOT_EXTENSION
  virtual void _popup_menu(const PackedStringArray &p_paths) override;
#else
  virtual void get_options(const Vector<String> &p_paths) override;
#endif

  void _on_open_in_editor(const PackedStringArray &p_paths);
  void _on_convert(const PackedStringArray &p_paths);

private:
  enum PendingAction {
    ACTION_NONE,
    ACTION_OPEN_IN_EDITOR,
    ACTION_CONVERT,
  };

  SSImporter *importer = nullptr;
  EditorFileDialog *file_dialog = nullptr;
  PendingAction pending_action = ACTION_NONE;
  String pending_ssab_path;
  PackedStringArray pending_missing_ssabs;
  PackedStringArray pending_valid_sspjs;
  PackedStringArray pending_valid_dst_dirs;

  bool _is_unsupported_for_editor() const;
  void _ensure_file_dialog();
  void _ask_user_for_sspj(const String &p_ssab_path, PendingAction p_action);
  void _on_sspj_file_selected(const String &p_sspj_path);
  void _do_open_in_editor(const String &p_sspj_path);
  void _do_convert(const String &p_ssab_path, const String &p_sspj_path);
};

#endif // #ifdef TOOLS_ENABLED
