#ifdef TOOLS_ENABLED

#include "ss_filesystem_menu.h"

#include "ss_importer.h"
#include "ss_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
using namespace godot;
#else
#include "core/config/project_settings.h"
#include "core/os/os.h"
#include "core/templates/hash_set.h"
#include "editor/editor_interface.h"
#include "scene/resources/texture.h"
#endif

void SSFileSystemContextMenu::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_open_in_editor", "paths"), &SSFileSystemContextMenu::_on_open_in_editor);
    ClassDB::bind_method(D_METHOD("_on_convert", "paths"), &SSFileSystemContextMenu::_on_convert);
    ClassDB::bind_method(D_METHOD("_on_sspj_file_selected", "path"), &SSFileSystemContextMenu::_on_sspj_file_selected);
}

SSFileSystemContextMenu::SSFileSystemContextMenu() {
}

SSFileSystemContextMenu::~SSFileSystemContextMenu() {
    if (file_dialog) {
        file_dialog->queue_free();
        file_dialog = nullptr;
    }
}

bool SSFileSystemContextMenu::_is_unsupported_for_editor() const {
    String os_name = OS::get_singleton()->get_name();
    return os_name == "Linux";
}

static bool _is_supported_output(const String &p_path) {
    String ext = p_path.get_extension();
    return ext == "ssab" || ext == "ssqb";
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
void SSFileSystemContextMenu::_popup_menu(const PackedStringArray &p_paths) {
#else
void SSFileSystemContextMenu::get_options(const Vector<String> &p_paths) {
#endif
    bool has_supported = false;
    for (int i = 0; i < p_paths.size(); i++) {
        if (_is_supported_output(p_paths[i])) {
            has_supported = true;
            break;
        }
    }
    if (!has_supported) {
        return;
    }

    if (!_is_unsupported_for_editor()) {
        add_context_menu_item("Open Source SSPJ", Callable(this, "_on_open_in_editor"), Ref<Texture2D>());
    }
    add_context_menu_item("Reconvert from SSPJ", Callable(this, "_on_convert"), Ref<Texture2D>());
}

void SSFileSystemContextMenu::_on_open_in_editor(const PackedStringArray &p_paths) {
    if (!importer) {
        print_line("SSFileSystemContextMenu: importer not set.");
        return;
    }

    // Dedupe by sspj.
    HashSet<String> opened;
    String pending_ssab_for_dialog;
    int pending_count = 0;

    for (int i = 0; i < p_paths.size(); i++) {
        String path = p_paths[i];
        if (!_is_supported_output(path)) {
            continue;
        }
        String sspj = importer->lookup_sspj_for_ssab(path);
        if (sspj.is_empty()) {
            pending_ssab_for_dialog = path;
            pending_count++;
            continue;
        }
        if (opened.has(sspj)) {
            continue;
        }
        opened.insert(sspj);
        _do_open_in_editor(sspj);
    }

    if (pending_count == 1) {
        _ask_user_for_sspj(pending_ssab_for_dialog, ACTION_OPEN_IN_EDITOR);
    } else if (pending_count > 1) {
        print_line(vformat("SSFileSystemContextMenu: %d file(s) without source record skipped. Right-click each individually to set their sspj.", pending_count));
    }
}

void SSFileSystemContextMenu::_on_convert(const PackedStringArray &p_paths) {
    if (!importer) {
        print_line("SSFileSystemContextMenu: importer not set.");
        return;
    }
    if (importer->is_importing()) {
        print_line("SSFileSystemContextMenu: Already importing. Please wait.");
        return;
    }

    // Dedupe by dst_dir. Each ssab implies one (dst_dir, sspj) reconvert job.
    HashSet<String> seen_dst_dirs;
    PackedStringArray sspjs;
    PackedStringArray dst_dirs;
    String pending_ssab_for_dialog;
    int pending_count = 0;

    for (int i = 0; i < p_paths.size(); i++) {
        String path = p_paths[i];
        if (!_is_supported_output(path)) {
            continue;
        }
        String sspj = importer->lookup_sspj_for_ssab(path);
        if (sspj.is_empty()) {
            pending_ssab_for_dialog = path;
            pending_count++;
            continue;
        }
        String dst_dir = path.get_base_dir();
        if (seen_dst_dirs.has(dst_dir)) {
            continue;
        }
        seen_dst_dirs.insert(dst_dir);
        sspjs.push_back(sspj);
        dst_dirs.push_back(dst_dir);
    }

    if (!sspjs.is_empty()) {
        importer->queue_reconvert(sspjs, dst_dirs);
    }

    if (pending_count == 1 && sspjs.is_empty()) {
        _ask_user_for_sspj(pending_ssab_for_dialog, ACTION_CONVERT);
    } else if (pending_count > 0) {
        print_line(vformat("SSFileSystemContextMenu: %d file(s) without source record skipped. Right-click each individually to set their sspj.", pending_count));
    }
}

void SSFileSystemContextMenu::_do_open_in_editor(const String &p_sspj_path) {
    Error err = OS::get_singleton()->shell_open(p_sspj_path);
    if (err != OK) {
        print_line(vformat("SSFileSystemContextMenu: failed to open sspj (%s) via OS shell. error=%d", p_sspj_path, (int)err));
    }
}

void SSFileSystemContextMenu::_do_convert(const String &p_ssab_path, const String &p_sspj_path) {
    String dst_dir = p_ssab_path.get_base_dir();
    PackedStringArray sspjs;
    PackedStringArray dst_dirs;
    sspjs.push_back(p_sspj_path);
    dst_dirs.push_back(dst_dir);
    importer->queue_reconvert(sspjs, dst_dirs);
}

void SSFileSystemContextMenu::_ensure_file_dialog() {
    if (file_dialog) {
        return;
    }
    file_dialog = memnew(EditorFileDialog);
    file_dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
    file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_FILE);
    file_dialog->clear_filters();
    file_dialog->add_filter("*.sspj", "SpriteStudio Project");
    file_dialog->connect("file_selected", Callable(this, "_on_sspj_file_selected"));
    EditorInterface::get_singleton()->get_base_control()->add_child(file_dialog);
}

void SSFileSystemContextMenu::_ask_user_for_sspj(const String &p_ssab_path, PendingAction p_action) {
    _ensure_file_dialog();
    pending_ssab_path = p_ssab_path;
    pending_action = p_action;
    file_dialog->popup_file_dialog();
}

void SSFileSystemContextMenu::_on_sspj_file_selected(const String &p_sspj_path) {
    if (pending_action == ACTION_NONE || pending_ssab_path.is_empty() || !importer) {
        pending_action = ACTION_NONE;
        pending_ssab_path = String();
        return;
    }

    // Stored sspj paths are always global (matching auto-recorded entries).
    String global_sspj = ProjectSettings::get_singleton()->globalize_path(p_sspj_path);
    importer->record_ssab_source(pending_ssab_path, global_sspj);

    PendingAction action = pending_action;
    String ssab_path = pending_ssab_path;
    pending_action = ACTION_NONE;
    pending_ssab_path = String();

    switch (action) {
        case ACTION_OPEN_IN_EDITOR:
            _do_open_in_editor(global_sspj);
            break;
        case ACTION_CONVERT:
            _do_convert(ssab_path, global_sspj);
            break;
        default:
            break;
    }
}

#endif // #ifdef TOOLS_ENABLED
