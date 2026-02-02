#include "gd_ssplayer_resource.h"

void GdSsPlayerResource::setSsabResource( const Ref<GdSsabResource>& resProject )
{
	_ssabRes = resProject;

	// GdNotifier::getInstance().notifyResourcePlayerChanged( this );
}

Ref<GdSsabResource> GdSsPlayerResource::getSsabResource() const
{
	return	_ssabRes;
}

void GdSsPlayerResource::_bind_methods()
{
	ClassDB::bind_method( D_METHOD( "set_ssab_resource", "res_ssab" ), &GdSsPlayerResource::setSsabResource );
	ClassDB::bind_method( D_METHOD( "get_ssab_resource" ), &GdSsPlayerResource::getSsabResource );

	ADD_PROPERTY(
		PropertyInfo(
			Variant::OBJECT,
			"res_ssab",
			PropertyHint::PROPERTY_HINT_RESOURCE_TYPE,
			"GdSsabResource"
		),
		"set_ssab_resource",
		"get_ssab_resource"
	);
}
