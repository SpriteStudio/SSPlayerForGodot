#ifdef TOOLS_ENABLED

#include "ss_importer.h"

#include "ss_macros.h"
#include "ss_progress_dialog.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/accept_dialog.hpp>
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/confirmation_dialog.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
using namespace godot;
#else
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/config_file.h"
#include "core/io/file_access.h"
#include "core/io/resource.h"
#include "core/os/os.h"
#include "editor/editor_interface.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#if VERSION_MAJOR >= 4
    #if VERSION_MINOR >= 5
    #include "editor/file_system/editor_file_system.h"
    #else
    #include "editor/editor_file_system.h"
    #endif
#endif
#endif

#include "ssconverter.hpp"

void SSImporter::_bind_methods() {
    ADD_SIGNAL(MethodInfo("import_started"));
    ADD_SIGNAL(MethodInfo("import_finished"));
    ADD_SIGNAL(MethodInfo("import_files_resolved", PropertyInfo(Variant::PACKED_STRING_ARRAY, "sspj_paths")));
    ClassDB::bind_method(D_METHOD("_on_budget_use_found"), &SSImporter::_on_budget_use_found);
    ClassDB::bind_method(D_METHOD("_on_budget_action", "action"), &SSImporter::_on_budget_action);
    ClassDB::bind_method(D_METHOD("_abort_scan_and_idle"), &SSImporter::_abort_scan_and_idle);
    ClassDB::bind_method(D_METHOD("_on_collision_overwrite"), &SSImporter::_on_collision_overwrite);
    ClassDB::bind_method(D_METHOD("_on_collision_cancel"), &SSImporter::_on_collision_cancel);
}

SSImporter::SSImporter() {
}

SSImporter::~SSImporter() {
}

int SSImporter::_concurrency() const {
    int n = OS::get_singleton()->get_processor_count();
    if (n > 8) {
        n = 8;
    }
    if (n < 1) {
        n = 1;
    }
    return n;
}

void SSImporter::_notification(int p_what) {
    if (p_what != NOTIFICATION_PROCESS) {
        return;
    }

    if (_is_scanning) {
        Context *ctx = (Context *)_scan_context;

        if (!_scan_canceling && _import_dialog && _import_dialog->is_canceled()) {
            _scan_canceling = true;
            ss_converter_abort(ctx);
        }

        if (_import_dialog) {
            int visited = (int)ss_converter_scan_visited(ctx);
            int found = (int)ss_converter_scan_found(ctx) + _plan_src.size();
            _import_dialog->step(vformat("Scanning... found %d / visited %d", found, visited), 0);
        }

        if (ss_converter_is_finished(ctx)) {
            if (_scan_canceling) {
                _abort_scan_and_idle();
                return;
            }
            if (ss_converter_scan_budget_exceeded(ctx)) {
                set_process(false);
                if (_import_dialog) {
                    _import_dialog->set_visible(false);
                }
                _show_budget_dialog();
                return;
            }
            _scan_collect_current_results();
            _scan_finish_or_advance();
        }
        return;
    }

    if (_is_converting) {
        if (!_convert_canceling && _import_dialog && _import_dialog->is_canceled()) {
            _convert_canceling = true;
            for (int i = 0; i < _active_ctx.size(); i++) {
                ss_converter_abort((Context *)_active_ctx[i]);
            }
        }

        _convert_poll();
        if (!_convert_canceling) {
            _convert_pump();
        }

        if (_import_dialog && _convert_done != _convert_prev_done) {
            String running = _active_src.is_empty() ? String() : _active_src[0].get_file();
            if (_convert_canceling) {
                _import_dialog->step(vformat("Canceling... %d/%d", _convert_done, _convert_total), _convert_done);
            } else if (!running.is_empty()) {
                _import_dialog->step(vformat("%s %d/%d (%s)", _session_title, _convert_done, _convert_total, running), _convert_done);
            } else {
                _import_dialog->step(vformat("%s %d/%d", _session_title, _convert_done, _convert_total), _convert_done);
            }
            _convert_prev_done = _convert_done;
        }

        bool done = _active_ctx.is_empty() && (_convert_canceling || _pending_index >= _convert_total);
        if (done) {
            _finalize_convert();
        }
        return;
    }

    if (_fs_syncing) {
        _poll_fs_sync();
        return;
    }
}

// --------------------------------------------------------------------------
// Scan phase
// --------------------------------------------------------------------------

void SSImporter::_scan_start_current() {
    const String &dir = _scan_dirs[_scan_dir_index];
    CharString dir_utf8 = dir.utf8();
    ss_converter_scan_dir((Context *)_scan_context, dir_utf8.get_data(), SCAN_MAX_DEPTH, (uintptr_t)_scan_cur_max_entries, _scan_cur_max_millis);
}

void SSImporter::_scan_collect_current_results() {
    Context *ctx = (Context *)_scan_context;
    uintptr_t count = ss_converter_scan_result_count(ctx);
    for (uintptr_t i = 0; i < count; i++) {
        const char *p = ss_converter_scan_result_at(ctx, i);
        if (!p) {
            continue;
        }
        String sspj = String::utf8(p);
        _plan_src.push_back(sspj);
        _plan_dst.push_back(_output_dir.path_join(sspj.get_file().get_basename()));
    }
}

void SSImporter::_scan_finish_or_advance() {
    _scan_dir_index++;
    if (_scan_dir_index < _scan_dirs.size()) {
        _scan_cur_max_entries = SCAN_MAX_ENTRIES;
        _scan_cur_max_millis = SCAN_MAX_MILLIS;
        _scan_start_current();
        return;
    }
    _end_scan_and_convert();
}

void SSImporter::_end_scan_and_convert() {
    // Loose .sspj keep the legacy layout: <output>/<stem>/
    for (int i = 0; i < _scan_loose_sspj.size(); i++) {
        const String &src = _scan_loose_sspj[i];
        _plan_src.push_back(src);
        _plan_dst.push_back(_output_dir.path_join(src.get_file().get_basename()));
    }

    if (_scan_context) {
        ss_converter_destroy((Context *)_scan_context);
        _scan_context = nullptr;
    }
    _is_scanning = false;
    if (_import_dialog) {
        _import_dialog->finish();
        _import_dialog = nullptr;
    }

    if (_plan_src.is_empty()) {
        AcceptDialog *dialog = memnew(AcceptDialog);
        dialog->set_title(tr("Import"));
        dialog->set_text(tr("No .sspj files were found in the dropped folder(s)."));
        EditorInterface::get_singleton()->get_base_control()->add_child(dialog);
        dialog->connect("confirmed", Callable(dialog, "queue_free"));
        dialog->connect("canceled", Callable(dialog, "queue_free"));
        dialog->popup_centered();
        _idle_reset();
        return;
    }

    // Let the dock reflect the discovered .sspj in its Recent list.
    PackedStringArray resolved;
    for (int i = 0; i < _plan_src.size(); i++) {
        resolved.push_back(_plan_src[i]);
    }
    emit_signal("import_files_resolved", resolved);

    _begin_convert_checked("Importing SSPJ:");
}

void SSImporter::_abort_scan_and_idle() {
    _free_budget_dialog();
    if (_scan_context) {
        ss_converter_destroy((Context *)_scan_context);
        _scan_context = nullptr;
    }
    if (_import_dialog) {
        _import_dialog->finish();
        _import_dialog = nullptr;
    }
    _idle_reset();
}

void SSImporter::_show_budget_dialog() {
    Context *ctx = (Context *)_scan_context;
    int found = _plan_src.size() + (int)ss_converter_scan_found(ctx);

    ConfirmationDialog *dlg = memnew(ConfirmationDialog);
    dlg->set_title(tr("Scan stopped"));
    dlg->set_text(vformat(tr("The folder is very large, so scanning stopped early.\nFound %d .sspj file(s) so far.\n\nImport what was found, keep scanning, or stop?"), found));
    dlg->get_ok_button()->set_text(tr("Import found"));
    dlg->get_cancel_button()->set_text(tr("Stop"));
    dlg->add_button(tr("Keep scanning"), true, "rescan");

    EditorInterface::get_singleton()->get_base_control()->add_child(dlg);
    dlg->connect("confirmed", Callable(this, "_on_budget_use_found"));
    dlg->connect("custom_action", Callable(this, "_on_budget_action"));
    dlg->connect("canceled", Callable(this, "_abort_scan_and_idle"));
    dlg->popup_centered();

    _budget_dialog = dlg;
}

void SSImporter::_free_budget_dialog() {
    if (_budget_dialog) {
        _budget_dialog->queue_free();
        _budget_dialog = nullptr;
    }
}

void SSImporter::_on_budget_use_found() {
    _free_budget_dialog();
    _scan_collect_current_results();
    _end_scan_and_convert();
}

void SSImporter::_on_budget_action(const StringName &p_action) {
    _free_budget_dialog();
    if (p_action != StringName("rescan")) {
        return;
    }
    // Keep scanning the same directory from scratch without budget limits; the
    // Cancel button on the progress dialog remains the safety valve.
    _scan_cur_max_entries = (uint64_t)1 << 60;
    _scan_cur_max_millis = 0;
    if (_import_dialog) {
        _import_dialog->show_progress("Scanning SSPJ:", 0);
    }
    set_process(true);
    _scan_start_current();
}

// --------------------------------------------------------------------------
// Convert phase (bounded-concurrency scheduler)
// --------------------------------------------------------------------------

void *SSImporter::_process_file(const String &source_sspj_path, const String &dst_dir_path) {
    auto ctx = ss_converter_create();

    CharString src_utf8 = source_sspj_path.utf8();
    CharString dst_utf8 = dst_dir_path.utf8();

    ss_converter_convert(ctx, src_utf8.get_data(), dst_utf8.get_data(), false, [](const char *msg) {
        print_line(String::utf8(msg));
    });

    return ctx;
}

void SSImporter::_begin_convert_checked(const String &p_dialog_title) {
    Dictionary map = _load_source_map();
    Vector<int> collisions = _find_collisions(map);
    if (collisions.is_empty()) {
        _begin_convert(p_dialog_title);
        return;
    }
    _pending_convert_title = p_dialog_title;
    _awaiting_collision = true;
    _show_collision_dialog(collisions);
}

Vector<int> SSImporter::_find_collisions(const Dictionary &p_map) const {
    Vector<int> out;
    Array keys = p_map.keys();
    for (int i = 0; i < _plan_dst.size(); i++) {
        const String &dst = _plan_dst[i];
        String src_abs = ProjectSettings::get_singleton()->globalize_path(_plan_src[i]).simplify_path();
        for (int k = 0; k < keys.size(); k++) {
            String ssab = keys[k];
            if (ssab.get_base_dir() != dst) {
                continue;
            }
            String owner_abs = _make_absolute_path(p_map[ssab]).simplify_path();
            if (owner_abs != src_abs) {
                out.push_back(i);
                break;
            }
        }
    }
    return out;
}

void SSImporter::_show_collision_dialog(const Vector<int> &p_collisions) {
    String msg = tr("Some outputs already belong to a different SpriteStudio project and would be overwritten:\n\n");
    for (int j = 0; j < p_collisions.size(); j++) {
        msg += vformat("- %s\n", _plan_dst[p_collisions[j]]);
    }
    msg += tr("\nOverwrite them anyway?");

    ConfirmationDialog *dlg = memnew(ConfirmationDialog);
    dlg->set_title(tr("Output name collision"));
    dlg->set_text(msg);
    dlg->get_ok_button()->set_text(tr("Overwrite"));
    dlg->get_cancel_button()->set_text(tr("Cancel"));
    EditorInterface::get_singleton()->get_base_control()->add_child(dlg);
    dlg->connect("confirmed", Callable(this, "_on_collision_overwrite"));
    dlg->connect("canceled", Callable(this, "_on_collision_cancel"));
    dlg->popup_centered();

    _collision_dialog = dlg;
}

void SSImporter::_on_collision_overwrite() {
    if (_collision_dialog) {
        _collision_dialog->queue_free();
        _collision_dialog = nullptr;
    }
    _awaiting_collision = false;
    _begin_convert(_pending_convert_title);
}

void SSImporter::_on_collision_cancel() {
    if (_collision_dialog) {
        _collision_dialog->queue_free();
        _collision_dialog = nullptr;
    }
    _idle_reset();
}

void SSImporter::_begin_convert(const String &p_dialog_title) {
    _session_title = p_dialog_title;
    _convert_total = _plan_src.size();
    _convert_done = 0;
    _convert_prev_done = -1;
    _pending_index = 0;
    _convert_canceling = false;
    _active_ctx.clear();
    _active_src.clear();
    _active_dst.clear();
    _failed_files.clear();
    _failed_reasons.clear();
    _import_generated_files.clear();
    _convert_source_map = _load_source_map();

    // Make sure every destination directory exists. Brand-new folders (and
    // everything the converter writes into them) are registered with the
    // editor filesystem by the post-convert sync phase.
    Ref<DirAccess> da = DirAccess::open("res://");
    if (da.is_valid()) {
        for (int i = 0; i < _plan_dst.size(); i++) {
            if (!da->dir_exists(_plan_dst[i])) {
                da->make_dir_recursive(_plan_dst[i]);
            }
        }
    }

    _is_converting = true;

    _import_dialog = memnew(SSProgressDialog);
    EditorInterface::get_singleton()->get_base_control()->add_child(_import_dialog);
    _import_dialog->show_progress(p_dialog_title, _convert_total);
    _import_dialog->step(vformat("%s 0/%d", p_dialog_title, _convert_total), 0);

    set_process(true);
    _convert_pump();
}

void SSImporter::_convert_pump() {
    if (_convert_canceling) {
        return;
    }
    int cap = _concurrency();
    while (_active_ctx.size() < cap && _pending_index < _convert_total) {
        String src = _plan_src[_pending_index];
        String dst = _plan_dst[_pending_index];
        _pending_index++;

        String global_src = ProjectSettings::get_singleton()->globalize_path(src);
        String global_dst = ProjectSettings::get_singleton()->globalize_path(dst);
        void *ctx = _process_file(global_src, global_dst);
        print_line("SSImporter: convert sspj file: " + src + ", to ssab files: " + dst);

        _active_ctx.push_back(ctx);
        _active_src.push_back(global_src);
        _active_dst.push_back(dst);
    }
}

void SSImporter::_convert_poll() {
    for (int i = _active_ctx.size() - 1; i >= 0; i--) {
        Context *ctx = (Context *)_active_ctx[i];
        if (!ss_converter_is_finished(ctx)) {
            continue;
        }

        CConverterError result = ss_converter_get_result(ctx);
        if (result == CConverterError::Success) {
            _record_ssabs_in_dir(_convert_source_map, _active_dst[i], _active_src[i]);
        } else if (result == CConverterError::ErrorAborted) {
            // Canceled mid-file; not a failure, nothing to record.
        } else {
            const char *err_msg = nullptr;
            uintptr_t err_len = 0;
            ss_converter_get_error(ctx, &err_msg, &err_len);
            String err_str = err_msg ? String::utf8(err_msg) : String("Unknown error");
            ERR_PRINT(vformat("SSImporter: convert failed for %s (error %d): %s", _active_src[i], (int)result, err_str));
            _failed_files.push_back(_active_src[i].get_file());
            _failed_reasons.push_back(err_str);
        }

        ss_converter_destroy(ctx);
        _active_ctx.remove_at(i);
        _active_src.remove_at(i);
        _active_dst.remove_at(i);
        _convert_done++;
    }
}

void SSImporter::_finalize_convert() {
    if (_import_dialog) {
        _import_dialog->finish();
        _import_dialog = nullptr;
    }

    bool any_success = !_import_generated_files.is_empty();
    String target_dir = (any_success && !_output_dir.is_empty()) ? _output_dir : String();

    if (!_failed_files.is_empty()) {
        String msg = tr("Some files failed to import.\n\n");
        for (int i = 0; i < _failed_files.size(); ++i) {
            msg += vformat("- %s: %s\n", _failed_files[i], _failed_reasons[i]);
        }
        msg += tr("\nPlease check the Output tab for details.");

        AcceptDialog *dialog = memnew(AcceptDialog);
        dialog->set_title(tr("Import Error"));
        dialog->set_text(msg);
        EditorInterface::get_singleton()->get_base_control()->add_child(dialog);
        dialog->connect("confirmed", Callable(dialog, "queue_free"));
        dialog->connect("canceled", Callable(dialog, "queue_free"));
        dialog->popup_centered();
    }

    if (any_success) {
        _evict_lru(_convert_source_map);
        _save_source_map(_convert_source_map);
    }

    // So far _import_generated_files holds only the ssab/ssqb that
    // _record_ssabs_in_dir captured. The converter also writes the sibling PNG
    // textures the player requires (the layout is always "reference", never
    // embedded) plus any sub-folders, and those were never added here -- so
    // update_file() never registered them and their appearance in the dock
    // depended on the editor happening to scan the output folder, which is
    // unreliable on Windows. Rebuild the list by walking each output directory
    // ourselves (right after the converter closed the files, from the same
    // process) so every generated file is registered directly by exact path.
    if (any_success) {
        Vector<String> out_dirs;
        for (int i = 0; i < _import_generated_files.size(); i++) {
            String d = _import_generated_files[i].get_base_dir();
            if (!out_dirs.has(d)) {
                out_dirs.push_back(d);
            }
        }
        Vector<String> all_files;
        for (int i = 0; i < out_dirs.size(); i++) {
            _collect_output_files(out_dirs[i], all_files);
        }
        if (!all_files.is_empty()) {
            _import_generated_files = all_files;
        }
    }

    // Registering the generated files with the editor filesystem is deferred
    // to the sync phase: doing it here races with any scan the editor already
    // has in flight (e.g. the scan_changes() fired when the window regained
    // focus from the OS drag & drop), which can silently swallow both the
    // update_file() calls and a scan() request, leaving the new files
    // invisible until the next full rescan (editor restart).
    _navigate_dir = target_dir;
    _enter_fs_sync();
}

void SSImporter::_idle_reset() {
    set_process(false);

    if (_collision_dialog) {
        _collision_dialog->queue_free();
        _collision_dialog = nullptr;
    }
    _awaiting_collision = false;
    _pending_convert_title = String();

    _is_scanning = false;
    _scan_canceling = false;
    _is_converting = false;
    _convert_canceling = false;

    _scan_dirs.clear();
    _scan_loose_sspj.clear();
    _scan_dir_index = 0;
    _scan_cur_max_entries = SCAN_MAX_ENTRIES;
    _scan_cur_max_millis = SCAN_MAX_MILLIS;

    _plan_src.clear();
    _plan_dst.clear();
    _active_ctx.clear();
    _active_src.clear();
    _active_dst.clear();
    _pending_index = 0;
    _convert_total = 0;
    _convert_done = 0;
    _convert_prev_done = -1;
    _failed_files.clear();
    _failed_reasons.clear();
    _import_generated_files.clear();
    _import_dialog = nullptr;

    _fs_syncing = false;
    _fs_scan_issued = false;
    _fs_settle_frames = 0;
    _fs_wait_frames = 0;
    _navigate_dir = String();

    emit_signal("import_finished");
}

// --------------------------------------------------------------------------
// Filesystem-sync phase
// --------------------------------------------------------------------------

void SSImporter::_enter_fs_sync() {
    // Release the convert-phase flag so the PROCESS poll falls through to
    // _poll_fs_sync(); everything else (generated file list, dialogs already
    // closed) is cleaned up by _idle_reset() once the sync finishes.
    _is_converting = false;
    _convert_canceling = false;

    _fs_syncing = true;
    _fs_scan_issued = false;
    _fs_settle_frames = 0;
    _fs_wait_frames = 0;
    set_process(true);
}

void SSImporter::_poll_fs_sync() {
#if defined(SPRITESTUDIO_GODOT_EXTENSION) || (VERSION_MAJOR >= 4 && VERSION_MINOR >= 6)
    auto *efs = EditorInterface::get_singleton()->get_resource_filesystem();
#else
    auto *efs = EditorInterface::get_singleton()->get_resource_file_system();
#endif
    if (!efs) {
        _finish_fs_sync();
        return;
    }

    _fs_wait_frames++;
    // Safety valve so a perpetually-busy editor cannot wedge the importer; on
    // timeout the (self-queuing) scan request below is still issued, so the
    // files are picked up eventually even though we stop waiting for them.
    bool timed_out = _fs_wait_frames > FS_SYNC_MAX_WAIT_FRAMES;

    if (!_fs_scan_issued) {
        // A scan that is already running may have listed the output
        // directories before the converter finished writing into them, so its
        // results cannot be trusted (and it would silently swallow update_file
        // and scan requests). Wait for the editor to go idle first.
        if (efs->is_scanning() && !timed_out) {
            return;
        }
        // Register the outputs we know about (this also creates the in-memory
        // entries for brand-new folders), then request a sources scan for
        // everything else the converter wrote (textures, subfolders). Unlike
        // scan(), which is dropped without retry while another scan runs,
        // scan_sources()/scan_changes() re-queues itself and is never lost; it
        // registers new subdirectories and auto-imports importable files.
        for (int i = 0; i < _import_generated_files.size(); i++) {
            efs->update_file(_import_generated_files[i]);
        }
        // Diagnostic for the Windows-only repro: a generated file we cannot
        // open for reading here means another process (e.g. antivirus) still
        // holds it, which no scan retry can fix. Logging it lets a real repro
        // distinguish an external file lock from scan timing.
        int unopenable = 0;
        for (int i = 0; i < _import_generated_files.size(); i++) {
            Ref<FileAccess> fa = FileAccess::open(_import_generated_files[i], FileAccess::READ);
            if (fa.is_null()) {
                unopenable++;
            }
        }
        if (unopenable > 0) {
            WARN_PRINT(vformat("SSImporter: %d of %d generated files were not openable at registration time (possible external file lock).", unopenable, (int)_import_generated_files.size()));
        }
#ifdef SPRITESTUDIO_GODOT_EXTENSION
        efs->scan_sources();
#else
        efs->scan_changes();
#endif
        _fs_scan_issued = true;
        _fs_settle_frames = 0;
        if (timed_out) {
            _finish_fs_sync();
        }
        return;
    }

    if (timed_out) {
        _finish_fs_sync();
        return;
    }

    // Wait for the requested scan to run to completion before revealing the
    // output folder. The scan may start a frame or two late (it queued behind
    // a scan that slipped in first) or may have already completed
    // synchronously, so instead of tracking start/stop edges just require the
    // filesystem to stay idle for a few consecutive frames.
    if (efs->is_scanning()) {
        _fs_settle_frames = 0;
        return;
    }
    _fs_settle_frames++;
    if (_fs_settle_frames >= FS_SYNC_SETTLE_FRAMES) {
        _finish_fs_sync();
    }
}

void SSImporter::_finish_fs_sync() {
    if (!_navigate_dir.is_empty()) {
        Object *fs_dock = (Object *)EditorInterface::get_singleton()->get_file_system_dock();
        if (fs_dock) {
            fs_dock->call_deferred("navigate_to_path", _navigate_dir);
        }
    }
    _idle_reset();
}

// --------------------------------------------------------------------------
// Public entry points
// --------------------------------------------------------------------------

#ifdef SPRITESTUDIO_GODOT_EXTENSION
void SSImporter::queue_import(const PackedStringArray &p_sspj_files, const String &p_output_dir) {
#else
void SSImporter::queue_import(const Vector<String> &p_sspj_files, const String &p_output_dir) {
#endif
    if (is_importing()) {
        WARN_PRINT("SSImporter: Already importing. Please wait.");
        return;
    }
    if (p_sspj_files.is_empty()) {
        return;
    }

    _output_dir = p_output_dir;
    _plan_src.clear();
    _plan_dst.clear();
    for (int i = 0; i < p_sspj_files.size(); i++) {
        String src = p_sspj_files[i];
        _plan_src.push_back(src);
        _plan_dst.push_back(p_output_dir.path_join(src.get_file().get_basename()));
    }

    emit_signal("import_started");
    _begin_convert_checked("Importing SSPJ:");
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
void SSImporter::queue_scan_and_import(const PackedStringArray &p_dirs, const PackedStringArray &p_loose_sspj, const String &p_output_dir) {
#else
void SSImporter::queue_scan_and_import(const Vector<String> &p_dirs, const Vector<String> &p_loose_sspj, const String &p_output_dir) {
#endif
    if (is_importing()) {
        WARN_PRINT("SSImporter: Already importing. Please wait.");
        return;
    }
    if (p_dirs.is_empty() && p_loose_sspj.is_empty()) {
        return;
    }

    _output_dir = p_output_dir;
    _plan_src.clear();
    _plan_dst.clear();

    _scan_loose_sspj.clear();
    for (int i = 0; i < p_loose_sspj.size(); i++) {
        _scan_loose_sspj.push_back(p_loose_sspj[i]);
    }

    _scan_dirs.clear();
    for (int i = 0; i < p_dirs.size(); i++) {
        _scan_dirs.push_back(p_dirs[i]);
    }
    _scan_dir_index = 0;
    _scan_cur_max_entries = SCAN_MAX_ENTRIES;
    _scan_cur_max_millis = SCAN_MAX_MILLIS;

    emit_signal("import_started");

    if (_scan_dirs.is_empty()) {
        // Nothing to scan: just convert the loose files.
        _end_scan_and_convert();
        return;
    }

    _scan_context = ss_converter_create();
    _is_scanning = true;
    _scan_canceling = false;

    _import_dialog = memnew(SSProgressDialog);
    EditorInterface::get_singleton()->get_base_control()->add_child(_import_dialog);
    _import_dialog->show_progress("Scanning SSPJ:", 0);
    _import_dialog->step("Scanning...", 0);

    set_process(true);
    _scan_start_current();
}

void SSImporter::queue_reconvert(const PackedStringArray &p_sspj_files, const PackedStringArray &p_dst_dirs) {
    if (is_importing()) {
        WARN_PRINT("SSImporter: Already importing. Please wait.");
        return;
    }
    if (p_sspj_files.is_empty() || p_sspj_files.size() != p_dst_dirs.size()) {
        return;
    }

    _output_dir = String();
    _plan_src.clear();
    _plan_dst.clear();
    for (int i = 0; i < p_sspj_files.size(); i++) {
        _plan_src.push_back(p_sspj_files[i]);
        _plan_dst.push_back(p_dst_dirs[i]);
    }

    emit_signal("import_started");
    _begin_convert("Reconverting SSPJ:");
}

// --------------------------------------------------------------------------
// Source map + helpers (unchanged)
// --------------------------------------------------------------------------

String SSImporter::_make_relative_path(const String &p_abs_path) const {
    String base_dir = ProjectSettings::get_singleton()->globalize_path("res://");
    base_dir = base_dir.replace("\\", "/");
    String target = p_abs_path.replace("\\", "/");

    if (target.is_empty()) {
        return target;
    }

    PackedStringArray base_parts = base_dir.split("/", false);
    PackedStringArray target_parts = target.split("/", false);

    int common_idx = 0;
    while (common_idx < base_parts.size() && common_idx < target_parts.size()) {
        if (base_parts[common_idx] == target_parts[common_idx]) {
            common_idx++;
        } else {
            break;
        }
    }

    if (common_idx == 0) {
        return target;
    }

    String rel_path = "";
    for (int i = common_idx; i < base_parts.size(); i++) {
        rel_path += "../";
    }
    for (int i = common_idx; i < target_parts.size(); i++) {
        rel_path += target_parts[i];
        if (i < target_parts.size() - 1) {
            rel_path += "/";
        }
    }

    return rel_path;
}

String SSImporter::_make_absolute_path(const String &p_rel_path) const {
    if (p_rel_path.is_empty()) {
        return p_rel_path;
    }
    if (p_rel_path.is_absolute_path() || p_rel_path.contains(":/")) {
        return p_rel_path;
    }
    String base_dir = ProjectSettings::get_singleton()->globalize_path("res://");
    return base_dir.path_join(p_rel_path).simplify_path();
}

Dictionary SSImporter::_load_source_map() const {
    Ref<ConfigFile> cfg;
    cfg.instantiate();
    Error err = cfg->load(SSPLAYER_SOURCES_CFG_PATH);
    if (err != OK) {
        return Dictionary();
    }
    if (!cfg->has_section("ssab_sources")) {
        return Dictionary();
    }

    Dictionary map;
    PackedStringArray keys = cfg->get_section_keys("ssab_sources");
    for (int i = 0; i < keys.size(); i++) {
        String ssab_path = keys[i];
        String sspj_rel_path = cfg->get_value("ssab_sources", ssab_path);
        map[ssab_path] = sspj_rel_path;
    }
    return map;
}

void SSImporter::_save_source_map(const Dictionary &p_map) {
    Ref<ConfigFile> cfg;
    cfg.instantiate();

    Array keys = p_map.keys();
    for (int i = 0; i < keys.size(); i++) {
        String ssab_path = keys[i];
        String sspj_rel_path = p_map[ssab_path];
        cfg->set_value("ssab_sources", ssab_path, sspj_rel_path);
    }

    cfg->save(SSPLAYER_SOURCES_CFG_PATH);
}

void SSImporter::_record_ssabs_in_dir(Dictionary &p_map, const String &p_dst_dir, const String &p_sspj_path) {
    Ref<DirAccess> da = DirAccess::open(p_dst_dir);
    if (da.is_null()) {
        return;
    }

    da->list_dir_begin();
    String fname = da->get_next();
    while (!fname.is_empty()) {
        if (!da->current_is_dir()) {
            String ext = fname.get_extension();
            if (ext == "ssab" || ext == "ssqb") {
                String output_path = p_dst_dir.path_join(fname);
                // Re-insert to bump to most-recent in iteration order.
                p_map.erase(output_path);
                p_map[output_path] = _make_relative_path(p_sspj_path);
                _refresh_cached_output(output_path);
                _import_generated_files.push_back(output_path);
            }
        }
        fname = da->get_next();
    }
    da->list_dir_end();
}

void SSImporter::_collect_output_files(const String &p_dir, Vector<String> &r_out) const {
    Ref<DirAccess> da = DirAccess::open(p_dir);
    if (da.is_null()) {
        return;
    }

    da->list_dir_begin();
    String fname = da->get_next();
    while (!fname.is_empty()) {
        // Guard against the navigational entries so the recursion terminates.
        if (fname != "." && fname != "..") {
            String path = p_dir.path_join(fname);
            if (da->current_is_dir()) {
                _collect_output_files(path, r_out);
            } else {
                // .import and .uid are editor-generated sidecars, not converter
                // outputs and not standalone resources. They are present when
                // reconverting into a directory that was already imported;
                // registering them makes the editor show them as broken ("X").
                // Register only the real files the converter wrote.
                String ext = fname.get_extension().to_lower();
                if (ext != "import" && ext != "uid") {
                    r_out.push_back(path);
                }
            }
        }
        fname = da->get_next();
    }
    da->list_dir_end();
}

void SSImporter::_refresh_cached_output(const String &p_output_path) {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    if (!ResourceLoader::get_singleton()->has_cached(p_output_path)) {
        return;
    }
    Ref<Resource> existing = ResourceLoader::get_singleton()->load(p_output_path, "", ResourceLoader::CACHE_MODE_REUSE);
#else
    if (!ResourceCache::has(p_output_path)) {
        return;
    }
    Ref<Resource> existing = ResourceCache::get_ref(p_output_path);
#endif
    if (existing.is_null()) {
        return;
    }
    // Both SSABResource and SSQBResource expose load_from_file via ClassDB.
    existing->call("load_from_file", p_output_path);
    existing->emit_changed();
}

void SSImporter::_evict_lru(Dictionary &p_map) {
    while (p_map.size() > MAX_SOURCE_MAP_ENTRIES) {
        Array keys = p_map.keys();
        if (keys.is_empty()) {
            break;
        }
        p_map.erase(keys[0]);
    }
}

String SSImporter::lookup_sspj_for_ssab(const String &p_ssab_path) const {
    Dictionary map = _load_source_map();
    if (map.has(p_ssab_path)) {
        return _make_absolute_path(map[p_ssab_path]);
    }
    return String();
}

String SSImporter::lookup_output_dir_for_sspj(const String &p_sspj_path) const {
    if (p_sspj_path.is_empty()) {
        return String();
    }
    Dictionary map = _load_source_map();
    Array keys = map.keys();
    // Iterate from most-recently-inserted backward so the latest known
    // location for this sspj wins.
    for (int i = keys.size() - 1; i >= 0; i--) {
        String ssab_path = keys[i];
        String sspj = _make_absolute_path(map[ssab_path]);
        if (sspj == p_sspj_path) {
            return ssab_path.get_base_dir();
        }
    }
    return String();
}

void SSImporter::record_ssab_source(const String &p_ssab_path, const String &p_sspj_path) {
    if (p_ssab_path.is_empty() || p_sspj_path.is_empty()) {
        return;
    }
    Dictionary map = _load_source_map();
    map.erase(p_ssab_path);
    map[p_ssab_path] = _make_relative_path(p_sspj_path);
    _evict_lru(map);
    _save_source_map(map);
}

#endif // #ifdef TOOLS_ENABLED
