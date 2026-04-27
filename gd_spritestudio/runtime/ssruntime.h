#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

constexpr static const int32_t SEED_MAGIC = 7573;

constexpr static const int32_t LIFE_EXTEND_SCALE = 8;

constexpr static const int32_t LIFE_EXTEND_MIN = 64;

using SsLogCallback = void(*)(int level, const char *message);

extern "C" {

void rust_eh_personality();

void ss_runtime_set_log_callback(SsLogCallback callback);

/// Creates an AnimationResource by copying the provided data.
///
/// # Security and Validation
/// To maximize performance and minimize binary size, `libssruntime` **does not validate** the input `.ssab` data.
/// Passing corrupted or maliciously crafted data will cause undefined behavior (e.g., out-of-bounds reads).
/// The caller MUST ensure the data is trusted or validated before passing it to this function.
///
/// # Safety
///
/// This function is unsafe because it dereferences a raw pointer.
/// The caller must ensure the pointer is valid and points to a valid memory block.
void *ss_resource_create_copy(const unsigned char *ptr,
                              uintptr_t len);

/// Creates an AnimationResource by borrowing the provided data.
///
/// # Security and Validation
/// To maximize performance and minimize binary size, `libssruntime` **does not validate** the input `.ssab` data.
/// Passing corrupted or maliciously crafted data will cause undefined behavior (e.g., out-of-bounds reads).
/// The caller MUST ensure the data is trusted or validated before passing it to this function.
///
/// # Safety
///
/// This function is unsafe because it dereferences a raw pointer.
/// The caller must ensure the pointer is valid and points to a valid memory block of at least `len` bytes,
/// AND the memory must remain valid and unmodified until this resource is destroyed.
void *ss_resource_create_borrow(const unsigned char *ptr,
                                uintptr_t len);

void ss_resource_destroy(void *resource);

void *ss_runtime_create();

void ss_runtime_destroy(void *context);

void ss_runtime_reset(void *context);

/// Binds an AnimationResource to a Context.
///
/// # Safety and Lifetimes
/// - To save memory, a single Resource can be bound to multiple Contexts.
/// - **Lifetime Safety:** This implementation uses `Arc` for internal reference counting.
///   It is safe to destroy the `resource` (via `ss_resource_destroy`) even if it is still bound
///   to contexts; the memory will be kept alive until all bound contexts are also destroyed or reset.
bool ss_runtime_bind_resource(void *context,
                              void *resource);

/// # Safety
///
/// This function is unsafe because it dereferences a raw pointer.
/// The caller must ensure the pointer is valid and points to a valid memory block.
bool ss_runtime_load_ssab_copy(void *context, const unsigned char *ptr, uintptr_t len);

/// # Safety
///
/// This function is unsafe because it dereferences a raw pointer.
/// The caller must ensure the pointer is valid and points to a valid memory block.
bool ss_runtime_load_ssab_borrow(void *context, const unsigned char *ptr, uintptr_t len);

/// # Safety
///
/// This function is unsafe because it dereferences raw pointers for output.
/// The caller must ensure `out_data` and `out_len` are valid if they are not null.
void ss_runtime_get_ssab(void *context, unsigned char **out_data, uintptr_t *out_len);

const unsigned char *ss_runtime_get_ssab_buf(void *context);

uintptr_t ss_runtime_get_ssab_len(void *context);

/// # Safety
///
/// This function is unsafe because it dereferences a raw pointer.
/// The caller must ensure the pointer is valid and points to a valid null-terminated string.
bool ss_runtime_setup_animation(void *context, const char *name);

/// # Safety
///
/// This function is unsafe because it dereferences raw pointers for output.
/// The caller must ensure `out_data` and `out_len` are valid if they are not null.
void ss_runtime_get_frame_data(void *context,
                               int frame_no,
                               unsigned char **out_data,
                               uintptr_t *out_len);

/// # Safety
///
/// This function is unsafe because it dereferences raw pointers for output.
/// The caller must ensure `out_data` and `out_len` are valid if they are not null.
void ss_runtime_get_world_matrices(void *context, const float **out_data, uintptr_t *out_len);

/// # Safety
///
/// This function is unsafe because it dereferences raw pointers for output.
/// The caller must ensure `out_data` and `out_len` are valid if they are not null.
void ss_runtime_get_z_order(void *context, const int32_t **out_data, uintptr_t *out_len);

int ss_runtime_get_frame_no(void *context);

float ss_runtime_get_frame_no_decimal(void *context);

bool ss_runtime_is_end_frame_reached(void *context);

bool ss_runtime_is_playing(void *context);

bool ss_runtime_is_pausing(void *context);

bool ss_runtime_get_skip_frames(void *context);

int32_t ss_runtime_get_playback_direction(void *context);

int32_t ss_runtime_get_playback_style(void *context);

int32_t ss_runtime_get_loops(void *context);

int32_t ss_runtime_get_remain_loops(void *context);

int32_t ss_runtime_get_start_frame(void *context);

int32_t ss_runtime_get_end_frame(void *context);

int32_t ss_runtime_get_fps(void *context);

void ss_runtime_set_skip_frames(void *context, bool skip);

void ss_runtime_set_loop(void *context, int loops);

void ss_runtime_set_frame_rate(void *context, int fps);

void ss_runtime_set_playback_direction(void *context, int direction, int style);

void ss_runtime_set_animation_speed(void *context, float speed_rate);

void ss_runtime_set_animation_section(void *context, int start_frame, int end_frame);

int32_t ss_runtime_update(void *context, float delta);

int32_t ss_runtime_update_animation(void *context, float delta);

void ss_runtime_play(void *context);

void ss_runtime_play_with_start_frame(void *context, int32_t frame_no);

void ss_runtime_pause(void *context);

void ss_runtime_stop(void *context);

void ss_runtime_set_frame_no(void *context, int frame);

void ss_runtime_next_frame(void *context);

void ss_runtime_prev_frame(void *context);

void *ss_effect_create(const void *resource, uint32_t effect_name_hash, uint32_t seed);

void ss_effect_update(void *effect_ctx, float frame);

void ss_effect_reset(void *effect_ctx);

int32_t ss_effect_get_lifetime(void *effect_ctx);

bool ss_effect_is_infinite(void *effect_ctx);

void ss_effect_get_state(void *effect_ctx, unsigned char **out_data, uintptr_t *out_len);

void ss_effect_destroy(void *effect_ctx);

/// Calculate the target frame for a child instance animation.
float ss_util_calculate_instance_frame(float parent_frame,
                                       int32_t key_frame,
                                       int32_t start_offset,
                                       float speed,
                                       bool independent,
                                       float accumulated_time);

void ss_runtime_set_world_matrix_calculation_enabled(void *context, bool enabled);

bool ss_runtime_is_world_matrix_calculation_enabled(void *context);

void ss_context_set_coordinate_system(void *context, int32_t coordinate_system);

int32_t ss_context_get_coordinate_system(void *context);

void ss_context_set_origin(void *context, int32_t origin);

int32_t ss_context_get_origin(void *context);

void ss_context_set_unit(void *context, int32_t unit);

int32_t ss_context_get_unit(void *context);

/// Transform multiple particle positions by a matrix.
/// matrix_ptr: pointer to float array of size 16.
/// particles_ptr: pointer to ParticleState array from ss_effect_get_state.
/// count: number of particles.
/// out_pos_ptr: pointer to float array (x, y, x, y, ...) to store transformed 2D coordinates.
void ss_effect_transform_particles(const float *matrix_ptr,
                                   const void *particles_ptr,
                                   int32_t count,
                                   float *out_pos_ptr);

int32_t ss_runtime_get_passed_event_count(void *context);

int32_t ss_runtime_get_passed_event_frame_no(void *context, int32_t index);

int32_t ss_runtime_get_passed_event_index(void *context, int32_t index);

/// Computes the local vertex coordinates for a part.
///
/// # Safety
/// - `input_data`: Must point to an array of at least 6 floats if `has_deform` (flag 4) is false,
///   or 14 floats if `has_deform` is true.
///   Layout: [size_x, size_y, pivot_x, pivot_y, pivot_offset_x, pivot_offset_y, (optional) dx0..3, dy0..3]
/// - `out_x`, `out_y`: Must point to buffers with at least 5 floats.
/// Computes the local vertices for a part using primitive parameters.
/// deform_x_ptr, deform_y_ptr: optional pointers to float arrays of size 4.
/// out_x, out_y: pointers to float arrays of size 5 to store results.
/// # Safety
/// This function is unsafe because it dereferences raw pointers.
int32_t ss_vertex_compute_local(float size_w,
                                float size_h,
                                float pivot_x,
                                float pivot_y,
                                float pivot_offset_x,
                                float pivot_offset_y,
                                bool h_flip,
                                bool v_flip,
                                const float *deform_x_ptr,
                                const float *deform_y_ptr,
                                float *out_x,
                                float *out_y);

/// Computes the local UV coordinates for a part.
///
/// # Safety
/// - `input_data`: Must point to an array of at least 9 floats.
///   Layout: [left, top, right, bottom, trans_u, trans_v, rot_deg, scale_u, scale_v]
/// - `out_u`, `out_v`: Must point to buffers with at least 5 floats.
/// Computes the local UV coordinates for a part using primitive parameters.
/// out_u, out_v: pointers to float arrays of size 5 to store results.
/// # Safety
/// This function is unsafe because it dereferences raw pointers.
int32_t ss_uv_compute_local(float u_left,
                            float v_top,
                            float u_right,
                            float v_bottom,
                            float trans_u,
                            float trans_v,
                            float rot_deg,
                            float scale_u,
                            float scale_v,
                            bool part_flip_h,
                            bool part_flip_v,
                            bool img_flip_h,
                            bool img_flip_v,
                            bool rotated,
                            float *out_u,
                            float *out_v);

bool ss_geometry_is_point_in_rect(float px, float py, float size_w, float size_h);

bool ss_geometry_is_point_in_quad(float tx, float ty, const float *vx, const float *vy);

}  // extern "C"
