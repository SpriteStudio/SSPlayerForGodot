R"GLSL(
// Pseudo-random hash ported from SS6 SDK (ss-scatter.fs:33-36).
float ss_scatter_rand(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void fragment() {
    // ss-scatter: ported from SS6 SDK (Common/Drawer/GLSL/ss-scatter.fs).
    // Per-texel-cell directional displacement: each grid cell is randomly
    // gated (by fRatio) to scatter its sample along a per-cell random angle.
    //   ss_param0 = fPower (scatter magnitude in pixel units)
    //   ss_param1 = fRatio (probability gate 0..1)
    //   ss_param2 = fPhase (offset; animating scrolls the pattern)
    float fPower = ss_param0;
    float fRatio = ss_param1;
    float fPhase = ss_param2;

    vec2 Coord = UV;
    vec2 px = TEXTURE_PIXEL_SIZE;
    vec2 u = floor(Coord / px) * px;

    vec2 t = u + vec2(fPhase);
    float r = ss_scatter_rand(t) * PI * 2.0 - PI;
    float r2 = step(ss_scatter_rand(t * r), fRatio);
    vec2 v = vec2(sin(r), cos(r)) * px * 96.0 * fPower * r2;

    vec4 Pixel = ss_input_texture(TEXTURE, Coord + v);

    vec3 blended = ss_partcolor_blend(Pixel.rgb, partcolor_color.rgb, partcolor_varg);
    float a = Pixel.a * partcolor_color.a;
    COLOR = ss_output_color(vec4(blended, a), partcolor_varg.w);
}
)GLSL"
