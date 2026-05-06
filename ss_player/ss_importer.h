#pragma once

#ifdef TOOLS_ENABLED

#include "ss_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
using namespace godot;
#else
#include "core/string/ustring.h"
#include "core/templates/vector.h"
#include "core/variant/dictionary.h"
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

  // Reconverts the given sspj files into their respective destination
  // directories without appending the sspj stem as a sub-folder. Both arrays
  // must have the same length. Used by the right-click "Convert" action on
  // existing ssab files.
  void queue_reconvert(const PackedStringArray &p_sspj_files, const PackedStringArray &p_dst_dirs);

  // Returns the sspj path previously recorded for the given ssab path,
  // or an empty String if no record exists.
  String lookup_sspj_for_ssab(const String &p_ssab_path) const;

  // Inserts (or updates) a single ssab → sspj entry. Used by the right-click
  // file-dialog fallback when no existing record is found.
  void record_ssab_source(const String &p_ssab_path, const String &p_sspj_path);

private:
  static const int MAX_SOURCE_MAP_ENTRIES = 500;

  Vector<void *> _import_contexts;
  Vector<String> _import_dst_dirs;
  Vector<String> _import_src_files;
  SSProgressDialog *_import_dialog = nullptr;
  String _session_title;
  Vector<bool> _import_finished_contexts;
  int _import_prev_num = 0;
  bool _is_importing = false;

  void *_process_file(const String &source_sspj_path, const String &dst_dir_path);
  void _enqueue_one(const String &p_sspj_path, const String &p_dst_dir);
  void _start_session(const String &p_dialog_title);
  void _finalize_import();

  Dictionary _load_source_map() const;
  void _save_source_map(const Dictionary &p_map);
  void _record_ssabs_in_dir(Dictionary &p_map, const String &p_dst_dir, const String &p_sspj_path);
  void _evict_lru(Dictionary &p_map);
};

#endif // #ifdef TOOLS_ENABLED
