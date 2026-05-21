R"GLSL(
void fragment() {
    // ss-sepia: ported from SS6 SDK (Common/Drawer/GLSL/ss-sepia.fs).
    // ss_param0 carries the signed strength `fPower`:
    //   fPower > 0  : warming sepia tint over the luminance-grayscale.
    //   fPower < 0  : pure luminance-grayscale, no warming.
    //   fPower == 0 : pass-through (mix factor abs(fPower) == 0).
    // |fPower| is the blend factor between the original sample and the
    // toned color.
    float fPower = ss_param0;
    vec4 Pixel = ss_input_texture(TEXTURE, UV);

    // ITU-R BT.601 weights from the SS6 reference (ss-sepia.fs:33-35).
    float gray = dot(Pixel.rgb, vec3(0.298912, 0.586611, 0.114478));
    vec3 GrayRgb = vec3(gray);
    // Sepia tint scale from the SS6 reference (ss-sepia.fs:43-45).
    vec3 SepiaRgb = GrayRgb * vec3(1.07, 0.74, 0.43);
    // step(edge, x) returns 1.0 when x >= edge — here we want 1.0 when
    // fPower <= 0.0, so we test step(fPower, 0.0).
    vec3 toned = mix(SepiaRgb, GrayRgb, step(fPower, 0.0));
    vec3 mixed_rgb = mix(Pixel.rgb, toned, abs(fPower));

    vec3 blended = ss_partcolor_blend(mixed_rgb, partcolor_color.rgb, partcolor_varg);
    float a = Pixel.a * partcolor_color.a;
    COLOR = ss_output_color(vec4(blended, a), partcolor_varg.w);
}
)GLSL"
