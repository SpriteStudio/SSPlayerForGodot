## Play, pause, resume, stop — and what each one leaves behind.
##
## Everything here steps with `advance()` under `ANIMATION_PROCESS_MANUAL`, so
## the assertions are about the transport rather than about how long a frame
## took on this machine. That is also the only way these can mean the same thing
## on three platforms.
extends "res://test_base.gd"

const BASIC := "res://ssab_generated/overall/Basic.ssab"

var player: Node
var dt := 1.0 / 30.0


func setup() -> void:
	player = make_player(BASIC)
	player.set_animation("anime_1")
	dt = 1.0 / maxf(1.0, float(player.get_frame_rate()))


func test_a_player_starts_stopped() -> void:
	not_ok(player.is_playing(), "not playing before play()")
	not_ok(player.is_pausing(), "not pausing before play()")
	near(player.get_frame_no(), 0.0, "the head starts at the first frame")


func test_play_starts_it_and_advance_moves_the_head() -> void:
	player.play()
	ok(player.is_playing(), "playing after play()")
	near(player.get_frame_no(), 0.0, "play() alone does not advance")
	for i in 5:
		player.advance(dt)
	near(player.get_frame_no(), 5.0, "five steps of one frame each", 0.001)


## The property that makes every other case here reproducible.
func test_manual_mode_advances_only_when_asked() -> void:
	eq(player.get_animation_process_mode(), 2, "MANUAL")
	player.play()
	var before: float = player.get_frame_no()
	# No advance() between these two reads: nothing else may move the head.
	near(player.get_frame_no(), before, "the head does not move on its own")


## `pause()` is not `stop()`: the head stays where it is and the player still
## reports itself playing, which is what lets a host resume without re-deciding
## what was playing.
func test_pause_holds_the_head_and_resume_gives_it_back() -> void:
	player.play()
	for i in 3:
		player.advance(dt)
	var held: float = player.get_frame_no()

	player.pause()
	ok(player.is_pausing(), "pausing after pause()")
	ok(player.is_playing(), "still reports playing while paused")
	player.advance(dt)
	near(player.get_frame_no(), held, "a paused player does not advance")

	player.resume()
	not_ok(player.is_pausing(), "not pausing after resume()")
	player.advance(dt)
	near(player.get_frame_no(), held + 1.0, "the head moves again after resume", 0.001)


## `stop()` leaves the head where it was rather than rewinding, so a host that
## wants the first frame back asks for it.
func test_stop_ends_playback_without_rewinding() -> void:
	player.play()
	for i in 4:
		player.advance(dt)
	var reached: float = player.get_frame_no()
	player.stop()
	not_ok(player.is_playing(), "not playing after stop()")
	near(player.get_frame_no(), reached, "stop() does not rewind")


func test_the_head_can_be_placed_by_hand() -> void:
	player.set_frame_no(7.0)
	near(player.get_frame_no(), 7.0, "set_frame_no")


func test_speed_scales_the_step() -> void:
	eq(player.get_speed_scale(), 1.0, "the default speed")
	player.set_speed_scale(2.0)
	eq(player.get_speed_scale(), 2.0, "the speed that took effect")
	player.play()
	player.advance(dt)
	near(player.get_frame_no(), 2.0, "one step at double speed", 0.001)


func test_the_head_runs_forward_by_default() -> void:
	player.play()
	player.advance(dt)
	ok(player.is_playing_forward(), "forward by default")
	eq(player.get_playback_direction(), 0, "the configured direction")


## Reverse does not mean "the same run, mirrored": `play()` puts the head on the
## LAST frame, because that is where a backwards pass begins. A seek before
## `play()` is therefore not a way to choose where reverse starts — it is
## overwritten — and a host that wants to start part-way seeks afterwards.
func test_reverse_starts_at_the_end_and_walks_back() -> void:
	player.set_playback_direction(1, 0)
	eq(player.get_playback_direction(), 1, "the configured direction")
	player.play()
	near(player.get_frame_no(), float(player.get_end_frame()),
		"reverse play() starts at the last frame")
	not_ok(player.is_playing_forward(), "the head is heading backwards")

	var from: float = player.get_frame_no()
	player.advance(dt)
	near(player.get_frame_no(), from - 1.0, "one step backwards", 0.001)


func test_a_seek_after_play_is_where_reverse_carries_on_from() -> void:
	player.set_playback_direction(1, 0)
	player.play()
	player.set_frame_no(10.0)
	near(player.get_frame_no(), 10.0, "the seek took")
	player.advance(dt)
	near(player.get_frame_no(), 9.0, "and the next step goes backwards from there", 0.001)


func test_flip_is_reported_back() -> void:
	not_ok(player.is_flipped_h(), "not flipped to start with")
	not_ok(player.is_flipped_v(), "not flipped to start with")
	player.set_flip_h(true)
	player.set_flip_v(true)
	ok(player.is_flipped_h(), "flip_h")
	ok(player.is_flipped_v(), "flip_v")
