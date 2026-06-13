#pragma once

#include "ss_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_format_loader.hpp>
#include <godot_cpp/classes/resource_format_saver.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/templates/vector.hpp>
using namespace godot;
#else
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
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
#else
  Vector<String> get_animation_names();
  Vector<String> get_cellmap_names();
#endif

  uint32_t get_cellmap_hash(const String &cellmap_name);

  const ss::format::SsAnimeBinary *get_ss_anime_binary();
  const uint8_t *get_data_ptr();
  int64_t get_data_size();
  ss::format::AnimationData *find_animation(const String &name);
  ss::format::AnimationData *find_animation_by_hash(uint32_t name_hash);
  String get_parent_dir() const;

#ifndef SPRITESTUDIO_GODOT_EXTENSION
  virtual Error copy_from(const Ref<Resource> &p_resource);
#endif
private:
    String _parent_dir;
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
