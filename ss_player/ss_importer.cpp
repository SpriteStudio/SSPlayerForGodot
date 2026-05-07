#ifdef TOOLS_ENABLED

#include "ss_importer.h"

#include "ss_macros.h"
#include "ss_progress_dialog.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
using namespace godot;
#else
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/resource.h"
#include "editor/editor_interface.h"
#include "editor/settings/editor_settings.h"
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
}

SSImporter::SSImporter() {
}

SSImporter::~SSImporter() {
}

void SSImporter::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_PROCESS: {
            if (!_is_importing) {
                break;
            }

            bool wait_for_finish = false;

            for (size_t i = 0; i < _import_contexts.size(); ++i) {
                void *ctx = _import_contexts[i];
                bool ret = ss_converter_is_finished((Context *)ctx);
                if (!ret) {
                    wait_for_finish = true;
                } else {
                    _import_finished_contexts.set(i, true);
                }
            }

            int finished_num = 0;
            for (size_t i = 0; i < _import_finished_contexts.size(); ++i) {
                if (_import_finished_contexts[i]) {
                    finished_num++;
                }
            }

            if (finished_num != _import_prev_num) {
                _import_dialog->step(vformat("%s %d/%d", _session_title, finished_num, _import_finished_contexts.size()), finished_num);
                _import_prev_num = finished_num;
            }

            if (!wait_for_finish) {
                _finalize_import();
            }
        } break;
    }
}

void *SSImporter::_process_file(const String &source_sspj_path, const String &dst_dir_path) {
    auto ctx = ss_converter_create();

    CharString src_utf8 = source_sspj_path.utf8();
    CharString dst_utf8 = dst_dir_path.utf8();

    ss_converter_convert(ctx, src_utf8.get_data(), dst_utf8.get_data(), [](const char *msg) {
        print_line(String::utf8(msg));
    });

    return ctx;
}

void SSImporter::_finalize_import() {
    _import_dialog->finish();

    Dictionary source_map = _load_source_map();
    bool any_success = false;

    for (size_t i = 0; i < _import_contexts.size(); ++i) {
        void *ctx = _import_contexts[i];
        CConverterError result = ss_converter_get_result((Context *)ctx);
        if (result == CConverterError::Success) {
            _record_ssabs_in_dir(source_map, _import_dst_dirs[i], _import_src_files[i]);
            any_success = true;
        } else {
            print_line(vformat("SSImporter: convert failed for %s (error %d)", _import_src_files[i], (int)result));
        }
        ss_converter_destroy((Context *)ctx);
    }

    if (any_success) {
        _evict_lru(source_map);
        _save_source_map(source_map);
    }

    _import_contexts.clear();
    _import_finished_contexts.clear();
    _import_src_files.clear();

#if defined(SPRITESTUDIO_GODOT_EXTENSION) || (VERSION_MAJOR >= 4 && VERSION_MINOR >= 6)
    auto *efs = EditorInterface::get_singleton()->get_resource_filesystem();
#else
    auto *efs = EditorInterface::get_singleton()->get_resource_file_system();
#endif
    for (int i = 0; i < _import_generated_files.size(); i++) {
        efs->update_file(_import_generated_files[i]);
    }
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    efs->scan_sources();
#else
    efs->scan_changes();
#endif
    _import_dst_dirs.clear();
    _import_generated_files.clear();
    _import_dialog = nullptr;
    _is_importing = false;
    set_process(false);

    emit_signal("import_finished");
}

Dictionary SSImporter::_load_source_map() const {
    Ref<EditorSettings> es = EditorInterface::get_singleton()->get_editor_settings();
    return es->get_project_metadata("spritestudio", "ssab_sources", Dictionary());
}

void SSImporter::_save_source_map(const Dictionary &p_map) {
    Ref<EditorSettings> es = EditorInterface::get_singleton()->get_editor_settings();
    es->set_project_metadata("spritestudio", "ssab_sources", p_map);
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
                p_map[output_path] = p_sspj_path;
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
        return map[p_ssab_path];
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
        String sspj = map[ssab_path];
        if (sspj == p_sspj_path) {
            return ssab_path.get_base_dir();
        }
    }
    return String();
}

void SSImporter::_enqueue_one(const String &p_sspj_path, const String &p_dst_dir) {
    String global_dst_dir = ProjectSettings::get_singleton()->globalize_path(p_dst_dir);
    String global_src_file_path = ProjectSettings::get_singleton()->globalize_path(p_sspj_path);
    void *ctx = _process_file(global_src_file_path, global_dst_dir);
    print_line("SSImporter: convert sspj file: " + p_sspj_path + ", to ssab files: " + p_dst_dir);
    _import_contexts.push_back(ctx);
    _import_dst_dirs.push_back(p_dst_dir);
    _import_src_files.push_back(global_src_file_path);
}

void SSImporter::_start_session(const String &p_dialog_title) {
    _session_title = p_dialog_title;
    _import_dialog = memnew(SSProgressDialog);
    EditorInterface::get_singleton()->get_base_control()->add_child(_import_dialog);
    _import_dialog->show_progress(p_dialog_title, _import_contexts.size());

    _import_finished_contexts.resize(_import_contexts.size());
    _import_prev_num = 0;
    _is_importing = true;
    _import_dialog->step(vformat("%s %d/%d", _session_title, 0, _import_finished_contexts.size()), 0);

    set_process(true);

    emit_signal("import_started");
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
void SSImporter::queue_import(const PackedStringArray &p_sspj_files, const String &p_output_dir) {
#else
void SSImporter::queue_import(const Vector<String> &p_sspj_files, const String &p_output_dir) {
#endif
    if (_is_importing) {
        print_line("SSImporter: Already importing. Please wait.");
        return;
    }
    if (p_sspj_files.is_empty()) {
        return;
    }

    Ref<DirAccess> da = DirAccess::open("res://");
    if (!da->dir_exists(p_output_dir)) {
        da->make_dir_recursive(p_output_dir);
    }

    for (int i = 0; i < p_sspj_files.size(); i++) {
        String src_file_path = p_sspj_files[i];
        String src_stem = src_file_path.get_file().get_basename();
        String dst_dir = p_output_dir.path_join(src_stem);
        _enqueue_one(src_file_path, dst_dir);
    }

    _start_session("Importing SSPJ:");
}

void SSImporter::queue_reconvert(const PackedStringArray &p_sspj_files, const PackedStringArray &p_dst_dirs) {
    if (_is_importing) {
        print_line("SSImporter: Already importing. Please wait.");
        return;
    }
    if (p_sspj_files.is_empty() || p_sspj_files.size() != p_dst_dirs.size()) {
        return;
    }

    Ref<DirAccess> da = DirAccess::open("res://");
    for (int i = 0; i < p_dst_dirs.size(); i++) {
        if (!da->dir_exists(p_dst_dirs[i])) {
            da->make_dir_recursive(p_dst_dirs[i]);
        }
    }

    for (int i = 0; i < p_sspj_files.size(); i++) {
        _enqueue_one(p_sspj_files[i], p_dst_dirs[i]);
    }
    _start_session("Reconverting SSPJ:");
}

void SSImporter::record_ssab_source(const String &p_ssab_path, const String &p_sspj_path) {
    if (p_ssab_path.is_empty() || p_sspj_path.is_empty()) {
        return;
    }
    Dictionary map = _load_source_map();
    map.erase(p_ssab_path);
    map[p_ssab_path] = p_sspj_path;
    _evict_lru(map);
    _save_source_map(map);
}

#endif // #ifdef TOOLS_ENABLED
