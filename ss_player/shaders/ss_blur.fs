R"GLSL(
void fragment() {
    // ss-blur: ported from SS6 SDK (Common/Drawer/GLSL/ss-blur.fs).
    // 9-tap box blur over a roughly 16-pixel radius scaled by ss_param0
    // (-1..1 focus shift).
    //
    // Note: the SS6 blur is unusual among the ss-* shaders in that it does
    // NOT route through `getBlendColor` (the PartColor compositing formula).
    // Each tap is simply multiplied by gl_Color, then the 9 are averaged.
    // We mirror that — `ss_partcolor_blend` is intentionally not called.
    float fFocus = ss_param0;
    vec2 px = TEXTURE_PIXEL_SIZE;
    vec2 d = vec2(fFocus * px.x * 16.0, fFocus * px.y * 16.0);
    vec2 tc = UV;

    vec4 sum = ss_input_texture(TEXTURE, tc);
    sum += ss_input_texture(TEXTURE, vec2(tc.x + d.x, tc.y + d.y));
    sum += ss_input_texture(TEXTURE, vec2(tc.x + d.x, tc.y));
    sum += ss_input_texture(TEXTURE, vec2(tc.x,       tc.y + d.y));
    sum += ss_input_texture(TEXTURE, vec2(tc.x - d.x, tc.y - d.y));
    sum += ss_input_texture(TEXTURE, vec2(tc.x + d.x, tc.y - d.y));
    sum += ss_input_texture(TEXTURE, vec2(tc.x - d.x, tc.y + d.y));
    sum += ss_input_texture(TEXTURE, vec2(tc.x - d.x, tc.y));
    sum += ss_input_texture(TEXTURE, vec2(tc.x,       tc.y - d.y));
    vec4 Pixel = sum / 9.0;

    COLOR = ss_output_color(Pixel * partcolor_color, partcolor_varg.w);
}
)GLSL"
