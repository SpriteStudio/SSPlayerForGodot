#pragma once

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/node2d.hpp>
using namespace godot;
#else
#include "scene/2d/node_2d.h"
#endif

#include "ss_internal_player.h"
#include "ssab_resource.h"

class SpriteStudioPlayer2D : public Node2D {
    GDCLASS( SpriteStudioPlayer2D, Node2D );

public:
    enum AnimationProcessMode {
        ANIMATION_PROCESS_PHYSICS,
        ANIMATION_PROCESS_IDLE,
    };

private:
    void _notification( int p_notification );
    SpriteStudioPlayer2D();
    ~SpriteStudioPlayer2D();
    static void _bind_methods();
    bool _set( const StringName& p_name, const Variant& p_property );
    bool _get( const StringName& p_name, Variant& r_property ) const;
    void _get_property_list( List<PropertyInfo>* p_list ) const;

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
    void pause();
    void stop();

    void set_flip_h( bool p_flip );
    bool is_flipped_h() const;
    void set_flip_v( bool p_flip );
    bool is_flipped_v() const;
    
    void set_offset( const Vector2& p_offset );
    Vector2 get_offset() const;

    void set_animation_process_mode(int p_mode);
    int get_animation_process_mode() const;

    void setSpeedScale( float p_speed );
    float getSpeedScale() const;
    void setFrame( float p_frame );
    float getFrame() const;

    int getTotalFrames() const;

    int getStartFrame() const;
    int getEndFrame() const;

    void setFrameRate( int p_fps );
    int getFrameRate() const;

    void setAnimationSection( int p_start, int p_end );
    int getAnimationSectionStart() const;
    int getAnimationSectionEnd() const;

    void setPlaybackDirection( int p_direction, int p_style );
    int getPlaybackDirection() const;
    int getPlaybackStyle() const;

    void setLoopCount( int p_count );
    int getLoopCount() const;

    void setFrameSkipEnabled( bool p_skip );
    bool isFrameSkipEnabled() const;

    void setSubFrameEnabled( bool p_enabled );
    bool isSubFrameEnabled() const;

    void set_cellmap_texture(const String &cellmap_name, const Ref<Texture2D> &texture);
    Ref<Texture2D> get_cellmap_texture(const String &cellmap_name) const;

    // ---- Part query API (consumed by SpriteStudioPartAttachment2D) --------
    // Resolve a part name to its index in the current binary; -1 if unknown.
    int get_part_index(const String& part_name) const;
    // Player-local Transform2D of the named part for the current frame. Returns
    // the identity when the part is unknown (use get_part_index to disambiguate).
    Transform2D get_part_transform(const String& part_name) const;
    // True if the named part is hidden on the current frame. False when unknown.
    bool is_part_hidden(const String& part_name) const;
    // All part names in the current binary (for the attachment's dropdown).
    PackedStringArray get_part_names() const;

    // ---- Override Layer (Phase 2): per-part runtime overrides -------------
    // Overrides win over both keyframes and blend for the named part. Return
    // false when the part name is unknown / no animation is bound.
    // priority: 0=on next keyframe (NON), 1=until next animation (default),
    // 2=permanent (survives animation changes). blend_op: 0=Mix 1=Mul 2=Add
    // 3=Sub. Color parts: Normal only; Cell parts: Normal + Mask.
    bool set_part_visibility_override(const String& part_name, bool force_hidden, bool cascade);
    bool clear_part_visibility_override(const String& part_name);
    bool set_part_color_override(const String& part_name, const Color& color, int blend_op, int priority);
    bool clear_part_color_override(const String& part_name);
    bool set_part_cell_override(const String& part_name, const String& cellmap_name, const String& cell_name, int priority);
    bool clear_part_cell_override(const String& part_name);
    bool clear_all_part_overrides();

private:
    // Engine-agnostic playback / render core. The Node2D wrapper feeds it the
    // host canvas item and per-tick delta, and forwards its events back to
    // GDScript signals via _SignalSink.
    SsInternalPlayer* _internal = nullptr;

    HashMap<String, Ref<Texture2D>> _cellmap_overrides;
    bool _autoplay = false;
    bool _flip_h = false;
    bool _flip_v = false;
    Vector2 _offset;
    AnimationProcessMode _process_mode = ANIMATION_PROCESS_IDLE;
    
    void _update_root_transform();
    void _push_coverage_screen_scale();

    // Adapter that turns SsInternalPlayer event callbacks into Node-level
    // emit_signal calls. Lifetime tied to the Node; lives in the cpp file.
    class _SignalSink;
    _SignalSink* _sink = nullptr;

    void _on_ssab_changed();
};
