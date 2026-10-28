#include "ss_button.h"

#include "format/ssab.h"
#include "ss_player_node_2d.h"
#include "ssab_resource.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/object.hpp>
#else
#include "core/config/engine.h"
#include "core/object/object.h"
#endif

namespace {

enum UiState {
    STATE_NORMAL,
    STATE_HOVER,
    STATE_PRESSED,
    STATE_DISABLED,
    STATE_FOCUSED,
    STATE_COUNT,
};

const char* const STATE_NAMES[STATE_COUNT] = { "normal", "hover", "pressed", "disabled", "focused" };
const char* const SELECTED_PREFIX = "selected_";
const char* const REDUCED_MOTION_LABEL = "reduced_motion";

// The labels a state is read from, in order: its own, then those it falls back
// to when the animation lacks it. A selected button tries every `selected_`
// label first, so a toggle that is on still looks on while hovered -- except
// that a press goes straight to `pressed`, being brief and the sign the press
// was taken.
void ss_state_chain(int p_state, bool p_selected, LocalVector<String>& r_chain) {
    static const int CHAINS[STATE_COUNT][4] = {
        { STATE_NORMAL, -1 },
        { STATE_HOVER, STATE_NORMAL, -1 },
        { STATE_PRESSED, STATE_HOVER, STATE_NORMAL, -1 },
        { STATE_DISABLED, STATE_NORMAL, -1 },
        { STATE_FOCUSED, STATE_NORMAL, -1 },
    };
    r_chain.clear();
    const int* chain = CHAINS[p_state];
    if (p_selected && p_state == STATE_PRESSED) {
        r_chain.push_back(String(SELECTED_PREFIX) + STATE_NAMES[STATE_PRESSED]);
        r_chain.push_back(STATE_NAMES[STATE_PRESSED]);
        chain = CHAINS[STATE_HOVER];
    }
    if (p_selected) {
        for (int i = 0; chain[i] >= 0; i++) {
            r_chain.push_back(String(SELECTED_PREFIX) + STATE_NAMES[chain[i]]);
        }
    }
    for (int i = 0; chain[i] >= 0; i++) {
        r_chain.push_back(STATE_NAMES[chain[i]]);
    }
}

// The section starting at p_label: to the frame before the next label of any
// name, or to the end of the animation.
bool ss_label_section(const ss::format::AnimationData* p_animation, const LocalVector<SsLabel>& p_labels,
                      const String& p_label, int& r_start, int& r_end) {
    int start = 0;
    if (!SsInternalPlayer::find_label_frame(p_animation, p_labels, p_label, start)) return false;
    int end = p_animation->end_frame();
    for (uint32_t i = 0; i < p_labels.size(); i++) {
        if (p_labels[i].frame > start) {
            end = p_labels[i].frame - 1;
            break;
        }
    }
    r_start = start;
    r_end = end;
    return true;
}

bool ss_is_state_name(const String& p_name) {
    const String stem = p_name.trim_prefix(SELECTED_PREFIX);
    for (int i = 0; i < STATE_COUNT; i++) {
        if (stem == STATE_NAMES[i]) return true;
    }
    return false;
}

float ss_align_axis(float p_view, float p_scale, float p_low, float p_size, SpriteStudioButton::AlignmentMode p_mode) {
    switch (p_mode) {
        case SpriteStudioButton::ALIGNMENT_BEGIN:
            return -p_scale * p_low;
        case SpriteStudioButton::ALIGNMENT_END:
            return p_view - p_scale * (p_low + p_size);
        default:
            return p_view * 0.5f - p_scale * (p_low + p_size * 0.5f);
    }
}

} // namespace

void SpriteStudioButton::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_stretch_mode", "mode"), &SpriteStudioButton::set_stretch_mode);
    ClassDB::bind_method(D_METHOD("get_stretch_mode"), &SpriteStudioButton::get_stretch_mode);
    ClassDB::bind_method(D_METHOD("set_alignment_horizontal", "alignment"), &SpriteStudioButton::set_alignment_horizontal);
    ClassDB::bind_method(D_METHOD("get_alignment_horizontal"), &SpriteStudioButton::get_alignment_horizontal);
    ClassDB::bind_method(D_METHOD("set_alignment_vertical", "alignment"), &SpriteStudioButton::set_alignment_vertical);
    ClassDB::bind_method(D_METHOD("get_alignment_vertical"), &SpriteStudioButton::get_alignment_vertical);

    ClassDB::bind_method(D_METHOD("set_hit_parts", "parts"), &SpriteStudioButton::set_hit_parts);
    ClassDB::bind_method(D_METHOD("get_hit_parts"), &SpriteStudioButton::get_hit_parts);
    ClassDB::bind_method(D_METHOD("set_hit_on_artwork", "enabled"), &SpriteStudioButton::set_hit_on_artwork);
    ClassDB::bind_method(D_METHOD("is_hit_on_artwork"), &SpriteStudioButton::is_hit_on_artwork);

    ClassDB::bind_method(D_METHOD("set_reduced_motion", "enabled"), &SpriteStudioButton::set_reduced_motion);
    ClassDB::bind_method(D_METHOD("is_reduced_motion"), &SpriteStudioButton::is_reduced_motion);
    ClassDB::bind_method(D_METHOD("set_state_clips", "clips"), &SpriteStudioButton::set_state_clips);
    ClassDB::bind_method(D_METHOD("get_state_clips"), &SpriteStudioButton::get_state_clips);

    ADD_GROUP("Fit", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "stretch_mode", PROPERTY_HINT_ENUM, "Fit,Cover,Keep"), "set_stretch_mode", "get_stretch_mode");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "alignment_horizontal", PROPERTY_HINT_ENUM, "Begin,Center,End"), "set_alignment_horizontal", "get_alignment_horizontal");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "alignment_vertical", PROPERTY_HINT_ENUM, "Begin,Center,End"), "set_alignment_vertical", "get_alignment_vertical");

    ADD_GROUP("Hit Area", "");
    // The part names come from the player, in _validate_property.
    ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "hit_parts"), "set_hit_parts", "get_hit_parts");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "hit_on_artwork"), "set_hit_on_artwork", "is_hit_on_artwork");

    ADD_GROUP("States", "");
    ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "state_clips"), "set_state_clips", "get_state_clips");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "reduced_motion"), "set_reduced_motion", "is_reduced_motion");

    BIND_ENUM_CONSTANT(STRETCH_FIT);
    BIND_ENUM_CONSTANT(STRETCH_COVER);
    BIND_ENUM_CONSTANT(STRETCH_KEEP);

    BIND_ENUM_CONSTANT(ALIGNMENT_BEGIN);
    BIND_ENUM_CONSTANT(ALIGNMENT_CENTER);
    BIND_ENUM_CONSTANT(ALIGNMENT_END);
}

void SpriteStudioButton::set_stretch_mode(StretchMode p_mode) {
    _stretch_mode = p_mode;
    _refit();
}
SpriteStudioButton::StretchMode SpriteStudioButton::get_stretch_mode() const { return _stretch_mode; }

void SpriteStudioButton::set_alignment_horizontal(AlignmentMode p_mode) {
    _alignment_horizontal = p_mode;
    _refit();
}
SpriteStudioButton::AlignmentMode SpriteStudioButton::get_alignment_horizontal() const { return _alignment_horizontal; }

void SpriteStudioButton::set_alignment_vertical(AlignmentMode p_mode) {
    _alignment_vertical = p_mode;
    _refit();
}
SpriteStudioButton::AlignmentMode SpriteStudioButton::get_alignment_vertical() const { return _alignment_vertical; }

void SpriteStudioButton::set_hit_parts(const PackedStringArray& p_parts) {
    _hit_parts = p_parts;
    update_configuration_warnings();
}
PackedStringArray SpriteStudioButton::get_hit_parts() const { return _hit_parts; }

void SpriteStudioButton::set_hit_on_artwork(bool p_enabled) { _hit_on_artwork = p_enabled; }
bool SpriteStudioButton::is_hit_on_artwork() const { return _hit_on_artwork; }

void SpriteStudioButton::set_reduced_motion(bool p_enabled) {
    _reduced_motion = p_enabled;
    queue_redraw();
}
bool SpriteStudioButton::is_reduced_motion() const { return _reduced_motion; }

void SpriteStudioButton::set_state_clips(const Dictionary& p_clips) {
    _state_clips = p_clips;
    _shown_state = -1;
    update_configuration_warnings();
    // The editor says so in the node's warnings; a script that sets them while
    // the game runs reads the Output panel instead.
    if (!Engine::get_singleton()->is_editor_hint()) {
        const PackedStringArray problems = _state_clip_problems();
        for (int i = 0; i < problems.size(); i++) {
            _report(problems[i], vformat("[SS] %s: %s", get_name(), problems[i]));
        }
    }
    queue_redraw();
}
Dictionary SpriteStudioButton::get_state_clips() const { return _state_clips; }

SpriteStudioPlayer2D* SpriteStudioButton::_find_player() const {
    // Spelled out: the engine's own default counts internal children, the
    // bound one does not.
    for (int i = 0; i < get_child_count(false); i++) {
        if (SpriteStudioPlayer2D* player = Object::cast_to<SpriteStudioPlayer2D>(get_child(i, false))) return player;
    }
    return nullptr;
}

void SpriteStudioButton::_update_player() {
    SpriteStudioPlayer2D* player = _find_player();
    const uint64_t id = player ? (uint64_t)player->get_instance_id() : 0;
    if (id != _player_id) {
        const Callable changed = callable_mp(this, &SpriteStudioButton::_on_player_rect_changed);
        Object* previous = _player_id ? ObjectDB::get_instance(ObjectID(_player_id)) : nullptr;
        if (previous && previous->is_connected("item_rect_changed", changed)) {
            previous->disconnect("item_rect_changed", changed);
        }
        if (player) {
            player->connect("item_rect_changed", changed);
        }
        _player_id = id;
        _shown_state = -1;
        _base_animation = String();
    }
    _refit();
    queue_redraw();
    update_configuration_warnings();
}

void SpriteStudioButton::_on_player_rect_changed() {
    if (_showing) return;
    _refit();
    queue_redraw();
    // hit_parts offers the part names of the SSAB the player now has.
    if (Engine::get_singleton()->is_editor_hint()) {
        notify_property_list_changed();
        update_configuration_warnings();
    }
}

void SpriteStudioButton::_refit() {
    SpriteStudioPlayer2D* player = _find_player();
    if (!player) return;
    const Vector2 view = get_size();
    // The animation the states are read from, so a clip that plays another
    // animation with another canvas does not resize the artwork mid-press.
    const Rect2 box = player->get_canvas_rect_of(_base_animation.is_empty() ? player->getCurrentAnimation() : _base_animation);
    float scale = 1.0f;
    Vector2 origin = view * 0.5f;
    if (_stretch_mode != STRETCH_KEEP && box.size.x > 0.0f && box.size.y > 0.0f && view.x > 0.0f && view.y > 0.0f) {
        const float sx = view.x / box.size.x;
        const float sy = view.y / box.size.y;
        scale = _stretch_mode == STRETCH_FIT ? MIN(sx, sy) : MAX(sx, sy);
        origin.x = ss_align_axis(view.x, scale, box.position.x, box.size.x, _alignment_horizontal);
        origin.y = ss_align_axis(view.y, scale, box.position.y, box.size.y, _alignment_vertical);
    }
    if (player->get_position() != origin) player->set_position(origin);
    if (player->get_scale() != Vector2(scale, scale)) player->set_scale(Vector2(scale, scale));
}

int SpriteStudioButton::_current_state(bool& r_selected) const {
    const DrawMode mode = get_draw_mode();
    r_selected = is_toggle_mode() && is_pressed();
    int state = STATE_NORMAL;
    if (mode == DRAW_DISABLED) {
        state = STATE_DISABLED;
    } else if (is_toggle_mode()) {
        // A toggle that is on draws as pressed, and one held down draws as the
        // toggle it would become, so pressed is the draw mode disagreeing with
        // the toggle.
        if (mode == DRAW_HOVER || mode == DRAW_HOVER_PRESSED) {
            state = STATE_HOVER;
        } else if ((mode == DRAW_PRESSED) != r_selected) {
            state = STATE_PRESSED;
        }
    } else if (mode == DRAW_PRESSED || mode == DRAW_HOVER_PRESSED) {
        state = STATE_PRESSED;
    } else if (mode == DRAW_HOVER) {
        state = STATE_HOVER;
    }
    // Focus a click gave is hidden as Button hides it.
    if (state == STATE_NORMAL && has_focus(true)) state = STATE_FOCUSED;
    return state;
}

bool SpriteStudioButton::_clip_section(const Ref<SSABResource>& p_res, const String& p_base,
                                       const ss::format::AnimationData* p_base_data, const String& p_name,
                                       String& r_animation, int& r_start, int& r_end) {
    if (!_state_clips.has(p_name)) return false;
    const Variant value = _state_clips[p_name];
    if (value.get_type() != Variant::DICTIONARY) return false;
    const Dictionary clip = value;
    const String clip_animation = clip.get("animation", String());
    const String clip_label = clip.get("label", String());
    if (clip_animation.is_empty() && clip_label.is_empty()) return false;

    const String animation = clip_animation.is_empty() ? p_base : clip_animation;
    const ss::format::AnimationData* data = animation == p_base ? p_base_data : p_res->find_animation(animation);
    int start = 0;
    int end = 0;
    bool found = false;
    if (data && clip_label.is_empty()) {
        start = data->start_frame();
        end = data->end_frame();
        found = true;
    } else if (data) {
        LocalVector<SsLabel> labels;
        SsInternalPlayer::get_label_table(data, labels);
        found = ss_label_section(data, labels, clip_label, start, end);
    }
    if (!found) {
        String what;
        if (!clip_animation.is_empty()) what = vformat("animation \"%s\"", clip_animation);
        if (!clip_label.is_empty()) what += vformat("%slabel \"%s\"", what.is_empty() ? "" : " with ", clip_label);
        _report(vformat("%s|%s|%s", p_name, clip_animation, clip_label),
                vformat("[SS] %s: state_clips[\"%s\"] names %s, which %s does not have, so the state plays as if it had no clip.",
                        get_name(), p_name, what, p_res->get_path()));
        return false;
    }
    r_animation = animation;
    r_start = start;
    r_end = end;
    return true;
}

void SpriteStudioButton::_show() {
    // The editor shows the player as its inspector left it; states are play.
    if (Engine::get_singleton()->is_editor_hint()) return;
    SpriteStudioPlayer2D* player = _find_player();
    if (!player) return;
    Ref<SSABResource> res = player->getSSABResource();
    if (res.is_null()) return;

    bool selected = false;
    const int state = _current_state(selected);
    const uint64_t pack = (uint64_t)res->get_instance_id();
    const uint32_t generation = res->get_generation();
    const String current = player->getCurrentAnimation();
    const bool same_pack = pack == _shown_pack && generation == _shown_generation;
    if (same_pack && state == _shown_state && selected == _shown_selected && _reduced_motion == _shown_reduced &&
        current == _shown_animation && player->getAnimationSectionStart() == _shown_start &&
        player->getAnimationSectionEnd() == _shown_end && player->isPlaying() != _shown_hold) {
        return;
    }
    if (!same_pack) _reported.clear();

    // An animation the button did not set was chosen by the scene or a script,
    // and is the one to read the states from from now on.
    const String base_before = _base_animation;
    if (_base_animation.is_empty() || current != _shown_animation || !res->find_animation(_base_animation)) {
        _base_animation = current;
    }
    const ss::format::AnimationData* base = res->find_animation(_base_animation);
    if (!base) return;
    LocalVector<SsLabel> labels;
    SsInternalPlayer::get_label_table(base, labels);

    LocalVector<String> chain;
    ss_state_chain(state, selected, chain);
    String animation = _base_animation;
    int start = base->start_frame();
    int end = base->end_frame();
    String source;
    for (uint32_t i = 0; i < chain.size() && source.is_empty(); i++) {
        if (_clip_section(res, _base_animation, base, chain[i], animation, start, end) ||
            ss_label_section(base, labels, chain[i], start, end)) {
            source = chain[i];
        }
    }
    bool hold = _reduced_motion;
    if (_reduced_motion && source == STATE_NAMES[STATE_NORMAL]) {
        int calm_start = 0;
        int calm_end = 0;
        if (ss_label_section(base, labels, REDUCED_MOTION_LABEL, calm_start, calm_end)) {
            animation = _base_animation;
            start = calm_start;
            end = calm_end;
            hold = false;
        }
    }

    _showing = true;
    player->setAnimation(animation);
    player->setAnimationSection(start, end);
    if (hold) {
        // Stopped rather than paused, so the held frame fires no events.
        player->stop();
        player->setFrameNo(start);
    } else {
        player->setLoopCount(-1);
        player->play();
    }
    _showing = false;

    _shown_state = state;
    _shown_selected = selected;
    _shown_reduced = _reduced_motion;
    _shown_pack = pack;
    _shown_generation = generation;
    _shown_animation = player->getCurrentAnimation();
    _shown_start = player->getAnimationSectionStart();
    _shown_end = player->getAnimationSectionEnd();
    _shown_hold = hold;
    if (_base_animation != base_before) _refit();
}

void SpriteStudioButton::_report(const String& p_key, const String& p_message) {
    if (_reported.has(p_key)) return;
    _reported.insert(p_key);
    WARN_PRINT(p_message);
}

PackedStringArray SpriteStudioButton::_state_clip_problems() const {
    PackedStringArray problems;
    const Array keys = _state_clips.keys();
    for (int i = 0; i < keys.size(); i++) {
        const String key = keys[i];
        if (!ss_is_state_name(key)) {
            problems.push_back(vformat("state_clips has the key \"%s\", which is not a state, so nothing reads it. The states are normal, hover, pressed, disabled and focused, each also with a \"selected_\" prefix.", key));
            continue;
        }
        const Variant value = _state_clips[keys[i]];
        const Dictionary clip = value.get_type() == Variant::DICTIONARY ? Dictionary(value) : Dictionary();
        if (String(clip.get("animation", String())).is_empty() && String(clip.get("label", String())).is_empty()) {
            problems.push_back(vformat("state_clips[\"%s\"] is not a dictionary naming an \"animation\", a \"label\" or both, so it is ignored.", key));
        }
    }
    return problems;
}

void SpriteStudioButton::_validate_property(PropertyInfo& p_property) const {
    if (p_property.name != StringName("hit_parts")) return;
    const SpriteStudioPlayer2D* player = _find_player();
    if (!player) return;
    p_property.hint = PROPERTY_HINT_TYPE_STRING;
    p_property.hint_string = vformat("%d/%d:%s", (int)Variant::STRING, (int)PROPERTY_HINT_ENUM_SUGGESTION,
                                     String(",").join(player->get_part_names()));
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool SpriteStudioButton::_has_point(const Vector2& p_point) const {
#else
bool SpriteStudioButton::has_point(const Point2& p_point) const {
#endif
    const bool in_rect = Rect2(Vector2(), get_size()).has_point(p_point);
    if (_hit_parts.is_empty() && !_hit_on_artwork) return in_rect;
    // Drawing clipped to the rect leaves nothing to press outside it.
    if (!in_rect && const_cast<SpriteStudioButton*>(this)->is_clipping_contents()) return false;
    const SpriteStudioPlayer2D* player = _find_player();
    if (!player) return in_rect;
    const Vector2 local = player->get_transform().affine_inverse().xform(p_point);
    return player->hit_test_part(local, _hit_parts) >= 0;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray SpriteStudioButton::_get_configuration_warnings() const {
    PackedStringArray warnings;
#else
PackedStringArray SpriteStudioButton::get_configuration_warnings() const {
    PackedStringArray warnings = BaseButton::get_configuration_warnings();
#endif
    const SpriteStudioPlayer2D* player = _find_player();
    if (player == nullptr) {
        warnings.push_back(tr("Add a SpriteStudioPlayer2D as a child of this node: the button is drawn by it and plays its states on it."));
    } else {
        const PackedStringArray parts = player->get_part_names();
        for (int i = 0; i < _hit_parts.size(); i++) {
            if (!parts.is_empty() && !parts.has(_hit_parts[i])) {
                warnings.push_back(vformat(tr("\"hit_parts\" names \"%s\", which the player's SSAB has no part of, so it takes no pointer."), _hit_parts[i]));
            }
        }
    }
    warnings.append_array(_state_clip_problems());
    return warnings;
}

void SpriteStudioButton::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_CHILD_ORDER_CHANGED:
            _update_player();
            break;
        case NOTIFICATION_RESIZED:
            _refit();
            break;
        case NOTIFICATION_DRAW:
            // BaseButton redraws on every change get_draw_mode reports, and
            // Control on focus, so this is where a state change arrives.
            _show();
            break;
    }
}
