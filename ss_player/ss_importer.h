#pragma once

#ifdef TOOLS_ENABLED

#include "ss_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
using namespace godot;
#else
#include "core/string/string_name.h"
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

  // True while a directory scan OR a conversion batch is in progress (including
  // while a pre-convert collision prompt is open and while the post-convert
  // filesystem sync is still waiting on the editor scan).
  bool is_importing() const { return _is_scanning || _is_converting || _awaiting_collision || _fs_syncing; }

#ifdef SPRITESTUDIO_GODOT_EXTENSION
  void queue_import(const PackedStringArray &p_sspj_files, const String &p_output_dir);
  // Recursively scan each directory in p_dirs for .sspj files, then batch-import
  // the discovered files together with p_loose_sspj into p_output_dir. The scan
  // runs asynchronously in libssconverter and is cancellable / budgeted.
  void queue_scan_and_import(const PackedStringArray &p_dirs, const PackedStringArray &p_loose_sspj, const String &p_output_dir);
#else
  void queue_import(const Vector<String> &p_sspj_files, const String &p_output_dir);
  void queue_scan_and_import(const Vector<String> &p_dirs, const Vector<String> &p_loose_sspj, const String &p_output_dir);
#endif

  // Reconverts the given sspj files into their respective destination
  // directories without appending the sspj stem as a sub-folder. Both arrays
  // must have the same length. Used by the right-click "Convert" action on
  // existing ssab files.
  void queue_reconvert(const PackedStringArray &p_sspj_files, const PackedStringArray &p_dst_dirs);

  // Returns the sspj path previously recorded for the given ssab path,
  // or an empty String if no record exists.
  String lookup_sspj_for_ssab(const String &p_ssab_path) const;

  // Returns the output directory for a previously imported sspj, derived
  // from the parent dir of the most recently inserted ssab/ssqb keyed to
  // this sspj. Returns an empty String when no record exists.
  String lookup_output_dir_for_sspj(const String &p_sspj_path) const;

  // Inserts (or updates) a single ssab → sspj entry. Used by the right-click
  // file-dialog fallback when no existing record is found.
  void record_ssab_source(const String &p_ssab_path, const String &p_sspj_path);

private:
  static const int MAX_SOURCE_MAP_ENTRIES = 500;

  // Directory-scan budget (passed to ss_converter_scan_dir). Policy lives on the
  // editor side so each engine integration can choose its own values; the scan
  // stops once any of these is exceeded and asks the user what to do.
  static const uint32_t SCAN_MAX_DEPTH = 16;
  static const uint64_t SCAN_MAX_ENTRIES = 50000;
  static const uint64_t SCAN_MAX_MILLIS = 5000;

  // Post-convert filesystem-sync budget. MAX_WAIT caps how long we wait for
  // the editor's scans (safety valve, ~30 s at 60 fps); SETTLE is how many
  // consecutive idle frames must pass before the requested scan is considered
  // done (covers the 1-2 frame gap before a self-queued scan actually starts).
  static const int FS_SYNC_MAX_WAIT_FRAMES = 1800;
  static const int FS_SYNC_SETTLE_FRAMES = 10;

  // ---- shared across scan + convert ----
  SSProgressDialog *_import_dialog = nullptr;
  Node *_budget_dialog = nullptr;
  Node *_collision_dialog = nullptr;
  bool _awaiting_collision = false;
  String _pending_convert_title;
  String _session_title;
  String _output_dir;          // res:// output root
  Vector<String> _plan_src;    // absolute .sspj paths to convert
  Vector<String> _plan_dst;    // matching res:// destination dirs
  Vector<String> _import_generated_files;

  // ---- filesystem-sync phase (post-convert) ----
  // A scan that is already running when conversion finishes may have listed
  // the output directories before the converter wrote into them, so its
  // results cannot be trusted; scan() requests issued while it runs are
  // silently dropped, and update_file() is a no-op during a full scan. The
  // sync phase therefore waits for the editor to go idle, registers the
  // generated files, requests a sources scan (self-queuing, never dropped)
  // and waits for it to complete before revealing the output folder.
  bool _fs_syncing = false;
  bool _fs_scan_issued = false;
  bool _fs_reimporting = false; // guards against re-entry while reimport_files() pumps the loop
  int _fs_settle_frames = 0;
  int _fs_wait_frames = 0;
  String _navigate_dir; // output folder to reveal in the dock when sync ends

  // ---- scan phase ----
  bool _is_scanning = false;
  bool _scan_canceling = false;
  void *_scan_context = nullptr;
  Vector<String> _scan_dirs;       // absolute dir paths to scan
  int _scan_dir_index = 0;
  Vector<String> _scan_loose_sspj; // absolute .sspj dropped directly
  uint64_t _scan_cur_max_entries = SCAN_MAX_ENTRIES;
  uint64_t _scan_cur_max_millis = SCAN_MAX_MILLIS;

  // ---- convert phase (bounded-concurrency scheduler) ----
  bool _is_converting = false;
  bool _convert_canceling = false;
  Vector<void *> _active_ctx;
  Vector<String> _active_src;  // absolute sspj (for error/record)
  Vector<String> _active_dst;  // res:// dst dir (for record)
  int _pending_index = 0;
  int _convert_total = 0;
  int _convert_done = 0;
  int _convert_prev_done = -1;
  Dictionary _convert_source_map;
  Vector<String> _failed_files;
  Vector<String> _failed_reasons;

  // scan helpers
  void _scan_start_current();
  void _scan_collect_current_results();
  void _scan_finish_or_advance();
  void _end_scan_and_convert();
  void _abort_scan_and_idle();
  void _show_budget_dialog();
  void _free_budget_dialog();
  void _on_budget_use_found();
  void _on_budget_action(const StringName &p_action);
  int _concurrency() const;

  // convert helpers
  void _begin_convert_checked(const String &p_dialog_title);
  Vector<int> _find_collisions(const Dictionary &p_map) const;
  void _show_collision_dialog(const Vector<int> &p_collisions);
  void _on_collision_overwrite();
  void _on_collision_cancel();
  void _begin_convert(const String &p_dialog_title);
  void _convert_pump();
  void _convert_poll();
  void _finalize_convert();

  void _idle_reset();

  void *_process_file(const String &source_sspj_path, const String &dst_dir_path);

  // filesystem-sync helpers (see the member block above for the rationale)
  void _enter_fs_sync();
  void _poll_fs_sync();
  void _finish_fs_sync();

  Dictionary _load_source_map() const;
  void _save_source_map(const Dictionary &p_map);
  String _make_relative_path(const String &p_abs_path) const;
  String _make_absolute_path(const String &p_rel_path) const;
  void _record_ssabs_in_dir(Dictionary &p_map, const String &p_dst_dir, const String &p_sspj_path);
  // Recursively collects every file the converter wrote under p_dir (ssab/ssqb,
  // the sibling PNG textures the player requires, and any sub-folder contents)
  // into r_out, so each can be registered with the editor filesystem by exact
  // path instead of relying on a directory scan to discover it.
  void _collect_output_files(const String &p_dir, Vector<String> &r_out) const;
  void _evict_lru(Dictionary &p_map);
  // Refreshes the in-memory cached resource (SSABResource / SSQBResource) so
  // any active SpriteStudioPlayer2D referencing it picks up the new binary
  // without requiring a scene reload. No-op if the resource is not cached.
  void _refresh_cached_output(const String &p_output_path);
};

#endif // #ifdef TOOLS_ENABLED
