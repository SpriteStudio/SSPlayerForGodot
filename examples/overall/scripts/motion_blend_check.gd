extends Node2D

# Visual check for motion blending (the track stack). Builds its own player and
# HUD, so the scene only has to hold this script. Run the scene and use the keys
# the HUD lists; the HUD shows what the runtime reports back, and the frame drawn
# around the character is get_canvas_rect(), so a switch of canvas shows.
#
# The packs it cycles through with Tab are the ones deploy-examples put here:
# Ringo's `dead` is the one animation on a wider canvas, and UserData carries an
# event key at frame 10 for the outgoing-event check.

const SSAB_PATHS := [
	"res://ssab_generated/Doll/Action02_Body.ssab",
	"res://ssab_generated/Ringo/Ringo.ssab",
	"res://ssab_generated/overall/UserData.ssab",
]
const FADE_SECONDS := 0.4
const SPEEDS := [1.0, 2.0, 0.5]
const LOG_LINES := 6

var player: SpriteStudioPlayer2D
var hud: Label
var paths: Array = []
var path_index := 0
var names: PackedStringArray

var overlay_index := -1   # what track 1 plays, -1 for nothing
var mix_source := -1      # a second source on track 0 (M), -1 for none
var mix_ratio := 0.5
var track0_faded := false
var speed_index := 0
var log_lines := PackedStringArray()


func _ready() -> void:
	for path in SSAB_PATHS:
		if ResourceLoader.exists(path):
			paths.append(path)
	if paths.is_empty():
		push_error("None of %s is here -- run scripts/deploy-examples first." % [SSAB_PATHS])
		return

	player = SpriteStudioPlayer2D.new()
	player.autoplay = false
	add_child(player)
	player.position = get_viewport_rect().size * Vector2(0.5, 0.75)

	player.user_data.connect(func(p): _log("user_data frame %d source %d" % [p.frame_no, p.source]))
	player.signal_emitted.connect(func(c, _v, i): _log("signal '%s' frame %d source %d" % [c, i.frame_no, i.source]))
	player.audio.connect(func(p): _log("audio frame %d source %d" % [p.frame_no, p.source]))
	player.animation_changed.connect(func(n): _log("animation_changed(%s)" % n))
	player.animation_started.connect(func(n): _log("animation_started(%s)" % n))
	player.animation_finished.connect(func(n): _log("animation_finished(%s)" % n))
	player.animation_looped.connect(func(n): _log("animation_looped(%s)" % n))

	var layer := CanvasLayer.new()
	add_child(layer)
	hud = Label.new()
	hud.position = Vector2(12, 8)
	hud.add_theme_font_size_override("font_size", 14)
	layer.add_child(hud)

	_load_pack()


func _load_pack() -> void:
	player.set_ssab_resource(load(paths[path_index]))
	names = player.get_animation_names()
	overlay_index = -1
	mix_source = -1
	track0_faded = false
	if names.is_empty():
		push_error("No animation in %s." % paths[path_index])
		return
	player.set_animation(names[0])
	player.play()


func _log(line: String) -> void:
	log_lines.append(line)
	if log_lines.size() > LOG_LINES:
		log_lines.remove_at(0)


func _current_index() -> int:
	return maxi(0, names.find(player.get_current_animation()))


func _wrap(index: int) -> int:
	return posmod(index, names.size())


func _unhandled_input(event: InputEvent) -> void:
	if player == null or not (event is InputEventKey and event.pressed and not event.echo):
		return
	if event.keycode == KEY_TAB:
		path_index = (path_index + 1) % paths.size()
		_load_pack()
		return
	if names.is_empty():
		return
	match event.keycode:
		KEY_C:
			# The incoming animation is the current one from this call on.
			player.crossfade(0, names[_wrap(_current_index() + 1)], FADE_SECONDS)
			# Every source the track carried fades out with the outgoing side, and
			# is released when the fade completes.
			mix_source = -1
		KEY_UP, KEY_DOWN:
			# set_animation() by name: after a crossfade, the animation it left is
			# no longer the current one, so going back switches.
			var step := 1 if event.keycode == KEY_UP else -1
			player.set_animation(names[_wrap(_current_index() + step)])
			overlay_index = -1
			mix_source = -1
			track0_faded = false
		KEY_S:
			overlay_index = _wrap(overlay_index + 1)
			player.set_track_animation(1, names[overlay_index])
		KEY_D:
			player.clear_track(1)
			overlay_index = -1
		KEY_1:
			player.set_track_alpha(1, 0.3)
		KEY_2:
			player.set_track_alpha(1, 0.6)
		KEY_3:
			player.set_track_alpha(1, 1.0)
		KEY_A:
			var additive := player.get_track_blend(1) == SpriteStudioPlayer2D.TRACK_BLEND_ADDITIVE
			player.set_track_blend(1, SpriteStudioPlayer2D.TRACK_BLEND_MIX if additive else SpriteStudioPlayer2D.TRACK_BLEND_ADDITIVE)
		KEY_0:
			track0_faded = not track0_faded
			player.fade_track(0, 0.0 if track0_faded else 1.0, FADE_SECONDS)
		KEY_COMMA, KEY_PERIOD:
			player.set_track_speed(1, 0.0)
			var step := -1.0 if event.keycode == KEY_COMMA else 1.0
			player.set_track_frame(1, maxf(0.0, player.get_track_frame(1) + step))
		KEY_R:
			player.set_track_speed(1, 1.0)
		KEY_Y:
			var start := player.get_track_sync(0) == SpriteStudioPlayer2D.TRACK_SYNC_START
			var sync := SpriteStudioPlayer2D.TRACK_SYNC_FREE if start else SpriteStudioPlayer2D.TRACK_SYNC_START
			player.set_track_sync(0, sync)
			player.set_track_sync(1, sync)
		KEY_M:
			if mix_source < 0:
				mix_source = player.add_track_source(0, names[_wrap(_current_index() + 1)], mix_ratio)
				player.set_source_weight(player.get_track_main_source(0), 1.0 - mix_ratio)
		KEY_LEFT, KEY_RIGHT:
			if mix_source >= 0:
				mix_ratio = clampf(mix_ratio + (0.1 if event.keycode == KEY_RIGHT else -0.1), 0.0, 1.0)
				player.set_source_weight(mix_source, mix_ratio)
				player.set_source_weight(player.get_track_main_source(0), 1.0 - mix_ratio)
		KEY_T:
			var source: int = player.get_track_main_source(1)
			if source >= 0:
				var firing := player.get_source_event_threshold(source) <= 1.0
				player.set_source_event_threshold(source, INF if firing else 1.0)
		KEY_V:
			# Track 0's speed is speed_scale; a crossfade plays the incoming
			# animation at it too.
			speed_index = (speed_index + 1) % SPEEDS.size()
			player.speed_scale = SPEEDS[speed_index]
		KEY_L:
			# The node's loop count, which it puts back on every animation track 0
			# switches to.
			player.loop_count = -1 if player.loop_count == 1 else 1
		KEY_K:
			if player.get_track_loop_count(1) != 0:
				player.set_track_loop_count(1, -1 if player.get_track_loop_count(1) == 1 else 1)
		KEY_E:
			var all := player.get_track_outgoing_event_threshold(0) == INF
			player.set_track_outgoing_event_threshold(0, 0.5 if all else INF)
		KEY_SPACE:
			if player.is_pausing():
				player.resume()
			else:
				player.pause()
		KEY_P:
			player.play()
		KEY_X:
			player.stop()
		KEY_BACKSPACE:
			# set_animation() folds the stack; selecting the same name again is a
			# no-op, so go by way of another animation.
			var current := player.get_current_animation()
			player.set_animation(names[_wrap(_current_index() + 1)])
			player.set_animation(current)
			player.play()
			overlay_index = -1
			mix_source = -1
			track0_faded = false


func _loops(count: int) -> String:
	return "inf" if count < 0 else ("-" if count == 0 else str(count))


func _track_line(track: int) -> String:
	if track >= player.get_track_count():
		return "track %d: -" % track
	var blend := "Additive" if player.get_track_blend(track) == SpriteStudioPlayer2D.TRACK_BLEND_ADDITIVE else "Mix"
	var sync := "Start" if player.get_track_sync(track) == SpriteStudioPlayer2D.TRACK_SYNC_START else "Free"
	var fade := player.get_track_fade_progress(track)
	return "track %d: frame %6.2f  alpha %.2f  %s  sync %s  fade %s  speed %.1f  loops %s  finished %s  main source %d" % [
		track, player.get_track_frame(track), player.get_track_alpha(track), blend, sync,
		("%.2f" % fade) if fade >= 0.0 else "-", player.get_track_speed(track),
		_loops(player.get_track_loop_count(track)), player.is_track_finished(track),
		player.get_track_main_source(track)]


func _process(_delta: float) -> void:
	queue_redraw()
	if hud == null or player == null:
		return
	var lines := PackedStringArray()
	lines.append("[Tab] next pack   [C] crossfade track 0 -> next   [Up/Down] set_animation next/previous")
	lines.append("[V] track 0 speed 1/2/0.5   [L] loop_count 1/inf   [E] outgoing events none/first half")
	lines.append("[S] lay next on track 1   [D] clear track 1   [K] track 1 loops 1/inf   [T] track 1 fires events")
	lines.append("[1/2/3] track 1 alpha 0.3/0.6/1.0   [A] Mix/Additive   [0] fade track 0 out/in")
	lines.append("[, .] scrub track 1 (speed 0)   [R] track 1 speed 1   [Y] sync Free/Start")
	lines.append("[M] add a 2nd source on track 0   [<- ->] its weight")
	lines.append("[Space] pause/resume   [P] play   [X] stop   [Backspace] reset the stack")
	lines.append("")
	lines.append("pack: %s" % paths[path_index].get_file())
	lines.append("current animation: %s   canvas %s   track 1 plays: %s" % [player.get_current_animation(),
		player.get_canvas_size(), names[overlay_index] if overlay_index >= 0 else "-"])
	if mix_source >= 0:
		lines.append("track 0 mix: main %.1f / source %d %.1f" % [1.0 - mix_ratio, mix_source, mix_ratio])
	lines.append("playing %s  pausing %s  finished %s  loop_count %s  speed_scale %.1f  tracks %d" % [
		player.is_playing(), player.is_pausing(), player.is_finished(), _loops(player.loop_count),
		player.speed_scale, player.get_track_count()])
	var outgoing := player.get_track_outgoing_event_threshold(0)
	lines.append("track 0 outgoing events: %s" % ("none" if outgoing == INF else "while its share >= %.1f" % outgoing))
	lines.append(_track_line(0))
	lines.append(_track_line(1))
	var overlay_source: int = player.get_track_main_source(1)
	if overlay_source >= 0:
		lines.append("track 1 event threshold: %s" % player.get_source_event_threshold(overlay_source))
	lines.append("")
	lines.append("log (newest last):")
	lines.append_array(log_lines)
	hud.text = "\n".join(lines)


func _draw() -> void:
	if player == null:
		return
	var rect := player.get_canvas_rect()
	if rect.size == Vector2():
		return
	draw_rect(Rect2(player.position + rect.position, rect.size), Color(0.3, 0.8, 1.0, 0.6), false, 2.0)
