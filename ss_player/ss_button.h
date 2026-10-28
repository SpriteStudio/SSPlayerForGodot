#pragma once

#include "ss_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/base_button.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
using namespace godot;
#else
#include "core/templates/hash_set.h"
#include "scene/gui/base_button.h"
#endif

class SpriteStudioPlayer2D;
class SSABResource;
namespace ss {
namespace format {
struct AnimationData;
}
} // namespace ss

// A BaseButton drawn by the SpriteStudioPlayer2D placed under it, as a
// CollisionShape2D is placed under its body. The player keeps its own
// inspector and editor preview; the button fits it into its rect, and while
// the game runs plays the label section of the state the button is in.
//
// Pointer narrowing lives here, on the node that is pressed: the viewport asks
// the pressed node's own has_point, and a child's answer cannot take back what
// a parent's rect already holds.
class SpriteStudioButton : public BaseButton {
    GDCLASS(SpriteStudioButton, BaseButton);

public:
    enum StretchMode {
        STRETCH_FIT,   // the whole canvas inside the rect
        STRETCH_COVER, // the rect inside the canvas
        STRETCH_KEEP,  // authored size, the player's origin at the rect's center
    };

    enum AlignmentMode {
        ALIGNMENT_BEGIN,
        ALIGNMENT_CENTER,
        ALIGNMENT_END,
    };

private:
    StretchMode _stretch_mode = STRETCH_FIT;
    AlignmentMode _alignment_horizontal = ALIGNMENT_CENTER;
    AlignmentMode _alignment_vertical = ALIGNMENT_CENTER;
    PackedStringArray _hit_parts;
    bool _hit_on_artwork = false;
    bool _reduced_motion = false;
    Dictionary _state_clips;

    // The player whose item_rect_changed is connected, by instance id so a
    // player freed under us is not dereferenced.
    uint64_t _player_id = 0;
    // What the player was last made to show. A redraw that changes none of it,
    // on a player still showing it, leaves playback alone, since replaying
    // would restart the section. Assigning an SSAB -- the same one included --
    // sets the player up afresh, and that is how it is seen.
    int _shown_state = -1;
    bool _shown_selected = false;
    bool _shown_reduced = false;
    uint64_t _shown_pack = 0;
    uint32_t _shown_generation = 0;
    String _shown_animation;
    int _shown_start = 0;
    int _shown_end = 0;
    bool _shown_hold = false;
    // The animation the states are read from: the player's current animation,
    // kept while a clip plays another one.
    String _base_animation;
    // Set while driving the player, whose own change notices are ours.
    bool _showing = false;
    HashSet<String> _reported;

    SpriteStudioPlayer2D* _find_player() const;
    void _update_player();
    void _on_player_rect_changed();
    void _refit();
    int _current_state(bool& r_selected) const;
    void _show();
    bool _clip_section(const Ref<SSABResource>& p_res, const String& p_base, const ss::format::AnimationData* p_base_data,
                       const String& p_name, String& r_animation, int& r_start, int& r_end);
    void _report(const String& p_key, const String& p_message);
    PackedStringArray _state_clip_problems() const;

protected:
    void _notification(int p_what);
    static void _bind_methods();
    void _validate_property(PropertyInfo& p_property) const;

public:
    void set_stretch_mode(StretchMode p_mode);
    StretchMode get_stretch_mode() const;
    void set_alignment_horizontal(AlignmentMode p_mode);
    AlignmentMode get_alignment_horizontal() const;
    void set_alignment_vertical(AlignmentMode p_mode);
    AlignmentMode get_alignment_vertical() const;

    void set_hit_parts(const PackedStringArray& p_parts);
    PackedStringArray get_hit_parts() const;
    void set_hit_on_artwork(bool p_enabled);
    bool is_hit_on_artwork() const;

    void set_reduced_motion(bool p_enabled);
    bool is_reduced_motion() const;
    void set_state_clips(const Dictionary& p_clips);
    Dictionary get_state_clips() const;

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    bool _has_point(const Vector2& p_point) const override;
    PackedStringArray _get_configuration_warnings() const override;
#else
    bool has_point(const Point2& p_point) const override;
    PackedStringArray get_configuration_warnings() const override;
#endif
};

VARIANT_ENUM_CAST(SpriteStudioButton::StretchMode);
VARIANT_ENUM_CAST(SpriteStudioButton::AlignmentMode);
