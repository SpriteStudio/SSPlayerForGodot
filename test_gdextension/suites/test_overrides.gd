## The part override layer, and the one rule that is easy to get wrong.
##
## **An override lands on the next update, not on the call.** `is_part_hidden`
## reports what the last computed frame said — `_part_hidden` is filled from the
## Brain's draw plan in `SsInternalPlayer` — so a host that sets an override and
## reads it back without stepping sees the old answer and concludes the call did
## nothing. Every case here steps once after setting, deliberately, and
## `test_an_override_lands_on_the_next_update` is the one that states it.
extends "res://test_base.gd"

const BASIC := "res://ssab_generated/overall/Basic.ssab"

var player: Node
var dt := 1.0 / 30.0
var part := ""


func setup() -> void:
	player = make_player(BASIC)
	player.set_animation("anime_1")
	dt = 1.0 / maxf(1.0, float(player.get_frame_rate()))
	player.play()
	player.advance(dt)
	# Not the root: hiding that one is the cascade case below.
	part = player.get_part_names()[1]


func test_a_part_is_visible_before_anything_overrides_it() -> void:
	not_ok(player.is_part_hidden(part), "'%s' starts visible" % part)


func test_an_override_lands_on_the_next_update() -> void:
	ok(player.set_part_visibility_override(part, true), "the call was accepted")
	not_ok(player.is_part_hidden(part), "not yet — nothing has been computed")
	player.advance(dt)
	ok(player.is_part_hidden(part), "hidden once a frame has been computed")


func test_clearing_gives_the_part_back() -> void:
	player.set_part_visibility_override(part, true)
	player.advance(dt)
	ok(player.is_part_hidden(part), "hidden")
	ok(player.clear_part_visibility_override(part), "the clear was accepted")
	player.advance(dt)
	not_ok(player.is_part_hidden(part), "visible again")


## Cascade is what makes hiding a limb hide what hangs off it.
func test_cascade_reaches_the_children() -> void:
	var root_part: String = player.get_part_names()[0]
	player.set_part_visibility_override(root_part, true, true)
	player.advance(dt)
	ok(player.is_part_hidden(part), "a child of the cascaded part is hidden")


func test_without_cascade_a_child_is_left_alone() -> void:
	var root_part: String = player.get_part_names()[0]
	player.set_part_visibility_override(root_part, true, false)
	player.advance(dt)
	not_ok(player.is_part_hidden(part), "the child keeps its own visibility")


func test_clear_all_undoes_every_override_at_once() -> void:
	player.set_part_visibility_override(part, true)
	player.advance(dt)
	ok(player.is_part_hidden(part), "hidden before the clear")
	player.clear_all_part_overrides()
	player.advance(dt)
	not_ok(player.is_part_hidden(part), "visible after clear_all")


## The by-index half of the API has to mean the same thing as the by-name half;
## a host that already resolved an index should not have to go back to a string.
func test_the_index_half_agrees_with_the_name_half() -> void:
	var index: int = player.find_part_index(part)
	gt(index, 0, "'%s' resolves to an index" % part)
	ok(player.set_part_visibility_override_by_index(index, true, false),
		"the by-index call was accepted")
	player.advance(dt)
	ok(player.is_part_hidden(part), "the by-index override hides the same part")
	player.clear_part_visibility_override_by_index(index)
	player.advance(dt)
	not_ok(player.is_part_hidden(part), "and the by-index clear gives it back")


func test_an_unknown_part_is_refused_rather_than_ignored() -> void:
	not_ok(player.set_part_visibility_override("no such part", true),
		"an override on a part that does not exist")
	eq(player.find_part_index("no such part"), -1, "and it has no index")


## Colour and cell overrides are accepted through the same door. What they draw
## is a rendering question and out of scope here; that the call reports success
## for a real part and failure for an imaginary one is not.
func test_colour_and_cell_overrides_are_accepted_for_a_real_part() -> void:
	# The two zeroes are COLOR_BLEND_MIX and OVERRIDE_PRIORITY_OVERWRITE_ON_NEXT_KEYFRAME.
	ok(player.set_part_color_override(part, Color(1, 0, 0, 1), 0, 0),
		"a colour override on '%s'" % part)
	player.advance(dt)
	ok(player.clear_part_color_override(part), "clearing it")

	var cellmap: String = player.get_cellmap_names()[0]
	var cells = player.get_cell_names(cellmap)
	if cells.is_empty():
		skip("the first cellmap of Basic has no cells to swap to")
		return
	ok(player.set_part_cell_override(part, cellmap, cells[0]),
		"a cell override on '%s'" % part)
	player.advance(dt)
	ok(player.clear_part_cell_override(part), "clearing it")
