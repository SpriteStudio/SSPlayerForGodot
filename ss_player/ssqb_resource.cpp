
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
  ClassDB::bind_method(D_METHOD("set_data", "data"), &SSQBResource::set_data);
  ClassDB::bind_method(D_METHOD("get_data"), &SSQBResource::get_data);

  ADD_PROPERTY(PropertyInfo(Variant::PACKED_BYTE_ARRAY, "data", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE), "set_data", "get_data");
}

bool SSQBResource::_take_binary(const PackedByteArray &p_bytes) {
  binary = p_bytes;
  // .ssqb is untrusted input; reject a structurally-invalid buffer here before
  // any accessor follows its offsets (see get_ss_sequence_binary).
  ::flatbuffers::Verifier verifier(binary.ptr(), binary.size());
  if (ss::format::VerifySsSequenceBinaryBuffer(verifier)) {
    return true;
  }
  binary.clear();
  return false;
}

Error SSQBResource::load_from_file(const String &path) {
  Error error = OK;
#ifdef SPRITESTUDIO_GODOT_EXTENSION
  PackedByteArray bytes = FileAccess::get_file_as_bytes(path);
  error = FileAccess::get_open_error();
#else
  PackedByteArray bytes = FileAccess::get_file_as_bytes(path, &error);
#endif
  // A file that cannot be read still replaces the bytes -- with none.
  const bool valid = _take_binary(bytes);
  if (error != OK)
    return error;
  return valid ? OK : ERR_INVALID_DATA;
}

void SSQBResource::set_data(const PackedByteArray &p_data) {
  // Empty is a resource that was never loaded, which is not worth a message.
  if (!_take_binary(p_data) && !p_data.is_empty()) {
    ERR_PRINT(vformat("[SS] SSQBResource.data: these %d bytes are not a usable SSQB, so none were kept.", p_data.size()));
  }
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
  if (binary.size() == 0) {
    return nullptr;
  }
  // binary is validated at load time (load_from_file runs the FlatBuffers
  // Verifier and clears the buffer on failure), so this cast is safe.
  return ss::format::GetSsSequenceBinary(this->binary.ptr());
}

const uint8_t *SSQBResource::get_data_ptr() { return this->binary.ptr(); }

#ifdef SPRITESTUDIO_GODOT_EXTENSION
Variant SSQBResourceFormatLoader::_load(const String &path,
                                          const String &original_path,
                                          bool use_sub_threads,
                                          int32_t cache_mode) const {
#else
Ref<Resource> SSQBResourceFormatLoader::load(
    const String &path, const String &original_path, Error *error,
    bool use_sub_threads, float *progress, CacheMode cache_mode) {
#endif
  Ref<SSQBResource> ssqb_file = memnew(SSQBResource);
  Error err = ssqb_file->load_from_file(path);
  if (err != OK) {
#ifdef SPRITESTUDIO_GODOT_EXTENSION
    // _load reports a failure by returning the code: a null resource is taken
    // for a success, and the caller gets OK with nothing loaded.
    return err;
#else
    if (error)
      *error = err;
    return Ref<Resource>();
#endif
  }

#ifndef SPRITESTUDIO_GODOT_EXTENSION
  if (error)
    *error = OK;
#endif
  return ssqb_file;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray SSQBResourceFormatLoader::_get_recognized_extensions() const {
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
String SSQBResourceFormatLoader::_get_resource_type(const String &path) const {
#else
String SSQBResourceFormatLoader::get_resource_type(const String &path) const {
#endif
  return path.ends_with(".ssqb") ? "SSQBResource" : "";
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool SSQBResourceFormatLoader::_handles_type(const StringName &type) const {
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
    const Ref<Resource> &resource) const {
  PackedStringArray extensions;
  if (Object::cast_to<SSQBResource>(*resource)) {
    extensions.push_back("ssqb");
  }
  return extensions;
}
#else
void SSQBResourceFormatSaver::get_recognized_extensions(
    const Ref<Resource> &resource, List<String> *p_extensions) const {
  if (Object::cast_to<SSQBResource>(*resource)) {
    p_extensions->push_back("ssqb");
  }
}
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool SSQBResourceFormatSaver::_recognize(const Ref<Resource> &resource) const {
#else
bool SSQBResourceFormatSaver::recognize(const Ref<Resource> &resource) const {
#endif
  return Object::cast_to<SSQBResource>(*resource) != nullptr;
}
