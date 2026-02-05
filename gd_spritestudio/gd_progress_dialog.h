#pragma once

#ifdef TOOLS_ENABLED

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    #include <godot_cpp/classes/window.hpp>
    #include <godot_cpp/classes/button.hpp>
    #include <godot_cpp/classes/v_box_container.hpp> 
    #include <godot_cpp/classes/label.hpp>
    #include <godot_cpp/classes/progress_bar.hpp>

    using namespace godot;
#else
    #include "scene/main/window.h"
    #include "scene/gui/button.h"
    #include "scene/gui/box_container.h"
    #include "scene/gui/label.h"
    #include "scene/gui/progress_bar.h"
#endif


class GdProgressDialog : public Window {
    GDCLASS(GdProgressDialog, Window);

private:
    VBoxContainer *vbox = nullptr;
    Label *status_label = nullptr;
    ProgressBar *progress_bar = nullptr;

    // Button *cancel_button = nullptr;
    // bool canceled = false;

protected:
    static void _bind_methods();

    // void _on_cancel_pressed();

    public:
    GdProgressDialog();

    void show_progress(const String &title, int total_steps);
    void step(const String &message, int step_value);    
    void finish();

    // bool is_canceled() const;
};
#endif // #ifdef TOOLS_ENABLED
