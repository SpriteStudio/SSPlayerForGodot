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
#include "ss_resource_inspector.h"

class SSEditorPlugin : public EditorPlugin {
    GDCLASS(SSEditorPlugin, EditorPlugin)

    SSImportControl *import_dock = nullptr;
    SSImporter *importer = nullptr;
    Ref<SSFileSystemContextMenu> filesystem_menu;
    Ref<SSResourceInspectorPlugin> inspector_plugin;
    SSCanvasDropOverlay *canvas_drop_overlay = nullptr;

    void _install_canvas_drop_overlay();
    void _remove_canvas_drop_overlay();

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
#endif
};

#endif // #ifdef TOOLS_ENABLED
