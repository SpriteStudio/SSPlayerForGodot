/*!
* \file		gd_io.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_IO_H
#define GD_IO_H

#include "gd_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/core/version.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string_name.hpp>
using namespace godot;
#else
#ifdef GD_V4
#include "core/string/ustring.h"
#endif
#ifdef GD_V3
#include "core/ustring.h"
#endif
#endif

class GdIO
{
private :
	GdIO();

public :
	virtual ~GdIO();

	static String loadStringFromFile( const String& strPath );
	static Error saveStringToFile( const String& strPath, const String& str );
	static Variant loadVariantFromFile( const String& strPath );
	static Error saveVariantToFile( const String& strPath, const Variant& val );
};

#endif // GD_IO_H
