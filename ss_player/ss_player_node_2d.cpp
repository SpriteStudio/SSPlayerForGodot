#include "ss_player_node_2d.h"
#include "ss_update_manager.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/engine.hpp>
#else
#include "core/config/engine.h"
#endif

class SpriteStudioPlayer2D::_SignalSink : public SsPlayerEventSink {
public:
    explicit _SignalSink(SpriteStudioPlayer2D* p_owner) : _owner(p_owner) {}

    void onAnimationStarted(const String& anim_name) override {
        _owner->emit_signal(SNAME("animation_started"), anim_name);
    }
    void onAnimationChanged(const String& anim_name) override {
        _owner->emit_signal(SNAME("animation_changed"), anim_name);
    }
    void onAnimationFinished(const String& anim_name) override {
        _owner->emit_signal(SNAME("animation_finished"), anim_name);
    }
    void onAnimationLooped(const String& anim_name) override {
        _owner->emit_signal(SNAME("animation_looped"), anim_name);
    }
    void onUserData(const Dictionary& payload) override {
        _owner->emit_signal(SNAME("user_data"), payload);
    }
    void onSignal(const String& command, const Dictionary& value) override {
        _owner->emit_signal(SNAME("signal_emitted"), command, value);
    }

private:
    SpriteStudioPlayer2D* _owner;
};


SpriteStudioPlayer2D::SpriteStudioPlayer2D() {
    _internal = memnew(SsInternalPlayer);
    _sink = memnew(_SignalSink(this));
    _internal->setEventSink(_sink);
    _internal->setSkipFrames(true);
    _internal->setSubFrameEnabled(false);
    SsUpdateManager::get().register_player(this);
}

SpriteStudioPlayer2D::~SpriteStudioPlayer2D() {
    SsUpdateManager::get().unregister_player(this);
    if (_internal) {
        _internal->setEventSink(nullptr);
        memdelete(_internal);
        _internal = nullptr;
    }
    if (_sink) {
        memdelete(_sink);
        _sink = nullptr;
    }
}

void SpriteStudioPlayer2D::setSSABResource(const Ref<SSABResource>& ssabRes) {
    Callable changed_cb = callable_mp(this, &SpriteStudioPlayer2D::_on_ssab_changed);
    Ref<SSABResource> prev = _internal->getSSABResource();
    if (prev.is_valid() && prev->is_connected("changed", changed_cb)) {
        prev->disconnect("changed", changed_cb);
    }

    _internal->setSSABResource(ssabRes);

    NOTIFY_PROPERTY_LIST_CHANGED();
    update_configuration_warnings();

    Ref<SSABResource> now = _internal->getSSABResource();
    if (now.is_valid()) {
        if (!now->is_connected("changed", changed_cb)) {
            now->connect("changed", changed_cb);
        }
        // Apply existing overrides to the new resource
        for (const KeyValue<String, Ref<Texture2D>> &E : _cellmap_overrides) {
            uint32_t hash = now->get_cellmap_hash(E.key);
            _internal->setCellMapOverrideTexture(hash, E.value);
        }
    }
}

void SpriteStudioPlayer2D::_on_ssab_changed() {
    _internal->onSSABReloaded();
    // Re-apply overrides after reload
    Ref<SSABResource> res = getSSABResource();
    if (res.is_valid()) {
        for (const KeyValue<String, Ref<Texture2D>> &E : _cellmap_overrides) {
            uint32_t hash = res->get_cellmap_hash(E.key);
            _internal->setCellMapOverrideTexture(hash, E.value);
        }
    }
    NOTIFY_PROPERTY_LIST_CHANGED();
    update_configuration_warnings();
}

Ref<SSABResource> SpriteStudioPlayer2D::getSSABResource() const {
    return _internal->getSSABResource();
}

void SpriteStudioPlayer2D::set_cellmap_texture(const String &cellmap_name, const Ref<Texture2D> &texture) {
    if (texture.is_valid()) {
        _cellmap_overrides[cellmap_name] = texture;
    } else {
        _cellmap_overrides.erase(cellmap_name);
    }

    Ref<SSABResource> res = getSSABResource();
    if (res.is_valid()) {
        uint32_t hash = res->get_cellmap_hash(cellmap_name);
        _internal->setCellMapOverrideTexture(hash, texture);
    }
}

Ref<Texture2D> SpriteStudioPlayer2D::get_cellmap_texture(const String &cellmap_name) const {
    if (_cellmap_overrides.has(cellmap_name)) {
        return _cellmap_overrides[cellmap_name];
    }
    return Ref<Texture2D>();
}

void SpriteStudioPlayer2D::setAnimation(const String& strName) {
    _internal->setAnimation(strName);
    NOTIFY_PROPERTY_LIST_CHANGED();
    update_configuration_warnings();
}

String SpriteStudioPlayer2D::getAnimation() const {
    return _internal->getAnimation();
}

void SpriteStudioPlayer2D::setAutoplay(bool p_autoplay) {
    _autoplay = p_autoplay;
}

bool SpriteStudioPlayer2D::isAutoplay() const {
    return _autoplay;
}

bool SpriteStudioPlayer2D::isPlaying() const { return _internal->isPlaying(); }
void SpriteStudioPlayer2D::play(float p_start_frame) { _internal->play(p_start_frame); }
bool SpriteStudioPlayer2D::isPausing() const { return _internal->isPausing(); }
void SpriteStudioPlayer2D::pause() { _internal->pause(); }
void SpriteStudioPlayer2D::stop() { _internal->stop(); }

void SpriteStudioPlayer2D::set_flip_h(bool p_flip) {
    _flip_h = p_flip;
    _update_root_transform();
}

bool SpriteStudioPlayer2D::is_flipped_h() const { return _flip_h; }

void SpriteStudioPlayer2D::set_flip_v(bool p_flip) {
    _flip_v = p_flip;
    _update_root_transform();
}

bool SpriteStudioPlayer2D::is_flipped_v() const { return _flip_v; }

void SpriteStudioPlayer2D::set_offset(const Vector2& p_offset) {
    _offset = p_offset;
    _update_root_transform();
}

Vector2 SpriteStudioPlayer2D::get_offset() const { return _offset; }

void SpriteStudioPlayer2D::_update_root_transform() {
    Transform2D xform;
    xform.columns[0].x = _flip_h ? -1.0 : 1.0;
    xform.columns[1].y = _flip_v ? -1.0 : 1.0;
    xform.columns[2] = _offset;
    _internal->setRootTransform(xform);
}

void SpriteStudioPlayer2D::_push_coverage_screen_scale() {
    // local-unit -> viewport-pixel scale = node global transform composed with
    // the viewport/camera transform. The mask coverage pass sizes its target to
    // the mask's on-screen footprint using this, so off-screen-small players use
    // small coverage textures. One frame of staleness on a zoom only nudges the
    // chosen size class, so reading it here (before update) is fine.
    const Vector2 sc = get_global_transform_with_canvas().get_scale();
    const float sx = sc.x < 0.0f ? -sc.x : sc.x;
    const float sy = sc.y < 0.0f ? -sc.y : sc.y;
    _internal->setCoverageScreenScale(sx > sy ? sx : sy);
}

void SpriteStudioPlayer2D::set_animation_process_mode(int p_mode) {
    AnimationProcessMode mode = (AnimationProcessMode)p_mode;
    if (_process_mode == mode) return;
    
    bool active = is_inside_tree();
    if (active) {
        if (_process_mode == ANIMATION_PROCESS_IDLE) {
            set_process_internal(false);
        } else {
            set_physics_process_internal(false);
        }
    }
    
    _process_mode = mode;
    
    if (active) {
        if (_process_mode == ANIMATION_PROCESS_IDLE) {
            set_process_internal(true);
        } else {
            set_physics_process_internal(true);
        }
    }
}

int SpriteStudioPlayer2D::get_animation_process_mode() const { return (int)_process_mode; }

void SpriteStudioPlayer2D::setSpeedScale(float p_speed) { _internal->setSpeed(p_speed); }
float SpriteStudioPlayer2D::getSpeedScale() const { return _internal->getSpeed(); }

void SpriteStudioPlayer2D::setFrame(float p_frame) { _internal->setFrame(p_frame); }
float SpriteStudioPlayer2D::getFrame() const { return _internal->getFrame(); }

int SpriteStudioPlayer2D::getTotalFrames() const { return _internal->getTotalFrames(); }

void SpriteStudioPlayer2D::setFrameRate(int p_fps) { _internal->setFrameRate(p_fps); }
int SpriteStudioPlayer2D::getFrameRate() const { return _internal->getFrameRate(); }

int SpriteStudioPlayer2D::getStartFrame() const { return _internal->getAnimationSectionStart(); }
int SpriteStudioPlayer2D::getEndFrame() const { return _internal->getAnimationSectionEnd(); }

void SpriteStudioPlayer2D::setAnimationSection(int p_start, int p_end) { _internal->setAnimationSection(p_start, p_end); }
int SpriteStudioPlayer2D::getAnimationSectionStart() const { return _internal->getAnimationSectionStart(); }
int SpriteStudioPlayer2D::getAnimationSectionEnd() const { return _internal->getAnimationSectionEnd(); }

void SpriteStudioPlayer2D::setPlaybackDirection(int p_direction, int p_style) { _internal->setPlaybackDirection(p_direction, p_style); }
int SpriteStudioPlayer2D::getPlaybackDirection() const { return _internal->getPlaybackDirection(); }
int SpriteStudioPlayer2D::getPlaybackStyle() const { return _internal->getPlaybackStyle(); }

void SpriteStudioPlayer2D::setLoopCount(int p_count) { _internal->setLoop(p_count); }
int SpriteStudioPlayer2D::getLoopCount() const { return _internal->getLoop(); }

void SpriteStudioPlayer2D::setFrameSkipEnabled(bool p_skip) { _internal->setSkipFrames(p_skip); }
bool SpriteStudioPlayer2D::isFrameSkipEnabled() const { return _internal->isSkipFrames(); }

void SpriteStudioPlayer2D::setSubFrameEnabled(bool p_enabled) { _internal->setSubFrameEnabled(p_enabled); }
bool SpriteStudioPlayer2D::isSubFrameEnabled() const { return _internal->isSubFrameEnabled(); }


void SpriteStudioPlayer2D::_bind_methods() {
    ClassDB::bind_method( D_METHOD( "set_ssab_resource", "res_ssab" ), &SpriteStudioPlayer2D::setSSABResource );
    ClassDB::bind_method( D_METHOD( "get_ssab_resource" ), &SpriteStudioPlayer2D::getSSABResource );
    ClassDB::bind_method( D_METHOD( "set_animation", "name" ), &SpriteStudioPlayer2D::setAnimation );
    ClassDB::bind_method( D_METHOD( "get_animation" ), &SpriteStudioPlayer2D::getAnimation );

    ClassDB::bind_method( D_METHOD( "set_autoplay", "autoplay" ), &SpriteStudioPlayer2D::setAutoplay );
    ClassDB::bind_method( D_METHOD( "is_autoplay" ), &SpriteStudioPlayer2D::isAutoplay );

    ClassDB::bind_method( D_METHOD( "is_playing" ), &SpriteStudioPlayer2D::isPlaying );
    ClassDB::bind_method( D_METHOD( "play", "start_frame" ), &SpriteStudioPlayer2D::play, DEFVAL(-1.0f) );
    ClassDB::bind_method( D_METHOD( "is_pausing" ), &SpriteStudioPlayer2D::isPausing );
    ClassDB::bind_method( D_METHOD( "pause" ), &SpriteStudioPlayer2D::pause );
    ClassDB::bind_method( D_METHOD( "stop" ), &SpriteStudioPlayer2D::stop );

    ClassDB::bind_method( D_METHOD( "set_speed_scale", "speed_scale" ), &SpriteStudioPlayer2D::setSpeedScale );
    ClassDB::bind_method( D_METHOD( "get_speed_scale" ), &SpriteStudioPlayer2D::getSpeedScale );
    ClassDB::bind_method( D_METHOD( "set_frame", "frame" ), &SpriteStudioPlayer2D::setFrame );
    ClassDB::bind_method( D_METHOD( "get_frame" ), &SpriteStudioPlayer2D::getFrame );

    ClassDB::bind_method( D_METHOD( "get_total_frames" ), &SpriteStudioPlayer2D::getTotalFrames );
    ClassDB::bind_method( D_METHOD( "get_start_frame" ), &SpriteStudioPlayer2D::getStartFrame );
    ClassDB::bind_method( D_METHOD( "get_end_frame" ), &SpriteStudioPlayer2D::getEndFrame );

    ClassDB::bind_method( D_METHOD( "set_frame_rate", "fps" ), &SpriteStudioPlayer2D::setFrameRate );
    ClassDB::bind_method( D_METHOD( "get_frame_rate" ), &SpriteStudioPlayer2D::getFrameRate );

    ClassDB::bind_method( D_METHOD( "set_animation_section", "start", "end" ), &SpriteStudioPlayer2D::setAnimationSection );
    ClassDB::bind_method( D_METHOD( "get_animation_section_start" ), &SpriteStudioPlayer2D::getAnimationSectionStart );
    ClassDB::bind_method( D_METHOD( "get_animation_section_end" ), &SpriteStudioPlayer2D::getAnimationSectionEnd );

    ClassDB::bind_method( D_METHOD( "set_playback_direction", "direction", "style" ), &SpriteStudioPlayer2D::setPlaybackDirection );
    ClassDB::bind_method( D_METHOD( "get_playback_direction" ), &SpriteStudioPlayer2D::getPlaybackDirection );
    ClassDB::bind_method( D_METHOD( "get_playback_style" ), &SpriteStudioPlayer2D::getPlaybackStyle );

    ClassDB::bind_method( D_METHOD( "set_loop_count", "count" ), &SpriteStudioPlayer2D::setLoopCount );
    ClassDB::bind_method( D_METHOD( "get_loop_count" ), &SpriteStudioPlayer2D::getLoopCount );

    ClassDB::bind_method( D_METHOD( "set_frame_skip_enabled", "enabled" ), &SpriteStudioPlayer2D::setFrameSkipEnabled );
    ClassDB::bind_method( D_METHOD( "is_frame_skip_enabled" ), &SpriteStudioPlayer2D::isFrameSkipEnabled );

    ClassDB::bind_method( D_METHOD( "set_sub_frame_enabled", "enabled" ), &SpriteStudioPlayer2D::setSubFrameEnabled );
    ClassDB::bind_method( D_METHOD( "is_sub_frame_enabled" ), &SpriteStudioPlayer2D::isSubFrameEnabled );

    ClassDB::bind_method( D_METHOD( "set_cellmap_texture", "cellmap_name", "texture" ), &SpriteStudioPlayer2D::set_cellmap_texture );
    ClassDB::bind_method( D_METHOD( "get_cellmap_texture", "cellmap_name" ), &SpriteStudioPlayer2D::get_cellmap_texture );

    ClassDB::bind_method( D_METHOD( "set_offset", "offset" ), &SpriteStudioPlayer2D::set_offset );
    ClassDB::bind_method( D_METHOD( "get_offset" ), &SpriteStudioPlayer2D::get_offset );

    BIND_CONSTANT( ANIMATION_PROCESS_PHYSICS );
    BIND_CONSTANT( ANIMATION_PROCESS_IDLE );
    
    ClassDB::bind_method( D_METHOD( "set_animation_process_mode", "mode" ), &SpriteStudioPlayer2D::set_animation_process_mode );
    ClassDB::bind_method( D_METHOD( "get_animation_process_mode" ), &SpriteStudioPlayer2D::get_animation_process_mode );

    ClassDB::bind_method( D_METHOD( "set_flip_h", "flip_h" ), &SpriteStudioPlayer2D::set_flip_h );
    ClassDB::bind_method( D_METHOD( "is_flipped_h" ), &SpriteStudioPlayer2D::is_flipped_h );
    ClassDB::bind_method( D_METHOD( "set_flip_v", "flip_v" ), &SpriteStudioPlayer2D::set_flip_v );
    ClassDB::bind_method( D_METHOD( "is_flipped_v" ), &SpriteStudioPlayer2D::is_flipped_v );

    ADD_SIGNAL(
        MethodInfo(
            "user_data",
            PropertyInfo(Variant::DICTIONARY, "payload")
        )
    );
    ADD_SIGNAL(
        MethodInfo(
            "signal_emitted",
            PropertyInfo(Variant::STRING, "command"),
            PropertyInfo(Variant::DICTIONARY, "value")
        )
    );

    ADD_SIGNAL(MethodInfo("animation_changed", PropertyInfo(Variant::STRING, "anim_name")));
    ADD_SIGNAL(MethodInfo("animation_started", PropertyInfo(Variant::STRING, "anim_name")));
    ADD_SIGNAL(MethodInfo("animation_finished", PropertyInfo(Variant::STRING, "anim_name")));
    ADD_SIGNAL(MethodInfo("animation_looped", PropertyInfo(Variant::STRING, "anim_name")));

    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "ssab", PROPERTY_HINT_RESOURCE_TYPE, "SSABResource"),
        "set_ssab_resource",
        "get_ssab_resource"
    );
}

bool SpriteStudioPlayer2D::_set(const StringName& p_name, const Variant& p_property) {
    String name = p_name;
    if (name == "animation") {
        setAnimation(p_property);
        return true;
    } else if (name == "autoplay") {
        setAutoplay(p_property);
        return true;
    } else if (name == "animation_process_mode") {
        set_animation_process_mode((int)p_property);
        return true;
    } else if (name == "frame") {
        setFrame(p_property);
        return true;
    } else if (name == "loop_count") {
        setLoopCount(p_property);
        return true;
    } else if (name == "speed_scale") {
        setSpeedScale(p_property);
        return true;
    } else if (name == "frame_skip_enabled") {
        setFrameSkipEnabled(p_property);
        return true;
    } else if (name == "sub_frame_enabled") {
        setSubFrameEnabled(p_property);
        return true;
    } else if (name == "playback_direction") {
        setPlaybackDirection((int)p_property, getPlaybackStyle());
        return true;
    } else if (name == "playback_style") {
        setPlaybackDirection(getPlaybackDirection(), (int)p_property);
        return true;
    } else if (name == "offset") {
        set_offset(p_property);
        return true;
    } else if (name == "flip_h") {
        set_flip_h(p_property);
        return true;
    } else if (name == "flip_v") {
        set_flip_v(p_property);
        return true;
    } else if (name == "frame_rate") {
        setFrameRate(p_property);
        return true;
    } else if (name == "animation_section_start") {
        setAnimationSection((int)p_property, getAnimationSectionEnd());
        return true;
    } else if (name == "animation_section_end") {
        setAnimationSection(getAnimationSectionStart(), (int)p_property);
        return true;
    }

    if (name.begins_with("cellmaps/")) {
        String cellmap_name = name.trim_prefix("cellmaps/");
        set_cellmap_texture(cellmap_name, p_property);
        return true;
    }

    return false;
}

bool SpriteStudioPlayer2D::_get(const StringName& p_name, Variant& r_property) const {
    String name = p_name;
    if (name == "animation") {
        r_property = getAnimation();
        return true;
    } else if (name == "autoplay") {
        r_property = isAutoplay();
        return true;
    } else if (name == "animation_process_mode") {
        r_property = get_animation_process_mode();
        return true;
    } else if (name == "frame") {
        r_property = getFrame();
        return true;
    } else if (name == "loop_count") {
        r_property = getLoopCount();
        return true;
    } else if (name == "speed_scale") {
        r_property = getSpeedScale();
        return true;
    } else if (name == "frame_skip_enabled") {
        r_property = isFrameSkipEnabled();
        return true;
    } else if (name == "sub_frame_enabled") {
        r_property = isSubFrameEnabled();
        return true;
    } else if (name == "playback_direction") {
        r_property = getPlaybackDirection();
        return true;
    } else if (name == "playback_style") {
        r_property = getPlaybackStyle();
        return true;
    } else if (name == "offset") {
        r_property = get_offset();
        return true;
    } else if (name == "flip_h") {
        r_property = is_flipped_h();
        return true;
    } else if (name == "flip_v") {
        r_property = is_flipped_v();
        return true;
    } else if (name == "frame_rate") {
        r_property = getFrameRate();
        return true;
    } else if (name == "animation_section_start") {
        r_property = getAnimationSectionStart();
        return true;
    } else if (name == "animation_section_end") {
        r_property = getAnimationSectionEnd();
        return true;
    }

    if (name.begins_with("cellmaps/")) {
        String cellmap_name = name.trim_prefix("cellmaps/");
        r_property = get_cellmap_texture(cellmap_name);
        return true;
    }

    return false;
}

void SpriteStudioPlayer2D::_get_property_list(List<PropertyInfo>* p_list) const {
    Ref<SSABResource> res = _internal->getSSABResource();
    bool has_res = !res.is_null();

    if (has_res) {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
        PackedStringArray anim_names = res->get_animation_names();
#else
        Vector<String> anim_names = res->get_animation_names();
#endif
        p_list->push_back(PropertyInfo(Variant::STRING, "animation", PROPERTY_HINT_ENUM, String(",").join(anim_names)));
    } else {
        p_list->push_back(PropertyInfo(Variant::STRING, "animation", PROPERTY_HINT_ENUM, ""));
    }

    int total = getTotalFrames();
    int max_frame = total > 0 ? total - 1 : 0;
    p_list->push_back(PropertyInfo(Variant::BOOL, "autoplay"));
    p_list->push_back(PropertyInfo(Variant::FLOAT, "frame", PROPERTY_HINT_RANGE, "0," + String::num(max_frame) + ",0.01", PROPERTY_USAGE_EDITOR));

    p_list->push_back(PropertyInfo(Variant::INT, "loop_count", PROPERTY_HINT_RANGE, "-1,9999,1,or_greater"));
    p_list->push_back(PropertyInfo(Variant::FLOAT, "speed_scale", PROPERTY_HINT_RANGE, "0,4,0.01,or_greater"));

    p_list->push_back(PropertyInfo(Variant::NIL, "Playback Options", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_GROUP));
    p_list->push_back(PropertyInfo(Variant::BOOL, "frame_skip_enabled"));
    p_list->push_back(PropertyInfo(Variant::BOOL, "sub_frame_enabled"));
    p_list->push_back(PropertyInfo(Variant::INT, "animation_process_mode", PROPERTY_HINT_ENUM, "Physics,Idle"));

    String section_range = "0," + String::num(max_frame) + ",1";
    p_list->push_back(PropertyInfo(Variant::NIL, "Section", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_GROUP));
    p_list->push_back(PropertyInfo(Variant::INT, "animation_section_start", PROPERTY_HINT_RANGE, section_range));
    p_list->push_back(PropertyInfo(Variant::INT, "animation_section_end", PROPERTY_HINT_RANGE, section_range));

    p_list->push_back(PropertyInfo(Variant::NIL, "Offset", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_GROUP));
    p_list->push_back(PropertyInfo(Variant::VECTOR2, "offset"));
    p_list->push_back(PropertyInfo(Variant::BOOL, "flip_h"));
    p_list->push_back(PropertyInfo(Variant::BOOL, "flip_v"));

    if (has_res) {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
        PackedStringArray cellmap_names = res->get_cellmap_names();
#else
        Vector<String> cellmap_names = res->get_cellmap_names();
#endif
        if (cellmap_names.size() > 0) {
            p_list->push_back(PropertyInfo(Variant::NIL, "CellMap Overrides", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_GROUP));
            for (int i = 0; i < cellmap_names.size(); i++) {
                p_list->push_back(PropertyInfo(Variant::OBJECT, "cellmaps/" + cellmap_names[i], PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"));
            }
        }
    }
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray SpriteStudioPlayer2D::_get_configuration_warnings() const {
    PackedStringArray warnings;
#else
PackedStringArray SpriteStudioPlayer2D::get_configuration_warnings() const {
    PackedStringArray warnings = Node2D::get_configuration_warnings();
#endif
    if (getSSABResource().is_null()) {
        warnings.push_back(tr("Assign an SSABResource to the \"ssab\" property to play an animation."));
    } else if (getAnimation().is_empty()) {
        warnings.push_back(tr("Select an animation in the \"animation\" property."));
    }
    return warnings;
}

void SpriteStudioPlayer2D::_notification(int p_notification) {
    switch (p_notification) {
        case NOTIFICATION_READY:
            if (!Engine::get_singleton()->is_editor_hint()) {
                if (_autoplay) {
                    play();
                }
            }
            break;
        case NOTIFICATION_ENTER_TREE:
            // Re-parent the InternalPlayer's root canvas item to ours so the
            // Node2D's transform / visibility / Z-order propagate. Also done
            // here so editor reloads (which destroy / reattach the canvas)
            // don't leave the InternalPlayer floating.
            _internal->setParentCanvasItem(get_canvas_item());
            if (_process_mode == ANIMATION_PROCESS_IDLE) {
                set_process_internal(true);
            } else {
                set_physics_process_internal(true);
            }
            break;
        case NOTIFICATION_EXIT_TREE:
            _internal->setParentCanvasItem(RID());
            break;
        case NOTIFICATION_INTERNAL_PROCESS:
            if (_process_mode == ANIMATION_PROCESS_IDLE) {
                SsUpdateManager::get().update_all(get_process_delta_time(), false);
            }
            break;
        case NOTIFICATION_INTERNAL_PHYSICS_PROCESS:
            if (_process_mode == ANIMATION_PROCESS_PHYSICS) {
                SsUpdateManager::get().update_all(get_physics_process_delta_time(), true);
            }
            break;
        case NOTIFICATION_DRAW:
            // The InternalPlayer handles the actual RenderingServer calls for
            // its per-batch canvas items, but they are nested under our
            // get_canvas_item() so we don't need to do anything here.
            break;
    }
}
