#ifdef TOOLS_ENABLED

#include "ss_playback_panel.h"

#include "ss_player_node_2d.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/check_button.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/h_slider.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/shortcut.hpp>
#include <godot_cpp/classes/spin_box.hpp>
#include <godot_cpp/classes/v_separator.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
using namespace godot;
#else
#include "core/input/input_event.h"
#include "core/input/shortcut.h"
#include "core/os/keyboard.h"
#include "core/variant/array.h"
#include "editor/editor_interface.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/check_button.h"
#include "scene/gui/control.h"
#include "scene/gui/label.h"
#include "scene/gui/separator.h"
#include "scene/gui/slider.h"
#include "scene/gui/spin_box.h"
#endif

// Builds a single-event Shortcut bound to a physical key (+ optional Shift),
// matching how Godot's AnimationPlayer editor binds its A/S/D playback keys
// (physical so the cluster stays put on non-QWERTY layouts). Key codes are
// passed as ASCII ints ('A'/'S'/'D') because the Key enumerators differ
// between the GDExtension (KEY_A) and module (Key::A) builds while the
// underlying values are identical.
static Ref<Shortcut> _ss_make_shortcut(int p_keycode, bool p_shift) {
    Ref<InputEventKey> ev;
    ev.instantiate();
    ev->set_physical_keycode((Key)p_keycode);
    if (p_shift) {
        ev->set_shift_pressed(true);
    }
    Ref<Shortcut> sc;
    sc.instantiate();
    Array events;
    events.push_back(ev);
    sc->set_events(events);
    return sc;
}

SSPlaybackPanel::SSPlaybackPanel() {
    _build_ui();
}

void SSPlaybackPanel::_build_ui() {
    // Row 1: transport buttons (left) + loop + speed (right).
    // Animation selection is intentionally NOT here — it lives in the
    // inspector's `animation` enum (single source of truth). This panel only
    // mirrors the resulting frame range/label via the player's
    // property_list_changed signal.
    HBoxContainer *toolbar = memnew(HBoxContainer);
    add_child(toolbar);

    _play_start_btn = memnew(Button);
    _play_start_btn->set_flat(true);
    _play_start_btn->set_tooltip_text(tr("Play from start"));
    _play_start_btn->connect("pressed", callable_mp(this, &SSPlaybackPanel::_on_play_start_pressed));
    toolbar->add_child(_play_start_btn);

    _play_btn = memnew(Button);
    _play_btn->set_flat(true);
    _play_btn->set_tooltip_text(tr("Play from current position"));
    _play_btn->connect("pressed", callable_mp(this, &SSPlaybackPanel::_on_play_pressed));
    toolbar->add_child(_play_btn);

    _stop_btn = memnew(Button);
    _stop_btn->set_flat(true);
    _stop_btn->set_tooltip_text(tr("Stop"));
    _stop_btn->connect("pressed", callable_mp(this, &SSPlaybackPanel::_on_stop_pressed));
    toolbar->add_child(_stop_btn);

    // Keyboard shortcuts mirroring Godot's AnimationPlayer editor (A/S/D
    // cluster). These are matched centrally in shortcut_input() (gated by panel
    // visibility, like the AnimationPlayer editor) instead of per-button via
    // set_shortcut_context — the latter only fires once a panel widget has
    // focus, which forces the user to click first. Tooltip key hints are
    // written manually to avoid the "(Physical)" suffix get_as_text() adds.
    _sc_play_start = _ss_make_shortcut('D', true);   // Play from start
    _sc_play = _ss_make_shortcut('D', false);        // Play from current
    _sc_stop = _ss_make_shortcut('S', false);        // Stop

    _play_start_btn->set_tooltip_text(_play_start_btn->get_tooltip_text() + " (Shift+D)");
    _play_btn->set_tooltip_text(_play_btn->get_tooltip_text() + " (D)");
    _stop_btn->set_tooltip_text(_stop_btn->get_tooltip_text() + " (S)");

    toolbar->add_child(memnew(VSeparator));

    // Push loop + speed to the right edge.
    Control *spacer = memnew(Control);
    spacer->set_h_size_flags(SIZE_EXPAND_FILL);
    toolbar->add_child(spacer);

    _loop_btn = memnew(CheckButton);
    _loop_btn->set_text(tr("Loop"));
    _loop_btn->set_tooltip_text(tr("Loop the animation indefinitely."));
    _loop_btn->connect("toggled", callable_mp(this, &SSPlaybackPanel::_on_loop_toggled));
    toolbar->add_child(_loop_btn);

    Label *speed_label = memnew(Label);
    speed_label->set_text(tr("Speed"));
    toolbar->add_child(speed_label);

    _speed_spin = memnew(SpinBox);
    _speed_spin->set_min(0.0);
    _speed_spin->set_max(4.0);
    _speed_spin->set_step(0.01);
    _speed_spin->set_allow_greater(true);
    _speed_spin->set_suffix("x");
    _speed_spin->set_tooltip_text(tr("Playback speed scale."));
    _speed_spin->connect("value_changed", callable_mp(this, &SSPlaybackPanel::_on_speed_changed));
    toolbar->add_child(_speed_spin);

    // Row 2: frame scrubber + current/total frame readout.
    HBoxContainer *scrubber = memnew(HBoxContainer);
    add_child(scrubber);

    _frame_slider = memnew(HSlider);
    _frame_slider->set_h_size_flags(SIZE_EXPAND_FILL);
    _frame_slider->set_v_size_flags(SIZE_SHRINK_CENTER);
    _frame_slider->set_min(0.0);
    _frame_slider->set_step(0.01);
    _frame_slider->set_tooltip_text(tr("Scrub to a frame."));
    _frame_slider->connect("value_changed", callable_mp(this, &SSPlaybackPanel::_on_slider_changed));
    scrubber->add_child(_frame_slider);

    _frame_spin = memnew(SpinBox);
    _frame_spin->set_min(0.0);
    _frame_spin->set_step(0.01);
    // Godot 4.3+ allows setting a custom step for the UI arrows while keeping a fine-grained underlying step.
    // Using string "set" ensures compilation compatibility with older Godot 4.x GDExtension headers.
    _frame_spin->set("custom_arrow_step", 1.0);
    _frame_spin->set_tooltip_text(tr("Current frame."));
    _frame_spin->connect("value_changed", callable_mp(this, &SSPlaybackPanel::_on_frame_spin_changed));
    scrubber->add_child(_frame_spin);

    _total_label = memnew(Label);
    _total_label->set_text("/ 0");
    scrubber->add_child(_total_label);

    _set_controls_enabled(false);
}

void SSPlaybackPanel::_apply_button_icons() {
    Control *base = EditorInterface::get_singleton()->get_base_control();
    if (!base) {
        return;
    }
    _play_start_btn->set_button_icon(base->get_theme_icon(SNAME("PlayStart"), SNAME("EditorIcons")));
    _play_btn->set_button_icon(base->get_theme_icon(SNAME("Play"), SNAME("EditorIcons")));
    _stop_btn->set_button_icon(base->get_theme_icon(SNAME("Stop"), SNAME("EditorIcons")));
}

void SSPlaybackPanel::_set_controls_enabled(bool p_enabled) {
    _play_start_btn->set_disabled(!p_enabled);
    _play_btn->set_disabled(!p_enabled);
    _stop_btn->set_disabled(!p_enabled);
    _loop_btn->set_disabled(!p_enabled);
    _speed_spin->set_editable(p_enabled);
    _frame_slider->set_editable(p_enabled);
    _frame_spin->set_editable(p_enabled);
}

void SSPlaybackPanel::set_player(SpriteStudioPlayer2D *p_player) {
    if (_player == p_player) {
        return;
    }

    Callable exiting_cb = callable_mp(this, &SSPlaybackPanel::_on_player_exiting);
    Callable plist_cb = callable_mp(this, &SSPlaybackPanel::_refresh_from_player);
    if (_player) {
        if (_player->is_connected("tree_exiting", exiting_cb)) {
            _player->disconnect("tree_exiting", exiting_cb);
        }
        if (_player->is_connected("property_list_changed", plist_cb)) {
            _player->disconnect("property_list_changed", plist_cb);
        }
    }

    _player = p_player;

    if (_player) {
        if (!_player->is_connected("tree_exiting", exiting_cb)) {
            _player->connect("tree_exiting", exiting_cb);
        }
        // Inspector picks a different animation / assigns a resource -> the
        // node fires property_list_changed; mirror the new frame range here.
        if (!_player->is_connected("property_list_changed", plist_cb)) {
            _player->connect("property_list_changed", plist_cb);
        }
    }

    _refresh_from_player();
}

void SSPlaybackPanel::_on_player_exiting() {
    _player = nullptr;
    _refresh_from_player();
}

void SSPlaybackPanel::_refresh_from_player() {
    if (!_player) {
        _updating = true;
        _frame_slider->set_max(0.0);
        _frame_slider->set_value(0.0);
        _frame_spin->set_max(0.0);
        _frame_spin->set_value(0.0);
        _total_label->set_text("/ 0");
        _updating = false;
        _set_controls_enabled(false);
        return;
    }

    _set_controls_enabled(true);

    _updating = true;

    int total = _player->getTotalFrames();
    int max_frame = total > 0 ? total - 1 : 0;
    _frame_slider->set_max(max_frame);
    _frame_spin->set_max(max_frame);
    _total_label->set_text("/ " + String::num_int64(max_frame));

    _speed_spin->set_value(_player->getSpeedScale());
    _loop_btn->set_pressed(_player->getLoopCount() != 1);

    _updating = false;

    _sync_playhead();
}

void SSPlaybackPanel::_sync_playhead() {
    if (!_player) {
        return;
    }
    _updating = true;
    float frame = _player->getFrame();
    _frame_slider->set_value(frame);
    _frame_spin->set_value(frame);
    _updating = false;
}

void SSPlaybackPanel::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_ENTER_TREE:
        case NOTIFICATION_THEME_CHANGED:
            _apply_button_icons();
            set_process(true);
            set_process_shortcut_input(true);
            break;
        case NOTIFICATION_PROCESS:
            // Follow the playhead while the node animates in the editor.
            if (_player && _player->isPlaying()) {
                _sync_playhead();
            }
            break;
    }
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
void SSPlaybackPanel::_shortcut_input(const Ref<InputEvent> &p_event) {
#else
void SSPlaybackPanel::shortcut_input(const Ref<InputEvent> &p_event) {
#endif
    // Matches Godot's AnimationPlayer editor: active whenever the panel is
    // visible. shortcut_input runs after GUI input, so a focused control that
    // consumes the key first takes priority (standard behavior).
    if (!_player || !is_visible_in_tree()) {
        return;
    }
    Ref<InputEventKey> k = p_event;
    if (k.is_null() || !k->is_pressed() || k->is_echo()) {
        return;
    }
    // Most-specific (modifier) shortcut first so Shift+D doesn't fall through
    // to the plain-D handler.
    if (_sc_play_start->matches_event(p_event)) {
        _on_play_start_pressed();
        accept_event();
    } else if (_sc_play->matches_event(p_event)) {
        _on_play_pressed();
        accept_event();
    } else if (_sc_stop->matches_event(p_event)) {
        _on_stop_pressed();
        accept_event();
    }
}

void SSPlaybackPanel::_on_play_start_pressed() {
    if (!_player) {
        return;
    }
    _player->setPlaybackDirection(0, _player->getPlaybackStyle());
    _player->play(0.0f);
}

void SSPlaybackPanel::_on_play_pressed() {
    if (!_player) {
        return;
    }
    // Play from the current frame explicitly. play() with no arg resumes via
    // ss_runtime_play, which does not restart from a stopped state; passing the
    // current frame routes through play_with_start_frame so it reliably plays
    // from wherever the playhead is.
    _player->play(_player->getFrame());
}

void SSPlaybackPanel::_on_stop_pressed() {
    if (!_player) {
        return;
    }
    _player->stop();
    _sync_playhead();
}

void SSPlaybackPanel::_on_loop_toggled(bool p_on) {
    if (_updating || !_player) {
        return;
    }
    _player->setLoopCount(p_on ? -1 : 1);
}

void SSPlaybackPanel::_on_speed_changed(double p_value) {
    if (_updating || !_player) {
        return;
    }
    _player->setSpeedScale((float)p_value);
}

void SSPlaybackPanel::_on_slider_changed(double p_value) {
    if (_updating || !_player) {
        return;
    }
    _player->setFrame((float)p_value);
    _updating = true;
    _frame_spin->set_value(p_value);
    _updating = false;
}

void SSPlaybackPanel::_on_frame_spin_changed(double p_value) {
    if (_updating || !_player) {
        return;
    }
    _player->setFrame((float)p_value);
    _updating = true;
    _frame_slider->set_value(p_value);
    _updating = false;
}

#endif // TOOLS_ENABLED
