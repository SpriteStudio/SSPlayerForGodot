/*!
* \file		ss_renderer_impl.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef SS_RENDERER_IMPL_H
#define SS_RENDERER_IMPL_H

#include "gd_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/variant/dictionary.hpp>
using namespace godot;
#else
#ifdef GD_V4
#include "core/variant/dictionary.h"
#endif
#ifdef GD_V3
#include "core/dictionary.h"
#endif
#endif

#include "SpriteStudio6-SDK/Common/Loader/ssloader_ssae.h"

#include "SpriteStudio6-SDK/Common/Animator/ssplayer_render.h"
#include "SpriteStudio6-SDK/Common/Animator/ssplayer_mesh.h"
#include "SpriteStudio6-SDK/Common/Animator/ssplayer_animedecode.h"

#include "ss_macros.h"
#include "gd_renderer.h"

SsSdkUsing

class SsRendererImpl : public ISsRenderer
{
public :
	int m_iSetup;
	SsRendererImpl();
	virtual ~SsRendererImpl();

	void draw( RID rid );

	void setCanvasItem( RID rid );
	void setCanvasSize( float fWidth, float fHeight );
	void setCanvasCenter( float fX, float fY );
	void setFps( int iFps );
	int getFps() const;
	void setTextureInterpolate( bool bSwitch );
	bool getTextureInterpolate() const;

	void createPartSprites( SsModel* pModel, SsProject* pProject );
	void updateUserData( SsAnimeDecoder* pDecoder );

	const std::map<int,SsUserDataAnime>& getUserData() const;
	const std::map<int,SsSignalAttr>& getSignal() const;

	virtual void initialize() override;

	virtual void renderSetup( SsAnimeDecoder* state ) override;
	virtual void renderPart( SsPartState* state ) override;
	virtual void renderMesh( SsMeshPart* mesh, float alpha );

	virtual void execMask( SsPartState* state ) override;
	virtual void clearMask() override;

	virtual void renderSpriteSimple( float matrix[16], int width, int height, SsVector2& pivot, SsVector2 uv1, SsVector2 uv2, const SsFColor& color ) override;

	virtual void SetAlphaBlendMode( SsBlendType::_enum type ) override;
	virtual void SetTexture( SsCellValue* cell ) override;

	virtual void enableMask( bool flag ) override;

private :
	struct PartSprite
	{
		RID					colorCanvasItemId;
		RID					alphaCanvasItemId;
		std::vector<RID>	vecChildColorCanvasItemId;
		std::vector<RID>	vecChildAlphaCanvasItemId;
		RID					materialId;
		RID					shaderId;
		SsBlendType::_enum	currentBlendType;
		SsString			currentId;
	};

	RID							m_CanvasItemId;
	float						m_fCanvasWidth;
	float						m_fCanvasHeight;
	float						m_fCanvasX;
	float						m_fCanvasY;
	int							m_iFps;

	int							m_iZOrder;
	int							m_iChildZOrder;

	bool						m_bTextureInterpolate;

	GdRenderer					m_RendererColor;
	GdRenderer					m_RendererAlpha;

	SsModel*					m_pCreatePartSpritesSourceModel;
	std::vector<PartSprite*>	m_vecPartSprite;
	std::map<SsPart*,std::vector<PartSprite*>>	m_mapPartSprite;
	std::map<SsPart*,int>		m_mapPartSpriteIdx;
	Dictionary					m_dicShader;
	std::map<int,SsUserDataAnime>	m_mapUserData;
	std::map<int,SsSignalAttr>		m_mapSignal;

	SsPartState*				m_pPartStateDrawing;
	SsCellValue*				m_pCellValueDrawing;
	SsBlendType::_enum			m_eBlendTypeDrawing;

	float	_rates[5];
	float	_args[16];

	void createPartSprites2( SsModel* pModel, SsProject* pProject, SsPart* pPart, PartSprite* pParent );

	void updatePartStateDrawing( SsPartState* pPartState );
	void updateCellValueDrawing( SsCellValue* pCellValue );
	void updateBlendTypeDrawing( SsBlendType::_enum eBlendType );

	void updateShaderSource( PartSprite& sprite, SsBlendType::_enum eBlendType, SsString strId );

	void makePrimitive( SsPartState* state );

	void freePartSprites();
};

#endif // SS_RENDERER_IMPL_H
