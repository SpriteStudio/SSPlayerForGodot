// Default fragment shader for SpriteStudio canvas_item draws.
// Applies the SS6 SDK PartColor compositing formula
// (Common/Drawer/GLSL/default.fs:27) followed by optional output PMA.
// Pairs with default.vs which sets up the per-vertex `partcolor_varg`
// (coefficient triplet + PMA flag) and `partcolor_color` (PartColor rgba,
// alpha pre-multiplied with the part's alpha on the CPU).
//
// File contents are a C++ raw string literal so the .fs file can be
// `#include`'d directly into a `const char*` definition. See
// ss_internal_player.cpp::_ensure_partcolor_shader() for the composition.

R"GLSL(
vec3 ss_partcolor_blend(vec3 pixel_rgb, vec3 color_rgb, vec4 varg) {
    return pixel_rgb * varg.x + mix(vec3(1.0), pixel_rgb, varg.z) * color_rgb * varg.y;
}

vec4 ss_apply_output_pma(vec4 c, float pma_flag) {
    return pma_flag > 0.5 ? vec4(c.rgb * c.a, c.a) : c;
}

void fragment() {
    vec4 p = texture(TEXTURE, UV);
    vec3 rgb = ss_partcolor_blend(p.rgb, partcolor_color.rgb, partcolor_varg);
    float a = p.a * partcolor_color.a;
    COLOR = ss_apply_output_pma(vec4(rgb, a), partcolor_varg.w);
}
)GLSL"
