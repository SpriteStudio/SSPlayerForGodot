R"GLSL(
void fragment() {
    // ss-wave: ported from SS6 SDK (Common/Drawer/GLSL/ss-wave.fs).
    // Sinusoidal horizontal warp driven by texture-V coordinate.
    //   ss_param0 = fWidth   (frequency along V)
    //   ss_param1 = fHeight  (amplitude in pixel-width units; *0.5 of 384)
    //   ss_param2 = fPhase   (phase shift; 0..1 == one full 2*PI period)
    // SS6 used args[A_U1]/args[A_V1] for the per-pixel UV step; we read
    // TEXTURE_PIXEL_SIZE.x directly.
    float fWidth  = ss_param0;
    float fHeight = ss_param1;
    float fPhase  = ss_param2;

    vec2 Coord = UV;
    vec2 px = TEXTURE_PIXEL_SIZE;
    // PI is a Godot canvas_item built-in constant — no local redefinition.

    float l = 384.0 * fHeight * 0.5;
    float s = l * sin(Coord.y * 384.0 * fWidth * 0.5 + PI * 2.0 * fPhase) * px.x;

    vec4 Pixel = ss_input_texture(TEXTURE, vec2(Coord.x + s, Coord.y));

    vec3 blended = ss_partcolor_blend(Pixel.rgb, partcolor_color.rgb, partcolor_varg);
    float a = Pixel.a * partcolor_color.a;
    COLOR = ss_output_color(vec4(blended, a), partcolor_varg.w);
}
)GLSL"
