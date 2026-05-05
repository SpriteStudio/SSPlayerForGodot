/*!
* \file		register_types.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef SS_PLAYER_REGISTER_TYPES_H
#define SS_PLAYER_REGISTER_TYPES_H

#include "ss_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/godot.hpp>
using namespace godot;
#else
#include "modules/register_module_types.h"
#endif

void register_ss_player_types();
void unregister_ss_player_types();

void initialize_ss_player_module( ModuleInitializationLevel p_level );
void uninitialize_ss_player_module( ModuleInitializationLevel p_level );

#endif // SS_PLAYER_REGISTER_TYPES_H