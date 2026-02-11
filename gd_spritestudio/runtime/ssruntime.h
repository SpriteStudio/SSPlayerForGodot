#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <new>

extern "C" {

void *ss_runtime_create();

void ss_runtime_destroy(void *context);

bool ss_runtime_load_ssab_copy(void *context, const unsigned char *ptr, uintptr_t len);

bool ss_runtime_load_ssab_borrow(void *context, const unsigned char *ptr, uintptr_t len);

void ss_runtime_get_ssab(void *context, unsigned char **out_data, uintptr_t *out_len);

const unsigned char *ss_runtime_get_ssab_buf(void *context);

uintptr_t ss_runtime_get_ssab_len(void *context);

bool ss_runtime_setup_animation(void *context, const char *name);

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
