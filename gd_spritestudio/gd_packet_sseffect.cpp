/*!
* \file		gd_packet_sseffect.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_packet_sseffect.h"

#include "ss_io.h"

GdPacketSsEffect::GdPacketSsEffect()
{
}

GdPacketSsEffect::~GdPacketSsEffect()
{
}

bool GdPacketSsEffect::write( const String& strRaw )
{
	auto		c = strRaw.utf8();

	if ( c.length() > 0 ) {
		libXML::XMLDocument		xml;

		if ( libXML::XML_SUCCESS == xml.Parse( c, c.length() ) ) {
			SsXmlIArchiver		ar( xml.GetDocument(), "SpriteStudioEffect" );
			SsEffectFile		effect;

			effect.__Serialize( &ar );

			SsIO::push( *this, effect );
		}
	}

	return	true;
}

bool GdPacketSsEffect::read( SsEffectFile* pEffect, const PoolByteArray& bytes )
{
	if ( !pEffect ) {
		return	false;
	}

	set_data_array( bytes );

	SsIO::pull( *this, *pEffect );

	return	true;
}

PoolByteArray GdPacketSsEffect::getBytes() const
{
	return	get_data_array();
}
