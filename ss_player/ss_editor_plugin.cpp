#ifdef TOOLS_ENABLED

#include "ss_editor_plugin.h"

#include "ss_player_node_2d.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
using namespace godot;
SSEditorPlugin::SSEditorPlugin() {
}
#else
#include "editor/editor_interface.h"
#include "editor/editor_node.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "scene/gui/button.h"
SSEditorPlugin::SSEditorPlugin(EditorNode *node) {
}
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
static Control *_find_control_by_class(Node *p_root, const String &p_class_name) {
    if (p_root == nullptr) {
        return nullptr;
    }
    if (p_root->get_class() == p_class_name) {
        Control *as_control = Object::cast_to<Control>(p_root);
        if (as_control != nullptr) {
            return as_control;
        }
    }
    int n = p_root->get_child_count();
    for (int i = 0; i < n; i++) {
        Control *found = _find_control_by_class(p_root->get_child(i), p_class_name);
        if (found != nullptr) {
            return found;
        }
    }
    return nullptr;
}
#endif

void SSEditorPlugin::_install_canvas_drop_overlay() {
    if (canvas_drop_overlay != nullptr) {
        return;
    }

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    Control *base = EditorInterface::get_singleton()->get_base_control();
    Control *viewport_control = _find_control_by_class(base, "CanvasItemEditorViewport");
#else
    CanvasItemEditor *ce = CanvasItemEditor::get_singleton();
    Control *viewport_control = ce ? ce->get_viewport_control() : nullptr;
#endif

    if (viewport_control == nullptr) {
        WARN_PRINT("SSEditorPlugin: CanvasItemEditorViewport not found; .ssab drop-to-viewport disabled.");
        return;
    }

    canvas_drop_overlay = memnew(SSCanvasDropOverlay);
    canvas_drop_overlay->set_name(String::utf8("SSCanvasDropOverlay"));
    viewport_control->add_child(canvas_drop_overlay);
}

void SSEditorPlugin::_remove_canvas_drop_overlay() {
    if (canvas_drop_overlay == nullptr) {
        return;
    }
    Node *parent = canvas_drop_overlay->get_parent();
    if (parent != nullptr) {
        parent->remove_child(canvas_drop_overlay);
    }
    canvas_drop_overlay->queue_free();
    canvas_drop_overlay = nullptr;
}

void SSEditorPlugin::_install_playback_panel() {
    if (playback_panel != nullptr) {
        return;
    }
    playback_panel = memnew(SSPlaybackPanel);
    playback_panel->set_name(String::utf8("SSPlayback"));
    playback_panel_button = add_control_to_bottom_panel(playback_panel, String::utf8("SpriteStudio"), Ref<Shortcut>());
    // Contextual: only reveal the tab while a SpriteStudioPlayer2D is selected.
    if (playback_panel_button) {
        playback_panel_button->hide();
    }
}

void SSEditorPlugin::_remove_playback_panel() {
    if (playback_panel == nullptr) {
        return;
    }
    remove_control_from_bottom_panel(playback_panel);
    playback_panel->queue_free();
    playback_panel = nullptr;
    playback_panel_button = nullptr;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool SSEditorPlugin::_handles(Object *p_object) const {
#else
bool SSEditorPlugin::handles(Object *p_object) const {
#endif
    return Object::cast_to<SpriteStudioPlayer2D>(p_object) != nullptr;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
void SSEditorPlugin::_edit(Object *p_object) {
#else
void SSEditorPlugin::edit(Object *p_object) {
#endif
    if (playback_panel) {
        playback_panel->set_player(Object::cast_to<SpriteStudioPlayer2D>(p_object));
    }
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
void SSEditorPlugin::_make_visible(bool p_visible) {
#else
void SSEditorPlugin::make_visible(bool p_visible) {
#endif
    if (p_visible) {
        if (playback_panel_button) {
            playback_panel_button->show();
        }
        if (playback_panel) {
            make_bottom_panel_item_visible(playback_panel);
        }
    } else {
        if (playback_panel_button) {
            playback_panel_button->hide();
        }
        if (playback_panel) {
            playback_panel->set_player(nullptr);
        }
    }
}

void SSEditorPlugin::_notification(int what) {
    switch (what) {
        case NOTIFICATION_ENTER_TREE: {
            if (this->importer == nullptr) {
                this->importer = memnew(SSImporter);
                this->importer->set_name(String::utf8("SSImporter"));
                add_child(this->importer);
            }

            if (this->import_dock == nullptr) {
                this->import_dock = memnew(SSImportControl);
                this->import_dock->set_name(String::utf8("SSPJ"));
                this->import_dock->set_importer(this->importer);
                add_control_to_dock(EditorPlugin::DOCK_SLOT_RIGHT_BL, this->import_dock, Ref<Shortcut>());
            }

            if (filesystem_menu.is_null()) {
                filesystem_menu.instantiate();
                filesystem_menu->set_importer(this->importer);
                add_context_menu_plugin(EditorContextMenuPlugin::CONTEXT_SLOT_FILESYSTEM, filesystem_menu);
            }

            if (inspector_plugin.is_null()) {
                inspector_plugin.instantiate();
                inspector_plugin->set_importer(this->importer);
                inspector_plugin->set_context_menu(filesystem_menu.ptr());
                add_inspector_plugin(inspector_plugin);
            }

            _install_canvas_drop_overlay();
            _install_playback_panel();
        } break;

        case NOTIFICATION_EXIT_TREE: {
            _remove_canvas_drop_overlay();
            _remove_playback_panel();

            if (inspector_plugin.is_valid()) {
                remove_inspector_plugin(inspector_plugin);
                inspector_plugin.unref();
            }
            if (filesystem_menu.is_valid()) {
                remove_context_menu_plugin(filesystem_menu);
                filesystem_menu.unref();
            }
            if (this->import_dock) {
                remove_control_from_docks(this->import_dock);
                this->import_dock->queue_free();
                this->import_dock = nullptr;
            }
            if (this->importer) {
                remove_child(this->importer);
                this->importer->queue_free();
                this->importer = nullptr;
            }
        } break;
    }
}

#endif // #ifdef TOOLS_ENABLED
