/*!
* \file		gd_loader_ssproject.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_loader_ssproject.h"

#include "gd_resource_ssproject.h"

#ifdef GD_V4
Ref<Resource> GdLoaderSsProject::load( const String& p_path, const String& p_original_path, Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode )
#endif
#ifdef GD_V3
RES GdLoaderSsProject::load( const String& p_path, const String& p_original_path, Error* r_error, bool p_no_subresource_cache )
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

void GdLoaderSsProject::get_recognized_extensions( List<String>* p_extensions ) const
{
	if ( !p_extensions->find( "sspj" ) ) {
		p_extensions->push_back( "sspj" );
	}
}

bool GdLoaderSsProject::handles_type( const String& p_type ) const
{
	return	ClassDB::is_parent_class( p_type, "GdResourceSsProject" );
}

String GdLoaderSsProject::get_resource_type( const String& p_path ) const
{
	return	"GdResourceSsProject";
}
