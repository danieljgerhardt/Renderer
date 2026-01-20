#pragma once

#include <vector>
#include "Support/WinInclude.h"
#include "Support/ComPointer.h"
#include "D3D/DXContext.h"
#include "DirectXMath.h"

#include "Scene/Util/Vertex.h"

using namespace DirectX;

class VertexBuffer {
public:
	VertexBuffer() = default;
	VertexBuffer(std::vector<Vertex>& vertexData, UINT vertexDataSize, UINT vertexDataStride);

	D3D12_VERTEX_BUFFER_VIEW passVertexDataToGPU(DXContext& context, ID3D12GraphicsCommandList6* cmdList);

	ComPointer<ID3D12Resource1>& getUploadBuffer();
	ComPointer<ID3D12Resource1>& getVertexBuffer();

	void releaseResources();

	UINT getVertexDataSize() const { return vertexDataSize; }
	UINT getVertexDataStride() const { return vertexDataStride; }

	void transitionState(ID3D12GraphicsCommandList6* cmdList, D3D12_RESOURCE_STATES state) {
		D3D12_RESOURCE_BARRIER barrier = {
			.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
			.Transition = {
				.pResource = vertexBuffer,
				.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
				.StateAfter = state
			}
		};

		cmdList->ResourceBarrier(1, &barrier);
	}

private:
	ComPointer<ID3D12Resource1> uploadBuffer;
	ComPointer<ID3D12Resource1> vertexBuffer;

	UINT vertexDataSize;
	UINT vertexDataStride;
	std::vector<Vertex> vertexData;
};