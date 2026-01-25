#pragma once

#include <vector>
#include "Support/WinInclude.h"
#include "Support/ComPointer.h"
#include "D3D/DXContext.h"

class IndexBuffer {
public:
	IndexBuffer() = default;
	IndexBuffer(std::vector<unsigned int> indexData, const UINT indexDataSize);

	D3D12_INDEX_BUFFER_VIEW passIndexDataToGPU(DXContext& context, ID3D12GraphicsCommandList6* cmdList);

	ComPointer<ID3D12Resource1>& getUploadBuffer();
	ComPointer<ID3D12Resource1>& getIndexBuffer();

	void transitionState(ID3D12GraphicsCommandList6* cmdList, D3D12_RESOURCE_STATES state) {
		D3D12_RESOURCE_BARRIER barrier = {
			.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
			.Transition = {
				.pResource = indexBuffer,
				.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
				.StateAfter = state
			}
		};

		cmdList->ResourceBarrier(1, &barrier);
	}

	void releaseResources();

private:
	ComPointer<ID3D12Resource1> uploadBuffer;
	ComPointer<ID3D12Resource1> indexBuffer;

	UINT indexDataSize{ 0 };
	std::vector<unsigned int> indexData;
};