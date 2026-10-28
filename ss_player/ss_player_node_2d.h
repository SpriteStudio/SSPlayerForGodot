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
        // The Brain's own enumerator names.
        OVERRIDE_PRIORITY_OVERWRITE_ON_NEXT_KEYFRAME, // the animation takes the attribute back at its next key, for good
        OVERRIDE_PRIORITY_HOLD_UNTIL_NEXT_ANIMATION,  // held until another animation is set up
        OVERRIDE_PRIORITY_PERMANENT,                  // survives animation changes
    };

    // How a track lays its animation over the tracks below it. Mirrors the
    // runtime's SS_RUNTIME_TRACK_BLEND_* encoding.
    enum TrackBlend {
        TRACK_BLEND_MIX,      // lerp toward the track's pose by its alpha
        TRACK_BLEND_ADDITIVE, // add its offset from the setup pose
    };

    // Where an animation laid on a track starts. Mirrors the runtime's
    // SS_RUNTIME_TRACK_SYNC_* encoding.
    enum TrackSync {
        TRACK_SYNC_FREE,  // from the head of its section
        TRACK_SYNC_START, // at the phase track 0's animation stands at
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
    String getCurrentAnimation() const;

    void setAutoplay( bool p_autoplay );
    bool isAutoplay() const;

    bool isPlaying() const;
    void play( float p_start_frame = -1.0f );
    bool isPausing() const;
    // Which way the playhead is actually travelling, not the configured heading
    // (getPlaybackDirection). Diverges under ping-pong, and it is what gates audio.
    bool isPlayingForward() const;
    // The loop pulse: true only inside the tick that crossed a boundary, which is
    // why it is not called is_looped, here or in the Brain.
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
    // The node's half of a crossfade or a track animation on track 0, which
    // switches the animation without a `setup_animation`.
    void _track_zero_switched();

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

    bool setAnimationSection( int p_start, int p_end );
    // The section from p_start_label's frame plus p_start_offset to
    // p_end_label's plus p_end_offset, in the current animation -- the rule an
    // Instance part's start and end labels follow. Unlike an Instance part, a
    // label the animation does not have is refused rather than read as its edge.
    bool set_animation_section_by_label( const String& p_start_label, const String& p_end_label,
                                         int p_start_offset = 0, int p_end_offset = 0 );
    // The labels of p_animation (the current one when empty) in frame order, as
    // { "name", "frame" } dictionaries. Empty for an animation the SSAB lacks.
    Array get_label_table( const String& p_animation = String() ) const;
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
    // Bus the built-in voices play on, as AudioStreamPlayer.bus: a name the bus
    // layout does not have plays on "Master". Ignored when a backend is set.
    void set_audio_bus(const StringName &p_bus);
    StringName get_audio_bus() const;
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
    // True if the named part is a bone-skinned mesh. Such a part's vertices are
    // drawn by skinning while its own transform stays near the setup pose, so it
    // is a poor follow target — the attachment node uses this to flag it. False
    // when the part is unknown or is not a skinned mesh.
    bool is_part_skinned_mesh(const String& part_name) const;
    // All part names in the current binary (for the attachment's dropdown).
    PackedStringArray get_part_names() const;
    // The part's outline on the current frame in this node's local space, flip
    // and offset included, as top-left, top-right, bottom-right, bottom-left.
    // Empty when the part is unknown, hidden or draws nothing of its own.
    PackedVector2Array get_part_quad(const String& part_name) const;
    // The index of the front-most part, among p_parts (every part when empty),
    // whose outline holds p_point (local space); -1 when none does. A name the
    // SSAB does not have is skipped.
    int hit_test_part(const Vector2& p_point, const PackedStringArray& p_parts = PackedStringArray()) const;

    // ---- Authored canvas --------------------------------------------------
    // The current animation's authored canvas — see SSABResource for what that
    // box is and is not. Zero-sized when nothing is bound.
    Vector2 get_canvas_size() const;
    // The same canvas placed against this node's origin by the animation's
    // pivot, with flip / offset composed in exactly as get_part_transform does,
    // so it is a box in THIS node's local space. Feed it to a
    // VisibleOnScreenEnabler2D to stop evaluating a player that is off screen —
    // re-reading it on `animation_changed`, because the canvas is authored per
    // animation and a pack's animations do not all share one.
    Rect2 get_canvas_rect() const;
    // get_canvas_rect for p_animation rather than the current one. Not bound:
    // SpriteStudioButton fits the animation its states are read from, which
    // stays put while a state plays another.
    Rect2 get_canvas_rect_of(const String& p_animation) const;

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

    // ---- Animation mixing: the track stack ---------------------------------
    // Track 0 is the animation set_animation() selects; the tracks above it lay
    // other animations of the same SSAB over it. set_animation() to another
    // animation folds the stack back to track 0 alone, every track's settings
    // included. Durations are in seconds, as `advance()` takes them. Mutators
    // return false when nothing is set up, the animation name is unknown, or
    // the runtime refuses the call. Reads of a track or source that does not
    // exist return 0 (check get_track_count() first), except
    // get_track_fade_progress() (-1 when not fading) and the source handles
    // (-1 for none), where 0 is a real answer.
    bool set_track_animation(int track, const String& anim_name);
    int get_track_count() const;
    bool clear_track(int track);
    bool set_track_alpha(int track, float alpha);
    float get_track_alpha(int track) const;
    bool set_track_blend(int track, TrackBlend blend);
    TrackBlend get_track_blend(int track) const;
    bool set_track_sync(int track, TrackSync sync);
    TrackSync get_track_sync(int track) const;
    bool set_track_frame(int track, float frame_no);
    float get_track_frame(int track) const;
    bool set_track_speed(int track, float speed);
    // A track's playback settings apply to every animation on it, those a
    // crossfade fades in included, and outlast them; track 0's are the
    // speed_scale / frame_rate / frame_skip_enabled / playback_direction this
    // node already has. The loop count belongs to the animation instead: one
    // laid on a track loops forever until set again, except on track 0, where
    // set_track_loop_count is set_loop_count and the node puts it back.
    float get_track_speed(int track) const;
    bool set_track_frame_rate(int track, int fps);
    int get_track_frame_rate(int track) const;
    bool set_track_frame_skip_enabled(int track, bool enabled);
    bool is_track_frame_skip_enabled(int track) const;
    bool set_track_playback_direction(int track, PlaybackDirection direction, PlaybackStyle style);
    PlaybackDirection get_track_playback_direction(int track) const;
    PlaybackStyle get_track_playback_style(int track) const;
    bool set_track_loop_count(int track, int count);
    int get_track_loop_count(int track) const;
    // States, read from the last tick: a finished track holds its last frame,
    // still mixed in, until the host fades or clears it. No signal is sent for
    // either; track 0's are animation_finished / animation_looped.
    bool is_track_finished(int track) const;
    bool track_just_looped(int track) const;
    // The outgoing side of a crossfade on `track` fires events while its share,
    // 1 - progress, is at least this. The default, INF, silences it from the
    // moment the fade begins.
    bool set_track_outgoing_event_threshold(int track, float threshold);
    float get_track_outgoing_event_threshold(int track) const;
    bool crossfade(int track, const String& anim_name, float duration);
    bool fade_track(int track, float target_alpha, float duration);
    float get_track_fade_progress(int track) const;
    bool cancel_track_fade(int track);
    int64_t get_track_main_source(int track) const;
    int64_t add_track_source(int track, const String& anim_name, float weight);
    bool set_source_weight(int64_t source, float weight);
    float get_source_weight(int64_t source) const;
    bool set_source_frame(int64_t source, float frame_no);
    float get_source_frame(int64_t source) const;
    bool set_source_event_threshold(int64_t source, float threshold);
    float get_source_event_threshold(int64_t source) const;

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
    StringName _audio_bus = StringName("Master");
    Ref<SpriteStudioAudioBackend> _audio_backend;

    // Routes one audio event to the backend / built-in controller, applying the
    // forward-playback and editor guards. The "audio" signal fires regardless.
    void _handle_audio(const Dictionary& payload);

    HashMap<String, Ref<Texture2D>> _cellmap_overrides;
    // The commands already reported as having done nothing, by the call and
    // what it was given, since the SSAB was set. See _refused.
    HashSet<String> _reported_refusals;
    bool _refused(bool p_ok, const String& p_call, const String& p_why = String());
    String _no_part(int p_index) const;
    String _no_animation(const String& p_name) const;
    // What the drawn box depends on -- the SSAB, the animation, flip, offset --
    // changed, which CanvasItem says with `item_rect_changed`, as Sprite2D does.
    void _drawn_rect_changed();
    // hit_test_part's resolved names, kept so a pointer moving over a button
    // does not allocate.
    mutable LocalVector<int> _hit_targets;
    // On, so a node dropped into a scene animates when the scene runs. A player
    // sitting on its first frame reads as broken, and there is no code to call
    // `play()` for a node that was placed rather than constructed -- which is
    // what an `autoplay` property is for. ForUnity's `AutoPlay` defaults the
    // same way, for the same reason.
    bool _autoplay = true;
    // The loop count as configured on the node — an inspector property, so it is
    // the node's own state and has to outlive an animation change. The runtime
    // resets its own count on every `setup_animation` (the transition is meant to
    // state it), and on every animation laid on track 0 by a crossfade or a
    // track animation, so this is pushed back afterwards by
    // _apply_transport_settings.
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
    // self_modulate only reaches a canvas item's own draw commands, and this
    // node has none: the parts hang off the internal root canvas item, so the
    // value is mirrored there. Setting it tells the node nothing, so every tick
    // compares against what was last pushed.
    void _push_self_modulate();
    // texture_filter / texture_repeat / light_mask likewise reach only the
    // node's own canvas item, so the parts' canvas items are handed them.
    // Filter and repeat -- this node's or an ancestor's -- arrive as a redraw,
    // where they are resolved; the light mask tells the node nothing, so every
    // tick passes it on and the core ignores what has not changed.
    void _resolve_texture_settings();
    void _push_canvas_item_defaults();
    int _resolved_texture_filter = 0;
    int _resolved_texture_repeat = 0;
    Color _pushed_self_modulate = Color(1, 1, 1, 1);

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
VARIANT_ENUM_CAST(SpriteStudioPlayer2D::TrackBlend);
VARIANT_ENUM_CAST(SpriteStudioPlayer2D::TrackSync);
