#include "Texture.h"

#include "ResourceUploadBatch.h"

#include <iostream>

Texture::Texture(DXContext* context, RenderPipeline* pipeline, TextureData textureData)
	: width(textureData.width), height(textureData.height), type(textureData.type), format(textureData.format)
{
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Width = width;
    resourceDesc.Height = height;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.MipLevels = mipLevels;
    resourceDesc.Format = format;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	if (type == TextureType::ENV_MAP) {
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        resourceDesc.DepthOrArraySize = 6;

        //todo - may not be necessary
        if (mipLevels > 1) {
			resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        CD3DX12_HEAP_PROPERTIES heapDefault(D3D12_HEAP_TYPE_DEFAULT);
        context->getDevice()->CreateCommittedResource(
            &heapDefault,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            nullptr,
            IID_PPV_ARGS(&textureResource));

        makeSrv(context, pipeline, D3D12_SRV_DIMENSION_TEXTURECUBE);

        return;
	}
    
    CD3DX12_HEAP_PROPERTIES heapDefault(D3D12_HEAP_TYPE_DEFAULT);
    context->getDevice()->CreateCommittedResource(
        &heapDefault,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
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

    UINT bytesPerPixel;
    D3D12_SUBRESOURCE_DATA subresource = {};
	if (textureData.format == DXGI_FORMAT_R32G32B32A32_FLOAT) {
		subresource.pData = textureData.imageDataFloat.data();
		bytesPerPixel = 16;
	}
    else {
        subresource.pData = textureData.imageData.data();
		bytesPerPixel = 4;
    }
    subresource.RowPitch = width * bytesPerPixel;
    subresource.SlicePitch = width * height * bytesPerPixel;

    UpdateSubresources(pipeline->getCommandList(), textureResource.Get(), textureUploadHeap, 0, 0, 1, &subresource);
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(textureResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
    pipeline->getCommandList()->ResourceBarrier(1, &barrier);

    //TODO - better logic for making srvs
    if (type == TextureType::DIFFUSE || (pipeline->getCommandListID() == CommandListID::PBR_RENDER_ID && type == TextureType::METALLIC_ROUGHNESS)) {
        makeSrv(context, pipeline);
    }
}

Texture::~Texture() {
	releaseResources();
}

D3D12_GPU_DESCRIPTOR_HANDLE Texture::getTextureGpuDescriptorHandle() {
	if (!srvCreated) {
		std::cerr << "Error: Attempted to get SRV handle for texture that has no SRV." << std::endl;
	}

    return textureGpuDescriptorHandle;
}

void Texture::makeSrv(DXContext* context, RenderPipeline* pipeline, D3D12_SRV_DIMENSION srvDimension) {
	if (srvCreated) {
		std::cerr << "Warning: Attempted to create SRV for texture that already has one." << std::endl;
        return;
	}

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = format;
    srvDesc.ViewDimension = srvDimension;
    srvDesc.Texture2D.MipLevels = mipLevels;

    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
    heapIndex = pipeline->getDescriptorHeap()->allocate(cpuHandle, textureGpuDescriptorHandle);
    context->getDevice()->CreateShaderResourceView(textureResource.Get(), &srvDesc, cpuHandle);

	srvCreated = true;
}

void Texture::generateMipMaps(DXContext* context, RenderPipeline* pipeline) {
	ID3D12Device6* device = context->getDevice();

	ResourceUploadBatch upload(device);

	upload.Begin();

    upload.GenerateMips(textureResource);

	auto finish = upload.End(context->getCommandQueue());

	finish.wait();
}

void Texture::releaseResources() {
	if (textureResource) {
		textureResource.Release();
	}

	if (textureUploadHeap) {
		textureUploadHeap.Release();
	}
}
