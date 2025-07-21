/*!
* \file		gd_loader_sscellmap.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_loader_sscellmap.h"

#include "gd_resource_sscellmap.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
Variant GdLoaderSsCellMap::_load(const String &p_path, const String &p_original_path, bool p_use_sub_threads, int32_t p_cache_mode)
#else
#ifdef GD_V4
Ref<Resource> GdLoaderSsCellMap::load( const String& p_path, const String& p_original_path, Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode )
#endif
#ifdef GD_V3
RES GdLoaderSsCellMap::load( const String& p_path, const String& p_original_path, Error* r_error, bool p_no_subresource_cache )
#endif
#endif
{
	Ref<GdResourceSsCellMap>	res = memnew( GdResourceSsCellMap );
	Error						err = res->loadFromFile( p_path, p_original_path );

	if ( err != OK ) {
		ERR_PRINT( String( "load error: " ) + String::num( err ) );
	}

#ifndef SPRITESTUDIO_GODOT_EXTENSION
	if ( r_error ) {
		*r_error = err;
	}
#endif

	return	res;
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray GdLoaderSsCellMap::_get_recognized_extensions()
{
	PackedStringArray extensions;
	extensions.push_back("ssce");
	return extensions;
}
#else
void GdLoaderSsCellMap::get_recognized_extensions( List<String>* p_extensions ) const
{
	if ( !p_extensions->find( "ssce" ) ) {
		p_extensions->push_back( "ssce" );
	}
}
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool GdLoaderSsCellMap::_handles_type(const StringName &p_type)
#else
bool GdLoaderSsCellMap::handles_type( const String& p_type ) const
#endif
{
	return	ClassDB::is_parent_class( p_type, "GdResourceSsCellMap" );
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
String GdLoaderSsCellMap::_get_resource_type(const String &p_path)
#else
String GdLoaderSsCellMap::get_resource_type( const String& p_path ) const
#endif
{
	return	"GdResourceSsCellMap";
}
