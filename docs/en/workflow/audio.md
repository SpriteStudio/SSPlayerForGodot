# 🔊 Audio Playback

Audio parts authored in SpriteStudio play through Godot **without any setup**: assign the `.ssab`, press play, and the sounds on the timeline sound. This page covers the three ways to take that over — turning it off, adjusting it, or replacing it with your own audio stack.

> [!NOTE]
> Audio is the one place where this player does more than the shared runtime. `libssruntime` only *reports* that an audio key was crossed; the sibling players hand that report to the application. `SpriteStudioPlayer2D` also ships a built-in player for it, because Godot already has everything needed to make the sound.

---

## How it works out of the box

The converter copies the sound files referenced by the project next to the generated `.ssab`, so they are ordinary Godot resources by the time you play. When the playhead crosses an audio key, the player resolves the referenced file through the bound `SSABResource` and starts an `AudioStreamPlayer` for it.

The voices are created on demand as **internal** children of the `SpriteStudioPlayer2D` node — they do not appear in the scene tree and are never saved with the scene — and they are pooled, so a finished voice is reused rather than freed and a busy animation does not churn nodes. `audio_volume` is a linear value converted to decibels on each voice.

Three properties control it, all on `SpriteStudioPlayer2D`:

| Property | Type | Default | Description |
| --- | --- | --- | --- |
| `Play Audio` (`play_audio`) | bool | `true` | Whether the built-in player makes sound. Turn it off to handle audio entirely yourself through the `audio` signal |
| `Audio Volume` (`audio_volume`) | float | `1.0` | Linear volume in `[0, 1]` applied to the built-in voices. Ignored when a backend is assigned |
| `Audio Backend` (`audio_backend`) | `SpriteStudioAudioBackend` | *(none)* | Replaces the built-in player. See [Routing audio elsewhere](#routing-audio-elsewhere) |

```gdscript
@onready var ss_player: SpriteStudioPlayer2D = $SpriteStudioPlayer2D

func _ready() -> void:
    ss_player.set_audio_volume(0.4)      # quieter
    # ss_player.set_play_audio(false)    # or silence the built-in player entirely
```

> [!TIP]
> The built-in playback runs in the **editor preview** as well, so scrubbing or previewing a cut with the SpriteStudio bottom panel is audible without launching the game.

---

## Playback semantics

Audio is **fire-and-forget**: a sound starts at the moment its frame is passed, and from then on it is an ordinary `AudioStreamPlayer` voice that runs to completion. That has consequences worth knowing before you build timing-critical audio on it.

- **Forward playback only.** Nothing sounds while the effective direction is backward — a reversed direction, the return leg of ping-pong, or a negative speed scale. This is a [shared limitation](../limitations.md#playback-feature-constraints), not a Godot one.
- **Seeking does not replay what it skipped.** Jumping the playhead fires only the destination frame's events, and a sound already playing is not re-synced to the new position.
- **Pausing the animation does not pause the sound.** `pause()` and `stop()` stop the *animation*; voices already sounding play out. Turning `play_audio` off does stop them, as does the node leaving the tree.
- **Re-firing overlaps rather than cuts.** A sound triggered again while a previous instance is still audible — across a loop boundary, typically — starts a second voice. Nothing cuts the first one off.
- **`loop_num` is a play count, not a flag.** SpriteStudio has no infinite audio loop: `1` plays once, `n` plays `n` times in a row.

> [!NOTE]
> If you need sound that pauses with the game, ducks, or crossfades, drive it yourself — either from the `audio` signal or from a [backend](#routing-audio-elsewhere). The built-in player deliberately implements the simple case exactly.

---

## Observing audio events

The `audio` signal is an **observation channel** and fires independently of everything above: in every playback direction, in the editor, and whether or not `play_audio` is on. Use it to react to a sound (a lip-flap, a screen shake) without taking over playback, or to replace playback entirely after setting `play_audio` to `false`.

```gdscript
func _ready() -> void:
    ss_player.set_play_audio(false)          # we will do it ourselves
    ss_player.audio.connect(_on_audio)

func _on_audio(payload: Dictionary) -> void:
    var ssab: SSABResource = ss_player.get_ssab_resource()
    var stream := ssab.get_sound_stream(payload["sound_list_name_hash"], payload["sound_name_hash"])
    if stream == null:
        return
    var voice := AudioStreamPlayer.new()
    add_child(voice)
    voice.stream = stream
    voice.finished.connect(voice.queue_free)
    voice.play()
```

| Payload key | Type | Meaning |
| --- | --- | --- |
| `part_index` | `int` | Index of the audio part that fired |
| `part_name` | `String` | Name of that part |
| `frame_no` | `int` | The frame the key sits on — not necessarily the frame it was noticed on, since one tick can step across several |
| `sound_list_name_hash` | `int` | Hash of the sound list name |
| `sound_name_hash` | `int` | Hash of the sound name |
| `sound_name` | `String` | Sound name as authored (present only when set) |
| `loop_num` | `int` | Play count |

The two hashes are the address of the sound inside the `.ssab`. Resolve them through the resource:

* [`get_sound_stream(sound_list_name_hash, sound_name_hash) -> AudioStream`](../api/resource.md#ssabresource) — the loaded stream, or `null` when the file is missing or unsupported. Results are cached per resource, including the misses.
* [`get_sound_info(sound_list_name_hash, sound_name_hash) -> Dictionary`](../api/resource.md#ssabresource) — metadata without loading anything: `alias`, `file_path`, `path`, `file_path_hash`, `time_total`.

---

## Routing audio elsewhere

To send sounds to audio middleware, to a bus layout of your own, or to a pooling scheme the built-in player does not implement, subclass **`SpriteStudioAudioBackend`** and assign it to the node's `Audio Backend` property.

```gdscript
# res://audio/my_backend.gd
extends SpriteStudioAudioBackend

func play_audio(payload: Dictionary, ssab: SSABResource, player: Node) -> void:
    var info := ssab.get_sound_info(payload["sound_list_name_hash"], payload["sound_name_hash"])
    if info.is_empty():
        return
    # info["path"] is the resolved res:// path; info["alias"] is the name authored in SpriteStudio.
    MyMiddleware.play(info["path"], payload["loop_num"])
```

Assign it in the inspector (drag the script onto `Audio Backend`, or save it as a `.tres`). One backend resource can serve any number of players — the node that fired the event arrives as `player`.

> [!IMPORTANT]
> **Assigning a backend always suppresses the built-in playback**, `audio_volume` included. The backend owns voice lifecycle and play counts completely; there is no partial hand-off. A backend that returns without doing anything for an event silences that event.

`play_audio` still gates the call, and the forward-only rule still applies — a backend is not invoked while playing backwards.

---

## Related

- [Scripting and Event-Driven Control](usage_scripting.md) — the other timeline events (`user_data`, `signal_emitted`) and the rest of the scripting API.
- [SpriteStudioPlayer2D API](../api/player.md#audio) — the method-level reference for the properties on this page.
- [Limitations & Scope](../limitations.md) — the constraints audio inherits from the shared runtime.
