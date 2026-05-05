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
    void setFrameRelative( float p_diff );
    float getFrame() const;

    // Marks this Player as a child of an Instance part on its parent Player
    // so it skips its own auto-update — the parent will drive it via
    // setFrameRelative every frame. Defaults to false (root Players run
    // their own playback).
    void setInstanceChildMode( bool p_enabled );

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
    bool _instance_child_mode = false;

    // One Player per Instance part on this Player, indexed by part_index.
    // Slots for non-Instance parts hold nullptr. The parent owns the
    // children via Godot's node tree (added with add_child); cleared and
    // rebuilt every fetchAnimation.
    Vector<SpriteStudioPlayer2D*> _instance_players;
    // Externally-referenced SSAB resources discovered through the parent's
    // `external_instances` array. Auto-loaded from the parent's directory
    // (libssconverter places one .ssab per ssae alongside each other) so a
    // PartTypeInstance whose ref_anime lives in another file can still be
    // resolved without the user wiring it manually.
    Vector<Ref<SSABResource>> _external_ssabs;

    // Per-frame draw context: SoA pointers and frame-shared bindings fetched
    // once at the top of `drawAnimation`. Each `_draw_part_TYPE` extracts its
    // own per-part slice from this struct so dispatch arguments stay flat.
    struct DrawFrame {
        RenderingServer *rs;
        const ss::runtime::FrameData *frameData;
        const ss::format::SsAnimeBinary *binary;
        // Parent's current frame, needed by _draw_part_instance to compute
        // diff = (parent_frame - event_frame) * speed before driving the
        // child Player via setFrameRelative.
        float frame_no;

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
    void _draw_part_instance(const DrawFrame &f, RID ci, int p_idx, const ss::runtime::PartState *part, const ss::format::PartData *partBinary, const float *draw_m);

    // Setup / teardown of the per-Instance-part child Players. Called from
    // fetchAnimation; results are stored in `_instance_players`.
    void _setup_instance_players();
    void _clear_instance_players();
    // Auto-load every SSAB referenced by `_ssabRes->external_instances` from
    // the same directory as the parent file. Cleared and rebuilt on every
    // setSSABResource.
    void _load_external_ssabs();
    // Searches `_ssabRes` first, then `_external_ssabs`, for an animation
    // whose `name_hash` matches. Sets `out_source` to the SSAB containing
    // the match (null when not found). Returns the animation name as
    // utf8 String (empty when not found).
    String _resolve_animation_by_hash(uint32_t name_hash, Ref<SSABResource>& out_source) const;
    void _apply_blend_material(RenderingServer *rs, RID ci, ss::format::BlendType blend_type);

    void _reconfigure();
    void _clear_canvas_items();
};
