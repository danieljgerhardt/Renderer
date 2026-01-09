#include "Texture.h"

//#include "ResourceUploadBatch.h"
#include "DirectXHelpers.h"

#include "DirectXTex.h"

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
            makeUav(context, pipeline, D3D12_UAV_DIMENSION_TEXTURE2DARRAY);
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

D3D12_GPU_DESCRIPTOR_HANDLE Texture::getUavGpuDescriptorHandle() {
    return uavGpuDescriptorHandle;
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

void Texture::makeUav(DXContext* context, RenderPipeline* pipeline, D3D12_UAV_DIMENSION uavDimension) {
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = uavDimension;
	
    if (uavDimension == D3D12_UAV_DIMENSION_TEXTURE2DARRAY) {
        uavDesc.Texture2DArray.MipSlice = 0;
        uavDesc.Texture2DArray.FirstArraySlice = 0;
        uavDesc.Texture2DArray.ArraySize = 6;
        uavDesc.Texture2DArray.PlaneSlice = 0;
    }
    else {
        uavDesc.Texture2D.MipSlice = 0;
        uavDesc.Texture2D.PlaneSlice = 0;
    }

	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle;
	UINT uavHeapIndex = pipeline->getDescriptorHeap()->allocate(cpuHandle, uavGpuDescriptorHandle);
	context->getDevice()->CreateUnorderedAccessView(textureResource.Get(), nullptr, &uavDesc, cpuHandle);
}


void Texture::generateMipMaps(DXContext* context, ComputePipeline* computePipeline, RenderPipeline* renderPipeline) {
    if (mipLevels <= 1) {
        std::cerr << "Attempted to generate mipmaps for texture with only 1 mip level." << std::endl;
        return;
    }

    D3D12_RESOURCE_DESC desc = textureResource->GetDesc();
    const UINT arraySize = 6;

    // Step 1: Read back all 6 array slices from base mip level
    DirectX::ScratchImage baseImage;
    baseImage.Initialize2D(format, width, height, arraySize, 1);

    for (UINT arraySlice = 0; arraySlice < arraySize; arraySlice++) {
        UINT subresource = D3D12CalcSubresource(0, arraySlice, 0, mipLevels, arraySize);

        D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
        UINT numRows;
        UINT64 rowSizeInBytes;
        UINT64 totalBytes;

        context->getDevice()->GetCopyableFootprints(&desc, subresource, 1, 0, &layout, &numRows, &rowSizeInBytes, &totalBytes);

        // Create readback buffer
        CD3DX12_HEAP_PROPERTIES readbackHeap(D3D12_HEAP_TYPE_READBACK);
        CD3DX12_RESOURCE_DESC readbackDesc = CD3DX12_RESOURCE_DESC::Buffer(totalBytes);

        ComPointer<ID3D12Resource> readbackBuffer;
        context->getDevice()->CreateCommittedResource(
            &readbackHeap,
            D3D12_HEAP_FLAG_NONE,
            &readbackDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&readbackBuffer)
        );

        // Transition texture to copy source
        CD3DX12_RESOURCE_BARRIER toCopySrc = CD3DX12_RESOURCE_BARRIER::Transition(
            textureResource.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            subresource
        );
        renderPipeline->getCommandList()->ResourceBarrier(1, &toCopySrc);

        // Copy from GPU to readback buffer
        CD3DX12_TEXTURE_COPY_LOCATION dst(readbackBuffer.Get(), layout);
        CD3DX12_TEXTURE_COPY_LOCATION src(textureResource.Get(), subresource);
        renderPipeline->getCommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

        // Transition back
        CD3DX12_RESOURCE_BARRIER toShaderResource = CD3DX12_RESOURCE_BARRIER::Transition(
            textureResource.Get(),
            D3D12_RESOURCE_STATE_COPY_SOURCE,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            subresource
        );
        renderPipeline->getCommandList()->ResourceBarrier(1, &toShaderResource);

        // Execute and wait for GPU to finish
        context->executeCommandList(renderPipeline->getCommandListID());
        context->resetCommandList(renderPipeline->getCommandListID());
        context->signalAndWait();

        // Map and read data
        void* pData;
        readbackBuffer->Map(0, nullptr, &pData);

        // Copy data to DirectXTex image for this array slice
        const DirectX::Image* baseImg = baseImage.GetImage(0, arraySlice, 0);
        uint8_t* srcPixels = static_cast<uint8_t*>(pData);
        uint8_t* dstPixels = baseImg->pixels;

        // Calculate actual bytes to copy per row (minimum of source and dest pitch)
        UINT bytesPerPixel = DirectX::BitsPerPixel(format) / 8;
        UINT actualRowBytes = width * bytesPerPixel;

        for (UINT row = 0; row < height; row++) {
            memcpy(dstPixels + row * baseImg->rowPitch, srcPixels + row * layout.Footprint.RowPitch, actualRowBytes);
        }

        readbackBuffer->Unmap(0, nullptr);
    }

    // Step 2: Generate mipmaps for entire array in one call
    DirectX::ScratchImage mipChain;
    HRESULT hr = DirectX::GenerateMipMaps(
        baseImage.GetImages(),
        baseImage.GetImageCount(),
        baseImage.GetMetadata(),
        DirectX::TEX_FILTER_DEFAULT,
        mipLevels,
        mipChain
    );

    if (FAILED(hr)) {
        std::cerr << "Failed to generate mipmaps" << std::endl;
        return;
    }

    // Step 3: Upload each mip level for each array slice back to GPU
    for (UINT arraySlice = 0; arraySlice < arraySize; arraySlice++) {
        for (UINT mip = 1; mip < mipLevels; mip++) {
            const DirectX::Image* mipImg = mipChain.GetImage(mip, arraySlice, 0);

            UINT mipWidth = std::max(1u, width >> mip);
            UINT mipHeight = std::max(1u, height >> mip);

            // Get footprint for this mip
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT mipLayout;
            UINT numRows;
            UINT64 rowSizeInBytes;
            UINT64 totalBytes;
            UINT mipSubresource = D3D12CalcSubresource(mip, arraySlice, 0, mipLevels, arraySize);
            context->getDevice()->GetCopyableFootprints(&desc, mipSubresource, 1, 0, &mipLayout, &numRows, &rowSizeInBytes, &totalBytes);

            // Create upload buffer
            CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
            CD3DX12_RESOURCE_DESC uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(totalBytes);

            ComPointer<ID3D12Resource> uploadBuffer;
            context->getDevice()->CreateCommittedResource(
                &uploadHeap,
                D3D12_HEAP_FLAG_NONE,
                &uploadDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&uploadBuffer)
            );

            // Map and copy data
            void* pUploadData;
            uploadBuffer->Map(0, nullptr, &pUploadData);

            uint8_t* uploadSrc = mipImg->pixels;
            uint8_t* uploadDst = static_cast<uint8_t*>(pUploadData);

            // Calculate actual row size in bytes for this mip level
            UINT bytesPerPixel = DirectX::BitsPerPixel(format) / 8;
            UINT actualRowSize = mipWidth * bytesPerPixel;

            for (UINT row = 0; row < mipHeight; row++) {
                memcpy(uploadDst + row * mipLayout.Footprint.RowPitch, uploadSrc + row * mipImg->rowPitch, actualRowSize);
            }

            uploadBuffer->Unmap(0, nullptr);

            // Transition to copy dest
            CD3DX12_RESOURCE_BARRIER toCopyDest = CD3DX12_RESOURCE_BARRIER::Transition(
                textureResource.Get(),
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_COPY_DEST,
                mipSubresource
            );
            renderPipeline->getCommandList()->ResourceBarrier(1, &toCopyDest);

            // Copy from upload buffer to texture
            CD3DX12_TEXTURE_COPY_LOCATION uploadLoc(uploadBuffer.Get(), mipLayout);
            CD3DX12_TEXTURE_COPY_LOCATION texLoc(textureResource.Get(), mipSubresource);
            renderPipeline->getCommandList()->CopyTextureRegion(&texLoc, 0, 0, 0, &uploadLoc, nullptr);

            // Transition back to shader resource
            CD3DX12_RESOURCE_BARRIER backToShaderResource = CD3DX12_RESOURCE_BARRIER::Transition(
                textureResource.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                mipSubresource
            );
            renderPipeline->getCommandList()->ResourceBarrier(1, &backToShaderResource);

            // Execute and wait
            context->executeCommandList(renderPipeline->getCommandListID());
            context->resetCommandList(renderPipeline->getCommandListID());
            context->signalAndWait();
        }
    }
}

void Texture::releaseResources() {
	if (textureResource) {
		textureResource.Release();
	}

	if (textureUploadHeap) {
		textureUploadHeap.Release();
	}
}
