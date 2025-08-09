static const char* shader_color_ss_hsb = R"(
/*!
* \file		shader_color_ss_hsb.gdshader
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

vec4 toHSB( vec4 col )
{
	vec4	K = vec4( 0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0 );
	vec4	p = mix( vec4( col.bg, K.wz ), vec4( col.gb, K.xy ), step( col.b, col.g ) );
	vec4	q = mix( vec4( p.xyw, col.r ), vec4( col.r, p.yzx ), step( p.x, col.r ) );

	float	d = q.x - min( q.w, q.y );
	float	e = 1.0e-10;

	return	vec4( abs( q.z + ( q.w - q.y ) / ( 6.0 * d + e ) ), d / ( q.x + e ), q.x, col.a );
}

vec4 toRGB( vec4 hsb )
{
	vec4	K = vec4( 1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0 );
	vec3	p = abs( fract( hsb.xxx + K.xyz ) * 6.0 - K.www );

	return	vec4( hsb.z * mix( K.xxx, clamp( p - K.xxx, 0.0, 1.0 ), hsb.y ), hsb.w );
}

vec4 shiftHSB( vec4 hsb, float fRatioH, float fRatioS, float fRatioB )
{
	vec4	shift = hsb;

	shift.x += fRatioH;
	shift.y = clamp( hsb.y + fRatioS, 0.0, 1.0 );
	shift.z = clamp( hsb.z + fRatioB, 0.0, 1.0 );

	if ( shift.x < 0.0 ) {
		shift.x += 1.0;
	}
	if ( shift.x > 1.0 ) {
		shift.x -= 1.0;
	}

	return	shift;
}

vec4 adjustHSB( vec4 col, float fRatioH, float fRatioS, float fRatioB )
{
	vec4	hsb;

	hsb = toHSB( col );

	hsb = shiftHSB( hsb, fRatioH, fRatioS, fRatioB );

	return	toRGB( hsb );
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
	float	fHue = P_0;
	float	fSaturation = P_1;
	float	fBrightness = P_2;

	if ( A_TW <= 0.0 ) {
		return	adjustHSB( col, fHue, fSaturation, fBrightness );
	}

//	vec4	Pixel = texture( tex, st );
	vec4	Pixel = ssTextureSample( tex, st );

	return	getBlendColor( col, adjustHSB( Pixel, fHue, fSaturation, fBrightness ) );
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
