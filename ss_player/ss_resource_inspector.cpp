#ifdef TOOLS_ENABLED

#include "ss_resource_inspector.h"

#include "ss_filesystem_menu.h"
#include "ss_importer.h"
#include "ss_macros.h"
#include "ss_player_node_2d.h"
#include "ssab_resource.h"
#include "ssqb_resource.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/button.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
using namespace godot;
#else
#include "core/config/project_settings.h"
#include "core/io/resource.h"
#include "core/os/os.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#endif

void SSResourceInspectorPlugin::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_open_pressed", "path"), &SSResourceInspectorPlugin::_on_open_pressed);
    ClassDB::bind_method(D_METHOD("_on_reconvert_pressed", "path"), &SSResourceInspectorPlugin::_on_reconvert_pressed);
    ClassDB::bind_method(D_METHOD("_on_reveal_pressed", "path"), &SSResourceInspectorPlugin::_on_reveal_pressed);
}

SSResourceInspectorPlugin::SSResourceInspectorPlugin() {
}

bool SSResourceInspectorPlugin::_is_unsupported_for_editor() const {
    String os_name = OS::get_singleton()->get_name();
    return os_name == "Linux";
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool SSResourceInspectorPlugin::_can_handle(Object *p_object) const {
#else
bool SSResourceInspectorPlugin::can_handle(Object *p_object) {
#endif
    return Object::cast_to<SSABResource>(p_object) != nullptr ||
           Object::cast_to<SSQBResource>(p_object) != nullptr ||
           Object::cast_to<SpriteStudioPlayer2D>(p_object) != nullptr;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
void SSResourceInspectorPlugin::_parse_begin(Object *p_object) {
#else
void SSResourceInspectorPlugin::parse_begin(Object *p_object) {
#endif
    String path;

    if (Resource *res = Object::cast_to<Resource>(p_object)) {
        path = res->get_path();
    } else if (SpriteStudioPlayer2D *player = Object::cast_to<SpriteStudioPlayer2D>(p_object)) {
        Ref<SSABResource> ssab = player->getSSABResource();
        if (ssab.is_valid()) {
            path = ssab->get_path();
        }
    }

    if (path.is_empty()) {
        // Either an unsaved resource or a player with no ssab assigned —
        // nothing actionable to wire up yet.
        return;
    }

    _add_action_buttons(path);
}

void SSResourceInspectorPlugin::_add_action_buttons(const String &p_path) {
    String sspj = importer ? importer->lookup_sspj_for_ssab(p_path) : String();
    String missing_tip = tr("No source SSPJ recorded yet — clicking will prompt to choose one.");

    HBoxContainer *hbox = memnew(HBoxContainer);

    if (!_is_unsupported_for_editor()) {
        Button *open_btn = memnew(Button);
        open_btn->set_text(tr("Open Source SSPJ"));
        open_btn->set_tooltip_text(sspj.is_empty() ? missing_tip : sspj);
        open_btn->connect("pressed", callable_mp(this, &SSResourceInspectorPlugin::_on_open_pressed).bind(p_path));
        hbox->add_child(open_btn);
    }

    Button *reconvert_btn = memnew(Button);
    reconvert_btn->set_text(tr("Reconvert"));
    reconvert_btn->set_tooltip_text(sspj.is_empty() ? missing_tip : tr("Reconvert from") + " " + sspj);
    reconvert_btn->connect("pressed", callable_mp(this, &SSResourceInspectorPlugin::_on_reconvert_pressed).bind(p_path));
    hbox->add_child(reconvert_btn);

    Button *reveal_btn = memnew(Button);
    reveal_btn->set_text(tr("Reveal"));
    reveal_btn->set_tooltip_text(tr("Show this file in the OS file manager."));
    reveal_btn->connect("pressed", callable_mp(this, &SSResourceInspectorPlugin::_on_reveal_pressed).bind(p_path));
    hbox->add_child(reveal_btn);

    add_custom_control(hbox);
}

void SSResourceInspectorPlugin::_on_open_pressed(const String &p_resource_path) {
    if (!context_menu) {
        return;
    }
    PackedStringArray paths;
    paths.push_back(p_resource_path);
    context_menu->_on_open_in_editor(paths);
}

void SSResourceInspectorPlugin::_on_reconvert_pressed(const String &p_resource_path) {
    if (!context_menu) {
        return;
    }
    PackedStringArray paths;
    paths.push_back(p_resource_path);
    context_menu->_on_convert(paths);
}

void SSResourceInspectorPlugin::_on_reveal_pressed(const String &p_resource_path) {
    String global = ProjectSettings::get_singleton()->globalize_path(p_resource_path);
    Error err = OS::get_singleton()->shell_show_in_file_manager(global, false);
    if (err != OK) {
        print_line(vformat("SSResourceInspectorPlugin: failed to reveal %s. error=%d", global, (int)err));
    }
}

#endif // #ifdef TOOLS_ENABLED
