R"GLSL(
void fragment() {
    vec4 p = ss_input_texture(TEXTURE, UV);
    vec3 rgb = ss_partcolor_blend(p.rgb, partcolor_color.rgb, partcolor_varg);
    float a = p.a * partcolor_color.a;
    COLOR = ss_output_color(vec4(rgb, a), partcolor_varg.w);
}
)GLSL"
