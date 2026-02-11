
#include "gd_ssab_resource.h"
#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/error_macros.hpp>
#else
#include "core/error/error_list.h"
#include "core/error/error_macros.h"
#include "core/io/file_access.h"
#endif

void GdSsabResource::_bind_methods() {
  ClassDB::bind_method(D_METHOD("load_from_file", "path"), &GdSsabResource::load_from_file);
  ClassDB::bind_method(D_METHOD("save_to_file", "path"), &GdSsabResource::save_to_file);
  ClassDB::bind_method(D_METHOD("get_animation_count"), &GdSsabResource::get_animation_count);
  ClassDB::bind_method(D_METHOD("get_animation_names"), &GdSsabResource::get_animation_names);
}

Error GdSsabResource::load_from_file(const String &path) {
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

Error GdSsabResource::save_to_file(const String &path) {
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

int GdSsabResource::get_animation_count() {
  auto ss_anime_binary = this->get_ss_anime_binary();
  return ss_anime_binary->animations()->size();
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray GdSsabResource::get_animation_names() {
    PackedStringArray vec;
#else
Vector<String> GdSsabResource::get_animation_names() {
    Vector<String> vec;
#endif
    auto ss_anime_binary = this->get_ss_anime_binary();
    auto num  = ss_anime_binary->animations()->size();
    for (int i=0; i < num; i++) {
      auto animation = ss_anime_binary->animations()->Get(i);
      auto name = animation->name();
      vec.push_back(String(name->c_str()));
    }
    return vec;
}

const ss::format::SsAnimeBinary *GdSsabResource::get_ss_anime_binary() {
    return ss::format::GetSsAnimeBinary(this->binary.ptr());
}

const uint8_t *GdSsabResource::get_data_ptr() {
    return this->binary.ptr();
}

int64_t GdSsabResource::get_data_size() {
    return this->binary.size();
}

ss::format::AnimationData *GdSsabResource::find_animation(const String &name) {
    auto ss_anime_binary = this->get_ss_anime_binary();
    auto num  = ss_anime_binary->animations()->size();
    for (int i=0; i < num; i++) {
        auto animation = ss_anime_binary->animations()->Get(i);
        auto anim_name = animation->name();
        if (name == String(anim_name->c_str())) {
            return (ss::format::AnimationData *)animation;
        }
    }
    return nullptr;
}

#ifndef SPRITESTUDIO_GODOT_EXTENSION
Error GdSsabResource::copy_from(const Ref<Resource> &p_resource) {
  auto error = Resource::copy_from(p_resource);
  if (error != OK)
    return error;
  const Ref<GdSsabResource> &ssabFile =
      static_cast<const Ref<GdSsabResource> &>(p_resource);
  this->binary = ssabFile->binary;
  emit_signal(SNAME("ssab_file_changed"));
  return OK;
}
#endif


#ifdef SPRITESTUDIO_GODOT_EXTENSION
Variant GdSsabResourceFormatLoader::_load(const String &path,
                                                 const String &original_path,
                                                 bool use_sub_threads,
                                                 int32_t cache_mode) {
#else
Ref<Resource> GdSsabResourceFormatLoader::load(const String &path,
                                                       const String &original_path,
                                                       Error *error, bool use_sub_threads,
                                                       float *progress,
                                                       CacheMode cache_mode) {
#endif
  Ref<GdSsabResource> ssab_file = memnew(GdSsabResource);
  ssab_file->load_from_file(path);
#ifndef SPRITESTUDIO_GODOT_EXTENSION
  if (error)
    *error = OK;
#endif
  return ssab_file;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray GdSsabResourceFormatLoader::_get_recognized_extensions() {
  PackedStringArray extensions;
  extensions.push_back("ssab");
  return extensions;
}
#else
void GdSsabResourceFormatLoader::get_recognized_extensions(List<String> *extensions) const {
  extensions->push_back("ssab");
}
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
String GdSsabResourceFormatLoader::_get_resource_type(const String &path) {
#else
String GdSsabResourceFormatLoader::get_resource_type(const String &path) const {
#endif
  return path.ends_with(".ssab") ? "GdSsabResource" : "";
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool GdSsabResourceFormatLoader::_handles_type(const StringName &type) {
#else
bool GdSsabResourceFormatLoader::handles_type(const String &type) const {
#endif
  return type == StringName("GdSsabResource") || ClassDB::is_parent_class(type, "GdSsabResource");
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
Error GdSsabResourceFormatSaver::_save(const Ref<Resource> &resource, const String &path, uint32_t flags) {
#else
Error GdSsabResourceFormatSaver::save(const Ref<Resource> &resource, const String &path, uint32_t flags) {
#endif
  Ref<GdSsabResource> res = resource;
  Error error = res->save_to_file(path);
  return error;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray GdSsabResourceFormatSaver::_get_recognized_extensions(const Ref<Resource> &resource) {
  PackedStringArray extensions;
  if (Object::cast_to<GdSsabResource>(*resource)) {
    extensions.push_back("bssab");
  }
  return extensions;
}
#else
void GdSsabResourceFormatSaver::get_recognized_extensions(const Ref<Resource> &resource, List<String> *p_extensions) const {
  if (Object::cast_to<GdSsabResource>(*resource)) {
    p_extensions->push_back("bssab");
  }
}
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool GdSsabResourceFormatSaver::_recognize(const Ref<Resource> &resource) {
#else
bool GdSsabResourceFormatSaver::recognize(const Ref<Resource> &resource) const {
#endif
  return Object::cast_to<GdSsabResource>(*resource) != nullptr;
}
