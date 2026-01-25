#ifdef TOOLS_ENABLED

#include "gd_clickable_label.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/display_server.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/core/class_db.hpp>
using namespace godot;
#else
#include "servers/display_server.h"

#endif

void GdClickableLabel::_bind_methods() {
    ADD_SIGNAL(MethodInfo("text_copied"));
}

GdClickableLabel::GdClickableLabel() {
    set_mouse_filter(MOUSE_FILTER_STOP);
    
    set_default_cursor_shape(CURSOR_POINTING_HAND);
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
void GdClickableLabel::_gui_input(const Ref<InputEvent> &p_event) {
#else
void GdClickableLabel::gui_input(const Ref<InputEvent> &p_event) {
#endif
    Ref<InputEventMouseButton> mb = p_event;

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MOUSE_BUTTON_LEFT) {
#else
    if (mb.is_valid() && mb->is_pressed() && mb->get_button_index() == MouseButton::LEFT) {
#endif
        String text_to_copy = get_text();
        DisplayServer::get_singleton()->clipboard_set(text_to_copy);
        emit_signal("text_copied");
        accept_event();
    }
}
#endif // #ifdef TOOLS_ENABLED
