## Sections named by labels, and the label table they are read from.
##
## `Signal.ssab`'s `anime_1` runs from frame 0 to 30 and carries two labels,
## `Start` on frame 10 and `End` on frame 20.
extends "res://test_base.gd"

const SIGNAL := "res://ssab_generated/overall/Signal.ssab"

var player: Node


func setup() -> void:
	player = make_player(SIGNAL)
	player.set_animation("anime_1")


func _section() -> Array:
	return [player.get_start_frame(), player.get_end_frame()]


func test_the_label_table_is_in_frame_order() -> void:
	eq(player.get_label_table(), [{"name": "Start", "frame": 10}, {"name": "End", "frame": 20}], "anime_1's labels")
	eq(player.get_label_table("anime_1"), player.get_label_table(), "named or current, the same table")
	eq(player.get_label_table("no such animation"), [], "an animation the pack lacks has none")


func test_a_section_by_label_runs_between_the_two_frames() -> void:
	ok(player.set_animation_section_by_label("Start", "End"), "Start to End")
	eq(_section(), [10, 20], "both labelled frames are in it")
	ok(player.set_animation_section_by_label("Start", "End", 1, -1), "with offsets")
	eq(_section(), [11, 19], "the offsets are added to the frames")
	not_ok(player.is_playing(), "a section by label does not play")


## `_start` and `_end` are SpriteStudio's names for an animation's own ends --
## not the current section's, which is what a section by label replaces.
func test_start_and_end_name_the_animations_own_ends() -> void:
	ok(player.set_animation_section(12, 15), "a narrow section first")
	ok(player.set_animation_section_by_label("_start", "Start", 0, -1), "_start")
	eq(_section(), [0, 9], "_start is frame 0")
	ok(player.set_animation_section_by_label("End", "_end"), "_end")
	eq(_section(), [20, 30], "_end is the last frame")


func test_what_names_no_section_is_refused_and_changes_nothing() -> void:
	ok(player.set_animation_section_by_label("Start", "End"), "a section to keep")
	player.set_frame_no(15.0)
	# The node reports each refusal once; the run has to stay silent.
	Engine.print_error_messages = false
	var missing: bool = player.set_animation_section_by_label("Start", "Ende")
	var backwards: bool = player.set_animation_section_by_label("End", "Start")
	Engine.print_error_messages = true
	not_ok(missing, "a label anime_1 does not have")
	not_ok(backwards, "labels that put the end before the start")
	eq(_section(), [10, 20], "the section stays")
	near(player.get_frame_no(), 15.0, "the playhead stays")


func test_with_no_animation_there_is_nothing_to_name() -> void:
	var bare := make_player()
	not_ok(bare.set_animation_section_by_label("Start", "End"), "nothing is set up")
	eq(bare.get_label_table(), [], "and there is no table")
