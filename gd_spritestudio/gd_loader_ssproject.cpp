/*!
* \file		gd_loader_ssproject.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_loader_ssproject.h"

#include "gd_resource_ssproject.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
Variant GdLoaderSsProject::_load(const String &p_path, const String &p_original_path, bool use_sub_threads, int32_t cache_mode)
#else
#ifdef GD_V4
Ref<Resource> GdLoaderSsProject::load( const String& p_path, const String& p_original_path, Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode )
#endif
#ifdef GD_V3
RES GdLoaderSsProject::load( const String& p_path, const String& p_original_path, Error* r_error, bool p_no_subresource_cache )
#endif
#endif
{
	Ref<GdResourceSsProject>	res = memnew( GdResourceSsProject );
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
PackedStringArray GdLoaderSsProject::_get_recognized_extensions()
{
	PackedStringArray extensions;
	extensions.push_back("sspj");
	return extensions;
}
#else
void GdLoaderSsProject::get_recognized_extensions( List<String>* p_extensions ) const
{
	if ( !p_extensions->find( "sspj" ) ) {
		p_extensions->push_back( "sspj" );
	}
}
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool GdLoaderSsProject::_handles_type(const StringName &p_type)
#else
bool GdLoaderSsProject::handles_type( const String& p_type ) const
#endif
{
	return	ClassDB::is_parent_class( p_type, "GdResourceSsProject" );
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
String GdLoaderSsProject::_get_resource_type(const String &path)
#else
String GdLoaderSsProject::get_resource_type( const String& p_path ) const
#endif
{
	return	"GdResourceSsProject";
}
