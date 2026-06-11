#include "ss_part_attachment_2d.h"

#include "ss_player_node_2d.h"

void SpriteStudioPartAttachment2D::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_part_name", "part_name"), &SpriteStudioPartAttachment2D::set_part_name);
    ClassDB::bind_method(D_METHOD("get_part_name"), &SpriteStudioPartAttachment2D::get_part_name);

    ClassDB::bind_method(D_METHOD("set_player_path", "path"), &SpriteStudioPartAttachment2D::set_player_path);
    ClassDB::bind_method(D_METHOD("get_player_path"), &SpriteStudioPartAttachment2D::get_player_path);

    ClassDB::bind_method(D_METHOD("set_remote_path", "path"), &SpriteStudioPartAttachment2D::set_remote_path);
    ClassDB::bind_method(D_METHOD("get_remote_path"), &SpriteStudioPartAttachment2D::get_remote_path);

    ClassDB::bind_method(D_METHOD("set_use_global_coordinates", "enabled"), &SpriteStudioPartAttachment2D::set_use_global_coordinates);
    ClassDB::bind_method(D_METHOD("get_use_global_coordinates"), &SpriteStudioPartAttachment2D::get_use_global_coordinates);

    ClassDB::bind_method(D_METHOD("set_update_position", "enabled"), &SpriteStudioPartAttachment2D::set_update_position);
    ClassDB::bind_method(D_METHOD("get_update_position"), &SpriteStudioPartAttachment2D::get_update_position);
    ClassDB::bind_method(D_METHOD("set_update_rotation", "enabled"), &SpriteStudioPartAttachment2D::set_update_rotation);
    ClassDB::bind_method(D_METHOD("get_update_rotation"), &SpriteStudioPartAttachment2D::get_update_rotation);
    ClassDB::bind_method(D_METHOD("set_update_scale", "enabled"), &SpriteStudioPartAttachment2D::set_update_scale);
    ClassDB::bind_method(D_METHOD("get_update_scale"), &SpriteStudioPartAttachment2D::get_update_scale);

    ClassDB::bind_method(D_METHOD("set_on_part_hidden", "mode"), &SpriteStudioPartAttachment2D::set_on_part_hidden);
    ClassDB::bind_method(D_METHOD("get_on_part_hidden"), &SpriteStudioPartAttachment2D::get_on_part_hidden);

    ADD_PROPERTY(PropertyInfo(Variant::STRING, "part_name"), "set_part_name", "get_part_name");
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "player_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "SpriteStudioPlayer2D"), "set_player_path", "get_player_path");
    ADD_PROPERTY(PropertyInfo(Variant::NODE_PATH, "remote_path", PROPERTY_HINT_NODE_PATH_VALID_TYPES, "Node2D"), "set_remote_path", "get_remote_path");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "use_global_coordinates"), "set_use_global_coordinates", "get_use_global_coordinates");

    ADD_GROUP("Update", "update_");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "update_position"), "set_update_position", "get_update_position");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "update_rotation"), "set_update_rotation", "get_update_rotation");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "update_scale"), "set_update_scale", "get_update_scale");

    ADD_GROUP("", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "on_part_hidden", PROPERTY_HINT_ENUM, "Follow Always,Hide Target"), "set_on_part_hidden", "get_on_part_hidden");

    BIND_ENUM_CONSTANT(FOLLOW_ALWAYS);
    BIND_ENUM_CONSTANT(HIDE_TARGET);
}

SpriteStudioPlayer2D* SpriteStudioPartAttachment2D::_resolve_player() const {
    if (!_player_path.is_empty()) {
        Node* n = get_node_or_null(_player_path);
        return Object::cast_to<SpriteStudioPlayer2D>(n);
    }
    // No explicit path: walk up to the nearest ancestor player.
    Node* p = get_parent();
    while (p) {
        SpriteStudioPlayer2D* player = Object::cast_to<SpriteStudioPlayer2D>(p);
        if (player) return player;
        p = p->get_parent();
    }
    return nullptr;
}

Node2D* SpriteStudioPartAttachment2D::_resolve_target() {
    if (!_remote_path.is_empty()) {
        Node* n = get_node_or_null(_remote_path);
        return Object::cast_to<Node2D>(n);
    }
    return this;
}

void SpriteStudioPartAttachment2D::_connect_player() {
    SpriteStudioPlayer2D* player = _resolve_player();
    if (!player) return;
    Callable cb = callable_mp(this, &SpriteStudioPartAttachment2D::_on_player_frame_updated);
    if (!player->is_connected("frame_updated", cb)) {
        player->connect("frame_updated", cb);
    }
}

void SpriteStudioPartAttachment2D::_disconnect_player() {
    // Resolve via the current member state — callers update _player_path only
    // after disconnecting, so this finds the player we actually connected to.
    // A freed player auto-disconnects, so a null result here is safe to ignore.
    SpriteStudioPlayer2D* player = _resolve_player();
    if (!player) return;
    Callable cb = callable_mp(this, &SpriteStudioPartAttachment2D::_on_player_frame_updated);
    if (player->is_connected("frame_updated", cb)) {
        player->disconnect("frame_updated", cb);
    }
}

void SpriteStudioPartAttachment2D::_on_player_frame_updated(float frame_no) {
    if (_part_name.is_empty()) return;
    SpriteStudioPlayer2D* player = _resolve_player();
    if (!player) return;
    Node2D* target = _resolve_target();
    if (!target) return;

    int idx = player->get_part_index(_part_name);
    if (idx < 0) {
        // Part absent from this animation: nothing to follow, so hide the target.
        if (target->is_visible()) target->set_visible(false);
        return;
    }

    if (_on_part_hidden == HIDE_TARGET && player->is_part_hidden(_part_name)) {
        if (target->is_visible()) target->set_visible(false);
        return;
    }

    // Re-show if a previous frame auto-hid it (absent / hidden).
    if (!target->is_visible()) target->set_visible(true);

    const Transform2D part_local = player->get_part_transform(_part_name);
    const Transform2D desired = _use_global_coordinates
        ? (player->get_global_transform() * part_local)
        : part_local;
    _apply_transform(target, desired);
}

void SpriteStudioPartAttachment2D::_apply_transform(Node2D* p_target, const Transform2D& p_desired) {
    const bool global = _use_global_coordinates;

    // Full copy: assign the whole affine so skew / negative scale survive
    // (the reason a Godot Transform2D can mirror the part exactly).
    if (_update_position && _update_rotation && _update_scale) {
        if (global) {
            p_target->set_global_transform(p_desired);
        } else {
            p_target->set_transform(p_desired);
        }
        return;
    }

    // Partial copy: only the enabled components (skew is not preserved here,
    // matching RemoteTransform2D's component-wise behaviour).
    if (global) {
        if (_update_position) p_target->set_global_position(p_desired.get_origin());
        if (_update_rotation) p_target->set_global_rotation(p_desired.get_rotation());
        if (_update_scale) p_target->set_global_scale(p_desired.get_scale());
    } else {
        if (_update_position) p_target->set_position(p_desired.get_origin());
        if (_update_rotation) p_target->set_rotation(p_desired.get_rotation());
        if (_update_scale) p_target->set_scale(p_desired.get_scale());
    }
}

void SpriteStudioPartAttachment2D::set_part_name(const String& p_name) {
    _part_name = p_name;
    update_configuration_warnings();
}
String SpriteStudioPartAttachment2D::get_part_name() const { return _part_name; }

void SpriteStudioPartAttachment2D::set_player_path(const NodePath& p_path) {
    if (is_inside_tree()) _disconnect_player();
    _player_path = p_path;
    if (is_inside_tree()) _connect_player();
    update_configuration_warnings();
}
NodePath SpriteStudioPartAttachment2D::get_player_path() const { return _player_path; }

void SpriteStudioPartAttachment2D::set_remote_path(const NodePath& p_path) {
    _remote_path = p_path;
    update_configuration_warnings();
}
NodePath SpriteStudioPartAttachment2D::get_remote_path() const { return _remote_path; }

void SpriteStudioPartAttachment2D::set_use_global_coordinates(bool p_enabled) { _use_global_coordinates = p_enabled; }
bool SpriteStudioPartAttachment2D::get_use_global_coordinates() const { return _use_global_coordinates; }

void SpriteStudioPartAttachment2D::set_update_position(bool p_enabled) { _update_position = p_enabled; }
bool SpriteStudioPartAttachment2D::get_update_position() const { return _update_position; }
void SpriteStudioPartAttachment2D::set_update_rotation(bool p_enabled) { _update_rotation = p_enabled; }
bool SpriteStudioPartAttachment2D::get_update_rotation() const { return _update_rotation; }
void SpriteStudioPartAttachment2D::set_update_scale(bool p_enabled) { _update_scale = p_enabled; }
bool SpriteStudioPartAttachment2D::get_update_scale() const { return _update_scale; }

void SpriteStudioPartAttachment2D::set_on_part_hidden(HiddenBehavior p_mode) { _on_part_hidden = p_mode; }
SpriteStudioPartAttachment2D::HiddenBehavior SpriteStudioPartAttachment2D::get_on_part_hidden() const { return _on_part_hidden; }

void SpriteStudioPartAttachment2D::_validate_property(PropertyInfo &p_property) const {
    if (p_property.name == StringName("part_name")) {
        const SpriteStudioPlayer2D* player = _resolve_player();
        if (player) {
            PackedStringArray names = player->get_part_names();
            // ENUM_SUGGESTION (not ENUM) so the field stays editable when the
            // player isn't resolvable at edit time — never lock out a name.
            p_property.hint = PROPERTY_HINT_ENUM_SUGGESTION;
            p_property.hint_string = String(",").join(names);
        }
    }
}

void SpriteStudioPartAttachment2D::_notification(int p_what) {
    switch (p_what) {
        case NOTIFICATION_ENTER_TREE:
            // Covers the common case (player is an ancestor, already in tree)
            // and re-parenting.
            _connect_player();
            break;
        case NOTIFICATION_READY:
            // Retry once the whole tree exists, for a player_path that points
            // to a node outside this subtree (not yet present at ENTER_TREE).
            // Idempotent: _connect_player guards on is_connected.
            _connect_player();
            break;
        case NOTIFICATION_EXIT_TREE:
            _disconnect_player();
            break;
    }
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray SpriteStudioPartAttachment2D::_get_configuration_warnings() const {
    PackedStringArray warnings;
#else
PackedStringArray SpriteStudioPartAttachment2D::get_configuration_warnings() const {
    PackedStringArray warnings = Node2D::get_configuration_warnings();
#endif
    if (_resolve_player() == nullptr) {
        warnings.push_back(tr("No SpriteStudioPlayer2D found. Set \"player_path\" or place this node under a SpriteStudioPlayer2D."));
    } else if (_part_name.is_empty()) {
        warnings.push_back(tr("Set \"part_name\" to the SpriteStudio part this node should follow."));
    }
    return warnings;
}
