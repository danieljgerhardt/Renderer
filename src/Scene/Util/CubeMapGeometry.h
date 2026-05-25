#pragma once

#include "Support/WinInclude.h"
#include "DirectXMath.h"

#include "D3D/ResourceManager.h"
#include "D3D/Pipeline/RenderPipeline.h"

inline void fillCubemapViewMatrices(std::array<XMMATRIX, 6>& viewMatrices) {
	viewMatrices[0] = XMMatrixLookToLH(XMVectorZero(), XMVectorSet(1.f, 0.f, 0.f, 0.f), XMVectorSet(0.f, -1.f, 0.f, 0.f));   // +X
	viewMatrices[1] = XMMatrixLookToLH(XMVectorZero(), XMVectorSet(-1.f, 0.f, 0.f, 0.f), XMVectorSet(0.f, -1.f, 0.f, 0.f));  // -X
	viewMatrices[2] = XMMatrixLookToLH(XMVectorZero(), XMVectorSet(0.f, 1.f, 0.f, 0.f), XMVectorSet(0.f, 0.f, -1.f, 0.f));    // +Y
	viewMatrices[3] = XMMatrixLookToLH(XMVectorZero(), XMVectorSet(0.f, -1.f, 0.f, 0.f), XMVectorSet(0.f, 0.f, 1.f, 0.f));  // -Y
	viewMatrices[4] = XMMatrixLookToLH(XMVectorZero(), XMVectorSet(0.f, 0.f, -1.f, 0.f), XMVectorSet(0.f, -1.f, 0.f, 0.f));   // +Z
	viewMatrices[5] = XMMatrixLookToLH(XMVectorZero(), XMVectorSet(0.f, 0.f, 1.f, 0.f), XMVectorSet(0.f, -1.f, 0.f, 0.f));  // -Z
}

inline void createCubeGeometry(DXContext* context, RenderPipeline* renderPipeline, D3D12_INDEX_BUFFER_VIEW& outIbv, D3D12_VERTEX_BUFFER_VIEW& outVbv) {
    ResourceManager& rm = ResourceManager::get();

    std::vector<Vertex> vertData{
        {XMFLOAT3(-1.f, -1.f, -1.f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(0.f, 1.f)},
        {XMFLOAT3(1.f,  -1.f, -1.f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(0.f, 0.f)},
        {XMFLOAT3(1.f,  1.f, -1.f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(1.f, 0.f)},
        {XMFLOAT3(-1.f, 1.f, -1.f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(1.f, 1.f)},
        {XMFLOAT3(-1.f, -1.f, 1.f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(0.f, 1.f)},
        {XMFLOAT3(1.f, -1.f, 1.f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(0.f, 0.f)},
        {XMFLOAT3(1.f,  1.f, 1.f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(1.f, 0.f)},
        {XMFLOAT3(-1.f, 1.f, 1.f), XMFLOAT3(0.f, 0.f, -1.f), XMFLOAT2(1.f, 1.f)}
    };
    ResourceHandle vbHandle = rm.createVertexBuffer(vertData.data(), (UINT)(vertData.size() * sizeof(Vertex)), (UINT)sizeof(Vertex));

    std::vector<UINT> indexData{
        1, 0, 3, 1, 3, 2,
        4, 5, 6, 4, 6, 7,
        5, 1, 2, 5, 2, 6,
        7, 6, 2, 7, 2, 3,
        0, 4, 7, 0, 7, 3,
        0, 1, 5, 0, 5, 4
    };
    ResourceHandle ibHandle = rm.createIndexBuffer(indexData, (UINT)(indexData.size() * sizeof(UINT)));

    outVbv = rm.getVertexBuffer(vbHandle)->passVertexDataToGPU(*context, renderPipeline->getCommandList());
    outIbv = rm.getIndexBuffer(ibHandle)->passIndexDataToGPU(*context, renderPipeline->getCommandList());

    //Transition both buffers to their usable states
    D3D12_RESOURCE_BARRIER barriers[2] = {};

    // Vertex buffer barrier
    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[0].Transition.pResource = rm.getVertexBuffer(vbHandle)->getVertexBuffer().Get();
    barriers[0].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[0].Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    barriers[0].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    // Index buffer barrier
    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barriers[1].Transition.pResource = rm.getIndexBuffer(ibHandle)->getIndexBuffer().Get();
    barriers[1].Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barriers[1].Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
    barriers[1].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    renderPipeline->getCommandList()->ResourceBarrier(2, barriers);
}