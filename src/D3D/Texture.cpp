#include "Texture.h"

//#include "ResourceUploadBatch.h"
#include "DirectXHelpers.h"

#include <iostream>

Texture::Texture(DXContext* context, RenderPipeline* pipeline, TextureData textureData)
	: width(textureData.width), height(textureData.height), type(textureData.type), format(textureData.format), mipLevels(textureData.mipLevels)
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

		if (mipLevels > 1) {
			uavMipGpuDescriptorHandles.resize(mipLevels);
            for (UINT mip = 1; mip < mipLevels; mip++) {
                makeUav(context, pipeline, mip, D3D12_UAV_DIMENSION_TEXTURE2DARRAY);
            }
		}

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

D3D12_GPU_DESCRIPTOR_HANDLE Texture::getSrvGpuDescriptorHandle() {
	if (!srvCreated) {
		std::cerr << "Error: Attempted to get SRV handle for texture that has no SRV." << std::endl;
	}

    return srvGpuDescriptorHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE Texture::getUavGpuDescriptorHandle(UINT mipSlice) {
    return uavMipGpuDescriptorHandles[mipSlice];
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
    heapIndex = pipeline->getDescriptorHeap()->allocate(cpuHandle, srvGpuDescriptorHandle);
    context->getDevice()->CreateShaderResourceView(textureResource.Get(), &srvDesc, cpuHandle);

	srvCreated = true;
}

void Texture::makeUav(DXContext* context, RenderPipeline* pipeline, UINT mipSlice, D3D12_UAV_DIMENSION uavDimension) {
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = format;
	uavDesc.ViewDimension = uavDimension;
	
    if (uavDimension == D3D12_UAV_DIMENSION_TEXTURE2DARRAY) {
        // For cubemaps (6 faces)
        uavDesc.Texture2DArray.MipSlice = mipSlice;
        uavDesc.Texture2DArray.FirstArraySlice = 0;
        uavDesc.Texture2DArray.ArraySize = 6;  // All 6 cubemap faces
        uavDesc.Texture2DArray.PlaneSlice = 0;
    }
    else {
        // For regular 2D textures
        uavDesc.Texture2D.MipSlice = mipSlice;
        uavDesc.Texture2D.PlaneSlice = 0;
    }

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	UINT uavHeapIndex = pipeline->getDescriptorHeap()->allocate(cpuHandle, uavMipGpuDescriptorHandles[mipSlice]);
	context->getDevice()->CreateUnorderedAccessView(textureResource.Get(), nullptr, &uavDesc, cpuHandle);
}

void Texture::generateMipMaps(DXContext* context, ComputePipeline* pipeline) {
    if (mipLevels <= 1) {
        std::cerr << "Warning: Attempted to generate mipmaps for texture with only 1 mip level." << std::endl;
        return;
    }

    CD3DX12_RESOURCE_BARRIER toUav = CD3DX12_RESOURCE_BARRIER::Transition(
        textureResource.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );
    pipeline->getCommandList()->ResourceBarrier(1, &toUav);

    ID3D12DescriptorHeap* descriptorHeaps[] = { pipeline->getDescriptorHeap()->getAddress() };
    pipeline->getCommandList()->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
    pipeline->getCommandList()->SetComputeRootSignature(pipeline->getRootSignature());
    pipeline->getCommandList()->SetPipelineState(pipeline->getPSO());

    // Loop over each cubemap face
    UINT srcWidth = width;
    UINT srcHeight = height;

    for (UINT srcMip = 0; srcMip < mipLevels - 1; ) {
        UINT numMips = std::min(4u, mipLevels - srcMip - 1);

        struct MipConstants {
            uint32_t SrcMipLevel;
            uint32_t NumMipLevels;
            float TexelSizeX;
            float TexelSizeY;
        } constants;

        constants.SrcMipLevel = srcMip;
        constants.NumMipLevels = numMips;
        constants.TexelSizeX = 1.0f / static_cast<float>(srcWidth >> 1);
        constants.TexelSizeY = 1.0f / static_cast<float>(srcHeight >> 1);

        pipeline->getCommandList()->SetComputeRoot32BitConstants(
            0,
            sizeof(MipConstants) / 4,
            &constants,
            0
        );

        // Set SRV for this specific face
        pipeline->getCommandList()->SetComputeRootDescriptorTable(1, srvGpuDescriptorHandle);

        // Set UAVs for this specific face
        pipeline->getCommandList()->SetComputeRootDescriptorTable(2, getUavGpuDescriptorHandle(srcMip + 1));

        UINT dispatchWidth = std::max(1u, srcWidth >> 1);
        UINT dispatchHeight = std::max(1u, srcHeight >> 1);
        UINT threadGroupX = (dispatchWidth + 7) / 8;
        UINT threadGroupY = (dispatchHeight + 7) / 8;

        pipeline->getCommandList()->Dispatch(threadGroupX, threadGroupY, 1);

        if (srcMip + numMips < mipLevels - 1) {
            CD3DX12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(textureResource.Get());
            pipeline->getCommandList()->ResourceBarrier(1, &uavBarrier);
        }

        srcMip += numMips;
        srcWidth = std::max(1u, srcWidth >> numMips);
        srcHeight = std::max(1u, srcHeight >> numMips);
    }

    CD3DX12_RESOURCE_BARRIER toPixelShaderResource = CD3DX12_RESOURCE_BARRIER::Transition(
        textureResource.Get(),
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );
    pipeline->getCommandList()->ResourceBarrier(1, &toPixelShaderResource);
}

void Texture::releaseResources() {
	if (textureResource) {
		textureResource.Release();
	}

	if (textureUploadHeap) {
		textureUploadHeap.Release();
	}
}
