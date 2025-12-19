#pragma once
#include <d3d12.h>
#include "./includes/d3dx12.h"
#include <wrl.h>
#include "DXContext.h"

// Sourced from https://github.com/stefanpgd/Compute-DirectX12-Tutorial/

class DescriptorHeap
{
public:
	DescriptorHeap(DXContext &context, D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int numberOfDescriptors,
		D3D12_DESCRIPTOR_HEAP_FLAGS flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	ComPointer<ID3D12DescriptorHeap>& get();
	ID3D12DescriptorHeap* getAddress();
	
	unsigned int getDescriptorSize();

	bool allocate(D3D12_CPU_DESCRIPTOR_HANDLE& outCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE& outGpuHandle);
	bool allocate(D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandlePtr, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandlePtr);
	void free(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

	void releaseResources() { 
		descriptorHeap.Release();
	}

private:
	ComPointer<ID3D12DescriptorHeap> descriptorHeap;

	unsigned int descriptorSize;
	unsigned int descriptorCount;
	std::vector<UINT> freeIndices;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuStart{ 0 };
	D3D12_GPU_DESCRIPTOR_HANDLE gpuStart{ 0 };
};