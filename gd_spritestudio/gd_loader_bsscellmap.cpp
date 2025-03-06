/*!
* \file		gd_loader_bsscellmap.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_loader_bsscellmap.h"

#include "gd_resource_sscellmap.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
Variant GdLoaderBssCellMap::_load(const String &p_path, const String &p_original_path, bool use_sub_threads, int32_t cache_mode)
#else
#ifdef GD_V4
Ref<Resource> GdLoaderBssCellMap::load( const String& p_path, const String& p_original_path, Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode )
#endif
#ifdef GD_V3
RES GdLoaderBssCellMap::load( const String& p_path, const String& p_original_path, Error* r_error, bool p_no_subresource_cache )
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
PackedStringArray GdLoaderBssCellMap::_get_recognized_extensions()
{
	PackedStringArray extensions;
	extensions.push_back("gdssce");
	return extensions;
}
#else
void GdLoaderBssCellMap::get_recognized_extensions( List<String>* p_extensions ) const
{
	if ( !p_extensions->find( "gdssce" ) ) {
		p_extensions->push_back( "gdssce" );
	}
}
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool GdLoaderBssCellMap::_handles_type(const StringName &p_type)
#else
bool GdLoaderBssCellMap::handles_type( const String& p_type ) const
#endif
{
	return	ClassDB::is_parent_class( p_type, "GdResourceSsCellMap" );
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
String GdLoaderBssCellMap::_get_resource_type(const String &path)
#else
String GdLoaderBssCellMap::get_resource_type( const String& p_path ) const
#endif
{
	return	"GdResourceSsCellMap";
}
