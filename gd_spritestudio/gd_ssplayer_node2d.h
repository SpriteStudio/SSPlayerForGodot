#pragma once

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/node2d.hpp>
using namespace godot;
#else
#include "scene/2d/node_2d.h"
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
    void play();
    bool isPausing() const;
    void pause();
    void stop();

private:
    Ref<GdSsabResource> _ssabRes;
    HashMap<uint32_t, Ref<Texture2D>> _textures;
    String _strAnimationSelected;
    ss::format::AnimationData* _currentAnimationData = nullptr;
    void *rutime_ctx = nullptr;
    int previous_frame_no = -1;

    void loadTextures(const Ref<GdSsabResource>& ssabRes);
	void updateAnimation(float delta);
    void fetchAnimation();
};
