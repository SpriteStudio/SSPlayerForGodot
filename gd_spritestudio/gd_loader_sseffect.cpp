/*!
* \file		gd_loader_sseffect.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_loader_sseffect.h"

#include "gd_resource_sseffect.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
Variant GdLoaderSsEffect::_load(const String &p_path, const String &p_original_path, bool p_use_sub_threads, int32_t p_cache_mode)
#else
#ifdef GD_V4
Ref<Resource> GdLoaderSsEffect::load( const String& p_path, const String& p_original_path, Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode )
#endif
#ifdef GD_V3
RES GdLoaderSsEffect::load( const String& p_path, const String& p_original_path, Error* r_error, bool p_no_subresource_cache )
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
PackedStringArray GdLoaderSsEffect::_get_recognized_extensions()
{
	PackedStringArray extensions;
	extensions.push_back("ssee");
	return extensions;
}
#else
void GdLoaderSsEffect::get_recognized_extensions( List<String>* p_extensions ) const
{
	if ( !p_extensions->find( "ssee" ) ) {
		p_extensions->push_back( "ssee" );
	}
}
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool GdLoaderSsEffect::_handles_type(const StringName &p_type)
#else
bool GdLoaderSsEffect::handles_type( const String& p_type ) const
#endif
{
	return	ClassDB::is_parent_class( p_type, "GdResourceSsEffect" );
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
String GdLoaderSsEffect::_get_resource_type(const String &p_path)
#else
String GdLoaderSsEffect::get_resource_type( const String& p_path ) const
#endif
{
	return	"GdResourceSsEffect";
}
