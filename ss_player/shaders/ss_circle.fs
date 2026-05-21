R"GLSL(
void fragment() {
    // ss-circle: ported from SS6 SDK (Common/Drawer/GLSL/ss-circle.fs).
    // Unwraps the cell rectangle into a disc: radial coord becomes uu
    // (0 at the rim, 1 at the centre) and angular coord becomes vv
    // (0..1 around the circle, offset by fPhase, direction-flipped by
    // fDirection).
    //   ss_param0 = fPhase
    //   ss_param1 = fDirection (>0 = CCW from SS6's perspective; <=0 flips)
    // Cell-rect UV bounds arrive via ss_cell_rect; A_U1/A_V1 become
    // TEXTURE_PIXEL_SIZE.
    float fPhase = ss_param0;
    float fDirection = ss_param1;

    float e = 1.0e-10;
    vec2 px = TEXTURE_PIXEL_SIZE;
    vec2 uv1 = vec2(px.x + e, px.y + e);

    vec2 lt_uv = ss_cell_rect.xy;
    vec2 rb_uv = ss_cell_rect.zw;
    vec2 center_uv = (lt_uv + rb_uv) * 0.5;

    // Lift everything to pixel space so the circle is geometrically round
    // even when the cell aspect ratio is non-square (uv1 has px.x != px.y).
    vec2 tx = UV / uv1;
    vec2 c  = center_uv / uv1;
    vec2 lt = lt_uv / uv1;
    vec2 rb = rb_uv / uv1;
    vec2 d  = rb - lt;
    vec2 v  = tx - c;
    float r = min(abs(rb.x - c.x), abs(rb.y - c.y)) + e;
    float dis = length(v);

    if (dis > r) discard;

    // Avoid NaN at the exact centre; the angular coord is undefined there
    // anyway, so any tangent direction works.
    vec2 nv = (dis > e) ? (v / dis) : vec2(1.0, 0.0);
    float uu = 1.0 - dis / r;
    float vv = (atan(nv.y, nv.x) / PI + 1.0) * 0.5 + fPhase;

    if (vv < 0.0) vv += 1.0;
    if (vv > 1.0) vv -= 1.0;
    if (fDirection <= 0.0) vv = 1.0 - vv;

    vec2 st = d * vec2(uu, vv) * uv1;
    vec4 Pixel = ss_input_texture(TEXTURE, lt_uv + st);

    vec3 blended = ss_partcolor_blend(Pixel.rgb, partcolor_color.rgb, partcolor_varg);
    float a = Pixel.a * partcolor_color.a;
    COLOR = ss_output_color(vec4(blended, a), partcolor_varg.w);
}
)GLSL"
