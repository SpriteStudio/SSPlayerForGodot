# 🧩 How the Player Is Put Together

For whoever works on SpriteStudio Player for Godot, or changes it for a project of their own. It
describes the parts by what they do; the [Build Guide](build.md) builds them, uses a changed build
in a project and checks it in Godot, and
[CONTRIBUTING](https://github.com/cri-middleware/SSPlayerForGodot/blob/HEAD/CONTRIBUTING.md) has
the constraints and the test suite.

What every SpriteStudio Player shares — how playback, motion blending, sound keys and part
tracking behave — is written once, in the SDK's
[Player Porting Guide](https://github.com/cri-middleware/SpriteStudio-SDK/blob/HEAD/libs/ssruntime/docs/README.md).

## Layers

All of the player is C++ under `ss_player/`, and one copy of it builds two ways: as a
**GDExtension** over `godot-cpp`, and as a **custom module** compiled into the engine. The
`#ifdef SPRITESTUDIO_GODOT_EXTENSION` blocks hold only the includes and the type conversions that
differ between the two.

**Playback.** `SpriteStudioPlayer2D` is the node: its properties, methods and signals. Under it is
a playback core with no node of its own, which drives `libssruntime` over its C API, walks each
frame's draw batches and draws them through `RenderingServer` on one root canvas item, with one
child core per Instance part; masks, effects and the per-part meshes live there. The canvas-item
shader is assembled from GLSL pieces when a material is made. Sound plays through the player
unless a `SpriteStudioAudioBackend` takes it over. `SpriteStudioPartAttachment2D` is the user's
node that follows a part, and `SpriteStudioButton` a button drawn by the player placed under it.
`SSABResource` and `SSQBResource` load the converted files; one
parse of a `.ssab` is shared by every player bound to it. The runtime libraries sit under
`ss_player/runtime/`, from `download-sdk` or `build-runtime`.

**The editor**, compiled only into editor builds: the **SS Importer** dock (drops, recent files,
Reconvert), the FileSystem dock's right-click menu, the `SSABResource` inspector, the playback
panel shown with a selected player, and dropping a `.ssab` onto the 2D viewport to make a player.
The importer converts a `.sspj` with `libssconverter` and records each output folder's source in
`res://.ssplayer_sources.cfg`.

**Around it:** `misc/spritestudio.gdextension` is the extension's manifest, copied into every
`addons/spritestudio/`; `test_gdextension/` is the headless suite, not a sample; `examples/` holds
the samples, with `dev_gdextension` and `dev_module` as scratch projects; `godot/` and
`godot-cpp/` are cloned beside the repository, not submodules.

## Why It Is Built the Way It Is

Each reason here spans files; the code it concerns carries the detail in its comments.

**The playback core has no node.** The core owns a root canvas item and draws through
`RenderingServer`, and `SpriteStudioPlayer2D` wraps it. An animation with a hundred parts is one
node in the scene tree, and nothing in the core depends on the tree: it needs a parent canvas item
and somewhere to send its events. `SpriteStudioPartAttachment2D` and `SpriteStudioButton` exist
because following a part and taking the pointer are where the user needs a node of their own.

**The button wraps a player rather than drawing one.** Only the pressed node can narrow where the
pointer lands: the viewport asks that node's own `has_point`, and a child cannot take back what its
parent's rect already holds. So the button is the `BaseButton`, and the `SpriteStudioPlayer2D` under
it keeps everything a player needs from its node — the viewport, the on-screen scale, the texture
settings, `self_modulate`, sound, signals — and the editor's playback panel, which work only with
that class. The button reads its state from `get_draw_mode()` when it is drawn, because
`BaseButton` signals neither hover nor disabled, and listens to the player's `item_rect_changed`,
which the player sends when what it draws may have moved: an SSAB, an animation, a flip, an
offset. It drives playback only while the game runs, so a saved scene holds what the inspector
set.

**Masks are a coverage bitmap, not a stencil.** Godot's canvas has no stencil buffer for a plugin,
so each frame the mask writers are drawn into an offscreen bitmap, one bit per writer, and each
masked part's shader reads it back. One bitmap serves a whole instance tree, owned by its top
player. Bits are packed into RGB, 24 to a tile and four tiles at most, so a frame holds 96 writers;
past that, writers are dropped with one warning, and a tree that needs more is restructured rather
than accommodated. The bitmaps come from one pool for the whole process, so video memory follows
the most players masking at once, not the number that ever did.

**Sound is played by the player, through voices it pools.** The runtime only reports that a sound
key was crossed. This player plays it, as the Unity, Web and Ren'Py players do in their engines;
the Flutter, React Native and wgpu players hand the report to the application. Each sound is a
fresh `AudioStreamPlayer`, created on demand as an internal child of the player — absent from the
scene tree and never saved — and returned to a pool when it has played its count, so a busy
animation does not churn nodes. A sound plays to the end even if the animation pauses; the SDK's
porting guide says why that is the rule every Player keeps. Assigning a `SpriteStudioAudioBackend`
turns built-in playback off entirely.

**Some properties are reachable by name but stored nowhere.** `frame_rate`,
`playback_direction` and `playback_style` are handled in `_set` / `_get` and left out of the
property list, so scripts can set them but neither the inspector nor a saved scene carries them. A
value a script sets is gone when the scene is reloaded.

**The source of every output folder is recorded in the project.** `res://.ssplayer_sources.cfg`
maps each output folder to its `.sspj`, which is what lets Reconvert, the FileSystem menu and
`deploy-examples` rebuild a pack without asking where it came from. The dock's recent files are a
different store: the editor's project metadata under `.godot/`, which is not committed, so they
belong to one machine.

**The module build's only guard is that it compiles.** The suite drives the GDExtension, and the
module build is compiled into an engine that would have to be built to test it. That is why the
two builds share one copy of the playback logic and the `#ifdef` blocks hold nothing but includes
and conversions (`run-tests.sh`'s header says the same).
