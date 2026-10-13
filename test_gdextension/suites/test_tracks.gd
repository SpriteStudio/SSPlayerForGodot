## Animation mixing: the track stack, its sources, and the one clock they share.
##
## What is asserted here is the node's seam onto the runtime — that every track
## and source call reaches it, that the node ticks the stack the way the runtime
## expects, and that events are read from the animation of the source that fired
## them. How tracks fold into a pose is the runtime's, and is tested there.
##
## Ringo carries several animations, which is what a stack needs; UserData.ssab
## carries one event key, which is what the event cases need. It has a single
## animation, so its overlays lay that same animation again.
extends "res://test_base.gd"

const RINGO := "res://ssab_generated/Ringo/Ringo.ssab"
const USER_DATA := "res://ssab_generated/overall/UserData.ssab"

var player: Node
var names: PackedStringArray
var dt := 1.0 / 30.0


func setup() -> void:
	player = make_player(RINGO)
	names = player.get_animation_names()
	player.set_animation(names[0])
	dt = 1.0 / maxf(1.0, float(player.get_frame_rate()))


func _step(n: int) -> void:
	for i in n:
		player.advance(dt)


func test_a_set_up_animation_is_track_0_alone() -> void:
	gt(names.size(), 1, "Ringo has more than one animation")
	eq(player.get_track_count(), 1, "one track")
	ok(player.get_track_main_source(0) >= 0, "track 0 has a main source")
	near(player.get_track_alpha(0), 1.0, "at full alpha")
	eq(player.get_track_blend(0), player.TRACK_BLEND_MIX, "mixing")
	eq(player.get_track_sync(0), player.TRACK_SYNC_FREE, "unsynced")
	near(player.get_track_fade_progress(0), -1.0, "and not fading")


func test_an_overlay_plays_on_the_character_clock() -> void:
	ok(player.set_track_animation(1, names[1]), "set_track_animation")
	eq(player.get_track_count(), 2, "a second track")
	player.play()
	_step(5)
	near(player.get_track_frame(1), 5.0, "the overlay moved with the clock", 0.001)


func test_a_crossfade_onto_an_empty_track_plays_at_once() -> void:
	player.play()
	ok(player.crossfade(1, names[1], 0.5), "crossfade")
	near(player.get_track_fade_progress(1), -1.0, "nothing to fade from")
	_step(3)
	near(player.get_track_frame(1), 3.0, "and it plays", 0.001)


## The incoming animation owns the track from the call on, is played while it
## fades in, and the character does not stop when the fade completes.
func test_a_crossfade_on_track_0_hands_it_over() -> void:
	player.play()
	_step(2)
	var outgoing: int = player.get_track_main_source(0)
	ok(player.crossfade(0, names[1], 4.0 * dt), "crossfade over four frames")
	var incoming: int = player.get_track_main_source(0)
	ne(incoming, outgoing, "track 0 carries the incoming source from the call on")
	_step(2)
	var progress: float = player.get_track_fade_progress(0)
	ok(progress > 0.0 and progress < 1.0, "half-way through the fade (%f)" % progress)
	_step(4)
	near(player.get_track_fade_progress(0), -1.0, "the fade has completed")
	eq(player.get_track_main_source(0), incoming, "and still carries it")
	ok(player.is_playing(), "and the character still plays")
	var frame: float = player.get_frame_no()
	_step(2)
	near(player.get_frame_no(), frame + 2.0, "the incoming animation advances", 0.001)


## A crossfade on track 0 makes the incoming animation the current one at the
## call, and says so once; a switch is not a start.
func test_a_crossfade_on_track_0_switches_the_current_animation_at_the_call() -> void:
	player.play()
	_step(2)
	var seen: Array = []
	player.animation_changed.connect(func(n): seen.append(["changed", n]))
	player.animation_started.connect(func(n): seen.append(["started", n]))
	ok(player.crossfade(0, names[1], 10.0 * dt), "crossfade")
	eq(player.get_current_animation(), names[1], "the incoming animation is current at once")
	eq(seen, [["changed", names[1]]], "animation_changed once, naming it, and no start")
	ok(player.crossfade(0, names[1], 10.0 * dt), "a crossfade to the current animation")
	eq(seen.size(), 1, "is no switch to report")


## The animation a crossfade left is no longer the current one, so selecting it
## again sets it up; selecting the incoming one during its fade is a no-op.
func test_set_animation_returns_to_the_animation_a_crossfade_left() -> void:
	player.play()
	_step(2)
	player.crossfade(0, names[1], 10.0 * dt)
	_step(2)
	player.set_animation(names[1])
	ok(player.get_track_fade_progress(0) > 0.0, "selecting the incoming one leaves the fade running")
	player.set_animation(names[0])
	eq(player.get_current_animation(), names[0], "back to the outgoing animation")
	near(player.get_track_fade_progress(0), -1.0, "set up afresh, with no fade")
	eq(player.get_track_count(), 1, "and track 0 alone")


## What is looked up by the current animation's name follows the switch: Ringo's
## `dead` is the one animation authored on a wider canvas.
func test_the_canvas_follows_a_crossfade_on_track_0() -> void:
	player.set_animation("wait1")
	var before: Vector2 = player.get_canvas_size()
	player.crossfade(0, "dead", 0.5)
	ne(player.get_canvas_size(), before, "the canvas is dead's at the call")
	eq(player.get_canvas_size(), player.get_ssab_resource().get_canvas_size("dead"), "and is dead's")


func test_set_track_animation_on_track_0_switches_too() -> void:
	var changed: Array = []
	player.animation_changed.connect(func(n): changed.append(n))
	ok(player.set_track_animation(1, names[1]), "an overlay")
	eq(player.get_current_animation(), names[0], "does not change the current animation")
	ok(player.set_track_animation(0, names[1]), "track 0")
	eq(player.get_current_animation(), names[1], "does")
	eq(changed, [names[1]], "and only that one is reported")


## The runtime lays every animation looping forever; the node puts its loop count
## back, so the incoming animation finishes on it and says so once.
func test_the_loop_count_carries_across_a_crossfade_on_track_0() -> void:
	var finished: Array = []
	player.animation_finished.connect(func(n): finished.append(n))
	player.set_animation("attack1")
	player.set_loop_count(1)
	player.play()
	_step(2)
	player.crossfade(0, "wait1", 4.0 * dt)
	eq(player.get_loop_count(), 1, "the loop count is the node's still")
	for i in 2000:
		if player.is_finished():
			break
		player.advance(dt)
	ok(player.is_finished(), "the incoming animation finished on it")
	eq(finished, ["wait1"], "animation_finished once, naming it")


## The outgoing animation running out mid-fade is not the character finishing:
## track 0 already reports the incoming one.
func test_an_outgoing_animation_running_out_does_not_finish_the_character() -> void:
	var finished: Array = []
	player.animation_finished.connect(func(n): finished.append(n))
	player.set_animation("attack1")
	player.set_loop_count(1)
	player.play()
	_step(15)
	player.crossfade(0, "wait1", 30.0 * dt)
	_step(20)
	ok(player.get_track_fade_progress(0) > 0.0, "still fading, past attack1's end")
	not_ok(player.is_finished(), "the character is not finished")
	ok(player.is_playing(), "and plays on")
	eq(finished, [], "no animation_finished")


func test_pause_holds_every_track_and_resume_releases_them() -> void:
	player.set_track_animation(1, names[1])
	player.play()
	_step(3)
	player.pause()
	var held: float = player.get_track_frame(1)
	_step(3)
	near(player.get_track_frame(1), held, "the overlay holds while paused")
	player.resume()
	_step(2)
	near(player.get_track_frame(1), held + 2.0, "and moves again after resume", 0.001)


## Only play() restarts a stopped character, and it heads out track 0 alone: an
## overlay keeps where it stands.
func test_stop_holds_the_stack_until_play() -> void:
	player.set_track_animation(1, names[1])
	player.play()
	_step(4)
	player.stop()
	var held: float = player.get_track_frame(1)
	_step(2)
	near(player.get_track_frame(1), held, "a stopped overlay holds")
	player.resume()
	_step(2)
	near(player.get_track_frame(1), held, "resume() does not restart a stop")
	player.play()
	near(player.get_frame_no(), 0.0, "play() heads track 0 out")
	near(player.get_track_frame(1), held, "and leaves the overlay where it stood")


## A track scrubbed by hand (speed 0) is where the host put it, through play().
func test_a_track_driven_by_hand_keeps_its_frame() -> void:
	player.set_track_animation(1, names[1])
	ok(player.set_track_speed(1, 0.0), "set_track_speed")
	ok(player.set_track_frame(1, 7.0), "set_track_frame")
	player.play()
	_step(3)
	near(player.get_track_frame(1), 7.0, "speed 0 holds the frame")


## Track 0 running out does not stop the clock: the overlay carries on, while
## is_playing and is_finished describe track 0. animation_finished is sent once.
func test_the_clock_runs_on_after_track_0_finishes() -> void:
	var finished := [0]
	player.animation_finished.connect(func(_n): finished[0] += 1)
	player.set_loop_count(1)
	player.set_track_animation(1, names[1])
	player.play()
	for i in 2000:
		if player.is_finished():
			break
		player.advance(dt)
	if not ok(player.is_finished(), "track 0 finished"):
		return
	not_ok(player.is_playing(), "and reports itself not playing")
	var overlay: float = player.get_track_frame(1)
	_step(2)
	ne(player.get_track_frame(1), overlay, "the overlay still moves")
	eq(finished[0], 1, "animation_finished fired once")

	# A pause now holds the overlay, but is_pausing describes track 0, which is
	# not playing: the pair never reads pausing without playing.
	player.pause()
	not_ok(player.is_pausing(), "is_pausing stays false after track 0 finished")
	overlay = player.get_track_frame(1)
	_step(2)
	near(player.get_track_frame(1), overlay, "but the overlay holds")


func test_set_animation_folds_the_stack() -> void:
	player.set_track_animation(2, names[1])
	player.set_track_sync(1, player.TRACK_SYNC_START)
	player.set_track_alpha(2, 0.5)
	eq(player.get_track_count(), 3, "three tracks")
	player.set_animation(names[1])
	eq(player.get_track_count(), 1, "track 0 alone after set_animation")
	eq(player.get_track_sync(1), player.TRACK_SYNC_FREE, "a read of a track gone is the default")


## TRACK_SYNC_START lays the animation at the phase track 0 stands at. The same
## animation on both, so the phase is the frame.
func test_sync_start_lays_an_animation_in_phase_with_track_0() -> void:
	player.play()
	_step(6)
	var phase_frame: float = player.get_frame_no()
	gt(phase_frame, 0.0, "track 0 is under way")

	player.set_track_animation(1, names[0])
	near(player.get_track_frame(1), 0.0, "TRACK_SYNC_FREE starts at the head")

	ok(player.set_track_sync(2, player.TRACK_SYNC_START), "set_track_sync on a track not yet laid")
	eq(player.get_track_sync(2), player.TRACK_SYNC_START, "read back")
	player.set_track_animation(2, names[0])
	near(player.get_track_frame(2), phase_frame, "TRACK_SYNC_START starts at track 0's phase", 0.01)

	var source: int = player.add_track_source(2, names[0], 0.0)
	near(player.get_source_frame(source), phase_frame, "add_track_source starts in phase too", 0.01)


func test_sources_on_one_track() -> void:
	var main: int = player.get_track_main_source(0)
	var other: int = player.add_track_source(0, names[1], 0.3)
	ok(other >= 0, "add_track_source returns a handle")
	ne(other, main, "a handle of its own")
	near(player.get_source_weight(other), 0.3, "the weight it was added at")
	ok(player.set_source_weight(main, 0.7), "set_source_weight")
	near(player.get_source_weight(main), 0.7, "read back")
	ok(player.set_source_frame(other, 4.0), "set_source_frame")
	near(player.get_source_frame(other), 4.0, "read back")
	eq(player.get_source_event_threshold(other), INF, "only track 0's main fires by default")
	ok(player.set_source_event_threshold(other, 0.2), "set_source_event_threshold")
	near(player.get_source_event_threshold(other), 0.2, "read back")


func test_fade_track_and_its_cancel() -> void:
	player.set_track_animation(1, names[1])
	player.play()
	ok(player.fade_track(1, 0.0, 10.0 * dt), "fade_track")
	_step(5)
	var progress: float = player.get_track_fade_progress(1)
	ok(progress > 0.0 and progress < 1.0, "under way (%f)" % progress)
	ok(player.cancel_track_fade(1), "cancel_track_fade")
	near(player.get_track_fade_progress(1), -1.0, "no longer fading")
	var alpha: float = player.get_track_alpha(1)
	ok(alpha > 0.0 and alpha < 1.0, "the alpha stays where the fade left it (%f)" % alpha)
	ok(player.clear_track(1), "clear_track")


## Refusals the node makes itself, before the runtime is asked: a name the pack
## does not have, and a track or source number that cannot name one.
func test_what_names_nothing_is_refused() -> void:
	not_ok(player.set_track_animation(1, "no such animation"), "an unknown animation")
	not_ok(player.crossfade(0, "no such animation", 0.1), "crossfade to one")
	eq(player.add_track_source(0, "no such animation", 1.0), -1, "add_track_source of one")
	not_ok(player.set_track_alpha(-1, 0.5), "a negative track")
	eq(player.get_track_main_source(7), -1, "the main source of a track that does not exist")
	near(player.get_track_alpha(7), 0.0, "the alpha of one")
	not_ok(player.set_source_weight(-1, 0.5), "a negative source")
	eq(player.get_track_count(), 1, "nothing above was laid")


func test_nothing_set_up_is_refused() -> void:
	var bare = make_player()
	eq(bare.get_track_count(), 0, "no stack without an animation")
	not_ok(bare.set_track_alpha(0, 0.5), "set_track_alpha")
	eq(bare.get_track_main_source(0), -1, "get_track_main_source")


func _events_player() -> Node:
	var p = make_player(USER_DATA)
	p.set_animation("anime_1")
	p.set_loop_count(1)
	return p


func _run_to_the_end(p: Node) -> void:
	var step := 1.0 / maxf(1.0, float(p.get_frame_rate()))
	for i in 600:
		if p.is_finished():
			return
		p.advance(step)


func _frame_10(got: Array) -> Array:
	return got.filter(func(d): return int(d.get("frame_no", -1)) == 10)


## An overlay at weight 1 does not fire by default; lowering its threshold to its
## weight makes it fire, and each event names the source that fired it.
func test_an_overlay_fires_events_only_once_opted_in() -> void:
	var p = _events_player()
	var got: Array = []
	p.user_data.connect(func(d): got.append(d))
	p.set_track_animation(1, "anime_1")
	p.play()
	_run_to_the_end(p)
	var once := _frame_10(got)
	if eq(once.size(), 1, "track 0's key alone by default"):
		eq(int(once[0].get("source", -1)), p.get_track_main_source(0), "fired by track 0's main source")

	var q = _events_player()
	got.clear()
	q.user_data.connect(func(d): got.append(d))
	q.set_track_animation(1, "anime_1")
	var overlay: int = q.get_track_main_source(1)
	q.set_source_event_threshold(overlay, 1.0)
	q.play()
	_run_to_the_end(q)
	var twice := _frame_10(got)
	if eq(twice.size(), 2, "the overlay's key too, once opted in"):
		var sources := twice.map(func(d): return int(d.get("source", -1)))
		has(sources, q.get_track_main_source(0), "one from track 0")
		has(sources, overlay, "one from the overlay")


## After a crossfade on track 0 the events come from the incoming source, read
## against its animation. The case stops at the first key.
func test_events_after_a_crossfade_on_track_0() -> void:
	var p = _events_player()
	var got: Array = []
	p.user_data.connect(func(d): got.append(d))
	p.play()
	var step := 1.0 / maxf(1.0, float(p.get_frame_rate()))
	p.advance(step)
	p.crossfade(0, "anime_1", step)
	for i in 3:
		p.advance(step)
	near(p.get_track_fade_progress(0), -1.0, "the crossfade has completed")
	var incoming: int = p.get_track_main_source(0)
	got.clear()
	for i in 600:
		if not _frame_10(got).is_empty():
			break
		p.advance(step)
	var at10 := _frame_10(got)
	if eq(at10.size(), 1, "the frame-10 key fired"):
		eq(int(at10[0].get("source", -1)), incoming, "from the incoming source")
		eq(at10[0].get("integer"), 987654321, "read from its animation's table")
