#pragma once

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
using namespace godot;
#else
#include "core/io/resource.h"
#include "core/variant/dictionary.h"
#include "scene/main/node.h"
#endif

#include "ssab_resource.h"

// Override hook for audio playback. By default SpriteStudioPlayer2D performs
// built-in AudioStreamPlayer one-shot playback, but to route audio events to a
// custom backend (e.g. audio middleware) assign a subclass of this resource to
// the player's "audio_backend" property.
//
// Subclass it (in GDScript or C++) and implement `play_audio`. The default
// implementation is a no-op — assigning a backend always suppresses built-in
// playback, so the backend fully owns play-count / lifecycle management. It is
// invoked only for forward playback (reverse audio is a documented limitation).
//
// GDScript example:
//   extends SpriteStudioAudioBackend
//   func play_audio(payload: Dictionary, ssab: SSABResource, player: Node) -> void:
//       var info := ssab.get_sound_info(payload["sound_list_name_hash"], payload["sound_name_hash"])
//       # ... hand info["path"] / payload["loop_num"] to your middleware ...
class SpriteStudioAudioBackend : public Resource {
    GDCLASS(SpriteStudioAudioBackend, Resource);

protected:
    static void _bind_methods();

public:
    // Plays one audio event. `payload` is the same Dictionary emitted by the
    // player's "audio" signal (part_index / sound_list_name_hash /
    // sound_name_hash / sound_name / loop_num). `ssab` resolves sound metadata
    // via get_sound_info / get_sound_stream; `player` is the originating node.
    virtual void play_audio(const Dictionary &payload, const Ref<SSABResource> &ssab, Node *player);
};
