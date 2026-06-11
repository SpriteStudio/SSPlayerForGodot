#pragma once

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/variant/node_path.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/transform2d.hpp>
using namespace godot;
#else
#include "scene/2d/node_2d.h"
#endif

class SpriteStudioPlayer2D;

// A user-owned node that mirrors a single SpriteStudio part's pose each frame.
// Modeled on Godot's RemoteTransform2D (remote_path / use_global_coordinates /
// update_position|rotation|scale) plus a part_name + player_path. The player
// never creates, owns, or frees this node: the user authors it in the scene
// tree and owns its (and its children's) lifecycle. The attachment only reads
// the part pose the player already computes and drives a transform — either its
// own (children follow via scene-tree inheritance) or a remote Node2D's
// (decoupled from the player's subtree). It connects to the player's
// `frame_updated` signal so it mirrors the part in the same frame it renders,
// independent of the player's process mode.
class SpriteStudioPartAttachment2D : public Node2D {
    GDCLASS(SpriteStudioPartAttachment2D, Node2D);

public:
    // Behaviour when the tracked part is hidden on the current frame. A part
    // absent from the animation entirely always hides the target regardless.
    enum HiddenBehavior {
        FOLLOW_ALWAYS, // keep following the part's frame while it is hidden
        HIDE_TARGET,   // hide the driven target while the part is hidden
    };

private:
    String _part_name;
    NodePath _player_path;  // empty -> nearest ancestor SpriteStudioPlayer2D
    NodePath _remote_path;  // empty -> drive self
    bool _use_global_coordinates = true;
    bool _update_position = true;
    bool _update_rotation = true;
    bool _update_scale = false; // off by default (Godot Transform2D carries the
                                // part's scale anyway when all three are on)
    HiddenBehavior _on_part_hidden = FOLLOW_ALWAYS;

    SpriteStudioPlayer2D* _resolve_player() const;
    Node2D* _resolve_target();
    void _connect_player();
    void _disconnect_player();
    void _on_player_frame_updated(float frame_no);
    void _apply_transform(Node2D* p_target, const Transform2D& p_desired);

protected:
    void _notification(int p_what);
    static void _bind_methods();
    // Turns the `part_name` field into an editor dropdown of the resolved
    // player's part names (editable, so a name still works when the player
    // can't be resolved at edit time). Same signature in module + godot-cpp.
    void _validate_property(PropertyInfo &p_property) const;

public:
    void set_part_name(const String& p_name);
    String get_part_name() const;

    void set_player_path(const NodePath& p_path);
    NodePath get_player_path() const;

    void set_remote_path(const NodePath& p_path);
    NodePath get_remote_path() const;

    void set_use_global_coordinates(bool p_enabled);
    bool get_use_global_coordinates() const;

    void set_update_position(bool p_enabled);
    bool get_update_position() const;
    void set_update_rotation(bool p_enabled);
    bool get_update_rotation() const;
    void set_update_scale(bool p_enabled);
    bool get_update_scale() const;

    void set_on_part_hidden(HiddenBehavior p_mode);
    HiddenBehavior get_on_part_hidden() const;

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    PackedStringArray _get_configuration_warnings() const override;
#else
    PackedStringArray get_configuration_warnings() const override;
#endif
};

VARIANT_ENUM_CAST(SpriteStudioPartAttachment2D::HiddenBehavior);
