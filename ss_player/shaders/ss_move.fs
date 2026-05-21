R"GLSL(
void fragment() {
    // ss-move: ported from SS6 SDK (Common/Drawer/GLSL/ss-move.fs).
    // Directional motion-blur-ish smear that accumulates back-traced
    // texture samples along (sin(dir*PI), cos(dir*PI)).
    //   ss_param0 = fDistance  (signed; sign reverses the trace direction)
    //   ss_param1 = fDirection (rotation in PI units)
    //   ss_param2 = fPower     (blend strength per step, 0..1)
    // SS6 used args[A_U1]/args[A_V1] for the per-pixel UV step; we read
    // TEXTURE_PIXEL_SIZE directly.
    float fDistance  = ss_param0;
    float fDirection = ss_param1;
    float fPower     = ss_param2;

    vec2 Coord = UV;
    vec2 px = TEXTURE_PIXEL_SIZE;
    // PI is a Godot canvas_item built-in constant — no local redefinition.

    int iCount = int(floor(abs(fDistance) * 96.0));
    // Safety cap so the dynamic loop bound stays predictable on tighter
    // shader backends. SS6's implicit ceiling is the same when fDistance
    // is in the usual 0..1 authoring range.
    if (iCount > 128) iCount = 128;

    vec2 Vel = vec2(sin(fDirection * PI) * px.x,
                    cos(fDirection * PI) * px.y) * sign(fDistance);

    Coord += Vel * float(iCount);

    Coord -= Vel;
    vec4 Pixel = ss_input_texture(TEXTURE, Coord);
    for (int i = 1; i < iCount; i++) {
        Coord -= Vel;
        Pixel = mix(ss_input_texture(TEXTURE, Coord), Pixel, 0.96 * abs(fPower));
    }

    vec3 blended = ss_partcolor_blend(Pixel.rgb, partcolor_color.rgb, partcolor_varg);
    float a = Pixel.a * partcolor_color.a;
    COLOR = ss_output_color(vec4(blended, a), partcolor_varg.w);
}
)GLSL"
