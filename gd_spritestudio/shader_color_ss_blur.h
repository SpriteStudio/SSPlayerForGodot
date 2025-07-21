static const char* shader_color_ss_blur = R"(
/*!
* \file		shader_color_ss_blur.gdshader
* \author	CRI Middleware Co., Ltd.
*/
shader_type canvas_item;
render_mode blend_mix;

uniform float src_ratio;
uniform float dst_ratio;
uniform float dst_src_ratio;

uniform float A_TW;
uniform float A_TH;
uniform float A_U1;
uniform float A_V1;
uniform float A_LU;
uniform float A_TV;
uniform float A_CU;
uniform float A_CV;
uniform float A_RU;
uniform float A_BV;
uniform float A_PM;

uniform float P_0;
uniform float P_1;
uniform float P_2;
uniform float P_3;

uniform float S_INTPL;

uniform sampler2D color;
uniform sampler2D alpha;
uniform sampler2D color_authentic;
uniform sampler2D alpha_authentic;

uniform float use_mask;
uniform float draw_mask;
uniform float composite;

vec4 ssCalcCompositeColor( int i, vec4 c, vec4 p )
{
	vec4	n = vec4( 1.0 );

	if ( i == 1 ) c.rgb = c.rgb + p.rgb;
	else if ( i == 2 ) c.rgb = c.rgb - p.rgb;
	else if ( i == 3 ) c.rgb = c.rgb * p.rgb;
	else if ( i == 4 ) c.rgb = c.rgb / p.rgb;
	else if ( i == 5 ) c.rgb = n.rgb - ( n.rgb - p.rgb ) * ( n.rgb - c.rgb );
	else if ( i == 6 ) {
		if ( ( p.r + p.g + p.b ) / 3.0 < 0.5 ) { c.rgb = 2.0 * p.rgb * c.rgb; }else{ c.rgb = n.rgb - 2.0 * ( n.rgb - p.rgb ) * ( n.rgb - c.rgb ); }
	}

	return	c;
}

vec4 getBlendColor( vec4 c, vec4 p )
{
	return	vec4( p.rgb * src_ratio + mix( vec3( 1.0 ), p.rgb, dst_src_ratio ) * c.rgb * dst_ratio, p.a * c.a );
}

vec4 ssTextureSample( sampler2D tex, vec2 st )
{
	vec2 uv = st;
	if(S_INTPL < 0.5 )	{
		uv *= vec2( A_TW, A_TH );
		uv = floor( uv ) + vec2( 0.5, 0.5 );
		uv *= vec2( A_U1, A_V1 );
	}

	return	texture( tex, uv );
}
vec4 ssPreProc( vec4 col, sampler2D tex, vec2 st )
{
	if ( A_TW <= 0.0 ) {
		return	col;
	}

	float	pix_w = 1.0 / A_TW;
	float	pix_h = 1.0 / A_TH;

	float	defocused_x = P_0 * pix_w * 16.0;
	float	defocused_y = P_0 * pix_h * 16.0;

//	vec4 out_color = texture(tex, st) * col;
//	out_color += texture(tex, vec2(st.x + defocused_x, st.y + defocused_y)) * col;
//	out_color += texture(tex, vec2(st.x + defocused_x, st.y)) * col;
//	out_color += texture(tex, vec2(st.x , st.y + defocused_y)) * col;
//	out_color += texture(tex, vec2(st.x - defocused_x, st.y - defocused_y)) * col;
//	out_color += texture(tex, vec2(st.x + defocused_x, st.y - defocused_y)) * col;
//	out_color += texture(tex, vec2(st.x - defocused_x, st.y + defocused_y)) * col;
//	out_color += texture(tex, vec2(st.x - defocused_x, st.y)) * col;
//	out_color += texture(tex, vec2(st.x , st.y - defocused_y)) * col;
	vec4 out_color = ssTextureSample(tex, st) * col;
	out_color += ssTextureSample(tex, vec2(st.x + defocused_x, st.y + defocused_y)) * col;
	out_color += ssTextureSample(tex, vec2(st.x + defocused_x, st.y)) * col;
	out_color += ssTextureSample(tex, vec2(st.x , st.y + defocused_y)) * col;
	out_color += ssTextureSample(tex, vec2(st.x - defocused_x, st.y - defocused_y)) * col;
	out_color += ssTextureSample(tex, vec2(st.x + defocused_x, st.y - defocused_y)) * col;
	out_color += ssTextureSample(tex, vec2(st.x - defocused_x, st.y + defocused_y)) * col;
	out_color += ssTextureSample(tex, vec2(st.x - defocused_x, st.y)) * col;
	out_color += ssTextureSample(tex, vec2(st.x , st.y - defocused_y)) * col;
	out_color = out_color / 9.0;

	return	out_color;
}

vec4 ssPostProc( vec4 c, vec4 p )
{
	float	fUm = use_mask;
	float	fDm = draw_mask;
	int		iCo = int( composite );

//	if ( fDm >= 0.0 ) { c = ssCalcDrawMaskColor( fDm, c ); fUm = 0.0; iCo = 0; }
//	if ( fUm > 0.0 ) { c = ssCalcUseMaskColor( c ); }
	if ( iCo > 0 ) { c.rgb = c.rgb + p.rgb; }

	return	c;
}

void fragment()
{
//	COLOR = ssPreProc( COLOR, TEXTURE, UV );
	COLOR = ssPreProc( COLOR, color_authentic, UV );
//	COLOR = ssMainProc();
//	COLOR = ssPostProc( COLOR, texture( color, SCREEN_UV, 0.0 ) );
}
)";
