/*!
* \file		ss_renderer_impl.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "ss_renderer_impl.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/rendering_server.hpp>
#define VisualServer    RenderingServer
using namespace godot;
#else
#ifdef GD_V4
#include "servers/rendering_server.h"
#define	VisualServer	RenderingServer
#endif
#ifdef GD_V3
#include "servers/visual_server.h"
#endif
#endif

#include "SpriteStudio6-SDK/Common/Animator/ssplayer_matrix.h"

#include "ss_macros.h"
#include "ss_texture_impl.h"

#include "shader_color.h"
#include "shader_color_ss_bmask.h"
#include "shader_color_ss_blur.h"
#include "shader_color_ss_circle.h"
#include "shader_color_ss_hsb.h"
#include "shader_color_ss_move.h"
#include "shader_color_ss_noise.h"
#include "shader_color_ss_outline.h"
#include "shader_color_ss_pix.h"
#include "shader_color_ss_scatter.h"
#include "shader_color_ss_sepia.h"
#include "shader_color_ss_spot.h"
#include "shader_color_ss_step.h"
#include "shader_color_ss_wave.h"

#include "shader_alpha.h"

SsSdkUsing

// static const RID ridTextureDummy = RID();

static void setupTextureCombinerTo_NoBlendRGB_MultiplyAlpha_()
{
#if 0
	// カラーは１００％テクスチャ
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
	// αだけ合成
	glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE0);
	glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PRIMARY_COLOR);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
	glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
#endif
}

static void setupSimpleTextureCombiner_for_PartsColor_( SsBlendType::_enum type, float rateOrAlpha, SsColorBlendTarget::_enum target )
{
#if 0
	//static const float oneColor[4] = {1.f,1.f,1.f,1.f};
	float constColor[4] = { 0.5f,0.5f,0.5f,rateOrAlpha };
	static const GLuint funcs[] = { GL_INTERPOLATE, GL_MODULATE, GL_ADD, GL_SUBTRACT };
	GLuint func = funcs[(int)type];
	GLuint srcRGB = GL_TEXTURE0;
	GLuint dstRGB = GL_PRIMARY_COLOR;

	// true:  頂点αをブレンドする。
	// false: constColor のαをブレンドする。
	bool combineAlpha = true;

	switch (type)
	{
	case SsBlendType::mix:
	case SsBlendType::mul:
	case SsBlendType::add:
	case SsBlendType::sub:
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		// rgb
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, func);

		// mix の場合、特殊
		if (type == SsBlendType::mix)
		{
			if (target == SsColorBlendTarget::whole)
			{
				// 全体なら、const 値で補間する
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_CONSTANT);
				glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, constColor);
			}
			else
			{
				// 頂点カラーのアルファをテクスチャに対する頂点カラーの割合にする。
				glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_PRIMARY_COLOR);

				combineAlpha = false;
			}
			// 強度なので 1 に近付くほど頂点カラーが濃くなるよう SOURCE0 を頂点カラーにしておく。
			std::swap(srcRGB, dstRGB);
		}
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, srcRGB);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, dstRGB);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_RGB, GL_SRC_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_RGB, GL_SRC_COLOR);
		break;
	case SsBlendType::screen:
	case SsBlendType::exclusion:
	case SsBlendType::invert:
	default:
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_RGB, GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE2_RGB, GL_CONSTANT);
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_MODULATE);
		break;
	}

	if (combineAlpha)
	{
		// alpha は常に掛け算
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PRIMARY_COLOR);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
	}
	else
	{
#if 1
		// 浮いた const 値を頂点αの代わりにブレンドする。v6.2.0+ 2018/06/21 endo
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_CONSTANT);
		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, constColor);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);
#else
		// ミックス＋頂点単位の場合αブレンドはできない。
		// αはテクスチャを100%使えれば最高だが、そうはいかない。
		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_REPLACE);
		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE0);
		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
#endif
	}
#endif
}

static inline float floatFromByte_( u8 color8 )
{
	return static_cast<float>(color8) / 255.f;
}

/// RGBA の各値を byte(0~255) -> float(0.0~1.0) に変換し、配列 dest の[0,1,2,3] に設定する。
static inline void rgbaByteToFloat_( float* dest, const SsColorBlendValue& src )
{
	const SsColor* srcColor = &src.rgba;

	dest[0] = floatFromByte_((u8)srcColor->r);
	dest[1] = floatFromByte_((u8)srcColor->g);
	dest[2] = floatFromByte_((u8)srcColor->b);
	dest[3] = floatFromByte_((u8)srcColor->a);
}

static inline void calcCenterVertexColor( float* colors, float* rates, float* vertexID )
{
	float a, r, g, b, rate;
	a = r = g = b = rate = 0;
	for (int i = 0; i < 4; i++)
	{
		int idx = i * 4;
		a += colors[idx++];
		r += colors[idx++];
		g += colors[idx++];
		b += colors[idx++];
		rate += rates[i];
	}

	//きれいな頂点変形への対応
	vertexID[4 * 2] = 4;
	vertexID[4 * 2 + 1] = 4;

	int idx = 4 * 4;
	colors[idx++] = a / 4.0f;
	colors[idx++] = r / 4.0f;
	colors[idx++] = g / 4.0f;
	colors[idx++] = b / 4.0f;
	rates[4] = rate / 4.0f;
}

SsRendererImpl::SsRendererImpl()
{
	setCanvasItem( RID() );
	setCanvasSize( 512, 512 );
	setCanvasCenter( 256, 256 );

	m_iZOrder = 0;
	m_iChildZOrder = 0;

	m_bTextureInterpolate = true;

	m_RendererColor.init();
	m_RendererAlpha.init();

	m_pCreatePartSpritesSourceModel = NULL;
	m_vecPartSprite.clear();
	m_mapPartSprite.clear();
	m_mapPartSpriteIdx.clear();

	m_dicShader["ss-bmask"] = shader_color_ss_bmask;
	m_dicShader["ss-blur"] = shader_color_ss_blur;
	m_dicShader["ss-circle"] = shader_color_ss_circle;
	m_dicShader["ss-hsb"] = shader_color_ss_hsb;
	m_dicShader["ss-move"] = shader_color_ss_move;
	m_dicShader["ss-noise"] = shader_color_ss_noise;
	m_dicShader["ss-outline"] = shader_color_ss_outline;
	m_dicShader["ss-pix"] = shader_color_ss_pix;
	m_dicShader["ss-scatter"] = shader_color_ss_scatter;
	m_dicShader["ss-sepia"] = shader_color_ss_sepia;
	m_dicShader["ss-spot"] = shader_color_ss_spot;
	m_dicShader["ss-step"] = shader_color_ss_step;
	m_dicShader["ss-wave"] = shader_color_ss_wave;
}

SsRendererImpl::~SsRendererImpl()
{
	freePartSprites();
}

void SsRendererImpl::draw( RID rid )
{
	VisualServer*		pVisualServer = VisualServer::get_singleton();
	Transform2D			transCanvas;
	Rect2				rect = Rect2( 0, 0, m_fCanvasWidth, m_fCanvasHeight );

#if defined(GD_V4) || defined(SPRITESTUDIO_GODOT_EXTENSION)
	transCanvas.set_scale( Size2( 1, -1 ) );
	transCanvas.translate_local( -m_fCanvasWidth + m_fCanvasX, 0 - m_fCanvasY );
#endif
#ifdef GD_V3
	transCanvas.translate( -m_fCanvasWidth + m_fCanvasX, -m_fCanvasHeight + m_fCanvasY );
#endif

//	pVisualServer->draw( false );

	pVisualServer->canvas_item_clear( rid );
	pVisualServer->canvas_item_add_set_transform( rid, transCanvas );
	pVisualServer->canvas_item_add_texture_rect( rid, rect, m_RendererColor.getTextureRid() );
}

void SsRendererImpl::setCanvasItem( RID rid )
{
	m_CanvasItemId = rid;
}

void SsRendererImpl::setCanvasSize( float fWidth, float fHeight )
{
	m_fCanvasWidth = fWidth;
	m_fCanvasHeight = fHeight;
}

void SsRendererImpl::setCanvasCenter( float fX, float fY )
{
	m_fCanvasX = fX;
	m_fCanvasY = fY;
}

void SsRendererImpl::setFps( int iFps )
{
	m_iFps = iFps;
}

int SsRendererImpl::getFps() const
{
	return	m_iFps;
}

void SsRendererImpl::setTextureInterpolate( bool bSwitch )
{
	m_bTextureInterpolate = bSwitch;
}
bool SsRendererImpl::getTextureInterpolate() const
{
	return m_bTextureInterpolate;
}

void SsRendererImpl::createPartSprites( SsModel* pModel, SsProject* pProject )
{
	m_RendererColor.setTranslate( m_fCanvasX, m_fCanvasY );
	m_RendererAlpha.setTranslate( m_fCanvasX, m_fCanvasY );
	m_RendererColor.setSize( m_fCanvasWidth, m_fCanvasHeight );
	m_RendererAlpha.setSize( m_fCanvasWidth, m_fCanvasHeight );

	if ( !pModel ) {
		return;
	}

	if ( m_pCreatePartSpritesSourceModel == pModel ) {
		return;
	}

	freePartSprites();

	m_pCreatePartSpritesSourceModel = pModel;

	VisualServer*		pVisualServer = VisualServer::get_singleton();

	m_vecPartSprite.resize( pModel->partList.size() );
	m_mapPartSprite.clear();
	m_mapPartSpriteIdx.clear();

	for ( int i = 0; i < pModel->partList.size(); i++ ) {
		auto			e = pModel->partList[i];
		PartSprite*		pSprite = new PartSprite();

		pSprite->colorCanvasItemId = pVisualServer->canvas_item_create();
		pSprite->alphaCanvasItemId = pVisualServer->canvas_item_create();

		pVisualServer->canvas_item_set_parent( pSprite->colorCanvasItemId, m_RendererColor.getCanvasItemRid() );
		pVisualServer->canvas_item_set_parent( pSprite->alphaCanvasItemId, m_RendererAlpha.getCanvasItemRid() );

		pSprite->vecChildColorCanvasItemId.clear();
		pSprite->vecChildAlphaCanvasItemId.clear();

		pSprite->materialId = pVisualServer->material_create();
		pSprite->shaderId = pVisualServer->shader_create();
		pSprite->currentBlendType = SsBlendType::invalid;
		pSprite->currentId = "";

		pVisualServer->material_set_shader( pSprite->materialId, pSprite->shaderId );

		m_vecPartSprite[e->arrayIndex] = pSprite;

		if ( e->type == SsPartType::instance ) {
			SsAnimePack*	pAnimePack = pProject->findAnimationPack( e->refAnimePack );
			SsAnimation*	pAnimation = pAnimePack->findAnimation( e->refAnime );

			createPartSprites2( &pAnimePack->Model, pProject, e, pSprite );
		}
	}
}

void SsRendererImpl::updateUserData( SsAnimeDecoder* pDecoder )
{
	m_mapUserData.clear();
	m_mapSignal.clear();

	if ( !pDecoder ) {
		return;
	}

	for ( int i = 0; i < pDecoder->getAnimeTotalFrame(); i++ ) {
		for ( auto it = pDecoder->getPartAnime().begin(); it != pDecoder->getPartAnime().end(); it++ ) {
			SsPart*			pPart = it->first;
			SsPartAnime*	pPartAnime = it->second;

			if ( !pPartAnime ) {
				continue;
			}

			for ( auto it2 = pPartAnime->attributes.begin(); it2 != pPartAnime->attributes.end(); it2++ ) {
				SsAttribute*	pAttr = *it2;

				switch ( pAttr->tag ) {
				case SsAttributeKind::user :
					for ( auto it3 = pAttr->key.begin(); it3 != pAttr->key.end(); it3++ ) {
						const SsKeyframe*	pKeyframe = *it3;
						SsUserDataAnime		userData;

						GetSsUserDataAnime( pKeyframe, userData );

						m_mapUserData[pKeyframe->time] = userData;
					}
					break;
				case SsAttributeKind::signal :
					for ( auto it3 = pAttr->key.begin(); it3 != pAttr->key.end(); it3++ ) {
						const SsKeyframe*	pKeyframe = *it3;
						SsSignalAttr		signalAttr;

						GetSsSignalAnime( pKeyframe, signalAttr );

						m_mapSignal[pKeyframe->time] = signalAttr;
					}
					break;
				default :
					break;
				}
			}
		}
	}
}

const std::map<int,SsUserDataAnime>& SsRendererImpl::getUserData() const
{
	return	m_mapUserData;
}

const std::map<int,SsSignalAttr>& SsRendererImpl::getSignal() const
{
	return	m_mapSignal;
}

void SsRendererImpl::initialize()
{
#if 0
//	if ( m_isInit ) return ;
	SSOpenGLShaderMan::Create();
	SSOpenGLVertexShader*	vs = new SSOpenGLVertexShader( "basic_vs" , glshader_sprite_vs );
	SSOpenGLFragmentShader*	fs1 = new SSOpenGLFragmentShader( "normal_fs" , glshader_sprite_fs );
	SSOpenGLFragmentShader*	fs2 = new SSOpenGLFragmentShader( "pot_fs" ,glshader_sprite_fs_pot );

	SSOpenGLProgramObject* pgo1 = new SSOpenGLProgramObject();
	SSOpenGLProgramObject* pgo2 = new SSOpenGLProgramObject();

	pgo1->Attach( vs );
	pgo1->Attach( fs1 );
	pgo1->Link();
	SSOpenGLShaderMan::PushPgObject( pgo1 );

	pgo2->Attach( vs );
	pgo2->Attach( fs2 );
	pgo2->Link();
	SSOpenGLShaderMan::PushPgObject( pgo2 );

	s_DefaultShaderMap.clear();
	for ( int i = 0; glshader_default[i].name != nullptr; i++ ) {
		s_DefaultShaderMap[glshader_default[i].name].reset( createProgramObject( glshader_default[i].name, glshader_default[i].vs, glshader_default[i].fs ) );
	}

//	m_isInit = true;
#endif
}

void SsRendererImpl::renderSetup( SsAnimeDecoder* state )
{
#if 0
	glDisableClientState( GL_COLOR_ARRAY );
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0 );

	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0);

	glBlendEquation( GL_FUNC_ADD );

	/*
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0);
	*/

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#endif
	VisualServer*		pVisualServer = VisualServer::get_singleton();

	if ( !state ) {
		return;
	}

	if ( m_iSetup <= 0 ) {
	m_iZOrder = 0;
	m_iChildZOrder = 0;

	updateUserData( state );

	for ( int i = 0; i < m_vecPartSprite.size(); i++ ) {
		auto	e = m_vecPartSprite[i];

		pVisualServer->canvas_item_clear( e->colorCanvasItemId );
		pVisualServer->canvas_item_clear( e->alphaCanvasItemId );

		for ( int j = 0; j < e->vecChildColorCanvasItemId.size(); j++ ) {
			auto	e2 = e->vecChildColorCanvasItemId[j];

			pVisualServer->canvas_item_clear( e2 );
		}

		for ( int j = 0; j < e->vecChildAlphaCanvasItemId.size(); j++ ) {
			auto	e2 = e->vecChildAlphaCanvasItemId[j];

			pVisualServer->canvas_item_clear( e2 );
		}
	}

	for ( auto it = m_mapPartSprite.begin(); it != m_mapPartSprite.end(); it++ ) {
		auto	e = it->second;

		for ( auto it2 = e.begin(); it2 != e.end(); it2++ ) {
			auto	ee = *it2;

			pVisualServer->canvas_item_clear( ee->colorCanvasItemId );
			pVisualServer->canvas_item_clear( ee->alphaCanvasItemId );

			for ( int j = 0; j < ee->vecChildColorCanvasItemId.size(); j++ ) {
				auto	e2 = ee->vecChildColorCanvasItemId[j];

				pVisualServer->canvas_item_clear( e2 );
			}

			for ( int j = 0; j < ee->vecChildAlphaCanvasItemId.size(); j++ ) {
				auto	e2 = ee->vecChildAlphaCanvasItemId[j];

				pVisualServer->canvas_item_clear( e2 );
			}
		}

		m_mapPartSpriteIdx[it->first] = 0;
	}
	}
	m_iSetup++;
}

void SsRendererImpl::renderPart( SsPartState* state )
{
	VisualServer*		pVisualServer = VisualServer::get_singleton();

	updatePartStateDrawing( state );

	m_iZOrder++;

	if ( state->hide ) {
		return;
	}

	SsPart*				pPart = state->part;
	SsCell*				pCell = state->cellValue.cell;
	ISSTexture*			pTexture = state->cellValue.texture;

	if ( !pPart ) {
		return;
	}

	if ( !pCell ) {
		return;
	}

	if ( !pTexture ) {
		return;
	}

	SsTextureImpl*		pTextureImpl = static_cast<SsTextureImpl*>( pTexture );
	Ref<Texture>		texture = pTextureImpl->getTexture();

	makePrimitive( state );

	PartSprite*			pSprite = NULL;

	if ( m_mapPartSprite.find( pPart ) != m_mapPartSprite.end() ) {
		if ( m_mapPartSpriteIdx[pPart] >= m_mapPartSprite[pPart].size() ) {
			m_mapPartSpriteIdx[pPart] = 0;
		}
		pSprite = m_mapPartSprite[pPart][m_mapPartSpriteIdx[pPart]];
		m_mapPartSpriteIdx[pPart] = m_mapPartSpriteIdx[pPart] + 1;
	}

	if ( !pSprite ) {
		pSprite = m_vecPartSprite[pPart->arrayIndex];
	}

	pVisualServer->canvas_item_set_z_index( pSprite->colorCanvasItemId, m_iZOrder );
	pVisualServer->canvas_item_set_z_index( pSprite->alphaCanvasItemId, m_iZOrder );

	pVisualServer->canvas_item_set_material( pSprite->colorCanvasItemId, pSprite->materialId );
	pVisualServer->canvas_item_set_material( pSprite->alphaCanvasItemId, pSprite->materialId );

#ifdef SPRITESTUDIO_GODOT_EXTENSION
	PackedVector2Array vecPosition;
	PackedInt32Array vecIndex;
	PackedColorArray vecColor;
	PackedVector2Array vecCoord;
	PackedInt32Array vecBone;
	PackedFloat32Array vecWeight;
#else
	Vector<Point2>		vecPosition;
	Vector<int>			vecIndex;
	Vector<Color>		vecColor;
	Vector<Point2>		vecCoord;
	Vector<int>			vecBone;
	Vector<float>		vecWeight;
#endif

	int		type = (int)state->partsColorValue.blendType;
	float*	rates = _rates;
	rates[0] = 1.0f;
	
	if ( state->is_parts_color ) {
		if ( state->partsColorValue.target == SsColorBlendTarget::whole ) {
			rates[0] = ( type == 0 ? state->partsColorValue.color.rate : (float)state->partsColorValue.color.rgba.a / 255.0f );	/* 0:mix */
		}else{
			rates[0] = state->partsColorValue.colors[0].rgba.a / 255.0f;
			rates[1] = state->partsColorValue.colors[1].rgba.a / 255.0f;
			rates[2] = state->partsColorValue.colors[2].rgba.a / 255.0f;
			rates[3] = state->partsColorValue.colors[3].rgba.a / 255.0f;

			int a = 0;
			a += state->partsColorValue.colors[0].rgba.a;
			a += state->partsColorValue.colors[1].rgba.a;
			a += state->partsColorValue.colors[2].rgba.a;
			a += state->partsColorValue.colors[3].rgba.a;
			a /= 4;
			rates[0] = (float)a / 255.0f;

			float 	alpha = state->alpha;
			if (state->localalpha != 1.0f)
			{
				alpha = state->localalpha;
			}
			for (int i = 0; i < 5; ++i)
			{
				if (state->partsColorValue.blendType == SsBlendType::mix)
				{
					state->colors[i * 4 + 3] *= alpha;
				}
			}
		}
	}else{
		rates[0] = 0.0f;
	}

	updateShaderSource( *pSprite, m_eBlendTypeDrawing, state->shaderValue.id );

	pVisualServer->material_set_param( pSprite->materialId, "src_ratio", ( type <= 1 ? 1.0f - rates[0] : 1.0f ) );	/* 1:mul, 0:mix */
	pVisualServer->material_set_param( pSprite->materialId, "dst_ratio", ( type == 3 ? -rates[0] : rates[0] ) );	/* 3:sub */
	pVisualServer->material_set_param( pSprite->materialId, "dst_src_ratio", ( type == 1 ? 1.0f : 0.0f ) );	/* 1:mul */

	pVisualServer->material_set_param( pSprite->materialId, "A_TW", _args[0] );
	pVisualServer->material_set_param( pSprite->materialId, "A_TH", _args[1] );
	pVisualServer->material_set_param( pSprite->materialId, "A_U1", _args[2] );
	pVisualServer->material_set_param( pSprite->materialId, "A_V1", _args[3] );
	pVisualServer->material_set_param( pSprite->materialId, "A_LU", _args[4] );
	pVisualServer->material_set_param( pSprite->materialId, "A_TV", _args[5] );
	pVisualServer->material_set_param( pSprite->materialId, "A_CU", _args[6] );
	pVisualServer->material_set_param( pSprite->materialId, "A_CV", _args[7] );
	pVisualServer->material_set_param( pSprite->materialId, "A_RU", _args[8] );
	pVisualServer->material_set_param( pSprite->materialId, "A_BV", _args[9] );
	pVisualServer->material_set_param( pSprite->materialId, "A_PM", _args[10] );
	pVisualServer->material_set_param( pSprite->materialId, "P_0", state->shaderValue.param[0] );
	pVisualServer->material_set_param( pSprite->materialId, "P_1", state->shaderValue.param[1] );
	pVisualServer->material_set_param( pSprite->materialId, "P_2", state->shaderValue.param[2] );
	pVisualServer->material_set_param( pSprite->materialId, "P_3", state->shaderValue.param[3] );

	pVisualServer->material_set_param( pSprite->materialId, "S_INTPL", (m_bTextureInterpolate) ? 1.0f : 0.0f );

//	Transform2D			transCanvas;
//	Rect2				rect = Rect2( 0, 0, m_fCanvasWidth, m_fCanvasHeight );

//	transCanvas.translate( -m_fCanvasWidth + m_fCanvasX, -m_fCanvasHeight + m_fCanvasY );

//	pVisualServer->canvas_item_clear( m_CanvasId );
//	pVisualServer->canvas_item_add_set_transform( m_CanvasId, transCanvas );
//	pVisualServer->canvas_item_add_texture_rect( m_CanvasId, rect, m_RendererColor.getTextureRid() );
//	pVisualServer->canvas_item_set_copy_to_backbuffer( m_CanvasId, true, m_RendererColor.getSize() );
//	pVisualServer->draw( false );
//	pVisualServer->sync();

	pVisualServer->material_set_param( pSprite->materialId, "color", RID() );
	pVisualServer->material_set_param( pSprite->materialId, "alpha", RID() );
	pVisualServer->material_set_param( pSprite->materialId, "color_authentic", texture->get_rid() );
	pVisualServer->material_set_param( pSprite->materialId, "alpha_authentic", texture->get_rid() );

	if ( state->partType == SsPartType::mesh ) {
		SsMeshPart*		mesh = state->meshPart.get();

		if ( mesh ) {
			std::vector<float>*				pVecf;
			std::vector<unsigned short>*	pVecus;

			Point2		point = Point2();
			SsVector3	vertexIn;
			SsVector3	vertexOut;

			pVecf = mesh->uvs.get();

			for ( int i = 0; i < pVecf->size(); i += 2 ) {
				vecCoord.push_back( Point2( pVecf->at( i + 0 ), pVecf->at( i + 1 ) ) );
			}

			pVecf = mesh->colors.get();

			for ( int i = 0; i < pVecf->size(); i += 4 ) {
				vecColor.push_back( Color( pVecf->at( i + 0 ), pVecf->at( i + 1 ), pVecf->at( i + 2 ), pVecf->at( i + 3 ) ) );
			}

			pVecf = mesh->draw_vertices.get();

			for ( int i = 0; i < pVecf->size(); i += 3 ) {
				if ( !mesh->isBind ) {
					vertexIn.x = pVecf->at( i + 0 );
					vertexIn.y = pVecf->at( i + 1 );

					MatrixTransformVector3( state->matrixLocal, vertexIn, vertexOut );

					point.x = vertexOut.x;
					point.y = vertexOut.y;
				}else{
					point.x = pVecf->at( i + 0 );
					point.y = pVecf->at( i + 1 );
				}

				vecPosition.push_back( point );
			}

			pVecus = mesh->indices.get();

			for ( int i = 0; i < pVecus->size(); i++ ) {
				vecIndex.push_back( pVecus->at( i ) );
			}

			pVisualServer->canvas_item_add_triangle_array(
				pSprite->colorCanvasItemId,
				vecIndex,
				vecPosition,
				vecColor,
				vecCoord,
				vecBone,
				vecWeight,
				RID()	// texture->get_rid()
			);
		}
	}else
	if ( state->partType == SsPartType::normal ) {
		const uint8_t	indicesFan[] = { 4, 3, 1, 4, 1, 0, 4, 0, 2, 4, 2, 3 };
		const uint8_t	indicesStrip[] = { 0, 1, 2, 1, 2, 3 };
		const uint8_t*	pIndices = NULL;
		size_t			szIndices = 0;
		size_t			szTri = 0;

		if ( ( state->is_vertex_transform ) || ( state->is_parts_color ) ) {
			pIndices = indicesFan;
			szIndices = sizeof( indicesFan ) / sizeof( uint8_t );
			szTri = szIndices / 3;
		}else{
			pIndices = indicesStrip;
			szIndices = sizeof( indicesStrip ) / sizeof( uint8_t );
			szTri = szIndices / 3;
		}

		for ( int i = 0; i < szIndices; i++ ) {
			vecIndex.push_back( pIndices[i] );
		}

		for ( int i = 0; i < 5; i++ ) {
			Point2		point = Point2();
			SsVector3	vertexIn;
			SsVector3	vertexOut;
			Color		col;
			Point2		coord;

			vertexIn.x = state->vertices[i * 3 + 0];
			vertexIn.y = state->vertices[i * 3 + 1];
			vertexIn.z = state->vertices[i * 3 + 2];

			MatrixTransformVector3( state->matrixLocal, vertexIn, vertexOut );

			point.x = vertexOut.x;
			point.y = vertexOut.y;

			vecPosition.push_back( point );

			col = Color( state->colors[i * 4 + 0], state->colors[i * 4 + 1], state->colors[i * 4 + 2], state->colors[i * 4 + 3] );

			vecColor.push_back( col );

			coord = Point2( state->uvs[i * 2 + 0], state->uvs[i * 2 + 1] );

			vecCoord.push_back( coord );
		}

		pVisualServer->canvas_item_add_triangle_array(
			pSprite->colorCanvasItemId,
			vecIndex,
			vecPosition,
			vecColor,
			vecCoord,
			vecBone,
			vecWeight,
			RID()	// texture->get_rid()
		);
	}else
	{
	}
}

void SsRendererImpl::renderMesh( SsMeshPart* mesh, float alpha )
{
	if (mesh == 0)return;

//	glPushMatrix();


	if (alpha == 0.0f)
	{
		return;
	}

//	glMatrixMode(GL_MODELVIEW);
	if (mesh->isBind)
	{
		//glLoadMatrixf(mesh->myPartState->matrix);
//		glLoadIdentity();
	}
	else {
//		glLoadMatrixf(mesh->myPartState->matrixLocal);
	}


	bool texture_is_pow2 = true;
//	int	gl_target = GL_TEXTURE_2D;
	if (mesh->targetTexture)
	{

		// テクスチャのサイズが2のべき乗かチェック
		if (mesh->targetTexture->isPow2())
		{
			// 2のべき乗
			texture_is_pow2 = true;
//			gl_target = GL_TEXTURE_2D;
		}
		else
		{
			// 2のべき乗ではない:NPOTテクスチャ
			texture_is_pow2 = false;
#if USE_GLEW
//			gl_target = GL_TEXTURE_RECTANGLE_ARB;
#else
//			gl_target = GL_TEXTURE_RECTANGLE;
#endif
		}


//		glEnable(gl_target);

//		SSTextureGL* tex_gl = (SSTextureGL*)mesh->targetTexture;
//		glBindTexture(gl_target, tex_gl->tex);

	}

	SsPartState* state = mesh->myPartState;

	// パーツカラーの指定
	std::vector<float>& colorsRaw = *(mesh->colors.get());
	if (state->is_parts_color)
	{

		// パーツカラーがある時だけブレンド計算する
		float setcol[4];
		if (state->partsColorValue.target == SsColorBlendTarget::whole)
		{
			// 単色
			const SsColorBlendValue& cbv = state->partsColorValue.color;
			setupSimpleTextureCombiner_for_PartsColor_(state->partsColorValue.blendType, cbv.rate, state->partsColorValue.target);
			rgbaByteToFloat_(setcol, cbv);
		}
		else
		{
			//4頂点はとりあえず左上のRGBAと Rate 
			const SsColorBlendValue& cbv = state->partsColorValue.colors[0];
			// コンバイナの設定は常に単色として行う。
			setupSimpleTextureCombiner_for_PartsColor_(state->partsColorValue.blendType, cbv.rate, SsColorBlendTarget::whole);
			rgbaByteToFloat_(setcol, cbv);
			if (state->partsColorValue.blendType == SsBlendType::mix)
			{
				setcol[3] = 1; // ミックス-頂点単位では A に Rate が入っておりアルファは無効なので1にしておく。
			}
		}
		for (size_t i = 0; i < mesh->ver_size; i++)
		{
			colorsRaw[i * 4 + 0] = setcol[0];
			colorsRaw[i * 4 + 1] = setcol[1];
			colorsRaw[i * 4 + 2] = setcol[2];
			colorsRaw[i * 4 + 3] = setcol[3] *alpha; // 不透明度を適用する。
		}
	}
	else {
		//WaitColor;
		//ウェイトカラーの合成色を頂点カラーとして使用（パーセント円の流用
		for (size_t i = 0; i < mesh->ver_size; i++)
		{
			colorsRaw[i * 4 + 0] = 1.0f;
			colorsRaw[i * 4 + 1] = 1.0f;
			colorsRaw[i * 4 + 2] = 1.0f;
			colorsRaw[i * 4 + 3] = alpha;
		}

//		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
//		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE);
//		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE0);
		// αだけ合成
//		glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
//		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE0_ALPHA, GL_TEXTURE0);
//		glTexEnvi(GL_TEXTURE_ENV, GL_SOURCE1_ALPHA, GL_PRIMARY_COLOR);
//		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND0_ALPHA, GL_SRC_ALPHA);
//		glTexEnvi(GL_TEXTURE_ENV, GL_OPERAND1_ALPHA, GL_SRC_ALPHA);

	}

//	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
//	glEnableClientState(GL_COLOR_ARRAY);
//	glEnableClientState(GL_VERTEX_ARRAY);

	// UV 配列を指定する
//	glTexCoordPointer(2, GL_FLOAT, 0, (GLvoid *)((mesh->uvs.get())->data()));

	// 頂点色を指定する
//	glColorPointer(4, GL_FLOAT, 0, (GLvoid *)((mesh->colors.get())->data()));

	// 頂点バッファの設定
//	glVertexPointer(3, GL_FLOAT, 0, (GLvoid *)((mesh->draw_vertices.get())->data()));

//	glDrawElements(GL_TRIANGLES, mesh->tri_size * 3, GL_UNSIGNED_SHORT, (mesh->indices.get())->data());

//	glPopMatrix();

	if (texture_is_pow2 == false)
	{
//		glDisable(gl_target);
	}
//	glDisable(GL_TEXTURE_2D);

	//ブレンドモード　減算時の設定を戻す
//	glBlendEquation(GL_FUNC_ADD);
}

void SsRendererImpl::execMask( SsPartState* state )
{
#if 0
	glEnable(GL_STENCIL_TEST);
	if (state->partType == SsPartType::mask)
		//if(0)
	{

		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		if (!(state->maskInfluence )) { //マスクが有効では無い＝重ね合わせる

			glStencilFunc(GL_ALWAYS, 1, ~0);  //常に通過
			glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
			//描画部分を1へ
	}
		else {
			glStencilFunc(GL_ALWAYS, 1, ~0);  //常に通過
			glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
		}


		glEnable(GL_ALPHA_TEST);

		//この設定だと
		//1.0fでは必ず抜けないため非表示フラグなし（＝1.0f)のときの挙動は考えたほうがいい

		//不透明度からマスク閾値へ変更
		float mask_alpha = (float)(255 - state->masklimen ) / 255.0f;
		glAlphaFunc(GL_GREATER, mask_alpha);
		;
		state->alpha = 1.0f;
}
	else {

		if ((state->maskInfluence )) //パーツに対してのマスクが有効か否か
		{
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			glStencilFunc(GL_NOTEQUAL, 0x1, 0x1);  //1と等しい
			glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		}
		else {
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			glDisable(GL_STENCIL_TEST);
		}

		// 常に無効
		glDisable(GL_ALPHA_TEST);

	}
#endif
	updatePartStateDrawing( state );
}

void SsRendererImpl::clearMask()
{
#if 0
	glClear(  GL_STENCIL_BUFFER_BIT );

	enableMask(false);
#endif
}

void SsRendererImpl::renderSpriteSimple( float matrix[16], int width, int height, SsVector2& pivot, SsVector2 uv1, SsVector2 uv2, const SsFColor& color )
{
	VisualServer*		pVisualServer = VisualServer::get_singleton();
	SsPartState*		state = m_pPartStateDrawing;

	if ( !state || !m_pCellValueDrawing ) {
		return;
	}
	if ( state->hide ) {
		return;
	}

	if ( state->partType != SsPartType::effect ) {
		return;
	}

	SsPart*				pPart = state->part;
	SsCell*				pCell = m_pCellValueDrawing->cell;
	ISSTexture*			pTexture = m_pCellValueDrawing->texture;
	PartSprite*			pSprite = m_vecPartSprite[pPart->arrayIndex];

	if ( !pPart ) {
		return;
	}

	if ( !pCell ) {
		return;
	}

	if ( !pTexture ) {
		return;
	}

	SsTextureImpl*		pTextureImpl = static_cast<SsTextureImpl*>( pTexture );
	Ref<Texture>		texture = pTextureImpl->getTexture();
	RID					colorCanvasItemId = RID();
	RID					alphaCanvasItemId = RID();

	if ( m_iChildZOrder >= pSprite->vecChildColorCanvasItemId.size() ) {
		colorCanvasItemId = pVisualServer->canvas_item_create();

		pSprite->vecChildColorCanvasItemId.push_back( colorCanvasItemId );
	}else{
		colorCanvasItemId = pSprite->vecChildColorCanvasItemId[m_iChildZOrder];
	}

	if ( m_iChildZOrder >= pSprite->vecChildAlphaCanvasItemId.size() ) {
		alphaCanvasItemId = pVisualServer->canvas_item_create();

		pSprite->vecChildAlphaCanvasItemId.push_back( alphaCanvasItemId );
	}else{
		alphaCanvasItemId = pSprite->vecChildAlphaCanvasItemId[m_iChildZOrder];
	}

	m_iZOrder++;
	m_iChildZOrder++;

	pVisualServer->canvas_item_set_parent( colorCanvasItemId, pSprite->colorCanvasItemId );
	pVisualServer->canvas_item_set_parent( alphaCanvasItemId, pSprite->alphaCanvasItemId );

	pVisualServer->canvas_item_set_z_index( colorCanvasItemId, m_iZOrder );
	pVisualServer->canvas_item_set_z_index( alphaCanvasItemId, m_iZOrder );

	pVisualServer->canvas_item_set_material( colorCanvasItemId, pSprite->materialId );
	pVisualServer->canvas_item_set_material( alphaCanvasItemId, pSprite->materialId );

	float w = width / 2.0f ;
	float h = height / 2.0f ;
	float vtx[8];

	vtx[0] =  -w - pivot.x; 
	vtx[1] =   h - pivot.y;
	vtx[2] =  -w - pivot.x; 
	vtx[3] =  -h - pivot.y;
	vtx[4] =   w - pivot.x; 
	vtx[5] =  -h - pivot.y;
	vtx[6] =   w - pivot.x; 
	vtx[7] =   h - pivot.y;

	float tuv[8];

	tuv[0] = uv1.x; tuv[1] = uv1.y;
	tuv[2] = uv1.x; tuv[3] = uv2.y;
	tuv[4] = uv2.x; tuv[5] = uv2.y;
	tuv[6] = uv2.x; tuv[7] = uv1.y;

#ifdef SPRITESTUDIO_GODOT_EXTENSION
	PackedVector2Array vecPosition;
	PackedInt32Array vecIndex;
	PackedColorArray vecColor;
	PackedVector2Array vecCoord;
	PackedInt32Array vecBone;
	PackedFloat32Array vecWeight;
#else
	Vector<Point2>		vecPosition;
	Vector<int>			vecIndex;
	Vector<Color>		vecColor;
	Vector<Point2>		vecCoord;
	Vector<int>			vecBone;
	Vector<float>		vecWeight;
#endif

	const uint8_t	indicesStrip[] = { 0, 1, 3, 1, 3, 2 };
	const uint8_t*	pIndices = NULL;
	size_t			szIndices = 0;
	size_t			szTri = 0;

	pIndices = indicesStrip;
	szIndices = sizeof( indicesStrip ) / sizeof( uint8_t );
	szTri = szIndices / 3;

	for ( int i = 0; i < szIndices; i++ ) {
		vecIndex.push_back( pIndices[i] );
	}

	for ( int i = 0; i < 4; i++ ) {
		Point2		point = Point2();
		SsVector3	vertexIn;
		SsVector3	vertexOut;
		Color		col;
		Point2		coord;

		vertexIn.x = vtx[i * 2 + 0];
		vertexIn.y = vtx[i * 2 + 1];

		MatrixTransformVector3( matrix, vertexIn, vertexOut );

		point.x = vertexOut.x;
		point.y = vertexOut.y;

		vecPosition.push_back( point );

		col = Color( color.r, color.g, color.b, color.a );

		vecColor.push_back( col );

		coord = Point2( tuv[i * 2 + 0], tuv[i * 2 + 1] );

		vecCoord.push_back( coord );
	}

	int		type = SsBlendType::mul;
	float*	rates = _rates;
	rates[0] = 1.0f;

	updateShaderSource( *pSprite, m_eBlendTypeDrawing, state->shaderValue.id );

	pVisualServer->material_set_param( pSprite->materialId, "src_ratio", ( type <= 1 ? 1.0f - rates[0] : 1.0f ) );
	pVisualServer->material_set_param( pSprite->materialId, "dst_ratio", ( type == 3 ? -rates[0] : rates[0] ) );
	pVisualServer->material_set_param( pSprite->materialId, "dst_src_ratio", ( type == 1 ? 1.0f : 0.0f ) );

	pVisualServer->material_set_param( pSprite->materialId, "A_TW", _args[0] );
	pVisualServer->material_set_param( pSprite->materialId, "A_TH", _args[1] );
	pVisualServer->material_set_param( pSprite->materialId, "A_U1", _args[2] );
	pVisualServer->material_set_param( pSprite->materialId, "A_V1", _args[3] );
	pVisualServer->material_set_param( pSprite->materialId, "A_LU", _args[4] );
	pVisualServer->material_set_param( pSprite->materialId, "A_TV", _args[5] );
	pVisualServer->material_set_param( pSprite->materialId, "A_CU", _args[6] );
	pVisualServer->material_set_param( pSprite->materialId, "A_CV", _args[7] );
	pVisualServer->material_set_param( pSprite->materialId, "A_RU", _args[8] );
	pVisualServer->material_set_param( pSprite->materialId, "A_BV", _args[9] );
	pVisualServer->material_set_param( pSprite->materialId, "A_PM", _args[10] );
	pVisualServer->material_set_param( pSprite->materialId, "P_0", state->shaderValue.param[0] );
	pVisualServer->material_set_param( pSprite->materialId, "P_1", state->shaderValue.param[1] );
	pVisualServer->material_set_param( pSprite->materialId, "P_2", state->shaderValue.param[2] );
	pVisualServer->material_set_param( pSprite->materialId, "P_3", state->shaderValue.param[3] );

	pVisualServer->material_set_param( pSprite->materialId, "S_INTPL", (m_bTextureInterpolate) ? 1.0f : 0.0f );

//	Transform2D			transCanvas;
//	Rect2				rect = Rect2( 0, 0, m_fCanvasWidth, m_fCanvasHeight );

//	transCanvas.translate( -m_fCanvasWidth + m_fCanvasX, -m_fCanvasHeight + m_fCanvasY );

//	pVisualServer->canvas_item_clear( m_CanvasId );
//	pVisualServer->canvas_item_add_set_transform( m_CanvasId, transCanvas );
//	pVisualServer->canvas_item_add_texture_rect( m_CanvasId, rect, m_RendererColor.getTextureRid() );
//	pVisualServer->canvas_item_set_copy_to_backbuffer( m_CanvasId, true, m_RendererColor.getSize() );
//	pVisualServer->draw( false );
//	pVisualServer->sync();

	/* MEMO: Ver.3 and Ver.4 handle vertex-colors differently on "fragment" shader.            */
	/*       In Ver.4, before user's "fragment" processing, "COLOR" is multiplied texel-color. */
	/*       In order to get unprocessed vertex-color, set dummy (always white) to "TEXTURE"   */
	/*         and decode original-texture on another-stage.  (Measures for Ver.4 spec.)       */
	pVisualServer->material_set_param( pSprite->materialId, "color", RID() );
	pVisualServer->material_set_param( pSprite->materialId, "alpha", RID() );
	pVisualServer->material_set_param( pSprite->materialId, "color_authentic", texture->get_rid() );
	pVisualServer->material_set_param( pSprite->materialId, "alpha_authentic", texture->get_rid() );

	pVisualServer->canvas_item_add_triangle_array(
		colorCanvasItemId,
		vecIndex,
		vecPosition,
		vecColor,
		vecCoord,
		vecBone,
		vecWeight,
		RID()	// texture->get_rid()
	);
}

void SsRendererImpl::SetAlphaBlendMode( SsBlendType::_enum type )
{
#if 0
	glBlendEquation( GL_FUNC_ADD );

	switch ( type )
	{
	case SsBlendType::mix:				//< 0 ブレンド（ミックス）
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case SsBlendType::mul:				//< 1 乗算
		glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		break;
	case SsBlendType::add:				//< 2 加算
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		break;
	case SsBlendType::sub:				//< 3 減算
										// TODO SrcAlpha を透明度として使えない
		glBlendEquation( GL_FUNC_REVERSE_SUBTRACT );

#if USE_GLEW
		glBlendFuncSeparateEXT( GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_DST_ALPHA );
#else
		glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_DST_ALPHA);
#endif
		break;
	case SsBlendType::mulalpha: 		//< 4 α乗算
		glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case SsBlendType::screen: 			//< 5 スクリーン
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE);
		break;
	case SsBlendType::exclusion:		//< 6 除外
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);
		break;
	case SsBlendType::invert: 			//< 7 反転
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
		break;
	}
#endif
	updateBlendTypeDrawing( type );
}

void SsRendererImpl::SetTexture( SsCellValue* cell )
{
#if 0
	int		gl_target = GL_TEXTURE_2D;
	bool texture_is_pow2 = true;

	if ( cellvalue == 0 ){
		return ;
	}
	SsCell * cell = cellvalue->cell;
	if ( cell == 0 ){
		return ;
	}

	ISSTexture*	texture = cellvalue->texture;
	if ( texture == 0 ){
		return ;
	}

	SsPoint2 texturePixelSize;
	texturePixelSize.x = cellvalue->texture->getWidth();
	texturePixelSize.y = cellvalue->texture->getHeight();

	if (texture)
	{
		// テクスチャのサイズが2のべき乗かチェック
		if ( texture->isPow2() )
		{
			// 2のべき乗
			texture_is_pow2 = true;
			gl_target = GL_TEXTURE_2D;
		}
		else
		{
			// 2のべき乗ではない:NPOTテクスチャ
			texture_is_pow2 = false;
#if USE_GLEW
			gl_target = GL_TEXTURE_RECTANGLE_ARB;
#else
			gl_target = GL_TEXTURE_RECTANGLE;
#endif
		}


		glEnable(gl_target);

		SSTextureGL* tex_gl = (SSTextureGL*)texture;
		glBindTexture(gl_target, tex_gl->tex);

	}
#endif
	updateCellValueDrawing( cell );
}

void SsRendererImpl::enableMask( bool flag )
{
#if 0
	if (flag)
	{
		glEnable(GL_STENCIL_TEST);
	}else{
		glDisable(GL_STENCIL_TEST);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	}
#endif
}

void SsRendererImpl::createPartSprites2( SsModel* pModel, SsProject* pProject, SsPart* pPart, PartSprite* pParent )
{
	if ( !pModel ) {
		return;
	}

	VisualServer*		pVisualServer = VisualServer::get_singleton();

	for ( int i = 0; i < pModel->partList.size(); i++ ) {
		auto			e = pModel->partList[i];
		PartSprite*		pSprite = new PartSprite();

		pSprite->colorCanvasItemId = pVisualServer->canvas_item_create();
		pSprite->alphaCanvasItemId = pVisualServer->canvas_item_create();

		pVisualServer->canvas_item_set_parent( pSprite->colorCanvasItemId, pParent->colorCanvasItemId );
		pVisualServer->canvas_item_set_parent( pSprite->alphaCanvasItemId, pParent->alphaCanvasItemId );

		pSprite->vecChildColorCanvasItemId.clear();
		pSprite->vecChildAlphaCanvasItemId.clear();

		pSprite->materialId = pVisualServer->material_create();
		pSprite->shaderId = pVisualServer->shader_create();
		pSprite->currentBlendType = SsBlendType::invalid;
		pSprite->currentId = "";

		pVisualServer->material_set_shader( pSprite->materialId, pSprite->shaderId );

		m_mapPartSprite[e].push_back( pSprite );
		m_mapPartSpriteIdx[e] = 0;

		if ( e->type == SsPartType::instance ) {
			SsAnimePack*	pAnimePack = pProject->findAnimationPack( e->refAnimePack );
			SsAnimation*	pAnimation = pAnimePack->findAnimation( e->refAnime );

			createPartSprites2( &pAnimePack->Model, pProject, e, pSprite );
		}
	}
}

void SsRendererImpl::updatePartStateDrawing( SsPartState* pPartState )
{
	m_pPartStateDrawing = pPartState;
	m_iChildZOrder = 0;
}

void SsRendererImpl::updateCellValueDrawing( SsCellValue* pCellValue )
{
	m_pCellValueDrawing = pCellValue;
}

void SsRendererImpl::updateBlendTypeDrawing( SsBlendType::_enum eBlendType )
{
	m_eBlendTypeDrawing = eBlendType;
}

void SsRendererImpl::updateShaderSource( PartSprite& sprite, SsBlendType::_enum eBlendType, SsString strId )
{
	VisualServer*	pVisualServer = VisualServer::get_singleton();

	if ( sprite.currentBlendType == eBlendType && sprite.currentId == strId ) {
		return;
	}

	sprite.currentBlendType = eBlendType;
	sprite.currentId = strId;

	float	fComposite = 0.0f;

	switch ( static_cast<int>(eBlendType) ) {
	case SsBlendType::mix :
		fComposite = 0.0f;
		break;
	case SsBlendType::mul :
		fComposite = 3.0f;
		break;
	case SsBlendType::add :
		fComposite = 1.0f;
		break;
	case SsBlendType::sub :
		fComposite = 2.0f;
		break;
	case SsBlendType::mulalpha :
		fComposite = 3.0f;
		break;
	case SsBlendType::screen :
		fComposite = 5.0f;
		break;
	case SsBlendType::exclusion :
		fComposite = 0.0f;
		break;
	case SsBlendType::invert :
		fComposite = 0.0f;
		break;
	}

	String	strShader = shader_color;

	if ( !strId.empty() ) {
		if ( m_dicShader.has( strId.c_str() ) ) {
			strShader = m_dicShader.get( strId.c_str(), shader_color );
		}else{
			WARN_PRINT_ONCE( "shader " + String( strId.c_str() ) + " not found." );
		}
	}

	pVisualServer->shader_set_code( sprite.shaderId, strShader );

	pVisualServer->material_set_param( sprite.materialId, "use_mask", 0.0f );
	pVisualServer->material_set_param( sprite.materialId, "draw_mask", -1.0f );
	pVisualServer->material_set_param( sprite.materialId, "composite", fComposite );
}

void SsRendererImpl::makePrimitive( SsPartState* state )
{
	bool texture_is_pow2 = true;
	bool color_blend_v4 = false;
	float fTexW = 16.0f;
	float fTexH = 16.0f;
	float fPixTX = 1.0f;
	float fPixTY = 1.0f;
	float fCoordLU = 0.0f;
	float fCoordTV = 0.0f;
	float fCoordCU = 0.0f;
	float fCoordCV = 0.0f;
	float fCoordRU = 0.0f;
	float fCoordBV = 0.0f;
	float vertexID[10];
//	bool colorBlendEnabled = false;
	bool partsColorEnabled = false;
	bool alphaBlendMix = false;

//	int		gl_target = GL_TEXTURE_2D;
	float*	rates = _rates;

//	SSOpenGLProgramObject*	pPrgObject = nullptr;

	if ( state->hide ) return ; //非表示なので処理をしない


	SsCell * cell = state->cellValue.cell;
	if ( cell == 0 ) return ;

	ISSTexture*	texture = state->cellValue.texture;
	if ( texture == 0 ) return ;

	SsPoint2 texturePixelSize;
	texturePixelSize.x = state->cellValue.texture->getWidth();
	texturePixelSize.y = state->cellValue.texture->getHeight();

	execMask(state);

	//Ver6 ローカル不透明度対応
	float 	alpha = state->alpha;
	if (state->localalpha != 1.0f)
	{
		alpha = state->localalpha;
	}

	if (cell)
	{
		for ( int i = 0 ; i < 5 ; i++ ) rates[i] = 0.0f;
	}else{
		for ( int i = 0 ; i < 5 ; i++ ) rates[i] = 1.0f;
	}

	if (cell)
	{
		// テクスチャのサイズが2のべき乗かチェック
		if ( texture->isPow2() )
		{
			// 2のべき乗
			texture_is_pow2 = true;
//			gl_target = GL_TEXTURE_2D;
		}
		else
		{
			// 2のべき乗ではない:NPOTテクスチャ
			texture_is_pow2 = false;
#if USE_GLEW
//			gl_target = GL_TEXTURE_RECTANGLE_ARB;
#else
//			gl_target = GL_TEXTURE_RECTANGLE;
#endif
		}


//		glEnable(gl_target);

#if SPRITESTUDIO6SDK_PROGRAMABLE_SHADER_ON
		if ( state->is_shader ) {
			std::map<SsString, std::unique_ptr<SSOpenGLProgramObject>>::const_iterator it = s_DefaultShaderMap.find( state->shaderValue.id );
			if ( it != s_DefaultShaderMap.end() ) {
				pPrgObject = s_DefaultShaderMap[state->shaderValue.id].get();
			}
		}
		if ( !pPrgObject ) {
			std::map<SsString, std::unique_ptr<SSOpenGLProgramObject>>::const_iterator it = s_DefaultShaderMap.find( "system::default" );
			if ( it != s_DefaultShaderMap.end() ) {
				pPrgObject = s_DefaultShaderMap["system::default"].get();
			}
		}

//		if ( glpgObject )
		if ( pPrgObject )
		{
			glActiveTexture(GL_TEXTURE0);
		}
#endif

//		SSTextureGL* tex_gl = (SSTextureGL*)texture;
//		glBindTexture(gl_target, tex_gl->tex);

		// フィルタ
//		GLint filterMode;
		SsTexFilterMode::_enum fmode = state->cellValue.filterMode;
		SsTexWrapMode::_enum wmode = state->cellValue.wrapMode;


		switch (fmode)
		{
		default:
		case SsTexFilterMode::nearlest:
//			filterMode = GL_NEAREST;
			break;
		case SsTexFilterMode::linear:
//			filterMode = GL_LINEAR;
			break;
		}

//		glTexParameteri(gl_target, GL_TEXTURE_MIN_FILTER, filterMode);
//		glTexParameteri(gl_target, GL_TEXTURE_MAG_FILTER, filterMode);

		// ラップモード
//		GLint wrapMode; 
		switch (wmode)
		{
		default:
		case SsTexWrapMode::clamp:
//			wrapMode = GL_CLAMP_TO_EDGE;
			if (texture_is_pow2 == false)
			{
//				wrapMode = GL_CLAMP;
			}
			break;
		case SsTexWrapMode::repeat:
//			wrapMode = GL_REPEAT;	// TODO 要動作確認
			break;
		case SsTexWrapMode::mirror:
//			wrapMode = GL_MIRRORED_REPEAT;	// TODO 要動作確認
			break;
		}

//		glTexParameteri(gl_target, GL_TEXTURE_WRAP_S, wrapMode);
//		glTexParameteri(gl_target, GL_TEXTURE_WRAP_T, wrapMode);

		// セルが持つUV値をまず設定
		//std::memcpy( state->uvs, state->cellValue.uvs, sizeof( state->uvs ));
		for (int i = 0; i < sizeof(state->cellValue.uvs) / sizeof(state->cellValue.uvs[0]); i++)
		{
			state->uvs[i * 2 + 0] = state->cellValue.uvs[i].x;
			state->uvs[i * 2 + 1] = state->cellValue.uvs[i].y;
		}

		// UV アニメの適用
//		glMatrixMode(GL_TEXTURE);
//		glLoadIdentity();
		float transMat[16];
		float anchorMat[16];
		float rotateMat[16];
		float scaleMat[16];
		float texMat[16];
		IdentityMatrix( transMat );
		IdentityMatrix( anchorMat );
		IdentityMatrix( rotateMat );
		IdentityMatrix( scaleMat );
		IdentityMatrix( texMat );

		float uvw = state->cellValue.uvs[3].x + state->cellValue.uvs[0].x;
		float uvh = state->cellValue.uvs[3].y + state->cellValue.uvs[0].y;
		SsVector2 uv_trans;

		if (texture_is_pow2)
		{
			uv_trans.x = state->uvTranslate.x;
			uv_trans.y = state->uvTranslate.y;

			fTexW = texturePixelSize.x;
			fTexH = texturePixelSize.y;
		}
		else
		{
			uv_trans.x = state->uvTranslate.x * texturePixelSize.x;
			uv_trans.y = state->uvTranslate.y * texturePixelSize.y;


			//中心座標を計算
			uvw*= texturePixelSize.x;
			uvh*= texturePixelSize.y;

			fTexW = texturePixelSize.x;
			fTexH = texturePixelSize.y;
		}

		uvw/=2.0f;
		uvh/=2.0f;

//		glTranslatef( uvw + uv_trans.x , uvh + uv_trans.y , 0 );
//		glRotatef( state->uvRotation, 0.0, 0.0, 1.0);
		TranslationMatrix( transMat, uvw + uv_trans.x, uvh + uv_trans.y, 0 );
		Matrix4RotationZ( rotateMat, state->uvRotation * Math_PI / 180.0 );

		float	uvsh = state->uvScale.x;
		float	uvsv = state->uvScale.y;

		if ( state->imageFlipH ) uvsh*=-1;
		if ( state->imageFlipV ) uvsv*=-1;

//		glScalef( uvsh , uvsv , 0.0);
//		glTranslatef( -uvw , -uvh , 0 );
		ScaleMatrix( scaleMat, uvsh, uvsv, 0.0 );
		TranslationMatrix( anchorMat, -uvw, -uvh, 0 );
		MultiplyMatrix( anchorMat, scaleMat, texMat );
		MultiplyMatrix( texMat, rotateMat, texMat );
		MultiplyMatrix( texMat, transMat, texMat );

//		glMatrixMode(GL_MODELVIEW);

		// オリジナルUVをまず指定する
		// 左下が 0,0 Ｚ字順
		static const int sUvOrders[][4] = {
			{0, 1, 2, 3},	// フリップなし
			{1, 0, 3, 2},	// 水平フリップ
			{2, 3, 0, 1},	// 垂直フリップ
			{3, 2, 1, 0},	// 両方フリップ
		};

		//memset( state->uvs , 0 , sizeof(float) * 10 );

		// イメージのみのフリップ対応

		int order = ( state->hFlip == true ? 1 : 0 ) + ( state->vFlip == true ? 1 :0 ) * 2;
		float	uvs[10];
		memset(uvs, 0, sizeof(float) * 10);
		const int * uvorder = &sUvOrders[order][0];
		if (texture_is_pow2)
		{
			fPixTX = 1.0f / texturePixelSize.x;
			fPixTY = 1.0f / texturePixelSize.y;
		}
		fCoordCU = 0.0f;
		fCoordCV = 0.0f;
		for (int i = 0; i < 4; ++i)
		{
			int idx = uvorder[i];
			if (texture_is_pow2 == false)
			{
				// GL_TEXTURE_RECTANGLE_ARBではuv座標系が0～1ではなくピクセルになるので変換
				uvs[idx * 2] = state->cellValue.uvs[i].x * texturePixelSize.x;
				uvs[idx * 2 + 1] = state->cellValue.uvs[i].y * texturePixelSize.y;

#if SPRITESTUDIO6SDK_USE_TRIANGLE_FIN
				//きれいな頂点変形への対応
				uvs[4 * 2] += uvs[idx * 2];
				uvs[4 * 2 + 1] += uvs[idx * 2 + 1];
#endif
			}
			else
			{
				uvs[idx * 2] = state->cellValue.uvs[i].x;
				uvs[idx * 2 + 1] = state->cellValue.uvs[i].y;

#if SPRITESTUDIO6SDK_USE_TRIANGLE_FIN
				//きれいな頂点変形への対応
				uvs[4 * 2] += uvs[idx * 2];
				uvs[4 * 2 + 1] += uvs[idx * 2 + 1];
#endif
			}

			// スコープを抜けた後の範囲外読み取りの修正
			//++uvorder;
		}

		for (int i = 0; i < 4; ++i)
		{
			int idx = uvorder[i];
			SsVector3	vertexIn;
			SsVector3	vertexOut;

			vertexIn.x = uvs[idx * 2];
			vertexIn.y = uvs[idx * 2 + 1];
			vertexIn.z = 1.0f;

			MatrixTransformVector3( texMat, vertexIn, vertexOut );
			uvs[idx * 2] = vertexOut.x;
			uvs[idx * 2 + 1] = vertexOut.y;
		}

		for (int i = 0; i < 4; ++i)
		{
			int idx = uvorder[i];
			if ( i == 0 ) {
				fCoordLU = uvs[idx * 2];
				fCoordTV = uvs[idx * 2 + 1];
				fCoordRU = uvs[idx * 2];
				fCoordBV = uvs[idx * 2 + 1];
			}else{
				fCoordLU = fCoordLU > uvs[idx * 2] ? uvs[idx * 2] : fCoordLU;
				fCoordTV = fCoordTV > uvs[idx * 2 + 1] ? uvs[idx * 2 + 1] : fCoordTV;
				fCoordRU = fCoordRU < uvs[idx * 2] ? uvs[idx * 2] : fCoordRU;
				fCoordBV = fCoordBV < uvs[idx * 2 + 1] ? uvs[idx * 2 + 1] : fCoordBV;
			}

			fCoordCU += uvs[idx * 2];
			fCoordCV += uvs[idx * 2 + 1];
		}

#if SPRITESTUDIO6SDK_USE_TRIANGLE_FIN
		//きれいな頂点変形への対応
		uvs[4*2]/=4.0f;
		uvs[4*2+1]/=4.0f;

#endif

		fCoordCU *= 0.25f;
		fCoordCV *= 0.25f;

		// UV 配列を指定する
//		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
//		glTexCoordPointer(2, GL_FLOAT, 0, (GLvoid *)uvs);
		for ( int i =0 ; i < 10 ; i ++ ) state->uvs[i] = uvs[i];
	}else{
		//セルがない状態というのは本当は有効な状態ではないような気がするがNANが入るので何かを入れる
		for ( int i =0 ; i < 10 ; i ++ ) state->uvs[i] = 0;

//		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
//		glTexCoordPointer(2, GL_FLOAT, 0, (GLvoid *)state->uvs);
		//テクスチャのバインドを解除する　状況が不定になるため
//		glBindTexture(gl_target, 0);
	}

	// αブレンドの設定
	SetAlphaBlendMode( state->alphaBlendType );

	//メッシュの場合描画
	if (state->partType == SsPartType::mesh)
	{
		this->renderMesh(state->meshPart.get() , alpha );
		return;
	}

	// パーツカラーの指定
	if (state->is_parts_color)
	{
		partsColorEnabled = true;
		//6.2対応　パーツカラー、頂点、ミックス時に不透明度を適用する
		// パーツカラーがある時だけブレンド計算する
		if (state->partsColorValue.target == SsColorBlendTarget::whole)
		{
			// 単色
			const SsColorBlendValue& cbv = state->partsColorValue.color;
			setupSimpleTextureCombiner_for_PartsColor_(state->partsColorValue.blendType, cbv.rate, state->partsColorValue.target);
			rgbaByteToFloat_(state->colors, cbv);
			state->colors[0 + 3] *= alpha; // 不透明度を掛ける
			rates[0] = 1.0f;
			vertexID[0] = 0;

			// 残り３つは先頭のをコピー
			for (int i = 1; i < 5; ++i)
			{
				memcpy(state->colors + i * 4, state->colors, sizeof(state->colors[0]) * 4);
				rates[i] = 1.0f;
				vertexID[i * 2] = 0;
				vertexID[i * 2 + 1] = 0;
				//vertexID[i] = 0;
			}
		}
		else
		{
			// 頂点単位
			setupSimpleTextureCombiner_for_PartsColor_(state->partsColorValue.blendType, alpha, state->partsColorValue.target); // ミックス用に const に不透明度を入れておく。

			for (int i = 0; i < 4; ++i)
			{
				const SsColorBlendValue& cbv = state->partsColorValue.colors[i];
				rgbaByteToFloat_(state->colors + i * 4, cbv);

				// 不透明度も掛ける (ミックスの場合、Rate として扱われるので不透明度を掛けてはいけない)
				if (state->partsColorValue.blendType != SsBlendType::mix)
				{
					state->colors[i * 4 + 3] *= alpha;
				}

				rates[i] = 1.0f;
				vertexID[i * 2] = i;
				vertexID[i * 2 + 1] = i;
			}

#if SPRITESTUDIO6SDK_USE_TRIANGLE_FIN
			// 中央頂点(index=4)のRGBA%値の計算
			calcCenterVertexColor(state->colors, rates, vertexID);
#else
			// クリアしとく
			state->colors[4 * 4 + 0] = 0;
			state->colors[4 * 4 + 1] = 0;
			state->colors[4 * 4 + 2] = 0;
			state->colors[4 * 4 + 3] = 0;
			rates[4] = 0;
#endif
		}
	}
	else
	{
		// パーツカラー無し
		calcCenterVertexColor(state->colors, rates, vertexID);
		for (int i = 0; i < 5; ++i)
			state->colors[i * 4 + 3] = alpha;

		// RGB=100%テクスチャ、A=テクスチャｘ頂点カラーの設定にする。
		setupTextureCombinerTo_NoBlendRGB_MultiplyAlpha_();
	}


//	glEnableClientState(GL_COLOR_ARRAY);
//	glColorPointer(4, GL_FLOAT, 0, (GLvoid *)state->colors);


	// 頂点バッファの設定
//	glEnableClientState(GL_VERTEX_ARRAY);

//	glVertexPointer(3, GL_FLOAT, 0, (GLvoid *)state->vertices);

//	glPushMatrix();

	// update で計算しておいた行列をロード
//	glMatrixMode(GL_MODELVIEW);
//	glLoadMatrixf(state->matrix);
//	glLoadMatrixf(state->matrixLocal);	//Ver6 ローカルスケール対応

//	GLint VertexLocation = -1;
	if (state->noCells)
	{
		//セルが無いので描画を行わない
	}else{

#if SPRITESTUDIO6SDK_PROGRAMABLE_SHADER_ON

		if ( state->is_shader )
		{
//          if ( glpgObject )
			if ( pPrgObject )
			{
				GLint uid;
				int		type = (int)state->partsColorValue.blendType;

				if ( state->meshPart && cell && cell->ismesh )
				{
					int		iCount = state->meshPart->targetCell->meshPointList.size();

					if ( s_iVArgCount < iCount ) {
						clearShaderCache();
						s_pVArg = (ShaderVArg*)malloc( sizeof( ShaderVArg ) * iCount );
						s_iVArgCount = iCount;
					}

					for ( int i = 0; i < iCount; i++ ) {
						s_pVArg[i].fSrcRatio = type <= 1 ? 1.0f - rates[0] : 1.0f;
						s_pVArg[i].fDstRatio = type == 3 ? -rates[0] : rates[0];
						s_pVArg[i].fDstSrcRatio = type == 1 ? 1.0f : 0.0f;
						s_pVArg[i].fReserved = 0.0f;
					}
				}else{
					int		iCount = 5;

					if ( s_iVArgCount < iCount ) {
						clearShaderCache();
						s_pVArg = (ShaderVArg*)malloc( sizeof( ShaderVArg ) * iCount );
						s_iVArgCount = iCount;
					}

					for ( int i = 0; i < iCount; i++ ) {
						s_pVArg[i].fSrcRatio = type <= 1 ? 1.0f - rates[i] : 1.0f;
						s_pVArg[i].fDstRatio = type == 3 ? -rates[i] : rates[i];
						s_pVArg[i].fDstSrcRatio = type == 1 ? 1.0f : 0.0f;
						s_pVArg[i].fReserved = 0.0f;
					}
				}

				VertexLocation = pPrgObject->GetAttribLocation( "varg" );
				if ( VertexLocation >= 0 ) {
					glVertexAttribPointer( VertexLocation , 4 , GL_FLOAT , GL_FALSE, 0, s_pVArg);//GL_FALSE→データを正規化しない
					glEnableVertexAttribArray(VertexLocation);//有効化
				}

				//シェーダのセットアップ
				pPrgObject->Enable();

				uid = pPrgObject->GetUniformLocation( "args" );
				if ( uid >= 0 ) {
					ShaderFArg	args;

					args.fTexW = fTexW;
					args.fTexH = fTexH;
					args.fPixTX = fPixTX;
					args.fPixTY = fPixTY;
					args.fCoordLU = fCoordLU;
					args.fCoordTV = fCoordTV;
					args.fCoordCU = fCoordCU;
					args.fCoordCV = fCoordCV;
					args.fCoordRU = fCoordRU;
					args.fCoordBV = fCoordBV;
					args.fPMA = 0.0f;
					args.fReserved1 = 0.0f;
					args.fReserved2 = 0.0f;
					args.fReserved3 = 0.0f;
					args.fReserved4 = 0.0f;
					args.fReserved5 = 0.0f;

					if (
						( !state->cellValue.texture )
						) {
						memset( &args, 0, sizeof( args ) );
					}

					glUniform1fv( uid, sizeof( args ) / sizeof( float ), (float*)&args );
				}

				uid = pPrgObject->GetUniformLocation( "params" );
				if ( uid >= 0 ) {
					glUniform1fv( uid, sizeof( state->shaderValue.param ) / sizeof( float ), state->shaderValue.param );
				}
			}
		}

#endif
#if SPRITESTUDIO6SDK_USE_TRIANGLE_FIN
		if ( state->is_vertex_transform || state->is_parts_color)
		{
//			static const GLubyte indices[] = { 4 , 3, 1, 0, 2 , 3};
//			glDrawElements(GL_TRIANGLE_FAN, 6, GL_UNSIGNED_BYTE, indices);
		}else{
//			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		}
#else
		// 頂点配列を描画
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

#endif
	}

#if SPRITESTUDIO6SDK_PROGRAMABLE_SHADER_ON
//	if ( glpgObject )
	{
		if ( state->is_shader )
		{
//			if ( glpgObject )
			if ( pPrgObject )
			{
				if ( VertexLocation >= 0 ) {
					glDisableVertexAttribArray(VertexLocation);//無効化
				}
				pPrgObject->Disable();
			}
		}
	}
#endif

//	glPopMatrix();

	if (texture_is_pow2 == false)
	{
//		glDisable(gl_target);
	}
//	glDisable(GL_TEXTURE_2D);

	//ブレンドモード　減算時の設定を戻す
//	glBlendEquation( GL_FUNC_ADD );

	_args[ 0] = fTexW;
	_args[ 1] = fTexH;
	_args[ 2] = fPixTX;
	_args[ 3] = fPixTY;
	_args[ 4] = fCoordLU;
	_args[ 5] = fCoordTV;
	_args[ 6] = fCoordCU;
	_args[ 7] = fCoordCV;
	_args[ 8] = fCoordRU;
	_args[ 9] = fCoordBV;
	_args[10] = 0.0f;
	_args[11] = 0.0f;
	_args[12] = 0.0f;
	_args[13] = 0.0f;
	_args[14] = 0.0f;
	_args[15] = 0.0f;
}

void SsRendererImpl::freePartSprites()
{
	VisualServer*		pVisualServer = VisualServer::get_singleton();

	for ( int i = 0; i < m_vecPartSprite.size(); i++ ) {
		auto	e = m_vecPartSprite[i];

		if ( e->colorCanvasItemId.is_valid() )
#ifdef SPRITESTUDIO_GODOT_EXTENSION
			pVisualServer->free_rid( e->colorCanvasItemId );
#else
			pVisualServer->free( e->colorCanvasItemId );
#endif

		if ( e->alphaCanvasItemId.is_valid() )
#ifdef SPRITESTUDIO_GODOT_EXTENSION
		pVisualServer->free_rid( e->colorCanvasItemId );
#else
		pVisualServer->free( e->alphaCanvasItemId );
#endif

		for ( int j = 0; j < e->vecChildColorCanvasItemId.size(); j++ ) {
			auto	e2 = e->vecChildColorCanvasItemId[j];

			if ( e2.is_valid() )
#ifdef SPRITESTUDIO_GODOT_EXTENSION
				pVisualServer->free_rid( e2 );
#else
				pVisualServer->free( e2 );
#endif
		}

		for ( int j = 0; j < e->vecChildAlphaCanvasItemId.size(); j++ ) {
			auto	e2 = e->vecChildAlphaCanvasItemId[j];

			if ( e2.is_valid() )
#ifdef SPRITESTUDIO_GODOT_EXTENSION
				pVisualServer->free_rid( e2 );
#else
				pVisualServer->free( e2 );
#endif
		}

		if ( e->materialId.is_valid() )
#ifdef SPRITESTUDIO_GODOT_EXTENSION
			pVisualServer->free_rid( e->materialId );
#else
			pVisualServer->free( e->materialId );
#endif
		if ( e->shaderId.is_valid() )
#ifdef SPRITESTUDIO_GODOT_EXTENSION
			pVisualServer->free_rid( e->shaderId );
#else
			pVisualServer->free( e->materialId );
#endif

		delete e;
	}

	for ( auto it = m_mapPartSprite.begin(); it != m_mapPartSprite.end(); it++ ) {
		auto	e = it->second;

		for ( auto it2 = e.begin(); it2 != e.end(); it2++ ) {
			auto	ee = *it2;

			if ( ee->colorCanvasItemId.is_valid() )
#ifdef SPRITESTUDIO_GODOT_EXTENSION
				pVisualServer->free_rid( ee->colorCanvasItemId );
#else
				pVisualServer->free( ee->colorCanvasItemId );
#endif
			if ( ee->alphaCanvasItemId.is_valid() )
#ifdef SPRITESTUDIO_GODOT_EXTENSION
				pVisualServer->free_rid( ee->alphaCanvasItemId );
#else
				pVisualServer->free( ee->alphaCanvasItemId );
#endif

			for ( int j = 0; j < ee->vecChildColorCanvasItemId.size(); j++ ) {
				auto	e2 = ee->vecChildColorCanvasItemId[j];

				if ( e2.is_valid() )
#ifdef SPRITESTUDIO_GODOT_EXTENSION
				pVisualServer->free_rid( e2 );
#else
				pVisualServer->free( e2 );
#endif
			}

			for ( int j = 0; j < ee->vecChildAlphaCanvasItemId.size(); j++ ) {
				auto	e2 = ee->vecChildAlphaCanvasItemId[j];

				if ( e2.is_valid() )
#ifdef SPRITESTUDIO_GODOT_EXTENSION
					pVisualServer->free_rid( e2 );
#else
					pVisualServer->free( e2 );
#endif
			}

			if ( ee->materialId.is_valid() )
#ifdef SPRITESTUDIO_GODOT_EXTENSION
				pVisualServer->free_rid( ee->materialId );
#else
				pVisualServer->free( ee->materialId );
#endif
			if ( ee->shaderId.is_valid() )
#ifdef SPRITESTUDIO_GODOT_EXTENSION
				pVisualServer->free_rid( ee->materialId );
#else
				pVisualServer->free( ee->shaderId );
#endif

			delete ee;
		}
	}

	m_pCreatePartSpritesSourceModel = NULL;
	m_vecPartSprite.clear();
	m_mapPartSprite.clear();
	m_mapPartSpriteIdx.clear();
}
