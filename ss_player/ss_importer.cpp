#ifdef TOOLS_ENABLED

#include "ss_importer.h"

#include "ss_macros.h"
#include "ss_progress_dialog.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
using namespace godot;
#else
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "editor/editor_interface.h"
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
                _import_dialog->step(vformat("Importing SSPJ: %d/%d", finished_num, _import_finished_contexts.size()), finished_num);
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
    for (size_t i = 0; i < _import_contexts.size(); ++i) {
        void *ctx = _import_contexts[i];
        ss_converter_destroy((Context *)ctx);
    }
    _import_contexts.clear();
    _import_finished_contexts.clear();

#if defined(SPRITESTUDIO_GODOT_EXTENSION) || (VERSION_MAJOR >= 4 && VERSION_MINOR >= 6)
    for (int i = 0; i < _import_dst_dirs.size(); i++) {
        EditorInterface::get_singleton()->get_resource_filesystem()->update_file(_import_dst_dirs[i]);
    }
    EditorInterface::get_singleton()->get_resource_filesystem()->scan();
#else
    for (int i = 0; i < _import_dst_dirs.size(); i++) {
        EditorInterface::get_singleton()->get_resource_file_system()->update_file(_import_dst_dirs[i]);
    }
    EditorInterface::get_singleton()->get_resource_file_system()->scan();
#endif
    _import_dst_dirs.clear();
    _import_dialog = nullptr;
    _is_importing = false;
    set_process(false);

    emit_signal("import_finished");
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
        String src_file = src_file_path.get_file();
        String src_stem = src_file.get_basename();
        String dst_dir = p_output_dir.path_join(src_stem);
        String global_dst_dir = ProjectSettings::get_singleton()->globalize_path(dst_dir);
        String global_src_file_path = ProjectSettings::get_singleton()->globalize_path(src_file_path);
        void *ctx = _process_file(global_src_file_path, global_dst_dir);
        print_line("SSImporter: convert sspj file: " + src_file_path + ", to ssab files: " + dst_dir);
        _import_contexts.push_back(ctx);
        _import_dst_dirs.push_back(dst_dir);
    }

    _import_dialog = memnew(SSProgressDialog);
    EditorInterface::get_singleton()->get_base_control()->add_child(_import_dialog);
    _import_dialog->show_progress("Importing SSPJ...", _import_contexts.size());

    _import_finished_contexts.resize(_import_contexts.size());
    _import_prev_num = 0;
    _is_importing = true;
    _import_dialog->step(vformat("Importing SSPJ: %d/%d", 0, _import_finished_contexts.size()), 0);

    set_process(true);

    emit_signal("import_started");
}

#endif // #ifdef TOOLS_ENABLED
