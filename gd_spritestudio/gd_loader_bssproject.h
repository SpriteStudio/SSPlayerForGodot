/*!
* \file		gd_loader_bssproject.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_LOADER_BSSPROJECT_H
#define GD_LOADER_BSSPROJECT_H

#include "gd_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/resource_format_loader.hpp>
using namespace godot;
#else
#include "core/io/resource_loader.h"
#endif

class GdLoaderBssProject : public ResourceFormatLoader
{
	GDCLASS( GdLoaderBssProject, ResourceFormatLoader );

public :
#ifdef SPRITESTUDIO_GODOT_EXTENSION
	static void _bind_methods(){};

	PackedStringArray _get_recognized_extensions();

	bool _handles_type(const StringName &p_type);

	String _get_resource_type(const String &path);

	Variant _load(const String &p_path, const String &p_original_path, bool use_sub_threads, int32_t cache_mode);
#else
#ifdef GD_V4
	virtual Ref<Resource> load( const String& p_path, const String& p_original_path = "", Error* r_error = nullptr, bool p_use_sub_threads = false, float* r_progress = nullptr, CacheMode p_cache_mode = CACHE_MODE_REUSE ) override;
#endif
#ifdef GD_V3
	virtual RES load( const String& p_path, const String& p_original_path = "", Error* r_error = nullptr, bool p_no_subresource_cache = false ) override;
#endif
	virtual void get_recognized_extensions( List<String>* p_extensions ) const override;
	virtual bool handles_type( const String& p_type ) const override;
	virtual String get_resource_type( const String& p_path ) const override;
#endif
};

#endif // GD_LOADER_BSSPROJECT_H
