#pragma once

#include "DXContext.h"

#include "D3D/Pipeline/RenderPipeline.h"
#include "D3D/Pipeline/ComputePipeline.h"
#include "D3D/Buffer/StructuredBuffer.h"

//https://github.com/microsoft/DirectXTK12/wiki/Textures

//TODO - unused, will be needed for more complex texture support
enum TextureFormat {
	RGBA32_FLOAT, //defaulting to this
};

enum TextureType {
	DIFFUSE = 0,
	NORMAL,
	EMISSIVE,
	METALLIC_ROUGHNESS,
	ENV_MAP,
	BRDF_LUT,
	PT_TARGET,
	NUM_TEXTURE_TYPES
};

struct TextureData {
	UINT width;
	UINT height;

	//TODO - not both nececssary
	std::vector<unsigned char> imageData;
	std::vector<float> imageDataFloat;

	TextureType type{ DIFFUSE };
	UINT mipLevels{ 1 };
	DXGI_FORMAT format{ DXGI_FORMAT_R8G8B8A8_UNORM };
};

class Texture
{
public:
	Texture() = delete;

	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;

	Texture(Texture&&) noexcept = default;
	Texture& operator=(Texture&&) noexcept = default;

	Texture(DXContext* context, Pipeline* pipeline, TextureData textureData);
	~Texture();

	D3D12_GPU_DESCRIPTOR_HANDLE getSrvGpuDescriptorHandle();
	D3D12_GPU_DESCRIPTOR_HANDLE getUavGpuDescriptorHandle();

	void makeSrv(DXContext* context, Pipeline* pipeline, D3D12_SRV_DIMENSION srvDimension = D3D12_SRV_DIMENSION_TEXTURE2D);
	void makeUav(DXContext* context, Pipeline* pipeline, D3D12_UAV_DIMENSION uavDimension = D3D12_UAV_DIMENSION_TEXTURE2D);

	void generateMipMaps(DXContext* context, RenderPipeline* renderPipeline);

	TextureType getType() { return type; }

	void releaseResources();

	D3D12_RESOURCE_DESC getResourceDesc() { return resourceDesc; }

	ID3D12Resource* getTextureResource() { return textureResource.Get(); }

	UINT getWidth() { return width; }
	UINT getHeight() { return height; }

private:
	ComPointer<ID3D12Resource> textureResource;
	ComPointer<ID3D12Resource> textureUploadHeap;

	UINT width, height;

	UINT mipLevels = 1;

	bool srvCreated{ false };

	TextureType type;

	DXGI_FORMAT format;

	UINT heapIndex{};

	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuDescriptorHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE uavGpuDescriptorHandle{};

	D3D12_RESOURCE_DESC resourceDesc;
};

