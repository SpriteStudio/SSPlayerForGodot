#ifdef TOOLS_ENABLED

#include "ss_macros.h"
#include "ss_progress_dialog.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    #include <godot_cpp/classes/display_server.hpp>
    #include <godot_cpp/classes/time.hpp>
    #include <godot_cpp/classes/panel.hpp>
    #include <godot_cpp/classes/rendering_server.hpp>

    using namespace godot;
#else
#if VERSION_MAJOR >= 4
    #if VERSION_MINOR >= 6
    #include "servers/display/display_server.h"
    #include "servers/rendering/rendering_server.h"
    #else
    #include "servers/display_server.h"
    #include "servers/rendering_server.h"
    #endif
#endif
    #include "core/os/time.h"
    #include "scene/gui/panel.h"
    #include "core/object/message_queue.h"
#endif

void SSProgressDialog::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_cancel_pressed"), &SSProgressDialog::_on_cancel_pressed);
}

SSProgressDialog::SSProgressDialog() {
    set_wrap_controls(true);
    set_visible(false);
    set_transient(true);
    set_exclusive(true);
    set_keep_title_visible(true);

#ifndef SPRITESTUDIO_GODOT_EXTENSION
    set_clamp_to_embedder(true);
#if VERSION_MAJOR >= 4
    #if VERSION_MINOR >= 5
    set_flag(FLAG_MINIMIZE_DISABLED, true);
    set_flag(FLAG_MAXIMIZE_DISABLED, true);
    #else
    set_flag(FLAG_RESIZE_DISABLED, true);
    #endif
#endif
#endif
    set_title("Processing...");

    Panel *background = memnew(Panel);
    background->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
    add_child(background);

    vbox = memnew(VBoxContainer);
    background->add_child(vbox);
    vbox->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT, Control::PRESET_MODE_KEEP_SIZE, 10);
    // add_child(vbox);

    status_label = memnew(Label);
    status_label->set_text("Please wait...");
    status_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
    vbox->add_child(status_label);

    progress_bar = memnew(ProgressBar);
    vbox->add_child(progress_bar);

    cancel_button = memnew(Button);
    cancel_button->set_text("Cancel");
    vbox->add_child(cancel_button);
    cancel_button->connect("pressed", Callable(this, "_on_cancel_pressed"));

    set_size(Size2(300, 130));
}


void SSProgressDialog::show_progress(const String &title, int total_steps) {
    canceled = false;
    cancel_button->set_disabled(false);
    set_title(title);
    // total_steps <= 0 means "unknown total" (e.g. directory scan): keep the bar
    // empty and rely on the status label for live counts.
    progress_bar->set_max(total_steps > 0 ? total_steps : 1);
    progress_bar->set_value(0);

    popup_centered();
}

void SSProgressDialog::step(const String &message, int step_value) {
    status_label->set_text(message);
    progress_bar->set_value(step_value);
}

void SSProgressDialog::_on_cancel_pressed() {
    canceled = true;
    status_label->set_text("Canceling...");
    cancel_button->set_disabled(true); // 二重押し防止
}

bool SSProgressDialog::is_canceled() const {
    return canceled;
}

void SSProgressDialog::finish() {
    hide();
    queue_free();
}

#endif // #ifdef TOOLS_ENABLED
