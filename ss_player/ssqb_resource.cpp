
#include "ssqb_resource.h"
#include "format/ssqb.h"
#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/error_macros.hpp>
#else
#include "core/error/error_list.h"
#include "core/error/error_macros.h"
#include "core/io/file_access.h"
#endif

void SSQBResource::_bind_methods() {
  ClassDB::bind_method(D_METHOD("load_from_file", "path"), &SSQBResource::load_from_file);
  ClassDB::bind_method(D_METHOD("save_to_file", "path"), &SSQBResource::save_to_file);
}

Error SSQBResource::load_from_file(const String &path) {
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

Error SSQBResource::save_to_file(const String &path) {
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

const ss::format::SsSequenceBinary *SSQBResource::get_ss_sequence_binary() {
  return ss::format::GetSsSequenceBinary(this->binary.ptr());
}

const uint8_t *SSQBResource::get_data_ptr() { return this->binary.ptr(); }

#ifndef SPRITESTUDIO_GODOT_EXTENSION
Error SSQBResource::copy_from(const Ref<Resource> &p_resource) {
  auto error = Resource::copy_from(p_resource);
  if (error != OK)
    return error;
  const Ref<SSQBResource> &ssqbFile =
      static_cast<const Ref<SSQBResource> &>(p_resource);
  this->binary = ssqbFile->binary;
  emit_signal(SNAME("ssqb_file_changed"));
  return OK;
}
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
Variant SSQBResourceFormatLoader::_load(const String &path,
                                          const String &original_path,
                                          bool use_sub_threads,
                                          int32_t cache_mode) {
#else
Ref<Resource> SSQBResourceFormatLoader::load(
    const String &path, const String &original_path, Error *error,
    bool use_sub_threads, float *progress, CacheMode cache_mode) {
#endif
  Ref<SSQBResource> ssqb_file = memnew(SSQBResource);
  ssqb_file->load_from_file(path);
#ifndef SPRITESTUDIO_GODOT_EXTENSION
  if (error)
    *error = OK;
#endif
  return ssqb_file;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray SSQBResourceFormatLoader::_get_recognized_extensions() {
  PackedStringArray extensions;
  extensions.push_back("ssqb");
  return extensions;
}
#else
void SSQBResourceFormatLoader::get_recognized_extensions(
    List<String> *extensions) const {
  extensions->push_back("ssqb");
}
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
String SSQBResourceFormatLoader::_get_resource_type(const String &path) {
#else
String SSQBResourceFormatLoader::get_resource_type(const String &path) const {
#endif
  return path.ends_with(".ssqb") ? "SSQBResource" : "";
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool SSQBResourceFormatLoader::_handles_type(const StringName &type) {
#else
bool SSQBResourceFormatLoader::handles_type(const String &type) const {
#endif
  return type == StringName("SSQBResource") ||
         ClassDB::is_parent_class(type, "SSQBResource");
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
Error SSQBResourceFormatSaver::_save(const Ref<Resource> &resource,
                                       const String &path, uint32_t flags) {
#else
Error SSQBResourceFormatSaver::save(const Ref<Resource> &resource,
                                      const String &path, uint32_t flags) {
#endif
  Ref<SSQBResource> res = resource;
  Error error = res->save_to_file(path);
  return error;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray SSQBResourceFormatSaver::_get_recognized_extensions(
    const Ref<Resource> &resource) {
  PackedStringArray extensions;
  if (Object::cast_to<SSQBResource>(*resource)) {
    extensions.push_back("bssqb");
  }
  return extensions;
}
#else
void SSQBResourceFormatSaver::get_recognized_extensions(
    const Ref<Resource> &resource, List<String> *p_extensions) const {
  if (Object::cast_to<SSQBResource>(*resource)) {
    p_extensions->push_back("bssqb");
  }
}
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool SSQBResourceFormatSaver::_recognize(const Ref<Resource> &resource) {
#else
bool SSQBResourceFormatSaver::recognize(const Ref<Resource> &resource) const {
#endif
  return Object::cast_to<SSQBResource>(*resource) != nullptr;
}
