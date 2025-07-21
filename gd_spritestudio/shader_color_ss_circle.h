static const char* shader_color_ss_circle = R"(
/*!
* \file		shader_color_ss_circle.gdshader
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
	float	fPhase = P_0;
	float	fDirection = P_1;

	if ( A_TW <= 0.0 ) {
		return	col;
	}

	float	Pi = 3.14159265358979;
	float	e = 1.0e-10;
	vec2	uv1 = vec2( A_U1 + e, A_V1 + e );
	vec2	tx = st / uv1;
	vec2	c = vec2( A_CU, A_CV ) / uv1;
	vec2	lt = vec2( A_LU, A_TV ) / uv1;
	vec2	rb = vec2( A_RU, A_BV ) / uv1;
	vec2	d = rb - lt;
	vec2	v = tx - c;
	float	r = min( abs( rb.x - c.x ), abs( rb.y - c.y ) ) + e;
	float	dis = length( v );

	if ( dis > r ) {
		pass = true;
	}

	vec2	nv = normalize( v );
	float	uu = 1.0 - dis / r;
	float	vv = ( atan( nv.y, nv.x ) / Pi + 1.0 ) * 0.5 + fPhase;

	if ( vv < 0.0 ) {
		vv += 1.0;
	}
	if ( vv > 1.0 ) {
		vv -= 1.0;
	}

	if ( fDirection <= 0.0 ) {
		vv = 1.0 - vv;
	}

	vec2	xy = d * vec2( uu, vv ) * uv1;

//	vec4	Pixel = texture( tex, vec2( A_LU, A_TV ) + xy );
	vec4	Pixel = ssTextureSample( tex, vec2( A_LU, A_TV ) + xy );

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
