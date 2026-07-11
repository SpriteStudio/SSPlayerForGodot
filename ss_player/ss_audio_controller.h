#pragma once

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string.hpp>
using namespace godot;
#else
#include "core/templates/hash_map.h"
#include "core/templates/vector.h"
#include "core/variant/dictionary.h"
#include "core/string/ustring.h"
#include "scene/audio/audio_stream_player.h"
#include "servers/audio/audio_stream.h"
#endif

#include "ss_audio_backend.h"
#include "ssab_resource.h"

// Built-in audio playback helper embedded in SpriteStudioPlayer2D (a plain C++
// class, not a Node). Turns one audio event into an AudioStreamPlayer voice,
// mirroring the SpriteStudioForUnity SpriteStudioAudioController design.
//
// Playback follows the ssruntime Player porting doc (40_components/90_audio):
// audio is fired-and-forgotten at the moment its frame is passed. There is no
// seek-offset sync, and playback is NOT coupled to animation pause/stop — once a
// sound starts it plays to completion (a paused animation keeps its sounds
// going). The caller applies the forward-playback filter (reverse audio is a
// documented limitation).
//
// Resolution: if a backend is supplied, the event is handed to it and built-in
// playback is skipped (the backend owns lifecycle). Otherwise built-in playback
// is fire-and-forget: every audio event starts a *fresh* voice and never stops
// a voice already playing, so a sound re-fired while a previous instance is
// still sounding (e.g. across an animation loop) overlaps rather than cutting
// it off. `loop_num` is the play *count* (SpriteStudio has no infinite loop):
//   * 1       : one-shot.
//   * >= 2    : play N times.
//   * <= 0    : cannot occur per spec; defensively not played.
// AudioStreamPlayer nodes are added to the owner on demand and reused from a
// pool once a voice finishes all its repetitions.
class SsAudioController {
public:
    explicit SsAudioController(Node *p_owner) : _owner(p_owner) {}
    ~SsAudioController();

    SsAudioController(const SsAudioController &) = delete;
    SsAudioController &operator=(const SsAudioController &) = delete;

    // Plays one audio event. `payload` is the Dictionary emitted by SsInternalPlayer
    // (part_index / sound_list_name_hash / sound_name_hash / sound_name / loop_num).
    // `backend` is optional (nullptr => built-in playback). `volume` is linear [0,1].
    void play(const Dictionary &payload, const Ref<SSABResource> &ssab,
              SpriteStudioAudioBackend *backend, float volume);

    // Per-frame maintenance: replays finite-loop voices across their play edges
    // and reclaims finished voices back to the pool. Cheap no-op when idle.
    void tick();

    // Stops every voice and returns its player to the pool. Used only for
    // resource cleanup (node leaves the tree / built-in playback disabled /
    // destroy) — never as a reaction to animation pause / stop / switch.
    void stop_all();

private:
    struct Voice {
        AudioStreamPlayer *player = nullptr;
        int loop_num = 1;
        int plays_done = 0;
    };

    Node *_owner;
    Vector<Voice> _voices;             // active voices (one entry per in-flight sound)
    Vector<AudioStreamPlayer *> _pool; // idle players available for reuse

    static void _start_voice(Voice &v, const Ref<AudioStream> &stream, float volume, int loop_num);

    AudioStreamPlayer *_rent();
    void _free(AudioStreamPlayer *player);
};
