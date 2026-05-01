#ifdef TOOLS_ENABLED

#ifndef CLICKABLE_LABEL_H
#define CLICKABLE_LABEL_H

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/input_event.hpp>
using namespace godot;
#else
#include "scene/gui/label.h"
#include "core/input/input_event.h"
#endif

class GdClickableLabel : public Label {
    GDCLASS(GdClickableLabel, Label);

protected:
    static void _bind_methods();

public:
    GdClickableLabel();

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    virtual void _gui_input(const Ref<InputEvent> &p_event) override;
#else
    virtual void gui_input(const Ref<InputEvent> &p_event) override;
#endif
};

#endif // CLICKABLE_LABEL_HPP
#endif // #ifdef TOOLS_ENABLED
