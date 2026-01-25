#pragma once

#include "gd_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/panel.hpp>
#include <godot_cpp/classes/label.hpp>
using namespace godot;
#else
#include "scene/gui/control.h"
#include "scene/gui/panel.h"
#include "scene/gui/label.h"
#endif

class GdSsImportControl : public Control {
  GDCLASS(GdSsImportControl, Control)

protected:
  static void _bind_methods();

public:
  GdSsImportControl();
  ~GdSsImportControl();

void _notification(int p_what);

  void process_file(const String &source_path);

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

  void *Ctx;
  Label *instruction_label = nullptr;
  Panel *background_panel = nullptr;
};
