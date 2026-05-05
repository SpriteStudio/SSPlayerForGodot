#pragma once

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/node2d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/canvas_item_material.hpp>
#include <godot_cpp/variant/rid.hpp>
using namespace godot;
#else
#include "scene/2d/node_2d.h"
#include "scene/resources/canvas_item_material.h"
#include "servers/rendering/rendering_server.h"
#endif

// #include "gd_ssplayer_resource.h"
#include "ssab_resource.h"

namespace ss {
namespace runtime {
struct FrameData;
struct PartState;
}
namespace format {
struct PartData;
struct Cell;
struct SsAnimeBinary;
}
}

class SpriteStudioPlayer2D : public Node2D {
    GDCLASS( SpriteStudioPlayer2D, Node2D );

protected:
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

    void setSubFrameEnabled( bool p_enabled );
    bool isSubFrameEnabled() const;

private:
    Ref<SSABResource> _ssabRes;
    HashMap<uint32_t, Ref<Texture2D>> _textures;
    HashMap<int, Ref<CanvasItemMaterial>> _blend_materials;
    Vector<RID> _canvas_items;
    String _strAnimationSelected;
    ss::format::AnimationData* _currentAnimationData = nullptr;
    void *runtime_ctx = nullptr;
    void *rutime_res = nullptr;
    float previous_frame_no = -1.0f;
    float _speed_rate = 1.0f;
    bool _sub_frame_enabled = false;

    // Per-frame draw context: SoA pointers and frame-shared bindings fetched
    // once at the top of `drawAnimation`. Each `_draw_part_TYPE` extracts its
    // own per-part slice from this struct so dispatch arguments stay flat.
    struct DrawFrame {
        RenderingServer *rs;
        const ss::runtime::FrameData *frameData;
        const ss::format::SsAnimeBinary *binary;

        const float *world_matrices;         uintptr_t world_matrices_len;
        const float *local_uvs;              uintptr_t local_uvs_len;
        const float *cell_meta;              uintptr_t cell_meta_len;
        const uint32_t *cell_texture_hashes; uintptr_t cell_texture_hashes_len;
        const float *local_vertices;         uintptr_t local_vertices_len;
        const float *shape_vertices;         uintptr_t shape_vertices_len;
        const float *shape_box_coords;       uintptr_t shape_box_coords_len;
        const int32_t *shape_vertex_counts;  uintptr_t shape_vertex_counts_len;
    };

    void loadTextures(const Ref<SSABResource>& ssabRes);
    void updateAnimation(float delta);
    void fetchAnimation();
    void drawAnimation(float frame_no);
    void _draw_part(const DrawFrame &f, RID ci, int p_idx, const ss::runtime::PartState *part, const ss::format::PartData *partBinary, const float *draw_m);
    void _draw_part_normal(const DrawFrame &f, RID ci, int p_idx, const ss::runtime::PartState *part, const ss::format::PartData *partBinary, const float *draw_m);
    void _draw_part_shape(const DrawFrame &f, RID ci, int p_idx, const ss::runtime::PartState *part, const ss::format::PartData *partBinary, const float *draw_m);
    void _apply_blend_material(RenderingServer *rs, RID ci, ss::format::BlendType blend_type);

    void _reconfigure();
    void _clear_canvas_items();
};
