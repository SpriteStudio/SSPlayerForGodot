#include "gd_ssplayer_node.h"

void GdSsPlayerNode::_bind_methods() {
    ADD_SIGNAL(
		MethodInfo(
			"user_data",
			PropertyInfo(
				Variant::INT,
				"flag"
			),
			PropertyInfo(
				Variant::INT,
				"int_value"
			),
			PropertyInfo(
				Variant::RECT2,
				"rect_value"
			),
			PropertyInfo(
				Variant::VECTOR2,
				"point_value"
			),
			PropertyInfo(
				Variant::STRING,
				"string_value"
			)
		)
	);
	ADD_SIGNAL(
		MethodInfo(
			"signal",
			PropertyInfo(
				Variant::STRING,
				"command"
			),
			PropertyInfo(
				Variant::DICTIONARY,
				"value"
			)
		)
	);

	ADD_PROPERTY(
		PropertyInfo(
			Variant::OBJECT,
			"res_player",
			PropertyHint::PROPERTY_HINT_RESOURCE_TYPE,
			"GdSsabResource"
		),
		"set_player_resource",
		"get_player_resource"
	);

	ADD_GROUP( "Animation Settings", "" );
}