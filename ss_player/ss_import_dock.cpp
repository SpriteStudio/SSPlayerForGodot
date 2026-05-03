#ifdef TOOLS_ENABLED

#include "ss_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/window.hpp>
using namespace godot;
#else
#include "core/io/dir_access.h"
#include "editor/editor_interface.h"
#include "editor/settings/editor_settings.h"
#if VERSION_MAJOR >= 4
    #if VERSION_MINOR >= 5
    #include "editor/file_system/editor_file_system.h"
    #else
    #include "editor/editor_file_system.h"
    #endif
#endif
#include "scene/main/window.h"
#endif

#include "ss_clickable_label.h"
#include "ss_import_dock.h"
#include "ss_progress_dialog.h"
#include "ssconverter.h"

void SSImportControl::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_window_files_dropped", "files"), &SSImportControl::_on_window_files_dropped);
    ClassDB::bind_method(D_METHOD("_on_line_edit_submitted", "text"), &SSImportControl::_on_line_edit_submitted);
    ClassDB::bind_method(D_METHOD("_on_browse_button_pressed"), &SSImportControl::_on_browse_button_pressed);
    ClassDB::bind_method(D_METHOD("_on_reset_button_pressed"), &SSImportControl::_on_reset_button_pressed);
    ClassDB::bind_method(D_METHOD("_on_dir_selected"), &SSImportControl::_on_dir_selected);
    ClassDB::bind_method(D_METHOD("_on_recent_file_pressed", "path"), &SSImportControl::_on_recent_file_pressed);
    ClassDB::bind_method(D_METHOD("_on_clear_history_pressed"), &SSImportControl::_on_clear_history_pressed);
}


SSImportControl::SSImportControl() {
    set_h_size_flags(Control::SIZE_EXPAND_FILL);
    set_v_size_flags(Control::SIZE_EXPAND_FILL);

    HBoxContainer *hbox = memnew(HBoxContainer);
    add_child(hbox);
    Label *label = memnew(Label);
    label->set_text("converter version:");
    hbox->add_child(label);

    SSClickableLabel *clickable_label = memnew(SSClickableLabel);
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
    reset_button->connect("pressed", callable_mp(this, &SSImportControl::_on_reset_button_pressed));
    hbox->add_child(reset_button);

    file_dialog = memnew(EditorFileDialog);
    file_dialog->set_access(EditorFileDialog::ACCESS_RESOURCES);
    file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_DIR);
    file_dialog->connect("dir_selected", Callable(this, "_on_dir_selected"));
    add_child(file_dialog);

    // Recent files section
    HBoxContainer *recent_hbox = memnew(HBoxContainer);
    add_child(recent_hbox);

    recent_label = memnew(Label);
    recent_label->set_text("Recent SSPJs:");
    recent_label->set_h_size_flags(SIZE_EXPAND_FILL);
    recent_hbox->add_child(recent_label);

    Button *clear_btn = memnew(Button);
    clear_btn->set_text(L"🗑");
    clear_btn->set_tooltip_text("Clear import history");
    clear_btn->connect("pressed", Callable(this, "_on_clear_history_pressed"));
    recent_hbox->add_child(clear_btn);

    ScrollContainer *scroll = memnew(ScrollContainer);
    scroll->set_custom_minimum_size(Size2(0, 100));
    scroll->set_h_size_flags(SIZE_EXPAND_FILL);
    add_child(scroll);

    recent_vbox = memnew(VBoxContainer);
    recent_vbox->set_h_size_flags(SIZE_EXPAND_FILL);
    scroll->add_child(recent_vbox);

    background_panel = memnew(Panel);
    background_panel->set_v_size_flags(Control::SIZE_EXPAND_FILL);
    background_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
    background_panel->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
    
    Ref<StyleBoxFlat> panel_style = memnew(StyleBoxFlat);
    panel_style->set_bg_color(Color(0.2, 0.2, 0.25, 0.6));
    panel_style->set_border_width_all(2);
    panel_style->set_border_color(Color(0.4, 0.4, 0.5, 0.8));
    panel_style->set_corner_radius_all(6);
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    background_panel->add_theme_stylebox_override("panel", panel_style);
#else
    background_panel->add_theme_style_override("panel", panel_style);
#endif
    
    add_child(background_panel);

    instruction_label = memnew(Label);
    instruction_label->set_text(L"Drop a sspj file here\n from out of the godot project");
    instruction_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    instruction_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
    instruction_label->set_anchors_preset(Control::PRESET_FULL_RECT);
    instruction_label->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
    background_panel->add_child(instruction_label);

    _load_settings();
}

SSImportControl::~SSImportControl() {
    stop_intercepting();

}

void SSImportControl::_notification(int p_what) {
    switch(p_what) {
        case NOTIFICATION_ENTER_TREE: {
            start_intercepting();
        } break;
        case NOTIFICATION_EXIT_TREE: {
            stop_intercepting();
        } break;
        case NOTIFICATION_PROCESS: {
            if (is_importing) {
                bool wait_for_finish = false;

                for (size_t i = 0; i < import_contexts.size(); ++i) {
                    void* ctx = import_contexts[i];
                    bool ret = ss_converter_is_finished((Context *)ctx);
                    if (!ret) {
                        wait_for_finish = true;
                    } else {
                        import_finished_contexts.set(i, true);
                    }
                }
                
                int finished_num = 0;
                for (size_t i = 0; i < import_finished_contexts.size(); ++i) {
                    if (import_finished_contexts[i]) {
                        finished_num++;
                    }
                }
                
                if (finished_num != import_prev_num) {
                    import_dialog->step(vformat("Importing SSPJ: %d/%d", finished_num, import_finished_contexts.size()), finished_num);
                    import_prev_num = finished_num;
                }

                if (!wait_for_finish) {
                    import_dialog->finish();
                    for (size_t i = 0; i < import_contexts.size(); ++i) {
                        void* ctx = import_contexts[i];
                        ss_converter_destroy((Context*)ctx);
                    }
                    import_contexts.clear();
                    import_finished_contexts.clear();

#if defined(SPRITESTUDIO_GODOT_EXTENSION) || (VERSION_MAJOR >= 4 && VERSION_MINOR >= 6)
                    for (int i = 0; i < import_dst_dirs.size(); i++) {
                        EditorInterface::get_singleton()->get_resource_filesystem()->update_file(import_dst_dirs[i]);
                    }
                    EditorInterface::get_singleton()->get_resource_filesystem()->scan();
#else
                    for (int i = 0; i < import_dst_dirs.size(); i++) {
                        EditorInterface::get_singleton()->get_resource_file_system()->update_file(import_dst_dirs[i]);
                    }
                    EditorInterface::get_singleton()->get_resource_file_system()->scan();
#endif
                    import_dst_dirs.clear();
                    import_dialog = nullptr;
                    is_importing = false;
                    set_process(false);
                }
            }
        } break;
    }
}

void* SSImportControl::process_file(const String &source_sspj_path, const String &dst_dir_path) {
    auto ctx = ss_converter_create();
    
    // Keep CharString alive until the end of this function call
    CharString src_utf8 = source_sspj_path.utf8();
    CharString dst_utf8 = dst_dir_path.utf8();
    
    ss_converter_convert(ctx, src_utf8.get_data(), dst_utf8.get_data(), [](const char *msg){
        print_line(String::utf8(msg));
    });

    return ctx;
}

void SSImportControl::start_intercepting() {
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

void SSImportControl::stop_intercepting() {
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
void SSImportControl::_on_window_files_dropped(const PackedStringArray &p_files) {
#else
void SSImportControl::_on_window_files_dropped(const Vector<String> &p_files) {
#endif
    if (is_reemitting) return;

    if (!is_visible_in_tree()) {
        _perform_default_drop_logic(p_files);
        return;
    }

    if (get_global_rect().has_point(get_global_mouse_position())) {

        if (is_importing) {
            print_line("SSImportControl: Already importing. Please wait.");
            return;
        }

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
            print_line("SSImportControl: sspj files not found.");
            return;
        }

        _start_import(sspj_files);
    } else {
        _perform_default_drop_logic(p_files);
    }
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
void SSImportControl::_start_import(const PackedStringArray &p_sspj_files) {
#else
void SSImportControl::_start_import(const Vector<String> &p_sspj_files) {
#endif
    String output_dir = path_line_edit->get_text();

    Ref<DirAccess> da = DirAccess::open("res://");
    if (!da->dir_exists(output_dir)) {
        da->make_dir_recursive(output_dir);
    }

    for (int i = 0; i < p_sspj_files.size(); i++) {
        String src_file_path = p_sspj_files[i];
        String src_file = src_file_path.get_file();
        String src_stem = src_file.get_basename();
        String dst_dir = output_dir.path_join(src_stem);
        String global_dst_dir = ProjectSettings::get_singleton()->globalize_path(dst_dir);
        String global_src_file_path = ProjectSettings::get_singleton()->globalize_path(src_file_path);
        void *ctx = process_file(global_src_file_path, global_dst_dir);
        print_line("SSImportControl: convert sspj file: " + src_file_path + ", to ssab files: " + dst_dir);
        import_contexts.push_back(ctx);
        import_dst_dirs.push_back(dst_dir);

        _add_to_recent_files(src_file_path);
    }

    import_dialog = memnew(SSProgressDialog);
    EditorInterface::get_singleton()->get_base_control()->add_child(import_dialog);
    import_dialog->show_progress("Importing SSPJ...", import_contexts.size());

    import_finished_contexts.resize(import_contexts.size());
    import_prev_num = 0;
    is_importing = true;
    import_dialog->step(vformat("Importing SSPJ: %d/%d", 0, import_finished_contexts.size()), 0);

    set_process(true);
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
void SSImportControl::_perform_default_drop_logic(const PackedStringArray &p_files) {

#else
void SSImportControl::_perform_default_drop_logic(const Vector<String> &p_files) {
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

void SSImportControl::_on_line_edit_submitted(const String& p_path) {
    _save_settings();
}

void SSImportControl::_on_browse_button_pressed() {
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

void SSImportControl::_on_reset_button_pressed() {
    path_line_edit->set_text(DEFAULT_PATH);
    _save_settings();
}

void SSImportControl::_on_dir_selected(const String &p_path) {
    path_line_edit->set_text(p_path);
    _save_settings();
}

void SSImportControl::_on_recent_file_pressed(const String &p_path) {
    if (is_importing) {
        print_line("SSImportControl: Already importing. Please wait.");
        return;
    }
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    PackedStringArray files;
#else
    Vector<String> files;
#endif
    files.push_back(p_path);
    _start_import(files);
}

void SSImportControl::_on_clear_history_pressed() {
    EditorInterface::get_singleton()->get_editor_settings()->set_project_metadata("spritestudio", "recent_files", PackedStringArray());
    _update_recent_files_ui();
}

void SSImportControl::_update_recent_files_ui() {
    // Clear existing buttons
    while (recent_vbox->get_child_count() > 0) {
        Node *child = recent_vbox->get_child(0);
        recent_vbox->remove_child(child);
        child->queue_free();
    }

    PackedStringArray recent_files = EditorInterface::get_singleton()->get_editor_settings()->get_project_metadata("spritestudio", "recent_files", PackedStringArray());

    if (recent_files.is_empty()) {
        Label *empty_label = memnew(Label);
        empty_label->set_text("No recent files.");
        empty_label->set_modulate(Color(1, 1, 1, 0.5));
        recent_vbox->add_child(empty_label);
    } else {
        for (int i = 0; i < recent_files.size(); i++) {
            String path = recent_files[i];
            Button *btn = memnew(Button);
            btn->set_text(path.get_file());
            btn->set_tooltip_text(path);
            btn->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
            btn->connect("pressed", callable_mp(this, &SSImportControl::_on_recent_file_pressed).bind(path));
            recent_vbox->add_child(btn);
        }
    }
}

void SSImportControl::_add_to_recent_files(const String &p_path) {
    Ref<EditorSettings> es = EditorInterface::get_singleton()->get_editor_settings();
    PackedStringArray recent_files = es->get_project_metadata("spritestudio", "recent_files", PackedStringArray());

    // Remove if already exists to move to top
    for (int i = 0; i < recent_files.size(); i++) {
        if (recent_files[i] == p_path) {
            recent_files.remove_at(i);
            break;
        }
    }

    recent_files.insert(0, p_path);

    // Keep only last 5
    if (recent_files.size() > 5) {
        recent_files.resize(5);
    }

    es->set_project_metadata("spritestudio", "recent_files", recent_files);

    _update_recent_files_ui();
}

void SSImportControl::_load_settings() {
    ProjectSettings *ps = ProjectSettings::get_singleton();
    String path = DEFAULT_PATH;

    if (ps->has_setting(SETTING_KEY)) {
        path = ps->get_setting(SETTING_KEY);
    } else {
        ps->set_setting(SETTING_KEY, DEFAULT_PATH);
    }

    path_line_edit->set_text(path);

    _update_recent_files_ui();
}

void SSImportControl::_save_settings() {
    ProjectSettings *ps = ProjectSettings::get_singleton();
    ps->set_setting(SETTING_KEY, path_line_edit->get_text());
    ps->save();
}

#endif // #ifdef TOOLS_ENABLED
