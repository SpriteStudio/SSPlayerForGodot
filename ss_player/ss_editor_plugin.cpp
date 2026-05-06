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
        } break;

        case NOTIFICATION_EXIT_TREE: {
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
