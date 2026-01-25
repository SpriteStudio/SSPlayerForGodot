#include "gd_ss_editor_plugin.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
GdSsEditorPlugin::GdSsEditorPlugin() {

}
#else
#include "editor/editor_node.h"
GdSsEditorPlugin::GdSsEditorPlugin(EditorNode *node) {
    this->import_dock = memnew(GdSsImportControl);
    import_dock->set_name(String::utf8("SSPJ Importer"));
    add_control_to_dock(DOCK_SLOT_RIGHT_UL, this->import_dock, Ref<Shortcut>());
}

void GdSsEditorPlugin::_notification(int what) {
    switch (what) {
        case NOTIFICATION_ENTER_TREE: {
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

#endif

