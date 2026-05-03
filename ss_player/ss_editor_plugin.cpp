#ifdef TOOLS_ENABLED

#include "ss_editor_plugin.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
SSEditorPlugin::SSEditorPlugin() {
}
#else
#include "editor/editor_node.h"
SSEditorPlugin::SSEditorPlugin(EditorNode *node) {
}
#endif

void SSEditorPlugin::_notification(int what) {
    switch (what) {
        case NOTIFICATION_ENTER_TREE: {
            if (this->import_dock == nullptr) {
                this->import_dock = memnew(SSImportControl);
                this->import_dock->set_name(String::utf8("SSPJ"));
                add_control_to_dock(EditorPlugin::DOCK_SLOT_RIGHT_BL, this->import_dock, Ref<Shortcut>());
            }
        } break;

        case NOTIFICATION_EXIT_TREE: {
            if (this->import_dock) {
                remove_control_from_docks(this->import_dock);
                this->import_dock->queue_free();
                this->import_dock = nullptr;
            }
        } break;
    }
}

#endif // #ifdef TOOLS_ENABLED
