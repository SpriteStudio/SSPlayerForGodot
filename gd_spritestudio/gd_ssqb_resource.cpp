
#include "gd_ssqb_resource.h"
#include "runtime/ssqb.h"
#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/error_macros.hpp>
#else
#include "core/error/error_list.h"
#include "core/error/error_macros.h"
#include "core/io/file_access.h"
#endif

void GdSsqbResource::_bind_methods() {
  ClassDB::bind_method(D_METHOD("load_from_file", "path"), &GdSsqbResource::load_from_file);
  ClassDB::bind_method(D_METHOD("save_to_file", "path"), &GdSsqbResource::save_to_file);
}

Error GdSsqbResource::load_from_file(const String &path) {
  Error error = OK;
#ifdef SPRITESTUDIO_GODOT_EXTENSION
  binary = FileAccess::get_file_as_bytes(path);
  if (binary.size() == 0)
    return ERR_INVALID_DATA;
#else
  binary = FileAccess::get_file_as_bytes(path, &error);
  if (error != OK)
    return error;
#endif

  return error;

  // return ERR_FILE_UNRECOGNIZED;
}

Error GdSsqbResource::save_to_file(const String &path) {
  Error error;
#ifdef SPRITESTUDIO_GODOT_EXTENSION
  Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
  if (!file.is_valid())
    return ERR_FILE_CANT_OPEN;
#else
  Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE, &error);
  if (error != OK)
    return error;
#endif
  file->store_buffer(binary.ptr(), binary.size());
  file->flush();
  return OK;
}

const ss::format::SsSequenceBinary *GdSsqbResource::get_ss_sequence_binary() {
  return ss::format::GetSsSequenceBinary(this->binary.ptr());
}

const uint8_t *GdSsqbResource::get_data_ptr() { return this->binary.ptr(); }

#ifndef SPRITESTUDIO_GODOT_EXTENSION
Error GdSsqbResource::copy_from(const Ref<Resource> &p_resource) {
  auto error = Resource::copy_from(p_resource);
  if (error != OK)
    return error;
  const Ref<GdSsqbResource> &ssqbFile =
      static_cast<const Ref<GdSsqbResource> &>(p_resource);
  this->binary = ssqbFile->binary;
  emit_signal(SNAME("ssqb_file_changed"));
  return OK;
}
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
Variant GdSsqbResourceFormatLoader::_load(const String &path,
                                          const String &original_path,
                                          bool use_sub_threads,
                                          int32_t cache_mode) {
#else
Ref<Resource> GdSsqbResourceFormatLoader::load(
    const String &path, const String &original_path, Error *error,
    bool use_sub_threads, float *progress, CacheMode cache_mode) {
#endif
  Ref<GdSsqbResource> ssqb_file = memnew(GdSsqbResource);
  ssqb_file->load_from_file(path);
#ifndef SPRITESTUDIO_GODOT_EXTENSION
  if (error)
    *error = OK;
#endif
  return ssqb_file;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray GdSsqbResourceFormatLoader::_get_recognized_extensions() {
  PackedStringArray extensions;
  extensions.push_back("ssqb");
  return extensions;
}
#else
void GdSsqbResourceFormatLoader::get_recognized_extensions(
    List<String> *extensions) const {
  extensions->push_back("ssqb");
}
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
String GdSsqbResourceFormatLoader::_get_resource_type(const String &path) {
#else
String GdSsqbResourceFormatLoader::get_resource_type(const String &path) const {
#endif
  return path.ends_with(".ssba") ? "GdSsqbResource" : "";
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool GdSsqbResourceFormatLoader::_handles_type(const StringName &type) {
#else
bool GdSsqbResourceFormatLoader::handles_type(const String &type) const {
#endif
  return type == StringName("GdSsqbResource") ||
         ClassDB::is_parent_class(type, "GdSsqbResource");
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
Error GdSsqbResourceFormatSaver::_save(const Ref<Resource> &resource,
                                       const String &path, uint32_t flags) {
#else
Error GdSsqbResourceFormatSaver::save(const Ref<Resource> &resource,
                                      const String &path, uint32_t flags) {
#endif
  Ref<GdSsqbResource> res = resource;
  Error error = res->save_to_file(path);
  return error;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray GdSsqbResourceFormatSaver::_get_recognized_extensions(
    const Ref<Resource> &resource) {
  PackedStringArray extensions;
  if (Object::cast_to<GdSsqbResource>(*resource)) {
    extensions.push_back("bssqb");
  }
  return extensions;
}
#else
void GdSsqbResourceFormatSaver::get_recognized_extensions(
    const Ref<Resource> &resource, List<String> *p_extensions) const {
  if (Object::cast_to<GdSsqbResource>(*resource)) {
    p_extensions->push_back("bssqb");
  }
}
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool GdSsqbResourceFormatSaver::_recognize(const Ref<Resource> &resource) {
#else
bool GdSsqbResourceFormatSaver::recognize(const Ref<Resource> &resource) const {
#endif
  return Object::cast_to<GdSsqbResource>(*resource) != nullptr;
}
