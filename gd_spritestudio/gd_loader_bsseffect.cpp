/*!
* \file		gd_loader_bsseffect.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_loader_bsseffect.h"

#include "gd_resource_sseffect.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
Variant GdLoaderBssEffect::_load(const String &p_path, const String &p_original_path, bool use_sub_threads, int32_t cache_mode)
#else
#ifdef GD_V4
Ref<Resource> GdLoaderBssEffect::load( const String& p_path, const String& p_original_path, Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode )
#endif
#ifdef GD_V3
RES GdLoaderBssEffect::load( const String& p_path, const String& p_original_path, Error* r_error, bool p_no_subresource_cache )
#endif
#endif
{
	Ref<GdResourceSsEffect>		res = memnew( GdResourceSsEffect );
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
PackedStringArray GdLoaderBssEffect::_get_recognized_extensions()
{
	PackedStringArray extensions;
	extensions.push_back("gdssee");
	return extensions;
}
#else
void GdLoaderBssEffect::get_recognized_extensions( List<String>* p_extensions ) const
{
	if ( !p_extensions->find( "gdssee" ) ) {
		p_extensions->push_back( "gdssee" );
	}
}
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool GdLoaderBssEffect::_handles_type(const StringName &p_type)
#else
bool GdLoaderBssEffect::handles_type( const String& p_type ) const
#endif
{
	return	ClassDB::is_parent_class( p_type, "GdResourceSsEffect" );
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
String GdLoaderBssEffect::_get_resource_type(const String &path)
#else
String GdLoaderBssEffect::get_resource_type( const String& p_path ) const
#endif
{
	return	"GdResourceSsEffect";
}
