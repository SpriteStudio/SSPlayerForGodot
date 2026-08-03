
#include "ssab_resource.h"
#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/error_macros.hpp>
#else
#include "core/error/error_list.h"
#include "core/error/error_macros.h"
#include "core/io/file_access.h"
#endif

void SSABResource::_bind_methods() {
  ClassDB::bind_method(D_METHOD("load_from_file", "path"), &SSABResource::load_from_file);
  ClassDB::bind_method(D_METHOD("save_to_file", "path"), &SSABResource::save_to_file);
  ClassDB::bind_method(D_METHOD("is_valid"), &SSABResource::is_valid);
  ClassDB::bind_method(D_METHOD("get_animation_count"), &SSABResource::get_animation_count);
  ClassDB::bind_method(D_METHOD("get_animation_names"), &SSABResource::get_animation_names);
  ClassDB::bind_method(D_METHOD("get_cellmap_names"), &SSABResource::get_cellmap_names);
  ClassDB::bind_method(D_METHOD("get_cell_names", "cellmap_name"), &SSABResource::get_cell_names);
  ClassDB::bind_method(D_METHOD("get_sound_stream", "sound_list_name_hash", "sound_name_hash"), &SSABResource::get_sound_stream);
  ClassDB::bind_method(D_METHOD("get_sound_info", "sound_list_name_hash", "sound_name_hash"), &SSABResource::get_sound_info);
  }
bool SSABResource::is_valid() const {
  // Verification walks the whole buffer, and nearly every accessor below gates
  // on it (selecting a player in the editor alone triggers several, and each
  // Instance child does one per animation lookup), so a multi-megabyte SSAB
  // would be re-verified many times per operation. The buffer is immutable
  // between load_from_file / copy_from, both of which reset the cache.
  if (_valid_cache < 0) {
    _valid_cache = _verify_binary() ? 1 : 0;
  }
  return _valid_cache != 0;
}

bool SSABResource::_verify_binary() const {
  if (binary.size() == 0) {
    return false;
  }

  ::flatbuffers::Verifier verifier(binary.ptr(), binary.size());
  if (!ss::format::VerifySsAnimeBinaryBuffer(verifier)) {
    return false;
  }

  const ss::format::SsAnimeBinary *ssab = ss::format::GetSsAnimeBinary(binary.ptr());
  if (ssab->parts() == nullptr || ssab->parts()->size() == 0) {
    return false;
  }

  if (ssab->animations() == nullptr || ssab->animations()->size() == 0) {
    return false;
  }

  // The runtime indexes parts() positionally over each animation's
  // parts_animation_data; parsing unchecked under panic=abort, a list longer
  // than parts() aborts the process at resource-create time. Reject that here
  // (a shorter list never indexes out of range, so only '>').
  const uint32_t parts_size = ssab->parts()->size();
  const auto *animations = ssab->animations();
  for (uint32_t a = 0; a < animations->size(); a++) {
    const auto *pad = animations->Get(a)->parts_animation_data();
    if (pad && pad->size() > parts_size) {
      return false;
    }
  }

  return true;
}

Error SSABResource::load_from_file(const String &path) {
  Error error = OK;
  _parent_dir = path.get_base_dir();
  // New buffer: drop the cached verification verdict before touching `binary`.
  // The generation is bumped up-front so the failure paths below, which leave
  // `binary` reassigned or cleared, invalidate outstanding borrows too.
  _valid_cache = -1;
  _generation++;
#ifdef SPRITESTUDIO_GODOT_EXTENSION
  binary = FileAccess::get_file_as_bytes(path);
  if (binary.size() == 0) {
    return ERR_INVALID_DATA;
  }
#else
  binary = FileAccess::get_file_as_bytes(path, &error);
  if (error != OK) {
    return error;
  }
#endif

  if (!is_valid()) {
    binary.clear();
    _valid_cache = -1;
    return ERR_INVALID_DATA;
  }

  return OK;
}

Error SSABResource::save_to_file(const String &path) {
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

int SSABResource::get_animation_count() {
  if (!is_valid()) {
    return 0;
  }
  auto ss_anime_binary = this->get_ss_anime_binary();
  return ss_anime_binary->animations()->size();
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray SSABResource::get_animation_names() {
    PackedStringArray vec;
#else
Vector<String> SSABResource::get_animation_names() {
    Vector<String> vec;
#endif
    if (!is_valid()) {
        return vec;
    }
    auto ss_anime_binary = this->get_ss_anime_binary();
    auto num  = ss_anime_binary->animations()->size();
    for (int i=0; i < num; i++) {
      auto animation = ss_anime_binary->animations()->Get(i);
      auto name = animation->name();
      vec.push_back(String::utf8(name->c_str()));
    }
    return vec;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray SSABResource::get_cellmap_names() {
    PackedStringArray vec;
#else
Vector<String> SSABResource::get_cellmap_names() {
    Vector<String> vec;
#endif
    if (!is_valid()) {
        return vec;
    }
    auto a = get_ss_anime_binary();
    if (a->cellmaps() != nullptr) {
        for (int i = 0; i < a->cellmaps()->size(); i++) {
            auto cellmap = a->cellmaps()->Get(i);
            vec.push_back(String::utf8(cellmap->name()->c_str()));
        }
    }
    if (a->external_textures() != nullptr) {
        for (int i = 0; i < a->external_textures()->size(); i++) {
            auto etexture = a->external_textures()->Get(i);
            vec.push_back(String::utf8(etexture->name()->c_str()));
        }
    }
    return vec;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray SSABResource::get_cell_names(const String &cellmap_name) {
    PackedStringArray vec;
#else
Vector<String> SSABResource::get_cell_names(const String &cellmap_name) {
    Vector<String> vec;
#endif
    if (!is_valid()) {
        return vec;
    }
    auto a = get_ss_anime_binary();
    if (a->cellmaps() != nullptr) {
        for (int i = 0; i < a->cellmaps()->size(); i++) {
            auto cellmap = a->cellmaps()->Get(i);
            if (cellmap_name != String::utf8(cellmap->name()->c_str())) {
                continue;
            }
            if (cellmap->cells() != nullptr) {
                for (int j = 0; j < cellmap->cells()->size(); j++) {
                    auto cell = cellmap->cells()->Get(j);
                    vec.push_back(String::utf8(cell->name()->c_str()));
                }
            }
            break;
        }
    }
    return vec;
}

uint32_t SSABResource::get_cellmap_hash(const String &cellmap_name) {
    if (!is_valid()) {
        return 0;
    }
    auto a = get_ss_anime_binary();
    if (a->cellmaps() != nullptr) {
        for (int i = 0; i < a->cellmaps()->size(); i++) {
            auto cellmap = a->cellmaps()->Get(i);
            if (cellmap_name == String::utf8(cellmap->name()->c_str())) {
                return cellmap->name_hash();
            }
        }
    }
    if (a->external_textures() != nullptr) {
        for (int i = 0; i < a->external_textures()->size(); i++) {
            auto etexture = a->external_textures()->Get(i);
            if (cellmap_name == String::utf8(etexture->name()->c_str())) {
                return etexture->name_hash();
            }
        }
    }
    return 0;
}

const ss::format::SsAnimeBinary *SSABResource::get_ss_anime_binary() {
    if (binary.size() == 0) {
        return nullptr;
    }
    return ss::format::GetSsAnimeBinary(this->binary.ptr());
}

const uint8_t *SSABResource::get_data_ptr() {
    return this->binary.ptr();
}

int64_t SSABResource::get_data_size() {
    return this->binary.size();
}

ss::format::AnimationData *SSABResource::find_animation(const String &name) {
    if (!is_valid()) {
        return nullptr;
    }
    auto ss_anime_binary = this->get_ss_anime_binary();
    auto num  = ss_anime_binary->animations()->size();
    for (int i=0; i < num; i++) {
        auto animation = ss_anime_binary->animations()->Get(i);
        auto anim_name = animation->name();
        if (name == String::utf8(anim_name->c_str())) {
            return (ss::format::AnimationData *)animation;
        }
    }
    return nullptr;
}

ss::format::AnimationData *SSABResource::find_animation_by_hash(uint32_t name_hash) {
    if (!is_valid()) {
        return nullptr;
    }
    auto ss_anime_binary = this->get_ss_anime_binary();
    auto num  = ss_anime_binary->animations()->size();
    for (int i=0; i < num; i++) {
        auto animation = ss_anime_binary->animations()->Get(i);
        if (animation && animation->name_hash() == name_hash) {
            return (ss::format::AnimationData *)animation;
        }
    }
    return nullptr;
}

String SSABResource::get_parent_dir() const {
    return this->_parent_dir;
}

const ss::format::SoundFile *SSABResource::_find_sound_file(uint32_t sound_list_name_hash,
                                                           uint32_t sound_name_hash) {
    if (!is_valid()) {
        return nullptr;
    }
    auto a = get_ss_anime_binary();
    auto sound_lists = a->sound_lists();
    if (sound_lists == nullptr) {
        return nullptr;
    }
    for (uint32_t i = 0; i < sound_lists->size(); i++) {
        auto sound_list = sound_lists->Get(i);
        if (!sound_list || sound_list->name_hash() != sound_list_name_hash) {
            continue;
        }
        auto files = sound_list->table_data();
        if (files == nullptr) {
            return nullptr;
        }
        for (uint32_t j = 0; j < files->size(); j++) {
            auto file = files->Get(j);
            if (file && file->name_hash() == sound_name_hash) {
                return file;
            }
        }
        return nullptr;
    }
    return nullptr;
}

Ref<AudioStream> SSABResource::get_sound_stream(uint32_t sound_list_name_hash, uint32_t sound_name_hash) {
    uint64_t key = ((uint64_t)sound_list_name_hash << 32) | (uint64_t)sound_name_hash;
    if (_sound_cache.has(key)) {
        return _sound_cache[key];
    }

    Ref<AudioStream> stream;
    const ss::format::SoundFile *file = _find_sound_file(sound_list_name_hash, sound_name_hash);
    if (file != nullptr && file->file_path() != nullptr) {
        String path = _parent_dir.path_join(String::utf8(file->file_path()->c_str()));
#ifdef SPRITESTUDIO_GODOT_EXTENSION
        Ref<Resource> res = ResourceLoader::get_singleton()->load(path, "AudioStream", ResourceLoader::CACHE_MODE_REUSE);
#else
        Ref<Resource> res = ResourceLoader::load(path, "AudioStream", ResourceFormatLoader::CACHE_MODE_REUSE, nullptr);
#endif
        stream = Ref<AudioStream>(res);
    }

    // Cache even a null result so a missing/unsupported file is not re-loaded on
    // every event; the identity cannot change for a given binary.
    _sound_cache[key] = stream;
    return stream;
}

Dictionary SSABResource::get_sound_info(uint32_t sound_list_name_hash, uint32_t sound_name_hash) {
    Dictionary info;
    const ss::format::SoundFile *file = _find_sound_file(sound_list_name_hash, sound_name_hash);
    if (file == nullptr) {
        return info;
    }
    if (file->name()) {
        info["alias"] = String::utf8(file->name()->c_str());
    }
    if (file->file_path()) {
        String rel = String::utf8(file->file_path()->c_str());
        info["file_path"] = rel;
        info["path"] = _parent_dir.path_join(rel);
    }
    info["file_path_hash"] = (int64_t)file->file_path_hash();
    info["time_total"] = (int64_t)file->time_total();
    return info;
}

#ifndef SPRITESTUDIO_GODOT_EXTENSION
Error SSABResource::copy_from(const Ref<Resource> &p_resource) {
  auto error = Resource::copy_from(p_resource);
  if (error != OK)
    return error;
  const Ref<SSABResource> &ssabFile =
      static_cast<const Ref<SSABResource> &>(p_resource);
  this->binary = ssabFile->binary;
  _valid_cache = -1;
  // This path emits no "changed", so the generation is the only signal a
  // borrower gets that its buffer was replaced.
  _generation++;
  return OK;
}
#endif


#ifdef SPRITESTUDIO_GODOT_EXTENSION
Variant SSABResourceFormatLoader::_load(const String &path,
                                                 const String &original_path,
                                                 bool use_sub_threads,
                                                 int32_t cache_mode) const {
#else
Ref<Resource> SSABResourceFormatLoader::load(const String &path,
                                                       const String &original_path,
                                                       Error *error, bool use_sub_threads,
                                                       float *progress,
                                                       CacheMode cache_mode) {
#endif
  Ref<SSABResource> ssab_file = memnew(SSABResource);
  Error err = ssab_file->load_from_file(path);
  if (err != OK) {
#ifndef SPRITESTUDIO_GODOT_EXTENSION
    if (error)
      *error = err;
#endif
    return Ref<Resource>();
  }

#ifndef SPRITESTUDIO_GODOT_EXTENSION
  if (error)
    *error = OK;
#endif
  return ssab_file;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray SSABResourceFormatLoader::_get_recognized_extensions() const {
  PackedStringArray extensions;
  extensions.push_back("ssab");
  return extensions;
}
#else
void SSABResourceFormatLoader::get_recognized_extensions(List<String> *extensions) const {
  extensions->push_back("ssab");
}
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
String SSABResourceFormatLoader::_get_resource_type(const String &path) const {
#else
String SSABResourceFormatLoader::get_resource_type(const String &path) const {
#endif
  return path.ends_with(".ssab") ? "SSABResource" : "";
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool SSABResourceFormatLoader::_handles_type(const StringName &type) const {
#else
bool SSABResourceFormatLoader::handles_type(const String &type) const {
#endif
  return type == StringName("SSABResource") || ClassDB::is_parent_class(type, "SSABResource");
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
Error SSABResourceFormatSaver::_save(const Ref<Resource> &resource, const String &path, uint32_t flags) {
#else
Error SSABResourceFormatSaver::save(const Ref<Resource> &resource, const String &path, uint32_t flags) {
#endif
  Ref<SSABResource> res = resource;
  Error error = res->save_to_file(path);
  return error;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray SSABResourceFormatSaver::_get_recognized_extensions(const Ref<Resource> &resource) const {
  PackedStringArray extensions;
  if (Object::cast_to<SSABResource>(*resource)) {
    extensions.push_back("ssab");
  }
  return extensions;
}
#else
void SSABResourceFormatSaver::get_recognized_extensions(const Ref<Resource> &resource, List<String> *p_extensions) const {
  if (Object::cast_to<SSABResource>(*resource)) {
    p_extensions->push_back("ssab");
  }
}
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool SSABResourceFormatSaver::_recognize(const Ref<Resource> &resource) const {
#else
bool SSABResourceFormatSaver::recognize(const Ref<Resource> &resource) const {
#endif
  return Object::cast_to<SSABResource>(*resource) != nullptr;
}
