R"GLSL(
void fragment() {
    // ss-outline: ported from SS6 SDK (Common/Drawer/GLSL/ss-outline.fs).
    // ss_param0 = fThreshold (signed); |fThreshold| is the alpha cutoff
    // used to test the 4-neighbour cross. A pixel survives when its own
    // alpha is above the cutoff AND at least one orthogonal neighbour is
    // below it — that is, when the pixel sits on the alpha edge.
    //
    // SS6 fed args[A_U1] / args[A_V1] (per-pixel UV step) which the
    // canvas_item shader gets for free via TEXTURE_PIXEL_SIZE.
    float fThreshold = ss_param0;
    float fRatio = abs(fThreshold);
    float e = 1.0e-5;

    vec2 px = TEXTURE_PIXEL_SIZE;

    float lo = 0.0;
    lo += step(ss_input_texture(TEXTURE, UV + vec2(-px.x, 0.0)).a, fRatio);
    lo += step(ss_input_texture(TEXTURE, UV + vec2(+px.x, 0.0)).a, fRatio);
    lo += step(ss_input_texture(TEXTURE, UV + vec2(0.0, -px.y)).a, fRatio);
    lo += step(ss_input_texture(TEXTURE, UV + vec2(0.0, +px.y)).a, fRatio);

    vec4 center = ss_input_texture(TEXTURE, UV);
    float hi = step(fRatio + e, center.a);

    float v = min(hi * lo, 1.0);
    if (v <= 0.0) discard;

    // SS6 calls getBlendColor(vec4(v)) — feed the outline intensity as both
    // rgb and alpha, then PartColor-composite normally.
    vec3 blended = ss_partcolor_blend(vec3(v), partcolor_color.rgb, partcolor_varg);
    float a = v * partcolor_color.a;
    COLOR = ss_output_color(vec4(blended, a), partcolor_varg.w);
}
)GLSL"
