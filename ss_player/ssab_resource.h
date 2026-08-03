#pragma once

#include "ss_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/audio_stream.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_format_loader.hpp>
#include <godot_cpp/classes/resource_format_saver.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>
using namespace godot;
#else
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/templates/hash_map.h"
#include "core/variant/dictionary.h"
#include "servers/audio/audio_stream.h"
#endif

#include "format/ssab.h"

class SSABResource : public Resource {
  GDCLASS(SSABResource, Resource);

protected:
  static void _bind_methods();

#ifdef SPRITESTUDIO_GODOT_EXTENSION
  PackedByteArray binary;
#else
  Vector<uint8_t> binary;
#endif

public:
  Error load_from_file(const String &path);
  Error save_to_file(const String &path);
  bool is_valid() const;
  int get_animation_count();
#ifdef SPRITESTUDIO_GODOT_EXTENSION
  PackedStringArray get_animation_names();
  PackedStringArray get_cellmap_names();
  // Cell names inside a cellmap (for the per-part cell override API).
  PackedStringArray get_cell_names(const String &cellmap_name);
#else
  Vector<String> get_animation_names();
  Vector<String> get_cellmap_names();
  Vector<String> get_cell_names(const String &cellmap_name);
#endif

  uint32_t get_cellmap_hash(const String &cellmap_name);

  const ss::format::SsAnimeBinary *get_ss_anime_binary();
  const uint8_t *get_data_ptr();
  int64_t get_data_size();
  // Bumped every time `binary` is replaced. Holders of a zero-copy borrow of
  // get_data_ptr() compare it to spot an in-place reload; the pointer alone
  // won't do, since a same-sized buffer often re-allocates at the same address.
  uint32_t get_generation() const { return _generation; }
  ss::format::AnimationData *find_animation(const String &name);
  ss::format::AnimationData *find_animation_by_hash(uint32_t name_hash);
  String get_parent_dir() const;

  // ---- Audio (sound) resolution -----------------------------------------
  // The SSAB embeds a table of SoundLists (each a table of SoundFiles). An
  // audio event carries the (sound_list_name_hash, sound_name_hash) pair; these
  // helpers resolve that pair against the binary. The actual sound files are
  // copied next to the .ssab at convert time (under e.g. "sound/"), so file_path
  // is relative to get_parent_dir(), mirroring how cellmap textures are loaded.

  // Loads (and caches) the AudioStream for the given sound identity. Returns an
  // invalid Ref when the identity is unknown or the file cannot be loaded.
  Ref<AudioStream> get_sound_stream(uint32_t sound_list_name_hash, uint32_t sound_name_hash);
  // SSAB sound metadata for the given identity (alias / file_path / file_path_hash
  // / time_total, plus the resolved res:// path). Empty Dictionary when unknown.
  // Intended for custom audio backends that bypass AudioStream playback.
  Dictionary get_sound_info(uint32_t sound_list_name_hash, uint32_t sound_name_hash);

#ifndef SPRITESTUDIO_GODOT_EXTENSION
  virtual Error copy_from(const Ref<Resource> &p_resource);
#endif
private:
    String _parent_dir;
    // Cached result of the whole-buffer FlatBuffers verification run by
    // is_valid(): -1 = not verified yet, 0 = invalid, 1 = valid. Reset to -1
    // wherever `binary` is replaced (load_from_file / copy_from).
    mutable int8_t _valid_cache = -1;
    // Bumped alongside `_valid_cache`; see get_generation().
    uint32_t _generation = 0;
    // The actual verification; is_valid() is the cached front-end.
    bool _verify_binary() const;
    // (sound_list_name_hash << 32 | sound_name_hash) -> loaded AudioStream.
    // Populated lazily by get_sound_stream so repeated events share one stream.
    HashMap<uint64_t, Ref<AudioStream>> _sound_cache;

    // Locate the SoundFile for a (list, name) hash pair; nullptr when unknown.
    const ss::format::SoundFile *_find_sound_file(uint32_t sound_list_name_hash,
                                                  uint32_t sound_name_hash);
};

class SSABResourceFormatLoader : public ResourceFormatLoader {
  GDCLASS(SSABResourceFormatLoader, ResourceFormatLoader);

public:
#ifdef SPRITESTUDIO_GODOT_EXTENSION
  static void _bind_methods() {};

  PackedStringArray _get_recognized_extensions() const override;

  bool _handles_type(const StringName &type) const override;

  String _get_resource_type(const String &path) const override;

  Variant _load(const String &path, const String &original_path,
                bool use_sub_threads, int32_t cache_mode) const override;
#else
  Ref<Resource> load(const String &path, const String &original_path,
                     Error *error, bool use_sub_threads, float *progress,
                     CacheMode cache_mode) override;

  void get_recognized_extensions(List<String> *extensions) const override;

  bool handles_type(const String &type) const override;

  String get_resource_type(const String &path) const override;
#endif
};

class SSABResourceFormatSaver : public ResourceFormatSaver {
  GDCLASS(SSABResourceFormatSaver, ResourceFormatSaver);

public:
#ifdef SPRITESTUDIO_GODOT_EXTENSION
  static void _bind_methods() {};

  Error _save(const Ref<Resource> &resource, const String &path,
              uint32_t flags) override;

  bool _recognize(const Ref<Resource> &resource) const override;

  PackedStringArray _get_recognized_extensions(const Ref<Resource> &resource) const override;
#else
  Error save(const Ref<Resource> &resource, const String &path,
             uint32_t flags) override;

  void get_recognized_extensions(const Ref<Resource> &resource,
                                 List<String> *p_extensions) const override;

  bool recognize(const Ref<Resource> &p_resource) const override;
#endif
};
