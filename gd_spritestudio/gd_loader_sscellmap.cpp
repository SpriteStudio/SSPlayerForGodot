/*!
* \file		gd_loader_sscellmap.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_loader_sscellmap.h"

#include "gd_resource_sscellmap.h"

#ifdef GD_V4
Ref<Resource> GdLoaderSsCellMap::load( const String& p_path, const String& p_original_path, Error* r_error, bool p_use_sub_threads, float* r_progress, CacheMode p_cache_mode )
#endif
#ifdef GD_V3
RES GdLoaderSsCellMap::load( const String& p_path, const String& p_original_path, Error* r_error, bool p_no_subresource_cache )
#endif
{
	Ref<GdResourceSsCellMap>	res = memnew( GdResourceSsCellMap );
	Error						err = res->loadFromFile( p_path, p_original_path );

	if ( err != OK ) {
		ERR_PRINT( String( "load error: " ) + String::num( err ) );
	}

	if ( r_error ) {
		*r_error = err;
	}

	return	res;
}

void GdLoaderSsCellMap::get_recognized_extensions( List<String>* p_extensions ) const
{
	if ( !p_extensions->find( "ssce" ) ) {
		p_extensions->push_back( "ssce" );
	}
}

bool GdLoaderSsCellMap::handles_type( const String& p_type ) const
{
	return	ClassDB::is_parent_class( p_type, "GdResourceSsCellMap" );
}

String GdLoaderSsCellMap::get_resource_type( const String& p_path ) const
{
	return	"GdResourceSsCellMap";
}
