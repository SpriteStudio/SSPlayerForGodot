#pragma once

#ifdef TOOLS_ENABLED

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/shortcut.hpp>
#include <godot_cpp/classes/v_box_container.hpp>
using namespace godot;
#else
#include "core/input/input_event.h"
#include "core/input/shortcut.h"
#include "scene/gui/box_container.h"
#endif

class SpriteStudioPlayer2D;

#ifdef SPRITESTUDIO_GODOT_EXTENSION
namespace godot {
class Button;
class CheckButton;
class HSlider;
class SpinBox;
class Label;
} // namespace godot
#else
class Button;
class CheckButton;
class HSlider;
class SpinBox;
class Label;
#endif

// Bottom-panel transport controls for SpriteStudioPlayer2D, mirroring the
// AnimationPlayer editor experience: an animation picker, play-from-start /
// play / play-backwards / pause / stop buttons, a frame scrubber, a loop
// toggle and a speed-scale field.
//
// The panel never touches the runtime directly; it drives the selected node
// through its public playback API and reads back state in _process to keep the
// scrubber and button states in sync while the animation runs in the editor.
class SSPlaybackPanel : public VBoxContainer {
    GDCLASS(SSPlaybackPanel, VBoxContainer)

protected:
    static void _bind_methods() {}
    void _notification(int p_what);

public:
    SSPlaybackPanel();

    // Centralized playback shortcuts (A/S/D cluster), matching Godot's
    // AnimationPlayer editor: handled in shortcut_input and gated by
    // is_visible_in_tree(). Same trade-off as the standard — a focused control
    // that consumes the key first (e.g. scene-tree type-ahead) takes priority.
    // Must be public: godot-cpp registers the virtual via a template that takes
    // &SSPlaybackPanel::_shortcut_input, which can't reach a protected member.
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    void _shortcut_input(const Ref<InputEvent> &p_event) override;
#else
    void shortcut_input(const Ref<InputEvent> &p_event) override;
#endif

    // The editor plugin feeds the currently edited node here (nullptr clears).
    void set_player(SpriteStudioPlayer2D *p_player);
    SpriteStudioPlayer2D *get_player() const { return _player; }

private:
    SpriteStudioPlayer2D *_player = nullptr;

    Button *_play_start_btn = nullptr;
    Button *_play_btn = nullptr;
    Button *_stop_btn = nullptr;
    CheckButton *_loop_btn = nullptr;
    SpinBox *_speed_spin = nullptr;
    HSlider *_frame_slider = nullptr;
    SpinBox *_frame_spin = nullptr;
    Label *_total_label = nullptr;

    // Guards programmatic control updates so they don't re-enter the
    // user-edit callbacks and fight the playhead sync.
    bool _updating = false;

    // Playback shortcuts, matched in shortcut_input(). Stored so the central
    // handler can compare against the live event.
    Ref<Shortcut> _sc_play_start;
    Ref<Shortcut> _sc_play;
    Ref<Shortcut> _sc_stop;

    void _build_ui();
    void _apply_button_icons();
    void _refresh_from_player();
    void _sync_playhead();
    void _set_controls_enabled(bool p_enabled);

    void _on_play_start_pressed();
    void _on_play_pressed();
    void _on_stop_pressed();
    void _on_loop_toggled(bool p_on);
    void _on_speed_changed(double p_value);
    void _on_slider_changed(double p_value);
    void _on_frame_spin_changed(double p_value);
    void _on_player_exiting();
};

#endif // TOOLS_ENABLED
