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

    if (_awaiting_reimport) {
        _poll_reimport();
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
    _pending_reimport.clear();
    _navigate_dir = String();
    _convert_source_map = _load_source_map();

    // Make sure every destination directory exists. A missing dir means new
    // folders appear in res:// and the editor filesystem needs a full rescan.
    Ref<DirAccess> da = DirAccess::open("res://");
    if (da.is_valid()) {
        for (int i = 0; i < _plan_dst.size(); i++) {
            if (!da->dir_exists(_plan_dst[i])) {
                _needs_full_scan = true;
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
            _record_generated_files_in_dir(_convert_source_map, _active_dst[i], _active_src[i]);
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

#if defined(SPRITESTUDIO_GODOT_EXTENSION) || (VERSION_MAJOR >= 4 && VERSION_MINOR >= 6)
    auto *efs = EditorInterface::get_singleton()->get_resource_filesystem();
#else
    auto *efs = EditorInterface::get_singleton()->get_resource_file_system();
#endif
    for (int i = 0; i < _import_generated_files.size(); i++) {
        efs->update_file(_import_generated_files[i]);
    }
    // A full recursive scan is the only thing that registers files inside a
    // brand-new subdirectory (font/, sound/) — update_file and scan_changes both
    // miss them. Force it whenever there are textures to import; the reimport phase
    // waits for this scan to finish before importing the exact files.
    if (_needs_full_scan || !_pending_reimport.is_empty()) {
        efs->scan();
    } else {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
        efs->scan_sources();
#else
        efs->scan_changes();
#endif
    }

    // If textures were written, hand off to the deterministic reimport phase: it
    // waits for the scan kicked above (or the editor's own auto-scan) to finish —
    // so every file, subfolders included, is registered — then reimports the exact
    // files on a normal frame. This replaces the old timer / `filesystem_changed`
    // approach, whose timing race made subfolder import flaky (sometimes needing a
    // focus toggle or an editor restart, sometimes not).
    _navigate_dir = target_dir;
    if (any_success && !_pending_reimport.is_empty()) {
        _enter_reimport_wait();
        return;
    }

    if (!target_dir.is_empty()) {
        Object *fs_dock = (Object *)EditorInterface::get_singleton()->get_file_system_dock();
        if (fs_dock) {
            fs_dock->call_deferred("navigate_to_path", target_dir);
        }
    }
    _idle_reset();
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
    _needs_full_scan = false;
    _import_dialog = nullptr;

    emit_signal("import_finished");
}

void SSImporter::_enter_reimport_wait() {
    // Release the convert-phase state so the PROCESS poll falls through to the
    // reimport branch instead of re-running conversion, but keep processing and do
    // NOT emit `import_finished` yet — that fires from `_finish_reimport`, once the
    // textures are actually imported.
    _is_converting = false;
    _convert_canceling = false;
    _active_ctx.clear();
    _active_src.clear();
    _active_dst.clear();
    _pending_index = 0;
    _convert_total = 0;
    _convert_done = 0;
    _convert_prev_done = -1;

    _awaiting_reimport = true;
    _reimport_scan_seen = false;
    _reimport_in_progress = false;
    _reimport_wait_frames = 0;
    _reimport_settle = 0;
    set_process(true);
}

void SSImporter::_poll_reimport() {
    // reimport_files() below shows a progress dialog that pumps the main loop, which
    // re-enters this PROCESS callback. Bail on re-entry so we never call
    // reimport_files() recursively (Godot forbids it: "reimport_files() recursively").
    if (_reimport_in_progress) {
        return;
    }
#if defined(SPRITESTUDIO_GODOT_EXTENSION) || (VERSION_MAJOR >= 4 && VERSION_MINOR >= 6)
    auto *efs = EditorInterface::get_singleton()->get_resource_filesystem();
#else
    auto *efs = EditorInterface::get_singleton()->get_resource_file_system();
#endif
    _reimport_wait_frames++;

    // Wait for the scan (ours from `_finalize_convert`, or the editor's own
    // auto-scan reacting to the freshly written files) to finish, so every file —
    // subfolders included — is registered before we reimport. A hard cap keeps a
    // perpetually-busy editor from wedging us here.
    bool timed_out = _reimport_wait_frames > 1800;
    if (!timed_out) {
        if (efs && efs->is_scanning()) {
            // A scan is running; remember it and keep waiting for it to finish.
            _reimport_scan_seen = true;
            _reimport_settle = 0;
            return;
        }
        // Not scanning right now. scan() is async, so if we haven't yet observed a
        // scan start, give it a brief grace period before concluding it's done —
        // otherwise we'd reimport before the scan registers the subfolder files.
        if (!_reimport_scan_seen && _reimport_wait_frames < 4) {
            return;
        }
        // Scan finished (or was instant). Let its registration settle a few frames.
        if (_reimport_settle < 3) {
            _reimport_settle++;
            return;
        }
    }

    // The scan above only REGISTERS the new files (they show as broken "×");
    // in-session, reimport_files() is the only thing that actually imports them.
    // The files are registered now (scan settled), so this first-imports them —
    // subfolders included. The re-entry guard at the top absorbs the PROCESS
    // callbacks fired by this call's own progress-dialog pump.
    if (efs && !_pending_reimport.is_empty()) {
        PackedStringArray to_reimport;
        for (int i = 0; i < _pending_reimport.size(); i++) {
            to_reimport.push_back(_pending_reimport[i]);
        }
        _reimport_in_progress = true;
        efs->reimport_files(to_reimport);
        _reimport_in_progress = false;
    }
    _pending_reimport.clear();
    _finish_reimport();
}

void SSImporter::_finish_reimport() {
    if (!_navigate_dir.is_empty()) {
        Object *fs_dock = (Object *)EditorInterface::get_singleton()->get_file_system_dock();
        if (fs_dock) {
            fs_dock->call_deferred("navigate_to_path", _navigate_dir);
        }
        _navigate_dir = String();
    }
    _awaiting_reimport = false;
    _reimport_scan_seen = false;
    _reimport_wait_frames = 0;
    _reimport_settle = 0;
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

void SSImporter::_record_generated_files_in_dir(Dictionary &p_map, const String &p_dst_dir, const String &p_sspj_path) {
    Ref<DirAccess> da = DirAccess::open(p_dst_dir);
    if (da.is_null()) {
        return;
    }

    da->list_dir_begin();
    String fname = da->get_next();
    while (!fname.is_empty()) {
        if (fname == "." || fname == "..") {
            fname = da->get_next();
            continue;
        }
        String path = p_dst_dir.path_join(fname);
        // Subdirectories (font/, sound/) the converter created are intentionally
        // NOT descended into: in-session, neither update_file nor scan() registers
        // a brand-new subdir with the EditorFileSystem, so its files can be neither
        // found nor reimported (they show up only after the editor's next startup /
        // full re-scan). Queuing them here just produces "Can't find file during
        // reimport" errors, so we leave them for that restart.
        if (!da->current_is_dir()) {
            String ext = fname.get_extension().to_lower();
            if (ext == "ssab" || ext == "ssqb") {
                // Re-insert to bump to most-recent in iteration order.
                p_map.erase(path);
                p_map[path] = _make_relative_path(p_sspj_path);
                _refresh_cached_output(path);
                _import_generated_files.push_back(path);
            } else if (ext != "import") {
                // A top-level texture the converter wrote alongside the binary.
                // update_file registers it (parent dir is known), so the deferred
                // reimport in _poll_reimport can first-import it in-session.
                // Not in the source map — only .ssab/.ssqb map to a .sspj.
                _import_generated_files.push_back(path);
                _pending_reimport.push_back(path);
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
