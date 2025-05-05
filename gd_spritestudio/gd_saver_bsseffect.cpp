/*!
* \file		gd_saver_bsseffect.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_saver_bsseffect.h"

#include "gd_io.h"
#include "gd_resource_ssdocument.h"

#include "ss_io.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
Error GdSaverBssEffect::_save(const Ref<Resource> &p_resource, const String &p_path, uint32_t p_flags)
#else
#ifdef GD_V4
Error GdSaverBssEffect::save( const Ref<Resource>& p_resource, const String& p_path, uint32_t p_flags )
#endif
#ifdef GD_V3
Error GdSaverBssEffect::save( const String& p_path, const RES& p_resource, uint32_t p_flags )
#endif
#endif
{
	if ( !Object::cast_to<GdResourceSsDocument>( *p_resource ) ) {
		return	FAILED;
	}

	Ref<GdResourceSsDocument>	resDocument = p_resource;

	if ( resDocument.is_null() ) {
		return	FAILED;
}

	String		strSource = resDocument->getSource();

	if ( strSource.length() == 0 ) {
		return	FAILED;
	}

	return	GdIO::saveVariantToFile( p_path, SsIO::toEffectVariant( strSource ) );
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
bool GdSaverBssEffect::_recognize(const Ref<Resource> &p_resource)
#else
#ifdef GD_V4
bool GdSaverBssEffect::recognize( const Ref<Resource>& p_resource ) const
#endif
#ifdef GD_V3
bool GdSaverBssEffect::recognize( const RES& p_resource ) const
#endif
#endif
{
	return	( Object::cast_to<GdResourceSsDocument>( *p_resource ) != NULL );
}

#ifdef SPRITESTUDIO_GODOT_EXTENSION
PackedStringArray GdSaverBssEffect::_get_recognized_extensions(const Ref<Resource> &resource)
{
    PackedStringArray extensions;
    if (Object::cast_to<GdResourceSsDocument>(*resource)) {
        extensions.push_back("gdssee");
    }
    return extensions;
}
#else
#ifdef GD_V4
void GdSaverBssEffect::get_recognized_extensions( const Ref<Resource>& p_resource, List<String>* p_extensions ) const
#endif
#ifdef GD_V3
void GdSaverBssEffect::get_recognized_extensions( const RES& p_resource, List<String>* p_extensions ) const
#endif
{
	if ( Object::cast_to<GdResourceSsDocument>( *p_resource ) ) {
		p_extensions->push_back( "gdssee" );
	}
}
#endif