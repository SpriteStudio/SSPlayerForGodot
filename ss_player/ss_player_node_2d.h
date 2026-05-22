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

    SpriteStudioPlayer2D();
    ~SpriteStudioPlayer2D();
    static void _bind_methods();
    bool _set( const StringName& p_name, const Variant& p_property );
    bool _get( const StringName& p_name, Variant& r_property ) const;
    void _get_property_list( List<PropertyInfo>* p_list ) const;
    void _notification( int p_notification );

public:
    void setSSABResource( const Ref<SSABResource>& ssabRes );
    Ref<SSABResource> getSSABResource() const;
    void setAnimation( const String& strName );
    String getAnimation() const;

    bool isPlaying() const;
    void play( float p_start_frame = -1.0f );
    bool isPausing() const;
    void pause();
    void stop();

    void setSpeed( float p_speed );
    float getSpeed() const;
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

private:
    // Engine-agnostic playback / render core. The Node2D wrapper feeds it the
    // host canvas item and per-tick delta, and forwards its events back to
    // GDScript signals via _SignalSink.
    SsInternalPlayer* _internal = nullptr;

    HashMap<String, Ref<Texture2D>> _cellmap_overrides;

    // Adapter that turns SsInternalPlayer event callbacks into Node-level
    // emit_signal calls. Lifetime tied to the Node; lives in the cpp file.
    class _SignalSink;
    _SignalSink* _sink = nullptr;

    void _on_ssab_changed();
};
