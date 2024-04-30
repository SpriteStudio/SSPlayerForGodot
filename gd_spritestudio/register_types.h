/*!
* \file		register_types.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_SPRITESTUDIO_REGISTER_TYPES_H
#define GD_SPRITESTUDIO_REGISTER_TYPES_H

#include "gd_macros.h"

#ifdef GD_V4
#include "modules/register_module_types.h"
#endif

void register_gd_spritestudio_types();
void unregister_gd_spritestudio_types();

#ifdef GD_V4
void initialize_gd_spritestudio_module( ModuleInitializationLevel p_level );
void uninitialize_gd_spritestudio_module( ModuleInitializationLevel p_level );
#endif

#endif // GD_SPRITESTUDIO_REGISTER_TYPES_H
