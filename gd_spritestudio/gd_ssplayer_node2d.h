#pragma once

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/rid.hpp>
using namespace godot;
#else
#include "scene/2d/node_2d.h"
#include "core/version.h"
#if VERSION_MAJOR >= 4 && VERSION_MINOR >= 6
#include "servers/rendering/rendering_server.h"
#else
#include "servers/rendering_server.h"
#endif
#endif

// #include "gd_ssplayer_resource.h"
#include "gd_ssab_resource.h"

class GdSsPlayerNode2D : public Node2D {
    GDCLASS( GdSsPlayerNode2D, Node2D );

protected:
    GdSsPlayerNode2D();
    ~GdSsPlayerNode2D();
    static void _bind_methods();
    bool _set( const StringName& p_name, const Variant& p_property );
    bool _get( const StringName& p_name, Variant& r_property ) const;
    void _get_property_list( List<PropertyInfo>* p_list ) const;
    void _notification( int p_notification );

public:
    void setSsabResource( const Ref<GdSsabResource>& ssabRes );
    Ref<GdSsabResource> getSsabResource() const;
    void setAnimation( const String& strName );
    String getAnimation() const;

    bool isPlaying() const;
    void play( int p_start_frame = -1 );
    bool isPausing() const;
    void pause();
    void stop();

    void setSpeed( float p_speed );
    float getSpeed() const;
    void setFrame( int p_frame );
    int getFrame() const;
    float getFrameDecimal() const;

    int getTotalFrames() const;

    void setFrameRate( int p_fps );
    int getFrameRate() const;

    void setAnimationSection( int p_start, int p_end );
    int getAnimationSectionStart() const;
    int getAnimationSectionEnd() const;

    void setPlaybackDirection( int p_direction, int p_style );
    int getPlaybackDirection() const;
    int getPlaybackStyle() const;

    void setLoop( int p_count );
    int getLoop() const;

    void setSkipFrames( bool p_skip );
    bool isSkipFrames() const;

private:
    Ref<GdSsabResource> _ssabRes;
    HashMap<uint32_t, Ref<Texture2D>> _textures;
    Vector<RID> _canvas_items;
    String _strAnimationSelected;
    ss::format::AnimationData* _currentAnimationData = nullptr;
    void *runtime_ctx = nullptr;
    void *rutime_res = nullptr;
    int previous_frame_no = -1;
    float _speed_rate = 1.0f;

    void loadTextures(const Ref<GdSsabResource>& ssabRes);
	void updateAnimation(float delta);
    void fetchAnimation();
    void drawAnimation();

    void _clear_canvas_items();
};
