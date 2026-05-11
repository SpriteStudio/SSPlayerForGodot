#include "ss_player_node_2d.h"

class SpriteStudioPlayer2D::_SignalSink : public SsPlayerEventSink {
public:
    explicit _SignalSink(SpriteStudioPlayer2D* p_owner) : _owner(p_owner) {}

    void onAnimationStarted(const String& anim_name) override {
        _owner->emit_signal("animation_started", anim_name);
    }
    void onAnimationChanged(const String& anim_name) override {
        _owner->emit_signal("animation_changed", anim_name);
    }
    void onAnimationFinished(const String& anim_name) override {
        _owner->emit_signal("animation_finished", anim_name);
    }
    void onAnimationLooped(const String& anim_name) override {
        _owner->emit_signal("animation_looped", anim_name);
    }
    void onUserData(const Dictionary& payload) override {
        _owner->emit_signal("user_data", payload);
    }
    void onSignal(const String& command, const Dictionary& value) override {
        _owner->emit_signal("signal", command, value);
    }

private:
    SpriteStudioPlayer2D* _owner;
};


SpriteStudioPlayer2D::SpriteStudioPlayer2D() {
    _internal = memnew(SsInternalPlayer);
    _sink = memnew(_SignalSink(this));
    _internal->setEventSink(_sink);
}

SpriteStudioPlayer2D::~SpriteStudioPlayer2D() {
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
}

String SpriteStudioPlayer2D::getAnimation() const {
    return _internal->getAnimation();
}

bool SpriteStudioPlayer2D::isPlaying() const { return _internal->isPlaying(); }
void SpriteStudioPlayer2D::play(float p_start_frame) { _internal->play(p_start_frame); }
bool SpriteStudioPlayer2D::isPausing() const { return _internal->isPausing(); }
void SpriteStudioPlayer2D::pause() { _internal->pause(); }
void SpriteStudioPlayer2D::stop() { _internal->stop(); }

void SpriteStudioPlayer2D::setSpeed(float p_speed) { _internal->setSpeed(p_speed); }
float SpriteStudioPlayer2D::getSpeed() const { return _internal->getSpeed(); }

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

void SpriteStudioPlayer2D::setLoop(int p_count) { _internal->setLoop(p_count); }
int SpriteStudioPlayer2D::getLoop() const { return _internal->getLoop(); }

void SpriteStudioPlayer2D::setSkipFrames(bool p_skip) { _internal->setSkipFrames(p_skip); }
bool SpriteStudioPlayer2D::isSkipFrames() const { return _internal->isSkipFrames(); }

void SpriteStudioPlayer2D::setSubFrameEnabled(bool p_enabled) { _internal->setSubFrameEnabled(p_enabled); }
bool SpriteStudioPlayer2D::isSubFrameEnabled() const { return _internal->isSubFrameEnabled(); }


void SpriteStudioPlayer2D::_bind_methods() {
    ClassDB::bind_method( D_METHOD( "set_ssab_resource", "res_ssab" ), &SpriteStudioPlayer2D::setSSABResource );
    ClassDB::bind_method( D_METHOD( "get_ssab_resource" ), &SpriteStudioPlayer2D::getSSABResource );
    ClassDB::bind_method( D_METHOD( "set_animation", "name" ), &SpriteStudioPlayer2D::setAnimation );
    ClassDB::bind_method( D_METHOD( "get_animation" ), &SpriteStudioPlayer2D::getAnimation );

    ClassDB::bind_method( D_METHOD( "is_playing" ), &SpriteStudioPlayer2D::isPlaying );
    ClassDB::bind_method( D_METHOD( "play", "start_frame" ), &SpriteStudioPlayer2D::play, DEFVAL(-1.0f) );
    ClassDB::bind_method( D_METHOD( "is_pausing" ), &SpriteStudioPlayer2D::isPausing );
    ClassDB::bind_method( D_METHOD( "pause" ), &SpriteStudioPlayer2D::pause );
    ClassDB::bind_method( D_METHOD( "stop" ), &SpriteStudioPlayer2D::stop );

    ClassDB::bind_method( D_METHOD( "set_speed", "speed" ), &SpriteStudioPlayer2D::setSpeed );
    ClassDB::bind_method( D_METHOD( "get_speed" ), &SpriteStudioPlayer2D::getSpeed );
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

    ClassDB::bind_method( D_METHOD( "set_loop", "count" ), &SpriteStudioPlayer2D::setLoop );
    ClassDB::bind_method( D_METHOD( "get_loop" ), &SpriteStudioPlayer2D::getLoop );

    ClassDB::bind_method( D_METHOD( "set_skip_frames", "skip" ), &SpriteStudioPlayer2D::setSkipFrames );
    ClassDB::bind_method( D_METHOD( "is_skip_frames" ), &SpriteStudioPlayer2D::isSkipFrames );

    ClassDB::bind_method( D_METHOD( "set_sub_frame_enabled", "enabled" ), &SpriteStudioPlayer2D::setSubFrameEnabled );
    ClassDB::bind_method( D_METHOD( "is_sub_frame_enabled" ), &SpriteStudioPlayer2D::isSubFrameEnabled );

    ClassDB::bind_method( D_METHOD( "set_cellmap_texture", "cellmap_name", "texture" ), &SpriteStudioPlayer2D::set_cellmap_texture );
    ClassDB::bind_method( D_METHOD( "get_cellmap_texture", "cellmap_name" ), &SpriteStudioPlayer2D::get_cellmap_texture );

    ADD_SIGNAL(
        MethodInfo(
            "user_data",
            PropertyInfo(Variant::DICTIONARY, "payload")
        )
    );
    ADD_SIGNAL(
        MethodInfo(
            "signal",
            PropertyInfo(Variant::STRING, "command"),
            PropertyInfo(Variant::DICTIONARY, "value")
        )
    );

    ADD_SIGNAL(MethodInfo("animation_changed", PropertyInfo(Variant::STRING, "anim_name")));
    ADD_SIGNAL(MethodInfo("animation_started", PropertyInfo(Variant::STRING, "anim_name")));
    ADD_SIGNAL(MethodInfo("animation_finished", PropertyInfo(Variant::STRING, "anim_name")));
    ADD_SIGNAL(MethodInfo("animation_looped", PropertyInfo(Variant::STRING, "anim_name")));

    ADD_PROPERTY(
        PropertyInfo(
            Variant::OBJECT,
            "ssab",
            PropertyHint::PROPERTY_HINT_RESOURCE_TYPE,
            "SSABResource"
        ),
        "set_ssab_resource",
        "get_ssab_resource"
    );

    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed"), "set_speed", "get_speed");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "loop"), "set_loop", "get_loop");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "skip_frames"), "set_skip_frames", "is_skip_frames");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "sub_frame_enabled"), "set_sub_frame_enabled", "is_sub_frame_enabled");

    ADD_GROUP("Animation Settings", "");
}

bool SpriteStudioPlayer2D::_set(const StringName& p_name, const Variant& p_property) {
    String name = p_name;
    if (name == "animation") {
        setAnimation(p_property);
        return true;
    } else if (name == "frame") {
        setFrame(p_property);
        return true;
    } else if (name == "loop") {
        setLoop(p_property);
        return true;
    } else if (name == "speed") {
        setSpeed(p_property);
        return true;
    } else if (name == "skip_frames") {
        setSkipFrames(p_property);
        return true;
    } else if (name == "sub_frame_enabled") {
        setSubFrameEnabled(p_property);
        return true;
    } else if (name == "playing") {
        if (p_property) {
            play();
        } else {
            stop();
        }
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
    } else if (name == "frame") {
        r_property = getFrame();
        return true;
    } else if (name == "loop") {
        r_property = getLoop();
        return true;
    } else if (name == "speed") {
        r_property = getSpeed();
        return true;
    } else if (name == "skip_frames") {
        r_property = isSkipFrames();
        return true;
    } else if (name == "sub_frame_enabled") {
        r_property = isSubFrameEnabled();
        return true;
    } else if (name == "playing") {
        r_property = isPlaying();
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
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    PackedStringArray vecAnimeName;
#else
    Vector<String> vecAnimeName;
#endif

    vecAnimeName.insert(0, "-- Empty --");

    Ref<SSABResource> res = _internal->getSSABResource();
    if (!res.is_null()) {
        vecAnimeName = res->get_animation_names();
    }

    PropertyInfo animasPropertyInfo;
    animasPropertyInfo.name = "animation";
    animasPropertyInfo.type = Variant::STRING;
    animasPropertyInfo.usage = PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE;
    animasPropertyInfo.hint_string = String(",").join(vecAnimeName);
    animasPropertyInfo.hint = PROPERTY_HINT_ENUM;
    p_list->push_back(animasPropertyInfo);

    animasPropertyInfo.name = "playing";
    animasPropertyInfo.type = Variant::BOOL;
    animasPropertyInfo.usage = PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE;
    animasPropertyInfo.hint = PROPERTY_HINT_NONE;
    p_list->push_back(animasPropertyInfo);

    animasPropertyInfo.name = "frame";
    animasPropertyInfo.type = Variant::FLOAT;
    animasPropertyInfo.usage = PROPERTY_USAGE_EDITOR;
    animasPropertyInfo.hint = PROPERTY_HINT_RANGE;
    animasPropertyInfo.hint_string = "0," + String::num(getTotalFrames() - 1) + ",0.01";
    p_list->push_back(animasPropertyInfo);

    if (!res.is_null()) {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
        PackedStringArray cellmapNames = res->get_cellmap_names();
#else
        Vector<String> cellmapNames = res->get_cellmap_names();
#endif
        if (cellmapNames.size() > 0) {
            p_list->push_back(PropertyInfo(Variant::NIL, "CellMap Overrides", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_GROUP));
            for (int i = 0; i < cellmapNames.size(); i++) {
                p_list->push_back(PropertyInfo(Variant::OBJECT, "cellmaps/" + cellmapNames[i], PROPERTY_HINT_RESOURCE_TYPE, "Texture2D", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_STORAGE));
            }
        }
    }
}

void SpriteStudioPlayer2D::_notification(int p_notification) {
    switch (p_notification) {
        case NOTIFICATION_ENTER_TREE:
            // Re-parent the InternalPlayer's root canvas item to ours so the
            // Node2D's transform / visibility / Z-order propagate. Also done
            // here so editor reloads (which destroy / reattach the canvas)
            // don't leave the InternalPlayer floating.
            _internal->setParentCanvasItem(get_canvas_item());
            set_process_internal(true);
            break;
        case NOTIFICATION_EXIT_TREE:
            _internal->setParentCanvasItem(RID());
            break;
        case NOTIFICATION_INTERNAL_PROCESS:
            _internal->update(get_process_delta_time());
            break;
        case NOTIFICATION_DRAW:
            // The InternalPlayer handles the actual RenderingServer calls for
            // its per-batch canvas items, but they are nested under our
            // get_canvas_item() so we don't need to do anything here.
            break;
    }
}
