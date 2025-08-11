/*!
* \file		gd_saver_bssanimepack.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_SAVER_BSSANIMEPACK_H
#define GD_SAVER_BSSANIMEPACK_H

#include "gd_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/resource_saver.hpp>
#include <godot_cpp/classes/resource_format_saver.hpp>
#include <godot_cpp/classes/resource.hpp>
using namespace godot;
#else
#include "core/io/resource_saver.h"
#endif

class GdSaverBssAnimePack : public ResourceFormatSaver
{
	GDCLASS( GdSaverBssAnimePack, ResourceFormatSaver );

public :
#ifdef SPRITESTUDIO_GODOT_EXTENSION
	static void _bind_methods(){};
	Error _save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags) override;
	bool _recognize(const Ref<Resource> &p_resource);
	PackedStringArray _get_recognized_extensions(const Ref<Resource> &p_resource);
#else
#ifdef GD_V4
	virtual Error save( const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags = 0 ) override;
	virtual bool recognize( const Ref<Resource>& p_resource ) const override;
	virtual void get_recognized_extensions( const Ref<Resource>& p_resource, List<String>* p_extensions ) const override;
#endif
#ifdef GD_V3
	virtual Error save( const String& p_path, const RES& p_resource, uint32_t p_flags = 0 ) override;
	virtual bool recognize( const RES& p_resource ) const override;
	virtual void get_recognized_extensions( const RES& p_resource, List<String>* p_extensions ) const override;
#endif
#endif
};

#endif // GD_SAVER_BSSANIMEPACK_H
