#include "ss_audio_controller.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#else
#include "core/math/math_funcs.h"
#include "core/variant/array.h"
#endif

// Linear [0,1] gain -> decibels for AudioStreamPlayer::set_volume_db. Silence
// (v <= 0) maps to a large negative dB the mixer treats as muted.
static float _volume_to_db(float v) {
    if (v <= 0.0f) {
        return -80.0f;
    }
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    return (float)UtilityFunctions::linear_to_db(v);
#else
    return Math::linear_to_db(v);
#endif
}

SsAudioController::~SsAudioController() {
    // The AudioStreamPlayer nodes are children of _owner and are freed by the
    // owner's node destruction; nothing to release here.
}

void SsAudioController::_start_voice(Voice &v, const Ref<AudioStream> &stream, float volume, int loop_num) {
    AudioStreamPlayer *src = v.player;
    src->stop();
    src->set_stream(stream);
    src->set_volume_db(_volume_to_db(volume));
    src->play();
    v.loop_num = loop_num;
    v.plays_done = 1;   // first repetition now playing
}

void SsAudioController::play(const Dictionary &payload, const Ref<SSABResource> &ssab,
                            SpriteStudioAudioBackend *backend, float volume) {
    // Override hook: hand the event off and skip built-in playback entirely
    // (the backend owns play-count / lifecycle). Dispatched via callv so a
    // GDScript subclass overriding play_audio is invoked.
    if (backend != nullptr) {
        Array args;
        args.push_back(payload);
        args.push_back(ssab);
        args.push_back(_owner);
        backend->callv("play_audio", args);
        return;
    }

    // loop_num is the play count; <= 0 cannot occur per spec -> do not play.
    int loop_num = (int)payload.get("loop_num", 1);
    if (loop_num <= 0) {
        return;
    }
    if (ssab.is_null()) {
        return;
    }

    uint32_t list_hash = (uint32_t)(int64_t)payload.get("sound_list_name_hash", 0);
    uint32_t name_hash = (uint32_t)(int64_t)payload.get("sound_name_hash", 0);

    Ref<AudioStream> stream = ssab->get_sound_stream(list_hash, name_hash);
    if (stream.is_null()) {
        return;   // unknown / unsupported sound -> safely silent
    }
    if (_owner == nullptr || !_owner->is_inside_tree()) {
        return;
    }

    // Fire-and-forget: always start a fresh voice, never stop one already
    // playing. A sound re-fired while a prior instance still sounds (e.g. an
    // animation loop re-passing the same frame) overlaps rather than cutting off.
    Voice v;
    v.player = _rent();
    if (v.player == nullptr) {
        return;
    }
    _start_voice(v, stream, volume, loop_num);
    _voices.push_back(v);
}

void SsAudioController::tick() {
    // Reverse walk so reclaiming a finished voice via remove_at doesn't shift
    // indices we haven't visited yet.
    for (int i = _voices.size() - 1; i >= 0; i--) {
        Voice &v = _voices.ptrw()[i];
        if (v.player == nullptr) {
            _voices.remove_at(i);
            continue;
        }
        if (v.player->is_playing()) {
            continue;   // current repetition still running
        }
        if (v.plays_done < v.loop_num) {
            v.player->play();   // start the next repetition
            v.plays_done++;
        } else {
            _free(v.player);            // all repetitions done -> back to pool
            _voices.remove_at(i);
        }
    }
}

void SsAudioController::stop_all() {
    for (int i = 0; i < _voices.size(); i++) {
        _free(_voices.ptrw()[i].player);
    }
    _voices.clear();
}

AudioStreamPlayer *SsAudioController::_rent() {
    if (!_pool.is_empty()) {
        AudioStreamPlayer *src = _pool[_pool.size() - 1];
        _pool.remove_at(_pool.size() - 1);
        return src;
    }
    if (_owner == nullptr || !_owner->is_inside_tree()) {
        return nullptr;
    }
    AudioStreamPlayer *src = memnew(AudioStreamPlayer);
    // Internal child: kept out of get_children() and never persisted with the scene.
    _owner->add_child(src, false, Node::INTERNAL_MODE_BACK);
    return src;
}

void SsAudioController::_free(AudioStreamPlayer *player) {
    if (player == nullptr) {
        return;
    }
    player->stop();
    player->set_stream(Ref<AudioStream>());
    _pool.push_back(player);
}
