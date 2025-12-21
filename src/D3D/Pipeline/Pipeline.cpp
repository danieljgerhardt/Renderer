#include "Pipeline.h"

Pipeline::Pipeline(DXContext& context, CommandListID cmdID)
	: cmdID(cmdID), cmdList(context.createCommandList(cmdID))
{
  	context.resetCommandList(cmdID);
}

ComPointer<ID3D12RootSignature>& Pipeline::getRootSignature() {
	return this->rootSignature;
}

DescriptorHeap* Pipeline::getDescriptorHeap() {
	return descriptorHeap;
}

void Pipeline::releaseResources() {
	rootSignature.Release();
}