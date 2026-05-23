#pragma once


// =========================================================================
// Shader pipeline composition
// =========================================================================
//
// The runtime shader for SpriteStudio canvas_item draws is composed at
// material-creation time by concatenating GLSL text pieces. Each piece lives
// in its own file under `shaders/` and is included here as a single C++ raw
// string literal (`R"GLSL(...)GLSL"` — the only C++ artifact in those files).
//
//   shaders/ss_library_vs.glsl  — vs-stage SpriteStudio library: shared constants,
//                                  varyings, and any helper functions used from vertex()
//   shaders/ss_library_fs.glsl  — fs-stage SpriteStudio library functions
//   shaders/default.vs          — default vertex stage (vertex() entry)
//   shaders/default.fs          — default fragment stage (fragment() entry)
//
// Library files are stage-organized, not feature-organized. Functions inside
// them follow the naming convention `ss_<CATEGORY>_<FEATURE>` (e.g.
// `ss_partcolor_blend`, `ss_output_color`) so the category can be recovered
// from the function name when a single file hosts many helpers.
//
// Concatenation order (top-down GLSL compilation):
//   SHADER_HEADER → render_mode → LIBRARY_VS → LIBRARY_FS → DEFAULT_VS → DEFAULT_FS
// Library functions come before entry points so vertex()/fragment() can call
// them without forward declarations.
//
// `_PartColorCoef` table (ss_library_vs.glsl) mirrors the SS6 SDK reference
// (Common/Drawer/ssplayer_render_gl.cpp:1203-1206):
//   Mix (0): fSrc=1-r, fDst=+r, fDstSrc=0   -> lerp(pixel, color, r)
//   Mul (1): fSrc=1-r, fDst=+r, fDstSrc=1   -> pixel * lerp(1, color, r)
//   Add (2): fSrc=1,   fDst=+r, fDstSrc=0   -> pixel + color * r
//   Sub (3): fSrc=1,   fDst=-r, fDstSrc=0   -> pixel - color * r
//
// `partcolor_color` is captured into a dedicated varying because Godot's
// canvas_item fragment stage modulates the built-in COLOR with the sampled
// texture before user fragment() runs; reading COLOR in fragment would lose
// the original PartColor.rgb.
//
// `ss_partcolor_blend()` applies the SS6 SDK PartColor compositing formula
// (Common/Drawer/GLSL/default.fs:27). `ss_input_texture()` and
// `ss_output_color()` are I/O extension points: the former wraps the input
// sampler call (so future variants can fold inverse-PMA or other input-side
// conversions there), and the latter wraps the output stage (currently
// optional premultiplied-alpha conversion; future linear/HDR conversions
// will land here as well).
const char* SHADER_HEADER = "shader_type canvas_item;\n";

const char* LIBRARY_VS =
#include "shaders/ss_library_vs.glsl"
;

const char* LIBRARY_FS =
#include "shaders/ss_library_fs.glsl"
;

const char* DEFAULT_VS =
#include "shaders/default.vs"
;

const char* DEFAULT_FS =
#include "shaders/default.fs"
;

// Stub fragment shader for per-part material dispatch verification. Inverts
// the red channel of the sampled texture so the dispatch path is visually
// distinguishable from Default. Kept around as a known-good per-part path
// regression check while more SS6-ported variants land.
const char* TESTSTUB_FS =
#include "shaders/teststub.fs"
;

// SS6 SDK port: "ss-sepia". Sepia / grayscale tone with a single signed
// strength parameter (ss_param0). See shaders/ss_sepia.fs for the
// migration mapping from the SS6 reference.
const char* SS_SEPIA_FS =
#include "shaders/ss_sepia.fs"
;

// SS6 SDK port: "ss-outline". 4-neighbour alpha-edge outline with a
// signed threshold parameter (ss_param0). Uses TEXTURE_PIXEL_SIZE in
// place of SS6's args[A_U1]/args[A_V1].
const char* SS_OUTLINE_FS =
#include "shaders/ss_outline.fs"
;

// SS6 SDK port: "ss-bmask". Brightness-threshold mask discarding pixels
// above/below `ss_param0`'s magnitude depending on its sign.
const char* SS_BMASK_FS =
#include "shaders/ss_bmask.fs"
;

// CBP mask coverage write shader. Unlike the SS_*_FS fragment bodies above,
// this is a complete canvas_item shader (its own shader_type / render_mode):
// it is set verbatim on the coverage-pass material, not combined with the
// vertex / library stages.
const char* SS_MASK_WRITE_SHADER =
#include "shaders/ss_mask_write.gdshader"
;

// SS6 SDK port: "ss-hsb". Hue/Saturation/Brightness shift driven by
// ss_param0..2. Defines ss_rgb_to_hsb / ss_hsb_to_rgb helpers inline.
const char* SS_HSB_FS =
#include "shaders/ss_hsb.fs"
;

// SS6 SDK port: "ss-step". Brightness-driven posterise. ss_param0 sets
// the threshold (signed), ss_param1 the number of stages, ss_param2 the
// mix between monochrome and texture-tinted output.
const char* SS_STEP_FS =
#include "shaders/ss_step.fs"
;

// SS6 SDK port: "ss-move". Directional smear/motion-blur driven by
// ss_param0..2 (distance / direction / power). Iterates up to ~128
// back-traced samples along sin/cos(direction).
const char* SS_MOVE_FS =
#include "shaders/ss_move.fs"
;

// SS6 SDK port: "ss-wave". Sine-warp horizontal displacement driven by
// ss_param0..2 (width / height / phase).
const char* SS_WAVE_FS =
#include "shaders/ss_wave.fs"
;

// SS6 SDK port: "ss-noise". Per-texel pseudo-random noise overlay driven
// by ss_param0..2 (power / color / phase).
const char* SS_NOISE_FS =
#include "shaders/ss_noise.fs"
;

// SS6 SDK port: "ss-blur". 9-tap box blur scaled by ss_param0 (focus
// shift). Unique among the ss-* family in skipping the PartColor formula
// — see ss_blur.fs for the rationale.
const char* SS_BLUR_FS =
#include "shaders/ss_blur.fs"
;

// SS6 SDK port: "ss-pix". UV-quantise pixelation; ss_param0 scales the
// block size in source texels.
const char* SS_PIX_FS =
#include "shaders/ss_pix.fs"
;

// SS6 SDK port: "ss-scatter". Per-cell directional UV scatter driven by
// ss_param0..2 (power / ratio / phase).
const char* SS_SCATTER_FS =
#include "shaders/ss_scatter.fs"
;

// SS6 SDK port: "ss-circle". Cell rect → polar unwrap. Needs `ss_cell_rect`
// to know the source rectangle; degenerates to a discard for parts that
// don't bind one (e.g. Shape).
const char* SS_CIRCLE_FS =
#include "shaders/ss_circle.fs"
;

// SS6 SDK port: "ss-spot". Radial spotlight gradient centred at the cell
// rect's centre. Also needs `ss_cell_rect`.
const char* SS_SPOT_FS =
#include "shaders/ss_spot.fs"
;

// One render_mode line per GPU framebuffer blend variant. The four entries
// correspond to SsBlendType::{Mix, Mul, Add, Sub} by enum value (0/1/2/3);
// any other blend value falls back to Mix.
inline const char* partcolor_render_mode_str(int blend_idx_for_render_mode) {
    switch (blend_idx_for_render_mode) {
        case 0: return "render_mode blend_mix;\n";  // Mix
        case 1: return "render_mode blend_mul;\n";  // Mul
        case 2: return "render_mode blend_add;\n";  // Add
        case 3: return "render_mode blend_sub;\n";  // Sub
        default: return "render_mode blend_mix;\n";
    }
}

struct EmbeddedShader {
    const char* id_name;
    const char* fs_source;
    bool is_per_part;
};

static const EmbeddedShader EMBEDDED_SHADERS[] = {
    { "Default",  DEFAULT_FS,  false },
    { "TestStub", TESTSTUB_FS, true  },
    { "ss-sepia",   SS_SEPIA_FS,   true  },
    { "ss-outline", SS_OUTLINE_FS, true  },
    { "ss-bmask",   SS_BMASK_FS,   true  },
    { "ss-hsb",     SS_HSB_FS,     true  },
    { "ss-step",    SS_STEP_FS,    true  },
    { "ss-move",    SS_MOVE_FS,    true  },
    { "ss-wave",    SS_WAVE_FS,    true  },
    { "ss-noise",   SS_NOISE_FS,   true  },
    { "ss-blur",    SS_BLUR_FS,    true  },
    { "ss-pix",     SS_PIX_FS,     true  },
    { "ss-scatter", SS_SCATTER_FS, true  },
    { "ss-circle",  SS_CIRCLE_FS,  true  },
    { "ss-spot",    SS_SPOT_FS,    true  },
    // Add new shader variants below. Each new entry needs:
    //   1. A new `shaders/<name>.fs` with R"GLSL(...)GLSL"-wrapped pure GLSL
    //   2. A `const char* <NAME>_FS = #include "shaders/<name>.fs" ;` above
    //   3. The entry here with the SS-side id string and is_per_part=true
};
