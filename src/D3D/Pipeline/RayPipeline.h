#pragma once

#include "Pipeline.h"

#include "Support/ShaderLib.h"

class RayPipeline : public Pipeline {
public:
	RayPipeline() = delete;
	RayPipeline(DXContext& context, CommandListID id, DescriptorHeap* dh);

	void releaseResources() override;

	ID3D12Resource* getShaderIds() { return shaderIds; }

	ID3D12StateObject* getStateObject() { return pso; }

protected:
	void createRootSignature(DXContext& context, std::vector<Shader*> shaders) override;

	void generateRootSignature(DXContext& context, Shader& vertexShader, Shader& pixelShader);

	void createPSOD() override;

	void createPipelineState(ComPointer<ID3D12Device6>& device) override;

	void initShaderTables();

	ShaderLib shaderLib;
	UINT shaderCount{ 3 };
	ID3D12Resource* shaderIds;

	ID3D12StateObject* pso;
};
