## The eight signals, their payloads, and when each one fires.
##
## Worth its own suite because a signal is the one part of the API that fails
## silently: a renamed signal, or one that stopped being emitted, breaks every
## host that connected to it and breaks nothing that a build would notice. The
## arity matters as much as the name — connecting a zero-argument callable to
## `animation_finished(anim_name)` is an error at emit time, not at connect time.
extends "res://test_base.gd"

const BASIC := "res://ssab_generated/overall/Basic.ssab"
const RINGO := "res://ssab_generated/Ringo/Ringo.ssab"
# The SDK's tests/overall packs that author one kind of timeline event each.
const USER_DATA := "res://ssab_generated/overall/UserData.ssab"
const SIGNALS := "res://ssab_generated/overall/Signal.ssab"
const SOUND := "res://ssab_generated/overall/Sound.ssab"

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


func test_the_node_declares_all_eight() -> void:
	for name in ["animation_started", "animation_changed", "animation_finished",
			"animation_looped", "frame_updated", "user_data", "signal_emitted", "audio"]:
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


## One pass of `anime_1`, stepped to its end.
##
## The cases below assert the keys authored after frame 0 only. Whether the
## frame a run starts on reports its own keys is the start edge's business, and
## asserting it here would test the transport rather than the signal.
func _play_once(p: Node) -> void:
	p.set_animation("anime_1")
	p.set_loop_count(1)
	p.play()
	var step := 1.0 / maxf(1.0, float(p.get_frame_rate()))
	for i in 600:
		if p.is_finished():
			return
		p.advance(step)


## UserData.ssab keys the `UDAT` part at frames 0 and 10; the frame-10 key holds
## 987654321, (56, 78), the rect 5 6 7 8 and 「10フレーム到達のお知らせ」.
func test_user_data_carries_the_key_and_where_it_sits() -> void:
	var p = make_player(USER_DATA)
	var got: Array = []
	p.user_data.connect(func(d): got.append(d))
	_play_once(p)

	var at10 := got.filter(func(d): return int(d.get("frame_no", -1)) == 10)
	if not eq(at10.size(), 1, "the frame-10 key fired once"):
		return
	var d: Dictionary = at10[0]
	eq(d.get("part_name"), "UDAT", "it names the part the key sits on")
	ok(int(d.get("part_index", -1)) >= 0, "and that part's index")
	eq(d.get("integer"), 987654321, "the integer")
	eq(d.get("point"), Vector2(56, 78), "the point")
	eq(d.get("rect"), Rect2(5, 6, 2, 2), "the rect, as position and size")
	eq(d.get("string"), "10フレーム到達のお知らせ", "the string")


## Signal.ssab keys `root` at frame 10 with two commands: `test_cmd`, which
## carries one parameter of every type, and `play_se`.
func test_signal_emitted_carries_each_command_and_its_parameters() -> void:
	var p = make_player(SIGNALS)
	var got: Array = []
	p.signal_emitted.connect(func(command, value, info): got.append([command, value, info]))
	_play_once(p)

	var commands := got.map(func(e): return e[0])
	has(commands, "test_cmd", "the key's first command fired")
	has(commands, "play_se", "and its second")
	for e in got:
		eq(e[2].get("part_name"), "root", "'%s' names the part it sits on" % e[0])
		eq(int(e[2].get("frame_no", -1)), 10, "'%s' names its key's frame" % e[0])
		if e[0] == "test_cmd":
			eq(e[1].get("text"), "文字列", "test_cmd's text parameter, keyed by its id")
			eq(e[1].get("num"), 0, "and its integer one")
		elif e[0] == "play_se":
			eq(e[1].get("volume"), 100, "play_se's volume, keyed by its id")
			near(float(e[1].get("pitch", -1.0)), 1.0, "and its pitch")


## Sound.ssab triggers MP3_44khz_mono_160kbps on `SE_looped` at frame 10, three
## plays, and WAV_16khz_mono_256kbps_s16 on `Speach` at frame 20. `audio` is the
## observation channel, so it fires with the node's own playback switched off.
func test_audio_fires_whether_or_not_the_node_plays_it() -> void:
	var p = make_player(SOUND)
	p.play_audio = false
	var got: Array = []
	p.audio.connect(func(d): got.append(d))
	_play_once(p)

	var by_name := {}
	for d in got:
		by_name[d.get("sound_name", "")] = d
	has(by_name, "MP3_44khz_mono_160kbps", "the frame-10 trigger fired")
	has(by_name, "WAV_16khz_mono_256kbps_s16", "and the frame-20 one")
	if by_name.has("MP3_44khz_mono_160kbps"):
		var mp3: Dictionary = by_name["MP3_44khz_mono_160kbps"]
		eq(mp3.get("part_name"), "SE_looped", "it names the part it sits on")
		eq(int(mp3.get("frame_no", -1)), 10, "and its key's frame")
		eq(int(mp3.get("loop_num", -1)), 3, "and the authored play count")
		ne(int(mp3.get("sound_name_hash", 0)), 0, "and a sound-name hash")
