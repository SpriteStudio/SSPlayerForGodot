#include "ss_player_node_2d.h"

class SpriteStudioPlayer2D::_SignalSink : public SsPlayerEventSink {
public:
    explicit _SignalSink(SpriteStudioPlayer2D* p_owner) : _owner(p_owner) {}

    void onAnimationStarted(const String& anim_name) override {
        _owner->emit_signal("animation_started", anim_name);
    }
    void onAnimationFinished(const String& anim_name) override {
        _owner->emit_signal("animation_finished", anim_name);
    }
    void onAnimationLooped(const String& anim_name) override {
        _owner->emit_signal("animation_looped", anim_name);
    }
    void onUserData(int flag, int int_value, const Rect2& rect_value, const Vector2& point_value, const String& string_value) override {
        _owner->emit_signal("user_data", flag, int_value, rect_value, point_value, string_value);
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
    if (now.is_valid() && !now->is_connected("changed", changed_cb)) {
        now->connect("changed", changed_cb);
    }
}

void SpriteStudioPlayer2D::_on_ssab_changed() {
    _internal->onSSABReloaded();
    NOTIFY_PROPERTY_LIST_CHANGED();
}

Ref<SSABResource> SpriteStudioPlayer2D::getSSABResource() const {
    return _internal->getSSABResource();
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
void SpriteStudioPlayer2D::setFrameRelative(float p_diff) { _internal->setFrameRelative(p_diff); }
float SpriteStudioPlayer2D::getFrame() const { return _internal->getFrame(); }

int SpriteStudioPlayer2D::getTotalFrames() const { return _internal->getTotalFrames(); }

void SpriteStudioPlayer2D::setFrameRate(int p_fps) { _internal->setFrameRate(p_fps); }
int SpriteStudioPlayer2D::getFrameRate() const { return _internal->getFrameRate(); }

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
    ClassDB::bind_method( D_METHOD( "set_frame_relative", "diff" ), &SpriteStudioPlayer2D::setFrameRelative );
    ClassDB::bind_method( D_METHOD( "get_frame" ), &SpriteStudioPlayer2D::getFrame );

    ClassDB::bind_method( D_METHOD( "get_total_frames" ), &SpriteStudioPlayer2D::getTotalFrames );

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

    ADD_SIGNAL(
        MethodInfo(
            "user_data",
            PropertyInfo(Variant::INT, "flag"),
            PropertyInfo(Variant::INT, "int_value"),
            PropertyInfo(Variant::RECT2, "rect_value"),
            PropertyInfo(Variant::VECTOR2, "point_value"),
            PropertyInfo(Variant::STRING, "string_value")
        )
    );
    ADD_SIGNAL(
        MethodInfo(
            "signal",
            PropertyInfo(Variant::STRING, "command"),
            PropertyInfo(Variant::DICTIONARY, "value")
        )
    );

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
    if (p_name == StringName("animation")) {
        setAnimation(p_property);
        return true;
    } else if (p_name == StringName("frame")) {
        setFrame(p_property);
        return true;
    } else if (p_name == StringName("loop")) {
        setLoop(p_property);
        return true;
    } else if (p_name == StringName("speed")) {
        setSpeed(p_property);
        return true;
    } else if (p_name == StringName("skip_frames")) {
        setSkipFrames(p_property);
        return true;
    } else if (p_name == StringName("sub_frame_enabled")) {
        setSubFrameEnabled(p_property);
        return true;
    } else if (p_name == StringName("playing")) {
        if (p_property) {
            play();
        } else {
            stop();
        }
        return true;
    }
    return false;
}

bool SpriteStudioPlayer2D::_get(const StringName& p_name, Variant& r_property) const {
    if (p_name == StringName("animation")) {
        r_property = getAnimation();
        return true;
    } else if (p_name == StringName("frame")) {
        r_property = getFrame();
        return true;
    } else if (p_name == StringName("loop")) {
        r_property = getLoop();
        return true;
    } else if (p_name == StringName("speed")) {
        r_property = getSpeed();
        return true;
    } else if (p_name == StringName("skip_frames")) {
        r_property = isSkipFrames();
        return true;
    } else if (p_name == StringName("sub_frame_enabled")) {
        r_property = isSubFrameEnabled();
        return true;
    } else if (p_name == StringName("playing")) {
        r_property = isPlaying();
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
}

void SpriteStudioPlayer2D::_notification(int p_notification) {
    switch (p_notification) {
        case NOTIFICATION_ENTER_TREE:
            // Re-parent the InternalPlayer's root canvas item to ours so the
            // Node2D's transform / visibility / Z-order propagate. Also done
            // here so editor reloads (which destroy / reattach the canvas)
            // don't leave the InternalPlayer floating.
            _internal->setParentCanvasItem(get_canvas_item());
            break;

        case NOTIFICATION_EXIT_TREE:
            // Detach before our canvas item is freed by Node2D — otherwise
            // _root_ci would briefly point at a dangling parent RID until
            // the next ENTER_TREE / dtor.
            _internal->setParentCanvasItem(RID());
            break;

        case NOTIFICATION_READY:
            set_process_internal(true);
            break;

        case NOTIFICATION_INTERNAL_PROCESS:
            _internal->update((float)get_process_delta_time());
            break;

        default:
            break;
    }
}
