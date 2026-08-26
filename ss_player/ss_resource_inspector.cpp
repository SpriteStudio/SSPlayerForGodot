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
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/h_box_container.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/animation.hpp>
#include <godot_cpp/classes/animation_library.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/editor_file_system.hpp>
using namespace godot;
#else
#include "core/config/project_settings.h"
#include "core/io/resource.h"
#include "core/os/os.h"
#include "editor/editor_interface.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/resources/animation.h"
#include "scene/resources/animation_library.h"
#include "core/io/resource_saver.h"
#if VERSION_MAJOR >= 4
    #if VERSION_MINOR >= 5
    #include "editor/file_system/editor_file_system.h"
    #else
    #include "editor/editor_file_system.h"
    #endif
#else
    #include "editor/editor_file_system.h"
#endif
#endif

void SSResourceInspectorPlugin::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_on_open_pressed", "path"), &SSResourceInspectorPlugin::_on_open_pressed);
    ClassDB::bind_method(D_METHOD("_on_reconvert_pressed", "path"), &SSResourceInspectorPlugin::_on_reconvert_pressed);
    ClassDB::bind_method(D_METHOD("_on_generate_animation_library_pressed", "path"), &SSResourceInspectorPlugin::_on_generate_animation_library_pressed);
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

    Control *base = EditorInterface::get_singleton()->get_base_control();

    if (!_is_unsupported_for_editor()) {
        Button *open_btn = memnew(Button);
        open_btn->set_text(tr("Open SSPJ"));
        if (base) open_btn->set_button_icon(base->get_theme_icon(SNAME("Load"), SNAME("EditorIcons")));
        open_btn->set_tooltip_text(sspj.is_empty() ? missing_tip : sspj);
        open_btn->connect("pressed", callable_mp(this, &SSResourceInspectorPlugin::_on_open_pressed).bind(p_path));
        hbox->add_child(open_btn);
    }

    Button *reconvert_btn = memnew(Button);
    reconvert_btn->set_text(tr("Reconvert"));
    if (base) reconvert_btn->set_button_icon(base->get_theme_icon(SNAME("Reload"), SNAME("EditorIcons")));
    reconvert_btn->set_tooltip_text(sspj.is_empty() ? missing_tip : tr("Reconvert from") + " " + sspj);
    reconvert_btn->connect("pressed", callable_mp(this, &SSResourceInspectorPlugin::_on_reconvert_pressed).bind(p_path));
    hbox->add_child(reconvert_btn);

    Button *anim_btn = memnew(Button);
    anim_btn->set_text(tr("Gen AnimLib"));
    if (base) anim_btn->set_button_icon(base->get_theme_icon(SNAME("AnimationLibrary"), SNAME("EditorIcons")));
    anim_btn->set_tooltip_text(tr("Generate an AnimationLibrary resource from this SSAB for use with AnimationPlayer."));
    anim_btn->connect("pressed", callable_mp(this, &SSResourceInspectorPlugin::_on_generate_animation_library_pressed).bind(p_path));
    hbox->add_child(anim_btn);

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

void SSResourceInspectorPlugin::_on_generate_animation_library_pressed(const String &p_resource_path) {
    Ref<SSABResource> ssab;
    ssab.instantiate();
    if (ssab->load_from_file(p_resource_path) != OK) {
        ERR_PRINT("Failed to load SSAB for AnimationLibrary generation: " + p_resource_path);
        return;
    }

    Ref<AnimationLibrary> library;
    library.instantiate();

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    PackedStringArray anim_names = ssab->get_animation_names();
#else
    Vector<String> anim_names = ssab->get_animation_names();
#endif

    for (int i = 0; i < anim_names.size(); i++) {
        String anim_name = anim_names[i];
        ss::format::AnimationData *data = ssab->find_animation(anim_name);
        if (!data) continue;

        float fps = (float)data->fps();
        if (fps <= 0.0f) fps = 60.0f;
        int total_frames = data->total_frame();
        float length = (float)total_frames / fps;

        Ref<Animation> anim;
        anim.instantiate();
        anim->set_length(length);
        anim->set_step(1.0f / fps);

        // Track 0: the selected animation. The path is a PROPERTY path, so it has
        // to be the exported name -- a track pointing at a property that no longer
        // exists is not an error, it just silently drives nothing.
        int track_anim = anim->add_track(Animation::TYPE_VALUE);
        anim->track_set_path(track_anim, NodePath(".:current_animation"));
        anim->track_insert_key(track_anim, 0.0, anim_name);
        anim->value_track_set_update_mode(track_anim, Animation::UPDATE_DISCRETE);

        // Track 1: the playhead. Same story as above.
        int track_frame = anim->add_track(Animation::TYPE_VALUE);
        anim->track_set_path(track_frame, NodePath(".:frame_no"));
        anim->track_insert_key(track_frame, 0.0, 0.0f);
        // The player exposes frames 0..total_frame-1, and the last frame occupies
        // the final 1/fps slice of `length`. Keying the last VALID frame at its own
        // start time keeps the interpolated rate at exactly one frame per 1/fps and
        // holds it until the clip ends; keying `total_frames` at `length` instead
        // would push one frame past the animation and get clamped by the runtime.
        const int max_frame = total_frames - 1;
        if (max_frame > 0) {
            anim->track_insert_key(track_frame, (double)max_frame / fps, (float)max_frame);
        }

        anim->value_track_set_update_mode(track_frame, Animation::UPDATE_CONTINUOUS);
        anim->track_set_interpolation_type(track_frame, Animation::INTERPOLATION_LINEAR);

        library->add_animation(anim_name, anim);
    }

    String out_path = p_resource_path.get_basename() + "_anims.res";
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    Error err = ResourceSaver::get_singleton()->save(library, out_path);
#else
    Error err = ResourceSaver::save(library, out_path);
#endif

    if (err == OK) {
#if defined(SPRITESTUDIO_GODOT_EXTENSION) || (VERSION_MAJOR >= 4 && VERSION_MINOR >= 6)
        auto *efs = EditorInterface::get_singleton()->get_resource_filesystem();
#else
        auto *efs = EditorInterface::get_singleton()->get_resource_file_system();
#endif
        if (efs) efs->scan();
#ifdef SPRITESTUDIO_GODOT_EXTENSION
        UtilityFunctions::print("Generated AnimationLibrary: " + out_path);
#else
        print_line("Generated AnimationLibrary: " + out_path);
#endif
    } else {
        ERR_PRINT("Failed to save AnimationLibrary to " + out_path);
    }
}

#endif // #ifdef TOOLS_ENABLED
