#pragma once

#include "DXContext.h"

//https://github.com/microsoft/DirectXTK12/wiki/Textures

//TODO - unused, will be needed for more complex texture support
enum TextureFormat {
	RGBA32_FLOAT, //essentially just defaulting to this
};

enum TextureType {
	DIFFUSE,
	NORMAL,
	EMISSIVE,
	METALLIC_ROUGHNESS
};

struct TextureData {
	UINT width;
	UINT height;
	std::vector<unsigned char> imageData;
	TextureType type;
};

class Texture
{
public:
	Texture() = delete;
	Texture(DXContext* context, UINT width, UINT height, std::vector<unsigned char> imageData, TextureType type);
	~Texture();
	ID3D12Resource* getTextureResource();
	void releaseResources();

private:
	ComPointer<ID3D12Resource> textureResource;
	UINT width, height;
	TextureType type;
};

