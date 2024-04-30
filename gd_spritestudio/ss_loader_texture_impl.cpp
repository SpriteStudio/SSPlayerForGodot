/*!
* \file		ss_loader_texture_impl.cpp
* \author	CRI Middleware Co., Ltd.
*/
#include "ss_loader_texture_impl.h"

#include "SpriteStudio6-SDK/Common/Helper/IsshTexture.h"

#include "ss_macros.h"

SsSdkUsing

static SSTextureLoader::DataHandle onLoadImageFromFile( const char* fileName, int* width, int* height, int* bpp )
{
	return	0;
}

static void onDecodeEndImageFile( SSTextureLoader::DataHandle handle )
{
}

static const char* onMessageGetFailureLoadFromFile()
{
	return	0;
}

static bool onCheckSizePow2( int width, int height )
{
	return	true;
}

void SsLoaderTextureImpl::setCallbacks()
{
	SSTextureLoader::FunctionLoadImageFromFile = onLoadImageFromFile;
	SSTextureLoader::FunctionDecodeEndImageFile = onDecodeEndImageFile;
	SSTextureLoader::FunctionMessageGetFailureLoadFromFile = onMessageGetFailureLoadFromFile;
	SSTextureLoader::FunctionCheckSizePow2 = onCheckSizePow2;
}
