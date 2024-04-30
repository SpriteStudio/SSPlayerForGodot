/*!
* \file		gd_saver_bsseffect.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_SAVER_BSSEFFECT_H
#define GD_SAVER_BSSEFFECT_H

#include "gd_macros.h"

#include "core/io/resource_saver.h"

class GdSaverBssEffect : public ResourceFormatSaver
{
	GDCLASS( GdSaverBssEffect, ResourceFormatSaver );

public :
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
};

#endif // GD_SAVER_BSSEFFECT_H
