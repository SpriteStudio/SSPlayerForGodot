## Assertions and per-case bookkeeping for the headless suite.
##
## A suite extends this and names its cases `test_*`; `run_tests.gd` finds them
## by reflection and calls each one with a fresh `setup()`/`teardown()` around
## it, so a case that leaves a player in a strange state cannot mislead the next.
##
## Three verdicts, not two. `fail()` is a defect. `skip()` is a case that
## **declared** it cannot run here — a host service this platform does not
## offer — and is reported apart from the passes rather than counted as one:
## the whole point of running on three platforms without asserting the same
## things on all three is that the difference stays visible.
extends RefCounted

var failures: Array[String] = []
var skips: Array[String] = []
var assertions := 0

var _case := ""
var _owned: Array[Node] = []

## The tree cases attach nodes to. `run_tests.gd` sets it before the first case.
var root: Node = null


func begin_case(name: String) -> void:
	_case = name


## Hook for a suite that needs one player per case. Called before each `test_*`.
func setup() -> void:
	pass


## Hook for a suite's own cleanup. Called after each `test_*`, pass or fail.
func teardown() -> void:
	pass


## Frees whatever `own()` was given, after `teardown()`.
func release_owned() -> void:
	for node in _owned:
		if is_instance_valid(node):
			if node.get_parent() != null:
				node.get_parent().remove_child(node)
			node.free()
	_owned.clear()


## Hands a node to the harness to free at the end of the case.
##
## Not a convenience: a `SpriteStudioPlayer2D` that is never freed leaks its
## canvas-item RIDs, and Godot reports that only at exit — as a warning, long
## after the case that caused it, and with nothing naming the culprit.
func own(node: Node) -> Node:
	_owned.append(node)
	return node


## A player parented to the test root, stepped by hand rather than by the clock.
##
## MANUAL is what makes a case reproducible: nothing advances between the
## assertions except the `advance()` calls the case itself makes, so the result
## does not depend on how long a frame took on this machine.
func make_player(ssab_path := "") -> Node:
	var player = ClassDB.instantiate("SpriteStudioPlayer2D")
	own(player)
	root.add_child(player)
	player.set_animation_process_mode(2)  # ANIMATION_PROCESS_MANUAL
	if ssab_path != "":
		player.set_ssab_resource(load(ssab_path))
	return player


func _record_failure(message: String) -> void:
	failures.append("%s: %s" % [_case, message])


func ok(condition: bool, what: String) -> bool:
	assertions += 1
	if not condition:
		_record_failure("%s -- expected true" % what)
	return condition


func not_ok(condition: bool, what: String) -> bool:
	return ok(not condition, what)


func eq(actual, expected, what: String) -> bool:
	assertions += 1
	if actual != expected:
		_record_failure("%s -- expected %s, got %s" % [what, expected, actual])
		return false
	return true


func ne(actual, unexpected, what: String) -> bool:
	assertions += 1
	if actual == unexpected:
		_record_failure("%s -- expected anything but %s" % [what, unexpected])
		return false
	return true


## Float comparison with a tolerance, which is every float comparison here.
##
## The frame arithmetic is the Brain's and is the same source on every platform,
## but it arrives through a `double` -> `float` boundary and there is no reason
## to demand the last bit of it agree across three compilers.
func near(actual: float, expected: float, what: String, eps := 0.0001) -> bool:
	assertions += 1
	if absf(actual - expected) > eps:
		_record_failure("%s -- expected %f +/- %f, got %f" % [what, expected, eps, actual])
		return false
	return true


func gt(actual, floor_value, what: String) -> bool:
	assertions += 1
	if not (actual > floor_value):
		_record_failure("%s -- expected greater than %s, got %s" % [what, floor_value, actual])
		return false
	return true


func has(collection, value, what: String) -> bool:
	assertions += 1
	if not (value in collection):
		_record_failure("%s -- %s is not in %s" % [what, value, collection])
		return false
	return true


## Declares this case unrunnable here, with the reason. Not a pass.
func skip(reason: String) -> void:
	skips.append("%s: %s" % [_case, reason])
