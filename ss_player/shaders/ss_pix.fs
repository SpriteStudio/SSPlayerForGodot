R"GLSL(
void fragment() {
    // ss-pix: ported from SS6 SDK (Common/Drawer/GLSL/ss-pix.fs).
    // Pixelation by quantising UV to a coarse grid of (1 + 96 * fPower)
    // source texels per cell. fPower of 0 leaves the original 1-texel grid
    // (i.e. no quantisation); larger fPower yields chunkier pixels.
    //   ss_param0 = fPower
    float fPower = ss_param0;
    vec2 px = TEXTURE_PIXEL_SIZE;

    float v = 1.0 + 96.0 * fPower;
    vec2 step_uv = vec2(v * px.x, v * px.y);
    vec2 Coord = floor(UV / step_uv) * step_uv;

    vec4 Pixel = ss_input_texture(TEXTURE, Coord);

    vec3 blended = ss_partcolor_blend(Pixel.rgb, partcolor_color.rgb, partcolor_varg);
    float a = Pixel.a * partcolor_color.a;
    COLOR = ss_output_color(vec4(blended, a), partcolor_varg.w);
}
)GLSL"
