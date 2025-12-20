#include "DescriptorHeap.h"

#include <cassert>
#include <stdexcept>

DescriptorHeap::DescriptorHeap(DXContext &context, D3D12_DESCRIPTOR_HEAP_TYPE type, unsigned int numberOfDescriptors, D3D12_DESCRIPTOR_HEAP_FLAGS flags)
	: descriptorCount(numberOfDescriptors)
{
	ComPointer<ID3D12Device6> device = context.getDevice();
	D3D12_DESCRIPTOR_HEAP_DESC description = {};
	description.NumDescriptors = numberOfDescriptors;
	description.Type = type;
	description.Flags = flags;

	if FAILED((device->CreateDescriptorHeap(&description, IID_PPV_ARGS(&descriptorHeap)))) {
		throw std::runtime_error("Could not create descriptor heap");
	};

#if defined(_DEBUG)
	descriptorHeap->SetName(L"RendererHeap");
#endif

	descriptorSize = device->GetDescriptorHandleIncrementSize(type);

	cpuStart = get()->GetCPUDescriptorHandleForHeapStart();
	if (type != D3D12_DESCRIPTOR_HEAP_TYPE_DSV && type != D3D12_DESCRIPTOR_HEAP_TYPE_RTV) {
		gpuStart = get()->GetGPUDescriptorHandleForHeapStart();
	}

	freeIndices.reserve(numberOfDescriptors);
	for (UINT n = numberOfDescriptors; n > 0; n--)
		freeIndices.push_back(n - 1);
}

ComPointer<ID3D12DescriptorHeap>& DescriptorHeap::get()
{
	return descriptorHeap;
}

ID3D12DescriptorHeap* DescriptorHeap::getAddress()
{
	return descriptorHeap.Get();
}

unsigned int DescriptorHeap::getDescriptorSize()
{
	return descriptorSize;
}

bool DescriptorHeap::allocate(D3D12_CPU_DESCRIPTOR_HANDLE& outCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE& outGpuHandle)
{
	if (freeIndices.empty())
		return false;

	UINT idx = freeIndices.back();
	freeIndices.pop_back();

	outCpuHandle.ptr = cpuStart.ptr + idx * descriptorSize;
	outGpuHandle.ptr = gpuStart.ptr + idx * descriptorSize;

	return true;
}

bool DescriptorHeap::allocate(D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandlePtr, D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandlePtr)
{
	if (freeIndices.empty())
		return false;

	UINT idx = freeIndices.back();
	freeIndices.pop_back();

	outCpuHandlePtr->ptr = cpuStart.ptr + idx * descriptorSize;
	outGpuHandlePtr->ptr = gpuStart.ptr + idx * descriptorSize;

	return true;
}

void DescriptorHeap::free(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
	int cpuIdx = (int)((cpuHandle.ptr - cpuStart.ptr) / descriptorSize);
	int gpuIdx = (int)((gpuHandle.ptr - gpuStart.ptr) / descriptorSize);
	assert(cpuIdx == gpuIdx);
	freeIndices.push_back(cpuIdx);
}

