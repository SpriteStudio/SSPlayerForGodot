#ifdef TOOLS_ENABLED

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/window.hpp>
using namespace godot;
#else
#include "core/version.h"
#include "core/io/dir_access.h"
#include "editor/editor_interface.h"
#if (VERSION_MAJOR >= 4 && VERSION_MINOR >= 5)
#include "editor/file_system/editor_file_system.h"
#else
#include "editor/editor_file_system.h"
#endif
#include "scene/main/window.h"
#endif

#include "gd_clickable_label.h"
#include "gd_ss_import_dock.h"
#include "gd_progress_dialog.h"
#include "ssconverter.h"

void GdSsImportControl::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_window_files_dropped", "files"), &GdSsImportControl::_on_window_files_dropped);
    ClassDB::bind_method(D_METHOD("_on_line_edit_submitted", "text"), &GdSsImportControl::_on_line_edit_submitted);
    ClassDB::bind_method(D_METHOD("_on_browse_button_pressed"), &GdSsImportControl::_on_browse_button_pressed);
    ClassDB::bind_method(D_METHOD("_on_reset_button_pressed"), &GdSsImportControl::_on_reset_button_pressed);
    ClassDB::bind_method(D_METHOD("_on_dir_selected"), &GdSsImportControl::_on_dir_selected);
}


GdSsImportControl::GdSsImportControl() {
    set_h_size_flags(Control::SIZE_EXPAND_FILL);
    set_v_size_flags(Control::SIZE_EXPAND_FILL);

    HBoxContainer *hbox = memnew(HBoxContainer);
    add_child(hbox);
    Label *label = memnew(Label);
    label->set_text("converter version:");
    hbox->add_child(label);

    GdClickableLabel *clickable_label = memnew(GdClickableLabel);
    const char *v = ss_converter_version();
    String version = String(v);
    clickable_label->set_text(version);
    ss_converter_version_free((char*)v);
    v = nullptr;
    hbox->add_child(clickable_label);
    
    hbox = memnew(HBoxContainer);
    add_child(hbox);    
    
    label = memnew(Label);
    label->set_text("Output Dir:");
    hbox->add_child(label);

    path_line_edit = memnew(LineEdit);
    path_line_edit->set_h_size_flags(SIZE_EXPAND_FILL);
    path_line_edit->set_editable(true);
    path_line_edit->connect("text_submitted", Callable(this, "_on_line_edit_submitted"));
    hbox->add_child(path_line_edit);

    browse_button = memnew(Button);
    browse_button->set_text("...");
    browse_button->set_tooltip_text("open EditorFileDialog"); 
    browse_button->connect("pressed", Callable(this, "_on_browse_button_pressed"));
    hbox->add_child(browse_button);    

    reset_button = memnew(Button);
    reset_button->set_text(L"⟲");
    reset_button->set_tooltip_text("Reset to default directory"); 
    reset_button->connect("pressed", callable_mp(this, &GdSsImportControl::_on_reset_button_pressed));
    hbox->add_child(reset_button);

    file_dialog = memnew(EditorFileDialog);
    file_dialog->set_access(EditorFileDialog::ACCESS_RESOURCES);
    file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_DIR);
    file_dialog->connect("dir_selected", Callable(this, "_on_dir_selected"));
    add_child(file_dialog);

    background_panel = memnew(Panel);
    background_panel->set_anchors_preset(Control::PRESET_FULL_RECT);
    background_panel->set_mouse_filter(Control::MOUSE_FILTER_IGNORE); 
    add_child(background_panel);

    instruction_label = memnew(Label);
    instruction_label->set_text(L"Drop a sspj file here\n from out of the godot project");
    instruction_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    instruction_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
    instruction_label->set_anchors_preset(Control::PRESET_FULL_RECT);
    instruction_label->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
    add_child(instruction_label);
      
    _load_settings();
}

GdSsImportControl::~GdSsImportControl() {
    stop_intercepting();

}

void GdSsImportControl::_notification(int p_what) {
    switch(p_what) {
        case NOTIFICATION_ENTER_TREE: {
            start_intercepting();
        } break;
        case NOTIFICATION_EXIT_TREE: {
            stop_intercepting();
        } break;
    }
}

void* GdSsImportControl::process_file(const String &source_sspj_path, const String &dst_dir_path) {
    auto ctx = ss_converter_create();
    ss_converter_convert(ctx, source_sspj_path.utf8().get_data(), dst_dir_path.utf8().get_data());
    return ctx;
}

void GdSsImportControl::start_intercepting() {
    if (is_intercepting) return;

    auto window = get_window();
    if (!window) return;

#ifdef SPRITESTUDIO_GODOT_EXTENSION 
    TypedArray<Dictionary> connections = window->get_signal_connection_list("files_dropped");
#else
    List<Connection> connections;
    window->get_signal_connection_list("files_dropped", &connections);
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION 
    for (int i = 0; i < connections.size(); i++) {
        Dictionary conn = connections[i];
        Callable target = conn["callable"];
#else
    for (const Connection &conn : connections) {
        Callable target = conn.callable;
#endif

        if (target.get_object() == this) continue;

        original_drop_handler = target;
        window->disconnect("files_dropped", original_drop_handler);
        
        break;
    }

    if (!window->is_connected("files_dropped", Callable(this, "_on_window_files_dropped"))) {
        window->connect("files_dropped", Callable(this, "_on_window_files_dropped"));
    }

    is_intercepting = true;
}

void GdSsImportControl::stop_intercepting() {
    if (!is_intercepting) return;

    auto window = get_window();
    if (!window) return;

    if (window->is_connected("files_dropped", Callable(this, "_on_window_files_dropped"))) {
        window->disconnect("files_dropped", Callable(this, "_on_window_files_dropped"));
    }

    if (original_drop_handler.is_valid()) {
        if (!window->is_connected("files_dropped", original_drop_handler)) {
            window->connect("files_dropped", original_drop_handler);
        }
    }

    is_intercepting = false;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
void GdSsImportControl::_on_window_files_dropped(const PackedStringArray &p_files) {
#else
void GdSsImportControl::_on_window_files_dropped(const Vector<String> &p_files) {
#endif
    if (is_reemitting) return;

    if (!is_visible_in_tree()) {
        _perform_default_drop_logic(p_files);
        return;
    }

    if (get_global_rect().has_point(get_global_mouse_position())) {
        
        // print_line("GdSsImportControl: Processing custom file drop...");

        // validate sspj file
#ifdef SPRITESTUDIO_GODOT_EXTENSION
        PackedStringArray sspj_files;
#else
        Vector<String> sspj_files;
#endif
        for (int i = 0; i < p_files.size(); i++) {
            String file_path = p_files[i];
            String ext = file_path.get_extension();
            if (ext == "sspj") {
                sspj_files.push_back(file_path);
            }
        }
        if (sspj_files.is_empty()) {
            print_line("GdSsImportControl: sspj files not found.");
            return;
        }

        String output_dir = path_line_edit->get_text();
        
        Ref<DirAccess> da = DirAccess::open("res://");
        if (!da->dir_exists(output_dir)) {
            //UtilityFunctions::printerr("Output directory does not exist, using default: " + output_dir);
            da->make_dir_recursive(output_dir);
        }

        Vector<void*> contexts;
        for (int i = 0; i < sspj_files.size(); i++) {
            String src_file_path = sspj_files[i];
            String src_file = src_file_path.get_file();
            String src_stem = src_file.get_basename();
            String dst_dir = output_dir.path_join(src_stem);
            String global_dst_dir = ProjectSettings::get_singleton()->globalize_path(dst_dir);
            void *ctx = process_file(src_file_path, global_dst_dir);
            print_line("GdSsImportControl: convert sspj file: " + src_file_path + ", to ssab files: " + dst_dir);
            contexts.push_back(ctx);
        }

        auto dialog = memnew(GdProgressDialog);
        EditorInterface::get_singleton()->get_base_control()->add_child(dialog);
        dialog->show_progress("Importing SSPJ...", contexts.size());

        Vector<bool> finished_contexts;
        finished_contexts.resize(contexts.size());
        bool wait_for_finish = true;
        while(wait_for_finish) {
            wait_for_finish = false;

            for (size_t i = 0; i < contexts.size(); ++i) {
                void* ctx = contexts[i];
                bool ret = ss_converter_is_finished((Context *)ctx);
                if (!ret) {
                    wait_for_finish = true;
                } else {
                    finished_contexts.set(i, true);
                }
            }
            int finished_num = 0;
            for (size_t i = 0; i < finished_contexts.size(); ++i) {
                if (finished_contexts[i]) {
                    finished_num++;
                }
            }
            dialog->step(vformat("Importing SSPJ: %d/%d", finished_num, finished_contexts.size()), finished_num);
        }
        dialog->finish();
        for (size_t i = 0; i < contexts.size(); ++i) {
            void* ctx = contexts[i];
            ss_converter_destroy((Context*)ctx);
        }
        
#ifdef SPRITESTUDIO_GODOT_EXTENSION
        EditorInterface::get_singleton()->get_resource_filesystem()->scan();
#else
        EditorInterface::get_singleton()->get_resource_file_system()->scan();
#endif
    } else {
        _perform_default_drop_logic(p_files);
    }
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
void GdSsImportControl::_perform_default_drop_logic(const PackedStringArray &p_files) {

#else
void GdSsImportControl::_perform_default_drop_logic(const Vector<String> &p_files) {
#endif
    Window *window = get_window();
    if (!window || !original_drop_handler.is_valid()) return;

    is_reemitting = true;

    if (!window->is_connected("files_dropped", original_drop_handler)) {
        window->connect("files_dropped", original_drop_handler);
    }

    window->emit_signal("files_dropped", p_files);

    if (window->is_connected("files_dropped", original_drop_handler)) {
        window->disconnect("files_dropped", original_drop_handler);
    }

    is_reemitting = false;
}

void GdSsImportControl::_on_line_edit_submitted(const String& p_path) {
    _save_settings();
}

void GdSsImportControl::_on_browse_button_pressed() {
    auto p = path_line_edit->get_text();
    Ref<DirAccess> da = DirAccess::open("res://");
    String dir;
    if (da->dir_exists(p)) {
        dir = p;
    } else {
        dir = "res://";
    }

    file_dialog->set_current_dir(dir);
    file_dialog->popup_file_dialog();
}

void GdSsImportControl::_on_reset_button_pressed() {
    path_line_edit->set_text(DEFAULT_PATH);
    _save_settings();
}

void GdSsImportControl::_on_dir_selected(const String &p_path) {
    path_line_edit->set_text(p_path);
    _save_settings();
}

void GdSsImportControl::_load_settings() {
    ProjectSettings *ps = ProjectSettings::get_singleton();
    String path = DEFAULT_PATH;

    if (ps->has_setting(SETTING_KEY)) {
        path = ps->get_setting(SETTING_KEY);
    } else {
        ps->set_setting(SETTING_KEY, DEFAULT_PATH);
    }
    
    path_line_edit->set_text(path);
}

void GdSsImportControl::_save_settings() {
    ProjectSettings *ps = ProjectSettings::get_singleton();
    ps->set_setting(SETTING_KEY, path_line_edit->get_text());
    ps->save();
}

#endif // #ifdef TOOLS_ENABLED
