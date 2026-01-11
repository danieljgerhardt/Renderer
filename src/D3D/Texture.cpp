#include "Texture.h"

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


void Texture::generateMipMaps(DXContext* context, RenderPipeline* renderPipeline) {
    if (mipLevels <= 1)
        return;

    ID3D12Device* device = context->getDevice();
    ID3D12GraphicsCommandList* cmd = renderPipeline->getCommandList();
    const UINT arraySize = 6;
    const size_t bpp = DirectX::BitsPerPixel(format) / 8;

    D3D12_RESOURCE_DESC desc = textureResource->GetDesc();
    std::vector<ComPointer<ID3D12Resource>> tempResources;

    // ============================================================
    // PHASE 1: READ BACK BASE MIP
    // ============================================================

    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(arraySize);
    std::vector<UINT> numRows(arraySize);
    std::vector<UINT64> rowSize(arraySize);

    CD3DX12_RESOURCE_BARRIER toCopySrc = CD3DX12_RESOURCE_BARRIER::Transition(
        textureResource.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_COPY_SOURCE,
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

    cmd->ResourceBarrier(1, &toCopySrc);

    for (UINT slice = 0; slice < arraySize; ++slice)
    {
        UINT sub = D3D12CalcSubresource(0, slice, 0, mipLevels, arraySize);

        UINT64 totalBytes;
        device->GetCopyableFootprints(&desc, sub, 1, 0,
            &footprints[slice], &numRows[slice], &rowSize[slice], &totalBytes);

        ComPointer<ID3D12Resource> readback;
        CD3DX12_HEAP_PROPERTIES heapReadback(D3D12_HEAP_TYPE_READBACK);
        CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(totalBytes);
        device->CreateCommittedResource(&heapReadback, D3D12_HEAP_FLAG_NONE,
            &bufferDesc, D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr, IID_PPV_ARGS(&readback));
        tempResources.push_back(readback);

        CD3DX12_TEXTURE_COPY_LOCATION dst(readback.Get(), footprints[slice]);
        CD3DX12_TEXTURE_COPY_LOCATION src(textureResource.Get(), sub);
        cmd->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
    }

    CD3DX12_RESOURCE_BARRIER toSRV = CD3DX12_RESOURCE_BARRIER::Transition(
        textureResource.Get(),
        D3D12_RESOURCE_STATE_COPY_SOURCE,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

    cmd->ResourceBarrier(1, &toSRV);
    context->executeCommandList(renderPipeline->getCommandListID());
    context->signalAndWait();
    context->resetCommandList(renderPipeline->getCommandListID());

    // ============================================================
    // PHASE 1B: REPACK INTO TIGHT CPU IMAGES
    // ============================================================

    std::vector<DirectX::ScratchImage> tightImages(arraySize);

    for (UINT slice = 0; slice < arraySize; ++slice)
    {
        tightImages[slice].Initialize2D(format, width, height, 1, 1);

        void* mapped = nullptr;
        tempResources[slice]->Map(0, nullptr, &mapped);

        const DirectX::Image* dst = tightImages[slice].GetImage(0, 0, 0);
        const uint8_t* src = static_cast<uint8_t*>(mapped);

        // FIXED: Use dst->rowPitch instead of calculating manually
        for (UINT row = 0; row < height; ++row)
        {
            memcpy(dst->pixels + row * dst->rowPitch,  // Use dst->rowPitch
                src + row * footprints[slice].Footprint.RowPitch,
                width * bpp);  // Copy width * bpp bytes per row
        }

        tempResources[slice]->Unmap(0, nullptr);
    }

    // ============================================================
// PHASE 2: GENERATE MIPS (CPU)
// ============================================================

    std::vector<DirectX::ScratchImage> mipChains(arraySize);

    for (UINT slice = 0; slice < arraySize; ++slice)
    {
        const DirectX::Image* baseImg = tightImages[slice].GetImage(0, 0, 0);

        DirectX::ScratchImage mipChain;

        // Try with explicit flags
        HRESULT hr = DirectX::GenerateMipMaps(
            *baseImg,
            DirectX::TEX_FILTER_SEPARATE_ALPHA | DirectX::TEX_FILTER_FORCE_NON_WIC,
            0,  // 0 = full chain
            mipChain
        );

        if (FAILED(hr))
        {
			//error report
			std::cerr << "Error: GenerateMipMaps failed for slice " << slice << " with HRESULT " << std::hex << hr << std::dec << std::endl;
            return;
        }

        mipChains[slice] = std::move(mipChain);
    }

    // ============================================================
    // PHASE 3: UPLOAD MIP LEVELS
    // ============================================================

    CD3DX12_RESOURCE_BARRIER toCopyDst = CD3DX12_RESOURCE_BARRIER::Transition(
        textureResource.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    cmd->ResourceBarrier(1, &toCopyDst);

    for (UINT slice = 0; slice < arraySize; ++slice)
    {
        for (UINT mip = 1; mip < mipLevels; ++mip)
        {
            UINT sub = D3D12CalcSubresource(mip, slice, 0, mipLevels, arraySize);
            const DirectX::Image* img = mipChains[slice].GetImage(mip, 0, 0);

            // Mip-specific resource desc
            D3D12_RESOURCE_DESC mipDesc = desc;
            mipDesc.Width = std::max<UINT64>(1, desc.Width >> mip);
            mipDesc.Height = std::max<UINT64>(1, desc.Height >> mip);
            mipDesc.MipLevels = 1;

            D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp;
            UINT rows;
            UINT64 rowBytes, total;
            device->GetCopyableFootprints(&mipDesc, 0, 1, 0, &fp, &rows, &rowBytes, &total);

            ComPointer<ID3D12Resource> upload;
            CD3DX12_HEAP_PROPERTIES heapUpload(D3D12_HEAP_TYPE_UPLOAD);
            CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(total);
            device->CreateCommittedResource(&heapUpload, D3D12_HEAP_FLAG_NONE,
                &bufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr, IID_PPV_ARGS(&upload));
            tempResources.push_back(upload);

            void* mapped;
            upload->Map(0, nullptr, &mapped);

            for (UINT row = 0; row < rows; ++row)
            {
                memcpy(static_cast<uint8_t*>(mapped) + row * fp.Footprint.RowPitch,
                    img->pixels + row * img->rowPitch,
                    static_cast<size_t>(rowBytes));
            }

            upload->Unmap(0, nullptr);

            CD3DX12_TEXTURE_COPY_LOCATION dst(textureResource.Get(), sub);
            CD3DX12_TEXTURE_COPY_LOCATION src(upload.Get(), fp);
            cmd->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        }
    }

    CD3DX12_RESOURCE_BARRIER toSRV2 = CD3DX12_RESOURCE_BARRIER::Transition(
        textureResource.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
    cmd->ResourceBarrier(1, &toSRV2);

    context->executeCommandList(renderPipeline->getCommandListID());
    context->signalAndWait();
    context->resetCommandList(renderPipeline->getCommandListID());

    tempResources.clear();
}

void Texture::releaseResources() {
    if (textureResource) {
        textureResource.Release();
    }

    if (textureUploadHeap) {
        textureUploadHeap.Release();
    }
}