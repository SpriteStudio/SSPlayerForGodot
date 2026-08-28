## Headless test runner for the SpriteStudio GDExtension.
##
## Run through `scripts/run-tests.{sh,ps1}`, which finds a Godot binary and
## points it here. By hand:
##
##     <godot> --headless --path test_gdextension --script res://run_tests.gd
##
## Scope, and why it is what it is. This exercises the **GDExtension** build,
## which is what a user drops into their project, and it exercises it through
## GDScript — the same door a user's game goes through. It therefore covers the
## bound API and the signals, and it does **not** cover drawing: `--headless`
## installs a dummy rasteriser, so pixels are not a thing that exists here. That
## is not a gap this runner should try to close; `NOTIFICATION_DRAW` is a no-op
## in `SpriteStudioPlayer2D` anyway, because the InternalPlayer issues its own
## RenderingServer calls.
##
## The custom-module build is not tested here and cannot be: a module is
## compiled into the engine, so testing it would mean building Godot rather than
## downloading it. What the two builds share is the playback logic — one copy —
## and what they do not share is a layer of `#ifdef SPRITESTUDIO_GODOT_EXTENSION`
## adapters, which are includes and type conversions. Those are what a compiler
## checks, so the module build's own guard is that it still builds.
##
## Exit status: 0 all passed - 1 a case failed - 2 preflight failed.
extends SceneTree

const SUITE_DIR := "res://suites"

## Fixture packs `deploy-examples` produces. Preflight refuses to start without
## them rather than letting every suite skip its way to a green run.
const REQUIRED_PACKS := {
	"res://ssab_generated/overall/Basic.ssab": "overall",
	"res://ssab_generated/Ringo/Ringo.ssab": "Ringo",
}

## Classes the extension registers. Absent means it did not load, which is the
## one failure worth telling apart from every other.
const REQUIRED_CLASSES := ["SpriteStudioPlayer2D", "SSABResource"]


func _init() -> void:
	var problems := _preflight()
	if not problems.is_empty():
		print("run_tests.gd: the suite cannot run yet.\n")
		for problem in problems:
			print("  !!  %s" % problem)
		print("")
		_finish(2)
		return

	var suites := _discover()
	if suites.is_empty():
		print("run_tests.gd: no suite found under %s" % SUITE_DIR)
		_finish(2)
		return

	var only := _only_filter()
	var total_cases := 0
	var total_assertions := 0
	var failures: Array[String] = []
	var skips: Array[String] = []

	print("== headless suite: %d files (%s) ==\n" % [suites.size(), Engine.get_version_info().string])

	for path in suites:
		var suite = load(path).new()
		suite.root = root
		var name := path.get_file().get_basename()
		var cases := _cases_of(suite)
		if not only.is_empty():
			cases = cases.filter(func(c): return only.any(func(o): return o in c or o in name))
		if cases.is_empty():
			continue

		var before_failures: int = suite.failures.size()
		for case in cases:
			suite.begin_case(case)
			suite.setup()
			suite.call(case)
			suite.teardown()
			suite.release_owned()
			total_cases += 1

		total_assertions += suite.assertions
		failures.append_array(suite.failures)
		skips.append_array(suite.skips)

		var failed: int = suite.failures.size() - before_failures
		var verdict := "FAIL" if failed > 0 else "PASS"
		var note := ""
		if not suite.skips.is_empty():
			note = "  %d skipped" % suite.skips.size()
		print("  %-4s  %-28s %2d cases, %3d assertions%s"
			% [verdict, name, cases.size(), suite.assertions, note])

	if not skips.is_empty():
		print("\n-- declared skips (not passes) --")
		for skip in skips:
			print("  SKIP  %s" % skip)

	if not failures.is_empty():
		print("\n-- failures --")
		for failure in failures:
			print("  FAIL  %s" % failure)

	print("\n== RESULT: %d cases, %d assertions, %d failed, %d skipped =="
		% [total_cases, total_assertions, failures.size(), skips.size()])
	_finish(1 if not failures.is_empty() else 0)


## Prints the marker `run-tests.*` looks for, then exits.
##
## The exit code alone is not enough to trust. A script error inside a case
## aborts `_init` before anything below it runs, and the tree then idles forever
## — so the wrapper passes `--quit-after`, which makes Godot exit 0 on the way
## out and turns a crashed run into a green one. The marker is what tells a run
## that finished apart from one that stopped in the middle.
func _finish(code: int) -> void:
	print("==== SUITE FINISHED ====")
	quit(code)


## The reasons the suite cannot run — empty when it can.
func _preflight() -> Array[String]:
	var problems: Array[String] = []

	var missing_classes := REQUIRED_CLASSES.filter(func(c): return not ClassDB.class_exists(c))
	if not missing_classes.is_empty():
		problems.append(
			"the extension did not load: %s not registered\n"
			% ", ".join(missing_classes)
			+ "      -> build it (scripts/build-extension.sh) and check\n"
			+ "         test_gdextension/addons/spritestudio/ has a binary for this platform")

	var missing_packs: Array[String] = []
	for path in REQUIRED_PACKS:
		if not ResourceLoader.exists(path) and not FileAccess.file_exists(path):
			missing_packs.append(REQUIRED_PACKS[path])
	if not missing_packs.is_empty():
		problems.append(
			"playback data is not deployed: %s\n" % ", ".join(missing_packs)
			+ "      -> scripts/deploy-examples.sh")

	return problems


## Suite scripts, in a stable order.
func _discover() -> Array[String]:
	var found: Array[String] = []
	var dir := DirAccess.open(SUITE_DIR)
	if dir == null:
		return found
	for file in dir.get_files():
		# Exported projects rename .gd to .gd.remap; this only ever runs from
		# source, but reading the basename keeps it honest either way.
		if file.ends_with(".gd"):
			found.append("%s/%s" % [SUITE_DIR, file])
	found.sort()
	return found


## `test_*` methods declared by the suite itself, not by test_base.gd.
func _cases_of(suite) -> Array[String]:
	var cases: Array[String] = []
	for method in suite.get_method_list():
		var name: String = method["name"]
		if name.begins_with("test_") and not cases.has(name):
			cases.append(name)
	cases.sort()
	return cases


## `--only=<substring>[,<substring>]`, passed through by run-tests.{sh,ps1}.
func _only_filter() -> Array[String]:
	for arg in OS.get_cmdline_user_args():
		if arg.begins_with("--only="):
			return Array(arg.trim_prefix("--only=").split(",", false))
	return []
