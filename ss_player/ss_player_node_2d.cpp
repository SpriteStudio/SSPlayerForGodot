#include "ss_player_node_2d.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/viewport.hpp>
#else
#include "core/config/engine.h"
#include "scene/main/viewport.h"
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
    void onSignal(const String& command, const Dictionary& value, const Dictionary& info) override {
        _owner->emit_signal(SNAME("signal_emitted"), command, value, info);
    }
    void onAudio(const Dictionary& payload) override {
        // Observation channel: always fires (any direction, even in the editor).
        _owner->emit_signal(SNAME("audio"), payload);
        // Built-in / backend playback (forward-only, runtime-only).
        _owner->_handle_audio(payload);
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
}

SpriteStudioPlayer2D::~SpriteStudioPlayer2D() {
    if (_audio_controller) {
        _audio_controller->stop_all();
        memdelete(_audio_controller);
        _audio_controller = nullptr;
    }
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
    _apply_transport_settings();

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

// Structural names live in the .ssab, not the runtime, so these forward to the
// bound SSABResource — the same source the inspector's cellmap list reads. They
// are the discovery half of set_part_cell_override(); having them on the node
// saves the caller a round-trip through get_ssab_resource(). Empty when no
// resource is bound (or the cellmap is unknown).

PackedStringArray SpriteStudioPlayer2D::get_cellmap_names() const {
    PackedStringArray names;
    Ref<SSABResource> res = _internal->getSSABResource();
    if (res.is_null()) return names;
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    names = res->get_cellmap_names();
#else
    Vector<String> src = res->get_cellmap_names();
    for (int i = 0; i < src.size(); i++) {
        names.push_back(src[i]);
    }
#endif
    return names;
}

PackedStringArray SpriteStudioPlayer2D::get_cell_names(const String& cellmap_name) const {
    PackedStringArray names;
    Ref<SSABResource> res = _internal->getSSABResource();
    if (res.is_null()) return names;
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    names = res->get_cell_names(cellmap_name);
#else
    Vector<String> src = res->get_cell_names(cellmap_name);
    for (int i = 0; i < src.size(); i++) {
        names.push_back(src[i]);
    }
#endif
    return names;
}

int SpriteStudioPlayer2D::find_part_index(const String& part_name) const {
    return _internal->resolve_part_index(part_name);
}

Transform2D SpriteStudioPlayer2D::get_part_transform(const String& part_name) const {
    Transform2D xf;
    int idx = _internal->resolve_part_index(part_name);
    if (idx >= 0 && _internal->try_get_part_local_transform(idx, xf)) {
        // The runtime's world matrix is relative to the internal root canvas
        // item, which carries flip / offset. Compose it here so the result is
        // the part's transform in THIS node's local space — otherwise an
        // attachment pinned to a part detaches the moment the player is flipped.
        xf = _make_root_transform() * xf;
    }
    return xf;
}

bool SpriteStudioPlayer2D::is_part_hidden(const String& part_name) const {
    int idx = _internal->resolve_part_index(part_name);
    if (idx < 0) return false;
    bool hidden = false;
    _internal->try_get_part_hidden(idx, hidden);
    return hidden;
}

PackedStringArray SpriteStudioPlayer2D::get_part_names() const {
    PackedStringArray names;
    int count = _internal->get_part_count();
    for (int i = 0; i < count; i++) {
        names.push_back(_internal->get_part_name(i));
    }
    return names;
}

// ---- Override Layer (Phase 2) --------------------------------------------

// By-index variants. The runtime addresses parts by index, so these skip the
// name lookup — resolve a name once via find_part_index() and reuse the index.
// An out-of-range index is a no-op returning false (guarded in SsInternalPlayer).

bool SpriteStudioPlayer2D::set_part_visibility_override_by_index(int part_index, bool force_hidden, bool cascade) {
    return _internal->set_part_visibility_override(part_index, force_hidden, cascade);
}

bool SpriteStudioPlayer2D::clear_part_visibility_override_by_index(int part_index) {
    return _internal->clear_part_visibility_override(part_index);
}

bool SpriteStudioPlayer2D::set_part_color_override_by_index(int part_index, const Color& color, ColorBlendOperation blend_op, OverridePriority priority) {
    return _internal->set_part_color_override(part_index, color, (int)blend_op, (int)priority);
}

bool SpriteStudioPlayer2D::set_part_color_override_corners_by_index(int part_index, const PackedColorArray& corners,
                                                                    ColorBlendOperation blend_op, OverridePriority priority) {
    // One argument rather than four, so this reads the same shape as the
    // single-colour form (SDK: 20_design/40_api_conventions). Four is the only
    // valid length: a shorter array would silently leave corners at whatever the
    // default Color is, which looks like a gradient bug rather than a bad call.
    ERR_FAIL_COND_V_MSG(corners.size() != 4, false,
                        "corners must hold exactly 4 colours, in left-top, right-top, left-bottom, right-bottom order");
    return _internal->set_part_color_override_corners(part_index, corners[0], corners[1], corners[2], corners[3], (int)blend_op, (int)priority);
}

bool SpriteStudioPlayer2D::clear_part_color_override_by_index(int part_index) {
    return _internal->clear_part_color_override(part_index);
}

bool SpriteStudioPlayer2D::set_part_cell_override_by_index(int part_index, const String& cellmap_name, const String& cell_name, OverridePriority priority) {
    return _internal->set_part_cell_override(part_index, cellmap_name, cell_name, (int)priority);
}

bool SpriteStudioPlayer2D::clear_part_cell_override_by_index(int part_index) {
    return _internal->clear_part_cell_override(part_index);
}

// Name-based convenience wrappers — resolve the name and delegate to the
// by-index variant (an unknown name resolves to -1 → no-op returning false).

bool SpriteStudioPlayer2D::set_part_visibility_override(const String& part_name, bool force_hidden, bool cascade) {
    return set_part_visibility_override_by_index(_internal->resolve_part_index(part_name), force_hidden, cascade);
}

bool SpriteStudioPlayer2D::clear_part_visibility_override(const String& part_name) {
    return clear_part_visibility_override_by_index(_internal->resolve_part_index(part_name));
}

bool SpriteStudioPlayer2D::set_part_color_override(const String& part_name, const Color& color, ColorBlendOperation blend_op, OverridePriority priority) {
    return set_part_color_override_by_index(_internal->resolve_part_index(part_name), color, blend_op, priority);
}

bool SpriteStudioPlayer2D::set_part_color_override_corners(const String& part_name, const PackedColorArray& corners,
                                                           ColorBlendOperation blend_op, OverridePriority priority) {
    return set_part_color_override_corners_by_index(_internal->resolve_part_index(part_name), corners, blend_op, priority);
}

bool SpriteStudioPlayer2D::clear_part_color_override(const String& part_name) {
    return clear_part_color_override_by_index(_internal->resolve_part_index(part_name));
}

bool SpriteStudioPlayer2D::set_part_cell_override(const String& part_name, const String& cellmap_name, const String& cell_name, OverridePriority priority) {
    return set_part_cell_override_by_index(_internal->resolve_part_index(part_name), cellmap_name, cell_name, priority);
}

bool SpriteStudioPlayer2D::clear_part_cell_override(const String& part_name) {
    return clear_part_cell_override_by_index(_internal->resolve_part_index(part_name));
}

bool SpriteStudioPlayer2D::clear_all_part_overrides() {
    return _internal->clear_all_part_overrides();
}

void SpriteStudioPlayer2D::setAnimation(const String& strName) {
    _internal->setAnimation(strName);
    _apply_transport_settings();
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
bool SpriteStudioPlayer2D::isPlayingForward() const { return _internal->isPlayingForward(); }
bool SpriteStudioPlayer2D::justLooped() const { return _internal->justLooped(); }
bool SpriteStudioPlayer2D::isFinished() const { return _internal->isFinished(); }

PackedStringArray SpriteStudioPlayer2D::get_animation_names() const {
    Ref<SSABResource> res = _internal->getSSABResource();
    return res.is_valid() ? res->get_animation_names() : PackedStringArray();
}
// Per the Player porting doc, built-in audio is fired-and-forgotten: it is not
// coupled to animation pause or stop. Sounds already playing run to completion.
void SpriteStudioPlayer2D::pause() { _internal->pause(); }
void SpriteStudioPlayer2D::resume() { _internal->resume(); }
void SpriteStudioPlayer2D::stop() { _internal->stop(); }

void SpriteStudioPlayer2D::_handle_audio(const Dictionary& payload) {
    if (!_play_audio) return;
    // Plays during real forward playback, including the editor preview (the
    // runtime also fires audio events on reverse playback, which is a limitation).
    if (!_internal->isPlayingForward()) return;

    if (_audio_controller == nullptr) {
        _audio_controller = memnew(SsAudioController(this));
    }
    _audio_controller->play(payload, getSSABResource(), _audio_backend.ptr(), _audio_volume);
}

void SpriteStudioPlayer2D::set_play_audio(bool p_enabled) {
    _play_audio = p_enabled;
    // Turning built-in playback off stops any in-flight built-in voices.
    if (!_play_audio && _audio_controller) _audio_controller->stop_all();
}
bool SpriteStudioPlayer2D::is_play_audio() const { return _play_audio; }

void SpriteStudioPlayer2D::set_audio_volume(float p_volume) { _audio_volume = p_volume; }
float SpriteStudioPlayer2D::get_audio_volume() const { return _audio_volume; }

void SpriteStudioPlayer2D::set_audio_backend(const Ref<SpriteStudioAudioBackend>& p_backend) {
    _audio_backend = p_backend;
}
Ref<SpriteStudioAudioBackend> SpriteStudioPlayer2D::get_audio_backend() const { return _audio_backend; }

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

Transform2D SpriteStudioPlayer2D::_make_root_transform() const {
    Transform2D xform;
    xform.columns[0].x = _flip_h ? -1.0 : 1.0;
    xform.columns[1].y = _flip_v ? -1.0 : 1.0;
    xform.columns[2] = _offset;
    return xform;
}

void SpriteStudioPlayer2D::_update_root_transform() {
    _internal->setRootTransform(_make_root_transform());
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

void SpriteStudioPlayer2D::_push_host_viewport() {
    // The mask coverage target is parented to this viewport so the server draws
    // it before us. Tree changes are the only thing that can move a node to
    // another viewport, so pushing it there keeps the link current.
    Viewport* vp = get_viewport();
    _internal->setHostViewport(vp ? vp->get_viewport_rid() : RID());
}

void SpriteStudioPlayer2D::set_animation_process_mode(AnimationProcessMode p_mode) {
    AnimationProcessMode mode = p_mode;
    if (_process_mode == mode) return;

    bool active = is_inside_tree();
    if (active) {
        if (_process_mode == ANIMATION_PROCESS_PHYSICS) {
            set_physics_process_internal(false);
        } else {
            set_process_internal(false);
        }
    }
    
    _process_mode = mode;
    
    // MANUAL keeps the idle notification: the animation is not advanced there,
    // but the audio controller still needs a per-frame tick, and the node has
    // to keep reporting its on-screen scale for the mask coverage pass.
    if (active) {
        if (_process_mode == ANIMATION_PROCESS_PHYSICS) {
            set_physics_process_internal(true);
        } else {
            set_process_internal(true);
        }
    }
}

SpriteStudioPlayer2D::AnimationProcessMode SpriteStudioPlayer2D::get_animation_process_mode() const { return _process_mode; }

void SpriteStudioPlayer2D::advance(double p_delta) {
    _push_coverage_screen_scale();
    _internal->update(p_delta);
    // Same post-update contract as an automatic tick: world matrices are final,
    // so part attachments mirror their parts before anything draws.
    emit_signal(SNAME("frame_updated"), _internal->getFrameNo());
}

void SpriteStudioPlayer2D::setSpeedScale(float p_speed) { _internal->setSpeed(p_speed); }
float SpriteStudioPlayer2D::getSpeedScale() const { return _internal->getSpeed(); }

void SpriteStudioPlayer2D::setFrameNo(float p_frame) { _internal->setFrameNo(p_frame); }
float SpriteStudioPlayer2D::getFrameNo() const { return _internal->getFrameNo(); }

int SpriteStudioPlayer2D::getTotalFrames() const { return _internal->getTotalFrames(); }

void SpriteStudioPlayer2D::setFrameRate(int p_fps) { _internal->setFrameRate(p_fps); }
int SpriteStudioPlayer2D::getFrameRate() const { return _internal->getFrameRate(); }

int SpriteStudioPlayer2D::getStartFrame() const { return _internal->getAnimationSectionStart(); }
int SpriteStudioPlayer2D::getEndFrame() const { return _internal->getAnimationSectionEnd(); }

void SpriteStudioPlayer2D::setAnimationSection(int p_start, int p_end) { _internal->setAnimationSection(p_start, p_end); }
void SpriteStudioPlayer2D::setAnimationSectionStart(int p_start) { setAnimationSection(p_start, getAnimationSectionEnd()); }
void SpriteStudioPlayer2D::setAnimationSectionEnd(int p_end) { setAnimationSection(getAnimationSectionStart(), p_end); }
int SpriteStudioPlayer2D::getAnimationSectionStart() const { return _internal->getAnimationSectionStart(); }
int SpriteStudioPlayer2D::getAnimationSectionEnd() const { return _internal->getAnimationSectionEnd(); }

void SpriteStudioPlayer2D::setPlaybackDirection(PlaybackDirection p_direction, PlaybackStyle p_style) { _internal->setPlaybackDirection((int)p_direction, (int)p_style); }
SpriteStudioPlayer2D::PlaybackDirection SpriteStudioPlayer2D::getPlaybackDirection() const { return (PlaybackDirection)_internal->getPlaybackDirection(); }
SpriteStudioPlayer2D::PlaybackStyle SpriteStudioPlayer2D::getPlaybackStyle() const { return (PlaybackStyle)_internal->getPlaybackStyle(); }

void SpriteStudioPlayer2D::setLoopCount(int p_count) {
    _loop_count = p_count;
    _internal->setLoop(p_count);
}
// Reads the runtime rather than the field: the two agree, because every path
// that re-runs `setup_animation` pushes the field back afterwards.
int SpriteStudioPlayer2D::getLoopCount() const { return _internal->getLoop(); }

void SpriteStudioPlayer2D::_apply_transport_settings() {
    _internal->setLoop(_loop_count);
}

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
    ClassDB::bind_method( D_METHOD( "is_playing_forward" ), &SpriteStudioPlayer2D::isPlayingForward );
    ClassDB::bind_method( D_METHOD( "just_looped" ), &SpriteStudioPlayer2D::justLooped );
    ClassDB::bind_method( D_METHOD( "is_finished" ), &SpriteStudioPlayer2D::isFinished );
    ClassDB::bind_method( D_METHOD( "get_animation_names" ), &SpriteStudioPlayer2D::get_animation_names );
    ClassDB::bind_method( D_METHOD( "pause" ), &SpriteStudioPlayer2D::pause );
    ClassDB::bind_method( D_METHOD( "resume" ), &SpriteStudioPlayer2D::resume );
    ClassDB::bind_method( D_METHOD( "stop" ), &SpriteStudioPlayer2D::stop );

    ClassDB::bind_method( D_METHOD( "set_speed_scale", "speed_scale" ), &SpriteStudioPlayer2D::setSpeedScale );
    ClassDB::bind_method( D_METHOD( "get_speed_scale" ), &SpriteStudioPlayer2D::getSpeedScale );
    ClassDB::bind_method( D_METHOD( "set_frame_no", "frame_no" ), &SpriteStudioPlayer2D::setFrameNo );
    ClassDB::bind_method( D_METHOD( "get_frame_no" ), &SpriteStudioPlayer2D::getFrameNo );

    ClassDB::bind_method( D_METHOD( "get_total_frames" ), &SpriteStudioPlayer2D::getTotalFrames );
    ClassDB::bind_method( D_METHOD( "get_start_frame" ), &SpriteStudioPlayer2D::getStartFrame );
    ClassDB::bind_method( D_METHOD( "get_end_frame" ), &SpriteStudioPlayer2D::getEndFrame );

    ClassDB::bind_method( D_METHOD( "set_frame_rate", "fps" ), &SpriteStudioPlayer2D::setFrameRate );
    ClassDB::bind_method( D_METHOD( "get_frame_rate" ), &SpriteStudioPlayer2D::getFrameRate );

    ClassDB::bind_method( D_METHOD( "set_animation_section", "start", "end" ), &SpriteStudioPlayer2D::setAnimationSection );
    ClassDB::bind_method( D_METHOD( "set_animation_section_start", "start" ), &SpriteStudioPlayer2D::setAnimationSectionStart );
    ClassDB::bind_method( D_METHOD( "set_animation_section_end", "end" ), &SpriteStudioPlayer2D::setAnimationSectionEnd );
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

    ClassDB::bind_method( D_METHOD( "get_cellmap_names" ), &SpriteStudioPlayer2D::get_cellmap_names );
    ClassDB::bind_method( D_METHOD( "get_cell_names", "cellmap_name" ), &SpriteStudioPlayer2D::get_cell_names );

    ClassDB::bind_method( D_METHOD( "set_play_audio", "enabled" ), &SpriteStudioPlayer2D::set_play_audio );
    ClassDB::bind_method( D_METHOD( "is_play_audio" ), &SpriteStudioPlayer2D::is_play_audio );
    ClassDB::bind_method( D_METHOD( "set_audio_volume", "volume" ), &SpriteStudioPlayer2D::set_audio_volume );
    ClassDB::bind_method( D_METHOD( "get_audio_volume" ), &SpriteStudioPlayer2D::get_audio_volume );
    ClassDB::bind_method( D_METHOD( "set_audio_backend", "backend" ), &SpriteStudioPlayer2D::set_audio_backend );
    ClassDB::bind_method( D_METHOD( "get_audio_backend" ), &SpriteStudioPlayer2D::get_audio_backend );

    ClassDB::bind_method( D_METHOD( "set_offset", "offset" ), &SpriteStudioPlayer2D::set_offset );
    ClassDB::bind_method( D_METHOD( "get_offset" ), &SpriteStudioPlayer2D::get_offset );

    ClassDB::bind_method( D_METHOD( "set_animation_process_mode", "mode" ), &SpriteStudioPlayer2D::set_animation_process_mode );
    ClassDB::bind_method( D_METHOD( "get_animation_process_mode" ), &SpriteStudioPlayer2D::get_animation_process_mode );
    ClassDB::bind_method( D_METHOD( "advance", "delta" ), &SpriteStudioPlayer2D::advance );

    ClassDB::bind_method( D_METHOD( "set_flip_h", "flip_h" ), &SpriteStudioPlayer2D::set_flip_h );
    ClassDB::bind_method( D_METHOD( "is_flipped_h" ), &SpriteStudioPlayer2D::is_flipped_h );
    ClassDB::bind_method( D_METHOD( "set_flip_v", "flip_v" ), &SpriteStudioPlayer2D::set_flip_v );
    ClassDB::bind_method( D_METHOD( "is_flipped_v" ), &SpriteStudioPlayer2D::is_flipped_v );

    ClassDB::bind_method( D_METHOD( "find_part_index", "part_name" ), &SpriteStudioPlayer2D::find_part_index );
    ClassDB::bind_method( D_METHOD( "get_part_transform", "part_name" ), &SpriteStudioPlayer2D::get_part_transform );
    ClassDB::bind_method( D_METHOD( "is_part_hidden", "part_name" ), &SpriteStudioPlayer2D::is_part_hidden );
    ClassDB::bind_method( D_METHOD( "get_part_names" ), &SpriteStudioPlayer2D::get_part_names );

    // ---- Override Layer (Phase 2): per-part runtime overrides -------------
    ClassDB::bind_method( D_METHOD( "set_part_visibility_override", "part_name", "force_hidden", "cascade" ), &SpriteStudioPlayer2D::set_part_visibility_override, DEFVAL(false) );
    ClassDB::bind_method( D_METHOD( "clear_part_visibility_override", "part_name" ), &SpriteStudioPlayer2D::clear_part_visibility_override );
    ClassDB::bind_method( D_METHOD( "set_part_color_override", "part_name", "color", "blend_op", "priority" ), &SpriteStudioPlayer2D::set_part_color_override, DEFVAL(COLOR_BLEND_MIX), DEFVAL(OVERRIDE_PRIORITY_HOLD_UNTIL_NEXT_ANIMATION) );
    ClassDB::bind_method( D_METHOD( "set_part_color_override_corners", "part_name", "corners", "blend_op", "priority" ), &SpriteStudioPlayer2D::set_part_color_override_corners, DEFVAL(COLOR_BLEND_MIX), DEFVAL(OVERRIDE_PRIORITY_HOLD_UNTIL_NEXT_ANIMATION) );
    ClassDB::bind_method( D_METHOD( "clear_part_color_override", "part_name" ), &SpriteStudioPlayer2D::clear_part_color_override );
    ClassDB::bind_method( D_METHOD( "set_part_cell_override", "part_name", "cellmap_name", "cell_name", "priority" ), &SpriteStudioPlayer2D::set_part_cell_override, DEFVAL(OVERRIDE_PRIORITY_HOLD_UNTIL_NEXT_ANIMATION) );
    ClassDB::bind_method( D_METHOD( "clear_part_cell_override", "part_name" ), &SpriteStudioPlayer2D::clear_part_cell_override );
    ClassDB::bind_method( D_METHOD( "clear_all_part_overrides" ), &SpriteStudioPlayer2D::clear_all_part_overrides );

    // By-index variants
    ClassDB::bind_method( D_METHOD( "set_part_visibility_override_by_index", "part_index", "force_hidden", "cascade" ), &SpriteStudioPlayer2D::set_part_visibility_override_by_index, DEFVAL(false) );
    ClassDB::bind_method( D_METHOD( "clear_part_visibility_override_by_index", "part_index" ), &SpriteStudioPlayer2D::clear_part_visibility_override_by_index );
    ClassDB::bind_method( D_METHOD( "set_part_color_override_by_index", "part_index", "color", "blend_op", "priority" ), &SpriteStudioPlayer2D::set_part_color_override_by_index, DEFVAL(COLOR_BLEND_MIX), DEFVAL(OVERRIDE_PRIORITY_HOLD_UNTIL_NEXT_ANIMATION) );
    ClassDB::bind_method( D_METHOD( "set_part_color_override_corners_by_index", "part_index", "corners", "blend_op", "priority" ), &SpriteStudioPlayer2D::set_part_color_override_corners_by_index, DEFVAL(COLOR_BLEND_MIX), DEFVAL(OVERRIDE_PRIORITY_HOLD_UNTIL_NEXT_ANIMATION) );
    ClassDB::bind_method( D_METHOD( "clear_part_color_override_by_index", "part_index" ), &SpriteStudioPlayer2D::clear_part_color_override_by_index );
    ClassDB::bind_method( D_METHOD( "set_part_cell_override_by_index", "part_index", "cellmap_name", "cell_name", "priority" ), &SpriteStudioPlayer2D::set_part_cell_override_by_index, DEFVAL(OVERRIDE_PRIORITY_HOLD_UNTIL_NEXT_ANIMATION) );
    ClassDB::bind_method( D_METHOD( "clear_part_cell_override_by_index", "part_index" ), &SpriteStudioPlayer2D::clear_part_cell_override_by_index );

    ADD_SIGNAL(
        MethodInfo(
            "user_data",
            PropertyInfo(Variant::DICTIONARY, "payload")
        )
    );
    // The conventions' stem for this one is `signal`
    // (SDK: 20_design/40_api_conventions, §4), and this is the one place the
    // family cannot spell it: `signal` is a GDScript keyword, so a signal by
    // that name cannot be connected to or awaited without fighting the parser.
    // `_emitted` is the smallest suffix that keeps the stem readable.
    ADD_SIGNAL(
        MethodInfo(
            "signal_emitted",
            PropertyInfo(Variant::STRING, "command"),
            PropertyInfo(Variant::DICTIONARY, "value"),
            PropertyInfo(Variant::DICTIONARY, "info")
        )
    );
    ADD_SIGNAL(
        MethodInfo(
            "audio",
            PropertyInfo(Variant::DICTIONARY, "payload")
        )
    );

    // Not in the conventions' required set (SDK: 20_design/40_api_conventions,
    // §4): a host that called set_animation() already knows what it selected.
    // It earns its place here because `animation` is an exported property, so
    // the inspector, an AnimationPlayer track or a tool script can mount a
    // different clip without any of the node's own callers touching it.
    ADD_SIGNAL(MethodInfo("animation_changed", PropertyInfo(Variant::STRING, "anim_name")));
    ADD_SIGNAL(MethodInfo("animation_started", PropertyInfo(Variant::STRING, "anim_name")));
    ADD_SIGNAL(MethodInfo("animation_finished", PropertyInfo(Variant::STRING, "anim_name")));
    ADD_SIGNAL(MethodInfo("animation_looped", PropertyInfo(Variant::STRING, "anim_name")));

    // Emitted at the end of each processed tick, after the frame's world
    // matrices are final. SpriteStudioPartAttachment2D connects to this to
    // mirror a part's pose in the same frame it is rendered.
    ADD_SIGNAL(MethodInfo("frame_updated", PropertyInfo(Variant::FLOAT, "frame_no")));

    // Properties are registered statically so they land in ClassDB (visible to
    // class_get_property_list and to binding generators). The hints that depend
    // on the bound resource — the `animation` name list and the frame / section
    // ranges — are injected per-instance by _validate_property. The order here
    // is the inspector order; `cellmaps/*` still comes from _get_property_list
    // because its entries depend on the resource's cell maps.
    ADD_PROPERTY(
        PropertyInfo(Variant::OBJECT, "ssab", PROPERTY_HINT_RESOURCE_TYPE, "SSABResource"),
        "set_ssab_resource",
        "get_ssab_resource"
    );
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "animation", PROPERTY_HINT_ENUM, ""), "set_animation", "get_animation");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "autoplay"), "set_autoplay", "is_autoplay");
    // Editor-only (never stored): the playhead is runtime state, but it stays in
    // the property list so an AnimationPlayer can keyframe it.
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "frame_no", PROPERTY_HINT_RANGE, "0,0,0.01", PROPERTY_USAGE_EDITOR), "set_frame_no", "get_frame_no");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "loop_count", PROPERTY_HINT_RANGE, "-1,9999,1,or_greater"), "set_loop_count", "get_loop_count");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed_scale", PROPERTY_HINT_RANGE, "0,4,0.01,or_greater"), "set_speed_scale", "get_speed_scale");

    ADD_GROUP("Playback Options", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "frame_skip_enabled"), "set_frame_skip_enabled", "is_frame_skip_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "sub_frame_enabled"), "set_sub_frame_enabled", "is_sub_frame_enabled");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "animation_process_mode", PROPERTY_HINT_ENUM, "Physics,Idle,Manual"), "set_animation_process_mode", "get_animation_process_mode");

    ADD_GROUP("Section", "");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "animation_section_start", PROPERTY_HINT_RANGE, "0,0,1"), "set_animation_section_start", "get_animation_section_start");
    ADD_PROPERTY(PropertyInfo(Variant::INT, "animation_section_end", PROPERTY_HINT_RANGE, "0,0,1"), "set_animation_section_end", "get_animation_section_end");

    ADD_GROUP("Offset", "");
    ADD_PROPERTY(PropertyInfo(Variant::VECTOR2, "offset"), "set_offset", "get_offset");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flip_h"), "set_flip_h", "is_flipped_h");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flip_v"), "set_flip_v", "is_flipped_v");

    ADD_GROUP("Audio", "");
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "play_audio"), "set_play_audio", "is_play_audio");
    ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "audio_volume", PROPERTY_HINT_RANGE, "0,1,0.01"), "set_audio_volume", "get_audio_volume");
    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "audio_backend", PROPERTY_HINT_RESOURCE_TYPE, "SpriteStudioAudioBackend"), "set_audio_backend", "get_audio_backend");

    BIND_ENUM_CONSTANT(ANIMATION_PROCESS_PHYSICS);
    BIND_ENUM_CONSTANT(ANIMATION_PROCESS_IDLE);
    BIND_ENUM_CONSTANT(ANIMATION_PROCESS_MANUAL);

    BIND_ENUM_CONSTANT(PLAYBACK_DIRECTION_FORWARD);
    BIND_ENUM_CONSTANT(PLAYBACK_DIRECTION_BACKWARD);

    BIND_ENUM_CONSTANT(PLAYBACK_STYLE_NORMAL);
    BIND_ENUM_CONSTANT(PLAYBACK_STYLE_PING_PONG);

    BIND_ENUM_CONSTANT(COLOR_BLEND_MIX);
    BIND_ENUM_CONSTANT(COLOR_BLEND_MUL);
    BIND_ENUM_CONSTANT(COLOR_BLEND_ADD);
    BIND_ENUM_CONSTANT(COLOR_BLEND_SUB);

    BIND_ENUM_CONSTANT(OVERRIDE_PRIORITY_OVERWRITE_ON_NEXT_KEYFRAME);
    BIND_ENUM_CONSTANT(OVERRIDE_PRIORITY_HOLD_UNTIL_NEXT_ANIMATION);
    BIND_ENUM_CONSTANT(OVERRIDE_PRIORITY_PERMANENT);
}

// Runtime-only properties. They stay reachable by name (`player.frame_rate = 30`)
// but are deliberately absent from the property list, so they are neither shown
// in the inspector nor stored in the scene. Everything the inspector does show
// is registered statically in _bind_methods.
bool SpriteStudioPlayer2D::_set(const StringName& p_name, const Variant& p_property) {
    String name = p_name;
    if (name == "frame_rate") {
        setFrameRate(p_property);
        return true;
    } else if (name == "playback_direction") {
        setPlaybackDirection((PlaybackDirection)(int)p_property, getPlaybackStyle());
        return true;
    } else if (name == "playback_style") {
        setPlaybackDirection(getPlaybackDirection(), (PlaybackStyle)(int)p_property);
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
    if (name == "frame_rate") {
        r_property = getFrameRate();
        return true;
    } else if (name == "playback_direction") {
        r_property = getPlaybackDirection();
        return true;
    } else if (name == "playback_style") {
        r_property = getPlaybackStyle();
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
    // One entry per cell map in the bound resource. Unlike the rest of the
    // inspector these cannot be registered statically, because both their count
    // and their names come from the resource.
    Ref<SSABResource> res = _internal->getSSABResource();
    if (res.is_null()) {
        return;
    }

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    PackedStringArray cellmap_names = res->get_cellmap_names();
#else
    Vector<String> cellmap_names = res->get_cellmap_names();
#endif
    if (cellmap_names.size() == 0) {
        return;
    }

    p_list->push_back(PropertyInfo(Variant::NIL, "CellMap Overrides", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_GROUP));
    for (int i = 0; i < cellmap_names.size(); i++) {
        p_list->push_back(PropertyInfo(Variant::OBJECT, "cellmaps/" + cellmap_names[i], PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"));
    }
}

void SpriteStudioPlayer2D::_validate_property(PropertyInfo& p_property) const {
    if (p_property.name == StringName("animation")) {
        // Turn the statically registered enum hint into the animation names of
        // the bound resource. Left empty when no resource is assigned.
        Ref<SSABResource> res = _internal->getSSABResource();
        if (res.is_valid()) {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
            PackedStringArray anim_names = res->get_animation_names();
#else
            Vector<String> anim_names = res->get_animation_names();
#endif
            p_property.hint_string = String(",").join(anim_names);
        }
        return;
    }

    // The playhead and the section endpoints are bounded by the current
    // animation's length, which only the instance knows.
    bool is_frame = p_property.name == StringName("frame_no");
    bool is_section = p_property.name == StringName("animation_section_start") ||
                      p_property.name == StringName("animation_section_end");
    if (is_frame || is_section) {
        int total = getTotalFrames();
        int max_frame = total > 0 ? total - 1 : 0;
        p_property.hint_string = "0," + String::num(max_frame) + (is_frame ? ",0.01" : ",1");
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
            _push_host_viewport();
            if (_process_mode == ANIMATION_PROCESS_PHYSICS) {
                set_physics_process_internal(true);
            } else {
                set_process_internal(true);
            }
            break;
        case NOTIFICATION_EXIT_TREE:
            _internal->setParentCanvasItem(RID());
            _internal->setHostViewport(RID());
            // The pooled AudioStreamPlayer children leave the tree with us and
            // stop; reset the controller's bookkeeping to match.
            if (_audio_controller) _audio_controller->stop_all();
            break;
        case NOTIFICATION_INTERNAL_PROCESS:
            if (_process_mode == ANIMATION_PROCESS_IDLE) {
                _push_coverage_screen_scale();
                _internal->update(get_process_delta_time());
                // Post-update: world matrices are final this tick, so part
                // attachments can mirror their parts in the same frame.
                emit_signal(SNAME("frame_updated"), _internal->getFrameNo());
            }
            // Audio voices advance independently of the animation's play/pause
            // state (fire-and-forget), so tick every frame the node processes.
            if (_audio_controller) _audio_controller->tick();
            break;
        case NOTIFICATION_INTERNAL_PHYSICS_PROCESS:
            if (_process_mode == ANIMATION_PROCESS_PHYSICS) {
                _push_coverage_screen_scale();
                _internal->update(get_physics_process_delta_time());
                emit_signal(SNAME("frame_updated"), _internal->getFrameNo());
            }
            if (_audio_controller) _audio_controller->tick();
            break;
        case NOTIFICATION_DRAW:
            // The InternalPlayer handles the actual RenderingServer calls for
            // its per-batch canvas items, but they are nested under our
            // get_canvas_item() so we don't need to do anything here.
            break;
    }
}
