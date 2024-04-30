/*!
* \file		gd_packet_ssanimepack.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_packet_ssanimepack.h"

#include "ss_io.h"

GdPacketSsAnimePack::GdPacketSsAnimePack()
{
}

GdPacketSsAnimePack::~GdPacketSsAnimePack()
{
}

bool GdPacketSsAnimePack::write( const String& strRaw )
{
	auto		c = strRaw.utf8();

	if ( c.length() > 0 ) {
		libXML::XMLDocument		xml;

		if ( libXML::XML_SUCCESS == xml.Parse( c, c.length() ) ) {
			SsXmlIArchiver		ar( xml.GetDocument(), "SpriteStudioAnimePack" );
			SsAnimePack			animePack;

			animePack.__Serialize( &ar );

			SsIO::push( *this, animePack );
		}
	}

	return	true;
}

bool GdPacketSsAnimePack::read( SsAnimePack* pAnimePack, const PoolByteArray& bytes )
{
	if ( !pAnimePack ) {
		return	false;
	}

	set_data_array( bytes );

	SsIO::pull( *this, *pAnimePack );

	return	true;
}

PoolByteArray GdPacketSsAnimePack::getBytes() const
{
	return	get_data_array();
}
