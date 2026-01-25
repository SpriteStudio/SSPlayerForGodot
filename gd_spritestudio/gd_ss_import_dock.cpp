#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
#include <godot_cpp/classes/window.hpp>
using namespace godot;
#else
#include "editor/editor_interface.h"
#include "editor/file_system/editor_file_system.h"
#include "scene/main/window.h"
#endif

#include "gd_ss_import_dock.h"
#include "ssconverter.h"

void GdSsImportControl::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_window_files_dropped", "files"), &GdSsImportControl::_on_window_files_dropped);
}


GdSsImportControl::GdSsImportControl() {
    this->Ctx = ss_converter_create();

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
        
    set_h_size_flags(Control::SIZE_EXPAND_FILL);
    set_v_size_flags(Control::SIZE_EXPAND_FILL);
    set_custom_minimum_size(Vector2(200, 100));
    set_mouse_filter(MOUSE_FILTER_STOP);
    set_focus_mode(FOCUS_CLICK);
}

GdSsImportControl::~GdSsImportControl() {
    stop_intercepting();

    ss_converter_destroy((Context *)this->Ctx);
    this->Ctx = nullptr;
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

void GdSsImportControl::process_file(const String &source_path) {
    ss_converter_convert((Context *)this->Ctx, source_path.utf8().get_data(), nullptr);
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
    print_line("DropInterceptor: Hijacked files_dropped signal.");
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
    print_line("GdSsImportControl: Restored original files_dropped signal.");
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
        
        print_line("GdSsImportControl: Processing custom file drop...");

        // validate sspj file

        for (int i = 0; i < p_files.size(); i++) {
            print_line("Converting: " + p_files[i]);
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
