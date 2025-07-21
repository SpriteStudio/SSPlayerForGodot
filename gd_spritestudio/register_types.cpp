/*!
* \file		register_types.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "register_types.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/godot.hpp>
#include <godot_cpp/core/class_db.hpp>
using namespace godot;
#else
#ifdef GD_V4
#include "core/object/class_db.h"
#endif
#ifdef GD_V3
#include "core/class_db.h"
#endif
#endif

#include "gd_node_ssplayer.h"
#include "gd_resource_ssdocument.h"
#include "gd_resource_ssplayer.h"
#include "gd_resource_ssproject.h"
#include "gd_resource_ssanimepack.h"
#include "gd_resource_sscellmap.h"
#include "gd_resource_sseffect.h"
#include "gd_loader_ssproject.h"
#include "gd_loader_ssanimepack.h"
#include "gd_loader_sscellmap.h"
#include "gd_loader_sseffect.h"
#include "gd_loader_bssproject.h"
#include "gd_loader_bssanimepack.h"
#include "gd_loader_bsscellmap.h"
#include "gd_loader_bsseffect.h"
#include "gd_saver_bssproject.h"
#include "gd_saver_bssanimepack.h"
#include "gd_saver_bsscellmap.h"
#include "gd_saver_bsseffect.h"

static Ref<GdLoaderSsProject>		s_LoaderSsProject;
static Ref<GdLoaderSsAnimePack>		s_LoaderSsAnimePack;
static Ref<GdLoaderSsCellMap>		s_LoaderSsCellMap;
static Ref<GdLoaderSsEffect>		s_LoaderSsEffect;
static Ref<GdLoaderBssProject>		s_LoaderBssProject;
static Ref<GdLoaderBssAnimePack>	s_LoaderBssAnimePack;
static Ref<GdLoaderBssCellMap>		s_LoaderBssCellMap;
static Ref<GdLoaderBssEffect>		s_LoaderBssEffect;
static Ref<GdSaverBssProject>		s_SaverBssProject;
static Ref<GdSaverBssAnimePack>		s_SaverBssAnimePack;
static Ref<GdSaverBssCellMap>		s_SaverBssCellMap;
static Ref<GdSaverBssEffect>		s_SaverBssEffect;

void register_gd_spritestudio_types()
{
	ClassDB::register_class<GdNodeSsPlayer>();

	ClassDB::register_class<GdResourceSsDocument>();
	ClassDB::register_class<GdResourceSsPlayer>();
	ClassDB::register_class<GdResourceSsProject>();
	ClassDB::register_class<GdResourceSsAnimePack>();
	ClassDB::register_class<GdResourceSsCellMap>();
	ClassDB::register_class<GdResourceSsEffect>();

	ClassDB::register_class<GdLoaderSsProject>();
	ClassDB::register_class<GdLoaderSsAnimePack>();
	ClassDB::register_class<GdLoaderSsCellMap>();
	ClassDB::register_class<GdLoaderSsEffect>();
	ClassDB::register_class<GdLoaderBssProject>();
	ClassDB::register_class<GdLoaderBssAnimePack>();
	ClassDB::register_class<GdLoaderBssCellMap>();
	ClassDB::register_class<GdLoaderBssEffect>();
	ClassDB::register_class<GdSaverBssProject>();
	ClassDB::register_class<GdSaverBssAnimePack>();
	ClassDB::register_class<GdSaverBssCellMap>();
	ClassDB::register_class<GdSaverBssEffect>();


#if defined(GD_V4) || defined(SPRITESTUDIO_GODOT_EXTENSION)
	s_LoaderSsProject.instantiate();
	s_LoaderSsAnimePack.instantiate();
	s_LoaderSsCellMap.instantiate();
	s_LoaderSsEffect.instantiate();
	s_LoaderBssProject.instantiate();
	s_LoaderBssAnimePack.instantiate();
	s_LoaderBssCellMap.instantiate();
	s_LoaderBssEffect.instantiate();
	s_SaverBssProject.instantiate();
	s_SaverBssAnimePack.instantiate();
	s_SaverBssCellMap.instantiate();
	s_SaverBssEffect.instantiate();
#endif
#ifdef GD_V3
	s_LoaderSsProject.instance();
	s_LoaderSsAnimePack.instance();
	s_LoaderSsCellMap.instance();
	s_LoaderSsEffect.instance();
	s_LoaderBssProject.instance();
	s_LoaderBssAnimePack.instance();
	s_LoaderBssCellMap.instance();
	s_LoaderBssEffect.instance();
	s_SaverBssProject.instance();
	s_SaverBssAnimePack.instance();
	s_SaverBssCellMap.instance();
	s_SaverBssEffect.instance();
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
	ResourceLoader::get_singleton()->add_resource_format_loader( s_LoaderSsProject );
	ResourceLoader::get_singleton()->add_resource_format_loader( s_LoaderSsAnimePack );
	ResourceLoader::get_singleton()->add_resource_format_loader( s_LoaderSsCellMap );
	ResourceLoader::get_singleton()->add_resource_format_loader( s_LoaderSsEffect );
	ResourceLoader::get_singleton()->add_resource_format_loader( s_LoaderBssProject );
	ResourceLoader::get_singleton()->add_resource_format_loader( s_LoaderBssAnimePack );
	ResourceLoader::get_singleton()->add_resource_format_loader( s_LoaderBssCellMap );
	ResourceLoader::get_singleton()->add_resource_format_loader( s_LoaderBssEffect );
	ResourceSaver::get_singleton()->add_resource_format_saver( s_SaverBssProject );
	ResourceSaver::get_singleton()->add_resource_format_saver( s_SaverBssAnimePack );
	ResourceSaver::get_singleton()->add_resource_format_saver( s_SaverBssCellMap );
	ResourceSaver::get_singleton()->add_resource_format_saver( s_SaverBssEffect );
#else
	ResourceLoader::add_resource_format_loader( s_LoaderSsProject );
	ResourceLoader::add_resource_format_loader( s_LoaderSsAnimePack );
	ResourceLoader::add_resource_format_loader( s_LoaderSsCellMap );
	ResourceLoader::add_resource_format_loader( s_LoaderSsEffect );
	ResourceLoader::add_resource_format_loader( s_LoaderBssProject );
	ResourceLoader::add_resource_format_loader( s_LoaderBssAnimePack );
	ResourceLoader::add_resource_format_loader( s_LoaderBssCellMap );
	ResourceLoader::add_resource_format_loader( s_LoaderBssEffect );
	ResourceSaver::add_resource_format_saver( s_SaverBssProject );
	ResourceSaver::add_resource_format_saver( s_SaverBssAnimePack );
	ResourceSaver::add_resource_format_saver( s_SaverBssCellMap );
	ResourceSaver::add_resource_format_saver( s_SaverBssEffect );
#endif
}

void unregister_gd_spritestudio_types()
{
#ifdef SPRITESTUDIO_GODOT_EXTENSION
	ResourceSaver::get_singleton()->remove_resource_format_saver( s_SaverBssEffect );
	ResourceSaver::get_singleton()->remove_resource_format_saver( s_SaverBssCellMap );
	ResourceSaver::get_singleton()->remove_resource_format_saver( s_SaverBssAnimePack );
	ResourceSaver::get_singleton()->remove_resource_format_saver( s_SaverBssProject );
	ResourceLoader::get_singleton()->remove_resource_format_loader( s_LoaderBssEffect );
	ResourceLoader::get_singleton()->remove_resource_format_loader( s_LoaderBssCellMap );
	ResourceLoader::get_singleton()->remove_resource_format_loader( s_LoaderBssAnimePack );
	ResourceLoader::get_singleton()->remove_resource_format_loader( s_LoaderBssProject );
	ResourceLoader::get_singleton()->remove_resource_format_loader( s_LoaderSsEffect );
	ResourceLoader::get_singleton()->remove_resource_format_loader( s_LoaderSsCellMap );
	ResourceLoader::get_singleton()->remove_resource_format_loader( s_LoaderSsAnimePack );
	ResourceLoader::get_singleton()->remove_resource_format_loader( s_LoaderSsProject );
#else
	ResourceSaver::remove_resource_format_saver( s_SaverBssEffect );
	ResourceSaver::remove_resource_format_saver( s_SaverBssCellMap );
	ResourceSaver::remove_resource_format_saver( s_SaverBssAnimePack );
	ResourceSaver::remove_resource_format_saver( s_SaverBssProject );
	ResourceLoader::remove_resource_format_loader( s_LoaderBssEffect );
	ResourceLoader::remove_resource_format_loader( s_LoaderBssCellMap );
	ResourceLoader::remove_resource_format_loader( s_LoaderBssAnimePack );
	ResourceLoader::remove_resource_format_loader( s_LoaderBssProject );
	ResourceLoader::remove_resource_format_loader( s_LoaderSsEffect );
	ResourceLoader::remove_resource_format_loader( s_LoaderSsCellMap );
	ResourceLoader::remove_resource_format_loader( s_LoaderSsAnimePack );
	ResourceLoader::remove_resource_format_loader( s_LoaderSsProject );
#endif
	
	s_SaverBssEffect.unref();
	s_SaverBssCellMap.unref();
	s_SaverBssAnimePack.unref();
	s_SaverBssProject.unref();
	s_LoaderBssEffect.unref();
	s_LoaderBssCellMap.unref();
	s_LoaderBssAnimePack.unref();
	s_LoaderBssProject.unref();
	s_LoaderSsEffect.unref();
	s_LoaderSsCellMap.unref();
	s_LoaderSsAnimePack.unref();
	s_LoaderSsProject.unref();
}

#if defined(GD_V4) || defined(SPRITESTUDIO_GODOT_EXTENSION)
void initialize_gd_spritestudio_module( ModuleInitializationLevel p_level )
{
	if ( p_level != MODULE_INITIALIZATION_LEVEL_SCENE ) {
		return;
	}

	register_gd_spritestudio_types();
}

void uninitialize_gd_spritestudio_module( ModuleInitializationLevel p_level )
{
	if ( p_level != MODULE_INITIALIZATION_LEVEL_SCENE ) {
		return;
	}

	unregister_gd_spritestudio_types();
}
#endif

#ifdef SPRITESTUDIO_GODOT_EXTENSION
extern "C" GDExtensionBool GDE_EXPORT spritestudio_godot_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
	init_obj.register_initializer(initialize_gd_spritestudio_module);
	init_obj.register_terminator(uninitialize_gd_spritestudio_module);
	init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
	return init_obj.init();
}
#endif
