R"GLSL(
// Pseudo-random hash ported from SS6 SDK (ss-noise.fs:31-34).
float ss_noise_rand(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

void fragment() {
    // ss-noise: ported from SS6 SDK (Common/Drawer/GLSL/ss-noise.fs).
    //   ss_param0 = fPower  (noise intensity 0..1)
    //   ss_param1 = fColor  (color decorrelation 0..1)
    //   ss_param2 = fPhase  (offset; animating this scrolls the noise)
    // SS6 used args[A_TW]/args[A_TH] to quantise the lookup to the
    // source-texel grid; we derive size = 1 / TEXTURE_PIXEL_SIZE.
    float fPower = ss_param0;
    float fColor = ss_param1;
    float fPhase = ss_param2;

    vec2 Coord = UV;
    vec2 px = TEXTURE_PIXEL_SIZE;
    // Quantise to the source-texel grid so the noise pattern is stable
    // per pixel rather than aliasing with the screen.
    vec2 u = floor(Coord / px) * px;

    vec4 Pixel = ss_input_texture(TEXTURE, Coord);

    vec2 t = u + vec2(fPhase);
    vec3 c;
    c.r = ss_noise_rand(t);
    c.g = mix(c.r, ss_noise_rand(t * c.r), fColor);
    c.b = mix(c.g, ss_noise_rand(t * c.g), fColor);

    Pixel.rgb = clamp(mix(Pixel.rgb, c.rgb, fPower), 0.0, 1.0);

    vec3 blended = ss_partcolor_blend(Pixel.rgb, partcolor_color.rgb, partcolor_varg);
    float a = Pixel.a * partcolor_color.a;
    COLOR = ss_output_color(vec4(blended, a), partcolor_varg.w);
}
)GLSL"
