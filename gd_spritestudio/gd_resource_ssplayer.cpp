/*!
* \file		gd_resource_ssplayer.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "gd_resource_ssplayer.h"

#include "gd_notifier.h"

GdResourceSsPlayer::GdResourceSsPlayer()
{
}

GdResourceSsPlayer::~GdResourceSsPlayer()
{
}

void GdResourceSsPlayer::setProjectResource( const Ref<GdResourceSsProject>& resProject )
{
	m_ResProject = resProject;

	GdNotifier::getInstance().notifyResourcePlayerChanged( this );
}

Ref<GdResourceSsProject> GdResourceSsPlayer::getProjectResource() const
{
	return	m_ResProject;
}

void GdResourceSsPlayer::_bind_methods()
{
	ClassDB::bind_method( D_METHOD( "set_project_resource", "res_project" ), &GdResourceSsPlayer::setProjectResource );
	ClassDB::bind_method( D_METHOD( "get_project_resource" ), &GdResourceSsPlayer::getProjectResource );

	ADD_PROPERTY(
		PropertyInfo(
			Variant::OBJECT,
			GdUiText( "res_project" ),
			PropertyHint::PROPERTY_HINT_RESOURCE_TYPE,
			"GdResourceSsProject"
		),
		"set_project_resource",
		"get_project_resource"
	);
}
