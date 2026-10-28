## SpriteStudioButton: how it fits its player, and the sections its states play.
##
## Cases run before the tree does, so no pointer, focus or layout reaches a
## button here. What the engine would send -- the resize, and the redraw that
## carries a state change -- is sent by hand, and the states code can set,
## disabled and a toggle that is on, are the ones shown. Pointer narrowing,
## hover and press need the tree: the Build Guide's script route.
##
## `Signal.ssab`'s `anime_1` runs from frame 0 to 30, with `Start` on frame 10
## and `End` on 20, on a 320 x 320 canvas centred on the origin.
extends "res://test_base.gd"

const SIGNAL := "res://ssab_generated/overall/Signal.ssab"
const RINGO := "res://ssab_generated/Ringo/Ringo.ssab"

var button: Control
var player: Node


func setup() -> void:
	button = own(ClassDB.instantiate("SpriteStudioButton")) as Control
	root.add_child(button)
	player = _player_under(button, SIGNAL)


func _player_under(parent: Node, path: String) -> Node:
	var child = ClassDB.instantiate("SpriteStudioPlayer2D")
	child.set_animation_process_mode(2)  # ANIMATION_PROCESS_MANUAL
	child.set_ssab_resource(load(path))
	parent.add_child(child)
	return child


func _resize(size: Vector2) -> void:
	button.size = size
	button.notification(Control.NOTIFICATION_RESIZED)


func _redraw() -> void:
	button.notification(CanvasItem.NOTIFICATION_DRAW)


func _section() -> Array:
	return [player.get_start_frame(), player.get_end_frame()]


func _step(frames: int) -> void:
	for i in frames:
		player.advance(1.0 / player.get_frame_rate())


func test_the_canvas_is_fitted_into_the_rect() -> void:
	eq(player.get_canvas_rect(), Rect2(-160, -160, 320, 320), "the canvas this case is about")
	_resize(Vector2(200, 100))
	near(player.scale.x, 0.3125, "fit: the height decides")
	eq(player.scale.x, player.scale.y, "one scale for both axes")
	eq(player.position, Vector2(100, 50), "centred")
	button.stretch_mode = 1  # STRETCH_COVER
	near(player.scale.x, 0.625, "cover: the width decides")
	button.stretch_mode = 2  # STRETCH_KEEP
	near(player.scale.x, 1.0, "keep: authored size")
	eq(player.position, Vector2(100, 50), "with the player's origin at the centre")


func test_the_alignment_takes_up_the_slack() -> void:
	_resize(Vector2(200, 100))
	button.alignment_horizontal = 0  # ALIGNMENT_BEGIN
	eq(player.position, Vector2(50, 50), "the canvas against the left edge")
	button.alignment_horizontal = 2  # ALIGNMENT_END
	eq(player.position, Vector2(150, 50), "against the right edge")


func test_the_fit_follows_what_moves_the_canvas() -> void:
	_resize(Vector2(200, 100))
	player.set_offset(Vector2(40, 0))
	near(player.position.x, 100 - 0.3125 * 40, "the offset canvas is centred again")


func test_with_no_labels_a_state_plays_the_whole_animation() -> void:
	_redraw()
	eq(_section(), [0, 30], "the whole of anime_1")
	ok(player.is_playing(), "playing")
	eq(player.get_loop_count(), -1, "looping")


func test_a_clip_plays_from_its_label_to_the_next() -> void:
	button.state_clips = {"normal": {"label": "Start"}}
	_redraw()
	eq(_section(), [10, 19], "Start up to the frame before End")
	near(player.get_frame_no(), 10.0, "from its first frame")
	ok(player.is_playing(), "playing")


func test_a_state_without_a_clip_falls_back_to_normal() -> void:
	button.state_clips = {"normal": {"label": "Start"}}
	button.disabled = true
	_redraw()
	eq(_section(), [10, 19], "disabled plays normal's")
	button.state_clips = {"normal": {"label": "Start"}, "disabled": {"label": "End"}}
	_redraw()
	eq(_section(), [20, 30], "and its own once it has one")


func test_a_toggle_that_is_on_reads_the_selected_clips_first() -> void:
	button.toggle_mode = true
	button.button_pressed = true
	button.state_clips = {"normal": {"label": "Start"}}
	_redraw()
	eq(_section(), [10, 19], "with no selected_normal, normal")
	button.state_clips = {"normal": {"label": "Start"}, "selected_normal": {"label": "End"}}
	_redraw()
	eq(_section(), [20, 30], "selected_normal")
	button.button_pressed = false
	_redraw()
	eq(_section(), [10, 19], "and normal again once it is off")


func test_reduced_motion_holds_the_first_frame() -> void:
	button.state_clips = {"normal": {"label": "Start"}}
	button.reduced_motion = true
	_redraw()
	not_ok(player.is_playing(), "stopped, so no event fires")
	near(player.get_frame_no(), 10.0, "on the section's first frame")
	button.reduced_motion = false
	_redraw()
	ok(player.is_playing(), "playing again without it")


func test_showing_the_same_state_again_leaves_playback_alone() -> void:
	button.state_clips = {"normal": {"label": "Start"}}
	_redraw()
	_step(3)
	_redraw()
	near(player.get_frame_no(), 13.0, "not restarted", 0.001)


## Assigning an SSAB sets the player up afresh, the same SSAB included, and
## the button shows its state on it again.
func test_an_ssab_assigned_again_is_shown_again() -> void:
	button.state_clips = {"normal": {"label": "Start"}}
	_redraw()
	player.set_ssab_resource(load(SIGNAL))
	_redraw()
	eq(_section(), [10, 19], "the clip's section again")
	ok(player.is_playing(), "playing")


func test_a_clip_naming_what_the_ssab_lacks_plays_as_if_absent() -> void:
	# The button reports the clip once; the run has to stay silent.
	Engine.print_error_messages = false
	button.state_clips = {"normal": {"label": "no such label"}}
	_redraw()
	Engine.print_error_messages = true
	eq(_section(), [0, 30], "the whole animation, as with no clip")


## Ringo's canvases are 800 x 600, except `dead`'s, which is 1300 x 600.
func test_a_clip_can_play_another_animation_without_refitting() -> void:
	var ringo_button := own(ClassDB.instantiate("SpriteStudioButton")) as Control
	root.add_child(ringo_button)
	var ringo := _player_under(ringo_button, RINGO)
	ringo_button.size = Vector2(400, 300)
	ringo_button.notification(Control.NOTIFICATION_RESIZED)
	near(ringo.scale.x, 0.5, "fitted to attack1, the animation selected")
	ringo_button.state_clips = {"normal": {"animation": "dead"}}
	ringo_button.notification(CanvasItem.NOTIFICATION_DRAW)
	eq(ringo.get_current_animation(), "dead", "the clip's animation")
	ok(ringo.is_playing(), "playing")
	near(ringo.scale.x, 0.5, "still fitted to attack1")


func test_an_animation_a_script_selects_is_the_one_read_from() -> void:
	var ringo_button := own(ClassDB.instantiate("SpriteStudioButton")) as Control
	root.add_child(ringo_button)
	var ringo := _player_under(ringo_button, RINGO)
	ringo_button.notification(CanvasItem.NOTIFICATION_DRAW)
	eq(ringo.get_current_animation(), "attack1", "the animation selected")
	ringo.set_animation("walk")
	ringo_button.notification(CanvasItem.NOTIFICATION_DRAW)
	eq(ringo.get_current_animation(), "walk", "not put back")
	ok(ringo.is_playing(), "and played")
