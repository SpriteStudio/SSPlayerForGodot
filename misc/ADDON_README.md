# SpriteStudio Player for Godot

Plays SpriteStudio animations in Godot 4.7: the `SpriteStudioPlayer2D` node, the `SSABResource` and `SSQBResource` resources, and an editor dock that converts `.sspj` projects to `.ssab`.

This folder is the whole add-on: `spritestudio.gdextension`, the native libraries in `bin/`, the editor icons, the licences, and the documentation in `docs/`.

`docs/` is the documentation of the release this folder came from, so it describes the API the binaries in `bin/` actually provide. The online documentation is built from the latest release and can be ahead of this copy.

## Where to look

| To find out | Read |
| --- | --- |
| What a method, property or signal does | [docs/api/player.md](docs/api/player.md), [docs/api/resource.md](docs/api/resource.md) |
| How to control playback and react to events from GDScript | [docs/workflow/usage_scripting.md](docs/workflow/usage_scripting.md) |
| How to check from the command line that an animation loads and plays | [docs/workflow/verify_playback.md](docs/workflow/verify_playback.md) |
| Why nothing draws, or why the node is missing | [docs/troubleshooting.md](docs/troubleshooting.md) |
| What is not supported | [docs/limitations.md](docs/limitations.md) |

The other pages in `docs/` cover installation, importing `.sspj` projects, audio, exporting and performance. Links between the pages work offline. The screenshots and videos those pages refer to are not included; they are in the online documentation, <https://cri-middleware.github.io/SSPlayerForGodot/> (Japanese: <https://cri-middleware.github.io/SSPlayerForGodot/ja/>).

The licence is `LICENSE.md`; third-party notices for the bundled libraries are in `licenses/`.
