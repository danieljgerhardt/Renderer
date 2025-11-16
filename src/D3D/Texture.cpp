#include "Texture.h"

#include "ResourceUploadBatch.h"

Texture::Texture(DXContext* context, UINT width, UINT height, std::vector<unsigned char> imageData, TextureType type)
	: width(width), height(height), type(type)
{
    D3D12_RESOURCE_DESC txtDesc = {};
    txtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    txtDesc.Alignment = 0;
    txtDesc.Width = width;
    txtDesc.Height = height;
    txtDesc.DepthOrArraySize = 1;
    txtDesc.MipLevels = 1;
    txtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    txtDesc.SampleDesc.Count = 1;
    txtDesc.SampleDesc.Quality = 0;
    txtDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    txtDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    context->getDevice()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &txtDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&textureResource));

    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = imageData.data();
    textureData.RowPitch = width * 4;
    textureData.SlicePitch = width * height * 4;

    DirectX::ResourceUploadBatch resourceUpload(context->getDevice());
    resourceUpload.Begin();

    resourceUpload.Upload(textureResource, 0, &textureData, 1);

    resourceUpload.Transition(
        textureResource,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    auto uploadDone = resourceUpload.End(context->getCommandQueue());
    uploadDone.wait();
}

Texture::~Texture()
{
	releaseResources();
}

ID3D12Resource* Texture::getTextureResource() {
	return textureResource;
}

void Texture::releaseResources()
{
	if (textureResource) {
		textureResource.Release();
		textureResource = nullptr;
	}
}
