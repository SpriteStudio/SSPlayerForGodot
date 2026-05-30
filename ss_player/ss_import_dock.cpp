#ifdef TOOLS_ENABLED

#include "ss_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/accept_dialog.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_settings.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
using namespace godot;
#else
#include "core/input/input_event.h"
#include "core/io/dir_access.h"
#include "core/os/os.h"
#include "editor/editor_interface.h"
#include "editor/settings/editor_settings.h"
#include "scene/gui/dialogs.h"
#include "scene/main/window.h"
#if VERSION_MAJOR >= 4
    #if VERSION_MINOR >= 5
    #include "editor/file_system/editor_file_system.h"
    #else
    #include "editor/editor_file_system.h"
    #endif
#endif
#endif

#include "ss_clickable_label.h"
#include "ss_import_dock.h"
#include "ss_importer.h"
#include "ssconverter.h"

void SSImportControl::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_window_files_dropped", "files"), &SSImportControl::_on_window_files_dropped);
    ClassDB::bind_method(D_METHOD("_on_line_edit_submitted", "text"), &SSImportControl::_on_line_edit_submitted);
    ClassDB::bind_method(D_METHOD("_on_browse_button_pressed"), &SSImportControl::_on_browse_button_pressed);
    ClassDB::bind_method(D_METHOD("_on_reset_button_pressed"), &SSImportControl::_on_reset_button_pressed);
    ClassDB::bind_method(D_METHOD("_on_open_dir_button_pressed"), &SSImportControl::_on_open_dir_button_pressed);
    ClassDB::bind_method(D_METHOD("_on_dir_selected"), &SSImportControl::_on_dir_selected);
    ClassDB::bind_method(D_METHOD("_on_recent_file_pressed", "path"), &SSImportControl::_on_recent_file_pressed);
    ClassDB::bind_method(D_METHOD("_on_recent_gui_input", "event", "path"), &SSImportControl::_on_recent_gui_input);
    ClassDB::bind_method(D_METHOD("_on_recent_menu_id_pressed", "id"), &SSImportControl::_on_recent_menu_id_pressed);
    ClassDB::bind_method(D_METHOD("_on_clear_recent_pressed"), &SSImportControl::_on_clear_recent_pressed);
}


SSImportControl::SSImportControl() {
    set_h_size_flags(Control::SIZE_EXPAND_FILL);
    set_v_size_flags(Control::SIZE_EXPAND_FILL);

    // 2. Output Dir row
    {
        VBoxContainer *output_vbox = memnew(VBoxContainer);
        add_child(output_vbox);

        HBoxContainer *header_hbox = memnew(HBoxContainer);
        output_vbox->add_child(header_hbox);

        Label *label = memnew(Label);
        label->set_text(tr("Output:"));
        header_hbox->add_child(label);

        Control *spacer = memnew(Control);
        spacer->set_h_size_flags(SIZE_EXPAND_FILL);
        header_hbox->add_child(spacer);

        browse_button = memnew(Button);
        browse_button->set_tooltip_text(tr("Choose output directory"));
        browse_button->connect("pressed", Callable(this, "_on_browse_button_pressed"));
        header_hbox->add_child(browse_button);

        reset_button = memnew(Button);
        reset_button->set_tooltip_text(tr("Reset to default directory"));
        reset_button->connect("pressed", callable_mp(this, &SSImportControl::_on_reset_button_pressed));
        header_hbox->add_child(reset_button);

        open_dir_button = memnew(Button);
        open_dir_button->set_tooltip_text(tr("Open in File Manager"));
        open_dir_button->connect("pressed", Callable(this, "_on_open_dir_button_pressed"));
        header_hbox->add_child(open_dir_button);

        path_line_edit = memnew(LineEdit);
        path_line_edit->set_h_size_flags(SIZE_EXPAND_FILL);
        path_line_edit->set_editable(true);
        path_line_edit->connect("text_submitted", Callable(this, "_on_line_edit_submitted"));
        output_vbox->add_child(path_line_edit);
    }

    file_dialog = memnew(EditorFileDialog);
    file_dialog->set_access(EditorFileDialog::ACCESS_RESOURCES);
    file_dialog->set_file_mode(EditorFileDialog::FILE_MODE_OPEN_DIR);
    file_dialog->connect("dir_selected", Callable(this, "_on_dir_selected"));
    add_child(file_dialog);

    // 3. Drop area (compact, fixed height)
    {
        drop_panel = memnew(Panel);
        drop_panel->set_custom_minimum_size(Size2(0, 200));
        drop_panel->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        drop_panel->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);

        Ref<StyleBoxFlat> panel_style = memnew(StyleBoxFlat);
        panel_style->set_bg_color(Color(0.18, 0.20, 0.26, 0.55));
        panel_style->set_border_width_all(2);
        panel_style->set_border_color(Color(0.45, 0.55, 0.75, 0.7));
        panel_style->set_corner_radius_all(8);
#ifdef SPRITESTUDIO_GODOT_EXTENSION
        drop_panel->add_theme_stylebox_override("panel", panel_style);
#else
        drop_panel->add_theme_style_override("panel", panel_style);
#endif
        add_child(drop_panel);

        instruction_label = memnew(Label);
        instruction_label->set_text(tr("Drop SSPJ here\n(drag from your file manager)"));
        instruction_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
        instruction_label->set_vertical_alignment(VERTICAL_ALIGNMENT_CENTER);
        instruction_label->set_anchors_preset(Control::PRESET_FULL_RECT);
        instruction_label->set_mouse_filter(Control::MOUSE_FILTER_IGNORE);
        drop_panel->add_child(instruction_label);
    }

    // 4. Recent SSPJs section
    {
        HBoxContainer *recent_header = memnew(HBoxContainer);
        add_child(recent_header);

        recent_label = memnew(Label);
        recent_label->set_text(tr("Recent SSPJs"));
        recent_label->set_h_size_flags(Control::SIZE_EXPAND_FILL);
        recent_header->add_child(recent_label);

        clear_recent_button = memnew(Button);
        clear_recent_button->set_tooltip_text(tr("Clear recent SSPJ files"));
        clear_recent_button->connect("pressed", Callable(this, "_on_clear_recent_pressed"));
        recent_header->add_child(clear_recent_button);

        ScrollContainer *scroll = memnew(ScrollContainer);
        scroll->set_h_size_flags(SIZE_EXPAND_FILL);
        scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
        add_child(scroll);

        recent_vbox = memnew(VBoxContainer);
        recent_vbox->set_h_size_flags(SIZE_EXPAND_FILL);
        scroll->add_child(recent_vbox);
    }

    // 5. Right-click popup (single shared instance)
    recent_popup = memnew(PopupMenu);
    recent_popup->connect("id_pressed", Callable(this, "_on_recent_menu_id_pressed"));
    add_child(recent_popup);

    // 6. Footer: converter version (moved to bottom)
    {
        HBoxContainer *hbox = memnew(HBoxContainer);
        add_child(hbox);

        Label *label = memnew(Label);
        label->set_text(tr("converter:"));
        hbox->add_child(label);

        SSClickableLabel *clickable_label = memnew(SSClickableLabel);
        const char *v = ss_converter_version();
        clickable_label->set_text(String(v));
        ss_converter_version_free((char *)v);
        v = nullptr;
        hbox->add_child(clickable_label);
    }

    _load_settings();
}

SSImportControl::~SSImportControl() {
    stop_intercepting();
}

void SSImportControl::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_THEME_CHANGED: {
            if (browse_button) browse_button->set_button_icon(get_theme_icon(SNAME("Load"), SNAME("EditorIcons")));
            if (reset_button) reset_button->set_button_icon(get_theme_icon(SNAME("Reload"), SNAME("EditorIcons")));
            if (open_dir_button) open_dir_button->set_button_icon(get_theme_icon(SNAME("Filesystem"), SNAME("EditorIcons")));
            if (clear_recent_button) clear_recent_button->set_button_icon(get_theme_icon(SNAME("Clear"), SNAME("EditorIcons")));
        } break;
        case NOTIFICATION_ENTER_TREE: {
            start_intercepting();
        } break;
        case NOTIFICATION_EXIT_TREE: {
            stop_intercepting();
        } break;
    }
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

        // Note: Disconnecting other plugins' drag-and-drop handlers is a brittle hack to work around Godot's
        // single-handler limitation for OS drag-and-drop. This might conflict if other plugins do the same.
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

        if (importer && importer->is_importing()) {
            WARN_PRINT("SSImportControl: Already importing. Please wait.");
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
            ERR_PRINT("SSImportControl: No .sspj files found.");
            AcceptDialog *dialog = memnew(AcceptDialog);
            dialog->set_title(tr("Import Error"));
            dialog->set_text(tr("No .sspj files found.\nPlease drop SpriteStudio project (.sspj) files."));
            EditorInterface::get_singleton()->get_base_control()->add_child(dialog);
            dialog->connect("confirmed", Callable(dialog, "queue_free"));
            dialog->connect("canceled", Callable(dialog, "queue_free"));
            dialog->popup_centered();
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
    if (!importer) {
        ERR_PRINT("SSImportControl: importer is not set.");
        return;
    }

    String output_dir = path_line_edit->get_text();

    for (int i = 0; i < p_sspj_files.size(); i++) {
        _add_to_recent_files(p_sspj_files[i]);
    }

    importer->queue_import(p_sspj_files, output_dir);
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

void SSImportControl::_on_line_edit_submitted(const String &p_path) {
    _save_settings();
    _ensure_output_dir_exists();
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
    _ensure_output_dir_exists();
}

void SSImportControl::_on_open_dir_button_pressed() {
    String path = path_line_edit->get_text();
    if (path.is_empty()) {
        return;
    }

    String global_path = ProjectSettings::get_singleton()->globalize_path(path);
    Error err = OS::get_singleton()->shell_open(global_path);
    if (err != OK) {
        ERR_PRINT(vformat("SSImportControl: failed to open output directory %s. error=%d", global_path, (int)err));
    }
}

void SSImportControl::_on_dir_selected(const String &p_path) {
    path_line_edit->set_text(p_path);
    _save_settings();
    _ensure_output_dir_exists();
}

void SSImportControl::_on_recent_file_pressed(const String &p_path) {
    if (importer && importer->is_importing()) {
        WARN_PRINT("SSImportControl: Already importing. Please wait.");
        return;
    }
    _reconvert_sspj(p_path);
}

void SSImportControl::_reconvert_sspj(const String &p_sspj_path) {
    if (!importer) {
        return;
    }

    // Move to top of recent list regardless of which path we take below.
    _add_to_recent_files(p_sspj_path);

    // Prefer the original output_dir from the source map (where the existing
    // ssabs already live). Falls back to the LineEdit-driven import flow when
    // we have no record yet (e.g. previous conversion failed).
    String original_dst_dir = importer->lookup_output_dir_for_sspj(p_sspj_path);
    if (!original_dst_dir.is_empty()) {
        PackedStringArray sspjs;
        PackedStringArray dst_dirs;
        sspjs.push_back(p_sspj_path);
        dst_dirs.push_back(original_dst_dir);
        importer->queue_reconvert(sspjs, dst_dirs);
        return;
    }

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    PackedStringArray files;
#else
    Vector<String> files;
#endif
    files.push_back(p_sspj_path);
    String output_dir = path_line_edit->get_text();
    importer->queue_import(files, output_dir);
}

void SSImportControl::_on_recent_gui_input(const Ref<InputEvent> &p_event, const String &p_path) {
    Ref<InputEventMouseButton> mb = p_event;
    if (!mb.is_valid() || !mb->is_pressed()) {
        return;
    }
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    if (mb->get_button_index() == MOUSE_BUTTON_RIGHT) {
#else
    if (mb->get_button_index() == MouseButton::RIGHT) {
#endif
        _show_recent_context_menu(p_path);
    }
}

void SSImportControl::_show_recent_context_menu(const String &p_path) {
    pending_recent_path = p_path;

    recent_popup->clear();
    String os_name = OS::get_singleton()->get_name();
    if (os_name != "Linux") {
        recent_popup->add_item(tr("Open SSPJ"), RECENT_MENU_OPEN_IN_EDITOR);
    }
    recent_popup->add_item(tr("Reconvert"), RECENT_MENU_RECONVERT);
    recent_popup->add_item(tr("Reveal"), RECENT_MENU_REVEAL);
    recent_popup->add_separator();
    recent_popup->add_item(tr("Remove from Recent"), RECENT_MENU_REMOVE);

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    Vector2i mouse = DisplayServer::get_singleton()->mouse_get_position();
#else
    Vector2i mouse = DisplayServer::get_singleton()->mouse_get_position();
#endif
    recent_popup->set_position(mouse);
    recent_popup->popup();
}

void SSImportControl::_on_recent_menu_id_pressed(int p_id) {
    String path = pending_recent_path;
    pending_recent_path = String();
    if (path.is_empty()) {
        return;
    }

    switch (p_id) {
        case RECENT_MENU_OPEN_IN_EDITOR: {
            Error err = OS::get_singleton()->shell_open(path);
            if (err != OK) {
                ERR_PRINT(vformat("SSImportControl: failed to open sspj %s. error=%d", path, (int)err));
            }
        } break;
        case RECENT_MENU_REVEAL: {
            Error err = OS::get_singleton()->shell_show_in_file_manager(path, false);
            if (err != OK) {
                ERR_PRINT(vformat("SSImportControl: failed to reveal %s. error=%d", path, (int)err));
            }
        } break;
        case RECENT_MENU_RECONVERT: {
            if (importer && importer->is_importing()) {
                WARN_PRINT("SSImportControl: Already importing. Please wait.");
                return;
            }
            _reconvert_sspj(path);
        } break;
        case RECENT_MENU_REMOVE: {
            _remove_from_recent_files(path);
        } break;
    }
}

void SSImportControl::_on_clear_recent_pressed() {
    Ref<EditorSettings> es = EditorInterface::get_singleton()->get_editor_settings();
    es->set_project_metadata("spritestudio", "recent_files", PackedStringArray());
    _update_recent_files_ui();
}

void SSImportControl::_remove_from_recent_files(const String &p_path) {
    Ref<EditorSettings> es = EditorInterface::get_singleton()->get_editor_settings();
    PackedStringArray recent_files = es->get_project_metadata("spritestudio", "recent_files", PackedStringArray());
    for (int i = 0; i < recent_files.size(); i++) {
        if (recent_files[i] == p_path) {
            recent_files.remove_at(i);
            break;
        }
    }
    es->set_project_metadata("spritestudio", "recent_files", recent_files);
    _update_recent_files_ui();
}

void SSImportControl::_update_recent_files_ui() {
    while (recent_vbox->get_child_count() > 0) {
        Node *child = recent_vbox->get_child(0);
        recent_vbox->remove_child(child);
        child->queue_free();
    }

    PackedStringArray recent_files = EditorInterface::get_singleton()->get_editor_settings()->get_project_metadata("spritestudio", "recent_files", PackedStringArray());

    if (recent_files.is_empty()) {
        Label *empty_label = memnew(Label);
        empty_label->set_text(tr("No recent files. Drop a sspj above to start."));
        empty_label->set_modulate(Color(1, 1, 1, 0.5));
        empty_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
        recent_vbox->add_child(empty_label);
        return;
    }

    for (int i = 0; i < recent_files.size(); i++) {
        String path = recent_files[i];
        String filename = path.get_file();
        String parent_basename = path.get_base_dir().get_file();
        String label_text = parent_basename.is_empty()
                                ? filename
                                : filename + String::utf8("  ·  ") + parent_basename;

        Button *btn = memnew(Button);
        btn->set_text(label_text);
        btn->set_tooltip_text(path);
        btn->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
        btn->set_clip_text(true);
        btn->connect("pressed", callable_mp(this, &SSImportControl::_on_recent_file_pressed).bind(path));
        btn->connect("gui_input", callable_mp(this, &SSImportControl::_on_recent_gui_input).bind(path));
        recent_vbox->add_child(btn);
    }
}

void SSImportControl::_add_to_recent_files(const String &p_path) {
    Ref<EditorSettings> es = EditorInterface::get_singleton()->get_editor_settings();
    PackedStringArray recent_files = es->get_project_metadata("spritestudio", "recent_files", PackedStringArray());

    for (int i = 0; i < recent_files.size(); i++) {
        if (recent_files[i] == p_path) {
            recent_files.remove_at(i);
            break;
        }
    }

    recent_files.insert(0, p_path);

    if (recent_files.size() > RECENT_FILES_CAP) {
        recent_files.resize(RECENT_FILES_CAP);
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

    _ensure_output_dir_exists();
    _update_recent_files_ui();
}

void SSImportControl::_save_settings() {
    ProjectSettings *ps = ProjectSettings::get_singleton();
    ps->set_setting(SETTING_KEY, path_line_edit->get_text());
    ps->save();
}

void SSImportControl::_ensure_output_dir_exists() {
    if (!path_line_edit) return;
    String path = path_line_edit->get_text();
    if (path.is_empty()) return;

    Ref<DirAccess> da = DirAccess::open("res://");
    if (da.is_null()) return;
    if (da->dir_exists(path)) return;
    da->make_dir_recursive(path);

#if defined(SPRITESTUDIO_GODOT_EXTENSION) || (VERSION_MAJOR >= 4 && VERSION_MINOR >= 6)
    auto *efs = EditorInterface::get_singleton()->get_resource_filesystem();
#else
    auto *efs = EditorInterface::get_singleton()->get_resource_file_system();
#endif
    if (!efs) return;
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    efs->scan_sources();
#else
    efs->scan_changes();
#endif
}

#endif // #ifdef TOOLS_ENABLED
