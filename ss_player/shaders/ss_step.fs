R"GLSL(
void fragment() {
    // ss-step: ported from SS6 SDK (Common/Drawer/GLSL/ss-step.fs).
    // ss_param0 = fThreshold (signed; sign flips bright/dark target),
    // ss_param1 = fStage     (number of posterise stages),
    // ss_param2 = fColor     (mix between mono and texture-tinted).
    float fThreshold = ss_param0;
    float fStage     = ss_param1;
    float fColor     = ss_param2;

    vec4 Pixel = ss_input_texture(TEXTURE, UV);

    float d = 1.0 + 255.0 * abs(fStage);
    float e = 1.0e-10;
    float t = abs(fThreshold);
    float b = abs(step(fThreshold, 0.0) - max(Pixel.r, max(Pixel.g, Pixel.b)));
    float r = clamp(floor((b - t) / (t + e) * 256.0 / d) * d / 256.0, 0.0, 1.0);
    float c = step(t, b) * r;
    vec3 v = mix(vec3(c), Pixel.rgb * c, fColor);

    vec3 blended = ss_partcolor_blend(v, partcolor_color.rgb, partcolor_varg);
    float a = Pixel.a * partcolor_color.a;
    COLOR = ss_output_color(vec4(blended, a), partcolor_varg.w);
}
)GLSL"
