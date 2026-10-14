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

#include "format/ssqb.h"

class SSQBResource : public Resource {
  GDCLASS(SSQBResource, Resource);

protected:
  static void _bind_methods();

  PackedByteArray binary;

public:
  Error load_from_file(const String &path);
  Error save_to_file(const String &path);
  // The bytes as a storage property, for the in-place reload Resource::copy_from
  // performs -- see SSABResource::set_data.
  void set_data(const PackedByteArray &p_data);
  PackedByteArray get_data() const { return binary; }

  const ss::format::SsSequenceBinary *get_ss_sequence_binary();
  const uint8_t *get_data_ptr();

private:
  // Every replacement of `binary` goes through here; false when the new bytes
  // do not verify, in which case none are kept.
  bool _take_binary(const PackedByteArray &p_bytes);
};

class SSQBResourceFormatLoader : public ResourceFormatLoader {
  GDCLASS(SSQBResourceFormatLoader, ResourceFormatLoader);

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

class SSQBResourceFormatSaver : public ResourceFormatSaver {
  GDCLASS(SSQBResourceFormatSaver, ResourceFormatSaver);

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
