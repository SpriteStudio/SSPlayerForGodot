/*!
* \file		ss_macros.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef SS_MACROS_H
#define SS_MACROS_H

#define SSPLAYER_SOURCES_CFG_PATH "res://.ssplayer_sources.cfg"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
  #include <godot_cpp/core/version.hpp>
  #include <godot_cpp/variant/string_name.hpp>
  // godot-cpp names the API it was generated from (api_version) but not as the
  // engine's hex, which the checks shared with the custom module compare.
  #define GODOT_VERSION_HEX (0x10000 * GODOT_VERSION_MAJOR + 0x100 * GODOT_VERSION_MINOR + GODOT_VERSION_PATCH)
  #define SNAME(x) ([]() -> const godot::StringName & { static const godot::StringName *_ss_sname = new godot::StringName(x); return *_ss_sname; }())
  #define SS_FILE_EXISTS(x) FileAccess::file_exists(x)
  #define RS_VIEWPORT_UPDATE_ONCE RenderingServer::VIEWPORT_UPDATE_ONCE
  #define RS_VIEWPORT_CLEAR_ALWAYS RenderingServer::VIEWPORT_CLEAR_ALWAYS
  #define RS_PRIMITIVE_TRIANGLES RenderingServer::PRIMITIVE_TRIANGLES
  #define RS_CANVAS_ITEM_TEXTURE_FILTER RenderingServer::CanvasItemTextureFilter
  #define RS_CANVAS_ITEM_TEXTURE_REPEAT RenderingServer::CanvasItemTextureRepeat
  namespace godot { class ResourceLoader; class ResourceSaver; }
  using SsResourceLoader = godot::ResourceLoader;
  using SsResourceSaver = godot::ResourceSaver;
#else
  #include "core/version.h"
  #include "core/string/string_name.h"
  #include "core/object/class_db.h"
  #include "core/object/callable_mp.h"
  #if GODOT_VERSION_HEX < 0x040700
    #error The custom module needs Godot 4.7 or later.
  #endif
  #ifndef SNAME
    #define SNAME(x) StringName(x)
  #endif
  #define SS_FILE_EXISTS(x) FileAccess::exists(x)
  #define RS_VIEWPORT_UPDATE_ONCE RSE::VIEWPORT_UPDATE_ONCE
  #define RS_VIEWPORT_CLEAR_ALWAYS RSE::VIEWPORT_CLEAR_ALWAYS
  #define RS_PRIMITIVE_TRIANGLES RSE::PRIMITIVE_TRIANGLES
  #define RS_CANVAS_ITEM_TEXTURE_FILTER RSE::CanvasItemTextureFilter
  #define RS_CANVAS_ITEM_TEXTURE_REPEAT RSE::CanvasItemTextureRepeat
  // The engine's own ResourceLoader and ResourceSaver are static. CoreBind's
  // are the singletons that scripts and godot-cpp call, so going through them
  // gives both build shapes the same calls. Callers include core/core_bind.h.
  namespace CoreBind { class ResourceLoader; class ResourceSaver; }
  using SsResourceLoader = CoreBind::ResourceLoader;
  using SsResourceSaver = CoreBind::ResourceSaver;
#endif

#endif // SS_MACROS_H