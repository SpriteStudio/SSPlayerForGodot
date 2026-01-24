#pragma once

#include "gd_macros.h"

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

#include "runtime/ssab.h"

class GdResourceSsabResource : public Resource {
  GDCLASS(GdResourceSsabResource, Resource);

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
  int get_animation_count();
#ifdef SPRITESTUDIO_GODOT_EXTENSION
  PackedStringArray get_animation_names();
#else
  Vector<String> get_animation_names();
#endif

  const ss::format::SsAnimeBinary *get_ss_anime_binary();
  const uint8_t *get_data_ptr();

#ifndef SPRITESTUDIO_GODOT_EXTENSION
  virtual Error copy_from(const Ref<Resource> &p_resource);
#endif
};

class GdResourceSsabResourceFormatLoader : public ResourceFormatLoader {
  GDCLASS(GdResourceSsabResourceFormatLoader, ResourceFormatLoader);

public:
#ifdef SPRITESTUDIO_GODOT_EXTENSION
  static void _bind_methods() {};

  PackedStringArray _get_recognized_extensions();

  bool _handles_type(const StringName &type);

  String _get_resource_type(const String &path);

  Variant _load(const String &path, const String &original_path,
                bool use_sub_threads, int32_t cache_mode);
#else
  Ref<Resource> load(const String &path, const String &original_path,
                     Error *error, bool use_sub_threads, float *progress,
                     CacheMode cache_mode) override;

  void get_recognized_extensions(List<String> *extensions) const override;

  bool handles_type(const String &type) const override;

  String get_resource_type(const String &path) const override;
#endif
};

class GdResourceSsabResourceFormatSaver : public ResourceFormatSaver {
  GDCLASS(GdResourceSsabResourceFormatSaver, ResourceFormatSaver);

public:
#ifdef SPRITESTUDIO_GODOT_EXTENSION
  static void _bind_methods() {};

  Error _save(const Ref<Resource> &resource, const String &path,
              uint32_t flags) override;

  bool _recognize(const Ref<Resource> &resource);

  PackedStringArray _get_recognized_extensions(const Ref<Resource> &resource);
#else
  Error save(const Ref<Resource> &resource, const String &path,
             uint32_t flags) override;

  void get_recognized_extensions(const Ref<Resource> &resource,
                                 List<String> *p_extensions) const override;

  bool recognize(const Ref<Resource> &p_resource) const override;
#endif
};
