R"GLSL(
void fragment() {
    // ss-bmask: ported from SS6 SDK (Common/Drawer/GLSL/ss-bmask.fs).
    // ss_param0 = fBrightness (signed). Pixels are discarded when their
    // max(rgb) lies on the wrong side of the threshold:
    //   fBrightness > 0 : keep pixels with max(rgb) >  fBrightness (mask dark)
    //   fBrightness < 0 : keep pixels with max(rgb) <  1 - |fBrightness| (mask bright)
    float fBrightness = ss_param0;
    vec4 Pixel = ss_input_texture(TEXTURE, UV);

    float b = abs(step(fBrightness, 0.0) - max(Pixel.r, max(Pixel.g, Pixel.b)));
    if (b <= abs(fBrightness)) discard;

    vec3 blended = ss_partcolor_blend(Pixel.rgb, partcolor_color.rgb, partcolor_varg);
    float a = Pixel.a * partcolor_color.a;
    COLOR = ss_output_color(vec4(blended, a), partcolor_varg.w);
}
)GLSL"
