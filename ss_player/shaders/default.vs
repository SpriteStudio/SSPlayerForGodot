R"GLSL(
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
    // CBP masking: coverage UV from the local vertex position via a 2x2 basis +
    // offset. Flat parts pass the player-local->UV diagonal; Instance children
    // fold in their slot matrix (which can rotate), so a per-axis scale is not
    // enough — the mapping needs a full 2x2.
    ss_mask_uv = vec2(dot(ss_mask_uv_basis.xy, VERTEX), dot(ss_mask_uv_basis.zw, VERTEX)) + ss_mask_uv_off;
}
)GLSL"
