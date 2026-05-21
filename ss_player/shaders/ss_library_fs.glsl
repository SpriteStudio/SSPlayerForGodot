R"GLSL(
vec4 ss_input_texture(sampler2D tex, vec2 uv) {
    return texture(tex, uv);
}

vec3 ss_partcolor_blend(vec3 pixel_rgb, vec3 color_rgb, vec4 varg) {
    return pixel_rgb * varg.x + mix(vec3(1.0), pixel_rgb, varg.z) * color_rgb * varg.y;
}

vec4 ss_output_color(vec4 c, float pma_flag) {
    return pma_flag > 0.5 ? vec4(c.rgb * c.a, c.a) : c;
}
)GLSL"
