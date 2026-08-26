#pragma once

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/core/binder_common.hpp>
using namespace godot;
#else
#include "scene/2d/node_2d.h"
#include "core/variant/binder_common.h"
#endif

#include "ss_audio_backend.h"
#include "ss_audio_controller.h"
#include "ss_internal_player.h"
#include "ssab_resource.h"

class SpriteStudioPlayer2D : public Node2D {
    GDCLASS( SpriteStudioPlayer2D, Node2D );

public:
    enum AnimationProcessMode {
        ANIMATION_PROCESS_PHYSICS,
        ANIMATION_PROCESS_IDLE,
        // The node stops advancing itself; the caller drives playback with
        // advance(). Audio voices still tick, because they are fire-and-forget
        // and outlive the frame that started them.
        ANIMATION_PROCESS_MANUAL,
    };

    // Playback direction / style. The values mirror the runtime's FFI encoding
    // (which normalizes to 0/1 on both sides of the boundary) — they are NOT
    // the Rust PlaybackDirection discriminants (Forward=1, Backward=-1).
    enum PlaybackDirection {
        PLAYBACK_DIRECTION_FORWARD,
        PLAYBACK_DIRECTION_BACKWARD,
    };

    enum PlaybackStyle {
        PLAYBACK_STYLE_NORMAL,
        PLAYBACK_STYLE_PING_PONG,
    };

    // Blend operation applied by the part color overrides. Mirrors the
    // runtime's `blend_op` encoding.
    enum ColorBlendOperation {
        COLOR_BLEND_MIX,
        COLOR_BLEND_MUL,
        COLOR_BLEND_ADD,
        COLOR_BLEND_SUB,
    };

    // How long a part override outlives the keyframes it overrides. Mirrors
    // the runtime's `priority_mode` encoding.
    enum OverridePriority {
        // The Brain's own enumerator names (SDK: 20_design/40_api_conventions).
        OVERRIDE_PRIORITY_OVERWRITE_ON_NEXT_KEYFRAME, // the animation takes the attribute back at its next key, for good
        OVERRIDE_PRIORITY_HOLD_UNTIL_NEXT_ANIMATION,  // held until another animation is set up
        OVERRIDE_PRIORITY_PERMANENT,                  // survives animation changes
    };

private:
    void _notification( int p_notification );
    SpriteStudioPlayer2D();
    ~SpriteStudioPlayer2D();
    static void _bind_methods();
    bool _set( const StringName& p_name, const Variant& p_property );
    bool _get( const StringName& p_name, Variant& r_property ) const;
    void _get_property_list( List<PropertyInfo>* p_list ) const;
    // Injects the hints that depend on the bound resource / current animation
    // (the `animation` name list, and the frame + section ranges) into the
    // statically registered properties. Same signature in module + godot-cpp.
    void _validate_property( PropertyInfo& p_property ) const;

public:
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    PackedStringArray _get_configuration_warnings() const override;
#else
    PackedStringArray get_configuration_warnings() const override;
#endif

    void setSSABResource( const Ref<SSABResource>& ssabRes );
    Ref<SSABResource> getSSABResource() const;
    void setAnimation( const String& strName );
    String getAnimation() const;

    void setAutoplay( bool p_autoplay );
    bool isAutoplay() const;

    bool isPlaying() const;
    void play( float p_start_frame = -1.0f );
    bool isPausing() const;
    // Which way the playhead is actually travelling, not the configured heading
    // (getPlaybackDirection). Diverges under ping-pong, and it is what gates audio.
    bool isPlayingForward() const;
    // The loop pulse: true only inside the tick that crossed a boundary, which is
    // why it is not called is_looped.
    bool justLooped() const;
    // The completion state, and sticky where just_looped is a pulse: it holds until
    // the next play()/set_animation(). Never true under an infinite loop count, and
    // not raised by stop(), so it separates a run that ended from one that was
    // stopped where `not is_playing()` cannot.
    bool isFinished() const;
    PackedStringArray get_animation_names() const;
    void pause();
    void resume();
    void stop();

    void set_flip_h( bool p_flip );
    bool is_flipped_h() const;
    void set_flip_v( bool p_flip );
    bool is_flipped_v() const;
    
    void set_offset( const Vector2& p_offset );
    Vector2 get_offset() const;

    void set_animation_process_mode(AnimationProcessMode p_mode);

private:
    // Pushes the node's own transport configuration back onto the runtime after
    // anything that runs `setup_animation` underneath us.
    void _apply_transport_settings();

public:
    AnimationProcessMode get_animation_process_mode() const;

    // Steps playback by p_delta seconds and emits frame_updated, exactly as an
    // automatic tick would. Meant for ANIMATION_PROCESS_MANUAL; calling it in
    // the other modes advances the animation on top of the node's own tick.
    void advance(double p_delta);

    void setSpeedScale( float p_speed );
    float getSpeedScale() const;
    void setFrameNo( float p_frame );
    float getFrameNo() const;

    int getTotalFrames() const;

    int getStartFrame() const;
    int getEndFrame() const;

    void setFrameRate( int p_fps );
    int getFrameRate() const;

    void setAnimationSection( int p_start, int p_end );
    // Single-endpoint setters backing the `animation_section_start` /
    // `animation_section_end` properties; each keeps the other endpoint.
    void setAnimationSectionStart( int p_start );
    void setAnimationSectionEnd( int p_end );
    int getAnimationSectionStart() const;
    int getAnimationSectionEnd() const;

    void setPlaybackDirection( PlaybackDirection p_direction, PlaybackStyle p_style );
    PlaybackDirection getPlaybackDirection() const;
    PlaybackStyle getPlaybackStyle() const;

    void setLoopCount( int p_count );
    int getLoopCount() const;

    void setFrameSkipEnabled( bool p_skip );
    bool isFrameSkipEnabled() const;

    void setSubFrameEnabled( bool p_enabled );
    bool isSubFrameEnabled() const;

    void set_cellmap_texture(const String &cellmap_name, const Ref<Texture2D> &texture);
    Ref<Texture2D> get_cellmap_texture(const String &cellmap_name) const;

    // Cellmap / cell names from the bound SSABResource — the discovery half of
    // set_part_cell_override(). Empty when no resource is bound.
    PackedStringArray get_cellmap_names() const;
    PackedStringArray get_cell_names(const String &cellmap_name) const;

    // ---- Audio -------------------------------------------------------------
    // Built-in audio playback for the animation's audio events. Toggle off to
    // handle audio yourself via the "audio" signal.
    void set_play_audio(bool p_enabled);
    bool is_play_audio() const;
    // Volume of built-in playback, linear [0,1]. Ignored when a backend is set.
    void set_audio_volume(float p_volume);
    float get_audio_volume() const;
    // Optional override backend; when set, built-in playback is suppressed and
    // audio events are routed to it instead.
    void set_audio_backend(const Ref<SpriteStudioAudioBackend> &p_backend);
    Ref<SpriteStudioAudioBackend> get_audio_backend() const;

    // ---- Part query API (consumed by SpriteStudioPartAttachment2D) --------
    // Resolve a part name to its index in the current binary; -1 if unknown.
    int find_part_index(const String& part_name) const;
    // Player-local Transform2D of the named part for the current frame. Returns
    // the identity when the part is unknown (use find_part_index to disambiguate).
    Transform2D get_part_transform(const String& part_name) const;
    // True if the named part is hidden on the current frame. False when unknown.
    bool is_part_hidden(const String& part_name) const;
    // All part names in the current binary (for the attachment's dropdown).
    PackedStringArray get_part_names() const;

    // ---- Override Layer (Phase 2): per-part runtime overrides -------------
    // Overrides win over both keyframes and blend for the named part. Return
    // false when the part name is unknown / no animation is bound.
    // See OverridePriority and ColorBlendOperation for the enum values.
    // Color parts: Normal only; Cell parts: Normal + Mask.
    bool set_part_visibility_override(const String& part_name, bool force_hidden, bool cascade);
    bool clear_part_visibility_override(const String& part_name);
    bool set_part_color_override(const String& part_name, const Color& color, ColorBlendOperation blend_op, OverridePriority priority);
    // Four-corner (per-vertex) color override. Corners follow the runtime's
    // order: left-top, right-top, left-bottom, right-bottom. Shares one slot
    // with set_part_color_override — clear_part_color_override clears either.
    bool set_part_color_override_corners(const String& part_name, const PackedColorArray& corners,
                                         ColorBlendOperation blend_op, OverridePriority priority);
    bool clear_part_color_override(const String& part_name);
    bool set_part_cell_override(const String& part_name, const String& cellmap_name, const String& cell_name, OverridePriority priority);
    bool clear_part_cell_override(const String& part_name);
    bool clear_all_part_overrides();
    // By-index variants (part_index from find_part_index): skip the name lookup.
    bool set_part_visibility_override_by_index(int part_index, bool force_hidden, bool cascade);
    bool clear_part_visibility_override_by_index(int part_index);
    bool set_part_color_override_by_index(int part_index, const Color& color, ColorBlendOperation blend_op, OverridePriority priority);
    bool set_part_color_override_corners_by_index(int part_index, const PackedColorArray& corners,
                                                  ColorBlendOperation blend_op, OverridePriority priority);
    bool clear_part_color_override_by_index(int part_index);
    bool set_part_cell_override_by_index(int part_index, const String& cellmap_name, const String& cell_name, OverridePriority priority);
    bool clear_part_cell_override_by_index(int part_index);

private:
    // Engine-agnostic playback / render core. The Node2D wrapper feeds it the
    // host canvas item and per-tick delta, and forwards its events back to
    // GDScript signals via _SignalSink.
    SsInternalPlayer* _internal = nullptr;

    // Built-in audio playback (lazily created on the first audio event during
    // forward playback). Owns pooled AudioStreamPlayer children.
    SsAudioController* _audio_controller = nullptr;
    bool _play_audio = true;
    float _audio_volume = 1.0f;
    Ref<SpriteStudioAudioBackend> _audio_backend;

    // Routes one audio event to the backend / built-in controller, applying the
    // forward-playback and editor guards. The "audio" signal fires regardless.
    void _handle_audio(const Dictionary& payload);

    HashMap<String, Ref<Texture2D>> _cellmap_overrides;
    // On, so a node dropped into a scene animates when the scene runs. A player
    // sitting on its first frame reads as broken, and there is no code to call
    // `play()` for a node that was placed rather than constructed -- which is
    // what an `autoplay` property is for. ForUnity's `AutoPlay` defaults the
    // same way, for the same reason.
    bool _autoplay = true;
    // The loop count as configured on the node — an inspector property, so it is
    // the node's own state and has to outlive an animation change. The runtime
    // resets its own count on every `setup_animation` (the transition is meant to
    // state it), so this is pushed back afterwards by _apply_transport_settings.
    // Speed, frame skip and the frame-rate override need no such handling: the
    // runtime carries those across a setup itself.
    int _loop_count = -1;
    bool _flip_h = false;
    bool _flip_v = false;
    Vector2 _offset;
    AnimationProcessMode _process_mode = ANIMATION_PROCESS_IDLE;
    
    // flip_h / flip_v / offset as one matrix. Applied to the internal root canvas
    // item (so it is NOT part of the Node2D transform) and composed onto part
    // transforms by get_part_transform, which must report where a part is drawn.
    Transform2D _make_root_transform() const;
    void _update_root_transform();
    void _push_coverage_screen_scale();
    void _push_host_viewport();

    // Adapter that turns SsInternalPlayer event callbacks into Node-level
    // emit_signal calls. Lifetime tied to the Node; lives in the cpp file.
    class _SignalSink;
    _SignalSink* _sink = nullptr;

    void _on_ssab_changed();
};

VARIANT_ENUM_CAST(SpriteStudioPlayer2D::AnimationProcessMode);
VARIANT_ENUM_CAST(SpriteStudioPlayer2D::PlaybackDirection);
VARIANT_ENUM_CAST(SpriteStudioPlayer2D::PlaybackStyle);
VARIANT_ENUM_CAST(SpriteStudioPlayer2D::ColorBlendOperation);
VARIANT_ENUM_CAST(SpriteStudioPlayer2D::OverridePriority);
