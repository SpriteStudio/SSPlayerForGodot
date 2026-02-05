#include "gd_progress_dialog.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    #include <godot_cpp/classes/display_server.hpp>
    #include <godot_cpp/classes/time.hpp>
    
    using namespace godot;
#else
    #include "servers/display_server.h"
    #include "core/os/time.h"
#endif

void GdProgressDialog::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_cancel_pressed"), &GdProgressDialog::_on_cancel_pressed);
}

GdProgressDialog::GdProgressDialog() {
set_exclusive(true);
    set_flag(Window::FLAG_ALWAYS_ON_TOP, true);
    set_title("Processing...");

    vbox = memnew(VBoxContainer);
    vbox->set_anchors_preset(Control::PRESET_FULL_RECT);
    add_child(vbox);

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


void GdProgressDialog::show_progress(const String &title, int total_steps) {
    canceled = false;
    cancel_button->set_disabled(false);
    set_title(title);
    progress_bar->set_max(total_steps);
    progress_bar->set_value(0);
    
    popup_centered();
    DisplayServer::get_singleton()->process_events();
}

void GdProgressDialog::step(const String &message, int step_value) {
    status_label->set_text(message);
    progress_bar->set_value(step_value);

    // これにより、ユーザーがボタンをクリックしたイベントが処理され、
    // _on_cancel_pressed() が呼び出される
    DisplayServer::get_singleton()->process_events();
    // オプショナル: 処理が速すぎてバーが見えない場合は少しウェイトを入れる
    // OS::get_singleton()->delay_usec(1000); 
}

void GdProgressDialog::_on_cancel_pressed() {
    canceled = true;
    status_label->set_text("Canceling...");
    cancel_button->set_disabled(true); // 二重押し防止
}

bool GdProgressDialog::is_canceled() const {
    return canceled;
}

void GdProgressDialog::finish() {
    hide();
    queue_free();
}
