#pragma once

#ifdef TOOLS_ENABLED

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
using namespace godot;
#else
#include "scene/gui/control.h"
#endif

// Transparent Control installed as a child of CanvasItemEditorViewport that
// claims drops of .ssab files and turns them into SpriteStudioPlayer2D nodes.
//
// Non-.ssab drops are explicitly rejected so that Godot's drop walk continues
// up to the host viewport (which still handles PNG/PackedScene/AudioStream).
// This relies on Viewport::_gui_drop walking parent CanvasItems when a child
// Control with mouse_filter != STOP returns false from can_drop_data.
class SSCanvasDropOverlay : public Control {
    GDCLASS(SSCanvasDropOverlay, Control)

protected:
    static void _bind_methods() {}

public:
    SSCanvasDropOverlay();

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    bool _can_drop_data(const Vector2 &p_at_position, const Variant &p_data) const override;
    void _drop_data(const Vector2 &p_at_position, const Variant &p_data) override;
#else
    bool can_drop_data(const Point2 &p_point, const Variant &p_data) const override;
    void drop_data(const Point2 &p_point, const Variant &p_data) override;
#endif

private:
    PackedStringArray _extract_ssab_paths(const Variant &p_data) const;
    Vector2 _viewport_to_world(const Vector2 &p_at_position) const;
    // Orchestrates the full drop: empty-scene root promotion + remaining
    // children with undo + selection update.
    void _do_drop(const Vector2 &p_drop_pos, const Variant &p_data);
};

#endif // TOOLS_ENABLED
