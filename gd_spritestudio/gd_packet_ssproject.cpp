/*!
* \file		gd_packet_ssproject.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_packet_ssproject.h"

#include "ss_io.h"

GdPacketSsProject::GdPacketSsProject()
{
}

GdPacketSsProject::~GdPacketSsProject()
{
}

bool GdPacketSsProject::write( const String& strRaw )
{
	auto		c = strRaw.utf8();

	if ( c.length() > 0 ) {
		libXML::XMLDocument		xml;

		if ( libXML::XML_SUCCESS == xml.Parse( c.get_data(), c.length() ) ) {
			SsXmlIArchiver		ar( xml.GetDocument(), "SpriteStudioProject" );
			SsProject			project;

			project.__Serialize( &ar );

			SsIO::push( *this, project );
		}
	}

	return	true;
}

bool GdPacketSsProject::read( SsProject* pProject, const PoolByteArray& bytes )
{
	if ( !pProject ) {
		return	false;
	}

	set_data_array( bytes );

	SsIO::pull( *this, *pProject );

	return	true;
}

PoolByteArray GdPacketSsProject::getBytes() const
{
	return	get_data_array();
}
