/*!
* \file		ss_io.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef SS_IO_H
#define SS_IO_H

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/stream_peer.hpp>
#include <godot_cpp/classes/stream_peer_buffer.hpp>
using namespace godot;
#else
#include "core/io/stream_peer.h"
#endif

#include "SpriteStudio6-SDK/Common/Loader/ssloader_sspj.h"

#include "ss_macros.h"

SsSdkUsing

class SsIO
{
private :
	SsIO();

public :
	virtual ~SsIO();

	static Variant toProjectVariant( const String& strRaw );
	static Variant toAnimePackVariant( const String& strRaw );
	static Variant toCellMapVariant( const String& strRaw );
	static Variant toEffectVariant( const String& strRaw );

	static SsProject* createProjectFromVariant( const Variant& val );
	static SsAnimePack* createAnimePackFromVariant( const Variant& val );
	static SsCellMap* createCellMapFromVariant( const Variant& val );
	static SsEffectFile* createEffectFromVariant( const Variant& val );

	static bool push( StreamPeerBuffer& st, SsProject& project );
	static bool push( StreamPeerBuffer& st, SsAnimePack& animePack );
	static bool push( StreamPeerBuffer& st, SsCellMap& cellMap );
	static bool push( StreamPeerBuffer& st, SsEffectFile& effect );
	static bool push( StreamPeerBuffer& st, SsProjectSetting& setting );
	static bool push( StreamPeerBuffer& st, SsAnimationSettings& setting );
	static bool push( StreamPeerBuffer& st, SsModel& model );
	static bool push( StreamPeerBuffer& st, SsEffectModel& model );
	static bool push( StreamPeerBuffer& st, SsTexWrapMode::_enum& eMode );
	static bool push( StreamPeerBuffer& st, SsTexFilterMode::_enum& eMode );
	static bool push( StreamPeerBuffer& st, SsPartsSortMode::_enum& eMode );
	static bool push( StreamPeerBuffer& st, SsPartType::_enum& eType );
	static bool push( StreamPeerBuffer& st, SsBoundsType::_enum& eType );
	static bool push( StreamPeerBuffer& st, SsInheritType::_enum& eType );
	static bool push( StreamPeerBuffer& st, SsBlendType::_enum& eType );
	static bool push( StreamPeerBuffer& st, SsIkRotationArrow::_enum& eArrow );
	static bool push( StreamPeerBuffer& st, SsAttributeKind::_enum& eKind );
	static bool push( StreamPeerBuffer& st, SsInterpolationType::_enum& eType );
	static bool push( StreamPeerBuffer& st, SsMeshDivType::_enum& eType );
	static bool push( StreamPeerBuffer& st, SsEffectNodeType::_enum& eType );
	static bool push( StreamPeerBuffer& st, SsRenderBlendType::_enum& eType );
	static bool push( StreamPeerBuffer& st, SsEffectFunctionType::enum_& eType );
	static bool push( StreamPeerBuffer& st, std::map<SsString,int>& map );
	static bool push( StreamPeerBuffer& st, std::vector<SsAnimation*>& vec );
	static bool push( StreamPeerBuffer& st, std::vector<SsPart*>& vec );
	static bool push( StreamPeerBuffer& st, std::vector<SsMeshBind*>& vec );
	static bool push( StreamPeerBuffer& st, std::vector<SsMeshBindInfo>& vec );
	static bool push( StreamPeerBuffer& st, std::vector<SsLabel*>& vec );
	static bool push( StreamPeerBuffer& st, std::vector<SsPartAnime*>& vec );
	static bool push( StreamPeerBuffer& st, std::vector<SsAttribute*>& vec );
	static bool push( StreamPeerBuffer& st, std::vector<SsKeyframe*>& vec );
	static bool push( StreamPeerBuffer& st, std::vector<SsCell*>& vec );
	static bool push( StreamPeerBuffer& st, std::vector<SsEffectNode*>& vec );
	static bool push( StreamPeerBuffer& st, std::vector<SsString>& vec );
	static bool push( StreamPeerBuffer& st, std::vector<SsPoint2>& vec );
	static bool push( StreamPeerBuffer& st, std::vector<SsTriangle>& vec );
	static bool push( StreamPeerBuffer& st, VarianceValue<float>& val );
	static bool push( StreamPeerBuffer& st, VarianceValue<int>& val );
	static bool push( StreamPeerBuffer& st, VarianceValue<SsU8Color>& val );
	static bool push( StreamPeerBuffer& st, SsTColor<u8>& col );
	static bool push( StreamPeerBuffer& st, SsAnimation& animation );
	static bool push( StreamPeerBuffer& st, SsPart& part );
	static bool push( StreamPeerBuffer& st, SsMeshBind& bind );
	static bool push( StreamPeerBuffer& st, SsMeshBindInfo& info );
	static bool push( StreamPeerBuffer& st, SsLabel& label );
	static bool push( StreamPeerBuffer& st, SsPartAnime& anime );
	static bool push( StreamPeerBuffer& st, SsAttribute& attribute );
	static bool push( StreamPeerBuffer& st, SsKeyframe& keyframe );
	static bool push( StreamPeerBuffer& st, SsCurve& curve );
	static bool push( StreamPeerBuffer& st, SsValue& value );
	static bool push( StreamPeerBuffer& st, SsArray& arr );
	static bool push( StreamPeerBuffer& st, SsHash& hash );
	static bool push( StreamPeerBuffer& st, SsCell& cell );
	static bool push( StreamPeerBuffer& st, SsEffectNode& node );
	static bool push( StreamPeerBuffer& st, SsEffectBehavior& behavior );
	static bool push( StreamPeerBuffer& st, SsString& str );
	static bool push( StreamPeerBuffer& st, SsPoint2& p );
	static bool push( StreamPeerBuffer& st, SsTriangle& tri );
	static bool push( StreamPeerBuffer& st, int32_t& iVal );
	static bool push( StreamPeerBuffer& st, int8_t& cVal );
	static bool push( StreamPeerBuffer& st, u8& byVal );
	static bool push( StreamPeerBuffer& st, float& fVal );
	static bool push( StreamPeerBuffer& st, bool& bVal );

	static bool pull( StreamPeerBuffer& st, SsProject& project );
	static bool pull( StreamPeerBuffer& st, SsAnimePack& animePack );
	static bool pull( StreamPeerBuffer& st, SsCellMap& cellMap );
	static bool pull( StreamPeerBuffer& st, SsEffectFile& effect );
	static bool pull( StreamPeerBuffer& st, SsProjectSetting& setting );
	static bool pull( StreamPeerBuffer& st, SsAnimationSettings& setting );
	static bool pull( StreamPeerBuffer& st, SsModel& model );
	static bool pull( StreamPeerBuffer& st, SsEffectModel& model );
	static bool pull( StreamPeerBuffer& st, SsTexWrapMode::_enum& eMode );
	static bool pull( StreamPeerBuffer& st, SsTexFilterMode::_enum& eMode );
	static bool pull( StreamPeerBuffer& st, SsPartsSortMode::_enum& eMode );
	static bool pull( StreamPeerBuffer& st, SsPartType::_enum& eType );
	static bool pull( StreamPeerBuffer& st, SsBoundsType::_enum& eType );
	static bool pull( StreamPeerBuffer& st, SsInheritType::_enum& eType );
	static bool pull( StreamPeerBuffer& st, SsBlendType::_enum& eType );
	static bool pull( StreamPeerBuffer& st, SsIkRotationArrow::_enum& eArrow );
	static bool pull( StreamPeerBuffer& st, SsAttributeKind::_enum& eKind );
	static bool pull( StreamPeerBuffer& st, SsInterpolationType::_enum& eType );
	static bool pull( StreamPeerBuffer& st, SsMeshDivType::_enum& eType );
	static bool pull( StreamPeerBuffer& st, SsEffectNodeType::_enum& eType );
	static bool pull( StreamPeerBuffer& st, SsRenderBlendType::_enum& eType );
	static bool pull( StreamPeerBuffer& st, SsEffectFunctionType::enum_& eType );
	static bool pull( StreamPeerBuffer& st, std::map<SsString,int>& map );
	static bool pull( StreamPeerBuffer& st, std::vector<SsAnimation*>& vec );
	static bool pull( StreamPeerBuffer& st, std::vector<SsPart*>& vec );
	static bool pull( StreamPeerBuffer& st, std::vector<SsMeshBind*>& vec );
	static bool pull( StreamPeerBuffer& st, std::vector<SsMeshBindInfo>& vec );
	static bool pull( StreamPeerBuffer& st, std::vector<SsLabel*>& vec );
	static bool pull( StreamPeerBuffer& st, std::vector<SsPartAnime*>& vec );
	static bool pull( StreamPeerBuffer& st, std::vector<SsAttribute*>& vec );
	static bool pull( StreamPeerBuffer& st, std::vector<SsKeyframe*>& vec );
	static bool pull( StreamPeerBuffer& st, std::vector<SsCell*>& vec );
	static bool pull( StreamPeerBuffer& st, std::vector<SsEffectNode*>& vec );
	static bool pull( StreamPeerBuffer& st, std::vector<SsString>& vec );
	static bool pull( StreamPeerBuffer& st, std::vector<SsPoint2>& vec );
	static bool pull( StreamPeerBuffer& st, std::vector<SsTriangle>& vec );
	static bool pull( StreamPeerBuffer& st, VarianceValue<float>& val );
	static bool pull( StreamPeerBuffer& st, VarianceValue<int>& val );
	static bool pull( StreamPeerBuffer& st, VarianceValue<SsU8Color>& val );
	static bool pull( StreamPeerBuffer& st, SsTColor<u8>& col );
	static bool pull( StreamPeerBuffer& st, SsAnimation& animation );
	static bool pull( StreamPeerBuffer& st, SsPart& part );
	static bool pull( StreamPeerBuffer& st, SsMeshBind& bind );
	static bool pull( StreamPeerBuffer& st, SsMeshBindInfo& info );
	static bool pull( StreamPeerBuffer& st, SsLabel& label );
	static bool pull( StreamPeerBuffer& st, SsPartAnime& anime );
	static bool pull( StreamPeerBuffer& st, SsAttribute& attribute );
	static bool pull( StreamPeerBuffer& st, SsKeyframe& keyframe );
	static bool pull( StreamPeerBuffer& st, SsCurve& curve );
	static bool pull( StreamPeerBuffer& st, SsValue& value );
	static bool pull( StreamPeerBuffer& st, SsArray& arr );
	static bool pull( StreamPeerBuffer& st, SsHash& hash );
	static bool pull( StreamPeerBuffer& st, SsCell& cell );
	static bool pull( StreamPeerBuffer& st, SsEffectNode& node );
	static bool pull( StreamPeerBuffer& st, SsEffectBehavior& behavior );
	static bool pull( StreamPeerBuffer& st, SsString& str );
	static bool pull( StreamPeerBuffer& st, SsPoint2& p );
	static bool pull( StreamPeerBuffer& st, SsTriangle& tri );
	static bool pull( StreamPeerBuffer& st, int32_t& iVal );
	static bool pull( StreamPeerBuffer& st, int8_t& cVal );
	static bool pull( StreamPeerBuffer& st, u8& byVal );
	static bool pull( StreamPeerBuffer& st, float& fVal );
	static bool pull( StreamPeerBuffer& st, bool& bVal );
};

#endif // SS_IO_H
