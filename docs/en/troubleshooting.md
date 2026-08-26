# 🩺 Troubleshooting

Most failures here look the same from the outside — **nothing on screen** — and are told apart by their causes rather than their symptoms. Check Godot's **Output** panel first: the plugin and the engine both name the path or the resource they could not use.

---

## The node itself is missing

### `SpriteStudioPlayer2D` is not in the "Create New Node" list

The extension did not load. In the **GDExtension** build, check in this order:

1. `res://addons/spritestudio/spritestudio.gdextension` exists — the `addons` folder must be copied to the **project root**, not into a subfolder.
2. The editor is a **Godot 4.7** build. GDExtension is officially supported from 4.7, and a library built against a different `godot-cpp` will not load.
3. `addons/spritestudio/bin/<platform>/` contains a library for the platform you are running. The Output panel names the file it tried to open.

In the **custom module** build there is no `addons` folder — the node exists only if the module was compiled into the editor binary you are running.

### It works in the editor but the exported game has no node

Export needs the **same target** you export with: `--export-debug` uses the `template_debug` library and `--export-release` the `template_release` one. If only an `editor` build exists, the export still "succeeds" and ships an extension with no library behind it. See [Build Guide](setup/build.md).

---

## The node is there but nothing draws

### No animation is selected

Assigning the `.ssab` is not enough. Pick a name in the **Animation** dropdown (or call `set_animation()`), then `play()` — or turn **Autoplay** on.

### The `.ssab` is invalid

`SSAB Error: Assigned resource is invalid (missing parts or animations).` in the Output panel means the binary loaded but carries no parts or no animations. Reconvert the `.sspj`; the file is most likely truncated or from an incompatible converter version.

### The textures are gone

Textures and sounds are resolved **relative to the directory the `.ssab` was loaded from**. Moving the `.ssab` out of the converter's output directory — without the `.png` files that were copied next to it — leaves the animation with nothing to sample. Godot logs the path it failed to load.

Keep the output directory (`res://ssab_generated` by default) intact, or move the whole directory.

### The animation is off-screen or scaled to nothing

`SpriteStudioPlayer2D` draws in SpriteStudio's pixel coordinates around the node's origin. An animation authored around a far-from-origin point can land outside the camera. Use **Offset** to shift the drawing without moving the node.

---

## A part is missing, everything else draws

- **Text parts and 9-slice parts are not drawn yet.** They keep their slot in the draw order, so the animation plays with a hole in it. See [Limitations & Scope](limitations.md).
- **A part hidden on this frame** is hidden by the animation, not by the plugin. Check the timeline in SpriteStudio, and check that no [visibility override](workflow/usage_scripting.md#part-overrides-color-cell-visibility) is left over from a script.
- **A part inside an Instance part** cannot be addressed by name from the outside — the child animation runs as a separate player. The Instance part itself can.

---

## Playback does not behave

### An override does not appear on screen

While playback is stopped or paused — or on any frame that does not advance — the drawing is not rebuilt, so setting or clearing an override changes nothing visible. Force a redraw with `set_frame_no(get_frame_no())`.

### `animation_finished` never fires

Under an infinite loop (`loop_count` of `-1`, or `0`) it never fires by design. `animation_looped` fires on every wrap instead.

### Events are skipped after a seek

Seeking fires only the destination frame's events. This is a [shared runtime constraint](limitations.md#playback-feature-constraints), not something the player can work around — drive logic off the frame number if you need to survive seeks.

### An Instance or Effect part looks wrong when reversing

`independent=true` parts evaluate correctly under **forward playback only**. A reverse direction and ping-pong return legs are not supported for them. (A negative `speed_scale` is not a third case — it stops the playhead rather than reversing it.)

---

## No sound

Work down the list — each item silences audio on its own:

1. **The playback direction is backward.** Reverse playback fires no audio at all, including the return leg of ping-pong and a negative `speed_scale`.
2. **`play_audio` is off**, or an **`audio_backend` is assigned** and its `play_audio()` does not reach your audio stack. Assigning a backend always suppresses the built-in player.
3. **The sound file did not load.** Godot logs the path. `.wav` / `.ogg` import as `AudioStream`; a format Godot does not import resolves to `null` and is silently skipped.
4. **`audio_volume` is 0**, or the game's audio bus is muted.

Details in [Audio Playback](workflow/audio.md).

---

## The editor integration does not work

### "Open SSPJ" does nothing

[OPTPiX SpriteStudio 7](https://www.webtech.co.jp/spritestudio/) must be installed on the machine you are working on, and `.sspj` must be associated with it — the button hands the file to the OS shell.

### "Reconvert" cannot find the source

The link from `.ssab` back to `.sspj` lives as a **relative path** in `.ssplayer_sources.cfg` at the project root. If the `.sspj` moved, right-click the `.ssab` in the FileSystem dock and choose **Reconvert**; you are prompted for the new location, and every other file in the same directory is re-linked from that one answer.

Track `.ssplayer_sources.cfg` in version control so the whole team gets working buttons after a clone.

### The preview does not animate in the editor

Select the node and use the **SpriteStudio** bottom panel — play from start / play from current / stop, plus a frame scrubber and loop and speed controls. Playback in the editor is driven from there, not from `Autoplay`.

---

## Web export

- **"GDExtension libraries are not supported by this engine version."** — the stock Web templates cannot load GDExtension. Supply an engine template built with `dlink_enabled=yes` and enable **Extensions Support** in the export preset, or use the **custom module** build, which needs neither. See [Exporting Your Project → Web](workflow/export.md#web).
- **The page loads but the animation never appears** — the runtime is built with WebAssembly SIMD and does not run on browsers without it. Treat SIMD as a distribution requirement.
- **COOP/COEP headers** are *not* needed: the Web build is single-threaded (`nothread`) on purpose, so a plain static server is enough.

---

## Still stuck

- [Limitations & Scope](limitations.md) — the constraints that are expected behavior rather than bugs.
- [Shared Limitations](https://cri-middleware.github.io/SpriteStudio-Docs/sdk/limitations/) — the ones that apply to every official player.
- [Report an issue](https://github.com/cri-middleware/SSPlayerForGodot/issues) — include the Godot version, the build variant (GDExtension / custom module), the platform, and the Output panel text.
