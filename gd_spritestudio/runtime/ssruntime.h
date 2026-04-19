#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

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

void ss_runtime_play(void *context);

void ss_runtime_play_with_start_frame(void *context, int32_t frame_no);

void ss_runtime_pause(void *context);

void ss_runtime_stop(void *context);

void ss_runtime_set_frame_no(void *context, int frame);

void ss_runtime_next_frame(void *context);

void ss_runtime_prev_frame(void *context);

}  // extern "C"
