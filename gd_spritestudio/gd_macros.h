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
    #define	GD_V4			//!< バージョン4.xのgodotが使用されています。
    #define EMPTY(x) ((x).is_empty())
    #define VARIANT_FLOAT Variant::FLOAT
    #define NOTIFY_PROPERTY_LIST_CHANGED() notify_property_list_changed()
  #elif VERSION_MAJOR>=3
    #define	GD_V3			//!< バージョン3.xのgodotが使用されています。
    #define EMPTY(x) ((x).empty())
    #define VARIANT_FLOAT Variant::REAL
    #define NOTIFY_PROPERTY_LIST_CHANGED() property_list_changed_notify()
  #else
    #error not supported godot version.
  #endif
#endif


/*!
* 次の関数はObject派生クラスでオーバーライドできます。
* これらの関数は仮想ではありません。仮想にしないでください。
* オーバーライドのたびに呼び出され、前の関数は無効になりません(多重レベル呼出し)。
*/
#define	GdMultilevelCall

#define	GdUiText( _a )		_a

#endif // GD_MACROS_H
