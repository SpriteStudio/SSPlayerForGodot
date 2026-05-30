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
  #define SNAME(x) godot::StringName(x)
  #define EMPTY(x) ((x).is_empty())
  #define VARIANT_FLOAT Variant::FLOAT
  #define NOTIFY_PROPERTY_LIST_CHANGED() notify_property_list_changed()
  #define SS_FILE_EXISTS(x) FileAccess::file_exists(x)
#else
  #include "core/version.h"
  #include "core/string/string_name.h"
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