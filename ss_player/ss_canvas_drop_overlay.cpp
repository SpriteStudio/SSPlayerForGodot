#ifdef TOOLS_ENABLED

#include "ss_canvas_drop_overlay.h"

#include "ss_player_node_2d.h"
#include "ssab_resource.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/editor_selection.hpp>
#include <godot_cpp/classes/editor_undo_redo_manager.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
using namespace godot;
#else
#include "core/io/resource_loader.h"
#include "core/object/class_db.h"
#include "core/variant/dictionary.h"
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "scene/main/node.h"
#endif

namespace {

Ref<SSABResource> _load_ssab(const String &p_path) {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    Ref<Resource> res = ResourceLoader::get_singleton()->load(p_path);
#else
    Ref<Resource> res = ResourceLoader::load(p_path);
#endif
    Ref<SSABResource> ssab = res;
    if (ssab.is_null()) {
        ERR_PRINT(vformat("SSCanvasDropOverlay: failed to load SSAB '%s'.", p_path));
    }
    return ssab;
}

// ClassDB::instantiate lets SpriteStudioPlayer2D keep its ctor private and
// still get constructed from outside the class.
SpriteStudioPlayer2D *_make_player(const String &p_name) {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    Variant v = ClassDB::instantiate("SpriteStudioPlayer2D");
    SpriteStudioPlayer2D *player = Object::cast_to<SpriteStudioPlayer2D>((Object *)v);
#else
    SpriteStudioPlayer2D *player = Object::cast_to<SpriteStudioPlayer2D>(ClassDB::instantiate("SpriteStudioPlayer2D"));
#endif
    if (player == nullptr) {
        ERR_PRINT("SSCanvasDropOverlay: ClassDB::instantiate(SpriteStudioPlayer2D) returned null.");
        return nullptr;
    }
    player->set_name(p_name);
    return player;
}

// Dispatch EditorInterface::add_root_node across Custom Module / GDExtension.
// In GDExtension the method exists in extension_api.json but the current
// godot-cpp header has no generated wrapper, so we call by name.
void _call_add_root_node(EditorInterface *p_ei, Node *p_node) {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    p_ei->call("add_root_node", p_node);
#else
    p_ei->add_root_node(p_node);
#endif
}

// Walk the dropped paths until one loads and gets accepted as the scene root.
// EditorInterface::add_root_node has no symmetric undo hook reachable from
// here, so this step is committed directly; the scene is left in an unsaved
// state — matching what File > New Scene would do for a fresh root.
SpriteStudioPlayer2D *_promote_first_as_root(
        EditorInterface *p_ei,
        const PackedStringArray &p_ssab_paths,
        const Vector2 &p_world_pos,
        int &r_next_index) {
    for (int i = 0; i < p_ssab_paths.size(); i++) {
        const String &ssab_path = p_ssab_paths[i];
        Ref<SSABResource> ssab = _load_ssab(ssab_path);
        if (ssab.is_null()) {
            continue;
        }

        SpriteStudioPlayer2D *player = _make_player(ssab_path.get_file().get_basename());
        if (player == nullptr) {
            return nullptr;
        }
        player->setSSABResource(ssab);
        player->set_position(p_world_pos);

        _call_add_root_node(p_ei, player);

        Node *new_root = p_ei->get_edited_scene_root();
        if (new_root != player) {
            // add_root_node refused (e.g. a scene root materialized between
            // our initial null check and the call). Free the orphan. Using
            // queue_free instead of memdelete so SpriteStudioPlayer2D's
            // destructor can stay private.
            player->queue_free();
            return nullptr;
        }
        r_next_index = i + 1;
        return player;
    }
    return nullptr;
}

// Queue add_child/set_owner/set_ssab_resource/set_position for each remaining
// path under a single undoable action. Resource assignment is deferred into
// the do-step so render resources wire up while the node is already in the
// tree, matching what the Inspector does when the user sets the SSAB property
// by hand.
SpriteStudioPlayer2D *_add_children_with_undo(
        EditorUndoRedoManager *p_undo_redo,
        Node *p_parent,
        const PackedStringArray &p_ssab_paths,
        int p_start_index,
        const Vector2 &p_local_pos,
        const String &p_action_name) {
    SpriteStudioPlayer2D *last_added = nullptr;
    p_undo_redo->create_action(p_action_name);

    for (int i = p_start_index; i < p_ssab_paths.size(); i++) {
        const String &ssab_path = p_ssab_paths[i];
        Ref<SSABResource> ssab = _load_ssab(ssab_path);
        if (ssab.is_null()) {
            continue;
        }

        SpriteStudioPlayer2D *player = _make_player(ssab_path.get_file().get_basename());
        if (player == nullptr) {
            continue;
        }

        p_undo_redo->add_do_method(p_parent, "add_child", player, true);
        p_undo_redo->add_do_method(player, "set_owner", p_parent);
        p_undo_redo->add_do_method(player, "set_ssab_resource", ssab);
        p_undo_redo->add_do_method(player, "set_position", p_local_pos);
        p_undo_redo->add_do_reference(player);
        p_undo_redo->add_undo_method(p_parent, "remove_child", player);

        last_added = player;
    }

    p_undo_redo->commit_action();
    return last_added;
}

} // namespace

SSCanvasDropOverlay::SSCanvasDropOverlay() {
    set_mouse_filter(MOUSE_FILTER_PASS);
    set_anchors_and_offsets_preset(PRESET_FULL_RECT);
}

PackedStringArray SSCanvasDropOverlay::_extract_ssab_paths(const Variant &p_data) const {
    PackedStringArray result;
    if (p_data.get_type() != Variant::DICTIONARY) {
        return result;
    }
    Dictionary d = p_data;
    if (!d.has("type") || String(d["type"]) != "files") {
        return result;
    }
    PackedStringArray files = d["files"];
    for (int i = 0; i < files.size(); i++) {
        if (files[i].get_extension() == "ssab") {
            result.push_back(files[i]);
        }
    }
    return result;
}

Vector2 SSCanvasDropOverlay::_viewport_to_world(const Vector2 &p_at_position) const {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    // CanvasItemEditor::get_canvas_transform() is not bound to ClassDB, but the
    // SubViewport returned by EditorInterface::get_editor_viewport_2d() carries
    // the same matrix via Viewport::set_global_canvas_transform() (see
    // CanvasItemEditor::_draw_viewport in Godot's editor).
    SubViewport *vp = EditorInterface::get_singleton()->get_editor_viewport_2d();
    if (vp == nullptr) {
        return p_at_position;
    }
    return vp->get_global_canvas_transform().affine_inverse().xform(p_at_position);
#else
    CanvasItemEditor *ce = CanvasItemEditor::get_singleton();
    if (ce == nullptr) {
        return p_at_position;
    }
    return ce->get_canvas_transform().affine_inverse().xform(p_at_position);
#endif
}

void SSCanvasDropOverlay::_do_drop(const Vector2 &p_drop_pos, const Variant &p_data) {
    PackedStringArray ssab_paths = _extract_ssab_paths(p_data);
    if (ssab_paths.is_empty()) {
        return;
    }

    EditorInterface *ei = EditorInterface::get_singleton();
    EditorUndoRedoManager *undo_redo = ei->get_editor_undo_redo();
    if (undo_redo == nullptr) {
        return;
    }

    Vector2 world_pos = _viewport_to_world(p_drop_pos);

    Node *scene_root = ei->get_edited_scene_root();
    SpriteStudioPlayer2D *last_added = nullptr;
    int next_index = 0;

    if (scene_root == nullptr) {
        last_added = _promote_first_as_root(ei, ssab_paths, world_pos, next_index);
        if (last_added == nullptr) {
            return;
        }
        scene_root = ei->get_edited_scene_root();
    }

    if (next_index < ssab_paths.size()) {
        // Drop position is in world (canvas) coords, but Node2D::set_position
        // takes a parent-local value. Mirror what Godot's own canvas drop
        // does (CanvasItemEditorViewport::_create_*_node) so the dropped node
        // visually lands under the cursor even when the parent is offset.
        Vector2 local_pos = world_pos;
        if (Node2D *parent_2d = Object::cast_to<Node2D>(scene_root)) {
            local_pos = parent_2d->get_global_transform().affine_inverse().xform(world_pos);
        }
        SpriteStudioPlayer2D *child = _add_children_with_undo(
                undo_redo, scene_root, ssab_paths, next_index, local_pos,
                tr("Add SpriteStudioPlayer2D"));
        if (child != nullptr) {
            last_added = child;
        }
    }

    if (last_added != nullptr) {
        EditorSelection *sel = ei->get_selection();
        if (sel != nullptr) {
            sel->clear();
            sel->add_node(last_added);
        }
    }
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool SSCanvasDropOverlay::_can_drop_data(const Vector2 &p_at_position, const Variant &p_data) const {
#else
bool SSCanvasDropOverlay::can_drop_data(const Point2 &p_point, const Variant &p_data) const {
#endif
    return !_extract_ssab_paths(p_data).is_empty();
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
void SSCanvasDropOverlay::_drop_data(const Vector2 &p_at_position, const Variant &p_data) {
    _do_drop(p_at_position, p_data);
}
#else
void SSCanvasDropOverlay::drop_data(const Point2 &p_point, const Variant &p_data) {
    _do_drop(p_point, p_data);
}
#endif

#endif // TOOLS_ENABLED
