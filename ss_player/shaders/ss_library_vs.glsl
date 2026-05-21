R"GLSL(
const vec4 _PartColorCoef[4] = vec4[4](
    vec4(-1.0,  1.0, 0.0, 0.0),
    vec4(-1.0,  1.0, 1.0, 0.0),
    vec4( 0.0,  1.0, 0.0, 0.0),
    vec4( 0.0, -1.0, 0.0, 0.0)
);

varying vec4 partcolor_varg;
varying vec4 partcolor_color;
)GLSL"
