## The five signals, their payloads, and when each one fires.
##
## Worth its own suite because a signal is the one part of the API that fails
## silently: a renamed signal, or one that stopped being emitted, breaks every
## host that connected to it and breaks nothing that a build would notice. The
## arity matters as much as the name — connecting a zero-argument callable to
## `animation_finished(anim_name)` is an error at emit time, not at connect time.
extends "res://test_base.gd"

const BASIC := "res://ssab_generated/overall/Basic.ssab"
const RINGO := "res://ssab_generated/Ringo/Ringo.ssab"

var player: Node
var dt := 1.0 / 30.0
var seen: Array = []
var frames: Array = []


func setup() -> void:
	seen = []
	frames = []
	player = make_player(BASIC)
	player.animation_started.connect(func(n): seen.append(["started", n]))
	player.animation_changed.connect(func(n): seen.append(["changed", n]))
	player.animation_finished.connect(func(n): seen.append(["finished", n]))
	player.animation_looped.connect(func(n): seen.append(["looped", n]))
	player.frame_updated.connect(func(f): frames.append(f))
	player.set_animation("anime_1")
	dt = 1.0 / maxf(1.0, float(player.get_frame_rate()))


func _kinds() -> Array:
	return seen.map(func(e): return e[0])


func test_the_node_declares_all_five() -> void:
	for name in ["animation_started", "animation_changed", "animation_finished",
			"animation_looped", "frame_updated"]:
		ok(player.has_signal(name), "the '%s' signal exists" % name)


## Setting up an animation is not starting it: a host that pre-selects an
## animation on a stopped player should not see a start.
func test_setting_an_animation_does_not_start_it() -> void:
	eq(_kinds().has("started"), false, "no start from set_animation alone")


func test_play_starts_it_and_names_it() -> void:
	player.play()
	player.advance(dt)
	has(_kinds(), "started", "animation_started fired")
	for entry in seen:
		if entry[0] == "started":
			eq(entry[1], "anime_1", "animation_started carries the animation name")


func test_frame_updated_fires_once_per_advance() -> void:
	player.play()
	for i in 6:
		player.advance(dt)
	eq(frames.size(), 6, "one frame_updated per advance()")
	ok(frames[-1] is float, "frame_updated carries a frame number")
	ok(frames[-1] > frames[0], "and the number moves")


## Two loops of a non-looping-forever animation: the boundary is a `looped`, the
## end of the last pass is a `finished`, and they arrive in that order.
func test_a_bounded_run_loops_then_finishes() -> void:
	player.set_loop_count(2)
	player.play()
	for i in 60:
		player.advance(dt)
	var kinds := _kinds()
	has(kinds, "looped", "animation_looped fired at the loop boundary")
	has(kinds, "finished", "animation_finished fired at the end")
	ok(kinds.find("looped") < kinds.find("finished"),
		"the loop boundary comes before the end")
	ok(player.is_finished(), "and the player reports itself finished")


func test_every_payload_is_the_animation_name() -> void:
	player.set_loop_count(1)
	player.play()
	for i in 40:
		player.advance(dt)
	gt(seen.size(), 0, "something fired")
	for entry in seen:
		eq(entry[1], "anime_1", "'%s' carries the animation name" % entry[0])


## The signal a host uses to react to a switch, as opposed to a start.
##
## Ringo rather than Basic, which carries a single animation and so has nothing
## to switch to.
func test_switching_animations_reports_the_change() -> void:
	var multi = make_player(RINGO)
	multi.set_animation_process_mode(2)
	var switches: Array = []
	multi.animation_changed.connect(func(n): switches.append(n))

	var names = multi.get_animation_names()
	gt(names.size(), 1, "Ringo has more than one animation")
	multi.set_animation(names[0])
	multi.play()
	multi.advance(dt)
	switches.clear()

	multi.set_animation(names[1])
	multi.advance(dt)
	eq(switches.size(), 1, "animation_changed fired once on the switch")
	if switches.size() == 1:
		eq(switches[0], names[1], "and it names the animation switched to")
