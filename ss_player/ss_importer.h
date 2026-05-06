#pragma once

#ifdef TOOLS_ENABLED

#include "ss_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
using namespace godot;
#else
#include "core/string/ustring.h"
#include "core/templates/vector.h"
#include "scene/main/node.h"
#endif

class SSProgressDialog;

class SSImporter : public Node {
  GDCLASS(SSImporter, Node)

protected:
  static void _bind_methods();

public:
  SSImporter();
  ~SSImporter();

  void _notification(int p_what);

  bool is_importing() const { return _is_importing; }

#ifdef SPRITESTUDIO_GODOT_EXTENSION
  void queue_import(const PackedStringArray &p_sspj_files, const String &p_output_dir);
#else
  void queue_import(const Vector<String> &p_sspj_files, const String &p_output_dir);
#endif

private:
  Vector<void *> _import_contexts;
  Vector<String> _import_dst_dirs;
  SSProgressDialog *_import_dialog = nullptr;
  Vector<bool> _import_finished_contexts;
  int _import_prev_num = 0;
  bool _is_importing = false;

  void *_process_file(const String &source_sspj_path, const String &dst_dir_path);
  void _finalize_import();
};

#endif // #ifdef TOOLS_ENABLED
