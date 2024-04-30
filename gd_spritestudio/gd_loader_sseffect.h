/*!
* \file		gd_loader_sseffect.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_LOADER_SSEFFECT_H
#define GD_LOADER_SSEFFECT_H

#include "gd_macros.h"

#include "core/io/resource_loader.h"

class GdLoaderSsEffect : public ResourceFormatLoader
{
	GDCLASS( GdLoaderSsEffect, ResourceFormatLoader );

public :
#ifdef GD_V4
	virtual Ref<Resource> load( const String& p_path, const String& p_original_path = "", Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr, CacheMode p_cache_mode = CACHE_MODE_REUSE ) override;
#endif
#ifdef GD_V3
	virtual RES load( const String& p_path, const String& p_original_path = "", Error* r_error = nullptr, bool p_no_subresource_cache = false ) override;
#endif
	virtual void get_recognized_extensions( List<String>* p_extensions ) const override;
	virtual bool handles_type( const String& p_type ) const override;
	virtual String get_resource_type( const String& p_path ) const override;
};

#endif // GD_LOADER_SSEFFECT_H
