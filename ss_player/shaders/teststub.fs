R"GLSL(
void fragment() {
    vec4 p = ss_input_texture(TEXTURE, UV);
    // Stub variant: invert the red channel of the sampled texture so this
    // shader is visually distinct from Default for dispatch verification.
    vec3 rgb = vec3(1.0 - p.r, p.g, p.b);
    rgb = ss_partcolor_blend(rgb, partcolor_color.rgb, partcolor_varg);
    float a = p.a * partcolor_color.a;
    COLOR = ss_output_color(vec4(rgb, a), partcolor_varg.w);
}
)GLSL"
