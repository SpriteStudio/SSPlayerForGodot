R"GLSL(
uniform sampler2D map0;
uniform sampler2D map1;

// Cell rectangle in UV space: (left_u, top_v, right_u, bottom_v).
// Populated per-part on the per-part material path (Default and other
// shareable variants leave it at zero — they don't reference it).
// `ss-circle` / `ss-spot` use this to clip-sample inside the part's
// cell bounds; the cell center is `(ss_cell_rect.xy + ss_cell_rect.zw) * 0.5`.
uniform vec4 ss_cell_rect;

uniform float ss_param0;
uniform float ss_param1;
uniform float ss_param2;
uniform float ss_param3;
uniform float ss_param4;
uniform float ss_param5;
uniform float ss_param6;
uniform float ss_param7;

// ---- CBP masking (P3) -----------------------------------------------------
// Coverage bitmap (offscreen RGBA8 = 32 writer bits), per-frame writer
// metadata, and this part's rank / polarity. Masking is off by default
// (ss_mask_enabled == false) so non-masked draws are untouched. `ss_mask_uv`
// is the fragment's coverage UV, written in the vertex stage from the
// player-local position via `ss_mask_uv_xform`.
// MUST be nearest: the coverage packs writer bits into the RGBA8 channels, so
// any linear blending across texels corrupts the per-bit decode (edge halos).
uniform sampler2D ss_mask_coverage : filter_nearest;
uniform bool ss_mask_enabled;
uniform vec4 ss_mask_uv_xform;        // (scale.x, scale.y, offset.x, offset.y)
uniform int ss_mask_count;            // active writer count (0..32)
uniform vec4 ss_mask_meta[32];        // per writer: (slot, bit, op_invert, is_clipping)
uniform float ss_mask_rank;           // this part's draw-order rank
uniform float ss_mask_visible_inside; // 1 = draw inside mask region, 0 = outside
varying vec2 ss_mask_uv;

vec4 ss_input_texture(sampler2D tex, vec2 uv) {
    return texture(tex, uv);
}

vec3 ss_partcolor_blend(vec3 pixel_rgb, vec3 color_rgb, vec4 varg) {
    return pixel_rgb * varg.x + mix(vec3(1.0), pixel_rgb, varg.z) * color_rgb * varg.y;
}

// Reconstruct the masked / unmasked state at this fragment by replaying the
// active mask writers in slot order (CBP). Returns true when the fragment
// should be drawn. Writer `i`'s coverage bit lives in `ss_mask_meta[i]`.
bool ss_mask_passes() {
    if (!ss_mask_enabled || ss_mask_count <= 0) {
        return true;
    }
    vec4 cov = texture(ss_mask_coverage, ss_mask_uv);
    int byte0 = int(cov.r * 255.0 + 0.5);
    int byte1 = int(cov.g * 255.0 + 0.5);
    int byte2 = int(cov.b * 255.0 + 0.5);
    int byte3 = int(cov.a * 255.0 + 0.5);
    int stencil = 0;
    for (int i = 0; i < ss_mask_count; i++) {
        vec4 m = ss_mask_meta[i];
        bool is_clipping = m.w > 0.5;
        // Mask part masks parts drawn before it (rank < slot); a clipping
        // writer masks parts drawn after it (rank > slot).
        bool active = is_clipping ? (ss_mask_rank > m.x) : (ss_mask_rank < m.x);
        if (!active) { continue; }
        int bit = int(m.y + 0.5);
        int chan = bit / 8;
        int b = bit - chan * 8;
        int byte_val = chan == 0 ? byte0 : (chan == 1 ? byte1 : (chan == 2 ? byte2 : byte3));
        if (((byte_val >> b) & 1) == 0) { continue; }
        if (m.z > 0.5) {
            stencil = (~stencil) & 0xFF; // invert (8-bit bitwise NOT)
        } else {
            stencil = (stencil + 1) & 0xFF; // increment (wrap)
        }
    }
    bool masked = stencil != 0;
    return masked == (ss_mask_visible_inside > 0.5);
}

vec4 ss_output_color(vec4 c, float pma_flag) {
    if (!ss_mask_passes()) {
        discard;
    }
    return pma_flag > 0.5 ? vec4(c.rgb * c.a, c.a) : c;
}
)GLSL"
