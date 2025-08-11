static const char* shader_color_ss_wave = R"(
/*!
* \file		shader_color_ss_wave.gdshader
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
vec4 ssPreProc( vec4 col, sampler2D tex, vec2 st, inout bool pass )
{
	float	fWidth = P_0;
	float	fHeight = P_1;
	float	fPhase = P_2;

	if ( A_TW <= 0.0 ) {
		return	col;
	}

	vec2	Coord;
	float	fPixW;
	float	fPixH;
	vec4	Pixel;
	float	l = 384.0 * fHeight * 0.5;
	float	s;
	float	Pi = 3.14159265358979;

	Coord = st;

	fPixW = A_U1;
	fPixH = A_V1;

	s = l * sin( Coord.y * 384.0 * fWidth * 0.5 + Pi * 2.0 * fPhase ) * fPixW;

//	Pixel = texture( tex, vec2( Coord.x + s, Coord.y ) );
	Pixel = ssTextureSample( tex, vec2( Coord.x + s, Coord.y ) );

	return	getBlendColor( col, Pixel );
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
	bool pass = false;
//	COLOR = ssPreProc( COLOR, TEXTURE, UV, pass );
	COLOR = ssPreProc( COLOR, color_authentic, UV, pass );
	if ( pass ) discard;
//	COLOR = ssMainProc();
//	COLOR = ssPostProc( COLOR, texture( color, SCREEN_UV, 0.0 ) );
}
)";
