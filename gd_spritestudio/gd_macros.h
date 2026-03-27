/*!
* \file		gd_macros.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_MACROS_H
#define GD_MACROS_H

#ifdef SPRITESTUDIO_GODOT_EXTENSION
  #include <godot_cpp/core/version.hpp>
  #define EMPTY(x) ((x).is_empty())
  #define VARIANT_FLOAT Variant::FLOAT
  #define NOTIFY_PROPERTY_LIST_CHANGED() notify_property_list_changed()
#else
  #include "core/version.h"
  #if VERSION_MAJOR>=4
    #define	GD_V4
    #define EMPTY(x) ((x).is_empty())
    #define VARIANT_FLOAT Variant::FLOAT
    #define NOTIFY_PROPERTY_LIST_CHANGED() notify_property_list_changed()
  #else
    #error not supported godot version.
  #endif
#endif

#endif // GD_MACROS_H