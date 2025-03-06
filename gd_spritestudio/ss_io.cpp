/*!
* \file		ss_io.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "ss_io.h"

#include "gd_packet_ssproject.h"
#include "gd_packet_ssanimepack.h"
#include "gd_packet_sscellmap.h"
#include "gd_packet_sseffect.h"

#if defined(GD_V4) || defined(SPRITESTUDIO_GODOT_EXTENSION)
#define	POOL_BYTE_ARRAY	PACKED_BYTE_ARRAY
#endif
#ifdef GD_V3
#endif

SsIO::SsIO()
{
}

SsIO::~SsIO()
{
}

Variant SsIO::toProjectVariant( const String& strRaw )
{
	Variant					val;
	GdPacketSsProject		packet;

	packet.write( strRaw );

	val = packet.getBytes();

	return	val;
}

Variant SsIO::toAnimePackVariant( const String& strRaw )
{
	Variant					val;
	GdPacketSsAnimePack		packet;

	packet.write( strRaw );

	val = packet.getBytes();

	return	val;
}

Variant SsIO::toCellMapVariant( const String& strRaw )
{
	Variant					val;
	GdPacketSsCellMap		packet;

	packet.write( strRaw );

	val = packet.getBytes();

	return	val;
}

Variant SsIO::toEffectVariant( const String& strRaw )
{
	Variant					val;
	GdPacketSsEffect		packet;

	packet.write( strRaw );

	val = packet.getBytes();

	return	val;
}

SsProject* SsIO::createProjectFromVariant( const Variant& val )
{
	if ( val.get_type() != Variant::POOL_BYTE_ARRAY ) {
		return	NULL;
	}

	GdPacketSsProject		packet;
	SsProject*				pProject = new SsProject();

	packet.read( pProject, val );

	return	pProject;
}

SsAnimePack* SsIO::createAnimePackFromVariant( const Variant& val )
{
	if ( val.get_type() != Variant::POOL_BYTE_ARRAY ) {
		return	NULL;
	}

	GdPacketSsAnimePack		packet;
	SsAnimePack*			pAnimePack = new SsAnimePack();

	packet.read( pAnimePack, val );

	return	pAnimePack;
}

SsCellMap* SsIO::createCellMapFromVariant( const Variant& val )
{
	if ( val.get_type() != Variant::POOL_BYTE_ARRAY ) {
		return	NULL;
	}

	GdPacketSsCellMap		packet;
	SsCellMap*				pCellMap = new SsCellMap();

	packet.read( pCellMap, val );

	return	pCellMap;
}

SsEffectFile* SsIO::createEffectFromVariant( const Variant& val )
{
	if ( val.get_type() != Variant::POOL_BYTE_ARRAY ) {
		return	NULL;
	}

	GdPacketSsEffect		packet;
	SsEffectFile*			pEffect = new SsEffectFile();

	packet.read( pEffect, val );

	return	pEffect;
}

bool SsIO::push( StreamPeerBuffer& st, SsProject& project )
{
	push( st, project.version );
	push( st, project.settings );
	push( st, project.cellmapNames );
	push( st, project.animepackNames );
	push( st, project.effectFileNames );
	push( st, project.sequencepackNames );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsAnimePack& animePack )
{
	push( st, animePack.version );
	push( st, animePack.settings );
	push( st, animePack.name );
	push( st, animePack.Model );
	push( st, animePack.cellmapNames );
	push( st, animePack.animeList );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsCellMap& cellMap )
{
	push( st, cellMap.version );
	push( st, cellMap.name );
	push( st, cellMap.imagePath );
	push( st, cellMap.pixelSize );
	push( st, cellMap.overrideTexSettings );
	push( st, cellMap.wrapMode );
	push( st, cellMap.filterMode );

	push( st, cellMap.cells );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsEffectFile& effect )
{
	push( st, effect.version );
	push( st, effect.name );
	push( st, effect.effectData );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsProjectSetting& setting )
{
	push( st, setting.animeBaseDirectory );
	push( st, setting.cellMapBaseDirectory );
	push( st, setting.imageBaseDirectory );
	push( st, setting.effectBaseDirectory );

	push( st, setting.exportBaseDirectory );
	push( st, setting.queryExportBaseDirectory );
	push( st, setting.wrapMode );
	push( st, setting.filterMode );
	push( st, setting.vertexAnimeFloat );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsAnimationSettings& setting )
{
	push( st, setting.fps );
	push( st, setting.frameCount );
	push( st, setting.canvasSize );
	push( st, setting.pivot );
	push( st, setting.sortMode );
	push( st, setting.startFrame );
	push( st, setting.endFrame );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsModel& model )
{
	push( st, model.partList );
	push( st, model.boneList );
	push( st, model.meshList );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsEffectModel& model )
{
	push( st, model.lockRandSeed );
	push( st, model.isLockRandSeed );
	push( st, model.fps );
	push( st, model.bgcolor );
	push( st, model.layoutScaleX );

	push( st, model.layoutScaleY );

	push( st, model.nodeList );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsTexWrapMode::_enum& eMode )
{
	int8_t	cVal = eMode;

	push( st, cVal );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsTexFilterMode::_enum& eMode )
{
	int8_t	cVal = eMode;

	push( st, cVal );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsPartsSortMode::_enum& eMode )
{
	int8_t	cVal = eMode;

	push( st, cVal );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsPartType::_enum& eType )
{
	int8_t	cVal = eType;

	push( st, cVal );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsBoundsType::_enum& eType )
{
	int8_t	cVal = eType;

	push( st, cVal );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsInheritType::_enum& eType )
{
	int8_t	cVal = eType;

	push( st, cVal );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsBlendType::_enum& eType )
{
	int8_t	cVal = eType;

	push( st, cVal );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsIkRotationArrow::_enum& eArrow )
{
	int8_t	cVal = eArrow;

	push( st, cVal );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsAttributeKind::_enum& eKind )
{
	int8_t	cVal = eKind;

	push( st, cVal );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsInterpolationType::_enum& eType )
{
	int8_t	cVal = eType;

	push( st, cVal );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsMeshDivType::_enum& eType )
{
	int8_t	cVal = eType;

	push( st, cVal );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsEffectNodeType::_enum& eType )
{
	int8_t	cVal = eType;

	push( st, cVal );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsRenderBlendType::_enum& eType )
{
	int8_t	cVal = eType;

	push( st, cVal );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsEffectFunctionType::enum_& eType )
{
	int8_t	cVal = eType;

	push( st, cVal );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, std::map<SsString,int>& map )
{
	int32_t		iSize = map.size();

	push( st, iSize );

	for ( auto it = map.begin(); it != map.end(); it++ ) {
		SsString	strKey = it->first;
		int			iValue = it->second;

		push( st, strKey );
		push( st, iValue );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, std::vector<SsAnimation*>& vec )
{
	int32_t		iSize = vec.size();

	push( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsAnimation*	pAnimation = vec.at( i );

		push( st, *pAnimation );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, std::vector<SsPart*>& vec )
{
	int32_t		iSize = vec.size();

	push( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsPart*			pPart = vec.at( i );

		push( st, *pPart );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, std::vector<SsMeshBind*>& vec )
{
	int32_t		iSize = vec.size();

	push( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsMeshBind*		pBind = vec.at( i );

		push( st, *pBind );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, std::vector<SsMeshBindInfo>& vec )
{
	int32_t		iSize = vec.size();

	push( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsMeshBindInfo	info = vec.at( i );

		push( st, info );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, std::vector<SsLabel*>& vec )
{
	int32_t		iSize = vec.size();

	push( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsLabel*		pLabel = vec.at( i );

		push( st, *pLabel );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, std::vector<SsPartAnime*>& vec )
{
	int32_t		iSize = vec.size();

	push( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsPartAnime*	pPartAnime = vec.at( i );

		push( st, *pPartAnime );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, std::vector<SsAttribute*>& vec )
{
	int32_t		iSize = vec.size();

	push( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsAttribute*	pAttribute = vec.at( i );

		push( st, *pAttribute );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, std::vector<SsKeyframe*>& vec )
{
	int32_t		iSize = vec.size();

	push( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsKeyframe*		pKeyframe = vec.at( i );

		push( st, *pKeyframe );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, std::vector<SsCell*>& vec )
{
	int32_t		iSize = vec.size();

	push( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsCell*			pCell = vec.at( i );

		push( st, *pCell );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, std::vector<SsEffectNode*>& vec )
{
	int32_t		iSize = vec.size();

	push( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsEffectNode*	pEffectNode = vec.at( i );

		push( st, *pEffectNode );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, std::vector<SsString>& vec )
{
	int32_t		iSize = vec.size();

	push( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsString	str = vec.at( i );

		push( st, str );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, std::vector<SsPoint2>& vec )
{
	int32_t		iSize = vec.size();

	push( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsPoint2	p = vec.at( i );

		push( st, p );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, std::vector<SsTriangle>& vec )
{
	int32_t		iSize = vec.size();

	push( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsTriangle	tri = vec.at( i );

		push( st, tri );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, VarianceValue<float>& val )
{
	float	v1 = val.getMinValue();
	float	v2 = val.getMaxValue();

	if ( val.isTypeNone() ) {
		push( st, v1 );
	}else
	if ( val.isTypeMinMax() ) {
		push( st, v1 );
		push( st, v2 );
	}else
	if ( val.isTypePlusMinus() ) {
		push( st, v2 );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, VarianceValue<int>& val )
{
	int		v1 = val.getMinValue();
	int		v2 = val.getMaxValue();

	if ( val.isTypeNone() ) {
		push( st, v1 );
	}else
	if ( val.isTypeMinMax() ) {
		push( st, v1 );
		push( st, v2 );
	}else
	if ( val.isTypePlusMinus() ) {
		push( st, v2 );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, VarianceValue<SsU8Color>& val )
{
	SsU8Color	v1 = val.getMinValue();
	SsU8Color	v2 = val.getMaxValue();

	if ( val.isTypeNone() ) {
		push( st, v1 );
	}else
	if ( val.isTypeMinMax() ) {
		push( st, v1 );
		push( st, v2 );
//	}else
//	if ( val.isTypePlusMinus() ) {
//		push( st, v2 );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsTColor<u8>& col )
{
	push( st, col.r );
	push( st, col.g );
	push( st, col.b );
	push( st, col.a );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsAnimation& animation )
{
	push( st, animation.name );
	push( st, animation.settings );
	push( st, animation.labels );
	push( st, animation.partAnimes );
	push( st, animation.isSetup );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsPart& part )
{
	push( st, part.name );
	push( st, part.arrayIndex );
	push( st, part.parentIndex );

	push( st, part.type );
	push( st, part.boundsType );
	push( st, part.inheritType );
	push( st, part.alphaBlendType );
	push( st, part.show );
	push( st, part.locked );
	push( st, part.colorLabel );
	push( st, part.maskInfluence );

	push( st, part.refAnimePack );
	push( st, part.refAnime );

	push( st, part.refEffectName );

	push( st, part.boneLength );
	push( st, part.bonePosition );
	push( st, part.boneRotation );
	push( st, part.weightPosition );
	push( st, part.weightImpact );
	push( st, part.meshWeightType );
	push( st, part.meshWeightStrong );
	push( st, part.IKDepth );
	push( st, part.IKRotationArrow );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsMeshBind& bind )
{
	push( st, bind.meshVerticesBindArray );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsMeshBindInfo& info )
{
	push( st, info.bindBoneNum );

	for ( int i = 0; i < info.bindBoneNum; i++ ) {
		push( st, info.boneIndex[i] );
		push( st, info.weight[i] );
		push( st, info.offset[i].x );
		push( st, info.offset[i].y );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsLabel& label )
{
	push( st, label.name );
	push( st, label.time );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsPartAnime& anime )
{
	push( st, anime.partName );
	push( st, anime.attributes );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsAttribute& attribute )
{
	push( st, attribute.tag );
	push( st, attribute.key );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsKeyframe& keyframe )
{
	push( st, keyframe.time );
	push( st, keyframe.ipType );

	if ( SsNeedsCurveParams( keyframe.ipType ) ) {
		push( st, keyframe.curve );
	}

	push( st, keyframe.value );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsCurve& curve )
{
	push( st, curve.startTime );
	push( st, curve.startValue );
	push( st, curve.endTime );
	push( st, curve.endValue );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsValue& value )
{
	SsValue&	v = value;

	push( st, v.type );
	push( st, v.org_txt );

	switch( v.type ) {
	case SsValue::string_type :
		push( st, *v._str );
		break;
	case SsValue::int_type :
		push( st, v._int );
		break;
	case SsValue::float_type :
		push( st, v._float );
		break;
	case SsValue::boolean_type :
		push( st, v._bool );
		break;
	case SsValue::array_type :
		push( st, *v._array );
		break;
	case SsValue::hash_type :
		push( st, *v._hash );
		break;
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsArray& arr )
{
	int32_t		iSize = arr.size();

	push( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsValue		v = arr.at( i );

		push( st, v );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsHash& hash )
{
	int32_t		iSize = hash.size();

	push( st, iSize );

	for ( auto it = hash.begin(); it != hash.end(); it++ ) {
		SsString	strKey = it->first;
		SsValue		v = it->second;

		push( st, strKey );
		push( st, v );
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsCell& cell )
{
	push( st, cell.name );
	push( st, cell.pos );
	push( st, cell.size );
	push( st, cell.pivot );
	push( st, cell.rotated );

	push( st, cell.ismesh );

	push( st, cell.innerPoint );
	push( st, cell.outerPoint );
	push( st, cell.meshPointList );
	push( st, cell.meshTriList );
	push( st, cell.divtype );
	push( st, cell.divw );
	push( st, cell.divh );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsEffectNode& node )
{
	push( st, node.arrayIndex );
	push( st, node.parentIndex );
	push( st, node.type );
	push( st, node.visible );
	push( st, node.behavior );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsEffectBehavior& behavior )
{
	push( st, behavior.CellName );
	push( st, behavior.CellMapName );
	push( st, behavior.BlendType );

	int32_t		iSize = behavior.plist.size();

	push( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsEffectElementBase*			v = behavior.plist.at( i ).get();
		SsEffectFunctionType::enum_		eType = v->myType;

		push( st, eType );

		switch ( static_cast<int>(eType) ) {
		case SsEffectFunctionType::Basic :
			{
				ParticleElementBasic*			w = (ParticleElementBasic*)v;

				push( st, w->maximumParticle );
				push( st, w->speed );
				push( st, w->lifespan );
				push( st, w->angle );
				push( st, w->angleVariance );
				push( st, w->interval );
				push( st, w->lifetime );
				push( st, w->attimeCreate );
				push( st, w->priority );
			}
			break;
		case SsEffectFunctionType::RndSeedChange :
			{
				ParticleElementRndSeedChange*	w = (ParticleElementRndSeedChange*)v;

				push( st, w->Seed );
			}
			break;
		case SsEffectFunctionType::Delay :
			{
				ParticleElementDelay*			w = (ParticleElementDelay*)v;

				push( st, w->DelayTime );
			}
			break;
		case SsEffectFunctionType::Gravity :
			{
				ParticleElementGravity*			w = (ParticleElementGravity*)v;

				push( st, w->Gravity );
			}
			break;
		case SsEffectFunctionType::Position :
			{
				ParticleElementPosition*		w = (ParticleElementPosition*)v;

				push( st, w->OffsetX );
				push( st, w->OffsetY );
			}
			break;
		case SsEffectFunctionType::Rotation :
			{
				ParticleElementRotation*		w = (ParticleElementRotation*)v;

				push( st, w->Rotation );
				push( st, w->RotationAdd );
			}
			break;
		case SsEffectFunctionType::TransRotation :
			{
				ParticleElementRotationTrans*	w = (ParticleElementRotationTrans*)v;

				push( st, w->RotationFactor );
				push( st, w->EndLifeTimePer );
			}
			break;
		case SsEffectFunctionType::TransSpeed :
			{
				ParticleElementTransSpeed*		w = (ParticleElementTransSpeed*)v;

				push( st, w->Speed );
			}
			break;
		case SsEffectFunctionType::TangentialAcceleration :
			{
				ParticleElementTangentialAcceleration*	w = (ParticleElementTangentialAcceleration*)v;

				push( st, w->Acceleration );
			}
			break;
		case SsEffectFunctionType::InitColor :
			{
				ParticleElementInitColor*		w = (ParticleElementInitColor*)v;

				push( st, w->Color );
			}
			break;
		case SsEffectFunctionType::TransColor :
			{
				ParticleElementTransColor*		w = (ParticleElementTransColor*)v;

				push( st, w->Color );
			}
			break;
		case SsEffectFunctionType::AlphaFade :
			{
				ParticleElementAlphaFade*		w = (ParticleElementAlphaFade*)v;

				push( st, w->disprange );
			}
			break;
		case SsEffectFunctionType::Size :
			{
				ParticleElementSize*			w = (ParticleElementSize*)v;

				push( st, w->SizeX );
				push( st, w->SizeY );
				push( st, w->ScaleFactor );
			}
			break;
		case SsEffectFunctionType::TransSize :
			{
				ParticleElementTransSize*		w = (ParticleElementTransSize*)v;

				push( st, w->SizeX );
				push( st, w->SizeY );
				push( st, w->ScaleFactor );
			}
			break;
		case SsEffectFunctionType::PointGravity :
			{
				ParticlePointGravity*			w = (ParticlePointGravity*)v;

				push( st, w->Position );
				push( st, w->Power );
			}
			break;
		case SsEffectFunctionType::TurnToDirectionEnabled :
			{
				ParticleTurnToDirectionEnabled*		w = (ParticleTurnToDirectionEnabled*)v;

				push( st, w->Rotation );
			}
			break;
		case SsEffectFunctionType::InfiniteEmitEnabled :
			{
				ParticleInfiniteEmitEnabled*	w = (ParticleInfiniteEmitEnabled*)v;
			}
			break;
		}
	}

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsString& str )
{
	st.put_utf8_string( str.c_str() );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsPoint2& p )
{
	st.put_float( p.x );
	st.put_float( p.y );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, SsTriangle& tri )
{
	st.put_32( tri.idxPo1 );
	st.put_32( tri.idxPo2 );
	st.put_32( tri.idxPo3 );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, int32_t& iVal )
{
	st.put_32( iVal );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, int8_t& cVal )
{
	st.put_8( cVal );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, u8& byVal )
{
	st.put_u8( byVal );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, float& fVal )
{
	st.put_float( fVal );

	return	true;
}

bool SsIO::push( StreamPeerBuffer& st, bool& bVal )
{
	int8_t	cVal = bVal ? 1 : 0;

	push( st, cVal );

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsProject& project )
{
	pull( st, project.version );
	pull( st, project.settings );
	pull( st, project.cellmapNames );
	pull( st, project.animepackNames );
	pull( st, project.effectFileNames );
	pull( st, project.sequencepackNames );

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsAnimePack& animePack )
{
	pull( st, animePack.version );
	pull( st, animePack.settings );
	pull( st, animePack.name );
	pull( st, animePack.Model );
	pull( st, animePack.cellmapNames );
	pull( st, animePack.animeList );

	//モデルにセットアップアニメーションを設定する
	int i;
	int size = (int)animePack.animeList.size();
	for (i = 0; i < size; i++)
	{
		SsAnimation* anime = animePack.animeList[i];
		if (anime->isSetup != 0)
		{
			animePack.Model.setupAnimation = anime;
			break;
		}
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsCellMap& cellMap )
{
	pull( st, cellMap.version );
	pull( st, cellMap.name );
	pull( st, cellMap.imagePath );
	pull( st, cellMap.pixelSize );
	pull( st, cellMap.overrideTexSettings );
	pull( st, cellMap.wrapMode );
	pull( st, cellMap.filterMode );

	pull( st, cellMap.cells );

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsEffectFile& effect )
{
	pull( st, effect.version );
	pull( st, effect.name );
	pull( st, effect.effectData );

	effect.effectData.effectName = effect.name;

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsProjectSetting& setting )
{
	pull( st, setting.animeBaseDirectory );
	pull( st, setting.cellMapBaseDirectory );
	pull( st, setting.imageBaseDirectory );
	pull( st, setting.effectBaseDirectory );

	pull( st, setting.exportBaseDirectory );
	pull( st, setting.queryExportBaseDirectory );
	pull( st, setting.wrapMode );
	pull( st, setting.filterMode );
	pull( st, setting.vertexAnimeFloat );

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsAnimationSettings& setting )
{
	pull( st, setting.fps );
	pull( st, setting.frameCount );
	pull( st, setting.canvasSize );
	pull( st, setting.pivot );
	pull( st, setting.sortMode );
	pull( st, setting.startFrame );
	pull( st, setting.endFrame );

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsModel& model )
{
	pull( st, model.partList );
	pull( st, model.boneList );
	pull( st, model.meshList );

	model.setupAnimation = NULL;

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsEffectModel& model )
{
	pull( st, model.lockRandSeed );
	pull( st, model.isLockRandSeed );
	pull( st, model.fps );
	pull( st, model.bgcolor );
	pull( st, model.layoutScaleX );
	if ( model.layoutScaleX == 0 ) model.layoutScaleX = 100;

	pull( st, model.layoutScaleY );
	if ( model.layoutScaleY == 0 ) model.layoutScaleY = 100;

	pull( st, model.nodeList );

	//ツリーの構築
	if ( model.nodeList.size() > 0 )
	{
//		model.root = model.nodeList[0];
		for ( size_t i = 1 ; i < model.nodeList.size() ; i++ )
		{
			int pi = model.nodeList[i]->parentIndex;
			if ( pi >= 0 )
			{
				model.nodeList[pi]->addChildEnd( model.nodeList[i] );
			}
		}
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsTexWrapMode::_enum& eMode )
{
	int8_t	cVal;

	pull( st, cVal );

	eMode = (SsTexWrapMode::_enum)cVal;

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsTexFilterMode::_enum& eMode )
{
	int8_t	cVal;

	pull( st, cVal );

	eMode = (SsTexFilterMode::_enum)cVal;

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsPartsSortMode::_enum& eMode )
{
	int8_t	cVal;

	pull( st, cVal );

	eMode = (SsPartsSortMode::_enum)cVal;

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsPartType::_enum& eType )
{
	int8_t	cVal;

	pull( st, cVal );

	eType = (SsPartType::_enum)cVal;

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsBoundsType::_enum& eType )
{
	int8_t	cVal;

	pull( st, cVal );

	eType = (SsBoundsType::_enum)cVal;

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsInheritType::_enum& eType )
{
	int8_t	cVal;

	pull( st, cVal );

	eType = (SsInheritType::_enum)cVal;

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsBlendType::_enum& eType )
{
	int8_t	cVal;

	pull( st, cVal );

	eType = (SsBlendType::_enum)cVal;

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsIkRotationArrow::_enum& eArrow )
{
	int8_t	cVal;

	pull( st, cVal );

	eArrow = (SsIkRotationArrow::_enum)cVal;

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsAttributeKind::_enum& eKind )
{
	int8_t	cVal;

	pull( st, cVal );

	eKind = (SsAttributeKind::_enum)cVal;

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsInterpolationType::_enum& eType )
{
	int8_t	cVal;

	pull( st, cVal );

	eType = (SsInterpolationType::_enum)cVal;

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsMeshDivType::_enum& eType )
{
	int8_t	cVal;

	pull( st, cVal );

	eType = (SsMeshDivType::_enum)cVal;

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsEffectNodeType::_enum& eType )
{
	int8_t	cVal;

	pull( st, cVal );

	eType = (SsEffectNodeType::_enum)cVal;

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsRenderBlendType::_enum& eType )
{
	int8_t	cVal;

	pull( st, cVal );

	eType = (SsRenderBlendType::_enum)cVal;

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsEffectFunctionType::enum_& eType )
{
	int8_t	cVal;

	pull( st, cVal );

	eType = (SsEffectFunctionType::enum_)cVal;

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, std::map<SsString,int>& map )
{
	int32_t		iSize;

	pull( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsString	strKey;
		int			iValue;

		pull( st, strKey );
		pull( st, iValue );

		map[strKey] = iValue;
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, std::vector<SsAnimation*>& vec )
{
	int32_t		iSize;

	pull( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsAnimation*	pAnimation = new SsAnimation();

		pull( st, *pAnimation );

		vec.push_back( pAnimation );
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, std::vector<SsPart*>& vec )
{
	int32_t		iSize;

	pull( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsPart*			pPart = new SsPart();

		pull( st, *pPart );

		vec.push_back( pPart );
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, std::vector<SsMeshBind*>& vec )
{
	int32_t		iSize;

	pull( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsMeshBind*		pBind = new SsMeshBind();

		pull( st, *pBind );

		vec.push_back( pBind );
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, std::vector<SsMeshBindInfo>& vec )
{
	int32_t		iSize;

	pull( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsMeshBindInfo	info;

		pull( st, info );

		vec.push_back( info );
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, std::vector<SsLabel*>& vec )
{
	int32_t		iSize;

	pull( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsLabel*		pLabel = new SsLabel();

		pull( st, *pLabel );

		vec.push_back( pLabel );
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, std::vector<SsPartAnime*>& vec )
{
	int32_t		iSize;

	pull( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsPartAnime*	pPartAnime = new SsPartAnime();

		pull( st, *pPartAnime );

		vec.push_back( pPartAnime );
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, std::vector<SsAttribute*>& vec )
{
	int32_t		iSize;

	pull( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsAttribute*	pAttribute = new SsAttribute();

		pull( st, *pAttribute );

		vec.push_back( pAttribute );
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, std::vector<SsKeyframe*>& vec )
{
	int32_t		iSize;

	pull( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsKeyframe*		pKeyframe = new SsKeyframe();

		pull( st, *pKeyframe );

		vec.push_back( pKeyframe );
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, std::vector<SsCell*>& vec )
{
	int32_t		iSize;

	pull( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsCell*			pCell = new SsCell();

		pull( st, *pCell );

		vec.push_back( pCell );
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, std::vector<SsEffectNode*>& vec )
{
	int32_t		iSize;

	pull( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsEffectNode*	pEffectNode = new SsEffectNode();

		pull( st, *pEffectNode );

		vec.push_back( pEffectNode );
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, std::vector<SsString>& vec )
{
	int32_t		iSize;

	pull( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsString	str;

		pull( st, str );

		vec.push_back( str );
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, std::vector<SsPoint2>& vec )
{
	int32_t		iSize;

	pull( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsPoint2	p;

		pull( st, p );

		vec.push_back( p );
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, std::vector<SsTriangle>& vec )
{
	int32_t		iSize;

	pull( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsTriangle	tri;

		pull( st, tri );

		vec.push_back( tri );
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, VarianceValue<float>& val )
{
	float	v1;
	float	v2;

	if ( val.isTypeNone() ) {
		pull( st, v1 );

		val = v1;
	}else
	if ( val.isTypeMinMax() ) {
		pull( st, v1 );
		pull( st, v2 );

		val.setMinMax( v1, v2 );
	}else
	if ( val.isTypePlusMinus() ) {
		pull( st, v2 );

		val.setPlusMinus( v2 );
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, VarianceValue<int>& val )
{
	int		v1;
	int		v2;

	if ( val.isTypeNone() ) {
		pull( st, v1 );

		val = v1;
	}else
	if ( val.isTypeMinMax() ) {
		pull( st, v1 );
		pull( st, v2 );

		val.setMinMax( v1, v2 );
	}else
	if ( val.isTypePlusMinus() ) {
		pull( st, v2 );

		val.setPlusMinus( v2 );
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, VarianceValue<SsU8Color>& val )
{
	SsU8Color	v1;
	SsU8Color	v2;

	if ( val.isTypeNone() ) {
		pull( st, v1 );

		val = v1;
	}else
	if ( val.isTypeMinMax() ) {
		pull( st, v1 );
		pull( st, v2 );

		val.setMinMax( v1, v2 );
//	}else
//	if ( val.isTypePlusMinus() ) {
//		pull( st, v2 );

//		val.setPlusMinus( v2 );
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsTColor<u8>& col )
{
	pull( st, col.r );
	pull( st, col.g );
	pull( st, col.b );
	pull( st, col.a );

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsAnimation& animation )
{
	pull( st, animation.name );
	pull( st, animation.settings );
	pull( st, animation.labels );
	pull( st, animation.partAnimes );
	pull( st, animation.isSetup );

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsPart& part )
{
	pull( st, part.name );
	pull( st, part.arrayIndex );
	pull( st, part.parentIndex );

	pull( st, part.type );
	pull( st, part.boundsType );
	pull( st, part.inheritType );
	pull( st, part.alphaBlendType );
	pull( st, part.show );
	pull( st, part.locked );
	pull( st, part.colorLabel );
	pull( st, part.maskInfluence );

	pull( st, part.refAnimePack );
	pull( st, part.refAnime );

	pull( st, part.refEffectName );

	pull( st, part.boneLength );
	pull( st, part.bonePosition );
	pull( st, part.boneRotation );
	pull( st, part.weightPosition );
	pull( st, part.weightImpact );
	pull( st, part.meshWeightType );
	pull( st, part.meshWeightStrong );
	pull( st, part.IKDepth );
	pull( st, part.IKRotationArrow );

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsMeshBind& bind )
{
	pull( st, bind.meshVerticesBindArray );

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsMeshBindInfo& info )
{
	pull( st, info.bindBoneNum );

	for ( int i = 0; i < info.bindBoneNum; i++ ) {
		pull( st, info.boneIndex[i] );
		pull( st, info.weight[i] );
		pull( st, info.offset[i].x );
		pull( st, info.offset[i].y );
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsLabel& label )
{
	pull( st, label.name );
	pull( st, label.time );

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsPartAnime& anime )
{
	pull( st, anime.partName );
	pull( st, anime.attributes );

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsAttribute& attribute )
{
	pull( st, attribute.tag );
	pull( st, attribute.key );

	attribute.key_dic.clear();
	for ( AttributeKeyList::iterator itr = attribute.key.begin() ; itr != attribute.key.end() ; itr++)
	{
		int time = (*itr)->time;
		SsKeyframe* key = (*itr);
		attribute.key_dic[time] = key;
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsKeyframe& keyframe )
{
	pull( st, keyframe.time );
	pull( st, keyframe.ipType );

	if ( SsNeedsCurveParams( keyframe.ipType ) ) {
		pull( st, keyframe.curve );
	}

	pull( st, keyframe.value );

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsCurve& curve )
{
	pull( st, curve.startTime );
	pull( st, curve.startValue );
	pull( st, curve.endTime );
	pull( st, curve.endValue );

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsValue& value )
{
	SsValue		v;

	pull( st, v.type );
	pull( st, v.org_txt );

	switch( v.type ) {
	case SsValue::string_type :
		v._str = new SsString();

		pull( st, *v._str );
		break;
	case SsValue::int_type :
		pull( st, v._int );
		break;
	case SsValue::float_type :
		pull( st, v._float );
		break;
	case SsValue::boolean_type :
		pull( st, v._bool );
		break;
	case SsValue::array_type :
		v._array = new SsArray();

		pull( st, *v._array );
		break;
	case SsValue::hash_type :
		v._hash = new SsHash();

		pull( st, *v._hash );
		break;
	}

	value = v;

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsArray& arr )
{
	int32_t		iSize;

	pull( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsValue		v;

		pull( st, v );

		arr.push_back( v );
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsHash& hash )
{
	int32_t		iSize;

	pull( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsString	strKey;
		SsValue		v;

		pull( st, strKey );
		pull( st, v );

		hash[strKey] = v;
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsCell& cell )
{
	pull( st, cell.name );
	pull( st, cell.pos );
	pull( st, cell.size );
	pull( st, cell.pivot );
	pull( st, cell.rotated );

	pull( st, cell.ismesh );

	pull( st, cell.innerPoint );
	pull( st, cell.outerPoint );
	pull( st, cell.meshPointList );
	pull( st, cell.meshTriList );
	pull( st, cell.divtype );
	pull( st, cell.divw );
	pull( st, cell.divh );

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsEffectNode& node )
{
	pull( st, node.arrayIndex );
	pull( st, node.parentIndex );
	pull( st, node.type );
	pull( st, node.visible );
	pull( st, node.behavior );

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsEffectBehavior& behavior )
{
	pull( st, behavior.CellName );
	pull( st, behavior.CellMapName );
	pull( st, behavior.BlendType );

	int32_t		iSize;

	pull( st, iSize );

	for ( int32_t i = 0; i < iSize; i++ ) {
		SsEffectElementBase*			v = NULL;
		SsEffectFunctionType::enum_		eType;

		pull( st, eType );

		switch ( static_cast<int>(eType) ) {
		case SsEffectFunctionType::Basic :
			{
				auto	w = new ParticleElementBasic();

				pull( st, w->maximumParticle );
				pull( st, w->speed );
				pull( st, w->lifespan );
				pull( st, w->angle );
				pull( st, w->angleVariance );
				pull( st, w->interval );
				pull( st, w->lifetime );
				pull( st, w->attimeCreate );
				pull( st, w->priority );

				v = w;
			}
			break;
		case SsEffectFunctionType::RndSeedChange :
			{
				auto	w = new ParticleElementRndSeedChange();

				pull( st, w->Seed );

				v = w;
			}
			break;
		case SsEffectFunctionType::Delay :
			{
				auto	w = new ParticleElementDelay();

				pull( st, w->DelayTime );

				v = w;
			}
			break;
		case SsEffectFunctionType::Gravity :
			{
				auto	w = new ParticleElementGravity();

				pull( st, w->Gravity );

				v = w;
			}
			break;
		case SsEffectFunctionType::Position :
			{
				auto	w = new ParticleElementPosition();

				pull( st, w->OffsetX );
				pull( st, w->OffsetY );

				v = w;
			}
			break;
		case SsEffectFunctionType::Rotation :
			{
				auto	w = new ParticleElementRotation();

				pull( st, w->Rotation );
				pull( st, w->RotationAdd );

				v = w;
			}
			break;
		case SsEffectFunctionType::TransRotation :
			{
				auto	w = new ParticleElementRotationTrans();

				pull( st, w->RotationFactor );
				pull( st, w->EndLifeTimePer );

				v = w;
			}
			break;
		case SsEffectFunctionType::TransSpeed :
			{
				auto	w = new ParticleElementTransSpeed();

				pull( st, w->Speed );

				v = w;
			}
			break;
		case SsEffectFunctionType::TangentialAcceleration :
			{
				auto	w = new ParticleElementTangentialAcceleration();

				pull( st, w->Acceleration );

				v = w;
			}
			break;
		case SsEffectFunctionType::InitColor :
			{
				auto	w = new ParticleElementInitColor();

				pull( st, w->Color );

				v = w;
			}
			break;
		case SsEffectFunctionType::TransColor :
			{
				auto	w = new ParticleElementTransColor();

				pull( st, w->Color );

				v = w;
			}
			break;
		case SsEffectFunctionType::AlphaFade :
			{
				auto	w = new ParticleElementAlphaFade();

				pull( st, w->disprange );

				v = w;
			}
			break;
		case SsEffectFunctionType::Size :
			{
				auto	w = new ParticleElementSize();

				pull( st, w->SizeX );
				pull( st, w->SizeY );
				pull( st, w->ScaleFactor );

				v = w;
			}
			break;
		case SsEffectFunctionType::TransSize :
			{
				auto	w = new ParticleElementTransSize();

				pull( st, w->SizeX );
				pull( st, w->SizeY );
				pull( st, w->ScaleFactor );

				v = w;
			}
			break;
		case SsEffectFunctionType::PointGravity :
			{
				auto	w = new ParticlePointGravity();

				pull( st, w->Position );
				pull( st, w->Power );

				v = w;
			}
			break;
		case SsEffectFunctionType::TurnToDirectionEnabled :
			{
				auto	w = new ParticleTurnToDirectionEnabled();

				pull( st, w->Rotation );

				v = w;
			}
			break;
		case SsEffectFunctionType::InfiniteEmitEnabled :
			{
				auto	w = new ParticleInfiniteEmitEnabled();

				v = w;
			}
			break;
		}

		if ( v ) {
			behavior.plist.push_back( std::move( std::unique_ptr<SsEffectElementBase>( v ) ) );
		}
	}

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsString& str )
{
	str = st.get_utf8_string().utf8();

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsPoint2& p )
{
	p.x = st.get_float();
	p.y = st.get_float();

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, SsTriangle& tri )
{
	tri.idxPo1 = st.get_32();
	tri.idxPo2 = st.get_32();
	tri.idxPo3 = st.get_32();

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, int32_t& iVal )
{
	iVal = st.get_32();

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, int8_t& cVal )
{
	cVal = st.get_8();

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, u8& byVal )
{
	byVal = st.get_u8();

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, float& fVal )
{
	fVal = st.get_float();

	return	true;
}

bool SsIO::pull( StreamPeerBuffer& st, bool& bVal )
{
	int8_t	cVal;

	pull( st, cVal );

	bVal = cVal != 0 ? true : false;

	return	true;
}
