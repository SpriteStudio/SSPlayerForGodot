/*!
* \file		register_types.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_SPRITESTUDIO_REGISTER_TYPES_H
#define GD_SPRITESTUDIO_REGISTER_TYPES_H

#include "gd_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/godot.hpp>
using namespace godot;
#else
#include "modules/register_module_types.h"
#endif

void register_gd_spritestudio_types();
void unregister_gd_spritestudio_types();

void initialize_gd_spritestudio_module( ModuleInitializationLevel p_level );
void uninitialize_gd_spritestudio_module( ModuleInitializationLevel p_level );

#endif // GD_SPRITESTUDIO_REGISTER_TYPES_H