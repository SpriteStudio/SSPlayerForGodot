/*!
* \file		gd_packet_sscellmap.h
* \author	CRI Middleware Co., Ltd.
*/
#ifndef GD_PACKET_SSCELLMAP_H
#define GD_PACKET_SSCELLMAP_H

#include "gd_macros.h"

#ifdef SPRITESTUDIO_GODOT_EXTENSION
#include <godot_cpp/classes/stream_peer.hpp>
#include <godot_cpp/classes/stream_peer_buffer.hpp>
using namespace godot;
#define	PoolByteArray	PackedByteArray
#else
#ifdef GD_V4
#include "core/io/stream_peer.h"
#define	PoolByteArray	PackedByteArray
#endif
#ifdef GD_V3
#include "core/io/stream_peer.h"
#endif
#endif

#include "SpriteStudio6-SDK/Common/Loader/ssloader_ssce.h"

#include "ss_macros.h"

SsSdkUsing

class GdPacketSsCellMap : public StreamPeerBuffer
{
public :
	GdPacketSsCellMap();
	virtual ~GdPacketSsCellMap();

	bool write( const String& strRaw );

	bool read( SsCellMap* pCellMap, const PoolByteArray& bytes );

	PoolByteArray getBytes() const;
};

#endif // GD_PACKET_SSCELLMAP_H
