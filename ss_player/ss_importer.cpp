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

#include "ssconverter.h"

void SSImporter::_bind_methods() {
    ADD_SIGNAL(MethodInfo("import_started"));
    ADD_SIGNAL(MethodInfo("import_finished"));
    ADD_SIGNAL(MethodInfo("import_files_resolved", PropertyInfo(Variant::PACKED_STRING_ARRAY, "sspj_paths")));
    ClassDB::bind_method(D_METHOD("_on_filesystem_changed", "dir"), &SSImporter::_on_filesystem_changed);
    ClassDB::bind_method(D_METHOD("_on_budget_use_found"), &SSImporter::_on_budget_use_found);
    ClassDB::bind_method(D_METHOD("_on_budget_action", "action"), &SSImporter::_on_budget_action);
    ClassDB::bind_method(D_METHOD("_abort_scan_and_idle"), &SSImporter::_abort_scan_and_idle);
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

    _begin_convert("Importing SSPJ:");
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

    ss_converter_convert(ctx, src_utf8.get_data(), dst_utf8.get_data(), [](const char *msg) {
        print_line(String::utf8(msg));
    });

    return ctx;
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

#if defined(SPRITESTUDIO_GODOT_EXTENSION) || (VERSION_MAJOR >= 4 && VERSION_MINOR >= 6)
    auto *efs = EditorInterface::get_singleton()->get_resource_filesystem();
#else
    auto *efs = EditorInterface::get_singleton()->get_resource_file_system();
#endif
    for (int i = 0; i < _import_generated_files.size(); i++) {
        efs->update_file(_import_generated_files[i]);
    }
    if (_needs_full_scan) {
        efs->scan();
    } else {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
        efs->scan_sources();
#else
        efs->scan_changes();
#endif
    }

    if (!target_dir.is_empty()) {
        if (!efs->is_connected("filesystem_changed", Callable(this, "_on_filesystem_changed"))) {
            efs->connect("filesystem_changed", Callable(this, "_on_filesystem_changed").bind(target_dir), Object::CONNECT_ONE_SHOT);
        }
    }

    _idle_reset();
}

void SSImporter::_idle_reset() {
    set_process(false);

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

void SSImporter::_on_filesystem_changed(const String &p_dir) {
    Object *fs_dock = (Object *)EditorInterface::get_singleton()->get_file_system_dock();
    if (fs_dock) {
        fs_dock->call_deferred("navigate_to_path", p_dir);
    }
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
    _begin_convert("Importing SSPJ:");
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
