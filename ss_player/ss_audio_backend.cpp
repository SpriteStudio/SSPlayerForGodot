#include "ss_audio_backend.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/core/class_db.hpp>
#else
#include "core/object/class_db.h"
#endif

void SpriteStudioAudioBackend::_bind_methods() {
    // Bound (not GDVIRTUAL) so a GDScript subclass that defines `play_audio`
    // transparently overrides it: the player dispatches through Object::callv,
    // which resolves to the script method when present and to this no-op default
    // otherwise. This keeps the module and GDExtension builds identical.
    ClassDB::bind_method(
        D_METHOD("play_audio", "payload", "ssab", "player"),
        &SpriteStudioAudioBackend::play_audio);
}

void SpriteStudioAudioBackend::play_audio(const Dictionary &payload, const Ref<SSABResource> &ssab, Node *player) {
    // Default: do nothing. Subclasses override to route to a custom backend.
}
