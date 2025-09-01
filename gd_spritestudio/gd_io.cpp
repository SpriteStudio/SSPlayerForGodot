/*!
* \file		gd_io.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_io.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/file_access.hpp>
using namespace godot;
#else
#ifdef GD_V4
#include "core/io/file_access.h"
#endif
#ifdef GD_V3
#include "core/os/file_access.h"
#endif
#include "core/io/stream_peer.h"
#endif

GdIO::GdIO()
{
}

GdIO::~GdIO()
{
}

String GdIO::loadStringFromFile( const String& strPath )
{
	Error				err;
	String				str = String( "" );

#ifdef SPRITESTUDIO_GODOT_EXTENSION
    str = FileAccess::get_file_as_string(strPath);
	if (str.length() == 0) {
		err = ERR_FILE_UNRECOGNIZED;
		ERR_PRINT( String( "loadStringFromFile error: " ) + String::num( err ) );

		return str;
	}

#endif
#ifdef GD_V4
	Ref<FileAccess>		resFileAccess = FileAccess::open( strPath, FileAccess::READ, &err );

	if ( err != OK ) {
		ERR_PRINT( String( "loadStringFromFile error: " ) + String::num( err ) );

		if ( resFileAccess.is_valid() ) {
			resFileAccess->close();
		}

		return	str;
	}

	while ( !resFileAccess->eof_reached() ) {
		str += resFileAccess->get_line();
	}

	resFileAccess->close();
#endif
#ifdef GD_V3
	FileAccess*			pFileAccess = FileAccess::open( strPath, FileAccess::READ, &err );

	if ( err != OK ) {
		ERR_PRINT( String( "loadStringFromFile error: " ) + String::num( err ) );

		if ( pFileAccess ) {
			pFileAccess->close();
		}

		return	str;
	}

	str = pFileAccess->get_as_utf8_string();

	pFileAccess->close();
#endif

	return	str;
}

Error GdIO::saveStringToFile( const String& strPath, const String& str )
{
	Error				err;

#ifdef SPRITESTUDIO_GODOT_EXTENSION
	Ref<FileAccess>		resFileAccess = FileAccess::open( strPath, FileAccess::WRITE);
	if ( !resFileAccess.is_valid() ) {
		err = ERR_FILE_CANT_OPEN;
		ERR_PRINT( String( "saveStringToFile error: " ) + String::num( err ) );

		return err;
	}

	resFileAccess->store_string( str );

	resFileAccess->close();

#endif
#ifdef GD_V4
	Ref<FileAccess>		resFileAccess = FileAccess::open( strPath, FileAccess::WRITE, &err );

	if ( err != OK ) {
		ERR_PRINT( String( "saveStringToFile error: " ) + String::num( err ) );

		if ( resFileAccess.is_valid() ) {
			resFileAccess->close();
		}

		return	err;
	}

	resFileAccess->store_string( str );

	resFileAccess->close();
#endif
#ifdef GD_V3
	FileAccess*			pFileAccess = FileAccess::open( strPath, FileAccess::WRITE, &err );

	if ( err != OK ) {
		ERR_PRINT( String( "saveStringToFile error: " ) + String::num( err ) );

		if ( pFileAccess ) {
			pFileAccess->close();
		}

		return	err;
	}

	pFileAccess->store_string( str );

	pFileAccess->close();
#endif

	return	err;
}

Variant GdIO::loadVariantFromFile( const String& strPath )
{
	Error				err;
	Variant				val = Variant( "" );

#ifdef SPRITESTUDIO_GODOT_EXTENSION
	Ref<FileAccess>		resFileAccess = FileAccess::open( strPath, FileAccess::READ);
	if ( !resFileAccess.is_valid() ) {
		err = ERR_FILE_CANT_OPEN;
		ERR_PRINT( String( "loadVariantFromFile error: " ) + String::num( err ) );
		return	val;
	}

	val = resFileAccess->get_var();

	resFileAccess->close();
#else
#ifdef GD_V4
	Ref<FileAccess>		resFileAccess = FileAccess::open( strPath, FileAccess::READ, &err );

	if ( err != OK ) {
		ERR_PRINT( String( "loadVariantFromFile error: " ) + String::num( err ) );

		if ( resFileAccess.is_valid() ) {
			resFileAccess->close();
		}

		return	val;
	}

	int					iSize = resFileAccess->get_length();
	uint8_t*			pBuf = memnew_arr( uint8_t, iSize );

	resFileAccess->get_buffer( pBuf, iSize );

	resFileAccess->close();

	StreamPeerBuffer	st;

	st.put_data( pBuf, iSize );
	st.seek( 0 );

	val = st.get_var();

	memdelete_arr( pBuf );
#endif
#ifdef GD_V3
	FileAccess*			pFileAccess = FileAccess::open( strPath, FileAccess::READ, &err );

	if ( err != OK ) {
		ERR_PRINT( String( "loadVariantFromFile error: " ) + String::num( err ) );

		if ( pFileAccess ) {
			pFileAccess->close();
		}

		return	val;
	}

	int					iSize = pFileAccess->get_len();
	uint8_t*			pBuf = memnew_arr( uint8_t, iSize );

	pFileAccess->get_buffer( pBuf, iSize );

	pFileAccess->close();

	StreamPeerBuffer	st;

	st.put_data( pBuf, iSize );
	st.seek( 0 );

	val = st.get_var();

	memdelete_arr( pBuf );
#endif
#endif

	return	val;
}

Error GdIO::saveVariantToFile( const String& strPath, const Variant& val )
{
	Error				err;

#ifdef SPRITESTUDIO_GODOT_EXTENSION
	Ref<FileAccess>		resFileAccess = FileAccess::open( strPath, FileAccess::WRITE);
	if ( !resFileAccess.is_valid() ) {
		err =  ERR_FILE_CANT_OPEN;
		ERR_PRINT( String( "saveVariantToFile error: " ) + String::num( err ) );

		return	err;
	}

	resFileAccess->store_var(val);

#endif
#ifdef GD_V4
	Ref<FileAccess>		resFileAccess = FileAccess::open( strPath, FileAccess::WRITE, &err );

	if ( err != OK ) {
		ERR_PRINT( String( "saveVariantToFile error: " ) + String::num( err ) );

		if ( resFileAccess.is_valid() ) {
			resFileAccess->close();
		}

		return	err;
	}

	StreamPeerBuffer	st;

	st.put_var( val );
	st.seek( 0 );

	int					iSize = st.get_size();
	uint8_t*			pBuf = memnew_arr( uint8_t, iSize );

	st.get_data( pBuf, iSize );

	resFileAccess->store_buffer( pBuf, iSize );

	resFileAccess->close();

	memdelete_arr( pBuf );
#endif
#ifdef GD_V3
	FileAccess*			pFileAccess = FileAccess::open( strPath, FileAccess::WRITE, &err );

	if ( err != OK ) {
		ERR_PRINT( String( "saveVariantToFile error: " ) + String::num( err ) );

		if ( pFileAccess ) {
			pFileAccess->close();
		}

		return	err;
	}

	StreamPeerBuffer	st;

	st.put_var( val );
	st.seek( 0 );

	int					iSize = st.get_size();
	uint8_t*			pBuf = memnew_arr( uint8_t, iSize );

	st.get_data( pBuf, iSize );

	pFileAccess->store_buffer( pBuf, iSize );

	pFileAccess->close();

	memdelete_arr( pBuf );
#endif

	return	err;
}
