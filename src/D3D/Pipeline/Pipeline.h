#pragma once

#include "../../Support/WinInclude.h"
#include "../../Support/Shader.h"
#include "../../Support/ComPointer.h"
#include "../../Support/Window.h"
#include "../DescriptorHeap.h"
#include "Support/RootSignatureBuilder.h"

class Pipeline {
public:
	Pipeline() = delete;
	Pipeline(DXContext& context, CommandListID cmdID);
	~Pipeline() = default;

	ComPointer<ID3D12RootSignature>& getRootSignature();
	DescriptorHeap* getDescriptorHeap();
	ComPointer<ID3D12PipelineState>& getPSO() { return pso; }
	ID3D12GraphicsCommandList6* getCommandList() { return cmdList; }
	CommandListID getCommandListID() { return cmdID; }

	virtual void releaseResources();

protected:
	virtual void createPSOD() = 0;
	virtual void createPipelineState(ComPointer<ID3D12Device6>& device) = 0;
	virtual void createRootSignature(DXContext& context, std::vector<Shader*> shaders) = 0;

	ComPointer<ID3D12RootSignature> rootSignature;

	DescriptorHeap* descriptorHeap;
	UINT numDescriptors{ 0 };

	ComPointer<ID3D12PipelineState> pso;

	CommandListID cmdID;

	ID3D12GraphicsCommandList6* cmdList;
};