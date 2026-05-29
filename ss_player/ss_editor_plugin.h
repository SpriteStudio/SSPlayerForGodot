#pragma once

#ifdef TOOLS_ENABLED

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/editor_plugin.hpp>
using namespace godot;
#else
#include "editor/plugins/editor_plugin.h"
#include "editor/editor_node.h"
#endif

#include "ss_canvas_drop_overlay.h"
#include "ss_filesystem_menu.h"
#include "ss_import_dock.h"
#include "ss_importer.h"
#include "ss_playback_panel.h"
#include "ss_resource_inspector.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
namespace godot {
class Button;
}
#else
class Button;
#endif

class SSEditorPlugin : public EditorPlugin {
    GDCLASS(SSEditorPlugin, EditorPlugin)

    SSImportControl *import_dock = nullptr;
    SSImporter *importer = nullptr;
    Ref<SSFileSystemContextMenu> filesystem_menu;
    Ref<SSResourceInspectorPlugin> inspector_plugin;
    SSCanvasDropOverlay *canvas_drop_overlay = nullptr;

    // Bottom-panel transport UI (AnimationPlayer-style) shown only while a
    // SpriteStudioPlayer2D is selected. `playback_panel_button` is the tab
    // toggle returned by add_control_to_bottom_panel; we hide/show it to make
    // the panel contextual.
    SSPlaybackPanel *playback_panel = nullptr;
    Button *playback_panel_button = nullptr;

    void _install_canvas_drop_overlay();
    void _remove_canvas_drop_overlay();
    void _install_playback_panel();
    void _remove_playback_panel();

protected:
    static void _bind_methods() {}
    void _notification(int what);

public:
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    explicit SSEditorPlugin();
#else
    explicit SSEditorPlugin(EditorNode *node);
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    String _get_plugin_name() const override {
        return "SpriteStudioEditorPlugin";
    }

    bool _handles(Object *p_object) const override;
    void _edit(Object *p_object) override;
    void _make_visible(bool p_visible) override;
#else
#if VERSION_MAJOR > 3 && VERSION_MINOR > 3
    String get_plugin_name() const override {
        return "SpriteStudioEditorPlugin";
    }
#else
    String get_name() const override {
        return "SpriteStudioEditorPlugin";
    }
#endif

    bool handles(Object *p_object) const override;
    void edit(Object *p_object) override;
    void make_visible(bool p_visible) override;
#endif
};

#endif // #ifdef TOOLS_ENABLED
