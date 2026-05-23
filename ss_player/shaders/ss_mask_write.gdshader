R"GLSL(
// CBP mask coverage write shader.
//
// Used only for rendering mask-writer geometry into the player's private
// coverage bitmap (an offscreen RGBA8 viewport). Each writer is assigned one
// of 32 bits; `mask_bit_color` carries that bit pre-encoded into the correct
// RGBA8 channel (e.g. R for bits 0..7, value (1<<(bit%8))/255). Additive
// blending then ORs the disjoint bits together across writers.
//
// Cutout: a texel contributes to the mask only where the writer sprite's alpha
// is strictly greater than `mask_threshold`. The player derives the threshold
// from the part's `mask` attribute (PartState.mask()); mask == 0 maps to
// threshold 1.0 so nothing is written (empty mask). Shape masks with no bound
// texture sample the default white texture (alpha 1.0) and so write their full
// geometry whenever the threshold is below 1.0.
shader_type canvas_item;
render_mode blend_add, unshaded;

uniform vec4 mask_bit_color;
uniform float mask_threshold;

void fragment() {
	// Cutout: only texels above the mask threshold contribute to the mask.
	if (texture(TEXTURE, UV).a <= mask_threshold) {
		discard;
	}
	// Godot's canvas `blend_add` is premultiplied (src.rgb * src.a), so the bit
	// must be written with alpha 1 or it is dropped. Bits live in R/G/B only
	// (24 writers); the alpha channel is the "any coverage" accumulator.
	COLOR = vec4(mask_bit_color.rgb, 1.0);
}
)GLSL"
