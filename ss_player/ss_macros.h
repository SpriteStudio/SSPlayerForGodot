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
  #define SNAME(x) ([]() -> const godot::StringName & { static const godot::StringName *_ss_sname = new godot::StringName(x); return *_ss_sname; }())
  #define EMPTY(x) ((x).is_empty())
  #define VARIANT_FLOAT Variant::FLOAT
  #define NOTIFY_PROPERTY_LIST_CHANGED() notify_property_list_changed()
  #define SS_FILE_EXISTS(x) FileAccess::file_exists(x)
  #define RS_VIEWPORT_UPDATE_ONCE RenderingServer::VIEWPORT_UPDATE_ONCE
  #define RS_VIEWPORT_CLEAR_ALWAYS RenderingServer::VIEWPORT_CLEAR_ALWAYS
  #define RS_PRIMITIVE_TRIANGLES RenderingServer::PRIMITIVE_TRIANGLES
#else
  #include "core/version.h"
  #include "core/string/string_name.h"
  #include "core/object/class_db.h"
  #include "core/object/callable_mp.h"
  #define RS_VIEWPORT_UPDATE_ONCE RSE::VIEWPORT_UPDATE_ONCE
  #define RS_VIEWPORT_CLEAR_ALWAYS RSE::VIEWPORT_CLEAR_ALWAYS
  #define RS_PRIMITIVE_TRIANGLES RSE::PRIMITIVE_TRIANGLES
  #if VERSION_MAJOR>=4
    #define	GD_V4
    #ifndef SNAME
      #define SNAME(x) StringName(x)
    #endif
    #define EMPTY(x) ((x).is_empty())
    #define VARIANT_FLOAT Variant::FLOAT
    #define NOTIFY_PROPERTY_LIST_CHANGED() notify_property_list_changed()
    #define SS_FILE_EXISTS(x) FileAccess::exists(x)
  #else
    #error not supported godot version.
  #endif
#endif

#endif // SS_MACROS_H