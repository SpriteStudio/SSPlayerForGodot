/*!
* \file		gd_resource_ssdocument.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_resource_ssdocument.h"

#include "gd_io.h"

GdResourceSsDocument::GdResourceSsDocument()
{
	m_strSource.resize(0);
}

GdResourceSsDocument::~GdResourceSsDocument()
{
}

Error GdResourceSsDocument::loadFromFile( const String& strPath, const String& strOrgPath )
{
	m_strSource = GdIO::loadStringFromFile( strPath );

	return	OK;
}

Error GdResourceSsDocument::saveToFile( const String& strPath, const Ref<Resource>& res )
{
	return	FAILED;
}

String GdResourceSsDocument::getSource() const
{
	return	m_strSource;
}

void GdResourceSsDocument::_bind_methods()
{
	ClassDB::bind_method( D_METHOD( "load_from_file", "path", "org_path" ), &GdResourceSsDocument::loadFromFile );
}
