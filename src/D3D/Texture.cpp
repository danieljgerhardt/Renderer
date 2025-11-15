#include "Texture.h"

#include "ResourceUploadBatch.h"

Texture::Texture(DXContext* context, UINT width, UINT height) : width(width), height(height) {
    D3D12_RESOURCE_DESC txtDesc = {};
    txtDesc.MipLevels = txtDesc.DepthOrArraySize = 1;
    txtDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    txtDesc.Width = width;
    txtDesc.Height = height;
    txtDesc.SampleDesc.Count = 1;
    txtDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

	context->getDevice()->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&txtDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&textureResource));

	static const uint32_t s_whitePixel = 0xFFFFFFFF;

	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = &s_whitePixel;
	textureData.RowPitch = txtDesc.Width * 4;
	textureData.SlicePitch = txtDesc.Height * txtDesc.Width * 4;

	DirectX::ResourceUploadBatch resourceUpload(context->getDevice());

	resourceUpload.Begin();

	resourceUpload.Upload(textureResource, 0, &textureData, 1);

	resourceUpload.Transition(
		textureResource,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

	auto uploadResourcesFinished = resourceUpload.End(context->getCommandQueue());

	uploadResourcesFinished.wait();
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
		textureResource->Release();
		textureResource = nullptr;
	}
}
