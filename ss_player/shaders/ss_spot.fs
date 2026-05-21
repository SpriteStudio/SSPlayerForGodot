R"GLSL(
void fragment() {
    // ss-spot: ported from SS6 SDK (Common/Drawer/GLSL/ss-spot.fs).
    // Radial spotlight gradient centred at the cell rect's centre. fPower
    // scales the radius; fDecay blends between two falloff curves
    // (1/r-style fR vs linear fL); fColor mixes between white and the
    // sampled texture for the lit colour.
    //   ss_param0 = fPower
    //   ss_param1 = fDecay (0..1; 0 = sharp 1/r, 1 = linear)
    //   ss_param2 = fColor (0 = white spot, 1 = tex-coloured spot)
    float fPower = ss_param0;
    float fDecay = ss_param1;
    float fColor = ss_param2;

    float e = 1.0e-10;
    float p = abs(fPower);
    float d_abs = abs(fDecay);
    vec2 px = TEXTURE_PIXEL_SIZE;
    vec2 uv1 = vec2(px.x + e, px.y + e);

    vec2 lt_uv = ss_cell_rect.xy;
    vec2 rb_uv = ss_cell_rect.zw;
    vec2 center_uv = (lt_uv + rb_uv) * 0.5;

    vec2 c = center_uv / uv1;
    vec2 rb = rb_uv / uv1;
    float l = min(abs(rb.x - c.x), abs(rb.y - c.y)) + e;
    vec2 t = UV / uv1;
    vec2 v = c - t;

    float fR = max(p / (length(v / l) + e) - p, 0.0);
    float fL = max(p - length(v / l), 0.0);
    fR = clamp(mix(fR, fL, d_abs), 0.0, 1.0);

    vec3 sampled = ss_input_texture(TEXTURE, UV).rgb;
    vec4 Pixel = vec4(mix(vec3(1.0), sampled, fColor), fR);

    vec3 blended = ss_partcolor_blend(Pixel.rgb, partcolor_color.rgb, partcolor_varg);
    float a = Pixel.a * partcolor_color.a;
    COLOR = ss_output_color(vec4(blended, a), partcolor_varg.w);
}
)GLSL"
