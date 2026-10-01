# 🔍 Verifying Playback from the Command Line

This page shows how to confirm that an `.ssab` loads and plays **without opening the editor** — in CI, or when a coding assistant is working on your project: it can run commands, but it cannot look at the editor.

There are two checks, and they answer different questions.

| Check | Answers | Needs |
| --- | --- | --- |
| [Numbers](#check-1-numbers) | Did the extension load, does the animation exist, does the playhead move? | Nothing — runs with `--headless` |
| [Pixels](#check-2-pixels) | Does anything actually draw, and what does it look like? | A display — a window opens briefly |

`--headless` installs a dummy renderer, so it produces no pixels at all. Anything about how an animation *looks* needs the second check.

The commands below call the editor binary as `godot`. Use a stock 4.7 editor build: a custom-module build already has the classes compiled in, and loading the extension on top of it registers them twice and aborts.

---

## Before the first run: import the project once

A project Godot has never opened has no `.godot/` folder, and the `.png` files beside an `.ssab` are not usable until the project has been imported. Import it once from the command line:

```bash
[ -f .godot/extension_list.cfg ] || { mkdir -p .godot; echo "res://addons/spritestudio/spritestudio.gdextension" > .godot/extension_list.cfg; }
godot --headless --path . --import
```

The first line matters. Without it Godot discovers the extension in the middle of the import and crashes on the way out (signal 11, exit status 134 with 4.7.2 on macOS); running `--import` a second time then exits 0, but a script or CI job should not depend on a crash. Naming the extension up front makes Godot load it at startup instead. Godot maintains that file itself afterwards, so this is only needed for a project it has never opened.

---

## Check 1: numbers

Save this as `smoke_check.gd` anywhere in the project and edit the two constants at the top.

```gdscript
extends SceneTree

const SSAB_PATH := "res://ssab_generated/Sample/Sample.ssab"
const ANIMATION := "anime_1"

var failures := 0


func check(condition: bool, what: String) -> void:
    print(("ok    " if condition else "FAIL  ") + what)
    if not condition:
        failures += 1


func _initialize() -> void:
    # No extension type is named in this file, so a project that did not load the
    # extension still reaches this check instead of dying in the parser (which
    # exits 0).
    if not ClassDB.class_exists("SpriteStudioPlayer2D"):
        print("FAIL  SpriteStudioPlayer2D is not registered -- the extension did not load")
        quit(2)
        return

    var ssab = load(SSAB_PATH)
    check(ssab != null, "loaded " + SSAB_PATH)
    if ssab == null:
        quit(2)
        return

    var player = ClassDB.instantiate("SpriteStudioPlayer2D")
    player.set_animation_process_mode(2)  # ANIMATION_PROCESS_MANUAL: only advance() moves it
    root.add_child(player)
    player.set_ssab_resource(ssab)

    # get_current_animation() echoes the name it was given, known or not, so the
    # name is checked against the asset's own list instead.
    check(ANIMATION in player.get_animation_names(), "animation '%s' exists" % ANIMATION)
    player.set_animation(ANIMATION)
    check(player.get_total_frames() > 0, "total frames > 0 (%d)" % player.get_total_frames())

    player.set_loop_count(1)
    player.play()
    var before: float = player.get_frame_no()
    player.advance(0.1)
    check(player.get_frame_no() > before, "advance(0.1) moved the playhead (%s -> %s)" % [before, player.get_frame_no()])

    player.free()
    if failures == 0:
        print("SMOKE PASS")
    quit(1 if failures > 0 else 0)
```

Run it, and let the last line decide:

```bash
out=$(godot --headless --path . --quit-after 600 --script res://smoke_check.gd 2>&1); echo "$out"
echo "$out" | grep -q '^SMOKE PASS$' && ! echo "$out" | grep -qE '^(SCRIPT ERROR|ERROR|WARNING):'
```

The exit status of that last line is the verdict: `0` is a pass.

### What can go wrong, and what it looks like

| Output | Meaning |
| --- | --- |
| `SMOKE PASS`, no `ERROR` lines | The extension loaded, the animation exists and the playhead moved. |
| `FAIL  SpriteStudioPlayer2D is not registered` | The extension did not load. See [Troubleshooting](../troubleshooting.md#the-node-itself-is-missing); on a project Godot has never opened, [import it first](#before-the-first-run-import-the-project-once). |
| `FAIL  animation 'x' exists`, with `ERROR: [SS] … has no animation "x"` | The name is not in the pack. See [Troubleshooting](../troubleshooting.md#the-animation-name-does-not-exist). |
| `SCRIPT ERROR: Parse Error: Could not find type "SpriteStudioPlayer2D"` | A script of your own names the extension's classes as types, and the extension is not loaded. |

### Three things that mislead

* **The exit status is not the verdict.** Godot exits `0` when a script fails to parse, which is exactly what happens to a script that names `SpriteStudioPlayer2D` or `SSABResource` as a type in a project where the extension did not load. That is why the script above reaches the extension only through `ClassDB`, and why a pass is the `SMOKE PASS` line rather than a status: a missing line is a failure.
* **`get_current_animation()` is not proof that the animation exists.** After `set_animation("nope")` it returns `"nope"`, while `get_total_frames()` still reports the previous animation and the node draws nothing; the only other sign is the `ERROR: [SS] …` line. Compare the name with `get_animation_names()`, and treat any `ERROR` line in the output as a failure — the command above does.
* **Step it by hand.** `set_animation_process_mode(2)` is `ANIMATION_PROCESS_MANUAL`: nothing moves except your own `advance(delta)` calls, so the result does not depend on how long a frame took. It is written as a number because spelling it `SpriteStudioPlayer2D.ANIMATION_PROCESS_MANUAL` would name the class. `--quit-after 600` is a frame count that only guards against a script that never reaches `quit()`.

---

## Check 2: pixels

This one needs a real renderer, so it runs **without** `--headless` and a window opens briefly. Save it as `snapshot.gd` and edit the constants.

```gdscript
extends SceneTree

const SSAB_PATH := "res://ssab_generated/Sample/Sample.ssab"
const ANIMATION := "anime_1"
const FRAME := 8.0
const OUT := "res://snapshot.png"


func _initialize() -> void:
    var player = ClassDB.instantiate("SpriteStudioPlayer2D")
    player.set_animation_process_mode(2)  # ANIMATION_PROCESS_MANUAL
    root.add_child(player)
    player.set_ssab_resource(load(SSAB_PATH))
    player.set_animation(ANIMATION)
    player.position = Vector2(576, 560)
    # Assigning frame_no is how you show a specific frame.
    player.frame_no = FRAME

    # The frame has not been drawn until this returns; reading the viewport before
    # it gives an image of the clear colour only.
    await RenderingServer.frame_post_draw
    var image := root.get_texture().get_image()
    image.save_png(ProjectSettings.globalize_path(OUT))

    # Anything other than the clear colour means something was drawn.
    var background := image.get_pixel(0, 0)
    var drawn := 0
    for y in range(0, image.get_height(), 2):
        for x in range(0, image.get_width(), 2):
            if image.get_pixel(x, y) != background:
                drawn += 1
    print("snapshot %dx%d, %d sampled pixels differ from the background" % [image.get_width(), image.get_height(), drawn])
    if drawn > 0:
        print("SNAPSHOT PASS")
    quit(0 if drawn > 0 else 1)
```

```bash
out=$(godot --path . --quit-after 600 --script res://snapshot.gd 2>&1); echo "$out"
echo "$out" | grep -q '^SNAPSHOT PASS$' && ! echo "$out" | grep -qE '^(SCRIPT ERROR|ERROR|WARNING):'
```

The image is `snapshot.png` in the project folder, `1152 x 648` — the size the viewport has when a script runs this way. Open it to see the frame. The pixel count is a coarse check that *something* drew: a wrong animation name gives `0`. It does not compare against a reference image, so whether the frame looks right is for whoever reads the PNG.

---

## See also

* [Troubleshooting](../troubleshooting.md) — what an empty or missing animation usually means.
* [SpriteStudioPlayer2D](../api/player.md) — every method used above.
