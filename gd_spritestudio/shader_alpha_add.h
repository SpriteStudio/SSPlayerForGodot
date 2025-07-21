static const char* shader_alpha_add = R"(
/*!
* \file		shader_alpha_add.gdshader
* \author	CRI Middleware Co., Ltd.
*/
shader_type canvas_item;
render_mode blend_add;

uniform float src_ratio;
uniform float dst_ratio;
uniform float dst_src_ratio;

uniform sampler2D color;
uniform sampler2D alpha;

vec4 getBlendColor( vec4 c, vec4 p )
{
	return	vec4( p.rgb * src_ratio + mix( vec3( 1.0 ), p.rgb, dst_src_ratio ) * c.rgb * dst_ratio, p.a * c.a );
}

void fragment()
{
	COLOR = getBlendColor( COLOR, texture( TEXTURE, UV ) );
}
)";
