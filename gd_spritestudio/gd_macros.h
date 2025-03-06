/*!
* \file		gd_macros.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_MACROS_H
#define GD_MACROS_H

#ifdef SPRITESTUDIO_GODOT_EXTENSION
  #include <godot_cpp/core/version.hpp>
#else
  #include "core/version.h"
  #if VERSION_MAJOR>=4
    #define	GD_V4			//!< バージョン4.xのgodotが使用されています。
  #elif VERSION_MAJOR>=3
    #define	GD_V3			//!< バージョン3.xのgodotが使用されています。
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
