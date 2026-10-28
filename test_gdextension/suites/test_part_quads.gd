## Part outlines, and the hit test that walks them front to back.
##
## The coordinates are the fixtures' own on their first frame: `Basic`'s
## `POS_XYZ` is a 64 x 64 cell around the origin, `Mesh`'s `mesh_deform` spans
## 40..88 x -24..24, and `Shape`'s `Shape` is 96 x 96 around the origin.
extends "res://test_base.gd"

const BASIC := "res://ssab_generated/overall/Basic.ssab"
const MESH := "res://ssab_generated/overall/Mesh.ssab"
const SHAPE := "res://ssab_generated/overall/Shape.ssab"


func _player(path: String) -> Node:
	var player := make_player(path)
	player.set_animation("anime_1")
	return player


func _quad(left: float, top: float, right: float, bottom: float) -> PackedVector2Array:
	return PackedVector2Array([Vector2(left, top), Vector2(right, top), Vector2(right, bottom), Vector2(left, bottom)])


func _near_quad(actual: PackedVector2Array, expected: PackedVector2Array, what: String) -> void:
	if not eq(actual.size(), 4, "%s has four corners" % what):
		return
	for i in 4:
		near(actual[i].x, expected[i].x, "%s corner %d x" % [what, i], 0.001)
		near(actual[i].y, expected[i].y, "%s corner %d y" % [what, i], 0.001)


func test_a_cell_part_is_its_drawn_corners() -> void:
	var player := _player(BASIC)
	_near_quad(player.get_part_quad("POS_XYZ"), _quad(-32, -32, 32, 32), "POS_XYZ")


## The outline is in the node's space, as get_part_transform is, so flip and
## offset -- which live below the node's own transform -- are in it.
func test_flip_and_offset_are_in_the_outline() -> void:
	var player := _player(BASIC)
	player.set_offset(Vector2(10, 20))
	_near_quad(player.get_part_quad("POS_XYZ"), _quad(-22, -12, 42, 52), "offset")
	player.set_offset(Vector2.ZERO)
	player.set_flip_h(true)
	_near_quad(player.get_part_quad("ROT_Z"), _quad(-32, 32, -96, 96), "flipped, mirrored corner for corner")


func test_a_mesh_part_is_the_box_around_its_vertices() -> void:
	var player := _player(MESH)
	_near_quad(player.get_part_quad("mesh_deform"), _quad(40, -24, 88, 24), "mesh_deform")


func test_a_shape_part_is_its_size_around_its_pivot() -> void:
	var player := _player(SHAPE)
	_near_quad(player.get_part_quad("Shape"), _quad(-48, -48, 48, 48), "Shape")


func test_what_draws_nothing_of_its_own_has_no_outline() -> void:
	var player := _player(BASIC)
	eq(player.get_part_quad("root"), PackedVector2Array(), "a null part")
	eq(player.get_part_quad("HIDE_NoKey"), PackedVector2Array(), "a hidden part")
	eq(player.get_part_quad("no such part"), PackedVector2Array(), "an unknown part")
	var mesh := _player(MESH)
	eq(mesh.get_part_quad("bone_1"), PackedVector2Array(), "a bone")


## `PRIO_0`, `PRIO_to_1` and `PRIO_to_2` sit on one square. On the first frame
## the last of them is drawn on top; by frame 10 `PRIO_to_2`'s priority has
## carried it there.
func test_the_front_most_part_answers() -> void:
	var player := _player(BASIC)
	var point := Vector2(0, -192)
	eq(player.hit_test_part(point), player.find_part_index("PRIO_0"), "frame 0")
	player.set_frame_no(10.0)
	eq(player.hit_test_part(point), player.find_part_index("PRIO_to_2"), "frame 10")
	eq(player.hit_test_part(point, PackedStringArray(["PRIO_to_1"])), player.find_part_index("PRIO_to_1"),
		"among the parts asked about")


func test_a_point_on_nothing_asked_about_misses() -> void:
	var player := _player(BASIC)
	eq(player.hit_test_part(Vector2(0, -40)), -1, "between the squares")
	eq(player.hit_test_part(Vector2(0, 0), PackedStringArray(["CELL"])), -1, "on a part not asked about")
	eq(player.hit_test_part(Vector2(0, 0), PackedStringArray(["no such part"])), -1, "only unknown names")
	eq(player.hit_test_part(Vector2(0, 0), PackedStringArray(["no such part", "POS_XYZ"])),
		player.find_part_index("POS_XYZ"), "an unknown name among them is skipped")


func test_the_point_is_in_the_nodes_space() -> void:
	var player := _player(BASIC)
	player.set_offset(Vector2(1000, 0))
	eq(player.hit_test_part(Vector2(0, 0)), -1, "the art moved away")
	eq(player.hit_test_part(Vector2(1000, 0)), player.find_part_index("POS_XYZ"), "and is found where it went")
