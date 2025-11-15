#pragma once

#include "DXContext.h"

//https://github.com/microsoft/DirectXTK12/wiki/Textures

class Texture
{
public:
	Texture() = delete;
	Texture(DXContext* context, UINT width, UINT height);
	~Texture();
	ID3D12Resource* getTextureResource();
	void releaseResources();

private:
	ID3D12Resource* textureResource;
	UINT width, height;
};

