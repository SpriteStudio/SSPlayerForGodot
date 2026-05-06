#pragma once

#ifdef TOOLS_ENABLED

#include "ss_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/editor_inspector_plugin.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string.hpp>
using namespace godot;
#else
#include "core/string/ustring.h"
#include "core/templates/vector.h"
#include "editor/inspector/editor_inspector.h"
#endif

class SSImporter;
class SSFileSystemContextMenu;

class SSResourceInspectorPlugin : public EditorInspectorPlugin {
  GDCLASS(SSResourceInspectorPlugin, EditorInspectorPlugin)

protected:
  static void _bind_methods();

public:
  SSResourceInspectorPlugin();

  void set_importer(SSImporter *p_importer) { importer = p_importer; }
  void set_context_menu(SSFileSystemContextMenu *p_menu) { context_menu = p_menu; }

#ifdef SPRITESTUDIO_GODOT_EXTENSION
  virtual bool _can_handle(Object *p_object) const override;
  virtual void _parse_begin(Object *p_object) override;
#else
  virtual bool can_handle(Object *p_object) override;
  virtual void parse_begin(Object *p_object) override;
#endif

  void _on_open_pressed(const String &p_resource_path);
  void _on_reconvert_pressed(const String &p_resource_path);
  void _on_reveal_pressed(const String &p_resource_path);

private:
  SSImporter *importer = nullptr;
  SSFileSystemContextMenu *context_menu = nullptr;

  bool _is_unsupported_for_editor() const;
  void _add_action_buttons(const String &p_path);
};

#endif // #ifdef TOOLS_ENABLED
