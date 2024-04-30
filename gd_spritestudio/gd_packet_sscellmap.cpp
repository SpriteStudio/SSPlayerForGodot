/*!
* \file		gd_packet_sscellmap.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_packet_sscellmap.h"

#include "ss_io.h"

GdPacketSsCellMap::GdPacketSsCellMap()
{
}

GdPacketSsCellMap::~GdPacketSsCellMap()
{
}

bool GdPacketSsCellMap::write( const String& strRaw )
{
	auto		c = strRaw.utf8();

	if ( c.length() > 0 ) {
		libXML::XMLDocument		xml;

		if ( libXML::XML_SUCCESS == xml.Parse( c, c.length() ) ) {
			SsXmlIArchiver		ar( xml.GetDocument(), "SpriteStudioCellMap" );
			SsCellMap			cellMap;

			cellMap.__Serialize( &ar );

			SsIO::push( *this, cellMap );
		}
	}

	return	true;
}

bool GdPacketSsCellMap::read( SsCellMap* pCellMap, const PoolByteArray& bytes )
{
	if ( !pCellMap ) {
		return	false;
	}

	set_data_array( bytes );

	SsIO::pull( *this, *pCellMap );

	return	true;
}

PoolByteArray GdPacketSsCellMap::getBytes() const
{
	return	get_data_array();
}
