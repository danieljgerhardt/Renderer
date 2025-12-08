#pragma once

#include "DXContext.h"

#include "D3D/Pipeline/RenderPipeline.h"
#include "D3D/Buffer/StructuredBuffer.h"

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
	Texture(DXContext* context, RenderPipeline* pipeline, UINT width, UINT height, std::vector<unsigned char> imageData, TextureType type);
	~Texture();

	D3D12_GPU_DESCRIPTOR_HANDLE getTextureGpuDescriptorHandle();

	void makeSrv(DXContext* context, RenderPipeline* pipeline);

	void releaseResources();

private:
	ComPointer<ID3D12Resource> textureResource;
	ComPointer<ID3D12Resource> textureUploadHeap;
	UINT width, height;
	TextureType type;

	UINT heapIndex;
	D3D12_GPU_DESCRIPTOR_HANDLE textureGpuDescriptorHandle;
};

