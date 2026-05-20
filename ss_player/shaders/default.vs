// Default vertex shader for SpriteStudio canvas_item draws.
// Common to most fragment variants — sets up the PartColor varyings that
// the fragment stage consumes (rate-folded coefficients + the raw vertex
// color). Add new .fs variants alongside this file and reuse it as-is.
//
// File contents are a C++ raw string literal so the .vs file can be
// `#include`'d directly into a `const char*` definition. See
// ss_internal_player.cpp::_ensure_partcolor_shader() for the composition.
//
// Coefficients mirror the SS6 SDK reference
// (Common/Drawer/ssplayer_render_gl.cpp:1203-1206):
//   Mix (0): fSrc=1-r, fDst=+r, fDstSrc=0   -> lerp(pixel, color, r)
//   Mul (1): fSrc=1-r, fDst=+r, fDstSrc=1   -> pixel * lerp(1, color, r)
//   Add (2): fSrc=1,   fDst=+r, fDstSrc=0   -> pixel + color * r
//   Sub (3): fSrc=1,   fDst=-r, fDstSrc=0   -> pixel - color * r
//
// `partcolor_color` captures the input vertex color into a dedicated
// varying because Godot's canvas_item fragment stage modulates the
// built-in COLOR with the sampled texture before user fragment() runs;
// reading COLOR in fragment loses the original PartColor.rgb.

R"GLSL(
const vec4 _PartColorCoef[4] = vec4[4](
    vec4(-1.0,  1.0, 0.0, 0.0),
    vec4(-1.0,  1.0, 1.0, 0.0),
    vec4( 0.0,  1.0, 0.0, 0.0),
    vec4( 0.0, -1.0, 0.0, 0.0)
);

varying vec4 partcolor_varg;
varying vec4 partcolor_color;

void vertex() {
    int idx = clamp(int(CUSTOM0.y + 0.5), 0, 3);
    vec4 c = _PartColorCoef[idx];
    float rate = CUSTOM0.x;
    partcolor_varg = vec4(
        1.0 + c.x * rate,
        c.y * rate,
        c.z,
        CUSTOM0.z
    );
    partcolor_color = COLOR;
}
)GLSL"
