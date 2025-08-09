/*!
* \file		gd_resource_ssdocument.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_RESOURCE_SSDOCUMENT_H
#define GD_RESOURCE_SSDOCUMENT_H

#include "gd_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/resource.hpp>
using namespace godot;
#else
#ifdef GD_V4
#include "core/io/resource.h"
#endif
#ifdef GD_V3
#include "core/resource.h"
#endif
#endif

class GdResourceSsDocument : public Resource
{
	GDCLASS( GdResourceSsDocument, Resource );

public :
	GdResourceSsDocument();
	virtual ~GdResourceSsDocument();

	Error loadFromFile( const String& strPath, const String& strOrgPath = "" );
	Error saveToFile( const String& strPath, const Ref<Resource>& res );

	String getSource() const;

protected :
	GdMultilevelCall static void _bind_methods();

private :
	String		m_strSource;
};

#endif // GD_RESOURCE_SSDOCUMENT_H
