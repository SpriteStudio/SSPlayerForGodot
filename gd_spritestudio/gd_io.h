/*!
* \file		gd_io.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_IO_H
#define GD_IO_H

#include "gd_macros.h"

#ifdef GD_V4
#include "core/string/ustring.h"
#endif
#ifdef GD_V3
#include "core/ustring.h"
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
