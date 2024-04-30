/*!
* \file		gd_loader_bssproject.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_loader_bssproject.h"

#include "gd_resource_ssproject.h"

#ifdef GD_V4
Ref<Resource> GdLoaderBssProject::load( const String& p_path, const String& p_original_path, Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode )
#endif
#ifdef GD_V3
RES GdLoaderBssProject::load( const String& p_path, const String& p_original_path, Error* r_error, bool p_no_subresource_cache )
#endif
{
	Ref<GdResourceSsProject>	res = memnew( GdResourceSsProject );
	Error						err = res->loadFromFile( p_path, p_original_path );

	if ( err != OK ) {
		ERR_PRINT( String( "load error: " ) + String::num( err ) );
	}

	if ( r_error ) {
		*r_error = err;
	}

	return	res;
}

void GdLoaderBssProject::get_recognized_extensions( List<String>* p_extensions ) const
{
	if ( !p_extensions->find( "gdsspj" ) ) {
		p_extensions->push_back( "gdsspj" );
	}
}

bool GdLoaderBssProject::handles_type( const String& p_type ) const
{
	return	ClassDB::is_parent_class( p_type, "GdResourceSsProject" );
}

String GdLoaderBssProject::get_resource_type( const String& p_path ) const
{
	return	"GdResourceSsProject";
}
