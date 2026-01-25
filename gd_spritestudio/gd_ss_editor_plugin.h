#pragma once

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/editor_plugin.hpp>
using namespace godot;
#else
#include "editor/plugins/editor_plugin.h"
#include "editor/editor_node.h"
#endif

#include "gd_ss_import_dock.h"

class GdSsEditorPlugin : public EditorPlugin {
    GDCLASS(GdSsEditorPlugin, EditorPlugin)

    GdSsImportControl *import_dock = nullptr;

protected:
    static void _bind_methods() {}

public:
#ifdef SPRITESTUDIO_GODOT_EXTENSION
	explicit GdSsEditorPlugin();
#else
    void _notification(int what);
	explicit GdSsEditorPlugin(EditorNode *node);
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