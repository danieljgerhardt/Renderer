#pragma once

#include <vector>
#include "Support/WinInclude.h"
#include "Support/ComPointer.h"
#include "D3D/DXContext.h"
#include "D3D/Pipeline/Pipeline.h"
#include "DirectXMath.h"

using namespace DirectX;

enum class BufferType {
	CBV,
	SRV,
	UAV
};

class StructuredBuffer {
public:
	StructuredBuffer() = default;

	StructuredBuffer(const void* data, unsigned int numEle, UINT eleSize);

	void copyDataFromGPU(DXContext& context, void* outputData, ID3D12GraphicsCommandList6* cmdList, D3D12_RESOURCE_STATES state, CommandListID cmdId);

	ComPointer<ID3D12Resource1>& getBuffer();

	CD3DX12_CPU_DESCRIPTOR_HANDLE getUavCpuDescriptorHandle();
	CD3DX12_CPU_DESCRIPTOR_HANDLE getSrvCpuDescriptorHandle();
	CD3DX12_CPU_DESCRIPTOR_HANDLE getCbvCpuDescriptorHandle();

	CD3DX12_GPU_DESCRIPTOR_HANDLE getUavGpuDescriptorHandle();
	CD3DX12_GPU_DESCRIPTOR_HANDLE getSrvGpuDescriptorHandle();
	CD3DX12_GPU_DESCRIPTOR_HANDLE getCbvGpuDescriptorHandle();

	D3D12_GPU_VIRTUAL_ADDRESS getGpuVirtualAddress();

	unsigned int getNumElements() { return numElements; }

	size_t getElementSize() { return elementSize; }

	void passCbvDataToGpu(DXContext& context, DescriptorHeap* dh);
	void passDataToGpu(DXContext& context, ID3D12GraphicsCommandList6* cmdList, CommandListID id);
	void createUav(DXContext& context, DescriptorHeap* dh);
	void createSrv(DXContext& context, DescriptorHeap* dh);

	void releaseResources();

private:
	ComPointer<ID3D12Resource1> buffer;

	CD3DX12_CPU_DESCRIPTOR_HANDLE uavCpuHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvCpuHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE cbvCpuHandle;

	CD3DX12_GPU_DESCRIPTOR_HANDLE uavGpuHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvGpuHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE cbvGpuHandle;

	bool isCbv = false;
	bool isUav = false;
	bool isSrv = false;

	const void* data;
	unsigned int numElements;
	UINT elementSize;
};