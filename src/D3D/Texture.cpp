#include "Texture.h"

#include "ResourceUploadBatch.h"

Texture::Texture(DXContext* context, RenderPipeline* pipeline, UINT width, UINT height, std::vector<unsigned char> imageData, TextureType type)
	: width(width), height(height), type(type)
{
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    
    CD3DX12_HEAP_PROPERTIES heapDefault(D3D12_HEAP_TYPE_DEFAULT);
    context->getDevice()->CreateCommittedResource(
        &heapDefault,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&textureResource));

    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(textureResource.Get(), 0, 1);

    // Create the GPU upload buffer.
	CD3DX12_HEAP_PROPERTIES heapUpload(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    context->getDevice()->CreateCommittedResource(
        &heapUpload,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&textureUploadHeap));

    D3D12_SUBRESOURCE_DATA subresource = {};
    subresource.pData = imageData.data();
    subresource.RowPitch = width * 4;
    subresource.SlicePitch = width * height * 4;

    UpdateSubresources(pipeline->getCommandList(), textureResource.Get(), textureUploadHeap, 0, 0, 1, &subresource);
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(textureResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    pipeline->getCommandList()->ResourceBarrier(1, &barrier);
}

Texture::~Texture()
{
	releaseResources();
}

D3D12_GPU_DESCRIPTOR_HANDLE Texture::getTextureGpuDescriptorHandle()
{
    return textureGpuDescriptorHandle;
}

void Texture::makeSrv(DXContext* context, RenderPipeline* pipeline)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; //TODO - may change
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
    heapIndex = pipeline->getDescriptorHeap()->allocate(cpuHandle, textureGpuDescriptorHandle);
    context->getDevice()->CreateShaderResourceView(textureResource.Get(), &srvDesc, cpuHandle);
}

void Texture::releaseResources()
{
	if (textureResource) {
		textureResource.Release();
	}

	if (textureUploadHeap) {
		textureUploadHeap.Release();
	}
}
