## What a pack tells the node about itself, before anything plays.
##
## `SSABResource` is a Resource, so `load()` is the whole loading story — the
## pack's textures and its instance sub-packs are resolved beside it. These are
## the reads a host does to build a UI (an animation picker, a cell list), and
## every one of them is a name that the API conventions settled, so they are
## also where a rename shows up first.
extends "res://test_base.gd"

const BASIC := "res://ssab_generated/overall/Basic.ssab"
const RINGO := "res://ssab_generated/Ringo/Ringo.ssab"

var player: Node


func setup() -> void:
	player = make_player(BASIC)


func test_a_pack_loads_as_its_own_resource_type() -> void:
	var res := load(BASIC)
	ok(res != null, "load() returned something")
	eq(res.get_class(), "SSABResource", "the pack's resource type")


func test_the_pack_names_its_animations() -> void:
	var names = player.get_animation_names()
	gt(names.size(), 0, "Basic has at least one animation")
	has(names, "anime_1", "Basic's animation")


func test_the_pack_names_its_cellmaps_and_cells() -> void:
	var cellmaps = player.get_cellmap_names()
	has(cellmaps, "common", "Basic's first cellmap")
	gt(player.get_cell_names("common").size(), 0, "cells in 'common'")
	eq(player.get_cell_names("no such cellmap").size(), 0,
		"an unknown cellmap has no cells")


func test_the_pack_names_its_parts_root_first() -> void:
	var parts = player.get_part_names()
	gt(parts.size(), 1, "Basic has parts")
	eq(parts[0], "root", "the part list starts at the root")


## The index is the currency of the `*_by_index` half of the override API, so
## the two ways of naming a part have to agree.
func test_a_part_resolves_from_its_name_to_its_index() -> void:
	var parts = player.get_part_names()
	for i in mini(parts.size(), 8):
		eq(player.find_part_index(parts[i]), i, "index of '%s'" % parts[i])
	eq(player.find_part_index("no such part"), -1, "an unknown part has no index")


func test_frame_metadata_describes_the_animation() -> void:
	player.set_animation("anime_1")
	eq(player.get_current_animation(), "anime_1", "the animation that is set up")
	gt(player.get_frame_rate(), 0, "frame rate")
	gt(player.get_total_frames(), 0, "total frames")
	eq(player.get_start_frame(), 0, "the first frame")
	eq(player.get_end_frame(), player.get_total_frames() - 1, "the last frame")


## The section defaults to the whole animation; `test_playback` covers narrowing it.
func test_the_section_starts_as_the_whole_animation() -> void:
	player.set_animation("anime_1")
	eq(player.get_animation_section_start(), player.get_start_frame(), "section start")
	eq(player.get_animation_section_end(), player.get_end_frame(), "section end")


## A second pack, so nothing above is an accident of the one file.
func test_a_second_pack_reads_the_same_way() -> void:
	var ringo := make_player(RINGO)
	gt(ringo.get_animation_names().size(), 1, "Ringo has several animations")
	gt(ringo.get_part_names().size(), 1, "Ringo has parts")
	eq(ringo.get_part_names()[0], "root", "Ringo's part list starts at the root")
